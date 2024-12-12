#include "UI2ColorGradient.h"
#include "GradientPreviewGraphNodeData.h"
#include <xcommon/IPluginManager.h>

CUI2ColorGradient::CUI2ColorGradient(IXCore *pCore, IXRender *pRender, ULONG uID):
	BaseClass(uID, "select"),
	m_pCore(pCore),
	m_pRender(pRender),
	m_editCallback(this),
	m_editColorCallback(this)
{
	m_szTextureName[0] = '!';

	if(!m_pRender->getGraph("xGradientPreview", &m_pRenderGraph))
	{
		LogFatal("Unable to load 'xGradientPreview' render graph!\n");
	}

	m_pGradient->getGradient0()->setColorKey(0, {float3_t(1.0f, 0.0f, 0.0f), 0.0f});
	m_pGradient->getGradient1()->setColorKey(1, {float3_t(0.0f, 1.0f, 0.0f), 1.0f});

	m_pRender->newCamera(&m_pCamera);
	m_pCamera->setProjectionMode(XCPM_ORTHOGONAL);
}

CUI2ColorGradient::~CUI2ColorGradient()
{
	abortEdit();
	abortEditColor();

	mem_release(m_pRenderGraph);
	mem_release(m_pRenderTarget);
	mem_release(m_pCamera);
}

gui::dom::IDOMnode* CUI2ColorGradient::createNode(gui::dom::IDOMdocument *pDomDocument)
{
	//m_pNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	//m_pNode->setAttribute(L"controld_id", StringW(m_id));
	//return(m_pNode);

	m_pNode = pDomDocument->createNode(L"div");
	m_pNode->classAdd(L"combobox-wrapper twogradients-wrapper");
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

	addItem("Color", "0");
	addItem("Gradient", "1");
	addItem("Random between two colors", "2");
	addItem("Random between two gradients", "3");

	m_pInputNode->classAdd(L"opt-0");

	m_pColor0Preview = pDomDocument->createNode(L"div");
	m_pColor0Preview->classAdd(L"inner");
	m_pColor0Preview->classAdd(L"opt-0");
	m_pColor0Preview->classAdd(L"opt-2");
	m_pColor0Preview->classAdd(L"part-0");
	m_pColor0Preview->setAttribute(L"onclick", L"handler");
	m_pColor0Preview->getStyleSelf()->background_color->set((int)0xFF0000FF);
	m_pColor0Preview->setUserData(this);
	m_pInputNode->appendChild(m_pColor0Preview);

	m_pColor0Textbox = pDomDocument->createNode(L"input");
	m_pColor0Textbox->classAdd(L"inner");
	m_pColor0Textbox->classAdd(L"opt-0");
	m_pColor0Textbox->classAdd(L"opt-2");
	m_pColor0Textbox->classAdd(L"part-1");
	m_pColor0Textbox->setAttribute(L"onchange", L"handler");
	m_pColor0Textbox->setAttribute(L"onkeyup", L"handler");
	m_pColor0Textbox->setUserData(this);
	m_pInputNode->appendChild(m_pColor0Textbox);

	m_pColor1Preview = pDomDocument->createNode(L"div");
	m_pColor1Preview->classAdd(L"inner");
	m_pColor1Preview->classAdd(L"opt-2");
	m_pColor1Preview->classAdd(L"part-0");
	m_pColor1Preview->classAdd(L"row-2");
	m_pColor1Preview->setAttribute(L"onclick", L"handler");
	m_pColor1Preview->getStyleSelf()->background_color->set((int)0x00FF00FF);
	m_pColor1Preview->setUserData(this);
	m_pInputNode->appendChild(m_pColor1Preview);

	m_pColor1Textbox = pDomDocument->createNode(L"input");
	m_pColor1Textbox->classAdd(L"inner");
	m_pColor1Textbox->classAdd(L"opt-2");
	m_pColor1Textbox->classAdd(L"part-1");
	m_pColor1Textbox->classAdd(L"row-2");
	m_pColor1Textbox->setAttribute(L"onchange", L"handler");
	m_pColor1Textbox->setAttribute(L"onkeyup", L"handler");
	m_pColor1Textbox->setUserData(this);
	m_pInputNode->appendChild(m_pColor1Textbox);


	m_pGradientPreview = pDomDocument->createNode(L"div");
	m_pGradientPreview->classAdd(L"inner");
	m_pGradientPreview->classAdd(L"opt-1");
	m_pGradientPreview->classAdd(L"opt-3");
	m_pGradientPreview->setAttribute(L"onclick", L"handler");
	m_pGradientPreview->setAttribute(L"onlayout", L"handler");
	//m_pGradientPreview->getStyleSelf()->background_image->set(L"!dev_null");
	m_pGradientPreview->setUserData(this);
	m_pInputNode->appendChild(m_pGradientPreview);

	// m_pGradientPreview1 = pDomDocument->createNode(L"div");
	// m_pGradientPreview1->classAdd(L"inner");
	// m_pGradientPreview1->classAdd(L"opt-3");
	// m_pGradientPreview1->classAdd(L"row-2");
	// m_pGradientPreview1->setAttribute(L"onclick", L"handler");
	// m_pGradientPreview1->setAttribute(L"onlayout", L"handler");
	// m_pGradientPreview1->getStyleSelf()->background_image->set(L"!dev_null");
	// m_pGradientPreview1->setUserData(this);
	// m_pInputNode->appendChild(m_pGradientPreview1);

	return(m_pNode);
}

