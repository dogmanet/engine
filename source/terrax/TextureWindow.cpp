#include <xcommon/IPluginManager.h>
#include "TextureWindow.h"
#include "TextureViewGraphNode.h"

HINSTANCE CTextureWindow::ms_hInstance;
IXRender *CTextureWindow::ms_pRender;
IXMaterialSystem *CTextureWindow::ms_pMaterialSystem;
UINT CTextureWindow::ms_uInstanceCounter;
IXRenderGraph *CTextureWindow::ms_pRenderGraph;
IXRenderGraphNodeInstance *CTextureWindow::ms_pRenderGraphNode;
Array<CTextureWindow*> CTextureWindow::ms_aDelayInit;

CTextureWindow::CTextureWindow(HWND hWnd):
	m_hWnd(hWnd)
{
	init();
}
CTextureWindow::~CTextureWindow()
{
	mem_release(m_pCurrentTexture);
	mem_release(m_pTarget);
	//mem_release(m_pController);

	int idx = ms_aDelayInit.indexOf(this);
	if(idx >= 0)
	{
		ms_aDelayInit.erase(idx);
	}
}

void CTextureWindow::init()
{
	if(ms_pRender)
	{
		char tmp[64];
		sprintf(tmp, "xTextureViewport_%u", ++ms_uInstanceCounter);
		ms_pRender->newFinalTarget(m_hWnd, tmp, &m_pTarget);
		m_pTarget->attachGraph(ms_pRenderGraph);


		//ms_pRenderGraph->
		/*IXRenderGraphNodeData *pNodeTarget;
		if(m_pTarget->getNodeTarget(ms_pRenderer, &pNodeTarget))
		{
			pNodeTarget->getInternalData(&X_TEXTURE_VIEW_INTERNALS_GUID, (void**)&m_pController);
			mem_release(pNodeTarget);
		}*/
		
		RECT rc;
		GetClientRect(m_hWnd, &rc);
		m_pTarget->resize(rc.right - rc.left, rc.bottom - rc.top);
	}
	else
	{
		ms_aDelayInit.push_back(this);
	}
}

bool CTextureWindow::RegisterWindowClass(HINSTANCE hInstance)
{
	ms_hInstance = hInstance;

	WNDCLASSEXW wcex = {sizeof(wcex)};

	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	//wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_LOGO));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	
	wcex.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
	CMB2WC wcs(WC_TEXTURE_VIEWPORT);
	wcex.lpszClassName = wcs;
	//wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	if(!RegisterClassExW(&wcex))
	{
		MessageBox(NULL, "Unable to register window class!", "Error", MB_OK | MB_ICONSTOP);
		return(false);
	}

	return(true);
}
bool CTextureWindow::UnregisterWindowClass()
{
	return(UnregisterClassW(CMB2WC(WC_TEXTURE_VIEWPORT), ms_hInstance));
}

void CTextureWindow::Setup(IXCore *pCore)
{
	IPluginManager *pPM = pCore->getPluginManager();
	ms_pMaterialSystem = (IXMaterialSystem*)pPM->getInterface(IXMATERIALSYSTEM_GUID);
	ms_pRender = (IXRender*)pPM->getInterface(IXRENDER_GUID);

	ms_pRender->getGraph("xTextureView", &ms_pRenderGraph);
	ms_pRenderGraph->getNodeInstance("node", &ms_pRenderGraphNode);

	fora(i, ms_aDelayInit)
	{
		ms_aDelayInit[i]->init();
	}
	ms_aDelayInit.clear();
}

LRESULT CALLBACK CTextureWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CTextureWindow *pThis;

	if(msg == WM_CREATE)
	{
		pThis = new CTextureWindow(hWnd);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
	}
	else
	{
		pThis = (CTextureWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	}

	if(pThis)
	{
		return(pThis->wndProc(hWnd, msg, wParam, lParam));
	}
	return(DefWindowProc(hWnd, msg, wParam, lParam));
}
LRESULT CALLBACK CTextureWindow::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_DESTROY:
		Release();
		break;

	case WM_SIZE:
		SAFE_CALL(m_pTarget, resize, LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_SETTEXT:
		m_wsCurrentTexture = (wchar_t*)lParam;
		mem_release(m_pCurrentTexture);
		ms_pMaterialSystem->getTexture(CWC2MB(m_wsCurrentTexture.c_str()), &m_pCurrentTexture);
		ms_pRenderGraphNode->setTexture(m_pTarget, 0, m_pCurrentTexture);
		// NO BREAK!
	case WM_PAINT:
		m_isDirty = true;
		// NO BREAK!
	default:
		return(DefWindowProcW(hWnd, msg, wParam, lParam));
	}

	return(0);
}
