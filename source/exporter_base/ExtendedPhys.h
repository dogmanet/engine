#ifndef __EXTENDED_PHYS_H
#define __EXTENDED_PHYS_H

#include "stdafx.h"
#include "IExtendedTab.h"

class CExtendedPhys final: public IExtendedTab
{
public:
	void init(HWND hWndParent) override;
	const char* getTitle() override
	{
		return("Physics");
	}

	void show(int X, int Y, int cx, int cy) override;
	void hide() override;

	void setWeight(float fWeight);
	float getWeight();

	void commit() override;
	void rollback() override;

private:
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void loadDefaults();

private:
	HWND m_hDlg = NULL;

	float m_fWeight = 1.0f;
};

#endif
