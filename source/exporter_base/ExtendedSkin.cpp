#include "stdafx.h"
#include "ExtendedSkin.h"


INT_PTR CALLBACK CExtendedSkin::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CExtendedSkin *pThis = (CExtendedSkin*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lParam);
		pThis = (CExtendedSkin*)lParam;
		pThis->m_hDlg = hWnd;
	}
	if(pThis)
	{
		return(pThis->dlgProc(hWnd, msg, wParam, lParam));
	}
	return(FALSE);
}

INT_PTR CALLBACK CExtendedSkin::dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		m_hListWnd = GetDlgItem(m_hDlg, IDC_SKIN_LIST);

		ListView_SetExtendedListViewStyle(m_hListWnd,
			ListView_GetExtendedListViewStyle(m_hListWnd) | LVS_EX_FULLROWSELECT | LVS_EX_AUTOSIZECOLUMNS);

		LV_COLUMN column;
		column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
		column.fmt = LVCFMT_LEFT;
		column.pszText = (LPTSTR)"id";
		column.cx = 20;
		ListView_InsertColumn(m_hListWnd, 0, &column);

		loadDefaults();

		break;

	case WM_NOTIFY:
		{
			LPNMHDR lpnmhdr = ((LPNMHDR)lParam);
			if(lpnmhdr->hwndFrom == m_hListWnd)
			{
				switch(lpnmhdr->code)
				{
				case NM_DBLCLK:
					{
						LPNMITEMACTIVATE lp_na = ((LPNMITEMACTIVATE)lParam);
						HWND hListView = m_hListWnd;
						LVHITTESTINFO lvhti;
						lvhti.pt = lp_na->ptAction;

						ListView_SubItemHitTest(hListView, &lvhti);

						if(lvhti.iItem == -1 || lvhti.iSubItem < 1)
							break;

						TCHAR text[128];
						ListView_GetItemText(hListView, lvhti.iItem, lvhti.iSubItem, text, sizeof(text));
						LVCOLUMN col;
						col.mask = LVCF_TEXT;
						TCHAR szColumnName[128];
						col.pszText = szColumnName;
						col.cchTextMax = sizeof(szColumnName) - 1;
						ListView_GetColumn(hListView, lvhti.iSubItem, &col);

						if(invokeEditor(hWnd, text, col.pszText))
						{
							ListView_SetItemText(hListView, lvhti.iItem, lvhti.iSubItem, text);
						}
					}
					break;

				case LVN_ITEMCHANGED:
					{
						int iSel = ListView_GetSelectedCount(m_hListWnd);
						EnableWindow(GetDlgItem(m_hDlg, IDC_SKIN_REMOVE), iSel);
					}
					break;
				}
			}
		}

		break;

	case WM_COMMAND:
		//SendMessage(exp->hProgressBar, PBM_SETPOS, 429, 0);
		switch(LOWORD(wParam))
		{
#if 0
		case IDOK:
			// commit changes
			{
				HWND hCustom = GetDlgItem(hWnd, IDC_CUSTOM1);
				int iRows = ListView_GetItemCount(hCustom);
				int iCols = m_vszHeaders.size();
				m_vvszRows.resize(iRows);
				TCHAR tmp[128];
				for(int i = 0; i < iRows; ++i)
				{
					m_vvszRows[i].resize(iCols);

					for(int j = 0; j < iCols; ++j)
					{
						ListView_GetItemText(hCustom, i, j + 1, tmp, sizeof(tmp));
						m_vvszRows[i][j] = tmp;
					}
				}
			}
			EndDialog(hWnd, 1);
			break;
#endif

		case IDC_SKIN_ADD:
			{
				TCHAR str[4];
				LV_ITEM item;
				item.mask = LVIF_TEXT;
				item.iItem = ListView_GetItemCount(m_hListWnd);
				item.iSubItem = 0;
				item.pszText = str;
				wsprintf(str, "%d", item.iItem + 1);
				item.cchTextMax = static_cast<int>(strlen(item.pszText));
				ListView_InsertItem(m_hListWnd, &item);

			}
			break;
		case IDC_SKIN_REMOVE:
			{
				int idx = getSelectedIndex();
				if(idx >= 0)
				{
					ListView_DeleteItem(m_hListWnd, idx);
					reindexRows();
				}
			}
			break;
		case IDC_SKIN_UP: // up
			{
				int idx = getSelectedIndex();
				if(idx < 0)
				{
					break;
				}

				if(idx > 0)
				{
					swapRows(idx, idx - 1, m_vszHeaders.size());
					reindexRows();
					ListView_SetItemState(m_hListWnd, idx, 0, LVIS_SELECTED);
					ListView_SetItemState(m_hListWnd, idx - 1, LVIS_SELECTED, LVIS_SELECTED);
				}
			}
			break;

		case IDC_SKIN_DOWN: // down
			{
				int idx = getSelectedIndex();
				if(idx < 0)
				{
					break;
				}

				if(idx < ListView_GetItemCount(m_hListWnd) - 1)
				{
					swapRows(idx, idx + 1, m_vszHeaders.size());
					reindexRows();
					ListView_SetItemState(m_hListWnd, idx, 0, LVIS_SELECTED);
					ListView_SetItemState(m_hListWnd, idx + 1, LVIS_SELECTED, LVIS_SELECTED);
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

void CExtendedSkin::init(HWND hWndParent)
{
	assert(!m_hDlg);

	m_hDlg = CreateDialogParamA(g_hInstance, MAKEINTRESOURCE(IDD_OP_SKIN), hWndParent, DlgProc, (LPARAM)this);
}

void CExtendedSkin::show(int X, int Y, int cx, int cy)
{
	ShowWindow(m_hDlg, SW_SHOWNORMAL);
	SetWindowPos(m_hDlg, HWND_TOP, X, Y, cx, cy, 0);
}
void CExtendedSkin::hide()
{
	ShowWindow(m_hDlg, SW_HIDE);
}

void CExtendedSkin::loadDefaults()
{
	if(!m_isHeaderLoaded)
	{
		LV_COLUMN column;
		column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
		column.fmt = LVCFMT_LEFT;
		for(int i = 0, l = m_vszHeaders.size(); i < l; ++i)
		{
			column.pszText = (LPTSTR)m_vszHeaders[i].c_str();
			column.cx = 80;
			ListView_InsertColumn(m_hListWnd, i + 1, &column);
		}
		m_isHeaderLoaded = true;
	}

	ListView_DeleteAllItems(m_hListWnd);

	for(int i = 0, l = m_vvszRows.size(); i < l; ++i)
	{
		TCHAR str[4];
		LV_ITEM item;
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 0;
		item.pszText = str;
		wsprintf(str, "%d", item.iItem + 1);
		item.cchTextMax = static_cast<int>(strlen(item.pszText));
		ListView_InsertItem(m_hListWnd, &item);

		for(int j = 0, jl = m_vvszRows[i].size(); j < jl; ++j)
		{
			item.iSubItem = j + 1;
			item.pszText = (LPTSTR)m_vvszRows[i][j].c_str();
			item.cchTextMax = static_cast<int>(strlen(item.pszText));
			ListView_SetItem(m_hListWnd, &item);
		}
	}
}

void CExtendedSkin::addColumn(LPCTSTR str)
{
	m_vszHeaders.push_back(str);
}

void CExtendedSkin::addOverride(int iSkin, int iColumn, LPCTSTR str)
{
	if((int)m_vvszRows.size() <= iSkin)
	{
		m_vvszRows.resize(iSkin + 1);
	}
	if((int)m_vvszRows[iSkin].size() <= iColumn)
	{
		m_vvszRows[iSkin].resize(iColumn + 1);
	}
	m_vvszRows[iSkin][iColumn] = str;
}

void CExtendedSkin::reindexRows()
{
	TCHAR str[4];
	for(int i = 0, l = ListView_GetItemCount(m_hListWnd); i < l; ++i)
	{
		wsprintf(str, "%d", i + 1);
		ListView_SetItemText(m_hListWnd, i, 0, str);
	}
}

int CExtendedSkin::getSelectedIndex()
{
	for(int i = 0, l = ListView_GetItemCount(m_hListWnd); i < l; ++i)
	{
		int res2 = ListView_GetItemState(m_hListWnd, i, LVIS_SELECTED);
		if(res2 == LVIS_SELECTED)
		{
			return(i);
		}
	}
	return(-1);
}

void CExtendedSkin::swapRows(int i1, int i2, int iColNumber)
{
	TCHAR tmp[128], tmp2[128];

	for(int i = 0; i < iColNumber; ++i)
	{
		ListView_GetItemText(m_hListWnd, i1, i + 1, tmp, sizeof(tmp));
		ListView_GetItemText(m_hListWnd, i2, i + 1, tmp2, sizeof(tmp2));
		ListView_SetItemText(m_hListWnd, i1, i + 1, tmp2);
		ListView_SetItemText(m_hListWnd, i2, i + 1, tmp);
	}
}

int CExtendedSkin::getColumnCount()
{
	return(m_vszHeaders.size());
}

LPCTSTR CExtendedSkin::getColumn(int i)
{
	return(m_vszHeaders[i].c_str());
}

LPCTSTR CExtendedSkin::getCell(int row, int col)
{
	return(m_vvszRows[row][col].c_str());
}

int CExtendedSkin::getRowCount()
{
	return(m_vvszRows.size());
}

INT_PTR CALLBACK CExtendedSkin::SkinsEditDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//CSkinEditor *exp = DLGetWindowLongPtr<CSkinEditor*>(hWnd);
	CExtendedSkin *exp = (CExtendedSkin*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	switch(msg)
	{
	case WM_INITDIALOG:
		exp = (CExtendedSkin*)lParam;
		//CenterWindow(hWnd, GetParent(hWnd));
		//DLSetWindowLongPtr(hWnd, lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
		//IDC_CUSTOM1
		//SetWindowText(GetDlgItem(hWnd, IDC_EDIT1), exp->m_szEditDefault);
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

bool CExtendedSkin::invokeEditor(HWND hWnd, LPCTSTR szDefault, LPCTSTR szTitle)
{
	m_szEditDefault = szDefault;
	m_szEditTitle = szTitle;
	return(DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG3), hWnd, SkinsEditDlgProc, (LPARAM)this) != 0);
}

void CExtendedSkin::commit()
{
	int iRows = ListView_GetItemCount(m_hListWnd);
	int iCols = m_vszHeaders.size();
	m_vvszRows.resize(iRows);
	TCHAR tmp[128];
	for(int i = 0; i < iRows; ++i)
	{
		m_vvszRows[i].resize(iCols);

		for(int j = 0; j < iCols; ++j)
		{
			ListView_GetItemText(m_hListWnd, i, j + 1, tmp, sizeof(tmp));
			m_vvszRows[i][j] = tmp;
		}
	}
}
void CExtendedSkin::rollback()
{
	loadDefaults();
}
