#ifndef __PLUGIN_H
#define __PLUGIN_H

#include <exporter_base/stdafx.h>

#include <msLib/msPlugIn.h>
#include <msLib/msLib.h>

#include <vector>
#include <map>


class CPlugin: public cMsPlugIn
{
	//friend INT_PTR CALLBACK ExportDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
	CPlugin();
	~CPlugin();

    int GetType();
    const char* GetTitle();
    int Execute(msModel *pModel);

private:
	HWND m_hProgressBar = NULL;
	HWND m_hDlgWnd = NULL;

	msModel *m_pMdl = NULL;
};

#endif
