#include "CurveEditorDialog.h"
#include "resource.h"

#include <windowsx.h>
#include <commctrl.h>
#include "terrax.h"
#include <core/sxcore.h>

CCurveEditorDialog::CCurveEditorDialog(HINSTANCE hInstance, HWND hMainWnd):
	m_hInstance(hInstance),
	m_hMainWnd(hMainWnd)
{
	//m_curve.getMaxCurve()->addKey(0.0f, 0.0f);
	//m_curve.getMaxCurve()->addKey(0.4f, 0.7f);
	//m_curve.getMaxCurve()->addKey(0.5f, 0.1f);
	//m_curve.getMaxCurve()->addKey(0.6f, 0.3f);
	//m_curve.getMaxCurve()->addKey(1.0f, 0.0f);
	m_curve.setMode(XMCM_TWO_CURVES);

	IXAnimationCurve *pCurve = m_curve.getMaxCurve();


	/*pCurve->addKey(0.20512706f, 0.3297542f);
	pCurve->addKey(0.7526245f, 0.5924988f);*/

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
	kf.fOutWeight = 1.0;
	kf.weightedMode = XKWM_OUT;
	pCurve->setKey(0, kf);

	kf = *pCurve->getKeyAt(1);
	//kf.fInTangent = 1.0f;
	kf.fInTangent = 0.0f;
	kf.fInWeight = 1.0;
	kf.weightedMode = XKWM_IN;
	pCurve->setKey(1, kf);


	registerClass();

	CreateDialogParamA(m_hInstance, MAKEINTRESOURCE(IDD_CURVE_EDITOR), hMainWnd, DlgProc, (LPARAM)this);
}

CCurveEditorDialog::~CCurveEditorDialog()
{
	mem_release(m_pNodeData);

	mem_release(m_pSurface);
	//mem_release(m_pSwapChain);
	mem_release(m_pFinalTarget);
	DestroyWindow(m_hDlgWnd);
	
	mem_release(m_pFrameIB);
	mem_release(m_pFrameRB);

	mem_release(m_pInnerIB);
	mem_release(m_pInnerRB);

	mem_release(m_pTransformCB);
	mem_release(m_pInformCB);

#if 0
	mem_release(m_pFont);

	mem_release(m_pTextRB);
	mem_release(m_pTextVD);
	mem_release(m_pTextIB);
	mem_release(m_pTextVB);
	mem_release(m_pTextColorCB);
	mem_release(m_pTextOffsetCB);
#endif
}

INT_PTR CALLBACK CCurveEditorDialog::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CCurveEditorDialog *pThis = (CCurveEditorDialog*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lParam);
		pThis = (CCurveEditorDialog*)lParam;
		pThis->m_hDlgWnd = hWnd;
	}
	if(pThis)
	{
		return(pThis->dlgProc(hWnd, msg, wParam, lParam));
	}
	return(FALSE);
}

static void GetChildRect(HWND hWnd, LPRECT lpRect)
{
	HWND hParentWnd = GetParent(hWnd);
	GetWindowRect(hWnd, lpRect);
	ScreenToClient(hParentWnd, &(((LPPOINT)lpRect)[0]));
	ScreenToClient(hParentWnd, &(((LPPOINT)lpRect)[1]));
}

