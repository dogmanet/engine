#ifndef __EXTENDED_H
#define __EXTENDED_H

#include "stdafx.h"

#include "IExtendedTab.h"

class CExtendedTabCallback;
class CExtended
{
public:
	CExtended(HWND hMainWnd);
	~CExtended();

	void show();
	void hide();
	bool isVisible();

	void addCustomTab(IExtendedTab *pTab);

	void onAddTab(CExtendedTabCallback *pTabCB);
	void onRemoveTab(CExtendedTabCallback *pTabCB);

	UINT getCustomTabCount();
	IExtendedTab* getCustomTab(UINT idx);

protected:
	INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	
	void onSelChanged();

	void commit();
	void rollback();

protected:

	HWND m_hDlgWnd;
	HWND m_hTabControl;
	IExtendedTab *m_pCurrentTab = NULL;

	Array<CExtendedTabCallback*> m_aCustomTabs;
};

#endif
