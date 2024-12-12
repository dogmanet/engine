#include "PropertyWindow.h"
#include "resource.h"
#include <xcommon/resource/IXResourceManager.h>
#include <core/sxcore.h>

#include <windowsx.h>
#include <commctrl.h>
#include <Commdlg.h>

class CEditorPropertyTabCallback final: public IXEditorPropertyTabCallback
{
public:
	CEditorPropertyTabCallback(CPropertyWindow *pPropertyWindow, IXEditorPropertyTab *pTab):
		m_pPropertyWindow(pPropertyWindow),
		m_pTab(pTab)
	{
	}
	~CEditorPropertyTabCallback()
	{
		m_pTab->setCallback(NULL);
	}

	void XMETHODCALLTYPE onAdd() override
	{
		m_pPropertyWindow->onAddTab(this);
	}
	void XMETHODCALLTYPE onRemove() override
	{
		m_pPropertyWindow->onRemoveTab(this);
	}

	IXEditorPropertyTab* getTab()
	{
		return(m_pTab);
	}

	void setTabIndex(int id)
	{
		m_iTabId = id;
	}

	int getTabIndex()
	{
		return(m_iTabId);
	}

private:
	CPropertyWindow *m_pPropertyWindow;
	IXEditorPropertyTab *m_pTab;
	int m_iTabId = -1;
};

//##########################################################################

static UINT g_uEditorDlgIds[] = {
	IDD_PROPEDIT_TEXT,
	IDD_PROPEDIT_FILE,
	IDD_PROPEDIT_COMBO,
	~0u,
	IDD_PROPEDIT_TEXT,
	IDD_PROPEDIT_TEXT,
	IDD_PROPEDIT_TEXT,
	IDD_PROPEDIT_TEXT,
	IDD_PROPEDIT_TEXT,
	IDD_PROPEDIT_TEXT,
};

static_assert(ARRAYSIZE(g_uEditorDlgIds) == XPET__LAST, "g_uEditorDlgIds must match X_PROP_EDITOR_TYPE");

WNDPROC CPropertyWindow::m_defEditProc = NULL;

CPropertyWindow::CPropertyWindow(HINSTANCE hInstance, HWND hMainWnd):
	m_hInstance(hInstance)
{
	CreateDialogParamA(m_hInstance, MAKEINTRESOURCE(IDD_DIALOG1), hMainWnd, DlgProc, (LPARAM)this);
	// show();

	// SetWindowPos(m_hDlgWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

CPropertyWindow::~CPropertyWindow()
{
	for(UINT i = 0, l = m_aCustomTabs.size(); i < l; ++i)
	{
		mem_delete(m_aCustomTabs[i]);
	}

	DestroyWindow(m_hDlgWnd);
}

void CPropertyWindow::show()
{
	ShowWindow(m_phEditors[m_editorActive], SW_HIDE);
	ShowWindow(m_hDlgWnd, SW_SHOWNA);
}
void CPropertyWindow::hide()
{
	ShowWindow(m_hDlgWnd, SW_HIDE);
	ShowWindow(m_phEditors[m_editorActive], SW_HIDE);
	m_resourceBrowserCallback.cancel();
}
bool CPropertyWindow::isVisible()
{
	return(IsWindowVisible(m_hDlgWnd));
}

INT_PTR CALLBACK CPropertyWindow::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CPropertyWindow *pThis = (CPropertyWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lParam);
		pThis = (CPropertyWindow*)lParam;
		pThis->m_hDlgWnd = hWnd;
	}
	if(pThis)
	{
		return(pThis->dlgProc(hWnd, msg, wParam, lParam));
	}
	return(FALSE);
}

INT_PTR CALLBACK CPropertyWindow::PropDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	/*case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLORBTN:
	case WM_CTLCOLORDLG:
	case WM_CTLCOLORLISTBOX:
		return((INT_PTR)GetSysColorBrush(COLOR_WINDOW));
		*/
	case WM_COMMAND:
	case WM_NOTIFY:
		return(SendMessage(GetParent(hWnd), msg, wParam, lParam));

	case WM_LBUTTONDOWN:
		SetFocus(GetParent(hWnd));
		return(TRUE);

	}
	return(FALSE);
}

