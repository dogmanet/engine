#ifndef __MATERIAL_EDITOR_H
#define __MATERIAL_EDITOR_H

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <common/array.h>
#include <common/assotiativearray.h>
#include <common/string.h>
#include <common/aastring.h>
#include <graphix/graphix.h>
#include <mtrl/IXMaterialSystem.h>
#include <xcommon/gui/IXFontManager.h>
#include "MaterialBrowser.h"
#include <xcommon/IXCore.h>
//#include "terrax.h"

class IXMaterial;
class CMaterialEditor;

class CTextureBrowserCallback: public IMaterialBrowserCallback
{
public:
	CTextureBrowserCallback(CMaterialEditor *pMatEditor);

	void prepareSelect(const char *szKey);

	void onSelected(const char *szName) override;
	void onCancel() override;

private:
	CMaterialEditor *m_pMatEditor;
	const char *m_szKey = NULL;
};

//#############################################################################

class CMaterialEditor final: public IXUnknownImplementation<IXUnknown>
{
private:
	INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	
public:
	CMaterialEditor(HINSTANCE hInstance, HWND hMainWnd, IXCore *pCore, IXMaterial *pMaterial);
	~CMaterialEditor();

	void browse();

	//void initGraphics(IGXDevice *pDev);
	//void render();
	//void swapBuffers();

	//void update(float dt);

	void setTexture(const char *szKey, const char *szTexture);

private:
	HINSTANCE m_hInstance;
	HWND m_hMainWnd;
	HWND m_hDlgWnd = NULL;
	HWND m_hPropsWnd = NULL;

	HMENU m_hTextureMenu = NULL;
	HMENU m_hTextureOptMenu = NULL;

	struct DialogItem
	{
		HWND hWnd;
		UINT uDeltaPos;
	};

	Array<DialogItem> m_aBottomList;
	UINT m_uPanelDeltaX = 0;
	UINT m_uPanelDeltaY = 0;

	UINT m_uMinHeight = 0;
	UINT m_uFixedWidth = 0;
	
	int m_iScrollPos = 0;
	int m_iScrollHeight = 0;
	int m_iTotalHeight = 0;

	UINT m_uPanelWidth = 0;
	UINT m_uPanelHeight = 0;

	IXMaterialSystem *m_pMaterialSystem = NULL;
	IXMaterial *m_pMaterial = NULL;

	Array<XMaterialProperty> m_aProperties;
	
	Array<HWND> m_ahChildren;

	CTextureBrowserCallback m_cbTextureBrowser;

	Array<const char*> m_aszFilters;

	bool m_isDirty = false;

private:
	//void registerClass();

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	
	INT_PTR CALLBACK innerDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK InnerDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK TrackbarWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void initFields();
	void updateScroll();

	bool save();

	void backup();
	void undo();

private:
	struct BackupRecordFlag
	{
		String sKey;
		bool isSet;
	};
	struct BackupRecordProp
	{
		String sKey;
		float4_t vVal;
	};
	struct BackupRecordTex
	{
		String sKey;
		String sTexture;
	};
	struct
	{
		String sShader;
		MTLTYPE_PHYSIC physType;
		Array<BackupRecordFlag> aFlags;
		Array<BackupRecordProp> aProps;
		Array<BackupRecordTex> aTexs;
	}
	m_backup;
};

#endif
