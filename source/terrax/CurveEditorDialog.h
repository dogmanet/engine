#ifndef __CURVE_EDITOR_DIALOG_H
#define __CURVE_EDITOR_DIALOG_H

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
// #include <common/array.h>
// #include <common/assotiativearray.h>
// #include <common/string.h>
// #include <common/aastring.h>
// #include <graphix/graphix.h>
// #include <mtrl/IXMaterialSystem.h>
// #include <xcommon/gui/IXFontManager.h>
#include <xcommon/render/IXRender.h>

#include "CurveEditorGraphNodeData.h"

class CCurveEditorDialog final
{
private:
	INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	
public:
	CCurveEditorDialog(HINSTANCE hInstance, HWND hMainWnd);
	~CCurveEditorDialog();

	void browse();

	void initGraphics(IXRender *pRender);
	void render();
	void swapBuffers();

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

	UINT m_uPanelWidth = 0;
	UINT m_uPanelHeight = 0;

#if 0
	IXMaterialSystem *m_pMaterialSystem = NULL;
	IXFont *m_pFont = NULL;
	IXFontManager *m_pFontManager = NULL;

	Array<XCharRect> m_aCharRects;
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
#endif

	IXCamera *m_pCamera = NULL;
	
	CMinMaxCurve m_curve;

	CCurveEditorGraphNodeData *m_pNodeData = NULL;

	bool m_isTrackingLeaveEvent = false;

	bool m_isDraggingKey = false;
	int m_iActiveCurve = -1;
	int m_iActiveKey = -1;

	bool m_isDraggingTangent = false;
	bool m_isDraggingInTangent = false;

private:
	void registerClass();

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void initViewport();

	bool hitTest(float fX, float fY, IXAnimationCurve *pCurve, int *piHitKeyframe);
};

#endif
