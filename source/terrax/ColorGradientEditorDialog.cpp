#include <xcommon/IPluginManager.h>

#include "ColorGradientEditorDialog.h"
#include "GradientPreviewGraphNodeData.h"

CColorGradientEditorDialog::CColorGradientEditorDialog(HWND hMainWnd, IXCore *pCore):
	m_hMainWnd(hMainWnd),
	m_pCore(pCore)
{
	// keep itself until window is closed
	AddRef();

	m_pXUI = (IXUI*)pCore->getPluginManager()->getInterface(IXUI_GUID);

	IXRender *pRender = (IXRender*)pCore->getPluginManager()->getInterface(IXRENDER_GUID);

	XWINDOW_DESC wdesc;
	wdesc.iPosX = XCW_USEDEFAULT;
	wdesc.iPosY = XCW_USEDEFAULT;
	wdesc.iSizeX = 450;
	wdesc.iSizeY = 200;
	wdesc.szTitle = "Gradient editor";
	wdesc.flags = XWF_BUTTON_CLOSE | XWF_TRANSPARENT | XWF_INIT_HIDDEN;

	
	m_pWindow = m_pXUI->createWindow(&wdesc, (IUIWindow*)hMainWnd); // temporary hack!
	m_pWindow->setCallback(EventProc, this);

	m_pOkButton = m_pXUI->createButton();
	m_pWindow->insertChild(m_pOkButton);
	m_pOkButton->setLabel("OK");

	m_pCancelButton = m_pXUI->createButton();
	m_pWindow->insertChild(m_pCancelButton);
	m_pCancelButton->setLabel("Cancel");

	m_pViewport = m_pXUI->createViewport();
	m_pWindow->insertChild(m_pViewport);

	m_pColorInput = m_pXUI->createColor();
	m_pWindow->insertChild(m_pColorInput);
	m_pColorInput->setVisible(false);

	m_pAlphaInput = m_pXUI->createTextBox();
	m_pWindow->insertChild(m_pAlphaInput);
	m_pAlphaInput->setVisible(false);

	m_pTopTrackbar = m_pXUI->createMultiTrackbar();
	m_pWindow->insertChild(m_pTopTrackbar);
	m_pTopTrackbar->setMarkerCount(3);
	m_pTopTrackbar->setMarkerPos(0, 0.0f);
	m_pTopTrackbar->setMarkerPos(1, 0.5f);
	m_pTopTrackbar->setMarkerPos(2, 1.0f);

	m_pBottomTrackbar = m_pXUI->createMultiTrackbar();
	m_pWindow->insertChild(m_pBottomTrackbar);
	m_pBottomTrackbar->setMarkerCount(2);
	m_pBottomTrackbar->setMarkerPos(0, 0.0f);
	m_pBottomTrackbar->setMarkerPos(1, 1.0f);
	m_pBottomTrackbar->setInvertedMode(true);

	m_pViewport->getRenderTarget(&m_pRenderTarget);

	pRender->newCamera(&m_pCamera);
	m_pCamera->setProjectionMode(XCPM_ORTHOGONAL);

	m_pRenderTarget->setCamera(m_pCamera);

	pRender->getGraph("xGradientPreview", &m_pRenderGraph);
	m_pRenderTarget->attachGraph(m_pRenderGraph);
	
	// m_gradient.setColorKeyCount(2);
	// m_gradient.setColorKey(0, {float3_t(0.0f, 0.0f, 0.0f), 0.0f});
	// m_gradient.setColorKey(1, {float3_t(1.0f, 1.0f, 1.0f), 1.0f});

	onResize();

	//edit(&m_gradient, NULL);
}

CColorGradientEditorDialog::~CColorGradientEditorDialog()
{
	mem_release(m_pAlphaInput);
	mem_release(m_pColorInput);
	mem_release(m_pRenderGraph);
	mem_release(m_pRenderTarget);
	mem_release(m_pTopTrackbar);
	mem_release(m_pBottomTrackbar);
	mem_release(m_pOkButton);
	mem_release(m_pCancelButton);
	mem_release(m_pCamera);
	mem_release(m_pWindow);
	mem_release(m_pViewport);
}

