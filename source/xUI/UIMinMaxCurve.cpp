#include "UIMinMaxCurve.h"
#include "CurvePreviewGraphNodeData.h"
#include <xcommon/IPluginManager.h>

CUIMinMaxCurve::CUIMinMaxCurve(IXCore *pCore, IXRender *pRender, ULONG uID):
	BaseClass(uID, "select"),
	m_pCore(pCore),
	m_pRender(pRender),
	m_editCallback(this)
{
	m_szTextureName[0] = '!';

	if(!m_pRender->getGraph("xCurvePreview", &m_pRenderGraph))
	{
		LogFatal("Unable to load 'xCurvePreview' render graph!\n");
	}

	m_pRender->newCamera(&m_pCamera);
	m_pCamera->setProjectionMode(XCPM_ORTHOGONAL);

	//m_curve.setMode(XMCM_TWO_CURVES);

	//IXAnimationCurve *pCurve = m_curve.getMaxCurve();


	/*pCurve->addKey(0.20512706f, 0.3297542f);
	pCurve->addKey(0.7526245f, 0.5924988f);*/
	/*
	pCurve->addKey(0.0f, 0.608581543f);
	pCurve->addKey(0.162895843f, 0.6514799f);
	pCurve->addKey(1.0f, 0.5397474f);

	XKeyframe kf = *pCurve->getKeyAt(0);
	kf.fInTangent = 1.65203309f;
	kf.fOutTangent = 1.65203309f;
	pCurve->setKey(0, kf);

	kf = *pCurve->getKeyAt(1);
	//kf.fInTangent = 1.13566625f;
	kf.fInTangent = INFINITY;
	kf.fOutTangent = 1.13566625f;
	pCurve->setKey(1, kf);

	kf = *pCurve->getKeyAt(2);
	kf.fInTangent = -0.6619926f;
	kf.fOutTangent = -0.6619926f;
	pCurve->setKey(2, kf);

	pCurve = m_curve.getMinCurve();
	pCurve->addKey(0.0f, 0.0f);
	pCurve->addKey(1.0f, 1.0f);

	kf = *pCurve->getKeyAt(0);
	kf.fOutTangent = 0.0f;
	kf.fOutWeight = 0.5;
	kf.weightedMode = XKWM_OUT;
	pCurve->setKey(0, kf);

	kf = *pCurve->getKeyAt(1);
	//kf.fInTangent = 1.0f;
	kf.fInTangent = 0.0f;
	kf.fInWeight = 1.0;
	kf.weightedMode = XKWM_IN;
	pCurve->setKey(1, kf);*/
}

CUIMinMaxCurve::~CUIMinMaxCurve()
{
	abortEdit();

	mem_release(m_pRenderGraph);
	mem_release(m_pRenderTarget);
	mem_release(m_pCamera);
}

