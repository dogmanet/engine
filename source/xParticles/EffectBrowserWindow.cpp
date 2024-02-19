#include "EffectBrowserWindow.h"
#include "resource.h"

#include <windowsx.h>
#include <shellapi.h>
#include <commoncontrols.h>

#if 0
BOOL EnumLevels(CLevelInfo *pInfo)
{
	pInfo->m_szName[0] = 0;
	pInfo->m_szLocalName[0] = 0;
	pInfo->m_szLastEdit[0] = 0;

	bool bFound = false;
	const char *szDirName = NULL;
	if(!pInfo->m_pIterator)
	{
		pInfo->m_pIterator = Core_GetIXCore()->getFileSystem()->getFolderList("levels/");
	}
	if(pInfo->m_pIterator && (szDirName = pInfo->m_pIterator->next()))
	{
		bFound = true;
	}

	if(!bFound)
	{
		mem_release(pInfo->m_pIterator);
		return(FALSE);
	}

	const char *szLevelName = basename(szDirName);
	strncpy(pInfo->m_szName, szLevelName, MAX_LEVEL_STRING - 1);

	{
		char szFullPath[1024], szFSPath[1024];
		sprintf(szFullPath, "levels/%s/%s.lvl", pInfo->m_szName, pInfo->m_szName);

		if(Core_GetIXCore()->getFileSystem()->resolvePath(szFullPath, szFSPath, sizeof(szFSPath)))
		{
			GetLastWriteTime(szFSPath, pInfo->m_szLastEdit);


			ISXConfig *pConfig = Core_OpConfig(szFSPath);
			if(pConfig->keyExists("level", "local_name"))
			{
				strncpy(pInfo->m_szLocalName, pConfig->getKey("level", "local_name"), MAX_LEVEL_STRING - 1);
			}
			else
			{
				strncpy(pInfo->m_szLocalName, pInfo->m_szName, MAX_LEVEL_STRING - 1);
			}

			if(pConfig->keyExists("level", "type"))
			{
				pInfo->m_isIndoor = !!strcasecmp(pConfig->getKey("level", "type"), "outdoor");
			}
			else
			{
				pInfo->m_isIndoor = true;
			}
			mem_release(pConfig);
		}

		//sprintf(szFullPath, "%s%s/preview.bmp", Core_RStringGet(G_RI_STRING_PATH_GS_LEVELS), pInfo->m_szName);
		//pInfo->m_bHasPreview = FileExistsFile(szFullPath);
	}

	return(TRUE);
}
#endif

CEffectBrowserWindow::CEffectBrowserWindow(HINSTANCE hInstance, HWND hMainWnd, IFileSystem *pFS, IXParticleSystem *pParticleSystem):
	m_hInstance(hInstance),
	m_hMainWnd(hMainWnd),
	m_pFS(pFS),
	m_pParticleSystem(pParticleSystem)
{
	registerClass();
	m_hContextMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1));
	HMENU hSubMenu = GetSubMenu(m_hContextMenu, 1);
	SetMenuDefaultItem(hSubMenu, IDOK, FALSE);

	CreateDialogParamW(hInstance, MAKEINTRESOURCEW(IDD_EFFECT_BROWSER), hMainWnd, DlgProc, (LPARAM)this);
	//ShowWindow(m_hDlgWnd, SW_SHOW);
}

CEffectBrowserWindow::~CEffectBrowserWindow()
{
	mem_release(m_pPlayer);
	mem_release(m_pCamera);
	mem_release(m_pFinalTarget);
	DestroyWindow(m_hDlgWnd);
	DestroyMenu(m_hContextMenu);
}