void CColorGradientEditorDialog::EventProc(void *pCtx, IUIControl *pControl, gui::IEvent *ev)
{
	CColorGradientEditorDialog *pThis = (CColorGradientEditorDialog*)pCtx;

	pThis->eventProc(pControl, ev);
}
void CColorGradientEditorDialog::eventProc(IUIControl *pControl, gui::IEvent *ev)
{
	if(ev->type == gui::GUI_EVENT_TYPE_CLOSE)
	{
		callAbort();
		//mem_release(this);
		return;
	}

	if(ev->type == gui::GUI_EVENT_TYPE_RESIZE)
	{
		onResize();
		return;
	}

	if(ev->type == gui::GUI_EVENT_TYPE_CHANGE)
	{
		//LogInfo("GUI_EVENT_TYPE_CHANGE\n");
		if(pControl == m_pBottomTrackbar || pControl == m_pTopTrackbar)
		{
			if(pControl == m_pBottomTrackbar)
			{
				int iSel = m_pBottomTrackbar->getSelection();

				if(iSel >= 0)
				{
					m_pTopTrackbar->setSelection(-1);
				}

				UINT uColorKeys = m_pBottomTrackbar->getMarkerCount();
				if(m_pGradient->getColorKeyCount() + 1 == uColorKeys && iSel == uColorKeys - 1)
				{
					float4_t vColor = m_pGradient->evaluate(m_pBottomTrackbar->getMarkerPos(iSel));
					vColor.w = 1.0f;
					m_pBottomTrackbar->setMarkerColor(iSel, vColor);
				}

				syncColors();

				m_pColorInput->setVisible(iSel >= 0);
				m_pAlphaInput->setVisible(false);
				if(iSel >= 0)
				{
					m_pColorInput->setValue(m_pBottomTrackbar->getMarkerColor(iSel));
				}
			}
			else
			{
				int iSel = m_pTopTrackbar->getSelection();

				if(iSel >= 0)
				{
					m_pBottomTrackbar->setSelection(-1);
				}

				UINT uAlphaKeys = m_pTopTrackbar->getMarkerCount();
				if(m_pGradient->getAlphaKeyCount() + 1 == uAlphaKeys && iSel == uAlphaKeys - 1)
				{
					float fAlpha = m_pGradient->evaluate(m_pTopTrackbar->getMarkerPos(iSel)).w;
					m_pTopTrackbar->setMarkerColor(iSel, float4_t(fAlpha, fAlpha, fAlpha, 1.0f));
				}

				syncAlphas();

				m_pColorInput->setVisible(false);
				m_pAlphaInput->setVisible(iSel >= 0);
				if(iSel >= 0)
				{
					char tmp[32];
					sprintf(tmp, "%g", m_pTopTrackbar->getMarkerColor(iSel).x);
					m_pAlphaInput->setValue(tmp);
				}
			}
		}
		else if(pControl == m_pColorInput)
		{
			int iSel = m_pBottomTrackbar->getSelection();
			if(iSel >= 0)
			{
				m_pBottomTrackbar->setMarkerColor(iSel, m_pColorInput->getValue());
				
				syncColors();
			}
		}
		return;
	}

	if(ev->type == gui::GUI_EVENT_TYPE_CLICK)
	{
		if(pControl == m_pOkButton)
		{
			callAccept();
			m_pWindow->close();
		}
		else if(pControl == m_pCancelButton)
		{
			callAbort();
			m_pWindow->close();
		}
		return;
	}

	if(ev->type == gui::GUI_EVENT_TYPE_KEYUP)
	{
		if(pControl == m_pAlphaInput)
		{
			int iSel = m_pTopTrackbar->getSelection();
			if(iSel >= 0)
			{
				float fAlpha;
				if(sscanf(m_pAlphaInput->getValue(), "%f", &fAlpha) == 1)
				{
					m_pTopTrackbar->setMarkerColor(iSel, float4_t(fAlpha, fAlpha, fAlpha, 1.0f));

					syncAlphas();
				}
			}
		}
		return;
	}
}

void CColorGradientEditorDialog::onResize()
{
	UINT uWidth = m_pWindow->getDesc()->iSizeX;
	UINT uHeight = m_pWindow->getDesc()->iSizeY;
	
	float fExternalMargin = 20.0f;
	float fDraggersRowHeight = 15.0f;
	float fButtonHeight = 24.0f;
	float fButtonWidth = 64.0f;
	float fControlHeight = 30.0f;
	float fSpacing = 10.0f;

	float fViewportWidth = uWidth - 2.0f * fExternalMargin;
	float fViewportHeight = uHeight - 2.0f * fExternalMargin - fButtonHeight - 2.0f * fSpacing - fControlHeight - 2.0f * fDraggersRowHeight;

	m_pViewport->setPosition(fExternalMargin, fExternalMargin + fDraggersRowHeight);
	m_pViewport->setSize(fViewportWidth, fViewportHeight);
	m_pCamera->setPosition(float3(fViewportWidth * 0.5f, fViewportHeight * 0.5f, -10.0f));

	m_pOkButton->setPosition(uWidth - fExternalMargin - 2.0f * fButtonWidth - fSpacing, fExternalMargin + 2.0f * fDraggersRowHeight + fViewportHeight + 2.0f * fSpacing + fControlHeight);
	m_pOkButton->setSize(fButtonWidth, fButtonHeight);

	m_pCancelButton->setPosition(uWidth - fExternalMargin - fButtonWidth, fExternalMargin + 2.0f * fDraggersRowHeight + fViewportHeight + 2.0f * fSpacing + fControlHeight);
	m_pCancelButton->setSize(fButtonWidth, fButtonHeight);

	m_pTopTrackbar->setPosition(fExternalMargin + 2.0f, fExternalMargin);
	m_pTopTrackbar->setSize(uWidth - 2.0f * fExternalMargin - 4.0f, fDraggersRowHeight);

	m_pBottomTrackbar->setPosition(fExternalMargin + 2.0f, fExternalMargin + fDraggersRowHeight + fViewportHeight);
	m_pBottomTrackbar->setSize(uWidth - 2.0f * fExternalMargin - 4.0f, fDraggersRowHeight);

	m_pColorInput->setPosition(fExternalMargin, fExternalMargin + 2.0f * fDraggersRowHeight + fViewportHeight + fSpacing);
	m_pColorInput->setSize(250.0f, fControlHeight);

	m_pAlphaInput->setPosition(fExternalMargin, fExternalMargin + 2.0f * fDraggersRowHeight + fViewportHeight + fSpacing);
	m_pAlphaInput->setSize(250.0f, fControlHeight);

	updatePeview();
}