gui::dom::IDOMnode* CUIMinMaxCurve::createNode(gui::dom::IDOMdocument *pDomDocument)
{
	//m_pNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	//m_pNode->setAttribute(L"controld_id", StringW(m_id));
	//return(m_pNode);

	m_pNode = pDomDocument->createNode(L"div");
	m_pNode->classAdd(L"combobox-wrapper minmaxcurve-wrapper");
	m_pNode->setAttribute(L"onchange", L"handler");
	m_pNode->setUserData(this);
	m_pLabel = pDomDocument->createNode(L"label");
	m_pInputNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	m_pInputNode->setAttribute(L"onchange", L"handler");
	m_pInputNode->setUserData(this);

	StringW wsId = StringW(L"select_") + StringW(m_id);

	m_pInputNode->setAttribute(L"id", StringW(m_id));
	m_pLabel->setAttribute(L"for", StringW(m_id));
	
	m_pNode->appendChild(m_pLabel, true);
	m_pNode->appendChild(m_pInputNode, true);

	addItem("Constant", "0");
	addItem("Curve", "1");
	addItem("Random between two constants", "2");
	addItem("Random between two curves", "3");

	m_pInputNode->classAdd(L"opt-0");

	m_pConstantTextbox = pDomDocument->createNode(L"input");
	m_pConstantTextbox->classAdd(L"inner");
	m_pConstantTextbox->classAdd(L"opt-0");
	m_pConstantTextbox->setAttribute(L"onchange", L"handler");
	m_pConstantTextbox->setAttribute(L"onkeyup", L"handler");
	m_pConstantTextbox->setUserData(this);
	m_pInputNode->appendChild(m_pConstantTextbox);

	m_pTwoConstantsTextbox0 = pDomDocument->createNode(L"input");
	m_pTwoConstantsTextbox0->classAdd(L"inner");
	m_pTwoConstantsTextbox0->classAdd(L"opt-2");
	m_pTwoConstantsTextbox0->classAdd(L"part-0");
	m_pTwoConstantsTextbox0->setAttribute(L"onchange", L"handler");
	m_pTwoConstantsTextbox0->setAttribute(L"onkeyup", L"handler");
	m_pTwoConstantsTextbox0->setUserData(this);
	m_pInputNode->appendChild(m_pTwoConstantsTextbox0);

	m_pTwoConstantsTextbox1 = pDomDocument->createNode(L"input");
	m_pTwoConstantsTextbox1->classAdd(L"inner");
	m_pTwoConstantsTextbox1->classAdd(L"opt-2");
	m_pTwoConstantsTextbox1->classAdd(L"part-1");
	m_pTwoConstantsTextbox1->setAttribute(L"onchange", L"handler");
	m_pTwoConstantsTextbox1->setAttribute(L"onkeyup", L"handler");
	m_pTwoConstantsTextbox1->setUserData(this);
	m_pInputNode->appendChild(m_pTwoConstantsTextbox1);

	m_pCurvePreview = pDomDocument->createNode(L"div");
	m_pCurvePreview->classAdd(L"inner");
	m_pCurvePreview->classAdd(L"opt-1");
	m_pCurvePreview->classAdd(L"opt-3");
	m_pCurvePreview->setAttribute(L"onclick", L"handler");
	m_pCurvePreview->setAttribute(L"onlayout", L"handler");
	//m_pCurvePreview->getStyleSelf()->background_image->set(L"!dev_null");
	m_pCurvePreview->setUserData(this);
	m_pInputNode->appendChild(m_pCurvePreview);

	return(m_pNode);
}

void XMETHODCALLTYPE CUIMinMaxCurve::setLabel(const char *szTitle)
{
	m_pLabel->setText(StringW(CMB2WC(szTitle)), true);
}

void CUIMinMaxCurve::cleanupNodes()
{
	BaseClass::cleanupNodes();
	m_pLabel = NULL;

	m_pConstantTextbox = NULL;
	m_pTwoConstantsTextbox0 = NULL;
	m_pTwoConstantsTextbox1 = NULL;
	m_pCurvePreview = NULL;
}

void CUIMinMaxCurve::addItem(const char *szName, const char *szValue)
{
	m_pInputNode->appendHTML(StringW(L"<option value=\"")+CMB2WC(szValue)+L"\">"+CMB2WC(szName)+L"</option>");
}
/*void XMETHODCALLTYPE CUIMinMaxCurve::removeItems()
{
	while(m_pInputNode->getChilds()->size() > 1)
	{
		m_pInputNode->removeChild(m_pInputNode->getChilds()[0][1]);
	}
}*/

/*void XMETHODCALLTYPE CUIMinMaxCurve::setValue(const char *szValue)
{
	m_pInputNode->setAttribute(L"value", StringW(CMB2WC(szValue)));
}
const char* XMETHODCALLTYPE CUIMinMaxCurve::getValue()
{
	m_sValue = m_pInputNode->getAttribute(L"value");
	return(m_sValue.c_str());
}*/