INT_PTR CALLBACK CPropertyWindow::EditorDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_COMMAND:
	case WM_NOTIFY:
		return(SendMessage(GetParent(hWnd), msg, wParam, lParam));

	case WM_LBUTTONDOWN:
		SetFocus(GetParent(GetParent(hWnd)));
		return(TRUE);

	}
	return(FALSE);
}

LRESULT CALLBACK CPropertyWindow::EditEnterDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_CHAR:
		if(wParam == VK_RETURN)
		{
			SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(IDC_OPE_FILE, EN_KILLFOCUS), (LPARAM)hWnd);
			return(0);
		}
	}
	return(CallWindowProc(m_defEditProc, hWnd, msg, wParam, lParam));
}

LRESULT CALLBACK CPropertyWindow::ClassListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CPropertyWindow *pThis = (CPropertyWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	switch(message)
	{
	case WM_KEYDOWN:
		switch(wParam)
		{
		case VK_ESCAPE:
		case VK_RETURN:
			ComboBox_ShowDropdown(GetParent(hWnd), FALSE);
			SetFocus(pThis->m_hDlgWnd);
			break;
		}
		break;
	}

	return(CallWindowProc(pThis->m_pfnClassListOldWndproc, hWnd, message, wParam, lParam));
}

INT_PTR CALLBACK CPropertyWindow::dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			m_hTabControl = GetDlgItem(hWnd, IDC_TAB1);

			TCITEM ti;
			ti.mask = TCIF_TEXT;
			ti.pszText = "Properties";
			TabCtrl_InsertItem(m_hTabControl, 0, &ti);
			ti.pszText = "Flags";
			TabCtrl_InsertItem(m_hTabControl, 1, &ti);
			// ti.pszText = "Outputs";
			// TabCtrl_InsertItem(m_hTabControl, 2, &ti);
			TabCtrl_SetCurSel(m_hTabControl, 0);

			RECT rcDisplay = {0};
			GetWindowRect(m_hTabControl, &rcDisplay);
			MapWindowPoints(HWND_DESKTOP, m_hDlgWnd, (LPPOINT)&rcDisplay, 2);
			TabCtrl_AdjustRect(m_hTabControl, FALSE, &rcDisplay);

			m_hPropTabs[0] = CreateDialog(m_hInstance, MAKEINTRESOURCE(IDD_OP_PROPS), m_hDlgWnd, PropDlgProc);
			m_hPropTabs[1] = CreateDialog(m_hInstance, MAKEINTRESOURCE(IDD_OP_FLAGS), m_hDlgWnd, PropDlgProc);
			// m_hPropTabs[2] = CreateDialog(m_hInstance, MAKEINTRESOURCE(IDD_OP_OUTPUTS), m_hDlgWnd, NULL);

			const int iXDelta = -3;

			m_iTabX = rcDisplay.left + iXDelta;
			m_iTabY = rcDisplay.top;
			m_iTabCX = rcDisplay.right - rcDisplay.left - iXDelta;
			m_iTabCY = rcDisplay.bottom - rcDisplay.top;

			for(UINT i = 0, l = m_hPropTabs.size(); i < l; ++i)
			{
				SetWindowPos(m_hPropTabs[i], HWND_TOP, m_iTabX, m_iTabY, m_iTabCX, m_iTabCY, 0);
			}
			
			onSelChanged();

			m_hNoPropsText = GetDlgItem(m_hDlgWnd, IDC_NO_PROPS);

			m_hClassListWnd = GetDlgItem(m_hPropTabs[0], IDC_CLASS_NAME);
			COMBOBOXINFO info = {sizeof(COMBOBOXINFO)};
			GetComboBoxInfo(m_hClassListWnd, &info);
			SetWindowLongPtr(info.hwndItem, GWLP_USERDATA, (LONG_PTR)this);
			m_pfnClassListOldWndproc = (WNDPROC)GetWindowLongPtr(info.hwndItem, GWLP_WNDPROC);
			SetWindowLongPtr(info.hwndItem, GWLP_WNDPROC, (LONG_PTR)ClassListWndProc);

			m_hPropListWnd = GetDlgItem(m_hPropTabs[0], IDC_LIST1);
			ListView_SetExtendedListViewStyle(m_hPropListWnd, ListView_GetExtendedListViewStyle(m_hPropListWnd) | LVS_EX_FULLROWSELECT);

			LV_COLUMNA lvColumn;
			memset(&lvColumn, 0, sizeof(lvColumn));

			RECT rc;
			GetClientRect(m_hPropListWnd, &rc);

			lvColumn.mask = LVCF_WIDTH | LVCF_TEXT;
			lvColumn.cx = (rc.right - rc.left) / 2;
			lvColumn.pszText = "Property";
			lvColumn.cchTextMax = strlen(lvColumn.pszText);
			ListView_InsertColumn(m_hPropListWnd, 0, &lvColumn);

			lvColumn.pszText = "Value";
			lvColumn.cchTextMax = strlen(lvColumn.pszText);
			ListView_InsertColumn(m_hPropListWnd, 1, &lvColumn);


			HWND hEditorArea = GetDlgItem(m_hPropTabs[0], IDC_EDITAREA);

			GetWindowRect(hEditorArea, &rcDisplay);
			MapWindowPoints(HWND_DESKTOP, m_hPropTabs[0], (LPPOINT)&rcDisplay, 2);
			ShowWindow(hEditorArea, SW_HIDE);

			int aiTextIds[] = {IDC_OPE_TEXT, IDC_OPE_FILE};

			for(int i = 0; i < XPET__LAST; ++i)
			{
				if(g_uEditorDlgIds[i] == ~0u)
				{
					m_phEditors[i] = NULL;
				}
				else
				{
					m_phEditors[i] = CreateDialog(m_hInstance, MAKEINTRESOURCE(g_uEditorDlgIds[i]), m_hPropTabs[0], EditorDlgProc);
					SetWindowPos(m_phEditors[i], HWND_TOP, rcDisplay.left, rcDisplay.top, 0, 0, SWP_NOSIZE);

					for(int j = 0, jl = ARRAYSIZE(aiTextIds); j < jl; ++j)
					{
						HWND wnd = GetDlgItem(m_phEditors[i], aiTextIds[j]);
						if(wnd)
						{
							if(!m_defEditProc)
							{
								m_defEditProc = (WNDPROC)GetWindowLongPtr(wnd, GWLP_WNDPROC);
							}
							SetWindowLongPtr(wnd, GWLP_WNDPROC, (LPARAM)EditEnterDlgProc);
						}
					}
				}
			}

			//ShowWindow(m_phEditors[XPET__LAST - 1], SW_SHOW);
			break;
		}
	case WM_CLOSE:
		//ShowWindow(hWnd, SW_HIDE);
		hide();
		if(m_pCallback)
		{
			m_pCallback->onApply();
		}
		break;
	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->code)
		{
		case TCN_SELCHANGE:
			onSelChanged();
			break;

		case LVN_ITEMCHANGED:
			if(((LPNMHDR)lParam)->idFrom == IDC_LIST1)
			{
				onPropListChanged();
			}
			break;
		}
		break;

	case WM_COMMAND:
		switch(HIWORD(wParam))
		{
		case CBN_EDITCHANGE:
			if(1)
			{
				int iLen = ComboBox_GetTextLength(m_hClassListWnd) + 1;
				char *szTemp = (char*)alloca(sizeof(char)* iLen);
				ComboBox_GetText(m_hClassListWnd, szTemp, iLen);
				filterClassList(szTemp);
			}
			break;

		case CBN_SELCHANGE:
			if(LOWORD(wParam) == IDC_OPE_COMBO)
			{
				HWND hCombo = (HWND)lParam;
				int iSel = ComboBox_GetCurSel(hCombo);
				if(iSel >= 0)
				{
					const char *szValue = (const char*)ComboBox_GetItemData(hCombo, iSel);

					int iSelLB = ListView_GetNextItem(m_hPropListWnd, -1, LVNI_SELECTED);
					if(iSelLB >= 0)
					{
						LVITEM lvItem;
						memset(&lvItem, 0, sizeof(lvItem));
						lvItem.iItem = iSelLB;
						lvItem.mask = LVIF_PARAM;
						ListView_GetItem(m_hPropListWnd, &lvItem);
						prop_s *pField = &m_aPropFields[AAString((char*)lvItem.lParam)];

						setPropFieldValue(pField->field.szKey, szValue);
						if(m_pCallback)
						{
							m_pCallback->onPropertyChanged(pField->field.szKey, szValue);
						}
					}
				}
			}
			else if(LOWORD(wParam) == IDC_CLASS_NAME)
			{
				if(m_pCallback)
				{
					HWND hCombo = (HWND)lParam;
					int iSel = ComboBox_GetCurSel(hCombo);
					if(iSel >= 0)
					{
						int iLen = ComboBox_GetTextLength(hCombo) + 1;
						char *szTemp = (char*)alloca(sizeof(char)* iLen);
						ComboBox_GetText(hCombo, szTemp, iLen);
						m_pCallback->onClassChanged(szTemp);
					}
				}
			}
			break;

		case EN_KILLFOCUS:
			if(LOWORD(wParam) == IDC_OPE_TEXT || LOWORD(wParam) == IDC_OPE_FILE)
			{
				HWND hEdit = (HWND)lParam;

				if(GetParent(hEdit) == m_phEditors[m_editorActive])
				{
					int iValLen = Edit_GetTextLength(hEdit) + 1;
					char *szValue = (char*)alloca(sizeof(char) * iValLen);
					Edit_GetText(hEdit, szValue, iValLen);

					int iSel = ListView_GetNextItem(m_hPropListWnd, -1, LVNI_SELECTED);
					if(iSel >= 0)
					{
						LVITEM lvItem;
						memset(&lvItem, 0, sizeof(lvItem));
						lvItem.iItem = iSel;
						lvItem.mask = LVIF_PARAM;
						ListView_GetItem(m_hPropListWnd, &lvItem);
						prop_s *pField = &m_aPropFields[AAString((char*)lvItem.lParam)];

						setPropFieldValue(pField->field.szKey, szValue);
						if(m_pCallback)
						{
							m_pCallback->onPropertyChanged(pField->field.szKey, szValue);
						}
					}
				}
			}
			break;
		}

		if(LOWORD(wParam) == IDC_OPE_FILE_SELECT)
		{
			IXEditorResourceBrowser *pBrowser;
			if(g_pEditor->getResourceBrowserForType(szCurrentFileType, &pBrowser))
			{
				HWND hEditWnd = GetDlgItem(m_phEditors[m_editorActive], IDC_OPE_FILE);
				int iTextLen = GetWindowTextLengthW(hEditWnd);
				wchar_t *wszText = (wchar_t*)alloca(sizeof(wchar_t) * (iTextLen + 1));
				GetWindowTextW(hEditWnd, wszText, iTextLen);

				//EnableWindow(hWnd, FALSE);
				m_resourceBrowserCallback.init(hWnd, hEditWnd, pBrowser);

				pBrowser->browse(szCurrentFileType, CWC2MB(wszText), &m_resourceBrowserCallback);
				mem_release(pBrowser);
				break;
			}

			OPENFILENAMEW ofn;
			wchar_t szFile[1024], szFilter[1024];
			memset(szFile, 0, sizeof(szFile));

			szFilter[0] = 0;
			wchar_t *szTmp = szFilter;

			if(!szCurrentFileType)
			{
				szTmp += swprintf(szTmp, L"All files") + 1;
				szTmp += swprintf(szTmp, L"*.*") + 1;
			}
			else if(!fstrcmp(szCurrentFileType, "model"))
			{
				auto pMgr = Core_GetIXCore()->getResourceManager();
				UINT uFormatCount = pMgr->getModelSupportedFormats();

				szTmp += swprintf(szTmp, L"All supported formats") + 1;
				for(UINT i = 0; i < uFormatCount; ++i)
				{
					auto pFmt = pMgr->getModelSupportedFormat(i);
					szTmp += swprintf(szTmp, L"*.%S;", pFmt->szExtension);
				}
				szTmp[-1] = 0;
				for(UINT i = 0; i < uFormatCount; ++i)
				{
					auto pFmt = pMgr->getModelSupportedFormat(i);
					szTmp += swprintf(szTmp, L"%S (*.%S)", pFmt->szDescription, pFmt->szExtension) + 1;
					szTmp += swprintf(szTmp, L"*.%S", pFmt->szExtension) + 1;
				}
			}
			else if(!fstrcmp(szCurrentFileType, "sound"))
			{
				/*szTmp += swprintf(szTmp, L"All supported formats") + 1;
				szTmp += swprintf(szTmp, L"*.ogg") + 1;

				szTmp += swprintf(szTmp, L"Ogg Vorbis (*.ogg)") + 1;
				szTmp += swprintf(szTmp, L"*.ogg") + 1;*/

				auto pMgr = Core_GetIXCore()->getResourceManager();
				UINT uFormatCount = pMgr->getSoundSupportedFormats();

				szTmp += swprintf(szTmp, L"All supported formats") + 1;
				for (UINT i = 0; i < uFormatCount; ++i)
				{
					auto pFmt = pMgr->getSoundSupportedFormat(i);
					szTmp += swprintf(szTmp, L"*.%S;", pFmt->szExtension);
				}
				szTmp[-1] = 0;
				for (UINT i = 0; i < uFormatCount; ++i)
				{
					auto pFmt = pMgr->getSoundSupportedFormat(i);
					szTmp += swprintf(szTmp, L"%S (*.%S)", pFmt->szDescription, pFmt->szExtension) + 1;
					szTmp += swprintf(szTmp, L"*.%S", pFmt->szExtension) + 1;
				}
			}
			else if(!fstrcmp(szCurrentFileType, "texture"))
			{
				auto pMgr = Core_GetIXCore()->getResourceManager();
				UINT uFormatCount = pMgr->getTextureSupportedFormats();

				szTmp += swprintf(szTmp, L"All supported formats") + 1;
				for(UINT i = 0; i < uFormatCount; ++i)
				{
					auto pFmt = pMgr->getTextureSupportedFormat(i);
					szTmp += swprintf(szTmp, L"*.%S;", pFmt->szExtension);
				}
				szTmp[-1] = 0;
				for(UINT i = 0; i < uFormatCount; ++i)
				{
					auto pFmt = pMgr->getTextureSupportedFormat(i);
					szTmp += swprintf(szTmp, L"%S (*.%S)", pFmt->szDescription, pFmt->szExtension) + 1;
					szTmp += swprintf(szTmp, L"*.%S", pFmt->szExtension) + 1;
				}
			}
			else
			{
				szTmp += swprintf(szTmp, L"All files") + 1;
				szTmp += swprintf(szTmp, L"*.*") + 1;
			}
			szTmp[0] = 0;

			memset(&ofn, 0, sizeof(OPENFILENAMEW));
			ofn.lStructSize = sizeof(OPENFILENAMEW);
			ofn.hwndOwner = NULL;
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = szFilter;
			ofn.nFilterIndex = 0;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

			if(GetOpenFileNameW(&ofn))
			{
				IFileSystem *pFS = Core_GetIXCore()->getFileSystem();
				wchar_t *str = szFile;
				while(*str)
				{
					if(*str == L'\\')
					{
						*str = L'/';
					}
					++str;
				}

				bool isFound = false;
				for(UINT i = 0, l = pFS->getRootCount(); i < l && !isFound; ++i)
				{
					const char *szDir = pFS->getRoot(i);
					//printf("%s\n", szDir);
					str = szFile;
					while(*str && *szDir && *str == *szDir)
					{
						++str; ++szDir;
					}
					if(str[0] == L'/')
					{
						++str;
					}
					if(!*szDir)
					{
						isFound = true;
					}
				}
				if(!isFound)
				{
					str = szFile;
				}


				HWND hEditWnd = GetDlgItem(m_phEditors[m_editorActive], IDC_OPE_FILE);
				SetWindowTextW(hEditWnd, str);
				SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_OPE_FILE, EN_KILLFOCUS), (LPARAM)hEditWnd);
			}
		}
		else if(LOWORD(wParam) == IDAPPLY)
		{
			if(m_pCallback)
			{
				m_pCallback->onApply();
			}
			hide();
		}
		else if(LOWORD(wParam) == IDCANCEL)
		{
			if(m_pCallback)
			{
				m_pCallback->onCancel();
			}
			hide();
		}
		else if(LOWORD(wParam) >= IDC_CHECK1 && LOWORD(wParam) <= IDC_CHECK16)
		{
			HWND hButton = (HWND)lParam;

			UINT uIdx = LOWORD(wParam) - IDC_CHECK1;
			UINT uChk = Button_GetCheck(hButton);
			if(uChk == BST_CHECKED)
			{
				m_uCurrentFlags |= 1u << (16 + uIdx);
			}
			else if(uChk == BST_UNCHECKED)
			{
				m_uCurrentFlags &= ~(1u << (16 + uIdx));
			}

			char tmp[16];
			sprintf(tmp, "%u", m_uCurrentFlags);

			setPropFieldValue(m_szFlagsField, tmp);
			if(m_pCallback)
			{
				m_pCallback->onPropertyChanged(m_szFlagsField, tmp);
			}
		}
		break;

	default:
		return(FALSE);
	}
	return(TRUE);
}

