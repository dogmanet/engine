#include "MaterialBrowser.h"
#include "resource.h"

#include <windowsx.h>
#include <commctrl.h>
#include <xcommon/render/IXRender.h>
#include "terrax.h"
#include <core/sxcore.h>

UINT GetWindowDPI(HWND hWnd);

CMaterialBrowser::CMaterialBrowser(HINSTANCE hInstance, HWND hMainWnd):
	m_hInstance(hInstance),
	m_hMainWnd(hMainWnd)
{
	registerClass();

	CreateDialogParamA(m_hInstance, MAKEINTRESOURCE(IDD_MAT_BROWSER), hMainWnd, DlgProc, (LPARAM)this);
}

CMaterialBrowser::~CMaterialBrowser()
{
	for(UINT i = 0, l = m_aMaterials.size(); i < l; ++i)
	{
		mem_release(m_aMaterials[i].pTexture);
		mem_release(m_aMaterials[i].pMaterial);
		mem_delete_a(m_aMaterials[i].pVertices);
	}

	mem_release(m_pFont);

	mem_delete(m_pScrollBar);
	mem_delete(m_pScrollbarEvent);

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

	mem_release(m_pTextRB);
	mem_release(m_pTextVD);
	mem_release(m_pTextIB);
	mem_release(m_pTextVB);
	mem_release(m_pTextColorCB);
	mem_release(m_pTextOffsetCB);
}

INT_PTR CALLBACK CMaterialBrowser::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CMaterialBrowser *pThis = (CMaterialBrowser*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lParam);
		pThis = (CMaterialBrowser*)lParam;
		pThis->m_hDlgWnd = hWnd;
	}
	if(pThis)
	{
		return(pThis->dlgProc(hWnd, msg, wParam, lParam));
	}
	return(FALSE);
}

void GetChildRect(HWND hWnd, LPRECT lpRect)
{
	HWND hParentWnd = GetParent(hWnd);
	GetWindowRect(hWnd, lpRect);
	ScreenToClient(hParentWnd, &(((LPPOINT)lpRect)[0]));
	ScreenToClient(hParentWnd, &(((LPPOINT)lpRect)[1]));
}

INT_PTR CALLBACK CMaterialBrowser::dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			m_hBrowserWnd = GetDlgItem(hWnd, IDC_MAT_BROWSER_PANEL);
			SetWindowLongPtr(m_hBrowserWnd, GWLP_USERDATA, (LONG_PTR)this);

			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_EDIT)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_TEXSIZE)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_FILTER)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_KEYWORD)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_NAME)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_OPAQUE)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_RELOAD)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_SELFILLUM)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_SIZE)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_TRANSPARENT)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_USEDONLY)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_STATIC_SIZE)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_STATIC_KEYWORDS)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_STATIC_FILTER)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_MATERIALS)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_TEXTURES)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_TRANSLATED)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDC_MAT_RAWFILES)});

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

			HWND hCombo = GetDlgItem(hWnd, IDC_MAT_TEXSIZE);
			ComboBox_AddString(hCombo, "128x128");
			ComboBox_AddString(hCombo, "256x256");
			ComboBox_AddString(hCombo, "512x512");
			ComboBox_SetCurSel(hCombo, 0);
			ComboBox_SetItemData(hCombo, 0, FRAME_SIZE_128);
			ComboBox_SetItemData(hCombo, 1, FRAME_SIZE_256);
			ComboBox_SetItemData(hCombo, 2, FRAME_SIZE_512);


			Button_SetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_OPAQUE), BST_CHECKED);
			Button_SetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_TRANSPARENT), BST_CHECKED);
			Button_SetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_SELFILLUM), BST_CHECKED);
			Button_SetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_MATERIALS), BST_CHECKED);
			Button_SetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_TEXTURES), BST_CHECKED);
			Button_SetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_TRANSLATED), BST_CHECKED);
			Button_SetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_RAWFILES), BST_CHECKED);

			SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_DLGMODALFRAME);

			m_fScale = ((float)GetWindowDPI(hWnd) / (float)USER_DEFAULT_SCREEN_DPI);
			break;
		}

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_MAT_TEXSIZE:
			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				HWND hCombo = (HWND)lParam;
				int iSel = ComboBox_GetCurSel(hCombo);
				if(iSel >= 0)
				{
					int iSize = ComboBox_GetItemData(hCombo, iSel);
					if(iSize)
					{
						m_frameSize = (FRAME_SIZE)iSize;
						invalidateTexts();
						layout();
					}
				}
			}
			break;

		case IDC_MAT_FILTER:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				scheduleFilterIn(1.0f);
			}
			break;

		case IDC_MAT_OPAQUE:
		case IDC_MAT_TRANSPARENT:
		case IDC_MAT_SELFILLUM:
		case IDC_MAT_USEDONLY:
		case IDC_MAT_MATERIALS:
		case IDC_MAT_TEXTURES:
		case IDC_MAT_TRANSLATED:
		case IDC_MAT_RAWFILES:
			scheduleFilterIn(0.5f);
			break;

		case IDC_MAT_RELOAD:
			m_pMaterialSystem->scanMaterials();
			filter();
			break;

		case IDC_MAT_EDIT:
			if(m_uSelectedItem != ~0)
			{
				IXMaterial *pMat;
				m_pMaterialSystem->loadMaterial(m_aMaterials[m_uSelectedItem].sName.c_str(), &pMat);
				new CMaterialEditor(m_hInstance, m_hMainWnd, Core_GetIXCore(), pMat);
			}
			break;

		case IDC_MAT_KEYWORD:
			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				scheduleFilterIn(0.5f);
			}
			break;
		}
		break;

	case WM_CLOSE:
		if(m_pCallback)
		{
			m_pCallback->onCancel();
			m_pCallback = NULL;
		}
		ShowWindow(m_hDlgWnd, SW_HIDE);
		break;

	case WM_DPICHANGED:
		m_fScale = ((float)LOWORD(wParam) / (float)USER_DEFAULT_SCREEN_DPI);
		m_pScrollBar->setScale(m_fScale);
		initFont();

		__fallthrough;
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

			m_uFontGenStart = ~0;
			m_uFontGenEnd = ~0;
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

	case WM_MOUSEWHEEL:
		{
			m_iScrollPos = (float)m_iScrollPos - (float)GET_WHEEL_DELTA_WPARAM(wParam) * 32.0f / (float)WHEEL_DELTA;

			if(m_iScrollPos < 0)
			{
				m_iScrollPos = 0;
			}
			else if(m_iScrollPos > m_iScrollHeight)
			{
				m_iScrollPos = m_iScrollHeight;
			}
			m_pScrollBar->setScrollPos(m_iScrollPos);

			preload();
			m_isDirty = true;
		}
		break;

	case WM_VSCROLL:
		{
			int iNewPos;    // new position 

			switch(LOWORD(wParam))
			{
				// User clicked the scroll bar shaft above the scroll box. 
			case SB_PAGEUP:
				iNewPos = m_iScrollPos - m_uPanelHeight;
				break;

				// User clicked the scroll bar shaft below the scroll box. 
			case SB_PAGEDOWN:
				iNewPos = m_iScrollPos + m_uPanelHeight;
				break;

				// User clicked the top arrow. 
			case SB_LINEUP:
				iNewPos = m_iScrollPos - m_frameSize;
				break;

				// User clicked the bottom arrow. 
			case SB_LINEDOWN:
				iNewPos = m_iScrollPos + m_frameSize;
				break;

				// User dragged the scroll box. 
			case SB_THUMBPOSITION:
				iNewPos = m_pScrollBar->getScrollPos();
				break;

			default:
				iNewPos = m_iScrollPos;
			}

			int iMaxScroll = m_iScrollHeight; // +m_uPanelHeight - 1;
			// New position must be between 0 and the screen height. 
			iNewPos = max(0, iNewPos);
			iNewPos = min(iMaxScroll, iNewPos);

			// If the current position does not change, do not scroll.
			if(iNewPos != m_iScrollPos)
			{
				m_iScrollPos = iNewPos;

				m_pScrollBar->setScrollPos(m_iScrollPos);

				preload();

				m_isDirty = true;
			}
		}
		return(0);

	default:
		return(FALSE);
	}
	return(TRUE);
}

