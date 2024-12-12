#ifndef __EXTENDED_ACTS_H
#define __EXTENDED_ACTS_H

#include "stdafx.h"
#include "IExtendedTab.h"

class CExtendedActs final: public IExtendedTab
{
public:
	void init(HWND hWndParent) override;
	const char* getTitle() override
	{
		return("Activities");
	}

	void show(int X, int Y, int cx, int cy) override;
	void hide() override;

	void commit() override;
	void rollback() override;

	void addActivity(const char *szName);
	const Array<String>& getList()
	{
		return(m_aActivities);
	}

	const Array<String> getUncommittedList();

	class IListener
	{
	public:
		virtual void onCommitted() = 0;
		virtual void onChanged() = 0;
	};

	void addListener(IListener *pListener);
	void removeListener(IListener *pListener);

	void promptForNew(bool isCommit = false);

private:
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK EditDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void loadDefaults();

	bool invokeEditor(HWND hWnd, LPCTSTR szDefault, LPCTSTR szTitle);

	void invokeListener(bool isCommit);

private:
	HWND m_hDlg = NULL;
	HWND m_hListWnd = NULL;

	const char *m_szEditDefault = NULL;
	const char *m_szEditTitle = NULL;

	Array<String> m_aActivities;

	Array<IListener*> m_apListeners;
};

#endif