INT_PTR CALLBACK CCurveEditorDialog::dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			m_hBrowserWnd = GetDlgItem(hWnd, IDC_RENDER_PANEL);
			SetWindowLongPtr(m_hBrowserWnd, GWLP_USERDATA, (LONG_PTR)this);

			m_aBottomList.push_back({GetDlgItem(hWnd, IDOK)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDCANCEL)});

			RECT rc, rcChild;
			GetClientRect(hWnd, &rc);
			for(UINT i = 0, l = m_aBottomList.size(); i < l; ++i)
			{
				GetChildRect(m_aBottomList[i].hWnd, &rcChild);
				m_aBottomList[i].uDeltaPos = rc.bottom - rcChild.top;
			}

			GetChildRect(m_hBrowserWnd, &rcChild);
			m_uPanelDeltaX = rc.right - rcChild.right;
			m_uPanelDeltaY = rc.bottom - rcChild.bottom;


			SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_DLGMODALFRAME);
			break;
		}

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		
		}
		break;

	case WM_CLOSE:
		/*
		if(m_pCallback)
		{
			m_pCallback->onCancel();
			m_pCallback = NULL;
		}
		*/
		ShowWindow(m_hDlgWnd, SW_HIDE);
		break;

	case WM_SIZE:
		{
			RECT rc, rcChild;
			GetClientRect(hWnd, &rc);


			for(UINT i = 0, l = m_aBottomList.size(); i < l; ++i)
			{
				GetChildRect(m_aBottomList[i].hWnd, &rcChild);
				MoveWindow(m_aBottomList[i].hWnd, rcChild.left, rc.bottom - m_aBottomList[i].uDeltaPos, rcChild.right - rcChild.left, rcChild.bottom - rcChild.top, FALSE);
			}

			MoveWindow(m_hBrowserWnd, 0, 0, rc.right - m_uPanelDeltaX, rc.bottom - m_uPanelDeltaY, FALSE);
			
			m_isScreenSizeChanged = true;
			if(!m_isResizing)
			{
				initViewport();
			}
			InvalidateRect(hWnd, &rc, TRUE);
		}
		break;

	case WM_ENTERSIZEMOVE:
		m_isResizing = true;
		m_isScreenSizeChanged = false;
		break;

	case WM_EXITSIZEMOVE:
		m_isResizing = false;
		if(m_isScreenSizeChanged)
		{
			m_isScreenSizeChanged = false;
			initViewport();
		}
		break;

	default:
		return(FALSE);
	}
	return(TRUE);
}

void CCurveEditorDialog::browse()
{
	ShowWindow(m_hDlgWnd, SW_SHOWNA);
	SetFocus(m_hDlgWnd);
}

void CCurveEditorDialog::registerClass()
{
	WNDCLASSEX wcex;
	HBRUSH hBrush = NULL;

	// Resetting the structure members before use
	memset(&wcex, 0, sizeof(WNDCLASSEX));

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = m_hInstance;
	wcex.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_ICON_LOGO));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);

	hBrush = CreateSolidBrush(RGB(0, 0, 0));


	wcex.hbrBackground = (HBRUSH)hBrush;
	wcex.lpszClassName = "XCurveEditor";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	if(!RegisterClassEx(&wcex))
	{
		MessageBox(NULL, "Unable to register window class!", "Error", MB_OK | MB_ICONSTOP);
	}
}

LRESULT CALLBACK CCurveEditorDialog::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CCurveEditorDialog *pThis = (CCurveEditorDialog*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(pThis)
	{
		return(pThis->wndProc(hWnd, msg, wParam, lParam));
	}
	return(DefWindowProc(hWnd, msg, wParam, lParam));
}