void CEffectBrowserWindow::setRender(IXRender *pRender)
{
	if(!pRender->newFinalTarget(m_hPreviewWnd, "xParticlesPreview", &m_pFinalTarget))
	{
		LogError("Could not create render target 'xParticlesPreview'\n");
		return;
	}

	RECT rc;
	GetClientRect(m_hPreviewWnd, &rc);
	m_pFinalTarget->resize(rc.right - rc.left, rc.bottom - rc.top);

	pRender->newCamera(&m_pCamera);
	m_pCamera->setPosition(float3(0.0f, 2.0f, -2.0f));
	m_pCamera->setOrientation(SMQuaternion(-SM_PIDIV4, 'x'));
	m_pCamera->setLayerMask(0x2);

	m_pFinalTarget->setCamera(m_pCamera);

	IXRenderGraph *pGraph;
	pRender->getGraph("xParticlePreview", &pGraph);
	m_pFinalTarget->attachGraph(pGraph);
	mem_release(pGraph);
}

UINT XMETHODCALLTYPE CEffectBrowserWindow::getResourceTypeCount()
{
	return(1);
}
const char* XMETHODCALLTYPE CEffectBrowserWindow::getResourceType(UINT uId)
{
	if(uId == 0)
	{
		return("effect");
	}
	return(NULL);
}

void XMETHODCALLTYPE CEffectBrowserWindow::browse(const char *szType, const char *szOldValue, IXEditorResourceBrowserCallback *pCallback)
{
	if(m_pCallback)
	{
		m_pCallback->onCancelled();
	}
	m_pCallback = pCallback;
	ShowWindow(m_hDlgWnd, SW_SHOW);
}

void XMETHODCALLTYPE CEffectBrowserWindow::cancel()
{
	SendMessage(m_hDlgWnd, WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), NULL);
}

void DisplayContextMenu(HWND hwnd, POINT pt, HMENU hMenu, int iSubmenu, int iCheckItem = -1)
{
	HMENU hmenuTrackPopup = GetSubMenu(hMenu, iSubmenu);

	MENUITEMINFOA mii;
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	mii.fState = MFS_UNCHECKED;
	for(UINT i = 0, l = GetMenuItemCount(hmenuTrackPopup); i < l; ++i)
	{
		SetMenuItemInfoA(hmenuTrackPopup, i, TRUE, &mii);
	}
	if(iCheckItem > 0)
	{
		mii.fState = MFS_CHECKED;
		SetMenuItemInfoA(hmenuTrackPopup, iCheckItem, FALSE, &mii);
	}

	//ClientToScreen(hwnd, (LPPOINT)&pt);
	// Display the shortcut menu. Track the right mouse 
	// button. 

	TrackPopupMenu(hmenuTrackPopup,
		TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		pt.x, pt.y, 0, hwnd, NULL
	);
}

INT_PTR CALLBACK CEffectBrowserWindow::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CEffectBrowserWindow *pThis = (CEffectBrowserWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lParam);
		pThis = (CEffectBrowserWindow*)lParam;
		pThis->m_hDlgWnd = hWnd;
	}
	if(pThis)
	{
		return(pThis->dlgProc(hWnd, msg, wParam, lParam));
	}
	return(FALSE);
}

