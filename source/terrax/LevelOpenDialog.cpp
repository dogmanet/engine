#include "LevelOpenDialog.h"
#include "resource.h"
#include <core/sxcore.h>

#include <windowsx.h>
#include <commctrl.h>

struct CLevelInfo
{
	char m_szName[MAX_LEVEL_STRING]; //!< имя папки уровня
	char m_szLocalName[MAX_LEVEL_STRING]; //!< Отображаемое имя уровня
	char m_szLastEdit[64];
	//bool m_bHasPreview;
	bool m_isIndoor;

	IFileIterator *m_pIterator = NULL; //!< для внутреннего использования
};

BOOL GetLastWriteTime(const char *szFile, LPTSTR lpszString)
{
	FILETIME ftCreate, ftAccess, ftWrite;
	SYSTEMTIME stUTC, stLocal;
	DWORD dwRet;

	HANDLE hFile = CreateFileW(CMB2WC(szFile), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		return(FALSE);
	}

	// Retrieve the file times for the file.
	if(!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
	{
		return(FALSE);
	}

	CloseHandle(hFile);

	// Convert the last-write time to local time.
	FileTimeToSystemTime(&ftWrite, &stUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

	// Build a string showing the date and time.
	sprintf(lpszString,
		TEXT("%02d.%02d.%d  %02d:%02d"),
		stLocal.wDay, stLocal.wMonth, stLocal.wYear,
		stLocal.wHour, stLocal.wMinute);

	return(TRUE);
}


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

CLevelOpenDialog::CLevelOpenDialog(HINSTANCE hInstance, HWND hMainWnd):
	m_hInstance(hInstance),
	m_hMainWnd(hMainWnd)
{
	//CreateDialogParamA(m_hInstance, MAKEINTRESOURCE(IDD_LEVEL_OPEN), hMainWnd, DlgProc, (LPARAM)this);
	//ShowWindow(m_hDlgWnd, SW_SHOWNA);
	//DialogBoxParamA(
}

CLevelOpenDialog::~CLevelOpenDialog()
{
	//DestroyWindow(m_hDlgWnd);
}

INT_PTR CALLBACK CLevelOpenDialog::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CLevelOpenDialog *pThis = (CLevelOpenDialog*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lParam);
		pThis = (CLevelOpenDialog*)lParam;
		pThis->m_hDlgWnd = hWnd;
	}
	if(pThis)
	{
		return(pThis->dlgProc(hWnd, msg, wParam, lParam));
	}
	return(FALSE);
}

INT_PTR CALLBACK CLevelOpenDialog::dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{

			m_hOkButtonWnd = GetDlgItem(m_hDlgWnd, IDOK);
			Button_Enable(m_hOkButtonWnd, FALSE);

			m_hListWnd = GetDlgItem(m_hDlgWnd, IDC_LIST1);
			ListView_SetExtendedListViewStyle(m_hListWnd, ListView_GetExtendedListViewStyle(m_hListWnd) | LVS_EX_FULLROWSELECT);

			LV_COLUMNA lvColumn;
			memset(&lvColumn, 0, sizeof(lvColumn));

			RECT rc;
			GetClientRect(m_hListWnd, &rc);

			lvColumn.mask = LVCF_WIDTH | LVCF_TEXT;
			lvColumn.cx = (rc.right - rc.left) / 4;
			lvColumn.pszText = "Name";
			lvColumn.cchTextMax = strlen(lvColumn.pszText);
			ListView_InsertColumn(m_hListWnd, 0, &lvColumn);

			lvColumn.pszText = "Local name";
			lvColumn.cchTextMax = strlen(lvColumn.pszText);
			ListView_InsertColumn(m_hListWnd, 1, &lvColumn);

			lvColumn.pszText = "Last edit";
			lvColumn.cchTextMax = strlen(lvColumn.pszText);
			ListView_InsertColumn(m_hListWnd, 2, &lvColumn);

			lvColumn.pszText = "Type";
			lvColumn.cchTextMax = strlen(lvColumn.pszText);
			ListView_InsertColumn(m_hListWnd, 3, &lvColumn);

			loadLevels();

			break;
		}
	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->code)
		{
		case LVN_ITEMCHANGED:
			if(((LPNMHDR)lParam)->idFrom == IDC_LIST1)
			{
				int iSel = ListView_GetNextItem(m_hListWnd, -1, LVNI_SELECTED);
				Button_Enable(m_hOkButtonWnd, iSel >= 0);
			}
			break;
		case NM_DBLCLK:
			{
				LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lParam;
				if(lpnmitem->iItem >= 0)
				{
					SendMessage(m_hDlgWnd, WM_COMMAND, MAKEWPARAM(IDOK, 0), NULL);
				}
			}
			break;
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			{
				int iSel = ListView_GetNextItem(m_hListWnd, -1, LVNI_SELECTED);
				LVITEMW lvItem;
				memset(&lvItem, 0, sizeof(lvItem));
				lvItem.iItem = iSel;
				lvItem.mask = LVIF_TEXT;
				WCHAR tmp[MAX_LEVEL_STRING];
				tmp[0] = 0;
				lvItem.pszText = tmp;
				lvItem.cchTextMax = MAX_LEVEL_STRING;
				//ListView_GetItem(m_hListWnd, &lvItem);
				SNDMSG(m_hListWnd, LVM_GETITEMW, 0, (LPARAM)(LV_ITEMW*)(&lvItem));
				strcpy(m_szLevelName, CWC2MB(tmp));
				EndDialog(m_hDlgWnd, 1);
			}
			break;
		case IDCANCEL:
			EndDialog(m_hDlgWnd, 0);
			break;
		}
		
	default:
		return(FALSE);
	}
	return(TRUE);
}

const char *CLevelOpenDialog::getLevelName()
{
	INT_PTR res = DialogBoxParamA(m_hInstance, MAKEINTRESOURCE(IDD_LEVEL_OPEN), m_hMainWnd, DlgProc, (LPARAM)this);
	if(!res || res == -1)
	{
		return(NULL);
	}

	return(m_szLevelName);
}

void CLevelOpenDialog::loadLevels()
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
