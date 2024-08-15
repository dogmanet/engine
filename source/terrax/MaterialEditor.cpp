#include "MaterialEditor.h"
#include "resource.h"

#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <xcommon/IXTextureFilter.h>
#include "terrax.h"
#include <xcommon/IPluginManager.h>

WNDPROC g_pfnTrackbarOldWndproc = NULL;

void GetChildRect(HWND hWnd, LPRECT lpRect);

CMaterialEditor::CMaterialEditor(HINSTANCE hInstance, HWND hMainWnd, IXCore *pCore, IXMaterial *pMaterial):
	m_hInstance(hInstance),
	m_hMainWnd(hMainWnd),
	m_cbTextureBrowser(this),
	m_pMaterial(pMaterial)
{
	add_ref(pMaterial);

	backup();

	//registerClass();
	IPluginManager *pPM = pCore->getPluginManager();

	m_hTextureMenu = CreatePopupMenu();
	m_hTextureOptMenu = CreatePopupMenu();
	AppendMenuA(m_hTextureMenu, MF_STRING, ID_MAT_BROWSER, "Browse...");
	AppendMenuA(m_hTextureOptMenu, MF_STRING, ID_MAT_BROWSER, "Browse...");
	AppendMenuA(m_hTextureOptMenu, MF_STRING, ID_EDIT_DELETE, "Clear");
	AppendMenuA(m_hTextureMenu, MF_SEPARATOR, 0, NULL);
	AppendMenuA(m_hTextureOptMenu, MF_SEPARATOR, 0, NULL);

	IXTextureFilter *pFilter;
	UINT ic = 0;
	while((pFilter = (IXTextureFilter*)pPM->getInterface(IXTEXTUREFILTER_GUID, ic++)))
	{
		CMB2WC wcs(pFilter->getDescription());
		AppendMenuW(m_hTextureMenu, MF_STRING, 3000 + (ic - 1), wcs);
		AppendMenuW(m_hTextureOptMenu, MF_STRING, 3000 + (ic - 1), wcs);
		m_aszFilters.push_back(pFilter->getName());
	}

	m_pMaterialSystem = (IXMaterialSystem*)pPM->getInterface(IXMATERIALSYSTEM_GUID);
	
	CreateDialogParamA(m_hInstance, MAKEINTRESOURCE(IDD_MAT_EDIT), hMainWnd, DlgProc, (LPARAM)this);
	ShowWindow(m_hDlgWnd, SW_SHOWNA);
	SetFocus(m_hDlgWnd);

	initFields();
	m_isDirty = false;
}

CMaterialEditor::~CMaterialEditor()
{
	DestroyMenu(m_hTextureMenu);
	DestroyMenu(m_hTextureOptMenu);
	mem_release(m_pMaterial);
}

INT_PTR CALLBACK CMaterialEditor::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CMaterialEditor *pThis = (CMaterialEditor*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lParam);
		pThis = (CMaterialEditor*)lParam;
		pThis->m_hDlgWnd = hWnd;
	}
	else if(msg == WM_DESTROY)
	{
		pThis->m_hDlgWnd = NULL;
		mem_release(pThis);
	}
	if(pThis)
	{
		return(pThis->dlgProc(hWnd, msg, wParam, lParam));
	}
	return(FALSE);
}

