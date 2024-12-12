#ifndef __EDITORBROWSERWINDOW_H
#define __EDITORBROWSERWINDOW_H

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <common/array.h>
#include <common/assotiativearray.h>
#include <common/string.h>
#include <common/aastring.h>
#include <xcommon/IFileSystem.h>
#include <xcommon/editor/IXEditorExtension.h>
#include <xcommon/render/IXRender.h>
#include <xcommon/particles/IXParticleSystem.h>
#include <xcommon/IXCore.h>

class CEffectBrowserWindow final: public IXUnknownImplementation<IXEditorResourceBrowser>
{
private:
	INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK EditBoxCtl_SubclassProc(HWND hWndEditBox, UINT message, WPARAM wParam, LPARAM lParam);
	WNDPROC m_pEditBoxPrevWndProc = NULL;

	static LRESULT CALLBACK TreeViewCtl_SubclassProc(HWND hWndEditBox, UINT message, WPARAM wParam, LPARAM lParam);
	WNDPROC m_pTreeViewPrevWndProc = NULL;

public:
	CEffectBrowserWindow(HINSTANCE hInstance, HWND hMainWnd, IXCore *pCore, IXParticleSystem *pParticleSystem);
	~CEffectBrowserWindow();

	//const char *getLevelName();
	void setRender(IXRender *pRender);

	UINT XMETHODCALLTYPE getResourceTypeCount() override;
	const char* XMETHODCALLTYPE getResourceType(UINT uId) override;

	void XMETHODCALLTYPE browse(const char *szType, const char *szOldValue, IXEditorResourceBrowserCallback *pCallback) override;

	void XMETHODCALLTYPE cancel() override;
private:
	struct Record
	{
		String sName;
		UINT uParent;
		bool isDir;

		HTREEITEM hTreeItem = NULL;
		bool isVisible = true;
		bool isFilterPassed = true;
		bool hasVisibleChild = true;

		bool isUserInteracted = false;
		bool isUserCollapsed = true;

		Record(const String &sName, UINT uParent, bool isDir):
			sName(sName),
			uParent(uParent),
			isDir(isDir)
		{}
		Record() = default;
	};

private:
	void registerClass();

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void scanFileSystem();
	void scanFileSystemLevel(const char *szCurrentFolder, UINT uParent, char *szTempBuf);

	void createTree(UINT uStartFrom = 0);

	void filter();

	Record* getRecordByHandle(HTREEITEM hItem, _out UINT *puIndex = NULL);

	bool renameItem(HTREEITEM hItem, const char *szName);

	void makePath(char *szBuf, Record *pRec);

	bool hasChildWithName(UINT uParent, const char *szName);

	void removeRecord(UINT uIndex);

private:
	HINSTANCE m_hInstance;
	HWND m_hMainWnd;
	HWND m_hDlgWnd = NULL;
	HMENU m_hContextMenu = NULL;
	IXCore *m_pCore = NULL;

	IXRenderTarget *m_pFinalTarget = NULL;
	IXCamera *m_pCamera = NULL;

	IFileSystem *m_pFS;

	HWND m_hTreeWnd = NULL;
	HWND m_hOkButtonWnd = NULL;
	HWND m_hPreviewWnd = NULL;

	bool m_hasFilter = false;

	Array<Record> m_aRecords;

	IXEditorResourceBrowserCallback *m_pCallback = NULL;

	IXParticleSystem *m_pParticleSystem = NULL;
	IXParticlePlayer *m_pPlayer = NULL;

	IXRender *m_pRender = NULL;
};

#endif