LRESULT CALLBACK CCurveEditorDialog::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int xPos;
	int yPos;
	switch(msg)
	{
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
		{
		SetFocus(hWnd);

		xPos = GET_X_LPARAM(lParam);
		yPos = GET_Y_LPARAM(lParam);

		if(msg == WM_LBUTTONDOWN)
		{
			float2_t vInPos, vOutPos;
			if(m_pNodeData->getSelectedPointTangents(&vInPos, &vOutPos))
			{
				const float c_fThreshold = 5.0f; // 5 px
				if(fabsf(vInPos.x - (float)xPos) < c_fThreshold && fabsf(vInPos.y - (float)yPos) < c_fThreshold)
				{
					// in handler
					m_isDraggingTangent = true;
					m_isDraggingInTangent = true;
				}
				else if(fabsf(vOutPos.x - (float)xPos) < c_fThreshold && fabsf(vOutPos.y - (float)yPos) < c_fThreshold)
				{
					// out handler
					m_isDraggingTangent = true;
					m_isDraggingInTangent = false;
				}

				if(m_isDraggingTangent)
				{
					SetCapture(hWnd);
					break;
				}
			}
		}

		float fX, fY;
		m_pNodeData->screenToGraph(xPos, yPos, &fX, &fY);

		int iHitCurve = -1;
		int iHitKey = -1;
		if(hitTest(fX, fY, m_curve.getMaxCurve(), &iHitKey))
		{
			iHitCurve = 1;
		}
		else if(hitTest(fX, fY, m_curve.getMinCurve(), &iHitKey))
		{
			iHitCurve = 0;
		}

		m_pNodeData->setHighlight(iHitCurve, iHitKey);

		bool isSelected = false;

		if(iHitCurve >= 0)
		{
			if(msg == WM_LBUTTONDBLCLK)
			{
				if(iHitKey < 0)
				{
					IXAnimationCurve *pCurve = iHitCurve == 0 ? m_curve.getMinCurve() : m_curve.getMaxCurve();

					const float c_fDtX = 0.001f;
					float fVal = pCurve->evaluate(fX);
					float fDxLeft = fVal - pCurve->evaluate(fX - c_fDtX);
					float fDxRight = pCurve->evaluate(fX + c_fDtX) - fVal;

					int idx = pCurve->addKey(fX, fVal);
					XKeyframe kf = *pCurve->getKeyAt(idx);
					kf.fInTangent = fDxLeft / c_fDtX;
					kf.fOutTangent = fDxRight / c_fDtX;
					pCurve->setKey(idx, kf);

					iHitKey = idx;
				}

				isSelected = true;
			}
			else if(iHitKey >= 0)
			{
				m_iActiveCurve = iHitCurve;
				m_iActiveKey = iHitKey;
				m_isDraggingKey = true;

				isSelected = true;
			}
		}

		if(isSelected)
		{
			m_pNodeData->setSelect(iHitCurve, iHitKey);
		}
		else
		{
			m_pNodeData->setSelect(-1, -1);
		}

		SetCapture(hWnd);
#if 0
		if(m_uPanelWidth - xPos <= m_pScrollBar->getWidth())
		{
			SetCapture(hWnd);
			m_isTrackingScroll = true;
			m_pScrollBar->onMouseDown(GET_Y_LPARAM(lParam));
		}
		else
		{
			if(msg == WM_LBUTTONDOWN)
			{
				int yPos = GET_Y_LPARAM(lParam);

				yPos += m_iScrollPos;

				for(UINT i = 0, l = m_aMaterials.size(); i < l; ++i)
				{
					MaterialItem &item = m_aMaterials[i];
					if(xPos > item.uXpos && yPos > item.uYpos && xPos <= item.uXpos + m_frameSize && yPos <= item.uYpos + m_frameSize + 20)
					{
						m_uSelectedItem = i;
						UINT uTexW = 0, uTexH = 0;

						if(item.pTexture)
						{
							uTexW = item.pTexture->getWidth();
							uTexH = item.pTexture->getHeight();
						}

						char tmp[64];
						sprintf(tmp, "%ux%u", uTexW, uTexH);

						SetDlgItemText(m_hDlgWnd, IDC_MAT_SIZE, tmp);
						SetDlgItemTextW(m_hDlgWnd, IDC_MAT_NAME, CMB2WC(item.sName.c_str()));

						break;
					}
				}
			}
			else if(m_uSelectedItem != ~0)
			{
				if(m_pCallback)
				{
					m_pCallback->onSelected(m_aMaterials[m_uSelectedItem].sName.c_str());
					m_pCallback = NULL;
				}
				ShowWindow(m_hDlgWnd, SW_HIDE);
			}
		}
#endif
		}
		break;
	case WM_LBUTTONUP:
		if(m_isDraggingKey || m_isDraggingTangent)
		{
			ReleaseCapture();
		}

		m_isDraggingKey = false;
		m_isDraggingTangent = false;
		break;

	case WM_MOUSEMOVE:
		{
			xPos = GET_X_LPARAM(lParam);
			yPos = GET_Y_LPARAM(lParam);

			if(m_isDraggingTangent)
			{
				m_pNodeData->calcNewTangent(xPos, yPos, m_isDraggingInTangent);
				break;
			}

			float fX, fY;
			m_pNodeData->screenToGraph(xPos, yPos, &fX, &fY);

			if(m_isDraggingKey)
			{
				assert(m_iActiveCurve >= 0);

				IXAnimationCurve *pCurve = m_iActiveCurve == 0 ? m_curve.getMinCurve() : m_curve.getMaxCurve();
				XKeyframe kf = *pCurve->getKeyAt(m_iActiveKey);
				kf.fTime = fX;
				kf.fValue = fY;
				m_iActiveKey = pCurve->moveKey(m_iActiveKey, kf);

				assert(m_iActiveKey >= 0);

				m_pNodeData->setHighlight(m_iActiveCurve, m_iActiveKey);
				m_pNodeData->setSelect(m_iActiveCurve, m_iActiveKey);
			}
			else
			{
				int iHitKey;
				if(hitTest(fX, fY, m_curve.getMaxCurve(), &iHitKey))
				{
					m_pNodeData->setHighlight(1, iHitKey);
				}
				else if(hitTest(fX, fY, m_curve.getMinCurve(), &iHitKey))
				{
					m_pNodeData->setHighlight(0, iHitKey);
				}
				else
				{
					m_pNodeData->setHighlight(-1, -1);
				}

				if(!m_isTrackingLeaveEvent)
				{
					TRACKMOUSEEVENT tme = {};
					tme.cbSize = sizeof(tme);
					tme.hwndTrack = hWnd;
					tme.dwFlags = TME_LEAVE;
					m_isTrackingLeaveEvent = TrackMouseEvent(&tme);
				}
			}
		}
		break;

	case WM_MOUSELEAVE:
		m_isTrackingLeaveEvent = false;;
		m_pNodeData->setHighlight(-1, -1);
		m_isDraggingKey = false;
		m_isDraggingTangent = false;
		break;

	case WM_PAINT:
		m_isDirty = true;
		// NO BREAK!
	default:
		return(DefWindowProc(hWnd, msg, wParam, lParam));
	}
	return(0);
}

