#ifndef __TEXTUREWINDOW_H
#define __TEXTUREWINDOW_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCore.h>
#include <mtrl/IXMaterialSystem.h>
#include "TextureViewGraphNodeData.h"

#define WC_TEXTURE_VIEWPORT "XTextureViewport"

class CTextureWindow final: public IXUnknownImplementation<IXUnknown>
{
public:
	static bool RegisterWindowClass(HINSTANCE hInstance);
	static bool UnregisterWindowClass();

	static void Setup(IXCore *pCore);

private:
	CTextureWindow(HWND hWnd);
	~CTextureWindow();
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void init();

private:
	static HINSTANCE ms_hInstance;
	static IXRender *ms_pRender;
	static IXMaterialSystem *ms_pMaterialSystem;
	static UINT ms_uInstanceCounter;
	static IXRenderGraph *ms_pRenderGraph;
	static IXRenderGraphNodeInstance *ms_pRenderGraphNode;
	static Array<CTextureWindow*> ms_aDelayInit;
	
	//IXCore *m_pCore = NULL;
	//IXRender *m_pRender = NULL;
	//IGXDevice *m_pDevice = NULL;
	//
	//IGXDepthStencilState *m_pDSDefault = NULL;
	//XRenderPassHandler *m_pRenderPassGeometry3D = NULL;
	
	HWND m_hWnd = NULL;

	IXTexture *m_pCurrentTexture = NULL;
	IXRenderTarget *m_pTarget = NULL;

	//CTextureViewController *m_pController = NULL;

	StringW m_wsCurrentTexture;
	bool m_isDirty = true;
};

#endif