void CPropertyWindow::onPropListChanged()
{
	if(ListView_GetSelectedCount(m_hPropListWnd))
	{
		int iSel = ListView_GetNextItem(m_hPropListWnd, -1, LVNI_SELECTED);
		if(iSel >= 0)
		{
			LVITEM lvItem;
			memset(&lvItem, 0, sizeof(lvItem));
			lvItem.iItem = iSel;
			lvItem.mask = LVIF_PARAM;
			ListView_GetItem(m_hPropListWnd, &lvItem);
			prop_s *pField = &m_aPropFields[AAString((char*)lvItem.lParam)];

			SetDlgItemTextW(m_hPropTabs[0], IDC_OP_HELP, pField->field.szHelp ? CMB2WC(pField->field.szHelp) : L"");

			initEditor(pField->field.editorType, pField->field.pEditorData, pField->sValue.c_str());
		}
	}
}

void CPropertyWindow::onSelChanged()
{
	int iSel = TabCtrl_GetCurSel(m_hTabControl);

	if(m_hCurrentTab)
	{
		ShowWindow(m_hCurrentTab, SW_HIDE);
		m_hCurrentTab = NULL;
	}
	if(m_pCurrentTab)
	{
		m_pCurrentTab->hide();
		m_pCurrentTab = NULL;
	}

	if(iSel < m_hPropTabs.size())
	{
		m_hCurrentTab = m_hPropTabs[iSel];
	}
	else
	{
		int i = m_aCustomTabs.indexOf(iSel, [](CEditorPropertyTabCallback *pTab, int iSel){
			return(pTab->getTabIndex() == iSel);
		});
		if(i >= 0)
		{
			m_pCurrentTab = m_aCustomTabs[i]->getTab();
		}
	}

	if(m_hCurrentTab)
	{
		ShowWindow(m_hCurrentTab, SW_SHOWNORMAL);
	}
	if(m_pCurrentTab)
	{
		m_pCurrentTab->show();
	}
}