void CMaterialBrowser::browse(IMaterialBrowserCallback *pCallback, bool bTextureOnly)
{
	m_pCallback = pCallback;

	bool isDirty = false;
	if(bTextureOnly)
	{
		Button_Enable(GetDlgItem(m_hDlgWnd, IDC_MAT_OPAQUE), FALSE);
		Button_Enable(GetDlgItem(m_hDlgWnd, IDC_MAT_TRANSPARENT), FALSE);
		Button_Enable(GetDlgItem(m_hDlgWnd, IDC_MAT_SELFILLUM), FALSE);
		isDirty |= Button_GetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_MATERIALS)) != BST_UNCHECKED;
		Button_SetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_MATERIALS), BST_UNCHECKED);
		Button_Enable(GetDlgItem(m_hDlgWnd, IDC_MAT_MATERIALS), FALSE);
		Button_Enable(GetDlgItem(m_hDlgWnd, IDC_MAT_EDIT), FALSE);
	}
	else
	{
		Button_Enable(GetDlgItem(m_hDlgWnd, IDC_MAT_OPAQUE), TRUE);
		Button_Enable(GetDlgItem(m_hDlgWnd, IDC_MAT_TRANSPARENT), TRUE);
		Button_Enable(GetDlgItem(m_hDlgWnd, IDC_MAT_SELFILLUM), TRUE);
		isDirty |= Button_GetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_MATERIALS)) != BST_CHECKED;
		Button_SetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_MATERIALS), BST_CHECKED);
		Button_Enable(GetDlgItem(m_hDlgWnd, IDC_MAT_MATERIALS), TRUE);
		Button_Enable(GetDlgItem(m_hDlgWnd, IDC_MAT_EDIT), TRUE);
	}

	if(isDirty)
	{
		filter();
	}

	ShowWindow(m_hDlgWnd, SW_SHOWNA);
	SetFocus(m_hDlgWnd);
}

void CMaterialBrowser::abort()
{
	if(m_pCallback)
	{
		m_pCallback->onCancel();
		m_pCallback = NULL;
	}
	ShowWindow(m_hDlgWnd, SW_HIDE);
}

void CMaterialBrowser::registerClass()
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
	wcex.lpszClassName = "XMaterialBrowser";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	if(!RegisterClassEx(&wcex))
	{
		MessageBox(NULL, "Unable to register window class!", "Error", MB_OK | MB_ICONSTOP);
	}
}

LRESULT CALLBACK CMaterialBrowser::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CMaterialBrowser *pThis = (CMaterialBrowser*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(pThis)
	{
		return(pThis->wndProc(hWnd, msg, wParam, lParam));
	}
	return(DefWindowProc(hWnd, msg, wParam, lParam));
}