INT_PTR CALLBACK CMaterialEditor::dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			m_hPropsWnd = GetDlgItem(hWnd, IDC_PROPS);
			SetWindowLongPtr(m_hPropsWnd, GWLP_USERDATA, (LONG_PTR)this);
			
			m_aBottomList.push_back({GetDlgItem(hWnd, IDOK)});
			m_aBottomList.push_back({GetDlgItem(hWnd, IDCANCEL)});

			RECT rc, rcChild;
			GetClientRect(hWnd, &rc);
			for(UINT i = 0, l = m_aBottomList.size(); i < l; ++i)
			{
				GetChildRect(m_aBottomList[i].hWnd, &rcChild);
				m_aBottomList[i].uDeltaPos = rc.bottom - rcChild.top;
			}
			
			GetChildRect(m_hPropsWnd, &rcChild);
			m_uPanelDeltaX = rc.right - rcChild.right;
			m_uPanelDeltaY = rc.bottom - rcChild.bottom;

			GetWindowRect(hWnd, &rc);
			m_uMinHeight = rc.bottom - rc.top;
			m_uFixedWidth = rc.right - rc.left;

			SetWindowLongPtr(m_hPropsWnd, DWLP_DLGPROC, (LONG_PTR)InnerDlgProc);

			HWND hShaderCombo = GetDlgItem(hWnd, IDC_COMBO_SHADER);

			IKeyIterator *pIter;
			m_pMaterialSystem->getMaterialShadersIterator(&pIter);
			while(!pIter->isEnd())
			{
				ComboBox_AddString(hShaderCombo, pIter->getCurrent());
				pIter->getNext();
			}
			mem_release(pIter);

			int idx = ComboBox_FindStringExact(hShaderCombo, 0, m_pMaterial->getShader());
			ComboBox_SetCurSel(hShaderCombo, idx);

			const char *szMatName = m_pMaterial->getName();
			const char *szTitleFormat = "%s - Material Editor";
			char *szWindowTitle = (char*)alloca(sizeof(char)* (strlen(szTitleFormat) + strlen(szMatName) + 1));
			sprintf(szWindowTitle, szTitleFormat, szMatName);
			SetWindowTextW(hWnd, CMB2WC(szWindowTitle));
			
			SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_DLGMODALFRAME);


			HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_PHYSTYPE);
			//MTLTYPE_PHYSIC aTypes;
			ComboBox_AddString(hCombo, "Concrete");
			ComboBox_AddString(hCombo, "Metal");
			ComboBox_AddString(hCombo, "Glass");
			ComboBox_AddString(hCombo, "Plastic");
			ComboBox_AddString(hCombo, "Wood");
			ComboBox_AddString(hCombo, "Flesh");
			ComboBox_AddString(hCombo, "Ground/sand");
			ComboBox_AddString(hCombo, "Water");
			ComboBox_AddString(hCombo, "Leaf/grass");
			assert(ComboBox_GetCount(hCombo) == MPT_COUNT);

			ComboBox_SetCurSel(hCombo, m_pMaterial->getPhysicsType());
			
			//HWND hBtn = CreateWindowExA(0, WC_BUTTON, "Test", WS_VISIBLE | WS_CHILD, 10, 150, 50, 25, m_hPropsWnd, (HMENU)ID_MAT_BROWSER, m_hInstance, NULL);
			//SetWindowFont(hBtn, GetStockObject(DEFAULT_GUI_FONT), FALSE);
			
			/*
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
			*/
			break;
		}

	case WM_MOUSEWHEEL:
		SendMessage(m_hPropsWnd, msg, wParam, lParam);
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_COMBO_SHADER:
			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				HWND hCombo = (HWND)lParam;
				int iLen = ComboBox_GetTextLength(hCombo);
				char *szShader = (char*)alloca(sizeof(char)* (iLen + 1));
				if(iLen)
				{
					ComboBox_GetText(hCombo, szShader, iLen + 1);
					m_pMaterial->setShader(szShader);
					initFields();
				}
				m_isDirty = true;
			}
			break;

		case IDC_COMBO_PHYSTYPE:
			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				HWND hCombo = (HWND)lParam;
				MTLTYPE_PHYSIC type = (MTLTYPE_PHYSIC)ComboBox_GetCurSel(hCombo);
				if(type >= 0 && type < MPT_COUNT && type != m_pMaterial->getPhysicsType())
				{
					m_pMaterial->setPhysicsType(type);
					m_isDirty = true;
				}
			}
			break;

		case IDOK:
			if(save())
			{
				PostMessage(hWnd, WM_CLOSE, 0, 0);
			}
			break;

		case IDCANCEL:
			undo();
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		}
		break;

	case WM_LBUTTONDOWN:
		SetFocus(hWnd);
		return(TRUE);

	case WM_NCHITTEST:
		{
			LRESULT result = DefWindowProc(hWnd, msg, wParam, lParam);
			switch(result)
			{
			case HTTOPLEFT:
			case HTTOPRIGHT:
			case HTBOTTOMLEFT:
			case HTBOTTOMRIGHT:
			case HTLEFT:
			case HTRIGHT:
				result = HTNOWHERE;
			}
			SetWindowLongPtr(hWnd, DWLP_MSGRESULT, result);
		}
		break;

	/*case WM_ERASEBKGND:
		{
			HBRUSH hBsh = (HBRUSH)GetStockObject(BLACK_BRUSH);

			RECT rc;
			GetClientRect(hWnd, &rc);
			FillRect((HDC)wParam, &rc, hBsh);
		}
		break;*/
		
	case WM_CLOSE:
		if(m_isDirty)
		{
			UINT mb = MessageBox(hWnd, "Save changes?", "Material editor", MB_YESNOCANCEL);
			if((mb == IDYES && !save()) || mb == IDCANCEL)
			{
				break;
			}
			if(mb == IDNO)
			{
				undo();
			}
		}
		
		DestroyWindow(m_hDlgWnd);
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

			GetChildRect(m_hPropsWnd, &rcChild);
			MoveWindow(m_hPropsWnd, rcChild.left, rcChild.top, rc.right - m_uPanelDeltaX - rcChild.left, rc.bottom - m_uPanelDeltaY - rcChild.top, FALSE);

			InvalidateRect(hWnd, &rc, TRUE);

			if(m_ahChildren.size())
			{
				UINT uVPadding = 5;

				GetChildRect(m_ahChildren[0], &rcChild);

				rcChild.top -= uVPadding;

				if(rcChild.top < 0)
				{
					GetClientRect(m_hPropsWnd, &rc);
					GetChildRect(m_ahChildren[m_ahChildren.size() - 1], &rcChild);
					rcChild.bottom += uVPadding;
					if(rcChild.bottom < rc.bottom)
					{
						ScrollWindow(m_hPropsWnd, 0, rc.bottom - rcChild.bottom, NULL, NULL);
					}
				}
			}

			//InvalidateRect(hWnd, NULL, FALSE);
			updateScroll();
			return(FALSE);
		}
		break;

	case WM_GETMINMAXINFO:
		{
			MINMAXINFO *mmi = (MINMAXINFO*)lParam;
			mmi->ptMinTrackSize.y = m_uMinHeight;
			mmi->ptMinTrackSize.x = m_uFixedWidth;
			mmi->ptMaxTrackSize.x = m_uFixedWidth;
			SetWindowLongPtr(hWnd, DWLP_MSGRESULT, 0);
		}
		break;
		/*
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

		*/
	default:
		return(FALSE);
	}
	return(TRUE);
}