void CPropertyWindow::clearClassList()
{
	m_asClassList.clearFast();
	ComboBox_Enable(m_hClassListWnd, FALSE);
	ComboBox_ResetContent(m_hClassListWnd);

	ShowWindow(m_hTabControl, SW_HIDE);
	ShowWindow(m_hNoPropsText, SW_SHOW);
	if(m_hCurrentTab)
	{
		ShowWindow(m_hCurrentTab, SW_HIDE);
	}
}
void CPropertyWindow::addClass(const char *szClassName)
{
	m_asClassList.push_back(szClassName);
	ComboBox_AddString(m_hClassListWnd, szClassName);
	int iCount = ComboBox_GetCount(m_hClassListWnd);
	if(iCount == 1)
	{
		ComboBox_SetCurSel(m_hClassListWnd, 0);
	}
	ComboBox_Enable(m_hClassListWnd, iCount > 1);
	ShowWindow(m_hTabControl, SW_SHOW);
	ShowWindow(m_hNoPropsText, SW_HIDE);
	if(m_hCurrentTab)
	{
		ShowWindow(m_hCurrentTab, SW_SHOW);
	}
}

void CPropertyWindow::filterClassList(const char *szFilter)
{
	int iLen = ComboBox_GetTextLength(m_hClassListWnd) + 1;
	char *szTemp = (char*)alloca(sizeof(char)* iLen);
	ComboBox_GetText(m_hClassListWnd, szTemp, iLen);

	DWORD dwRange = SendMessage(m_hClassListWnd, CB_GETEDITSEL, NULL, NULL);

	ComboBox_ShowDropdown(m_hClassListWnd, TRUE);
	SendMessage(m_hClassListWnd, WM_SETCURSOR, 0, 0);
	ComboBox_ResetContent(m_hClassListWnd);
	for(UINT i = 0, l = m_asClassList.size(); i < l; ++i)
	{
		if(!szFilter[0] || strstr(m_asClassList[i].c_str(), szFilter))
		{
			ComboBox_AddString(m_hClassListWnd, m_asClassList[i].c_str());
		}
	}
	ComboBox_SetCurSel(m_hClassListWnd, 0);
	ComboBox_SetText(m_hClassListWnd, szTemp);
	SendMessage(m_hClassListWnd, CB_SETEDITSEL, NULL, dwRange);
}