INT_PTR CALLBACK CEffectBrowserWindow::dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			m_hOkButtonWnd = GetDlgItem(m_hDlgWnd, IDOK);
			Button_Enable(m_hOkButtonWnd, FALSE);

			m_hTreeWnd = GetDlgItem(m_hDlgWnd, IDC_TREE);

			SetWindowLongPtr(m_hTreeWnd, GWLP_USERDATA, (LONG_PTR)this);
			m_pTreeViewPrevWndProc = (WNDPROC)GetWindowLongPtrW(m_hTreeWnd, GWLP_WNDPROC);
			SetWindowLongPtrW(m_hTreeWnd, GWLP_WNDPROC, (LONG_PTR)TreeViewCtl_SubclassProc);

			HIMAGELIST himg;
			if(SUCCEEDED(SHGetImageList(SHIL_SMALL, IID_IImageList, reinterpret_cast<void**>(&himg))))
			{
				TreeView_SetImageList(m_hTreeWnd, himg, TVSIL_NORMAL);
			}

			m_hPreviewWnd = GetDlgItem(m_hDlgWnd, IDC_PREVIEW);

			scanFileSystem();
			createTree();
		}
		break;

	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->code)
		{
		case NM_DBLCLK:
			{
				LPNMHDR lpnmh = (LPNMHDR)lParam;;
				if(lpnmh->idFrom == IDC_TREE)
				{
					SendMessage(m_hDlgWnd, WM_COMMAND, MAKEWPARAM(IDOK, 0), NULL);
				}
			}
			break;

		case TVN_SELCHANGEDW:
			{
				LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
				if(pnmtv->hdr.idFrom == IDC_TREE)
				{
					Record *pRec = getRecordByHandle(pnmtv->itemNew.hItem);

					Button_Enable(m_hOkButtonWnd, pRec && !pRec->isDir);

					if(pRec && !pRec->isDir)
					{
						char szName[MAX_PATH];
						szName[0] = 0;
						makePath(szName, pRec);
						strcat(szName, ".eff");

						mem_release(m_pPlayer);
						IXParticleEffect *pEffect;
						if(m_pParticleSystem->getEffect(szName/* + strlen("effects/")*/, &pEffect))
						{
							IXParticlePlayer *pPlayer;
							m_pParticleSystem->newEffectPlayer(pEffect, &m_pPlayer);

							m_pPlayer->play();

							mem_release(pEffect);
						}
					}
				}
			}
			break;

		case TVN_ITEMEXPANDED:
			{
				LPNMTREEVIEW lpnmtv = (LPNMTREEVIEW)lParam;
				if(lpnmtv->hdr.idFrom == IDC_TREE)
				{
					Record *pRec = getRecordByHandle(lpnmtv->itemNew.hItem);

					assert(pRec);
					if(pRec)
					{
						pRec->isUserInteracted = true;
						pRec->isUserCollapsed = lpnmtv->action != TVE_EXPAND;
					}
				}
			}
			break;

		case TVN_BEGINLABELEDITW:
			{
				LPNMTVDISPINFOW lpnmtvinfo = (LPNMTVDISPINFOW)lParam;
				if(lpnmtvinfo->hdr.idFrom == IDC_TREE)
				{
					HWND hEditBoxWnd = TreeView_GetEditControl(m_hTreeWnd);

					SetWindowLongPtr(hEditBoxWnd, GWLP_USERDATA, (LONG_PTR)this);
					m_pEditBoxPrevWndProc = (WNDPROC)GetWindowLongPtrW(hEditBoxWnd, GWLP_WNDPROC);
					SetWindowLongPtrW(hEditBoxWnd, GWLP_WNDPROC, (LONG_PTR)EditBoxCtl_SubclassProc);
				}
			}
			break;

		case TVN_ENDLABELEDITW:
			{
				LPNMTVDISPINFOW lpnmtvinfo = (LPNMTVDISPINFOW)lParam;
				if(lpnmtvinfo->hdr.idFrom == IDC_TREE)
				{
					BOOL isRenamed = lpnmtvinfo->item.pszText != NULL && renameItem(lpnmtvinfo->item.hItem, CWC2MB(lpnmtvinfo->item.pszText));
					SetWindowLongPtr(hWnd, DWLP_MSGRESULT, isRenamed);
					return(TRUE);
				}
			}
			break;
		}
		break;

	case WM_CONTEXTMENU:
		if((HWND)wParam == m_hTreeWnd)
		{
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			bool useMenu = true;
			if(pt.x == -1 && pt.y == -1)
			{
				HTREEITEM hSelItem = TreeView_GetSelection(m_hTreeWnd);
				if(hSelItem)
				{
					RECT tvRect; // The tv window coordinates relative to the client area
					GetWindowRect(m_hTreeWnd, &tvRect);

					RECT nodeRect; // The TREEITEM coordinates relative to the tv
					if(TreeView_GetItemRect(m_hTreeWnd, hSelItem, &nodeRect, 1))
					{
						int a = 0;
					}

					pt.x = tvRect.left + nodeRect.left;
					pt.y = tvRect.top + nodeRect.bottom;

					if(nodeRect.right && tvRect.right) // We have rectangles
					{
						
					}
				}
			}
			else
			{
				RECT rcTree;
				HTREEITEM htvItem;
				TVHITTESTINFO htInfo = {0};

				GetWindowRect(m_hTreeWnd, &rcTree);              // get its window coordinates
				htInfo.pt.x = pt.x - rcTree.left;              // convert to client coordinates
				htInfo.pt.y = pt.y - rcTree.top;

				if((htvItem = TreeView_HitTest(m_hTreeWnd, &htInfo)))
				{    // hit test
					TreeView_SelectItem(m_hTreeWnd, htvItem);           // success; select the item
					/* display your context menu */
				}
				else
				{
					useMenu = false;
				}
			}

			if(useMenu)
			{
				HTREEITEM hItem = TreeView_GetSelection(m_hTreeWnd);
				if(hItem)
				{
					Record *pRec = getRecordByHandle(hItem);
					assert(pRec);
					if(pRec)
					{
						DisplayContextMenu(m_hDlgWnd, pt, m_hContextMenu, pRec->isDir ? 0 : 1);
					}
				}
			}
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_EDIT_FILTER:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				//scheduleFilterIn(1.0f);
				filter();
			}
			break;

		case ID_ADD_NEWFOLDER:
		case ID_ADD_NEWEFFECT:
			{
				bool isFolder = LOWORD(wParam) == ID_ADD_NEWFOLDER;
				HTREEITEM hItem = TreeView_GetSelection(m_hTreeWnd);
				UINT uIndex;
				Record *pRec = getRecordByHandle(hItem, &uIndex);
				if(hItem && pRec)
				{
					char szName[MAX_PATH];
					strcpy(szName, isFolder ? "New folder" : "New effect");
					size_t len = strlen(szName);
					int iIndex = 1;
					while(hasChildWithName(uIndex, szName))
					{
						szName[len] = 0;
						sprintf(szName + len, " (%i)", ++iIndex);
					}

					m_aRecords.push_back({szName, uIndex, isFolder});

					Record *pNewRec = &m_aRecords[m_aRecords.size() - 1];

					szName[0] = 0;
					makePath(szName, pNewRec);

					bool isFailed = false;
					if(isFolder)
					{
						if(!m_pFS->createDirectory(szName))
						{
							isFailed = true;
						}
					}
					else
					{
						strcat(szName, ".eff");
						IFile *pFile = m_pFS->openFile(szName, FILE_MODE_WRITE);
						if(!pFile)
						{
							isFailed = true;
						}
						mem_release(pFile);
					}

					if(isFailed)
					{
						m_aRecords.erase(m_aRecords.size() - 1);
					}
					else
					{
						createTree(m_aRecords.size() - 1);

						TreeView_Expand(m_hTreeWnd, hItem, TVE_EXPAND);

						TreeView_SelectItem(m_hTreeWnd, pNewRec->hTreeItem);
						TreeView_EditLabel(m_hTreeWnd, pNewRec->hTreeItem);
					}
				}
			}
			break;

		case ID_FOLDER_RENAME:
		case ID_EFFECT_RENAME:
			TreeView_EditLabel(m_hTreeWnd, TreeView_GetSelection(m_hTreeWnd));
			break;

		case ID_FOLDER_DELETE:
			{
				HTREEITEM hItem = TreeView_GetSelection(m_hTreeWnd);
				UINT uIndex;
				Record *pRec = getRecordByHandle(hItem, &uIndex);
				if(hItem && pRec && pRec->isDir && uIndex && MessageBox(hWnd, "Are you sure want to permanently delete this folder with all it's contents?", "Delete folder", MB_YESNO) == IDYES)
				{
					char szDir[MAX_PATH];
					szDir[0] = 0;
					makePath(szDir, pRec);
					if(m_pFS->deleteDirectory(szDir))
					{
						removeRecord(uIndex);
						TreeView_DeleteItem(m_hTreeWnd, hItem);
					}
				}
			}
			break;

		case ID_EFFECT_DELETE:
			{
				HTREEITEM hItem = TreeView_GetSelection(m_hTreeWnd);
				UINT uIndex;
				Record *pRec = getRecordByHandle(hItem, &uIndex);
				if(hItem && pRec && !pRec->isDir && uIndex && MessageBox(hWnd, "Are you sure want to permanently delete this effect?", "Delete effect", MB_YESNO) == IDYES)
				{
					char szFile[MAX_PATH];
					szFile[0] = 0;
					makePath(szFile, pRec);
					strcat(szFile, ".eff");

					TODO("Uncomment me!");
				//	if(m_pFS->unlink(szFile))
				//	{
				//		m_aRecords.erase(uIndex);
				//		for(UINT i = uIndex, l = m_aRecords.size(); i < l; ++i)
				//		{
				//			Record &rec = m_aRecords[i];
				//			if(rec.uParent >= uIndex)
				//			{
				//				--rec.uParent;
				//			}
				//		}
				//		TreeView_DeleteItem(m_hTreeWnd, hItem);
				//	}
				}
			}
			break;

		case ID_EFFECT_EDIT:
			TODO("open edit dialog");
			break;

		case IDOK:
			if(HIWORD(wParam) == 0)
			{
				HTREEITEM hItem = TreeView_GetSelection(m_hTreeWnd);
				Record *pRec = getRecordByHandle(hItem);
				if(hItem && pRec && !pRec->isDir)
				{
					if(m_pCallback)
					{
						char szFile[MAX_PATH];
						szFile[0] = 0;
						makePath(szFile, pRec);
						strcat(szFile, ".eff");
						m_pCallback->onSelected(szFile);
						m_pCallback = NULL;
					}
					ShowWindow(m_hDlgWnd, SW_HIDE);
				}
				//EndDialog(m_hDlgWnd, 1);
			}
			break;
		case IDCANCEL:
			//EndDialog(m_hDlgWnd, 0);
			SAFE_CALL(m_pCallback, onCancelled);
			m_pCallback = NULL;
			ShowWindow(m_hDlgWnd, SW_HIDE);
			break;
		}
		break;

	case WM_SHOWWINDOW:
		if(!wParam)
		{
			TreeView_SelectItem(m_hTreeWnd, 0);
			mem_release(m_pPlayer);
		}
		break;
		
	default:
		return(FALSE);
	}
	return(TRUE);
}

