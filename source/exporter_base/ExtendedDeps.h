#ifndef __EXTENDED_DEPS_H
#define __EXTENDED_DEPS_H

#include "stdafx.h"
#include "IExtendedTab.h"
#include <dseplugin/ModelFile.h>

class CExtendedDeps final: public IExtendedTab
{
public:
	void init(HWND hWndParent) override;
	const char* getTitle() override
	{
		return("Imports");
	}

	void show(int X, int Y, int cx, int cy) override;
	void hide() override;

	void commit() override;
	void rollback() override;

	void addImport(ModelPart &pt);

	UINT getImportCount()
	{
		return(m_aParts.size());
	}

	const ModelPart& getImport(UINT idx);

private:
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK EditDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void loadDefaults();
	
	bool invokeEditor(int iItem);

private:
	HWND m_hDlg = NULL;
	HWND m_hListWnd = NULL;

	int m_iEditRow = -1;
	
	Array<String> m_asActivities;
	Array<ModelPart> m_aParts;
};

#endif