void CCurveEditorDialog::initGraphics(IXRender *pRender)
{
	m_pRender = pRender;
	m_pDev = m_pRender->getDevice();

	initViewport();

	IXRenderGraph *pRenderGraph;
	if(!m_pRender->getGraph("xCurveEditor", &pRenderGraph))
	{
		LogFatal("Unable to load 'xCurveEditor' render graph!\n");
	}
	m_pFinalTarget->attachGraph(pRenderGraph);

	IXRenderGraphNodeInstance *pNodeInstance = NULL;
	if(!pRenderGraph->getNodeInstance("node", &pNodeInstance))
	{
		LogFatal("Unable to find node 'node' in 'xCurveEditor' render graph!\n");
	}

	IXRenderGraphNodeData *pNodeData = NULL;
	pNodeInstance->getNodeData(m_pFinalTarget, &pNodeData);

	m_pNodeData = (CCurveEditorGraphNodeData*)pNodeData;

	m_pNodeData->updateLine(&m_curve);

	mem_release(pNodeInstance);

	mem_release(pRenderGraph);

#if 0
	m_pMaterialSystem = (IXMaterialSystem*)Core_GetIXCore()->getPluginManager()->getInterface(IXMATERIALSYSTEM_GUID);

	m_pFontManager = (IXFontManager*)Core_GetIXCore()->getPluginManager()->getInterface(IXFONTMANAGER_GUID);
	if(m_pFontManager)
	{
		m_pFontManager->getFont(&m_pFont, "gui/fonts/tahoma.ttf", 10);
		m_pFontManager->getFontVertexDeclaration(&m_pTextVD);
	}
#endif
}
void CCurveEditorDialog::render()
{
	//if(IsWindowVisible(m_hBrowserWnd) && m_isDirty)
	{
		IGXContext *pCtx = m_pDev->getThreadContext();
		IGXSurface *pOldRT = pCtx->getColorTarget();
		pCtx->setColorTarget(m_pSurface);
		//IGXDepthStencilSurface *pOldDS = pCtx->getDepthStencilSurface();
		pCtx->setDepthStencilSurface(NULL);

		pCtx->clear(GX_CLEAR_COLOR, float4(0, 1, 0, 0));

#if 0
		pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);
		pCtx->setRasterizerState(NULL);
		pCtx->setBlendState(g_xRenderStates.pBlendAlpha);

		pCtx->setVSConstant(m_pTransformCB, SCR_CAMERA);

		IGXBaseTexture *pTexture;

		int iYStart = m_iScrollPos - m_frameSize-27;
		int iYEnd = m_iScrollPos + m_uPanelHeight;
		for(UINT i = 0, l = m_aMaterials.size(); i < l; ++i)
		{
			MaterialItem &item = m_aMaterials[i];
			if((int)item.uYpos >= iYStart && item.uYpos <= iYEnd)
			{
				pCtx->setBlendState(g_xRenderStates.pBlendAlpha);

				drawFrame(item.uXpos, item.uYpos, m_frameSize, item.uTitleWidth, m_uSelectedItem == i ? 1.0f : 0.0f);

				if(item.pTexture)
				{
					if(!item.isTransparent && !item.isTexture)
					{
						pCtx->setBlendState(NULL);
					}

					m_pRender->bindShader(pCtx, m_idInnerShader);

					item.pTexture->getAPITexture(&pTexture, item.uCurrentFrame);
					pCtx->setPSTexture(pTexture);
					mem_release(pTexture);

					pCtx->setIndexBuffer(m_pInnerIB);
					pCtx->setRenderBuffer(m_pInnerRB);
					pCtx->drawIndexed(m_uInnerVC, m_uInnerPC);

					m_pRender->unbindShader(pCtx);
				}
			}
		}

		m_pScrollBar->render();

		if(m_uTextQuadCount)
		{
			IGXTexture2D *pTex;
			m_pFont->getTexture(0, &pTex);
			pCtx->setPSTexture(pTex);
			mem_release(pTex);
			pCtx->setRenderBuffer(m_pTextRB);
			pCtx->setIndexBuffer(m_pTextIB);
			pCtx->setPSConstant(m_pTextColorCB);
			m_pTextOffsetCB->update(&float4_t(0.0f, (float)m_iScrollPos, 0.0f, 0.0f));
			pCtx->setVSConstant(m_pTextOffsetCB, 6);
			m_pRender->bindShader(pCtx, m_idTextShader);
			pCtx->drawIndexed(m_uTextVertexCount, m_uTextQuadCount * 2);
			m_pRender->unbindShader(pCtx);
		}
#endif
		//pCtx->setDepthStencilSurface(pOldDS);
		//mem_release(pOldDS);
		pCtx->setColorTarget(pOldRT);
		pCtx->downsampleColorTarget(m_pSurface, pOldRT);
		mem_release(pOldRT);


		//IGXSurface *pRT = m_pSwapChain->getColorTarget();
		//mem_release(pRT);

		//m_isDirty = false;
		m_bDoSwap = true;
	}
}
void CCurveEditorDialog::swapBuffers()
{
	if(IsWindowVisible(m_hBrowserWnd) && m_bDoSwap)
	{
		//m_pSwapChain->swapBuffers();
		m_bDoSwap = false;
	}
}