void XMETHODCALLTYPE CUI2ColorGradient::setLabel(const char *szTitle)
{
	m_pLabel->setText(StringW(CMB2WC(szTitle)), true);
}

void CUI2ColorGradient::cleanupNodes()
{
	BaseClass::cleanupNodes();
	m_pLabel = NULL;

	m_pColor0Textbox = NULL;
	m_pColor0Preview = NULL;
	m_pColor1Textbox = NULL;
	m_pColor1Preview = NULL;
	m_pGradientPreview = NULL;
}

void CUI2ColorGradient::addItem(const char *szName, const char *szValue)
{
	m_pInputNode->appendHTML(StringW(L"<option value=\"")+CMB2WC(szValue)+L"\">"+CMB2WC(szName)+L"</option>");
}


void CUI2ColorGradient::dispatchEvent(gui::IEvent *ev)
{
	if(ev->type == gui::GUI_EVENT_TYPE_CHANGE)
	{
		if(ev->target == m_pInputNode)
		{
			const StringW &wsOption = ev->relatedTarget->getAttribute(L"value");
			X2COLOR_GRADIENT_MODE mode = (X2COLOR_GRADIENT_MODE)wsOption.toInt();
			setMode(mode);
		}
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_LAYOUT && ev->target == m_pGradientPreview)
	{
		UINT uW = m_pGradientPreview->getInnerWidth();
		UINT uH = m_pGradientPreview->getInnerHeight();

		//LogInfo("Resized: %ux%u\n", uW, uH);

		if(uW != m_uRenderWidth || uH != m_uRenderHeight)
		{
			m_uRenderWidth = uW;
			m_uRenderHeight = uH;
			if(m_pRenderTarget)
			{
				m_pRenderTarget->resize(m_uRenderWidth, m_uRenderHeight);
				m_pCamera->setPosition(float3(m_uRenderWidth * 0.5f, m_uRenderHeight * 0.5f, -10.0f));
				m_pGradientPreview->getStyleSelf()->background_image->set(CMB2WC(m_szTextureName));
				m_pGradientPreview->updateStyles();

				updatePeview();
			}
		}
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_CLICK)
	{
		if(ev->target == m_pGradientPreview)
		{
			startEdit((ev->offsetY < ev->target->getInnerHeight() / 2) ? 0 : 1);
		}
		else if(ev->target == m_pColor0Preview)
		{
			startEditColor(0);
		}
		else if(ev->target == m_pColor1Preview)
		{
			startEditColor(1);
		}
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_KEYUP)
	{
		const StringW &wsText = ev->target->getText();
		float4_t vColor = 0.0f;
		int nParts = swscanf(wsText.c_str(), L"%g %g %g %g", &vColor.x, &vColor.y, &vColor.z, &vColor.w);
		TODO("setValid(iParsed != 0);");
		if(nParts > 0)
		{
			if(nParts < 4)
			{
				vColor.w = 1.0f;
			}
			if(nParts == 1)
			{
				vColor.y = vColor.z = vColor.x;
			}
			else if(nParts == 2)
			{
				vColor.z = 0.0f;
			}

			if(ev->target == m_pColor0Textbox)
			{
				setColor(0, vColor, true, false);
			}
			else if(ev->target == m_pColor1Textbox)
			{
				setColor(1, vColor, true, false);
			}

			gui::IEvent ev2;
			ev2.type = gui::GUI_EVENT_TYPE_CHANGE;
			ev2.target = m_pNode;
			ev2.relatedTarget = ev->target;
			m_pNode->dispatchEvent(ev2);
		}
	}
}