LRESULT CALLBACK CMaterialBrowser::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int xPos;
	switch(msg)
	{
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
		xPos = GET_X_LPARAM(lParam);

		SetFocus(hWnd);

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

				xPos = (int)((float)xPos / m_fScale);
				yPos = (int)((float)yPos / m_fScale);

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

		break;
	case WM_LBUTTONUP:
		if(m_isTrackingScroll)
		{
			m_isTrackingScroll = false;
			m_pScrollBar->onMouseUp();
			ReleaseCapture();
		}
		break;
	case WM_MOUSEMOVE:
		{
			xPos = GET_X_LPARAM(lParam);
			bool isHover = m_isTrackingScroll || (m_uPanelWidth - xPos <= m_pScrollBar->getWidth());
			if(isHover != m_isHoverScroll)
			{
				m_isHoverScroll = isHover;
				m_pScrollBar->onMouseHover(isHover);
			}
			if(isHover)
			{
				m_pScrollBar->onMouseMove(GET_Y_LPARAM(lParam));

				if(!m_isTrackingScroll && !m_isTrackingLeaveEvent)
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
		m_isTrackingLeaveEvent = false;
		if(m_isHoverScroll)
		{
			m_isHoverScroll = false;
			m_pScrollBar->onMouseHover(false);
		}
		break;

	case WM_PAINT:
		m_isDirty = true;
		// NO BREAK!
	default:
		return(DefWindowProc(hWnd, msg, wParam, lParam));
	}
	return(0);
}

void CMaterialBrowser::initGraphics(IXRender *pRender)
{
	m_pRender = pRender;
	m_pDev = m_pRender->getDevice();

	m_pScrollbarEvent = new CScrollEventListener(m_hDlgWnd);
	m_pScrollBar = new CScrollBar(pRender, m_pScrollbarEvent);			
	m_pScrollBar->setScale(m_fScale);


	initViewport();

	IXRenderGraph *pRenderGraph;
	m_pRender->getGraph("xMaterialBrowser", &pRenderGraph);
	m_pFinalTarget->attachGraph(pRenderGraph);
	mem_release(pRenderGraph);

	initHelpers();


	m_pMaterialSystem = (IXMaterialSystem*)Core_GetIXCore()->getPluginManager()->getInterface(IXMATERIALSYSTEM_GUID);

	m_pFontManager = (IXFontManager*)Core_GetIXCore()->getPluginManager()->getInterface(IXFONTMANAGER_GUID);
	initFont();

	filter();
}
void CMaterialBrowser::render()
{
	//if(IsWindowVisible(m_hBrowserWnd) && m_isDirty)
	{
		IGXContext *pCtx = m_pDev->getThreadContext();
		IGXSurface *pOldRT = pCtx->getColorTarget();
		pCtx->setColorTarget(m_pSurface);
		//IGXDepthStencilSurface *pOldDS = pCtx->getDepthStencilSurface();
		pCtx->setDepthStencilSurface(NULL);

		pCtx->clear(GX_CLEAR_COLOR, float4(0, 0, 0, 0));

		pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);
		pCtx->setRasterizerState(NULL);
		pCtx->setBlendState(g_xRenderStates.pBlendAlpha);

		pCtx->setVSConstant(m_pTransformCB, SCR_CAMERA);

		preload();

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
			m_pTextOffsetCB->update(&float4_t(0.0f, (float)m_iScrollPos * m_fScale, 0.0f, 0.0f));
			pCtx->setVSConstant(m_pTextOffsetCB, 6);
			m_pRender->bindShader(pCtx, m_idTextShader);
			pCtx->drawIndexed(m_uTextVertexCount, m_uTextQuadCount * 2);
			m_pRender->unbindShader(pCtx);
		}

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
void CMaterialBrowser::swapBuffers()
{
	if(IsWindowVisible(m_hBrowserWnd) && m_bDoSwap)
	{
		//m_pSwapChain->swapBuffers();
		m_bDoSwap = false;
	}
}

void CMaterialBrowser::initViewport()
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
		m_pRender->newFinalTarget(m_hBrowserWnd, "xMaterialBrowser", &m_pFinalTarget);
	}
	m_pFinalTarget->resize(m_uPanelWidth, m_uPanelHeight);

	float fWidth = (float)m_uPanelWidth;
	float fHeight = (float)m_uPanelHeight;

	SMMATRIX mProj = SMMatrixTranslation(-0.5f, -0.5f, 0.0f) * SMMatrixOrthographicLH(fWidth, fHeight, 1.0f, 2000.0f);
	SMMATRIX mView = SMMatrixLookToLH(float3(fWidth * 0.5f, -fHeight * 0.5f, -1.0f), float3(0.0f, 0.0f, 1.0f), float3(0.0f, 1.0f, 0.0f));

	if(!m_pTransformCB)
	{
		m_pTransformCB = m_pDev->createConstantBuffer(sizeof(SMMATRIX));
	}
	m_pTransformCB->update(&SMMatrixTranspose(mView * mProj));

	layout();
}

