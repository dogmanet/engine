#include "stdafx.h"
#include "ExtendedDeps.h"


INT_PTR CALLBACK CExtendedDeps::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CExtendedDeps *pThis = (CExtendedDeps*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lParam);
		pThis = (CExtendedDeps*)lParam;
		pThis->m_hDlg = hWnd;
	}
	if(pThis)
	{
		return(pThis->dlgProc(hWnd, msg, wParam, lParam));
	}
	return(FALSE);
}

INT_PTR CALLBACK CExtendedDeps::dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			m_hListWnd = GetDlgItem(m_hDlg, IDC_IMP_LIST);

			//CheckDlgButton(m_hDlgWnd, IDC_GIB_OBJECT, TRUE);
			//SetDlgItemText(m_hDlg, IDC_PHYS_WEIGHT, "1.0");
			ListView_SetExtendedListViewStyle(m_hListWnd, ListView_GetExtendedListViewStyle(m_hListWnd) | LVS_EX_FULLROWSELECT);

			LV_COLUMNA lvColumn;
			memset(&lvColumn, 0, sizeof(lvColumn));

			RECT rc;
			GetClientRect(m_hListWnd, &rc);

			int iWidth = rc.right - rc.left;

			lvColumn.mask = LVCF_WIDTH | LVCF_TEXT;
			lvColumn.cx = iWidth / 7;
			lvColumn.pszText = "#";
			lvColumn.cchTextMax = strlen(lvColumn.pszText);
			ListView_InsertColumn(m_hListWnd, 0, &lvColumn);

			lvColumn.pszText = "Name";
			lvColumn.cchTextMax = strlen(lvColumn.pszText);
			ListView_InsertColumn(m_hListWnd, 1, &lvColumn);

			lvColumn.pszText = "Frames";
			lvColumn.cchTextMax = strlen(lvColumn.pszText);
			ListView_InsertColumn(m_hListWnd, 2, &lvColumn);

			lvColumn.pszText = "Looped";
			lvColumn.cchTextMax = strlen(lvColumn.pszText);
			ListView_InsertColumn(m_hListWnd, 3, &lvColumn);

			lvColumn.pszText = "Speed";
			lvColumn.cchTextMax = strlen(lvColumn.pszText);
			ListView_InsertColumn(m_hListWnd, 4, &lvColumn);

			lvColumn.pszText = "Activity";
			lvColumn.cchTextMax = strlen(lvColumn.pszText);
			ListView_InsertColumn(m_hListWnd, 5, &lvColumn);

			lvColumn.pszText = "Chance";
			lvColumn.cchTextMax = strlen(lvColumn.pszText);
			ListView_InsertColumn(m_hListWnd, 6, &lvColumn);

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
				case NM_DBLCLK:
					{
						LPNMITEMACTIVATE lp_na = ((LPNMITEMACTIVATE)lParam);
						HWND hListView = m_hListWnd;
						LVHITTESTINFO lvhti;
						lvhti.pt = lp_na->ptAction;

						ListView_SubItemHitTest(hListView, &lvhti);

						if(lvhti.iItem == -1/* || lvhti.iSubItem < 1*/)
							break;

						invokeEditor(lvhti.iItem);
					}
					break;

				case LVN_ITEMCHANGED:
					{
						int iSel = ListView_GetSelectedCount(m_hListWnd);
						EnableWindow(GetDlgItem(m_hDlg, IDC_ANIM_REMOVE), iSel);
					}
					break;
				}
			}
		}

		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_IMP_REMOVE:
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
		}
		break;
	default:
		return(FALSE);
	}
	return(TRUE);
}

void CExtendedDeps::init(HWND hWndParent)
{
	assert(!m_hDlg);

	m_hDlg = CreateDialogParamA(g_hInstance, MAKEINTRESOURCE(IDD_OP_IMPORTS), hWndParent, DlgProc, (LPARAM)this);
}

void CExtendedDeps::show(int X, int Y, int cx, int cy)
{
	ShowWindow(m_hDlg, SW_SHOWNORMAL);
	SetWindowPos(m_hDlg, HWND_TOP, X, Y, cx, cy, 0);
}
void CExtendedDeps::hide()
{
	ShowWindow(m_hDlg, SW_HIDE);
}