void CUI2ColorGradient::startEdit(int iPart)
{
	if(!m_pGradientEditor)
	{
		m_pGradientEditor = (IXColorGradientEditor*)m_pCore->getPluginManager()->getInterface(IXCOLORGRADIENTEDITOR_GUID);
	}

	if(!m_pGradientEditor)
	{
		LogError("Interface IXColorGradientEditor is not available!\n");
		return;
	}

	if(m_pGradient->getMode() == X2CGM_GRADIENT)
	{
		m_pGradientEditor->edit(m_pGradient->getGradient0(), &m_editCallback);
	}
	else if(m_pGradient->getMode() == X2CGM_TWO_GRADIENTS)
	{
		m_pGradientEditor->edit(iPart ? m_pGradient->getGradient1() : m_pGradient->getGradient0(), &m_editCallback);
	}
	m_isEditing = true;
}

void CUI2ColorGradient::abortEdit()
{
	if(m_isEditing)
	{
		m_pGradientEditor->abort();
		m_isEditing = false;
	}
}

void CUI2ColorGradient::onEditAccept()
{
	m_isEditing = false;
	updatePeview();

	gui::IEvent ev;
	ev.type = gui::GUI_EVENT_TYPE_CHANGE;
	ev.target = m_pNode;
	ev.relatedTarget = m_pGradientPreview;
	m_pNode->dispatchEvent(ev);
}
void CUI2ColorGradient::onEditCancel()
{
	m_isEditing = false;
	updatePeview();
}

void CUI2ColorGradient::updatePeview()
{
	if(m_pRenderTarget)
	{
		IXRenderGraphNodeInstance *pNodeInstance = NULL;
		if(!m_pRenderGraph->getNodeInstance("node", &pNodeInstance))
		{
			LogFatal("Unable to find node 'node' in 'xGradientPreview' render graph!\n");
		}

		IXRenderGraphNodeData *pNodeData = NULL;
		if(pNodeInstance->getNodeData(m_pRenderTarget, &pNodeData))
		{
			CGradientPreviewGraphNodeData *pInternalNodeData = (CGradientPreviewGraphNodeData*)pNodeData;

			pInternalNodeData->update(m_pGradient);

			mem_release(pNodeData);
		}
		mem_release(pNodeInstance);
	}
}

void XMETHODCALLTYPE CUI2ColorGradient::setEnabled(bool bEnable)
{
	BaseClass::setEnabled(bEnable);

	if(bEnable)
	{
		m_pGradientPreview->removePseudoclass(0x00004);
		m_pColor0Preview->removePseudoclass(0x00004);
		m_pColor0Textbox->removePseudoclass(0x00004);
		m_pColor1Preview->removePseudoclass(0x00004);
		m_pColor1Textbox->removePseudoclass(0x00004);
	}
	else
	{
		m_pGradientPreview->addPseudoclass(0x00004);
		m_pColor0Preview->addPseudoclass(0x00004);
		m_pColor0Textbox->addPseudoclass(0x00004);
		m_pColor1Preview->addPseudoclass(0x00004);
		m_pColor1Textbox->addPseudoclass(0x00004);
	}
}

void XMETHODCALLTYPE CUI2ColorGradient::setGradient(IX2ColorGradients *pGradient)
{
	if(pGradient)
	{
		m_pGradient = pGradient;
		setMode(pGradient->getMode());

		TODO("Update inputs");
		setColor(0, pGradient->getColor0());
		setColor(1, pGradient->getColor1());

		updatePeview();
	}
	else
	{
		m_pGradient = &m_gradient;
	}
}

void CUI2ColorGradient::setMode(X2COLOR_GRADIENT_MODE mode)
{
	m_pInputNode->setAttribute(L"class", StringW(L"opt-") + mode);

	abortEdit();
	abortEditColor();

	m_pGradient->setMode(mode);
	switch(mode)
	{
	case X2CGM_COLOR:
	case X2CGM_TWO_COLORS:
		mem_release(m_pRenderTarget);
		break;

	case X2CGM_GRADIENT:
	case X2CGM_TWO_GRADIENTS:
		if(!m_pRenderTarget)
		{
			m_pRender->newTextureTarget(&m_pRenderTarget, m_szTextureName + 1, sizeof(m_szTextureName) - 1);
			m_pRenderTarget->setCamera(m_pCamera);
			m_pRenderTarget->attachGraph(m_pRenderGraph);

			if(m_uRenderWidth && m_uRenderHeight)
			{
				m_pRenderTarget->resize(m_uRenderWidth, m_uRenderHeight);
				m_pCamera->setPosition(float3(m_uRenderWidth * 0.5f, m_uRenderHeight * 0.5f, -10.0f));
				m_pGradientPreview->getStyleSelf()->background_image->set(CMB2WC(m_szTextureName));
				m_pGradientPreview->updateStyles();
			}
		}

		updatePeview();
		break;
	}

	gui::IEvent ev;
	ev.type = gui::GUI_EVENT_TYPE_CHANGE;
	ev.target = m_pNode;
	ev.relatedTarget = m_pInputNode;
	m_pNode->dispatchEvent(ev);
}