void CMaterialEditor::browse()
{
	ShowWindow(m_hDlgWnd, SW_SHOWNA);
	SetFocus(m_hDlgWnd);
}
/*
void CMaterialEditor::registerClass()
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
}*/

LRESULT CALLBACK CMaterialEditor::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CMaterialEditor *pThis = (CMaterialEditor*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(pThis)
	{
		return(pThis->wndProc(hWnd, msg, wParam, lParam));
	}
	return(DefWindowProc(hWnd, msg, wParam, lParam));
}

LRESULT CALLBACK CMaterialEditor::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int xPos;
	switch(msg)
	{
		/*
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
		*/
	default:
		return(DefWindowProc(hWnd, msg, wParam, lParam));
	}
	return(0);
}
#if 0
void CMaterialEditor::initGraphics(IGXDevice *pDev)
{
	m_pDev = pDev;

	m_pScrollbarEvent = new CScrollEventListener(m_hDlgWnd);
	m_pScrollBar = new CScrollBar(m_pDev, m_pScrollbarEvent);

	initViewport();

	initHelpers();


	m_pMaterialSystem = (IXMaterialSystem*)Core_GetIXCore()->getPluginManager()->getInterface(IXMATERIALSYSTEM_GUID);

	m_pFontManager = (IXFontManager*)Core_GetIXCore()->getPluginManager()->getInterface(IXFONTMANAGER_GUID);
	if(m_pFontManager)
	{
		m_pFontManager->getFont(&m_pFont, "gui/fonts/tahoma.ttf", 10);
		m_pFontManager->getFontVertexDeclaration(&m_pTextVD);
	}

	filter();
}
void CMaterialEditor::render()
{
	if(IsWindowVisible(m_hBrowserWnd) && m_isDirty)
	{
		IGXContext *pCtx = m_pDev->getThreadContext();
		IGXSurface *pOldRT = pCtx->getColorTarget();
		pCtx->setColorTarget(m_pSurface);
		IGXDepthStencilSurface *pOldDS = pCtx->getDepthStencilSurface();
		pCtx->unsetDepthStencilSurface();

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

					SGCore_ShaderBind(m_idInnerShader);

					item.pTexture->getAPITexture(&pTexture, item.uCurrentFrame);
					pCtx->setPSTexture(pTexture);
					mem_release(pTexture);

					pCtx->setIndexBuffer(m_pInnerIB);
					pCtx->setRenderBuffer(m_pInnerRB);
					pCtx->drawIndexed(m_uInnerVC, m_uInnerPC);

					SGCore_ShaderUnBind();
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
			SGCore_ShaderBind(m_idTextShader);
			pCtx->drawIndexed(m_uTextVertexCount, m_uTextQuadCount * 2);
			SGCore_ShaderUnBind();
		}

		pCtx->setDepthStencilSurface(pOldDS);
		mem_release(pOldDS);
		pCtx->setColorTarget(pOldRT);
		mem_release(pOldRT);


		IGXSurface *pRT = m_pSwapChain->getColorTarget();
		pCtx->downsampleColorTarget(m_pSurface, pRT);
		mem_release(pRT);

		//m_isDirty = false;
		m_bDoSwap = true;
	}
}
void CMaterialEditor::swapBuffers()
{
	if(IsWindowVisible(m_hBrowserWnd) && m_bDoSwap)
	{
		m_pSwapChain->swapBuffers();
		m_bDoSwap = false;
	}
}

void CMaterialEditor::initViewport()
{
	RECT rc;
	GetClientRect(m_hBrowserWnd, &rc);
	m_uPanelWidth = rc.right - rc.left;
	m_uPanelHeight = rc.bottom - rc.top;

	mem_release(m_pSwapChain);
	mem_release(m_pSurface);
	m_pSwapChain = m_pDev->createSwapChain(m_uPanelWidth, m_uPanelHeight, m_hBrowserWnd);
	m_pSurface = m_pDev->createColorTarget(m_uPanelWidth, m_uPanelHeight, GXFMT_A8B8G8R8_SRGB, GXMULTISAMPLE_4_SAMPLES, false);
	//m_pDepthStencilSurface = pDev->createDepthStencilSurface(m_uPanelWidth, m_uPanelHeight, GXFMT_D24S8, GXMULTISAMPLE_NONE);


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

void CMaterialEditor::initHelpers()
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


	m_idFrameShader = SGCore_ShaderCreateKit(SGCore_ShaderLoad(SHADER_TYPE_VERTEX, "terrax_mat_frame.vs"), SGCore_ShaderLoad(SHADER_TYPE_PIXEL, "terrax_colored2.ps"));

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


	m_idInnerShader = SGCore_ShaderCreateKit(SGCore_ShaderLoad(SHADER_TYPE_VERTEX, "terrax_mat_inner.vs"), SGCore_ShaderLoad(SHADER_TYPE_PIXEL, "terrax_mat_inner.ps"));

	m_idTextShader = SGCore_ShaderCreateKit(SGCore_ShaderLoad(SHADER_TYPE_VERTEX, "gui_text.vs"), SGCore_ShaderLoad(SHADER_TYPE_PIXEL, "gui_main.ps"));

	m_pTextColorCB = m_pDev->createConstantBuffer(sizeof(float4_t));
	fBlueColor.w = 1.0f;
	m_pTextColorCB->update(&fBlueColor);
	m_pTextOffsetCB = m_pDev->createConstantBuffer(sizeof(float4_t));
}
#endif