#if 0
const char *CEffectBrowserWindow::getLevelName()
{
	INT_PTR res = DialogBoxParamA(m_hInstance, MAKEINTRESOURCE(IDD_LEVEL_OPEN), m_hMainWnd, DlgProc, (LPARAM)this);
	if(!res || res == -1)
	{
		return(NULL);
	}

	return(m_szLevelName);
}

void CEffectBrowserWindow::loadLevels()
{
	CLevelInfo levelInfo;

	LVITEMW lvItem;
	memset(&lvItem, 0, sizeof(lvItem));
	lvItem.mask = LVIF_TEXT | LVIF_PARAM;
	
	LVFINDINFOA lvFindInfo;
	memset(&lvFindInfo, 0, sizeof(lvFindInfo));
	lvFindInfo.flags = LVFI_PARAM;

	UINT uIndex = 0;

	while(EnumLevels(&levelInfo))
	{
		StringW tmp2(CMB2WC(levelInfo.m_szName));

		lvItem.pszText = (LPWSTR)tmp2.c_str();
		lvItem.cchTextMax = tmp2.length();
		lvItem.lParam = (LPARAM)uIndex;
		SNDMSG(m_hListWnd, LVM_INSERTITEMW, 0, (LPARAM)(const LV_ITEMW *)(&lvItem));

		
		lvFindInfo.lParam = (LPARAM)uIndex;
		int iID = ListView_FindItem(m_hListWnd, -1, &lvFindInfo);

 		// ListView_SetItemText(m_hListWnd, iID, 1, (LPSTR)levelInfo.m_szLocalName);
		ListView_SetItemText(m_hListWnd, iID, 2, (LPSTR)levelInfo.m_szLastEdit);
		ListView_SetItemText(m_hListWnd, iID, 3, (LPSTR)(levelInfo.m_isIndoor ? "Indoor" : "Outdoor"));


		StringW tmp(CMB2WC(levelInfo.m_szLocalName));
		LV_ITEMW _macro_lvi;
		_macro_lvi.iSubItem = 1;
		_macro_lvi.pszText = (LPWSTR)tmp.c_str();
		SNDMSG(m_hListWnd, LVM_SETITEMTEXTW, (WPARAM)iID, (LPARAM)(LV_ITEM *)&_macro_lvi);
		
		++uIndex;
	}
}