void CMaterialBrowser::initHelpers()
{
	struct FrameVertex
	{
		float2_t vPos;
		float2_t vScaleFactor;
		float4_t vColor;
		float fHighlightFactor;
		float fTitleFactor;
	};

	float4_t fBlueColor(198.0f / 255.0f, 240.0f / 255.0f, 253.0f / 255.0f, 144.0f / 255.0f);
	float4_t fRedColor(1.0f, 187.0f / 255.0f, 71.0f / 255.0f, 204.0f / 255.0f);

	FrameVertex aVertices[] = {
		// object innermost
		{float2_t(113.9631f, -18.1490f),  float2_t(1.0f, 0.0f), fRedColor, 0.0f, 0.0f},
		{float2_t(3.2289f,   -18.1259f),  float2_t(0.0f, 0.0f), fRedColor, 0.0f, 0.0f},
		{float2_t(4.4848f,   -19.3785f),  float2_t(0.0f, 0.0f), fRedColor, 0.0f, 0.0f},
		{float2_t(132.4848f, -19.3785f),  float2_t(1.0f, 0.0f), fRedColor, 0.0f, 0.0f},
		{float2_t(135.0321f, -18.3394f),  float2_t(1.0f, 0.0f), fRedColor, 0.0f, 0.0f},
		{float2_t(133.6069f, -16.9113f),  float2_t(1.0f, 0.0f), fRedColor, 0.0f, 0.0f},
		{float2_t(115.1857f, -16.9113f),  float2_t(1.0f, 0.0f), fRedColor, 0.0f, 0.0f},
		{float2_t(135.0321f, -37.1310f),  float2_t(1.0f, 0.0f), fRedColor, 0.0f, 0.0f},
		{float2_t(132.4848f, -147.3785f), float2_t(1.0f, 1.0f), fRedColor, 0.0f, 0.0f},
		{float2_t(133.8319f, -148.7631f), float2_t(1.0f, 1.0f), fRedColor, 0.0f, 0.0f},
		{float2_t(133.7938f, -38.3808f),  float2_t(1.0f, 0.0f), fRedColor, 0.0f, 0.0f},
		{float2_t(4.0426f,  -150.0456f),  float2_t(0.0f, 1.0f), fRedColor, 0.0f, 0.0f},
		{float2_t(4.4848f,  -147.3785f),  float2_t(0.0f, 1.0f), fRedColor, 0.0f, 0.0f},
		{float2_t(3.2172f,  -128.5050f),  float2_t(0.0f, 1.0f), fRedColor, 0.0f, 0.0f},
		{float2_t(2.0573f,  -129.6658f),  float2_t(0.0f, 1.0f), fRedColor, 0.0f, 0.0f},
		{float2_t(2.0574f,  -148.0175f),  float2_t(0.0f, 1.0f), fRedColor, 0.0f, 0.0f},
		{float2_t(22.9638f, -148.8071f),  float2_t(0.0f, 1.0f), fRedColor, 0.0f, 0.0f},
		{float2_t(21.7437f, -150.0456f),  float2_t(0.0f, 1.0f), fRedColor, 0.0f, 0.0f},

		// object title
		{float2_t(0.0386f,   -67.3793f), float2_t(0.0f, 0.3f), fBlueColor, 0.0f, 0.0f},
		{float2_t(3.2212f,   -70.5707f), float2_t(0.0f, 0.3f), fBlueColor, 0.0f, 0.0f},
		{float2_t(3.2235f,   -68.9292f), float2_t(0.0f, 0.3f), fBlueColor, 0.0f, 0.0f},
		{float2_t(1.0994f,   -66.8128f), float2_t(0.0f, 0.3f), fBlueColor, 0.0f, 0.0f},
		{float2_t(133.8268f, -68.4960f), float2_t(1.0f, 0.3f), fBlueColor, 0.0f, 0.0f},
		{float2_t(133.8285f, -70.5891f), float2_t(1.0f, 0.3f), fBlueColor, 0.0f, 0.0f},
		{float2_t(138.4990f, -65.9372f), float2_t(1.0f, 0.3f), fBlueColor, 0.0f, 0.0f},
		{float2_t(137.1619f, -65.0212f), float2_t(1.0f, 0.3f), fBlueColor, 0.0f, 0.0f},
		{float2_t(0.0317f,   -11.5093f), float2_t(0.0f, 0.0f), fBlueColor, 0.0f, 0.0f},
		{float2_t(1.1019f,   -15.6547f), float2_t(0.0f, 0.0f), fBlueColor, 0.0f, 0.0f},
		{float2_t(1.1264f,   -14.7688f), float2_t(0.0f, 0.0f), fBlueColor, 0.0f, 0.0f},
		{float2_t(1.1265f,   -11.9992f), float2_t(0.0f, 0.0f), fBlueColor, 0.0f, 0.0f},
		{float2_t(138.4984f, -16.2037f), float2_t(1.0f, 0.0f), fBlueColor, 0.0f, 0.0f},
		{float2_t(137.1215f, -17.0767f), float2_t(1.0f, 0.0f), fBlueColor, 0.0f, 0.0f},
		{float2_t(6.4113f,    -0.0172f), float2_t(0.0f, 0.0f), fBlueColor, 0.0f, 0.0f},
		{float2_t(6.9560f,    -1.0464f), float2_t(0.0f, 0.0f), fBlueColor, 0.0f, 0.0f},
		{float2_t(68.5653f,   -0.0488f), float2_t(0.0f, 0.0f), fBlueColor, 0.0f, 1.0f},
		{float2_t(67.9023f,   -1.0475f), float2_t(0.0f, 0.0f), fBlueColor, 0.0f, 1.0f},
		{float2_t(74.0843f,  -10.6504f), float2_t(0.0f, 0.0f), fBlueColor, 0.0f, 1.0f},
		{float2_t(73.4461f,  -11.7053f), float2_t(0.0f, 0.0f), fBlueColor, 0.0f, 1.0f},
		{float2_t(111.2503f, -11.7231f), float2_t(1.0f, 0.0f), fBlueColor, 0.0f, 0.0f},
		{float2_t(111.9313f, -10.6424f), float2_t(1.0f, 0.0f), fBlueColor, 0.0f, 0.0f},
		{float2_t(114.1821f, -12.7673f), float2_t(1.0f, 0.0f), fBlueColor, 0.0f, 0.0f},
		{float2_t(113.0957f, -13.5664f), float2_t(1.0f, 0.0f), fBlueColor, 0.0f, 0.0f},
		{float2_t(135.0677f, -12.7658f), float2_t(1.0f, 0.0f), fBlueColor, 0.0f, 0.0f},
		{float2_t(134.2801f, -14.1879f), float2_t(1.0f, 0.0f), fBlueColor, 0.0f, 0.0f},
		{float2_t(114.0449f, -14.1871f), float2_t(1.0f, 0.0f), fBlueColor, 0.0f, 0.0f},
		{float2_t(112.8880f, -15.5461f), float2_t(1.0f, 0.0f), fBlueColor, 0.0f, 0.0f},
		{float2_t(112.0354f, -14.8258f), float2_t(1.0f, 0.0f), fBlueColor, 0.0f, 0.0f},
	
		// object selector
		{float2_t(124.4909f,   2.9029f), float2_t(1.0f, 0.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(123.8050f,   1.8588f), float2_t(1.0f, 0.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(124.4114f,   0.8322f), float2_t(1.0f, 0.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(138.4736f,   0.8288f), float2_t(1.0f, 0.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(139.1122f,   2.9022f), float2_t(1.0f, 0.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(141.4260f,   0.6073f), float2_t(1.0f, 0.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(139.6764f,  -0.3527f), float2_t(1.0f, 0.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(141.4661f,  -7.6108f), float2_t(1.0f, 0.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(139.7170f,  -7.6079f), float2_t(1.0f, 0.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(140.5804f,  -8.1092f), float2_t(1.0f, 0.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(16.0975f, -153.3459f), float2_t(0.0f, 1.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(16.7834f, -152.3019f), float2_t(0.0f, 1.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(16.1770f, -151.2752f), float2_t(0.0f, 1.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(2.1148f,  -151.2719f), float2_t(0.0f, 1.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(1.4762f,  -153.3453f), float2_t(0.0f, 1.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(-0.8376f, -151.0503f), float2_t(0.0f, 1.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(0.9120f,  -150.0904f), float2_t(0.0f, 1.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(-0.8777f, -142.8323f), float2_t(0.0f, 1.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(0.8714f,  -142.8352f), float2_t(0.0f, 1.0f), fRedColor, 1.0f, 0.0f},
		{float2_t(0.0080f,  -142.3339f), float2_t(0.0f, 1.0f), fRedColor, 1.0f, 0.0f}
	};

	m_uFrameVC = ARRAYSIZE(aVertices);

	USHORT aIndices[] = {
		2, 1, 3,
		4, 3, 5,
		6, 5, 7,
		3, 1, 5,
		7, 5, 1,
		5, 8, 4,
		9, 4, 10,
		8, 11, 4,
		10, 4, 11,
		13, 12, 3,
		2, 3, 14,
		15, 14, 16,
		3, 12, 14,
		16, 14, 12,
		10, 17, 9,
		12, 13, 18,
		9, 17, 13,
		18, 13, 17,
		20, 19, 21,
		21, 19, 22,
		24, 23, 25,
		23, 26, 25,
		19, 27, 22,
		22, 27, 28,
		28, 27, 29,
		29, 27, 30,
		25, 26, 31,
		26, 32, 31,
		27, 33, 30,
		30, 33, 34,
		33, 35, 34,
		34, 35, 36,
		35, 37, 36,
		36, 37, 38,
		38, 37, 39,
		37, 40, 39,
		40, 41, 39,
		39, 41, 42,
		31, 32, 43,
		32, 44, 43,
		43, 44, 41,
		44, 45, 41,
		45, 46, 41,
		41, 46, 42,
		42, 46, 47,
		46, 28, 47,
		47, 28, 29,
		49, 48, 50,
		50, 48, 51,
		48, 52, 51,
		52, 53, 51,
		51, 53, 54,
		53, 55, 54,
		54, 55, 56,
		55, 57, 56,
		59, 58, 60,
		60, 58, 61,
		58, 62, 61,
		62, 63, 61,
		61, 63, 64,
		63, 65, 64,
		64, 65, 66,
		65, 67, 66
	};

	for(UINT i = 0, l = ARRAYSIZE(aIndices); i < l; ++i)
	{
		--aIndices[i];
	}

	m_uFramePC = ARRAYSIZE(aIndices) / 3;

	GXVertexElement oLayout[] =
	{
		{0, 0, GXDECLTYPE_FLOAT2, GXDECLUSAGE_POSITION},
		{0, 8, GXDECLTYPE_FLOAT2, GXDECLUSAGE_TEXCOORD},
		{0, 16, GXDECLTYPE_FLOAT4, GXDECLUSAGE_TEXCOORD1},
		{0, 32, GXDECLTYPE_FLOAT1, GXDECLUSAGE_TEXCOORD2},
		{0, 36, GXDECLTYPE_FLOAT1, GXDECLUSAGE_TEXCOORD3},
		GX_DECL_END()
	};
	IGXVertexDeclaration *pVD = m_pDev->createVertexDeclaration(oLayout);

	m_pFrameIB = m_pDev->createIndexBuffer(sizeof(USHORT) * ARRAYSIZE(aIndices), GXBUFFER_USAGE_STATIC, GXIT_UINT16, aIndices);

	IGXVertexBuffer *pVB = m_pDev->createVertexBuffer(sizeof(FrameVertex) * ARRAYSIZE(aVertices), GXBUFFER_USAGE_STATIC, aVertices);
	
	m_pFrameRB = m_pDev->createRenderBuffer(1, &pVB, pVD);

	mem_release(pVB);
	mem_release(pVD);

	m_idFrameShader = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "terrax_mat_frame.vs"), m_pRender->loadShader(SHADER_TYPE_PIXEL, "terrax_colored2.ps"));

	m_pInformCB = m_pDev->createConstantBuffer(sizeof(m_frameState));


	struct InnerVertex
	{
		float2_t vPos;
		float2_t vScaleFactor;
		float2_t vTexCoord;
	};

	InnerVertex aVertices2[] = {
		float2_t(4.4848f,    -19.3785f), float2_t(0.0f, 0.0f), float2_t(0.0f, 0.0f),
		float2_t(4.4848f,   -147.3785f), float2_t(0.0f, 1.0f), float2_t(0.0f, 1.0f),
		float2_t(132.4848f,  -19.3785f), float2_t(1.0f, 0.0f), float2_t(1.0f, 0.0f),
		float2_t(132.4848f, -147.3785f), float2_t(1.0f, 1.0f), float2_t(1.0f, 1.0f)
	};

	m_uInnerVC = ARRAYSIZE(aVertices2);

	USHORT aIndices2[] = {
		1, 3, 2, 4, 2, 3
	};

	for(UINT i = 0, l = ARRAYSIZE(aIndices2); i < l; ++i)
	{
		--aIndices2[i];
	}

	m_uInnerPC = ARRAYSIZE(aIndices2) / 3;

	GXVertexElement oLayout2[] =
	{
		{0, 0, GXDECLTYPE_FLOAT2, GXDECLUSAGE_POSITION},
		{0, 8, GXDECLTYPE_FLOAT2, GXDECLUSAGE_TEXCOORD},
		{0, 16, GXDECLTYPE_FLOAT2, GXDECLUSAGE_TEXCOORD1},
		GX_DECL_END()
	};
	pVD = m_pDev->createVertexDeclaration(oLayout2);

	m_pInnerIB = m_pDev->createIndexBuffer(sizeof(USHORT) * ARRAYSIZE(aIndices2), GXBUFFER_USAGE_STATIC, GXIT_UINT16, aIndices2);

	pVB = m_pDev->createVertexBuffer(sizeof(InnerVertex) * ARRAYSIZE(aVertices2), GXBUFFER_USAGE_STATIC, aVertices2);

	m_pInnerRB = m_pDev->createRenderBuffer(1, &pVB, pVD);

	mem_release(pVB);
	mem_release(pVD);


	m_idInnerShader = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "terrax_mat_inner.vs"), m_pRender->loadShader(SHADER_TYPE_PIXEL, "terrax_mat_inner.ps"));

	m_idTextShader = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "gui_text.vs"), m_pRender->loadShader(SHADER_TYPE_PIXEL, "gui_main.ps"));

	m_pTextColorCB = m_pDev->createConstantBuffer(sizeof(float4_t));
	fBlueColor.w = 1.0f;
	m_pTextColorCB->update(&fBlueColor);
	m_pTextOffsetCB = m_pDev->createConstantBuffer(sizeof(float4_t));
}

void CMaterialBrowser::drawFrame(int iXpos, int iYpos, FRAME_SIZE frameSize, UINT uTitleLen, float fSelection)
{
	IGXContext *pCtx = m_pDev->getThreadContext();

	m_pRender->bindShader(pCtx, m_idFrameShader);

	float vSizeDiff = (float)(frameSize - 128);

	float fTitleMax = vSizeDiff + 39.0f;

	m_frameState.vSize = float2_t(vSizeDiff, -vSizeDiff);
	m_frameState.fTitleSize = (float)uTitleLen - 60.0f;
	if(m_frameState.fTitleSize > fTitleMax)
	{
		m_frameState.fTitleSize = fTitleMax;
	}

	m_frameState.fHighlight = fSelection;
	m_frameState.vPosition = float2_t((float)iXpos, (float)-iYpos + (float)m_iScrollPos);

	m_frameState.fScale = m_fScale;

	m_pInformCB->update(&m_frameState);
	pCtx->setVSConstant(m_pInformCB, 6);

	pCtx->setIndexBuffer(m_pFrameIB);
	pCtx->setRenderBuffer(m_pFrameRB);
	pCtx->drawIndexed(m_uFrameVC, m_uFramePC);

	m_pRender->unbindShader(pCtx);
}

void CMaterialBrowser::layout()
{
	const UINT c_uXInterval = 17 + m_frameSize;
	const UINT c_uYInterval = 27 + m_frameSize;

	UINT uXVal = 0;
	UINT uYVal = 0;

	UINT uPanelWidth = (UINT)((float)m_uPanelWidth / m_fScale);

	for(UINT i = 0, l = m_aMaterials.size(); i < l; ++i)
	{
		MaterialItem &item = m_aMaterials[i];

		item.uXpos = uXVal;
		item.uYpos = uYVal;

		uXVal += c_uXInterval;
		if(uXVal + c_uXInterval >= uPanelWidth - m_pScrollBar->getWidth())
		{
			uXVal = 0;
			uYVal += c_uYInterval;
		}
	}

	if(uXVal == 0)
	{
		uYVal -= c_uYInterval;
	}


	m_iScrollHeight = (int)(uYVal + c_uYInterval) - (int)(m_uPanelHeight / m_fScale);
	if(m_iScrollHeight < 0)
	{
		m_iScrollHeight = 0;
	}

	if(m_iScrollPos > m_iScrollHeight)
	{
		m_iScrollPos = m_iScrollHeight;
	}

	m_pScrollBar->setPos(m_uPanelWidth - m_pScrollBar->getWidth(), 0);
	m_pScrollBar->setSize(m_uPanelHeight);
	m_pScrollBar->setScrollMax(m_iScrollHeight);
	m_pScrollBar->setScrollPos(m_iScrollPos);
	m_pScrollBar->setPageSize(m_uPanelHeight);

	preload();

	m_isDirty = true;
}

void CMaterialBrowser::filter()
{
	UINT uListSize = m_pMaterialSystem->getScannedMaterialsCount();

	IXMaterial *pMat;
	bool isTexture, isTranslated, isMaterial;
	const char *szName; 
	

	for(UINT i = 0, l = m_aMaterials.size(); i < l; ++i)
	{
		mem_release(m_aMaterials[i].pTexture);
		mem_release(m_aMaterials[i].pMaterial);
		mem_delete_a(m_aMaterials[i].pVertices);
		m_aMaterials[i].uQuads = 0;
	}

	m_aMaterials.clearFast();

	char szFilter[256];
	GetDlgItemText(m_hDlgWnd, IDC_MAT_FILTER, szFilter, sizeof(szFilter));
	szFilter[sizeof(szFilter) - 1] = 0;

	bool isIncludeTransparent = !IsWindowEnabled(GetDlgItem(m_hDlgWnd, IDC_MAT_TRANSPARENT)) || Button_GetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_TRANSPARENT)) == BST_CHECKED;
	bool isIncludeOpaque = !IsWindowEnabled(GetDlgItem(m_hDlgWnd, IDC_MAT_OPAQUE)) || Button_GetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_OPAQUE)) == BST_CHECKED;
	bool isIncludeSelfillum = !IsWindowEnabled(GetDlgItem(m_hDlgWnd, IDC_MAT_SELFILLUM)) || Button_GetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_SELFILLUM)) == BST_CHECKED;

	bool isMaterialF = Button_GetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_MATERIALS)) == BST_CHECKED;
	bool isTextureF = Button_GetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_TEXTURES)) == BST_CHECKED;
	bool isTranslatedF = Button_GetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_TRANSLATED)) == BST_CHECKED;
	bool isRawfile = Button_GetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_RAWFILES)) == BST_CHECKED;
	bool isOnlyUsed = Button_GetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_USEDONLY)) == BST_CHECKED;

	for(UINT i = 0; i < uListSize; ++i)
	{
		szName = m_pMaterialSystem->getScannedMaterial(i, &pMat, &isTexture, &isTranslated, &isMaterial);

		if(strcasestr(szName, szFilter))
		{
			bool isTransparent = pMat && pMat->isTransparent();
			bool isOpaque = !isTransparent;

			if(
				(
					isMaterialF && isMaterial ||
					isTextureF && isTexture
				) &&
				(
					isTranslatedF && isTranslated ||
					isRawfile && !isTranslated
				) &&
				(
					isIncludeOpaque && isOpaque ||
					isIncludeTransparent && isTransparent ||
					isIncludeSelfillum && (pMat && pMat->isEmissive())
				) && (
					!isOnlyUsed || m_pMaterialSystem->isMaterialLoaded(szName)
				)
			)
			{
				MaterialItem &item = m_aMaterials[m_aMaterials.size()];
				item.sName = szName;
				item.isTexture = isTexture;
				item.isMaterial = isMaterial;
				item.isTranslated = isTranslated;
				item.pMaterial = pMat;
				item.pTexture = NULL;
				item.pVertices = NULL;

				item.isTransparent = isTransparent;
			}
		}
	}

	m_uFontGenStart = ~0;
	m_uFontGenEnd = ~0;
	m_uTextQuadCount = 0;

	m_uSelectedItem = ~0;

	layout();
}

