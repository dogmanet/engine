#include "stdafx.h"
#include "ExtendedActs.h"


INT_PTR CALLBACK CExtendedActs::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CExtendedActs *pThis = (CExtendedActs*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lParam);
		pThis = (CExtendedActs*)lParam;
		pThis->m_hDlg = hWnd;
	}
	if(pThis)
	{
		return(pThis->dlgProc(hWnd, msg, wParam, lParam));
	}
	return(FALSE);
}

INT_PTR CALLBACK CExtendedActs::dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			m_hListWnd = GetDlgItem(m_hDlg, IDC_ACT_LIST);

			ListView_SetExtendedListViewStyle(m_hListWnd,
				ListView_GetExtendedListViewStyle(m_hListWnd) | LVS_EX_FULLROWSELECT | LVS_EX_AUTOSIZECOLUMNS);

			LV_COLUMNA lvColumn;
			memset(&lvColumn, 0, sizeof(lvColumn));

			RECT rc;
			GetClientRect(m_hListWnd, &rc);

			int iWidth = rc.right - rc.left;

			lvColumn.mask = LVCF_WIDTH | LVCF_TEXT;
			lvColumn.cx = iWidth / 1;
			lvColumn.pszText = "Name";
			lvColumn.cchTextMax = (int)strlen(lvColumn.pszText);
			ListView_InsertColumn(m_hListWnd, 0, &lvColumn);

			//lvColumn.pszText = "Looped";
			//lvColumn.cchTextMax = strlen(lvColumn.pszText);
			//ListView_InsertColumn(m_hListWnd, 1, &lvColumn);

			loadDefaults();
		}
		break;

	case WM_NOTIFY:
		{
			LPNMHDR lpnmhdr = ((LPNMHDR)lParam);
			if(lpnmhdr->hwndFrom == m_hListWnd)
			{
				switch(lpnmhdr->code)
				{
				case LVN_ITEMCHANGED:
					{
						int iSel = ListView_GetSelectedCount(m_hListWnd);
						EnableWindow(GetDlgItem(m_hDlg, IDC_ACT_REMOVE), iSel);
					}
					break;
				}
			}
		}

		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_ACT_ADD:
			promptForNew();
			break;

		case IDC_ACT_REMOVE:
			{
				int idx = -1;
				for(int i = 0, l = ListView_GetItemCount(m_hListWnd); i < l; ++i)
				{
					int res2 = ListView_GetItemState(m_hListWnd, i, LVIS_SELECTED);
					if(res2 == LVIS_SELECTED)
					{
						idx = i;
						break;
					}
				}
				if(idx >= 0)
				{
					ListView_DeleteItem(m_hListWnd, idx);
					invokeListener(false);
				}
			}
			break;
		}
		break;
	default:
		return(FALSE);
	}
	return(TRUE);
}

bool CExtendedActs::invokeEditor(HWND hWnd, LPCTSTR szDefault, LPCTSTR szTitle)
{
	m_szEditDefault = szDefault;
	m_szEditTitle = szTitle;
	return(DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG3), hWnd, EditDlgProc, (LPARAM)this) != 0);
}

void CExtendedActs::init(HWND hWndParent)
{
	assert(!m_hDlg);

	m_hDlg = CreateDialogParamA(g_hInstance, MAKEINTRESOURCE(IDD_OP_ACTIVITIES), hWndParent, DlgProc, (LPARAM)this);
}

void CExtendedActs::show(int X, int Y, int cx, int cy)
{
	ShowWindow(m_hDlg, SW_SHOWNORMAL);
	SetWindowPos(m_hDlg, HWND_TOP, X, Y, cx, cy, 0);
}
void CExtendedActs::hide()
{
	ShowWindow(m_hDlg, SW_HIDE);
}