void CColorGradientEditorDialog::updatePeview()
{
	if(m_pRenderTarget && m_pGradient)
	{
		IXRenderGraphNodeInstance *pNodeInstance = NULL;
		if(!m_pRenderGraph->getNodeInstance("node", &pNodeInstance))
		{
			LogFatal("Unable to find node 'node' in 'xCurveEditor' render graph!\n");
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

void XMETHODCALLTYPE CColorGradientEditorDialog::edit(IXColorGradient *pGradient, IXColorGradientEditorCallback *pCallback)
{
	assert(pGradient);

	callAbort();
	m_pWindow->show();

	m_gradientBackup.setFrom(pGradient);

	m_pGradient = pGradient;

	UINT uColorKeys = m_pGradient->getColorKeyCount();
	m_pBottomTrackbar->setMarkerCount(uColorKeys);
	for(UINT i = 0; i < uColorKeys; ++i)
	{
		const XColorKey *pKey = m_pGradient->getColorKeyAt(i);
		m_pBottomTrackbar->setMarkerPos(i, pKey->fTime);
		m_pBottomTrackbar->setMarkerColor(i, float4(pKey->vColor, 1.0f));
	}

	UINT uAlphaKeys = m_pGradient->getAlphaKeyCount();
	m_pTopTrackbar->setMarkerCount(uAlphaKeys);
	for(UINT i = 0; i < uAlphaKeys; ++i)
	{
		const XAlphaKey *pKey = m_pGradient->getAlphaKeyAt(i);
		m_pTopTrackbar->setMarkerPos(i, pKey->fTime);
		m_pTopTrackbar->setMarkerColor(i, float4(pKey->fAlpha, pKey->fAlpha, pKey->fAlpha, 1.0f));
	}

	m_pCallback = pCallback;
	updatePeview();
}
void XMETHODCALLTYPE CColorGradientEditorDialog::abort()
{
	callAbort();
	m_pWindow->close();
}

void CColorGradientEditorDialog::syncColors()
{
	UINT uColorKeys = m_pBottomTrackbar->getMarkerCount();
	m_pGradient->setColorKeyCount(uColorKeys);
	for(UINT i = 0; i < uColorKeys; ++i)
	{
		m_pGradient->setColorKey(i, {m_pBottomTrackbar->getMarkerColor(i), m_pBottomTrackbar->getMarkerPos(i)});
	}

	m_pGradient->sortKeys();

	updatePeview();
}
void CColorGradientEditorDialog::syncAlphas()
{
	UINT uAlphaKeys = m_pTopTrackbar->getMarkerCount();
	m_pGradient->setAlphaKeyCount(uAlphaKeys);
	for(UINT i = 0; i < uAlphaKeys; ++i)
	{
		m_pGradient->setAlphaKey(i, {m_pTopTrackbar->getMarkerColor(i).x, m_pTopTrackbar->getMarkerPos(i)});
	}

	m_pGradient->sortKeys();

	updatePeview();
}

void CColorGradientEditorDialog::callAccept()
{
	m_pBottomTrackbar->setSelection(-1);
	m_pTopTrackbar->setSelection(-1);
	SAFE_CALL(m_pCallback, onAccept);
	m_pCallback = NULL;
	m_pGradient = NULL;
}
void CColorGradientEditorDialog::callAbort()
{
	m_pBottomTrackbar->setSelection(-1);
	m_pTopTrackbar->setSelection(-1);
	SAFE_CALL(m_pGradient, setFrom, &m_gradientBackup);
	SAFE_CALL(m_pCallback, onCancel);
	m_pCallback = NULL;
	m_pGradient = NULL;
}
