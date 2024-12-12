#ifndef __EXTENDED_SKIN_H
#define __EXTENDED_SKIN_H

#include "stdafx.h"
#include "IExtendedTab.h"

class CExtendedSkin final: public IExtendedTab
{
public:
	void init(HWND hWndParent) override;
	const char* getTitle() override
	{
		return("Skins");
	}

	void show(int X, int Y, int cx, int cy) override;
	void hide() override;

	void addColumn(LPCTSTR str);
	void addOverride(int iSkin, int iColumn, LPCTSTR str);

	int getColumnCount();
	int getRowCount();
	LPCTSTR getColumn(int i);
	LPCTSTR getCell(int row, int col);

	void commit() override;
	void rollback() override;


private:
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK SkinsEditDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	
	void loadDefaults();

	bool invokeEditor(HWND hWnd, LPCTSTR szDefault, LPCTSTR szTitle);

	LPCTSTR m_szEditDefault = NULL;
	LPCTSTR m_szEditTitle = NULL;

	void reindexRows();
	int getSelectedIndex();
	void swapRows(int i1, int i2, int iColNumbers);

private:
	HWND m_hDlg = NULL;
	HWND m_hListWnd = NULL;

	bool m_isHeaderLoaded = false;

	Array<String> m_vszHeaders;
	Array<Array<String>> m_vvszRows;
};

#endif
