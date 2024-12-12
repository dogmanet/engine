#include "stdafx.h"
#include "ExtendedLods.h"


INT_PTR CALLBACK CExtendedLods::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CExtendedLods *pThis = (CExtendedLods*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lParam);
		pThis = (CExtendedLods*)lParam;
		pThis->m_hDlg = hWnd;
	}
	if(pThis)
	{
		return(pThis->dlgProc(hWnd, msg, wParam, lParam));
	}
	return(FALSE);
}

INT_PTR CALLBACK CExtendedLods::dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			m_hListWnd = GetDlgItem(m_hDlg, IDC_LOD_LIST);

			//CheckDlgButton(m_hDlgWnd, IDC_GIB_OBJECT, TRUE);
			//SetDlgItemText(m_hDlg, IDC_PHYS_WEIGHT, "1.0");
			ListView_SetExtendedListViewStyle(m_hListWnd, ListView_GetExtendedListViewStyle(m_hListWnd) | LVS_EX_FULLROWSELECT);

			LV_COLUMNA lvColumn;
			memset(&lvColumn, 0, sizeof(lvColumn));

			RECT rc;
			GetClientRect(m_hListWnd, &rc);

			int iWidth = rc.right - rc.left;

			lvColumn.mask = LVCF_WIDTH | LVCF_TEXT;
			lvColumn.cx = iWidth / 3;
			lvColumn.pszText = "#";
			lvColumn.cchTextMax = (int)strlen(lvColumn.pszText);
			ListView_InsertColumn(m_hListWnd, 0, &lvColumn);

			lvColumn.pszText = "Polys";
			lvColumn.cchTextMax = (int)strlen(lvColumn.pszText);
			ListView_InsertColumn(m_hListWnd, 1, &lvColumn);

			lvColumn.pszText = "Subsets";
			lvColumn.cchTextMax = (int)strlen(lvColumn.pszText);
			ListView_InsertColumn(m_hListWnd, 2, &lvColumn);

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
						EnableWindow(GetDlgItem(m_hDlg, IDC_LOD_REMOVE), iSel);
					}
					break;
				}
			}
		}

		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_LOD_REMOVE:
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
				}
			}
			break;

		case IDC_LOD_UP: // up
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
				if(idx > 0)
				{
					swapRows(idx, idx - 1, 3);
					ListView_SetItemState(m_hListWnd, idx, 0, LVIS_SELECTED);
					ListView_SetItemState(m_hListWnd, idx - 1, LVIS_SELECTED, LVIS_SELECTED);
				}
			}
			break;

		case IDC_SKIN_DOWN: // down
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
				if(idx < 0)
				{
					break;
				}

				if(idx < ListView_GetItemCount(m_hListWnd) - 1)
				{
					swapRows(idx, idx + 1, 3);
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

void CExtendedLods::init(HWND hWndParent)
{
	assert(!m_hDlg);

	m_hDlg = CreateDialogParamA(g_hInstance, MAKEINTRESOURCE(IDD_OP_LODS), hWndParent, DlgProc, (LPARAM)this);
}

void CExtendedLods::show(int X, int Y, int cx, int cy)
{
	ShowWindow(m_hDlg, SW_SHOWNORMAL);
	SetWindowPos(m_hDlg, HWND_TOP, X, Y, cx, cy, 0);
}
void CExtendedLods::hide()
{
	ShowWindow(m_hDlg, SW_HIDE);
}

void CExtendedLods::loadDefaults()
{
	ListView_DeleteAllItems(m_hListWnd);
	
	char tmp[32];

	for(int i = 0, l = m_aList.size(); i < l; ++i)
	{
		Lod &lod = m_aList[i];
		LV_ITEM item;
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 0;
		item.pszText = tmp;
		sprintf(tmp, "%d", i);
		item.cchTextMax = static_cast<int>(strlen(item.pszText));
		ListView_InsertItem(m_hListWnd, &item);

		sprintf(tmp, "%u", lod.uTris);
		item.iSubItem = 1;
		item.cchTextMax = static_cast<int>(strlen(item.pszText));
		ListView_SetItem(m_hListWnd, &item);

		sprintf(tmp, "%u", lod.uSubsets);
		item.iSubItem = 2;
		item.cchTextMax = static_cast<int>(strlen(item.pszText));
		ListView_SetItem(m_hListWnd, &item);
	}
}

void CExtendedLods::commit()
{
	Array<Lod> aNewLods;

	char tmp[127];
	for(int i = 0, l = ListView_GetItemCount(m_hListWnd); i < l; ++i)
	{
		ListView_GetItemText(m_hListWnd, i, 0, tmp, sizeof(tmp));
		int idx = -1;
		if(sscanf(tmp, "%d", &idx))
		{
			aNewLods.push_back(m_aList[idx]);
		}
	}

	m_aList = aNewLods;

	invokeListener();

	for(UINT i = 0, l = m_aList.size(); i < l; ++i)
	{
		m_aList[i].iRow = i;
	}
}
void CExtendedLods::rollback()
{
	loadDefaults();
}

void CExtendedLods::swapRows(int i1, int i2, int iColNumber)
{
	TCHAR tmp[128], tmp2[128];

	for(int i = 0; i < iColNumber; ++i)
	{
		ListView_GetItemText(m_hListWnd, i1, i, tmp, sizeof(tmp));
		ListView_GetItemText(m_hListWnd, i2, i, tmp2, sizeof(tmp2));
		ListView_SetItemText(m_hListWnd, i1, i, tmp2);
		ListView_SetItemText(m_hListWnd, i2, i, tmp);
	}
}

void CExtendedLods::addLod(UINT uTris, UINT uSubsets)
{
	m_aList.push_back({m_aList.size(), uTris, uSubsets});
}

void CExtendedLods::addListener(IListener *pListener)
{
	int idx = m_apListeners.indexOf(pListener);
	if(idx < 0)
	{
		m_apListeners.push_back(pListener);
	}
}
void CExtendedLods::removeListener(IListener *pListener)
{
	int idx = m_apListeners.indexOf(pListener);
	if(idx >= 0)
	{
		m_apListeners.erase(idx);
	}
}

void CExtendedLods::invokeListener()
{
	for(UINT i = 0, l = m_apListeners.size(); i < l; ++i)
	{
		m_apListeners[i]->onCommitted();
	}
}