void CMaterialEditor::initFields()
{
	UINT uVPadding = 5;
	UINT uHPadding = 5;
	UINT uBrowseWidth = 25;
	const UINT c_uRangeEditWidth = 75;

	int iTopOffset = 0;
	if(m_ahChildren.size())
	{
		RECT rect;
		GetChildRect(m_ahChildren[0], &rect);
		iTopOffset = rect.top - uVPadding;
	}

	fora(i, m_ahChildren)
	{
		DestroyWindow(m_ahChildren[i]);
	}
	m_ahChildren.clearFast();

	if(!m_pMaterial)
	{
		return;
	}
	UINT uCount = m_pMaterialSystem->getMaterialPropertyCount(m_pMaterial);
	if(!uCount)
	{
		return;
	}
	m_aProperties.resizeFast(uCount);
	//XMaterialProperty *pProps = (XMaterialProperty*)alloca(sizeof(XMaterialProperty) * uCount);
	uCount = m_pMaterialSystem->getMaterialProperties(m_aProperties, m_pMaterial, true);
	if(!uCount)
	{
		return;
	}
	m_aProperties.resizeFast(uCount);

	m_aProperties.push_back(XMATERIAL_PARAM_RANGE("Phys durability", "durability", 10.0f, 1000.0f, 1000.0f));
	m_aProperties.push_back(XMATERIAL_PARAM_RANGE("Phys density", "density", 10.0f, 1000.0f, 1000.0f));
	m_aProperties.push_back(XMATERIAL_PARAM_RANGE("Hit chance", "hit_chance", 0.0f, 1.0f, 1.0f));
	m_aProperties.push_back(XMATERIAL_PARAM_FLAG("Two-sided", "twosided", NULL));
	m_aProperties.push_back(XMATERIAL_PARAM_FLAG("Render only in editor", "editorial", NULL));
	

	RECT rc;
	GetClientRect(m_hPropsWnd, &rc);
	Array<HWND> aGropBoxes;
	UINT uControlWidth;
	int iTop = iTopOffset;
	XMaterialProperty *pProp;
	char tmp[64];
	fora(i, m_aProperties)
	{
		pProp = &m_aProperties[i];

		uControlWidth = rc.right - rc.left - uHPadding * 2;

		HWND hControl;
		switch(pProp->type)
		{
		case XMPT_PARAM_TEXTURE:
		case XMPT_PARAM_TEXTURE_CUBE:
		case XMPT_PARAM_TEXTURE_OPT:
		case XMPT_PARAM_TEXTURE_CUBE_OPT:
			// caption, textfield, select btn
		case XMPT_PARAM_COLOR:
			// caption, textfield, picker(?)

			iTop += uVPadding;
			hControl = CreateWindowExA(0, WC_STATIC, pProp->szTitle, WS_VISIBLE | WS_CHILD, rc.left + uHPadding, iTop, uControlWidth, 15, m_hPropsWnd, NULL, m_hInstance, NULL);
			m_ahChildren.push_back(hControl);
			iTop += 15;

			hControl = CreateWindowExA(WS_EX_CLIENTEDGE | WS_EX_NOPARENTNOTIFY, WC_EDIT, "", WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, rc.left + uHPadding, iTop, uControlWidth - uBrowseWidth - 2, 20, m_hPropsWnd, (HMENU)(1000 + i), m_hInstance, NULL);
			m_ahChildren.push_back(hControl);

			if(pProp->type == XMPT_PARAM_COLOR)
			{
				float4_t vColor = m_pMaterial->getParam(pProp->szKey);
				sprintf(tmp, "%f %f %f %f", vColor.x, vColor.y, vColor.z, vColor.w);
				SetWindowTextA(hControl, tmp);
			}
			else
			{
				const char *szTexture = m_pMaterial->getTextureName(pProp->szKey);
				SetWindowTextW(hControl, szTexture ? CMB2WC(szTexture) : L"");
			}

			hControl = CreateWindowExA(0, WC_BUTTON, "...", WS_VISIBLE | WS_CHILD | WS_TABSTOP, rc.left + uHPadding + uControlWidth - uBrowseWidth, iTop, uBrowseWidth, 20, m_hPropsWnd, (HMENU)(2000 + i), m_hInstance, NULL);
			m_ahChildren.push_back(hControl);

			iTop += 20;
			iTop += uVPadding;

			//m_pMaterial->getParamHandler();
			break;

		case XMPT_PARAM_FLAG:
			// checkbox
			{
				hControl = CreateWindowExA(0, WC_BUTTON, pProp->szTitle, WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, rc.left + uHPadding, iTop + uVPadding, uControlWidth, 15, m_hPropsWnd, (HMENU)(1000 + i), m_hInstance, NULL);
				m_ahChildren.push_back(hControl);

				IMaterialFlag *pFlagHandler = m_pMaterial->getFlagHandler(pProp->szKey);

				bool isSet = pFlagHandler ? pFlagHandler->get() : pProp->fValue.x > 0.0f;
				Button_SetCheck(hControl, isSet ? BST_CHECKED : BST_UNCHECKED);

				iTop += 15 + uVPadding * 2;
			}
			break;

		case XMPT_PARAM_GROUP:
			hControl = CreateWindowExA(0, WC_BUTTON, pProp->szTitle ? pProp->szTitle : "", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, rc.left + uHPadding, iTop + uVPadding, uControlWidth, 15, m_hPropsWnd, NULL, m_hInstance, NULL);
			m_ahChildren.push_back(hControl);
			iTop += 15 + uVPadding;
			uHPadding += 10;
			aGropBoxes.push_back(hControl);
			//WC_GROP
			break;
		case XMPT_PARAM_GROUP_END:
			{
				assert(aGropBoxes.size());
				if(!aGropBoxes.size())
				{
					break;
				}

				hControl = aGropBoxes[aGropBoxes.size() - 1];
				aGropBoxes.erase(aGropBoxes.size() - 1);

				if(hControl == GetLastSibling(hControl))
				{
					assert(hControl == m_ahChildren[m_ahChildren.size() - 1]);
					m_ahChildren.erase(m_ahChildren.size() - 1);
					DestroyWindow(hControl);
					iTop -= 15 + uVPadding;
				}
				else
				{
					RECT rect;
					GetChildRect(hControl, &rect);
					MoveWindow(hControl, rect.left, rect.top, rect.right - rect.left, iTop - rect.top, TRUE);
					iTop += uVPadding;
				}
				uHPadding -= 10;
			}
			break;

		case XMPT_PARAM_RANGE:
			// caption, rangebar, textfield(with spinner?)
			iTop += uVPadding;
			hControl = CreateWindowExA(0, WC_STATIC, pProp->szTitle, WS_VISIBLE | WS_CHILD, rc.left + uHPadding, iTop, uControlWidth, 15, m_hPropsWnd, NULL, m_hInstance, NULL);
			m_ahChildren.push_back(hControl);
			iTop += 15;

			hControl = CreateWindowExA(WS_EX_CLIENTEDGE | WS_EX_NOPARENTNOTIFY, WC_EDIT, "", WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, rc.left + uHPadding, iTop, c_uRangeEditWidth, 20, m_hPropsWnd, (HMENU)(1000 + i), m_hInstance, NULL);
			m_ahChildren.push_back(hControl);


			IMaterialProperty *pPropHandler = m_pMaterial->getParamHandler(pProp->szKey);

			float fVal = pPropHandler ? pPropHandler->get().x : pProp->fValue.x;
			sprintf(tmp, "%f", fVal);
			SetWindowTextA(hControl, tmp);

			hControl = CreateWindowExA(WS_EX_NOPARENTNOTIFY, TRACKBAR_CLASS, "", WS_VISIBLE | WS_CHILD | WS_TABSTOP | PBS_MARQUEE | PBS_SMOOTHREVERSE, rc.left + uHPadding + c_uRangeEditWidth, iTop, uControlWidth - c_uRangeEditWidth, 20, m_hPropsWnd, (HMENU)(2000 + i), m_hInstance, NULL);
			m_ahChildren.push_back(hControl);
			SendMessage(hControl, TBM_SETRANGEMIN, FALSE, 0);
			SendMessage(hControl, TBM_SETRANGEMAX, FALSE, 65535);
			SendMessage(hControl, TBM_SETPOS, TRUE, (DWORD)(((fVal - pProp->fValue.y) / (pProp->fValue.z - pProp->fValue.y)) * 65535.0f));

			if(!g_pfnTrackbarOldWndproc)
			{
				g_pfnTrackbarOldWndproc = (WNDPROC)GetWindowLongPtr(hControl, GWLP_WNDPROC);
			}
			SetWindowLongPtr(hControl, GWLP_WNDPROC, (LONG_PTR)TrackbarWndProc);

			iTop += 20;
			iTop += uVPadding;
			
			break;
		}
	}

	HGDIOBJ hDefaultFont = GetStockObject(DEFAULT_GUI_FONT);
	fora(i, m_ahChildren)
	{
		SetWindowFont(m_ahChildren[i], hDefaultFont, FALSE);
	}

	m_iTotalHeight = iTop - iTopOffset;

	if(iTopOffset < 0 && iTop < rc.bottom)
	{
		int iOffset = rc.bottom - iTop;
		if(iOffset > -iTopOffset)
		{
			iOffset = -iTopOffset;
		}
		ScrollWindow(m_hPropsWnd, 0, iOffset, NULL, NULL);
	}

	SetFocus(m_hPropsWnd);

	updateScroll();
}

