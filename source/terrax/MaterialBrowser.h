#ifndef __MATERIAL_BROWSER_H
#define __MATERIAL_BROWSER_H

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <common/array.h>
#include <common/assotiativearray.h>
#include <common/string.h>
#include <common/aastring.h>
#include <graphix/graphix.h>
#include <mtrl/IXMaterialSystem.h>
#include <xcommon/gui/IXFontManager.h>
#include <xcommon/render/IXRender.h>
//#include "terrax.h"

#include "ScrollBar.h"

class CMaterialBrowser;
class CScrollEventListener final: public IScrollEventListener
{
public:
	CScrollEventListener(HWND hWnd);

	void onPageUp() override;
	void onPageDown() override;
	void onLineUp() override;
	void onLineDown() override;
	void onScroll(int iScroll) override;

private:
	HWND m_hWnd;
};

class IXMaterial;

class IMaterialBrowserCallback
{
public:
	virtual void onSelected(const char *szName) = 0;
	virtual void onCancel() = 0;
};

class CMaterialBrowser final
{
private:
	INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	
public:
	CMaterialBrowser(HINSTANCE hInstance, HWND hMainWnd);
	~CMaterialBrowser();

	void browse(IMaterialBrowserCallback *pCallback, bool bTextureOnly = false);
	void abort();

	void initGraphics(IXRender *pRender);
	void render();
	void swapBuffers();

	void update(float dt);

private:
	HINSTANCE m_hInstance;
	HWND m_hMainWnd;
	HWND m_hDlgWnd = NULL;
	HWND m_hBrowserWnd = NULL;

	struct DialogItem
	{
		HWND hWnd;
		UINT uDeltaPos;
	};

	Array<DialogItem> m_aBottomList;
	UINT m_uPanelDeltaX = 0;
	UINT m_uPanelDeltaY = 0;

	bool m_isDirty = false;
	bool m_bDoSwap = false;
	IXRender *m_pRender = NULL;
	IGXDevice *m_pDev = NULL;
	//IGXSwapChain *m_pSwapChain = NULL;
	IGXSurface *m_pSurface = NULL;
	IXRenderTarget *m_pFinalTarget = NULL;

	bool m_isResizing = false;
	bool m_isScreenSizeChanged = false;

	IGXIndexBuffer *m_pFrameIB = NULL;
	IGXRenderBuffer *m_pFrameRB = NULL;
	UINT m_uFrameVC = 0;
	UINT m_uFramePC = 0;

	IGXIndexBuffer *m_pInnerIB = NULL;
	IGXRenderBuffer *m_pInnerRB = NULL;
	UINT m_uInnerVC = 0;
	UINT m_uInnerPC = 0;

	IGXConstantBuffer *m_pTransformCB = NULL;
	IGXConstantBuffer *m_pInformCB = NULL;

	struct FrameState
	{
		float2_t vSize;
		float fTitleSize;
		float fHighlight;

		float2_t vPosition;
		float fScale;
		float _dummy;
	};

	FrameState m_frameState;

	ID m_idFrameShader = -1;
	ID m_idInnerShader = -1;

	enum FRAME_SIZE
	{
		FRAME_SIZE_128 = 128,
		FRAME_SIZE_256 = 256,
		FRAME_SIZE_512 = 512
	};

	FRAME_SIZE m_frameSize = FRAME_SIZE_128;

	struct MaterialItem
	{
		UINT uXpos;
		UINT uYpos;
		String sName;
		IXMaterial *pMaterial;
		IXTexture *pTexture;
		bool isTexture;
		bool isMaterial;
		bool isTranslated;
		bool isTransparent;

		UINT uQuads;
		UINT uTitleWidth;
		XFontVertex *pVertices;

		UINT uCurrentFrame;
		UINT uTotalFrames;
		float fFrameTime;
		float fCurrentTime;
	};

	Array<MaterialItem> m_aMaterials;
	
	int m_iScrollPos = 0;
	int m_iScrollHeight = 0;

	UINT m_uPanelWidth = 0;
	UINT m_uPanelHeight = 0;

	CScrollBar *m_pScrollBar = NULL;
	bool m_isTrackingScroll = false;
	bool m_isHoverScroll = false;
	bool m_isTrackingLeaveEvent = false;
	CScrollEventListener *m_pScrollbarEvent = NULL;

	IXMaterialSystem *m_pMaterialSystem = NULL;
	IXFont *m_pFont = NULL;
	IXFontManager *m_pFontManager = NULL;

	Array<XCharRect> m_aCharRects;
	UINT m_uFontGenStart = ~0;
	UINT m_uFontGenEnd = ~0;
	Array<XFontGPUVertex> m_aTextVB;
	UINT m_uTextRBSize = 0;
	UINT m_uTextVertexCount = 0;
	UINT m_uTextQuadCount = 0;
	IGXRenderBuffer *m_pTextRB = NULL;
	IGXVertexBuffer *m_pTextVB = NULL;
	IGXIndexBuffer *m_pTextIB = NULL;
	IGXVertexDeclaration *m_pTextVD = NULL;
	ID m_idTextShader = -1;
	IGXConstantBuffer *m_pTextColorCB = NULL;
	IGXConstantBuffer *m_pTextOffsetCB = NULL;

	bool m_isFilterScheduled = false;
	float m_fFilterTimer = 0.0f;
	float m_fFilterAt = 0.0f;

	UINT m_uSelectedItem = ~0;

	IMaterialBrowserCallback *m_pCallback = NULL;

	//UINT m_uCurrentDpi = USER_DEFAULT_SCREEN_DPI;
	float m_fScale = 1.0f;

private:
	void registerClass();

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void initViewport();
	void initHelpers();

	void drawFrame(int iXpos, int iYpos, FRAME_SIZE frameSize, UINT uTitleLen, float fSelection);

	void layout();

	void filter();

	void preload();

	void scheduleFilterIn(float fSeconds);

	void initFont();
	void invalidateTexts();
};

#endif