#endif


void CEffectBrowserWindow::registerClass()
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
	//wcex.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_ICON_LOGO));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);

	hBrush = CreateSolidBrush(RGB(0, 0, 0));


	wcex.hbrBackground = (HBRUSH)hBrush;
	wcex.lpszClassName = "XEffectBrowser";
	//wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	if(!RegisterClassEx(&wcex))
	{
		MessageBox(NULL, "Unable to register window class!", "Error", MB_OK | MB_ICONSTOP);
	}
}

LRESULT CALLBACK CEffectBrowserWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CEffectBrowserWindow *pThis = (CEffectBrowserWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(pThis)
	{
		return(pThis->wndProc(hWnd, msg, wParam, lParam));
	}
	return(DefWindowProc(hWnd, msg, wParam, lParam));
}

LRESULT CALLBACK CEffectBrowserWindow::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//int xPos;
	switch(msg)
	{
#if 0
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
#endif
	default:
		return(DefWindowProc(hWnd, msg, wParam, lParam));
	}
	return(0);
}

LRESULT CALLBACK CEffectBrowserWindow::EditBoxCtl_SubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CEffectBrowserWindow *pThis = (CEffectBrowserWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	LRESULT ret = CallWindowProcW(pThis->m_pEditBoxPrevWndProc, hWnd, message, wParam, lParam);
	if(message == WM_GETDLGCODE)
	{
		MSG *lpmsg = (MSG*)lParam;
		if(lpmsg && lpmsg->message == WM_KEYDOWN &&
			(lpmsg->wParam == VK_ESCAPE || lpmsg->wParam == VK_RETURN)
		)
		{
			ret |= DLGC_WANTALLKEYS;
		}
	}

	return(ret);
}