void CMaterialEditor::updateScroll()
{
	RECT rc;
	GetClientRect(m_hPropsWnd, &rc);
	UINT uWndHeight = rc.bottom - rc.top;

	SCROLLINFO si = {sizeof(si)};

	si.fMask = SIF_RANGE | SIF_PAGE | SIF_DISABLENOSCROLL;
	si.nPage = uWndHeight;
	si.nMin = 0;
	si.nMax = m_iTotalHeight;

	SetScrollInfo(m_hPropsWnd, SB_VERT, &si, TRUE);
}

INT_PTR CALLBACK CMaterialEditor::InnerDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CMaterialEditor *pThis = (CMaterialEditor*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lParam);
		pThis = (CMaterialEditor*)lParam;
		pThis->m_hDlgWnd = hWnd;
	}
	if(pThis)
	{
		return(pThis->innerDlgProc(hWnd, msg, wParam, lParam));
	}
	return(FALSE);
}

INT_PTR CALLBACK CMaterialEditor::innerDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_COMMAND:
		if(LOWORD(wParam) >= 1000 && LOWORD(wParam) < 2000)
		{
			// main control
			UINT id = LOWORD(wParam) - 1000;
			XMaterialProperty &prop = m_aProperties[id];
			HWND hControl = (HWND)lParam;

			if(prop.type == XMPT_PARAM_FLAG)
			{
				m_pMaterial->setFlag(prop.szKey, Button_GetCheck((HWND)lParam) == BST_CHECKED);
				m_isDirty = true;
				initFields();
			}
			else if(
				prop.type == XMPT_PARAM_TEXTURE ||
				prop.type == XMPT_PARAM_TEXTURE_OPT ||
				prop.type == XMPT_PARAM_TEXTURE_CUBE ||
				prop.type == XMPT_PARAM_TEXTURE_CUBE_OPT
			)
			{
				if(HIWORD(wParam) == EN_KILLFOCUS)
				{
					int iLen = GetWindowTextLengthW(hControl);
					wchar_t *szText = (wchar_t*)alloca(sizeof(wchar_t) * (iLen + 1));
					GetWindowTextW(hControl, szText, iLen + 1);
					CWC2MB sz(szText);

					if(fstrcmp(m_pMaterial->getTextureName(prop.szKey), sz))
					{
						m_pMaterial->setTexture(prop.szKey, sz);
						m_isDirty = true;

						if(prop.type == XMPT_PARAM_TEXTURE_OPT || prop.type == XMPT_PARAM_TEXTURE_CUBE_OPT)
						{
							initFields();
						}
					}
				}
			}
			else if(prop.type == XMPT_PARAM_COLOR || prop.type == XMPT_PARAM_RANGE)
			{
				if(HIWORD(wParam) == EN_CHANGE)
				{
					float4_t vParam(0.0f, 0.0f, 0.0f, 1.0f);
					char tmp[64];
					GetWindowTextA(hControl, tmp, sizeof(tmp));

					int res = sscanf(tmp, "%f %f %f %f", &vParam.x, &vParam.y, &vParam.z, &vParam.w);

					if(prop.type == XMPT_PARAM_COLOR)
					{
						if(res == 3)
						{
							vParam.w = 1.0f;
							++res;
						}
						if(res == 4)
						{
							m_pMaterial->setParam(prop.szKey, vParam);
							m_isDirty = true;
						}
					}
					else
					{
						if(res == 1)
						{
							m_pMaterial->setParam(prop.szKey, vParam);
							m_isDirty = true;
							SendMessage(GetDlgItem(hWnd, id + 2000), TBM_SETPOS, TRUE, (DWORD)(((vParam.x - prop.fValue.y) / (prop.fValue.z - prop.fValue.y)) * 65535.0f));
						}
					}
				}
			}
		}
		else if(LOWORD(wParam) >= 2000 && LOWORD(wParam) < 3000)
		{
			// support control
			UINT id = LOWORD(wParam) - 2000;
			XMaterialProperty &prop = m_aProperties[id];
			HWND hMainControl = GetDlgItem(hWnd, id + 1000);
			HWND hControl = (HWND)lParam;


			if(prop.type == XMPT_PARAM_COLOR)
			{
				char tmp[64];
				GetWindowTextA(hMainControl, tmp, sizeof(tmp));

				float4_t vColor(0.0f, 0.0f, 0.0f, 1.0f);
				sscanf(tmp, "%f %f %f %f", &vColor.x, &vColor.y, &vColor.z, &vColor.w);

				CHOOSECOLOR cc = {sizeof(cc)};
				static COLORREF s_acrCustClr[16];

				cc.hwndOwner = m_hDlgWnd;
				cc.lpCustColors = s_acrCustClr;
				cc.rgbResult = RGB((UINT)(vColor.x * 255.0f), (UINT)(vColor.y * 255.0f), (UINT)(vColor.z * 255.0f));
				cc.Flags = CC_FULLOPEN | CC_RGBINIT;

				if(ChooseColor(&cc) == TRUE)
				{
					vColor.x = (float)GetRValue(cc.rgbResult) / 255.0f;
					vColor.y = (float)GetGValue(cc.rgbResult) / 255.0f;
					vColor.z = (float)GetBValue(cc.rgbResult) / 255.0f;

					sprintf(tmp, "%f %f %f %f", vColor.x, vColor.y, vColor.z, vColor.w);
					SetWindowTextA(hMainControl, tmp);
				}
			}
			else if(
				prop.type == XMPT_PARAM_TEXTURE ||
				prop.type == XMPT_PARAM_TEXTURE_OPT ||
				prop.type == XMPT_PARAM_TEXTURE_CUBE ||
				prop.type == XMPT_PARAM_TEXTURE_CUBE_OPT
				)
			{
				RECT rc;
				GetChildRect(hControl, &rc);
				POINT pt;
				pt.x = rc.left;
				pt.y = rc.bottom;

				ClientToScreen(hWnd, (LPPOINT)&pt);

				// parse filters

				HMENU hMenu = (prop.type == XMPT_PARAM_TEXTURE_OPT || prop.type == XMPT_PARAM_TEXTURE_CUBE_OPT) ? m_hTextureOptMenu : m_hTextureMenu;

				MENUITEMINFOA mii = {sizeof(mii)};
				mii.fMask = MIIM_STATE;
				mii.fState = MFS_UNCHECKED;

				for(UINT i = 0, l = GetMenuItemCount(hMenu); i < l; ++i)
				{
					SetMenuItemInfoA(hMenu, i, TRUE, &mii);
				}
				mii.fState = MFS_CHECKED;

				int iOldLen = GetWindowTextLengthW(hMainControl);

				wchar_t *wszOldTexture = (wchar_t*)alloca(sizeof(wchar_t) * (iOldLen + 1));
				GetWindowTextW(hMainControl, wszOldTexture, iOldLen + 1);
				CWC2MB sz(wszOldTexture);
				int iLen = strlen(sz);
				char *szTexture = (char*)alloca(sizeof(char) * (iLen + 1));
				strcpy(szTexture, sz);

				int iParts = parse_str(szTexture, NULL, NULL, '|');
				char **aszParts = (char**)alloca(sizeof(char*) * iParts);
				parse_str(szTexture, aszParts, iParts, '|');

				for(int i = 1; i < iParts; ++i)
				{
					int idx = m_aszFilters.indexOf(aszParts[i], [](const char *a, const char *b){
						return(!strcmp(a, b));
					});
					if(idx >= 0)
					{
						SetMenuItemInfoA(hMenu, idx + 3000, FALSE, &mii);
					}
				}

				UINT uCmd = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
				switch(uCmd)
				{
				case ID_EDIT_DELETE:
					m_pMaterial->setTexture(prop.szKey, "");
					m_isDirty = true;
					initFields();
					break;

				case ID_MAT_BROWSER:
					m_cbTextureBrowser.prepareSelect(prop.szKey);
					g_pMaterialBrowser->browse(&m_cbTextureBrowser, true);
					break;
					
				default:
					if(uCmd >= 3000)
					{
						MENUITEMINFOA mii = {sizeof(mii)};
						mii.fMask = MIIM_STATE;
						GetMenuItemInfoA(hMenu, uCmd, FALSE, &mii);

						uCmd -= 3000;

						if(mii.fState)
						{
							// remove filter

							char *szNewTexName = (char*)alloca(sizeof(char) * (iLen + 1));
							strcpy(szNewTexName, aszParts[0]);

							for(int i = 1; i < iParts; ++i)
							{
								if(strcmp(aszParts[i], m_aszFilters[uCmd]))
								{
									strcat(szNewTexName, "|");
									strcat(szNewTexName, aszParts[i]);
								}
							}

							SetWindowTextW(hMainControl, CMB2WC(szNewTexName));
						}
						else
						{
							// add filter
							CMB2WC wcs(m_aszFilters[uCmd]);
							wchar_t *wszNewTexName = (wchar_t*)alloca(sizeof(wchar_t)* (iOldLen + 1 + (wcslen(wcs) + 1)));
							wcscpy(wszNewTexName, wszOldTexture);
							wcscat(wszNewTexName, L"|");
							wcscat(wszNewTexName, wcs);

							SetWindowTextW(hMainControl, wszNewTexName);
						}

						SendMessage(m_hPropsWnd, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hMainControl), EN_KILLFOCUS), (LPARAM)hMainControl);
					}
				}
			}
		}
		break;

	case WM_HSCROLL:
		{
			float fPos = (float)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0) / 65535.0f;
			UINT uId = GetDlgCtrlID((HWND)lParam);
			assert(uId >= 2000 && uId < 3000);
			uId -= 2000;

			float fVal = lerpf(m_aProperties[uId].fValue.y, m_aProperties[uId].fValue.z, fPos);
			
			HWND hControl = GetDlgItem(hWnd, uId + 1000);
			char tmp[64];
			sprintf(tmp, "%f", fVal);
			SetWindowText(hControl, tmp);

			m_pMaterial->setParam(m_aProperties[uId].szKey, float4_t(fVal, 0.0f, 0.0f, 0.0f));
			m_isDirty = true;
		}
		break;

	case WM_LBUTTONDOWN:
		SetFocus(GetParent(hWnd));
		return(TRUE);

	case WM_MOUSEWHEEL:
		{
			SCROLLINFO scrInfo = {sizeof(SCROLLINFO)};

			scrInfo.fMask = SIF_ALL;
			GetScrollInfo(hWnd, SB_VERT, &scrInfo);

			int currentPos = scrInfo.nPos;

			scrInfo.nPos -= (float)GET_WHEEL_DELTA_WPARAM(wParam) * 32.0f / (float)WHEEL_DELTA;


			scrInfo.fMask = SIF_POS;
			SetScrollInfo(hWnd, SB_VERT, &scrInfo, TRUE);
			GetScrollInfo(hWnd, SB_VERT, &scrInfo);

			int yScroll = currentPos - scrInfo.nPos;
			ScrollWindow(hWnd, 0, yScroll, NULL, NULL);
		}
		break;

	case WM_VSCROLL:
		{
			SCROLLINFO scrInfo = {sizeof(SCROLLINFO)};

			scrInfo.fMask = SIF_ALL;
			GetScrollInfo(hWnd, SB_VERT, &scrInfo);

			int currentPos = scrInfo.nPos;

			switch(LOWORD(wParam))
			{
			case SB_LINEUP:
				scrInfo.nPos -= 20;
				break;
			case SB_LINEDOWN:
				scrInfo.nPos += 20;
				break;
			case SB_THUMBTRACK:
				scrInfo.nPos = scrInfo.nTrackPos;
				break;
			case SB_PAGEUP:
				scrInfo.nPos -= scrInfo.nPage;
				break;
			case SB_PAGEDOWN:
				scrInfo.nPos += scrInfo.nPage;
				break;
			}

			scrInfo.fMask = SIF_POS;
			SetScrollInfo(hWnd, SB_VERT, &scrInfo, TRUE);
			GetScrollInfo(hWnd, SB_VERT, &scrInfo);

			int yScroll = currentPos - scrInfo.nPos;
			ScrollWindow(hWnd, 0, yScroll, NULL, NULL);
		}
		break;

	default:
		return(FALSE);
	}
	return(TRUE);
}