void CPropertyWindow::clearProps()
{
	m_resourceBrowserCallback.cancel();
	ShowWindow(m_phEditors[m_editorActive], SW_HIDE);
	ListView_DeleteAllItems(m_hPropListWnd);
	m_aPropFields.clear();
}
void CPropertyWindow::addPropField(const X_PROP_FIELD *pField, const char *szValue)
{
	m_aPropFields[AAString(pField->szKey)] = {*pField, szValue ? szValue : ""};

	if(pField->editorType == XPET_FLAGS)
	{
		const char **aszFlags = (const char**)pField->pEditorData;
		for(UINT i = 0; i < 16; ++i)
		{
			SetDlgItemTextA(m_hPropTabs[1], IDC_CHECK1 + i, aszFlags[i] ? aszFlags[i] : "");
		}
		m_szFlagsField = pField->szKey;
	}
	else
	{
		LVITEMA lvItem;
		memset(&lvItem, 0, sizeof(lvItem));
		lvItem.mask = LVIF_TEXT | LVIF_PARAM;
		lvItem.pszText = (LPSTR)pField->szName;
		lvItem.cchTextMax = strlen(lvItem.pszText);
		lvItem.lParam = (LPARAM)pField->szKey;
		ListView_InsertItem(m_hPropListWnd, &lvItem);
	}
	if(szValue)
	{
		setPropFieldValue(pField->szKey, szValue);
	}
}
void CPropertyWindow::setPropFieldValue(const char *szKey, const char *szValue)
{
	prop_s *pField = &m_aPropFields[AAString(szKey)];
	pField->sValue = szValue;

	if(pField->field.editorType == XPET_FLAGS)
	{
		UINT uFlags = 0;
		sscanf(szValue, "%u", &uFlags);
		for(UINT i = 0; i < 16; ++i)
		{
			CheckDlgButton(m_hPropTabs[1], IDC_CHECK1 + i, (uFlags & (1 << (i + 16))) > 0 ? BST_CHECKED : BST_UNCHECKED);
		}
		m_uCurrentFlags = uFlags;
	}
	else
	{
		LVFINDINFOA lvFindInfo;
		memset(&lvFindInfo, 0, sizeof(lvFindInfo));
		lvFindInfo.flags = LVFI_PARAM;
		lvFindInfo.lParam = (LPARAM)szKey;
		int iID = ListView_FindItem(m_hPropListWnd, -1, &lvFindInfo);

		ListView_SetItemText(m_hPropListWnd, iID, 1, (LPSTR)pField->sValue.c_str());
	}
	onPropListChanged();
}