LRESULT CALLBACK CEffectBrowserWindow::TreeViewCtl_SubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CEffectBrowserWindow *pThis = (CEffectBrowserWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	LRESULT ret = CallWindowProcW(pThis->m_pTreeViewPrevWndProc, hWnd, message, wParam, lParam);
	switch(message)
	{
	case WM_GETDLGCODE:
		{
			MSG *lpmsg = (MSG*)lParam;
			if(lpmsg && lpmsg->message == WM_KEYDOWN && lpmsg->wParam == VK_F2)
			{
				ret |= DLGC_WANTALLKEYS;
			}
		}
		break;

	case WM_KEYUP:
		if(wParam == VK_F2)
		{
			HTREEITEM hItem = TreeView_GetSelection(hWnd);
			if(hItem)
			{
				TreeView_EditLabel(hWnd, hItem);
			}
		}
		break;
	}

	return(ret);
}

void CEffectBrowserWindow::scanFileSystem()
{
	m_aRecords.clearFast();
	char szTemp[MAX_PATH];
	scanFileSystemLevel("effects", 0, szTemp);
}
void CEffectBrowserWindow::scanFileSystemLevel(const char *szCurrentFolder, UINT uParent, char *szTempBuf)
{
	m_aRecords.push_back({basename(szCurrentFolder), uParent, true});

	uParent = m_aRecords.size() - 1;

	IFileIterator *pIter = m_pFS->getFolderList(szCurrentFolder);
	if(pIter)
	{
		const char *szDirName;
		while((szDirName = pIter->next()))
		{
			scanFileSystemLevel(szDirName, uParent, szTempBuf);
		}
		mem_release(pIter);
	}

	pIter = m_pFS->getFileList(szCurrentFolder, "eff");
	if(pIter)
	{
		const char *szFileName;
		while((szFileName = pIter->next()))
		{
			strcpy_s(szTempBuf, MAX_PATH, basename(szFileName));
			szTempBuf[strlen(szTempBuf) - 4] = 0;
			m_aRecords.push_back({szTempBuf, uParent, false});
		}
		mem_release(pIter);
	}
}