void CUIMinMaxCurve::dispatchEvent(gui::IEvent *ev)
{
	if(ev->type == gui::GUI_EVENT_TYPE_CHANGE)
	{
		if(ev->target == m_pInputNode)
		{
			const StringW &wsOption = ev->relatedTarget->getAttribute(L"value");
			XMINMAX_CURVE_MODE mode = (XMINMAX_CURVE_MODE)wsOption.toInt();
			setMode(mode);
		}
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_LAYOUT && ev->target == m_pCurvePreview)
	{
		UINT uW = m_pCurvePreview->getInnerWidth();
		UINT uH = m_pCurvePreview->getInnerHeight();

		//LogInfo("Resized: %ux%u\n", uW, uH);

		if(uW != m_uRenderWidth || uH != m_uRenderHeight)
		{
			m_uRenderWidth = uW;
			m_uRenderHeight = uH;
			if(m_pRenderTarget)
			{
				m_pRenderTarget->resize(m_uRenderWidth, m_uRenderHeight);
				m_pCamera->setPosition(float3(m_uRenderWidth * 0.5f, m_uRenderHeight * 0.5f, -10.0f));
				m_pCurvePreview->getStyleSelf()->background_image->set(CMB2WC(m_szTextureName));
				m_pCurvePreview->updateStyles();

				updatePeview();
			}
		}
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_CLICK && ev->target == m_pCurvePreview)
	{
		startEdit();
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_KEYUP)
	{
		const StringW &wsText = ev->target->getText();
		float fValue = 0.0f;
		int iParsed = swscanf(wsText.c_str(), L"%g", &fValue);
		TODO("setValid(iParsed != 0);");
		if(iParsed > 0)
		{
			if(ev->target == m_pConstantTextbox || ev->target == m_pTwoConstantsTextbox1)
			{
				m_pCurve->setMax(fValue);
			}
			else if(ev->target == m_pTwoConstantsTextbox0)
			{
				m_pCurve->setMin(fValue);
			}

			gui::IEvent ev2;
			ev2.type = gui::GUI_EVENT_TYPE_CHANGE;
			ev2.target = m_pNode;
			ev2.relatedTarget = ev->target;
			m_pNode->dispatchEvent(ev2);
		}
	}
}

void CUIMinMaxCurve::startEdit()
{
	if(!m_pCurveEditor)
	{
		m_pCurveEditor = (IXCurveEditor*)m_pCore->getPluginManager()->getInterface(IXCURVEEDITOR_GUID);
	}

	if(!m_pCurveEditor)
	{
		LogError("Interface IXCurveEditor is not available!\n");
		return;
	}

	m_pCurveEditor->edit(m_pCurve, &m_editCallback);
	m_isEditing = true;
}

void CUIMinMaxCurve::abortEdit()
{
	if(m_isEditing)
	{
		m_pCurveEditor->abort();
		m_isEditing = false;
	}
}

void CUIMinMaxCurve::onEditAccept()
{
	m_isEditing = false;
	updatePeview();

	gui::IEvent ev;
	ev.type = gui::GUI_EVENT_TYPE_CHANGE;
	ev.target = m_pNode;
	ev.relatedTarget = m_pCurvePreview;
	m_pNode->dispatchEvent(ev);
}
void CUIMinMaxCurve::onEditCancel()
{
	m_isEditing = false;
	updatePeview();
}

void CUIMinMaxCurve::updatePeview()
{
	if(m_pRenderTarget)
	{
		IXRenderGraphNodeInstance *pNodeInstance = NULL;
		if(!m_pRenderGraph->getNodeInstance("node", &pNodeInstance))
		{
			LogFatal("Unable to find node 'node' in 'xCurveEditor' render graph!\n");
		}

		IXRenderGraphNodeData *pNodeData = NULL;
		if(pNodeInstance->getNodeData(m_pRenderTarget, &pNodeData))
		{
			CCurvePreviewGraphNodeData *pInternalNodeData = (CCurvePreviewGraphNodeData*)pNodeData;

			pInternalNodeData->updateLine(m_pCurve);

			mem_release(pNodeData);
		}

		mem_release(pNodeInstance);
	}
}