LRESULT CALLBACK CMaterialEditor::TrackbarWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message == WM_MOUSEWHEEL)
	{
		RECT rc;
		GetClientRect(hWnd, &rc);
		POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
		if(pt.x < rc.left || pt.x > rc.right || pt.y < rc.top || pt.y > rc.bottom)
		{
			SendMessage(GetParent(hWnd), message, wParam, lParam);
			return(0);
		}
	}
	return(CallWindowProc(g_pfnTrackbarOldWndproc, hWnd, message, wParam, lParam));
}

void CMaterialEditor::setTexture(const char *szKey, const char *szTexture)
{
	if(!m_hDlgWnd)
	{
		return;
	}

	int idx = m_aProperties.indexOf(szKey, [](const XMaterialProperty &prop, const char *szKey){
		return(prop.szKey == szKey);
	});

	if(idx >= 0)
	{
		const XMaterialProperty &prop = m_aProperties[idx];
		HWND hControl = GetDlgItem(m_hPropsWnd, idx + 1000);

		wchar_t *wszFilters = NULL;
		int iOldLen = GetWindowTextLengthW(hControl);
		if(iOldLen)
		{
			wchar_t *wszOldTexture = (wchar_t*)alloca(sizeof(wchar_t)* (iOldLen + 1));
			GetWindowTextW(hControl, wszOldTexture, iOldLen + 1);
			
			wszFilters = wszOldTexture;
			while(wszFilters[0] && wszFilters[0] != L'|')
			{
				wszFilters++;
			}
		}

		if(wszFilters && wszFilters[0])
		{
			CMB2WC wcs(szTexture);
			size_t sizeLen = wcslen(wcs);
			sizeLen += wcslen(wszFilters);

			wchar_t *wszNewTexture = (wchar_t*)alloca(sizeof(wchar_t)* (sizeLen + 1));
			wszNewTexture[0] = 0;
			wcscat(wszNewTexture, wcs);
			wcscat(wszNewTexture, wszFilters);
			SetWindowTextW(hControl, wszNewTexture);
		}
		else
		{
			SetWindowTextW(hControl, CMB2WC(szTexture));
		}

		SendMessage(m_hPropsWnd, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hControl), EN_KILLFOCUS), (LPARAM)hControl);
	}
	//m_pMaterial->setTexture(szKey, szTexture);
}