void CPropertyWindow::initEditor(X_PROP_EDITOR_TYPE type, const void *pData, const char *szValue)
{
	if(m_editorActive != XPET__LAST)
	{
		ShowWindow(m_phEditors[m_editorActive], SW_HIDE);
		m_resourceBrowserCallback.cancel();
	}
	m_editorActive = type;
	HWND hEditorDlg = m_phEditors[m_editorActive];
	ShowWindow(hEditorDlg, SW_SHOW);

	switch(type)
	{
	case XPET_TEXT:
		SetDlgItemText(hEditorDlg, IDC_OPE_TEXT, szValue);
		break;
	case XPET_FILE:
		SetDlgItemText(hEditorDlg, IDC_OPE_FILE, szValue);
		szCurrentFileType = (char*)pData;
		break;
	case XPET_COMBO:
		if(1)
		{
			HWND hCombo = GetDlgItem(hEditorDlg, IDC_OPE_COMBO);
			ComboBox_ResetContent(hCombo);
			edt_kv *pKV = (edt_kv*)pData;
			while(pKV->szKey)
			{
				ComboBox_AddString(hCombo, pKV->szKey);
				ComboBox_SetItemData(hCombo, ComboBox_GetCount(hCombo) - 1, pKV->szValue);
				if(!fstrcmp(pKV->szValue, szValue))
				{
					ComboBox_SetCurSel(hCombo, ComboBox_GetCount(hCombo) - 1);
				}
				++pKV;
			}
		}
		break;
	}
}