void CMaterialBrowser::preload()
{
	std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now() + std::chrono::microseconds(5000);

	XFontBuildParams xfbp;
	XFontStringMetrics xfsm = {};

	UINT uStartItem = ~0;
	UINT uEndItem = ~0;


	UINT uMaxTitle = (UINT)((float)(m_frameSize - 39) * m_fScale);
	UINT uTotalQuads = 0;

	int iYStart = m_iScrollPos - m_frameSize * 3;
	int iYEnd = m_iScrollPos + m_uPanelHeight + m_frameSize * 3;
	for(UINT i = 0, l = m_aMaterials.size(); i < l; ++i)
	{
		MaterialItem &item = m_aMaterials[i];
		if((int)item.uYpos >= iYStart && item.uYpos <= iYEnd)
		{
			if(!item.pVertices)
			{
				m_aCharRects.resizeFast(item.sName.length());

				xfbp.pVertices = NULL;
				xfbp.pCharRects = m_aCharRects;
				m_pFont->buildString(item.sName.c_str(), xfbp, &xfsm);
				item.pVertices = new XFontVertex[xfsm.uVertexCount];
				xfbp.pVertices = item.pVertices;
				m_pFont->buildString(item.sName.c_str(), xfbp, &xfsm);

				UINT ch;
				XCharRect *cr = m_aCharRects;
				for(UINT j = 0; j < xfsm.uQuadCount; ++j)
				{
					ch = cr->uChar;

					if(cr->iLeft > uMaxTitle)
					{
						break;
					}
					item.uTitleWidth = cr->iRight;
					++cr;
				}
				item.uQuads = cr - m_aCharRects;
				item.uTitleWidth = (UINT)((float)item.uTitleWidth / m_fScale);
			}
			uTotalQuads += item.uQuads;

			if(uStartItem == ~0)
			{
				uStartItem = i;
			}
			uEndItem = i;
		}
		else
		{
			mem_delete_a(item.pVertices);
		}
	}

	if(m_uFontGenStart != uStartItem || m_uFontGenEnd != uEndItem)
	{
		m_uFontGenStart = uStartItem;
		m_uFontGenEnd = uEndItem;

		if(uStartItem != ~0)
		{
			// generate mesh

			m_aTextVB.clearFast();
			m_aTextVB.reserve(uTotalQuads * 4);
			for(UINT i = uStartItem; i <= uEndItem; ++i)
			{
				MaterialItem &item = m_aMaterials[i];
				for(UINT j = 0; j < item.uQuads; ++j)
				{
					for(UINT k = 0; k < 4; ++k)
					{
						m_aTextVB.push_back(item.pVertices[j * 4 + k]);
						m_aTextVB[m_aTextVB.size() - 1].vPos.y *= -1.0f;
						m_aTextVB[m_aTextVB.size() - 1].vPos.x += (item.uXpos + 7) * m_fScale;
						m_aTextVB[m_aTextVB.size() - 1].vPos.y -= item.uYpos * m_fScale;
					}
				}
			}

			m_uTextVertexCount = uTotalQuads * 4;

			if(m_uTextRBSize < m_uTextVertexCount)
			{
				m_uTextRBSize = m_uTextVertexCount;
				mem_release(m_pTextRB);
				mem_release(m_pTextVB);
				m_pTextVB = m_pDev->createVertexBuffer(sizeof(XFontGPUVertex) * m_uTextVertexCount, GXBUFFER_USAGE_STREAM, m_aTextVB);

				m_pTextRB = m_pDev->createRenderBuffer(1, &m_pTextVB, m_pTextVD);
			}
			else
			{
				void *pData;
				if(m_pTextVB->lock(&pData, GXBL_WRITE))
				{
					memcpy(pData, m_aTextVB, sizeof(XFontGPUVertex) * m_uTextVertexCount);
					m_pTextVB->unlock();
				}
			}

			mem_release(m_pTextIB);
			m_pFontManager->getFontIndexBuffer(uTotalQuads, &m_pTextIB);
			m_uTextQuadCount = uTotalQuads;
		}
		else
		{
			m_uTextQuadCount = 0;
		}
	}

	for(UINT i = 0, l = m_aMaterials.size(); i < l; ++i)
	{
		MaterialItem &item = m_aMaterials[i];
		if((int)item.uYpos >= iYStart && item.uYpos <= iYEnd)
		{
			if(!item.pTexture)
			{
				if(item.pMaterial)
				{
					IKeyIterator *pIter = NULL;
					const char *szTexture = item.pMaterial->getTextureName("txBase");
					if(!szTexture)
					{
						pIter = item.pMaterial->getTexturesIterator();
						if(pIter)
						{
							szTexture = pIter->getCurrent();
						}
					}

					if(szTexture)
					{
						m_pMaterialSystem->loadTexture(szTexture, &item.pTexture);
					}

					mem_release(pIter);
				}
				else
				{
					m_pMaterialSystem->loadTexture((item.sName + "|from_srgb").c_str(), &item.pTexture);
				}

				item.uCurrentFrame = 0;
				item.uTotalFrames = 1;
				item.fCurrentTime = 0.0f;

				if(item.pTexture)
				{
					item.uTotalFrames = item.pTexture->getNumFrames();
					item.fFrameTime = item.pTexture->getFrameTime();
				}

				if(std::chrono::steady_clock::now() > tp)
				{
					break;
				}
			}
		}
		else
		{
			mem_release(item.pTexture);
		}
	}
}