static HTREEITEM InsertItem(HWND hWnd, const wchar_t *wszText, TVINSERTSTRUCTW &tvins)
{
	tvins.item.pszText = (LPWSTR)wszText;
	tvins.item.cchTextMax = (int)wcslen(tvins.item.pszText);

	return((HTREEITEM)SNDMSG((hWnd), TVM_INSERTITEMW, 0, (LPARAM)(LPTV_INSERTSTRUCTW)(&tvins)));
}

void CEffectBrowserWindow::createTree(UINT uStartFrom)
{
	if(!uStartFrom)
	{
		TreeView_DeleteAllItems(m_hTreeWnd);
	}

	TVINSERTSTRUCTW tvins;

	tvins.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
	tvins.hInsertAfter = TVI_LAST;

	//m_aRecords[0].hTreeItem = TVI_ROOT;

	for(UINT i = uStartFrom, l = m_aRecords.size(); i < l; ++i)
	{
		Record &rec = m_aRecords[i];
		if(rec.isVisible || !i)
		{
			if(i)
			{
				Record &parent = m_aRecords[rec.uParent];
				tvins.hParent = parent.hTreeItem;
			}
			else
			{
				tvins.hParent = TVI_ROOT;
			}

			tvins.item.iImage = tvins.item.iSelectedImage = rec.isDir ? SIID_FOLDER : SIID_DOCNOASSOC;

			tvins.item.stateMask = TVIS_EXPANDED;
			tvins.item.state = (rec.isUserInteracted && !rec.isUserCollapsed || m_hasFilter && rec.hasVisibleChild) || !i ? TVIS_EXPANDED : 0;

			//if(rec.hTreeItem)
			//{
			//	TreeView_SetItemState(m_hTreeWnd, rec.hTreeItem, tvins.item.state, tvins.item.stateMask);
			//}
			//else
			{
				//UINT uPrev = i - 1;
				//while(uPrev > 0)
				//{
				//	Record &prev = m_aRecords[uPrev];
				//	if(prev.uParent != rec.uParent)
				//	{
				//		tvins.hInsertAfter = TVI_LAST;
				//		break;
				//	}
				//	if(prev.hTreeItem)
				//	{
				//		tvins.hInsertAfter = prev.hTreeItem;
				//		break;
				//	}
				//	--uPrev;
				//}
				
				rec.hTreeItem = InsertItem(m_hTreeWnd, CMB2WC(rec.sName.c_str()), tvins);
			}
		}
		//else if(rec.hTreeItem)
		//{
		//	TreeView_DeleteItem(m_hTreeWnd, rec.hTreeItem);
		//	rec.hTreeItem = NULL;
		//}
	}
}

