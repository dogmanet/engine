#include "stdafx.h"
#include "Extended.h"

//#include <windowsx.h>
//#include <commctrl.h>
//#include <Commdlg.h>

class CExtendedTabCallback final
{
public:
	CExtendedTabCallback(CExtended *pExtended, IExtendedTab *pTab):
		m_pExtended(pExtended),
		m_pTab(pTab)
	{
	}
	~CExtendedTabCallback()
	{
		mem_delete(m_pTab);
	}


	IExtendedTab* getTab()
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
	CExtended *m_pExtended;
	IExtendedTab *m_pTab;
	int m_iTabId = -1;
};

//##########################################################################

CExtended::CExtended(HWND hMainWnd)
{
	m_hDlgWnd = CreateDialogParamA(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG4), hMainWnd, DlgProc, (LPARAM)this);
	// show();

	// SetWindowPos(m_hDlgWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

CExtended::~CExtended()
{
	for(UINT i = 0, l = m_aCustomTabs.size(); i < l; ++i)
	{
		mem_delete(m_aCustomTabs[i]);
	}

	DestroyWindow(m_hDlgWnd);
}

void CExtended::show()
{
	ShowWindow(m_hDlgWnd, SW_SHOWNA);
}
void CExtended::hide()
{
	ShowWindow(m_hDlgWnd, SW_HIDE);
}
bool CExtended::isVisible()
{
	return(IsWindowVisible(m_hDlgWnd) != FALSE);
}

INT_PTR CALLBACK CExtended::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CExtended *pThis = (CExtended*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lParam);
		pThis = (CExtended*)lParam;
		pThis->m_hDlgWnd = hWnd;
	}
	if(pThis)
	{
		return(pThis->dlgProc(hWnd, msg, wParam, lParam));
	}
	return(FALSE);
}

INT_PTR CALLBACK CExtended::dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			m_hTabControl = GetDlgItem(hWnd, IDC_TAB1);
			
			//onSelChanged();

			break;
		}
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		commit();
		break;
	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->code)
		{
		case TCN_SELCHANGE:
			onSelChanged();
			break;

		// case LVN_ITEMCHANGED:
		// 	if(((LPNMHDR)lParam)->idFrom == IDC_LIST1)
		// 	{
		// 		onPropListChanged();
		// 	}
		// 	break;
		}
		break;

	case WM_COMMAND:
		switch(HIWORD(wParam))
		{
		case CBN_SELCHANGE:
		//	if(LOWORD(wParam) == IDC_OPE_COMBO)
		//	{
		//		HWND hCombo = (HWND)lParam;
		//		int iSel = ComboBox_GetCurSel(hCombo);
		//		if(iSel >= 0)
		//		{
		//			const char *szValue = (const char*)ComboBox_GetItemData(hCombo, iSel);
		//
		//			int iSelLB = ListView_GetNextItem(m_hPropListWnd, -1, LVNI_SELECTED);
		//			LVITEM lvItem;
		//			memset(&lvItem, 0, sizeof(lvItem));
		//			lvItem.iItem = iSelLB;
		//			lvItem.mask = LVIF_PARAM;
		//			ListView_GetItem(m_hPropListWnd, &lvItem);
		//			prop_s *pField = &m_aPropFields[AAString((char*)lvItem.lParam)];
		//
		//			setPropFieldValue(pField->field.szKey, szValue);
		//			if(m_pCallback)
		//			{
		//				m_pCallback->onPropertyChanged(pField->field.szKey, szValue);
		//			}
		//		}
		//	}
		//	else if(LOWORD(wParam) == IDC_CLASS_NAME)
		//	{
		//		if(m_pCallback)
		//		{
		//			HWND hCombo = (HWND)lParam;
		//			int iSel = ComboBox_GetCurSel(hCombo);
		//			if(iSel >= 0)
		//			{
		//				int iLen = ComboBox_GetTextLength(hCombo) + 1;
		//				char *szTemp = (char*)alloca(sizeof(char)* iLen);
		//				ComboBox_GetText(hCombo, szTemp, iLen);
		//				m_pCallback->onClassChanged(szTemp);
		//			}
		//		}
		//	}
			break;

		case EN_KILLFOCUS:
		//	if(LOWORD(wParam) == IDC_OPE_TEXT || LOWORD(wParam) == IDC_OPE_FILE)
		//	{
		//		HWND hEdit = (HWND)lParam;
		//
		//		if(GetParent(hEdit) == m_phEditors[m_editorActive])
		//		{
		//			int iValLen = Edit_GetTextLength(hEdit) + 1;
		//			char *szValue = (char*)alloca(sizeof(char) * iValLen);
		//			Edit_GetText(hEdit, szValue, iValLen);
		//
		//			int iSel = ListView_GetNextItem(m_hPropListWnd, -1, LVNI_SELECTED);
		//			LVITEM lvItem;
		//			memset(&lvItem, 0, sizeof(lvItem));
		//			lvItem.iItem = iSel;
		//			lvItem.mask = LVIF_PARAM;
		//			ListView_GetItem(m_hPropListWnd, &lvItem);
		//			prop_s *pField = &m_aPropFields[AAString((char*)lvItem.lParam)];
		//
		//			setPropFieldValue(pField->field.szKey, szValue);
		//			if(m_pCallback)
		//			{
		//				m_pCallback->onPropertyChanged(pField->field.szKey, szValue);
		//			}
		//		}
		//	}
			break;
		}

		
		if(LOWORD(wParam) == IDAPPLY)
		{
			commit();
			hide();
		}
		else if(LOWORD(wParam) == IDCANCEL)
		{
			rollback();
			hide();
		}
		break;

	default:
		return(FALSE);
	}
	return(TRUE);
}