void CExtendedDeps::loadDefaults()
{
	ListView_DeleteAllItems(m_hListWnd);
	
	char tmp[32];

	for(int i = 0, l = m_aParts.size(); i < l; ++i)
	{
		ModelSequence &ms = m_aSequences[i].ms;
		LV_ITEM item;
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 0;
		item.pszText = tmp;
		sprintf(tmp, "%d", i + 1);
		item.cchTextMax = static_cast<int>(strlen(item.pszText));
		ListView_InsertItem(m_hListWnd, &item);

		item.pszText = ms.name;
		item.iSubItem = 1;
		item.cchTextMax = static_cast<int>(strlen(item.pszText));
		ListView_SetItem(m_hListWnd, &item);

		item.pszText = tmp; 
		item.iSubItem = 2;
		sprintf(tmp, "%u", ms.iNumFrames);
		item.cchTextMax = static_cast<int>(strlen(item.pszText));
		ListView_SetItem(m_hListWnd, &item);

		item.iSubItem = 3;
		sprintf(tmp, ms.bLooped ? "Yes" : "No");
		item.cchTextMax = static_cast<int>(strlen(item.pszText));
		ListView_SetItem(m_hListWnd, &item);

		item.iSubItem = 4;
		sprintf(tmp, "%d", ms.framerate);
		item.cchTextMax = static_cast<int>(strlen(item.pszText));
		ListView_SetItem(m_hListWnd, &item);

		item.iSubItem = 5;
		item.pszText = (LPSTR)m_aSequences[i].sActivity.c_str();
		item.cchTextMax = static_cast<int>(strlen(item.pszText));
		ListView_SetItem(m_hListWnd, &item);

		item.iSubItem = 6; 
		item.pszText = tmp;
		sprintf(tmp, "%u", ms.act_chance);
		item.cchTextMax = static_cast<int>(strlen(item.pszText));
		ListView_SetItem(m_hListWnd, &item);
	}
}

void CExtendedDeps::commit()
{
	// publish changes to m_aSequences

	for(UINT i = 0, l = m_aSequences.size(); i < l; ++i)
	{
		m_aSequences[i].toDelete = true;
	}

	char tmp[127];
	for(int i = 0, l = ListView_GetItemCount(m_hListWnd); i < l; ++i)
	{
		ListView_GetItemText(m_hListWnd, i, 0, tmp, sizeof(tmp));
		int idx = -1;
		if(sscanf(tmp, "%d", &idx))
		{
			--idx;
			m_aSequences[idx].toDelete = false;
			ModelSequence &ms = m_aSequences[idx].ms;

			ListView_GetItemText(m_hListWnd, i, 5, tmp, sizeof(tmp));
			m_aSequences[idx].sActivity = tmp;

			ListView_GetItemText(m_hListWnd, i, 1, ms.name, sizeof(ms.name));
			ms.name[MODEL_MAX_NAME - 1] = 0;

			ListView_GetItemText(m_hListWnd, i, 3, tmp, sizeof(tmp));
			ms.bLooped = !strcmp(tmp, "Yes");

			ListView_GetItemText(m_hListWnd, i, 4, tmp, sizeof(tmp));
			sscanf(tmp, "%d", &ms.framerate);

			ListView_GetItemText(m_hListWnd, i, 6, tmp, sizeof(tmp));
			sscanf(tmp, "%u", &ms.act_chance);
		}
	}

	for(int i = m_aSequences.size() - 1; i > 0; --i)
	{
		if(m_aSequences[i].toDelete)
		{
			for(UINT j = 0; j < m_aSequences[i].ms.iNumFrames; ++j)
			{
				mem_delete_a(m_aSequences[i].ms.m_vmAnimData[j]);
			}
			mem_delete_a(m_aSequences[i].ms.m_vmAnimData);
			
			m_aSequences.erase(i);
		}
	}
}
void CExtendedDeps::rollback()
{
	loadDefaults();
}

void CExtendedDeps::setActivityList(const Array<String> &list)
{
	m_asActivities = list;
}

void CExtendedDeps::addImport(ModelPart &pt)
{
	String sAct = "";
	if(ms.activity && ms.activity - 1 < m_asActivities.size())
	{
		sAct = m_asActivities[ms.activity - 1];
	}
	m_aSequences.push_back({ms, sAct});
}

const ModelSequence& CExtendedDeps::getSequence(UINT idx)
{
	ModelSequence &ms = m_aSequences[idx].ms;
	String &s = m_aSequences[idx].sActivity;
	ms.activity = 0;
	if(s.length())
	{
		int idx = m_asActivities.indexOf(s);
		if(idx >= 0)
		{
			ms.activity = idx + 1;
		}
	}

	return(ms);
}

bool CExtendedDeps::invokeEditor(int iItem)
{
	m_iEditRow = iItem;
	return(DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG5), m_hDlg, EditDlgProc, (LPARAM)this));
}

