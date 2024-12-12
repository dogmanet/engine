#include "stdafx.h"
#include "ExtendedPhys.h"


INT_PTR CALLBACK CExtendedPhys::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CExtendedPhys *pThis = (CExtendedPhys*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lParam);
		pThis = (CExtendedPhys*)lParam;
		pThis->m_hDlg = hWnd;
	}
	if(pThis)
	{
		return(pThis->dlgProc(hWnd, msg, wParam, lParam));
	}
	return(FALSE);
}

INT_PTR CALLBACK CExtendedPhys::dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		loadDefaults();

		break;

	default:
		return(FALSE);
	}
	return(TRUE);
}

void CExtendedPhys::init(HWND hWndParent)
{
	assert(!m_hDlg);

	m_hDlg = CreateDialogParamA(g_hInstance, MAKEINTRESOURCE(IDD_OP_PHYS), hWndParent, DlgProc, (LPARAM)this);
}

void CExtendedPhys::show(int X, int Y, int cx, int cy)
{
	ShowWindow(m_hDlg, SW_SHOWNORMAL);
	SetWindowPos(m_hDlg, HWND_TOP, X, Y, cx, cy, 0);
}
void CExtendedPhys::hide()
{
	ShowWindow(m_hDlg, SW_HIDE);
}

void CExtendedPhys::loadDefaults()
{
	//CheckDlgButton(m_hDlgWnd, IDC_GIB_OBJECT, TRUE);
	//SetDlgItemText(m_hDlg, IDC_PHYS_WEIGHT, "1.0");
	rollback();
}

void CExtendedPhys::setWeight(float fWeight)
{
	m_fWeight = fWeight;

	rollback();
}

float CExtendedPhys::getWeight()
{
	return(m_fWeight);
}

void CExtendedPhys::commit()
{
	char tmp[32];
	GetDlgItemText(m_hDlg, IDC_PHYS_WEIGHT, tmp, sizeof(tmp));
	sscanf(tmp, "%f", &m_fWeight);
}
void CExtendedPhys::rollback()
{
	char tmp[32];
	sprintf(tmp, "%f", m_fWeight);

	SetDlgItemText(m_hDlg, IDC_PHYS_WEIGHT, tmp);
}