void CExtended::onSelChanged()
{
	int iSel = TabCtrl_GetCurSel(m_hTabControl);

	if(m_pCurrentTab)
	{
		m_pCurrentTab->hide();
		m_pCurrentTab = NULL;
	}

	//if(iSel < m_hPropTabs.size())
	//{
	//	m_hCurrentTab = m_hPropTabs[iSel];
	//}
	//else
	//{
		int i = m_aCustomTabs.indexOf(iSel, [](CExtendedTabCallback *pTab, int iSel){
			return(pTab->getTabIndex() == iSel);
		});
		if(i >= 0)
		{
			m_pCurrentTab = m_aCustomTabs[i]->getTab();
		}
	//}

	if(m_pCurrentTab)
	{
		RECT rcDisplay = {0};
		GetWindowRect(m_hTabControl, &rcDisplay);
		MapWindowPoints(HWND_DESKTOP, m_hDlgWnd, (LPPOINT)&rcDisplay, 2);
		TabCtrl_AdjustRect(m_hTabControl, FALSE, &rcDisplay);

		const int iXDelta = -3;

		int iTabX = rcDisplay.left + iXDelta;
		int iTabY = rcDisplay.top;
		int iTabCX = rcDisplay.right - rcDisplay.left - iXDelta;
		int iTabCY = rcDisplay.bottom - rcDisplay.top;

		m_pCurrentTab->show(iTabX, iTabY, iTabCX, iTabCY);
	}
}

void CExtended::addCustomTab(IExtendedTab *pTab)
{
	CExtendedTabCallback *pCB = new CExtendedTabCallback(this, pTab);

	m_aCustomTabs.push_back(pCB);
	pTab->init(m_hDlgWnd);

	onAddTab(pCB);

	if(m_aCustomTabs.size() == 1)
	{
		TabCtrl_SetCurSel(m_hTabControl, 0);
		onSelChanged();
	}
}

void CExtended::onAddTab(CExtendedTabCallback *pTabCB)
{
	IExtendedTab *pTab = pTabCB->getTab();

	TCITEM ti;
	ti.mask = TCIF_TEXT;
	ti.pszText = strdupa(pTab->getTitle());
	int idx = TabCtrl_InsertItem(m_hTabControl, TabCtrl_GetItemCount(m_hTabControl), &ti);
	pTabCB->setTabIndex(idx);
}
void CExtended::onRemoveTab(CExtendedTabCallback *pTabCB)
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

UINT CExtended::getCustomTabCount()
{
	return(m_aCustomTabs.size());
}
IExtendedTab* CExtended::getCustomTab(UINT idx)
{
	return(m_aCustomTabs[idx]->getTab());
}

void CExtended::commit()
{
	for(UINT i = 0, l = m_aCustomTabs.size(); i < l; ++i)
	{
		m_aCustomTabs[i]->getTab()->commit();
	}
}

void CExtended::rollback()
{
	for(UINT i = 0, l = m_aCustomTabs.size(); i < l; ++i)
	{
		m_aCustomTabs[i]->getTab()->rollback();
	}
}