void CPropertyWindow::setClassName(const char *szClassName)
{
	int idx = ComboBox_FindStringExact(m_hClassListWnd, -1, szClassName);
	if(idx == -1 && szClassName[0])
	{
		addClass(szClassName);
		idx = ComboBox_FindStringExact(m_hClassListWnd, -1, szClassName);
	}
	ComboBox_SetCurSel(m_hClassListWnd, idx);
}

void CPropertyWindow::allowClassChange(bool bAllow)
{
	ComboBox_Enable(m_hClassListWnd, bAllow ? 1 : 0);
}

void CPropertyWindow::addCustomTab(IXEditorPropertyTab *pTab)
{
	assert(m_iTabX);

	CEditorPropertyTabCallback *pCB = new CEditorPropertyTabCallback(this, pTab);

	m_aCustomTabs.push_back(pCB);
	pTab->init(m_hDlgWnd, m_iTabX, m_iTabY, m_iTabCX, m_iTabCY);
	pTab->setCallback(pCB);
}

void CPropertyWindow::onAddTab(CEditorPropertyTabCallback *pTabCB)
{
	IXEditorPropertyTab *pTab = pTabCB->getTab();

	TCITEM ti;
	ti.mask = TCIF_TEXT;
	ti.pszText = strdupa(pTab->getTitle());
	int idx = TabCtrl_InsertItem(m_hTabControl, TabCtrl_GetItemCount(m_hTabControl), &ti);
	pTabCB->setTabIndex(idx);
}
void CPropertyWindow::onRemoveTab(CEditorPropertyTabCallback *pTabCB)
{
	if(m_pCurrentTab == pTabCB->getTab())
	{
		TabCtrl_SetCurSel(m_hTabControl, 0);
		onSelChanged();
	}

	int iDel = pTabCB->getTabIndex();
	TabCtrl_DeleteItem(m_hTabControl, iDel);
	pTabCB->setTabIndex(-1);

	for(UINT i = 0, l = m_aCustomTabs.size(); i < l; ++i)
	{
		pTabCB = m_aCustomTabs[i];
		if(pTabCB->getTabIndex() > iDel)
		{
			pTabCB->setTabIndex(pTabCB->getTabIndex() - 1);
		}
	}
}