void XMETHODCALLTYPE CUIMinMaxCurve::setEnabled(bool bEnable)
{
	BaseClass::setEnabled(bEnable);

	if(bEnable)
	{
		m_pCurvePreview->removePseudoclass(0x00004);
		m_pConstantTextbox->removePseudoclass(0x00004);
		m_pTwoConstantsTextbox0->removePseudoclass(0x00004);
		m_pTwoConstantsTextbox1->removePseudoclass(0x00004);
	}
	else
	{
		m_pCurvePreview->addPseudoclass(0x00004);
		m_pConstantTextbox->addPseudoclass(0x00004);
		m_pTwoConstantsTextbox0->addPseudoclass(0x00004);
		m_pTwoConstantsTextbox1->addPseudoclass(0x00004);
	}
}

void XMETHODCALLTYPE CUIMinMaxCurve::setCurve(IXMinMaxCurve *pCurve)
{
	if(pCurve)
	{
		m_pCurve = pCurve;
		setMode(pCurve->getMode());

		m_pConstantTextbox->setText(pCurve->getMax(), TRUE);
		m_pTwoConstantsTextbox0->setText(pCurve->getMin(), TRUE);
		m_pTwoConstantsTextbox1->setText(m_pConstantTextbox->getText(), TRUE);

		updatePeview();
	}
	else
	{
		m_pCurve = &m_curve;
	}
}

void CUIMinMaxCurve::setMode(XMINMAX_CURVE_MODE mode)
{
	m_pInputNode->setAttribute(L"class", StringW(L"opt-") + mode);

	bool isEditing = m_isEditing;
	abortEdit();

	m_pCurve->setMode(mode);
	switch(mode)
	{
	case XMCM_CONSTANT:
	case XMCM_TWO_CONSTANTS:
		mem_release(m_pRenderTarget);
		break;

	case XMCM_CURVE:
	case XMCM_TWO_CURVES:
		if(SMIsZero(m_pCurve->getMax() - m_pCurve->getMin()))
		{
			m_pCurve->setMax(m_pCurve->getMin() + 1.0f);
		}

		if(!m_pRenderTarget)
		{
			m_pRender->newTextureTarget(&m_pRenderTarget, m_szTextureName + 1, sizeof(m_szTextureName) - 1);
			m_pRenderTarget->setCamera(m_pCamera);
			m_pRenderTarget->attachGraph(m_pRenderGraph);

			if(m_uRenderWidth && m_uRenderHeight)
			{
				m_pRenderTarget->resize(m_uRenderWidth, m_uRenderHeight);
				m_pCamera->setPosition(float3(m_uRenderWidth * 0.5f, m_uRenderHeight * 0.5f, -10.0f));
				m_pCurvePreview->getStyleSelf()->background_image->set(CMB2WC(m_szTextureName));
				m_pCurvePreview->updateStyles();
			}
		}

		updatePeview();

		if(isEditing)
		{
			startEdit();
		}
		break;
	}

	gui::IEvent ev;
	ev.type = gui::GUI_EVENT_TYPE_CHANGE;
	ev.target = m_pNode;
	ev.relatedTarget = m_pInputNode;
	m_pNode->dispatchEvent(ev);
}

void XMETHODCALLTYPE CUIMinMaxCurve::setVisible(bool isVisible)
{
	if(isVisible)
	{
		setCurve(m_pCurve);
	}
	BaseClass::setVisible(isVisible);
}

//##########################################################################

CCurveEditorCallback::CCurveEditorCallback(CUIMinMaxCurve *pControl):
	m_pControl(pControl)
{
}

void XMETHODCALLTYPE CCurveEditorCallback::onAccept()
{
	m_pControl->onEditAccept();
}
void XMETHODCALLTYPE CCurveEditorCallback::onCancel()
{
	m_pControl->onEditCancel();
}
