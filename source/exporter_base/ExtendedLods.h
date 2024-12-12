#ifndef __EXTENDED_LODS_H
#define __EXTENDED_LODS_H

#include "stdafx.h"
#include "IExtendedTab.h"

class CExtendedLods final: public IExtendedTab
{
public:
	void init(HWND hWndParent) override;
	const char* getTitle() override
	{
		return("LoDs");
	}

	void show(int X, int Y, int cx, int cy) override;
	void hide() override;

	void commit() override;
	void rollback() override;

	void addLod(UINT uTris, UINT uSubsets);

	UINT getCount()
	{
		return(m_aList.size());
	}
	int getItem(UINT id)
	{
		return(m_aList[id].iRow);
	}

	class IListener
	{
	public:
		virtual void onCommitted() = 0;
	};

	void addListener(IListener *pListener);
	void removeListener(IListener *pListener);

private:
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void loadDefaults();
	
	void swapRows(int i1, int i2, int iColNumber);

	void invokeListener();

private:
	HWND m_hDlg = NULL;
	HWND m_hListWnd = NULL;

	struct Lod
	{
		int iRow;
		UINT uTris;
		UINT uSubsets;
	};

	Array<Lod> m_aList;

	Array<IListener*> m_apListeners;
};

#endif