void CCurveEditorDialog::initViewport()
{
	RECT rc;
	GetClientRect(m_hBrowserWnd, &rc);
	m_uPanelWidth = rc.right - rc.left;
	m_uPanelHeight = rc.bottom - rc.top;

	//mem_release(m_pSwapChain);
	mem_release(m_pSurface);
	//m_pSwapChain = m_pDev->createSwapChain(m_uPanelWidth, m_uPanelHeight, m_hBrowserWnd);
	m_pSurface = m_pDev->createColorTarget(m_uPanelWidth, m_uPanelHeight, GXFMT_A8B8G8R8_SRGB, GXMULTISAMPLE_4_SAMPLES);
	//m_pDepthStencilSurface = pDev->createDepthStencilSurface(m_uPanelWidth, m_uPanelHeight, GXFMT_D24S8, GXMULTISAMPLE_NONE);
	if(!m_pFinalTarget)
	{
		m_pRender->newFinalTarget(m_hBrowserWnd, "xCurveEditor", &m_pFinalTarget);
		m_pRender->newCamera(&m_pCamera);
		m_pFinalTarget->setCamera(m_pCamera);
		m_pCamera->setProjectionMode(XCPM_ORTHOGONAL);
	}
	m_pFinalTarget->resize(m_uPanelWidth, m_uPanelHeight);
	m_pCamera->setPosition(float3(m_uPanelWidth * 0.5f, m_uPanelHeight * 0.5f, -10.0f));

	float fWidth = (float)m_uPanelWidth;
	float fHeight = (float)m_uPanelHeight;

	/*SMMATRIX mProj = SMMatrixTranslation(-0.5f, -0.5f, 0.0f) * SMMatrixOrthographicLH(fWidth, fHeight, 1.0f, 2000.0f);
	SMMATRIX mView = SMMatrixLookToLH(float3(fWidth * 0.5f, -fHeight * 0.5f, -1.0f), float3(0.0f, 0.0f, 1.0f), float3(0.0f, 1.0f, 0.0f));

	if(!m_pTransformCB)
	{
		m_pTransformCB = m_pDev->createConstantBuffer(sizeof(SMMATRIX));
	}
	m_pTransformCB->update(&SMMatrixTranspose(mView * mProj));*/
}