UINT CPropertyWindow::getCustomTabCount()
{
	return(m_aCustomTabs.size());
}
IXEditorPropertyTab* CPropertyWindow::getCustomTab(UINT idx)
{
	return(m_aCustomTabs[idx]->getTab());
}

//##########################################################################

void CEditorResourceBrowserCallback::init(HWND hDlgWnd, HWND hEditWnd, IXEditorResourceBrowser *pBrowser)
{
	if(m_pBrowser)
	{
		m_pBrowser->cancel();
	}
	assert(!m_pBrowser);
	m_hEditWnd = hEditWnd;
	m_hDlgWnd = hDlgWnd;
	add_ref(pBrowser);
	m_pBrowser = pBrowser;
}

void XMETHODCALLTYPE CEditorResourceBrowserCallback::onSelected(const char *szFile)
{
	//EnableWindow(m_hDlgWnd, TRUE);
	SetWindowTextW(m_hEditWnd, CMB2WC(szFile));
	SendMessage(m_hDlgWnd, WM_COMMAND, MAKEWPARAM(IDC_OPE_FILE, EN_KILLFOCUS), (LPARAM)m_hEditWnd);
	mem_release(m_pBrowser);
}

void XMETHODCALLTYPE CEditorResourceBrowserCallback::onCancelled()
{
	//EnableWindow(m_hDlgWnd, TRUE);
	mem_release(m_pBrowser);
}

void CEditorResourceBrowserCallback::cancel()
{
	SAFE_CALL(m_pBrowser, cancel);
}