INT_PTR CALLBACK CExtendedActs::EditDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//CSkinEditor *exp = DLGetWindowLongPtr<CSkinEditor*>(hWnd);
	CExtendedActs *exp = (CExtendedActs*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	switch(msg)
	{
	case WM_INITDIALOG:
		exp = (CExtendedActs*)lParam;
		//CenterWindow(hWnd, GetParent(hWnd));
		//DLSetWindowLongPtr(hWnd, lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
		//IDC_CUSTOM1
		//SetWindowText(GetDlgItem(hWnd, IDC_EDIT1), exp->m_szEditDefault);
		SetWindowText(GetDlgItem(hWnd, IDC_PROMPT2), "");
		SetWindowText(GetDlgItem(hWnd, IDC_EDIT_PROMPT), exp->m_szEditTitle);
		{
			HWND ed1 = GetDlgItem(hWnd, IDC_CUSTOM1);
			if(ed1)
			{
				SetWindowText(ed1, exp->m_szEditDefault);
				//ed1->SetText(exp->m_szEditDefault);
				//ed1->GiveFocus();
				//ReleaseICustEdit(ed1);
				SetFocus(ed1);
			}
		}
		//SetFocus(GetDlgItem(hWnd, IDC_CUSTOM1));
		SendMessage(GetDlgItem(hWnd, IDC_CUSTOM1), EM_SETSEL, 0, -1);
		return(FALSE);

	case WM_COMMAND:
		//SendMessage(exp->hProgressBar, PBM_SETPOS, 429, 0);
		switch(LOWORD(wParam))
		{
		case IDOK:
			{
				// commit changes
				HWND ed1 = GetDlgItem(hWnd, IDC_CUSTOM1);
				if(ed1)
				{
					GetWindowText(ed1, (LPTSTR)exp->m_szEditDefault, 127);
					//ed1->GetText((LPTSTR)exp->m_szEditDefault, 127);
					//ReleaseICustEdit(ed1);
				}
				//GetWindowText(GetDlgItem(hWnd, IDC_EDIT1), (LPWSTR)exp->m_szEditDefault, 127);
				EndDialog(hWnd, 1);
			}
			break;
		case IDCANCEL:
			EndDialog(hWnd, 0);
			break;
		}
		break;
	default:
		return(FALSE);
	}
	return(TRUE);
}

void CExtendedActs::loadDefaults()
{
	ListView_DeleteAllItems(m_hListWnd);

	for(int i = 0, l = m_aActivities.size(); i < l; ++i)
	{
		LV_ITEM item;
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 0;
		item.pszText = (LPTSTR)m_aActivities[i].c_str();
		item.cchTextMax = static_cast<int>(strlen(item.pszText));
		ListView_InsertItem(m_hListWnd, &item);
	}

	invokeListener(true);
}

void CExtendedActs::commit()
{
	m_aActivities = getUncommittedList();

	invokeListener(true);
}

const Array<String> CExtendedActs::getUncommittedList()
{
	int iRows = ListView_GetItemCount(m_hListWnd);
	Array<String> aActivities(iRows);
	TCHAR tmp[128];
	for(int i = 0; i < iRows; ++i)
	{
		ListView_GetItemText(m_hListWnd, i, 0, tmp, sizeof(tmp));
		aActivities[i] = tmp;
	}

	return(aActivities);
}

void CExtendedActs::rollback()
{
	loadDefaults();
}

void CExtendedActs::addActivity(const char *szName)
{
	m_aActivities.push_back(szName);

	invokeListener(true);
}

void CExtendedActs::addListener(IListener *pListener)
{
	int idx = m_apListeners.indexOf(pListener);
	if(idx < 0)
	{
		m_apListeners.push_back(pListener);
	}
}
void CExtendedActs::removeListener(IListener *pListener)
{
	int idx = m_apListeners.indexOf(pListener);
	if(idx >= 0)
	{
		m_apListeners.erase(idx);
	}
}

void CExtendedActs::invokeListener(bool isCommit)
{
	for(UINT i = 0, l = m_apListeners.size(); i < l; ++i)
	{
		if(isCommit)
		{
			m_apListeners[i]->onCommitted();
		}
		else
		{
			m_apListeners[i]->onChanged();
		}
	}
}

void CExtendedActs::promptForNew(bool isCommit)
{
	char tmp[127];
	tmp[0] = 0;
	if(invokeEditor(m_hDlg, tmp, "Activity name"))
	{
		LV_ITEM item;
		item.mask = LVIF_TEXT;
		item.iItem = ListView_GetItemCount(m_hListWnd);
		item.iSubItem = 0;
		item.pszText = tmp;
		item.cchTextMax = static_cast<int>(strlen(item.pszText));
		ListView_InsertItem(m_hListWnd, &item);

		invokeListener(isCommit);
	}
}