bool CMaterialEditor::save()
{
	if(m_pMaterial->save())
	{
		m_isDirty = false;
		return(true);
	}

	MessageBox(m_hDlgWnd, "Unable to save material!", "Material editor", MB_ICONSTOP | MB_OK);

	return(false);
}

void CMaterialEditor::backup()
{
	m_backup.sShader = m_pMaterial->getShader();
	m_backup.physType = m_pMaterial->getPhysicsType();

	const char *szKey;

	IKeyIterator *pIter = m_pMaterial->getFlagsIterator();
	while(!pIter->isEnd())
	{
		szKey = pIter->getCurrent();
		m_backup.aFlags.push_back({szKey, m_pMaterial->getFlag(szKey)});
		pIter->getNext();
	}
	mem_release(pIter);

	pIter = m_pMaterial->getParamsIterator();
	while(!pIter->isEnd())
	{
		szKey = pIter->getCurrent();
		m_backup.aProps.push_back({szKey, m_pMaterial->getParam(szKey)});
		pIter->getNext();
	}
	mem_release(pIter);

	pIter = m_pMaterial->getTexturesIterator();
	while(!pIter->isEnd())
	{
		szKey = pIter->getCurrent();
		m_backup.aTexs.push_back({szKey, m_pMaterial->getTextureName(szKey)});
		pIter->getNext();
	}
	mem_release(pIter);
}