void CMaterialBrowser::update(float dt)
{
	if(m_isFilterScheduled)
	{
		m_fFilterTimer += dt;
		if(m_fFilterTimer >= m_fFilterAt)
		{
			m_isFilterScheduled = false;
			filter();
		}
	}

	m_pScrollBar->update(dt);

	for(UINT i = 0, l = m_aMaterials.size(); i < l; ++i)
	{
		MaterialItem &item = m_aMaterials[i];
		if(item.pTexture && item.uTotalFrames > 1)
		{
			item.fCurrentTime += dt;
			if(item.fCurrentTime > item.fFrameTime)
			{
				item.fCurrentTime -= item.fFrameTime;

				++item.uCurrentFrame;
				item.uCurrentFrame %= item.uTotalFrames;
			}
		}
	}
}

void CMaterialBrowser::scheduleFilterIn(float fSeconds)
{
	m_fFilterTimer = 0.0f;
	m_fFilterAt = fSeconds;
	m_isFilterScheduled = true;
}

void CMaterialBrowser::initFont()
{
	if(m_pFontManager)
	{
		mem_release(m_pFont);
		m_pFontManager->getFont(&m_pFont, "gui/fonts/tahoma.ttf", (UINT)(10.0f * m_fScale));
		m_pFontManager->getFontVertexDeclaration(&m_pTextVD);

		invalidateTexts();
	}
}
void CMaterialBrowser::invalidateTexts()
{
	m_uFontGenStart = ~0;
	m_uFontGenEnd = ~0;

	for(UINT i = 0, l = m_aMaterials.size(); i < l; ++i)
	{
		mem_delete_a(m_aMaterials[i].pVertices);
	}
}

//#############################################################################

CScrollEventListener::CScrollEventListener(HWND hWnd):
	m_hWnd(hWnd)
{
}

void CScrollEventListener::onPageUp()
{
	SendMessage(m_hWnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEUP, 0), NULL);
}
void CScrollEventListener::onPageDown()
{
	SendMessage(m_hWnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEDOWN, 0), NULL);
}
void CScrollEventListener::onLineUp()
{
	SendMessage(m_hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), NULL);
}
void CScrollEventListener::onLineDown()
{
	SendMessage(m_hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), NULL);
}
void CScrollEventListener::onScroll(int iScroll)
{
	SendMessage(m_hWnd, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, iScroll), NULL);
}