void XMETHODCALLTYPE CUI2ColorGradient::setVisible(bool isVisible)
{
	if(isVisible)
	{
		setGradient(m_pGradient);
	}
	BaseClass::setVisible(isVisible);
}

void CUI2ColorGradient::setColor(int idx, const float4_t &vColor, bool bTriggerChange, bool bUpdateText)
{
	gui::dom::IDOMnode *pNodeTextbox = idx == 1 ? m_pColor1Textbox : m_pColor0Textbox;
	gui::dom::IDOMnode *pNodePreview = idx == 1 ? m_pColor1Preview : m_pColor0Preview;

	if(bUpdateText)
	{
		wchar_t wszValue[256];
		swprintf(wszValue, L"%.3g %.3g %.3g %.3g", vColor.x, vColor.y, vColor.z, vColor.w);
		pNodeTextbox->setText(wszValue, TRUE);
	}

	UINT uColor =
		(((UINT)((vColor.x) * 255.f) & 0xFF) << 24) |
		(((UINT)((vColor.y) * 255.f) & 0xFF) << 16) |
		(((UINT)((vColor.z) * 255.f) & 0xFF) << 8) |
		0xFF;

	pNodePreview->getStyleSelf()->background_color->set((int)uColor);
	pNodePreview->updateStyles();

	if(bTriggerChange)
	{
		if(idx)
		{
			m_pGradient->setColor1(vColor);
		}
		else
		{
			m_pGradient->setColor0(vColor);
		}

		gui::IEvent ev;
		ev.type = gui::GUI_EVENT_TYPE_CHANGE;
		ev.target = m_pNode;
		if(bUpdateText)
		{
			ev.relatedTarget = idx ? m_pColor1Preview : m_pColor0Preview;
		}
		else
		{
			ev.relatedTarget = idx ? m_pColor1Textbox : m_pColor0Textbox;
		}
		m_pNode->dispatchEvent(ev);
	}
}

void CUI2ColorGradient::startEditColor(int iPart)
{
	if(!m_pColorPicker)
	{
		m_pColorPicker = (IXColorPicker*)m_pCore->getPluginManager()->getInterface(IXCOLORPICKER_GUID);
	}

	if(!m_pColorPicker)
	{
		LogError("Interface IXColorPicker is not available!\n");
		return;
	}

	m_isEditingColor = true;
	m_iColorEditingPart = iPart;

	m_pColorPicker->pick(iPart ? m_pGradient->getColor1() : m_pGradient->getColor0(), &m_editColorCallback);
}

void CUI2ColorGradient::abortEditColor()
{
	if(m_isEditingColor)
	{
		m_pColorPicker->abort();
		m_isEditingColor = false;
	}
}

void CUI2ColorGradient::onColorEditAccept(const float4_t &vColor)
{
	m_isEditingColor = false;
	setColor(m_iColorEditingPart, vColor, true);
}
void CUI2ColorGradient::onColorEditPreview(const float4_t &vColor)
{
	setColor(m_iColorEditingPart, vColor, true);
}
void CUI2ColorGradient::onColorEditCancel(const float4_t &vStartColor)
{
	m_isEditing = false;
	setColor(m_iColorEditingPart, vStartColor, true);
}

//##########################################################################

CGradientEditorCallback::CGradientEditorCallback(CUI2ColorGradient *pControl):
	m_pControl(pControl)
{}

void XMETHODCALLTYPE CGradientEditorCallback::onAccept()
{
	m_pControl->onEditAccept();
}
void XMETHODCALLTYPE CGradientEditorCallback::onCancel()
{
	m_pControl->onEditCancel();
}

//##########################################################################

CColorPickerCallback::CColorPickerCallback(CUI2ColorGradient *pControl):
	m_pControl(pControl)
{
}

void XMETHODCALLTYPE CColorPickerCallback::onAccept(const float4_t &vColor)
{
	m_pControl->onColorEditAccept(vColor);
}

void XMETHODCALLTYPE CColorPickerCallback::onPreview(const float4_t &vColor)
{
	m_pControl->onColorEditPreview(vColor);
}
void XMETHODCALLTYPE CColorPickerCallback::onCancel(const float4_t &vStartColor)
{
	m_pControl->onColorEditCancel(vStartColor);
}