void CEffectBrowserWindow::filter()
{
	char szFilter[256];
	GetDlgItemText(m_hDlgWnd, IDC_EDIT_FILTER, szFilter, sizeof(szFilter));
	szFilter[sizeof(szFilter) - 1] = 0;

	m_hasFilter = szFilter[0] != 0;

	if(!szFilter[0])
	{
		for(UINT i = 1, l = m_aRecords.size(); i < l; ++i)
		{
			Record &rec = m_aRecords[i];

			rec.isFilterPassed = true;
			rec.isVisible = true;
		}
	}
	else
	{
		for(UINT i = 1, l = m_aRecords.size(); i < l; ++i)
		{
			Record &rec = m_aRecords[i];

			rec.hasVisibleChild = false;
			rec.isVisible = rec.isFilterPassed = strcasestr(rec.sName.c_str(), szFilter) != NULL;

			if(rec.isFilterPassed)
			{
				UINT uParent = rec.uParent;
				while(uParent)
				{
					Record &parent = m_aRecords[uParent];
					parent.isVisible = true;
					parent.hasVisibleChild = true;
					uParent = parent.uParent;
				}
			}
			else
			{
				UINT uParent = rec.uParent;
				while(uParent)
				{
					Record &parent = m_aRecords[uParent];
					if(parent.isFilterPassed)
					{
						rec.isVisible = true;
						break;
					}
					uParent = parent.uParent;
				}
			}
		}
	}

	createTree();
}

CEffectBrowserWindow::Record* CEffectBrowserWindow::getRecordByHandle(HTREEITEM hItem, _out UINT *puIndex)
{
	int idx = m_aRecords.indexOf(hItem, [](const Record &r, HTREEITEM h){
		return(r.hTreeItem == h);
	});
	
	if(idx >= 0)
	{
		if(puIndex)
		{
			*puIndex = (UINT)idx;
		}
		return(&m_aRecords[idx]);
	}

	return(NULL);
}

bool CEffectBrowserWindow::renameItem(HTREEITEM hItem, const char *szName)
{
	Record *pRec = getRecordByHandle(hItem);
	if(pRec)
	{
		if(hasChildWithName(pRec->uParent, szName))
		{
			MessageBoxW(m_hDlgWnd, L"Can't rename because a file or folder with that name already exists", L"File and Folder Rename", MB_OK | MB_ICONSTOP);
			/*{
				TreeView_EditLabel(m_hTreeWnd, lpnmtvinfo->item.hItem);
				HWND hEditBoxWnd = TreeView_GetEditControl(m_hTreeWnd);
				SetWindowTextW(hEditBoxWnd, lpnmtvinfo->item.pszText);
			}*/
			return(false);
		}

		// call FS to rename file
		// rescan dir level
		// update tree level
		char szPrevName[MAX_PATH], szNewName[MAX_PATH];
		szPrevName[0] = szNewName[0] = 0;
		makePath(szPrevName, pRec);

		strcpy(szNewName, szPrevName);
		dirname(szNewName);
		strcat(szNewName, szName);

		TODO("Implement me!");
	//	if(m_pFS->rename(szPrevName, szNewName))
	//	{
	//		pRec->sName = szName;
	//		return(true);
	//	}

	}
	return(false);
}

void CEffectBrowserWindow::makePath(char *szBuf, Record *pRec)
{
	UINT uParent = pRec->uParent;
	Record *pParent = &m_aRecords[uParent];
	if(pParent != pRec)
	{
		makePath(szBuf, pParent);
		strcat(szBuf, "/");
	}
	strcat(szBuf, pRec->sName.c_str());
}

bool CEffectBrowserWindow::hasChildWithName(UINT uParent, const char *szName)
{
	for(UINT i = 1, l = m_aRecords.size(); i < l; ++i)
	{
		Record *pCur = &m_aRecords[i];
		if(pCur->uParent == uParent && !strcmp(szName, pCur->sName.c_str()))
		{
			return(true);
		}
	}

	return(false);
}

void CEffectBrowserWindow::removeRecord(UINT uIndex)
{
	for(UINT i = uIndex + 1, l = m_aRecords.size(); i < l; ++i)
	{
		Record &rec = m_aRecords[i];
		if(rec.uParent == uIndex)
		{
			removeRecord(i);
		}
	}

	m_aRecords.erase(uIndex);

	for(UINT i = uIndex, l = m_aRecords.size(); i < l; ++i)
	{
		Record &rec = m_aRecords[i];
		if(rec.uParent >= uIndex)
		{
			--rec.uParent;
		}
	}
}