void CMaterialEditor::undo()
{
	m_pMaterial->setShader(m_backup.sShader.c_str());
	m_pMaterial->setPhysicsType(m_backup.physType);

	fora(i, m_backup.aFlags)
	{
		m_pMaterial->setFlag(m_backup.aFlags[i].sKey.c_str(), m_backup.aFlags[i].isSet);
	}

	fora(i, m_backup.aProps)
	{
		m_pMaterial->setParam(m_backup.aProps[i].sKey.c_str(), m_backup.aProps[i].vVal);
	}

	fora(i, m_backup.aTexs)
	{
		m_pMaterial->setTexture(m_backup.aTexs[i].sKey.c_str(), m_backup.aTexs[i].sTexture.c_str());
	}

	m_isDirty = false;
}

//#############################################################################

CTextureBrowserCallback::CTextureBrowserCallback(CMaterialEditor *pMatEditor):
	m_pMatEditor(pMatEditor)
{
}

void CTextureBrowserCallback::prepareSelect(const char *szKey)
{
	m_szKey = szKey;
	add_ref(m_pMatEditor);
}

void CTextureBrowserCallback::onSelected(const char *szName)
{
	m_pMatEditor->setTexture(m_szKey, szName);

	m_pMatEditor->Release();
}
void CTextureBrowserCallback::onCancel()
{
	m_pMatEditor->Release();
}