INT_PTR CALLBACK CExtendedDeps::EditDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//CSkinEditor *exp = DLGetWindowLongPtr<CSkinEditor*>(hWnd);
	CExtendedAnim *exp = (CExtendedAnim*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			exp = (CExtendedAnim*)lParam;
			//CenterWindow(hWnd, GetParent(hWnd));
			//DLSetWindowLongPtr(hWnd, lParam);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);

			HWND hCombo = GetDlgItem(hWnd, IDC_ANIMATION_ACTIVITY);

			ComboBox_AddString(hCombo, "");
			auto &list = exp->m_asActivities;
			for(UINT i = 0, l = list.size(); i < l; ++i)
			{
				ComboBox_AddString(hCombo, list[i].c_str());
			}

			TCHAR text[128];
			ListView_GetItemText(exp->m_hListWnd, exp->m_iEditRow, 1, text, sizeof(text));
			SetWindowText(GetDlgItem(hWnd, IDC_ANIMATION_NAME), text);
			ListView_GetItemText(exp->m_hListWnd, exp->m_iEditRow, 3, text, sizeof(text));
			CheckDlgButton(hWnd, IDC_ANIMATION_LOOP, !strcmp(text, "Yes"));
			ListView_GetItemText(exp->m_hListWnd, exp->m_iEditRow, 4, text, sizeof(text));
			SetWindowText(GetDlgItem(hWnd, IDC_ANIMATION_SPEED), text);
			ListView_GetItemText(exp->m_hListWnd, exp->m_iEditRow, 5, text, sizeof(text));
			int iSel = ComboBox_FindStringExact(hCombo, -1, text);
			ComboBox_SetCurSel(hCombo, iSel);
			//SetWindowText(GetDlgItem(hWnd, IDC_ANIMATION_ACTIVITY), text);
			ListView_GetItemText(exp->m_hListWnd, exp->m_iEditRow, 6, text, sizeof(text));
			SetWindowText(GetDlgItem(hWnd, IDC_ANIMATION_ACTIVITY_CHANCE), text);
		}
		return(FALSE);

	case WM_COMMAND:
		//SendMessage(exp->hProgressBar, PBM_SETPOS, 429, 0);
		switch(LOWORD(wParam))
		{
		case IDOK:
			{
				// commit changes

				HWND hList = exp->m_hListWnd;
				char name[127];

				GetDlgItemText(hWnd, IDC_ANIMATION_NAME, name, sizeof(name));

				char tmp[127];
				bool isValid = true;
				for(int i = 0, l = ListView_GetItemCount(hList); i < l; ++i)
				{
					if(i != exp->m_iEditRow)
					{
						ListView_GetItemText(hList, i, 1, tmp, sizeof(tmp));
						if(!strcasecmp(tmp, name))
						{
							isValid = false;
							MessageBox(hWnd, "Animation with that name already exists!", "Error", MB_OK | MB_ICONSTOP);
							break;
						}
					}
				}

				GetDlgItemText(hWnd, IDC_ANIMATION_SPEED, tmp, sizeof(tmp));
				int itmp;
				if(!sscanf(tmp, "%d", &itmp))
				{
					MessageBox(hWnd, "Invalid animation speed!", "Error", MB_OK | MB_ICONSTOP);
					isValid = false;
				}
				GetDlgItemText(hWnd, IDC_ANIMATION_ACTIVITY_CHANCE, tmp, sizeof(tmp));
				if(!sscanf(tmp, "%d", &itmp) || itmp <= 0)
				{
					MessageBox(hWnd, "Invalid chance!", "Error", MB_OK | MB_ICONSTOP);
					isValid = false;
				}
				if(!isValid)
				{
					break;
				}

				ListView_SetItemText(hList, exp->m_iEditRow, 1, name);
				ListView_SetItemText(hList, exp->m_iEditRow, 3, IsDlgButtonChecked(hWnd, IDC_ANIMATION_LOOP) ? "Yes" : "No");

				GetDlgItemText(hWnd, IDC_ANIMATION_SPEED, tmp, sizeof(tmp));
				ListView_SetItemText(hList, exp->m_iEditRow, 4, tmp);

				GetDlgItemText(hWnd, IDC_ANIMATION_ACTIVITY, tmp, sizeof(tmp));
				ListView_SetItemText(hList, exp->m_iEditRow, 5, tmp);

				GetDlgItemText(hWnd, IDC_ANIMATION_ACTIVITY_CHANCE, tmp, sizeof(tmp));
				ListView_SetItemText(hList, exp->m_iEditRow, 6, tmp);

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
