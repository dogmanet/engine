#ifndef __PROPERTY_WINDOW_H
#define __PROPERTY_WINDOW_H

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <common/array.h>
#include <common/assotiativearray.h>
#include <common/string.h>
#include <common/aastring.h>
#include <xcommon/editor/IXEditorExtension.h>
#include "terrax.h"

class CEditorResourceBrowserCallback final: public IXEditorResourceBrowserCallback
{
public:
	void init(HWND hDlgWnd, HWND hEditWnd, IXEditorResourceBrowser *pBrowser);
	void XMETHODCALLTYPE onSelected(const char *szFile) override;
	void XMETHODCALLTYPE onCancelled() override;
	void cancel();
private:
	HWND m_hDlgWnd = NULL;
	HWND m_hEditWnd = NULL;
	IXEditorResourceBrowser *m_pBrowser = NULL;
};

class CEditorPropertyTabCallback;
class CPropertyWindow
{
public:
	CPropertyWindow(HINSTANCE hInstance, HWND hMainWnd);
	~CPropertyWindow();

	void show();
	void hide();
	bool isVisible();

	void clearClassList();
	void addClass(const char *szClassName);

	void clearProps();
	void addPropField(const X_PROP_FIELD *pField, const char *szValue = NULL);
	void setPropFieldValue(const char *szKey, const char *szValue);

	void setClassName(const char *szClassName);
	void allowClassChange(bool bAllow);

	void addCustomTab(IXEditorPropertyTab *pTab);

	class ICallback
	{
	public:
		virtual void onClassChanged(const char *szNewClassName) = 0;
		virtual void onCancel() = 0;
		virtual void onApply() = 0;
		virtual bool onPropertyChanged(const char *szKey, const char *szValue) = 0;
	};

	void setCallback(ICallback *pCallback)
	{
		m_pCallback = pCallback;
	}

	void onAddTab(CEditorPropertyTabCallback *pTabCB);
	void onRemoveTab(CEditorPropertyTabCallback *pTabCB);

	UINT getCustomTabCount();
	IXEditorPropertyTab* getCustomTab(UINT idx);

protected:
	INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK PropDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK EditorDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK EditEnterDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK ClassListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	
	void onSelChanged();
	void onPropListChanged();
	void filterClassList(const char *szFilter);
	void initEditor(X_PROP_EDITOR_TYPE type, const void *pData, const char *szValue);

protected:

	static WNDPROC m_defEditProc;

	HINSTANCE m_hInstance;
	HWND m_hDlgWnd;
	HWND m_hTabControl;
	HWND m_hCurrentTab = NULL;
	IXEditorPropertyTab *m_pCurrentTab = NULL;
	Array<HWND> m_hPropTabs;

	HWND m_hClassListWnd = NULL;
	HWND m_hPropListWnd = NULL;
	HWND m_hNoPropsText = NULL;

	WNDPROC m_pfnClassListOldWndproc;

	Array<String> m_asClassList;

	HWND m_phEditors[XPET__LAST];

	struct prop_s
	{
		X_PROP_FIELD field;
		String sValue;
	};

	struct edt_kv
	{
		const char *szKey;
		const char *szValue;
	};

	AssotiativeArray<AAString, prop_s> m_aPropFields;
	X_PROP_EDITOR_TYPE m_editorActive = XPET__LAST;

	ICallback *m_pCallback = NULL;

	const char *szCurrentFileType = NULL;

	UINT m_uCurrentFlags = 0;
	const char *m_szFlagsField = NULL;


	Array<CEditorPropertyTabCallback*> m_aCustomTabs;
	int m_iTabX = 0;
	int m_iTabY = 0;
	int m_iTabCX = 0;
	int m_iTabCY = 0;


	CEditorResourceBrowserCallback m_resourceBrowserCallback;
};

#endif