bool CCurveEditorDialog::hitTest(float fX, float fY, IXAnimationCurve *pCurve, int *piHitKeyframe)
{
	float fXScale, fYScale;
	m_pNodeData->getScale(&fXScale, &fYScale);
	const float c_fThreshold = 10.0f; // 10 px

	float fCurveVal = pCurve->evaluate(fX);
	bool isOnCurve = fabsf(fY - fCurveVal) < c_fThreshold * fYScale;

	if(!isOnCurve)
	{
		float fY0 = pCurve->evaluate(fX - c_fThreshold * fXScale);
		float fY1 = pCurve->evaluate(fX + c_fThreshold * fXScale);

		if(fY0 > fY1)
		{
			fY0 += c_fThreshold * fYScale;
			fY1 -= c_fThreshold * fYScale;

			isOnCurve = fY <= fY0 && fY >= fY1;
		}
		else
		{
			fY0 -= c_fThreshold * fYScale;
			fY1 += c_fThreshold * fYScale;

			isOnCurve = fY >= fY0 && fY <= fY1;
		}

	}

	int iMinKey = -1;
	if(isOnCurve)
	{
		float fMinD = -1.0f;
		for(UINT i = 0, l = pCurve->getKeyframeCount(); i < l; ++i)
		{
			const XKeyframe *pKey = pCurve->getKeyAt(i);
			float fD = fabsf(fX - pKey->fTime);
			if(fD < (c_fThreshold * fXScale) && /*fabsf(fY - pKey->fValue) < (c_fThreshold * fYScale) &&*/ (fD < fMinD || iMinKey < 0))
			{
				iMinKey = i;
				fMinD = fD;
			}
		}
	}

	*piHitKeyframe = iMinKey;

	return(isOnCurve);
}
