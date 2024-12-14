
/******************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017
See the license in LICENSE
******************************************************/

#define _CRT_SECURE_NO_WARNINGS

//#include <vld.h> 
#include <skyxengine.h>
#include <io.h>
#include <windowsx.h>
#include <commctrl.h>
#pragma comment(lib, "Comctl32.lib")
#include "resource.h"

#include "terrax.h"
#include "Grid.h"
#include "PropertyWindow.h"

#include <xcommon/editor/IXEditorObject.h>
#include <xcommon/editor/IXEditable.h>
#include <xcommon/IXRenderable.h>
#include <xcommon/resource/IXResourceManager.h>
#include <xcommon/render/IXRenderUtils.h>
#include <xcommon/IXModelWriter.h>
#include <xcommon/IXJSON.h>
#include <mtrl/IXMaterialSystem.h>
#include "UndoManager.h"
#include "Tools.h"

#include <xEngine/IXEngine.h>
#include <xWindow/IXWindowSystem.h>
#include "Editor.h"

#include "ProxyObject.h"

#include "3DOverlayGraphNode.h"
#include "2DGraphNode.h"
#include "TextureViewGraphNode.h"
#include "TextureWindow.h"
#include "MaterialBrowserGraphNode.h"
#include "CurveEditorGraphNode.h"
#include "GradientPreviewGraphNode.h"

#include <xcommon/editor/IXEditorImporter.h>

#ifdef _DEBUG
#	pragma comment(lib, "xWindow_d.lib")
#	pragma comment(lib, "xEngine_d.lib")
#else
#	pragma comment(lib, "xWindow.lib")
#	pragma comment(lib, "xEngine.lib")
#endif

GX_ENABLE_HIGH_PERFORMANCE_DUAL_GPU();

extern HWND g_hWndMain;
CGrid *g_pGrid = NULL;
CTerraXRenderStates g_xRenderStates;

extern CPropertyWindow *g_pPropWindow;

CEditor *g_pEditor;

IXEditorGizmoMove *g_pGizmoMove = NULL;
IXEditorGizmoRotate *g_pGizmoRotate = NULL;

ATOM XRegisterClass(HINSTANCE hInstance);
BOOL XInitInstance(HINSTANCE, int);

Array<IXEditable*> g_pEditableSystems;
Array<IXEditorObject*> g_pLevelObjects;
Map<AAString, IXEditable*> g_mEditableSystems;
Map<XGUID, IXEditorModel*> g_apLevelModels;
Map<IXEditorObject*, ICompoundObject*> g_mObjectsLocation;
Array<CProxyObject*> g_apProxies;
Array<CGroupObject*> g_apGroups;
//SGeom_GetCountModels()
Array<IXEditorImporter*> g_pEditorImporters;

Array<IXEditorExtension*> g_apExtensions;

static IGXVertexBuffer *g_pBorderVertexBuffer;
static IGXRenderBuffer *g_pBorderRenderBuffer;

CUndoManager *g_pUndoManager = NULL;
extern HWND g_hComboTypesWnd;

extern bool g_bViewportCaptionDirty[4];

String g_sLevelName;

DECLARE_PROFILER_INTERNAL();

IEventChannel<XEventEditorXformType> *g_pXformEventChannel = NULL;

void XUpdateWindowTitle();

HACCEL g_hAccelTableMain = NULL;
HACCEL g_hAccelTableEdit = NULL;
IXEngine *g_pEngine = NULL;
IXRender *g_pRender = NULL;

IGXConstantBuffer *g_pCameraConstantBuffer = NULL;

IXRenderTarget *g_pTopLeftTarget = NULL;
IXRenderTarget *g_pTopRightTarget = NULL;
IXRenderTarget *g_pBottomLeftTarget = NULL;
IXRenderTarget *g_pBottomRightTarget = NULL;

IGXSwapChain *g_pGuiSwapChain = NULL;
IGXDepthStencilSurface *g_pGuiDepthStencilSurface = NULL;
//IGXDepthStencilSurface *g_pCurMatDepthStencilSurface = NULL;
IGXDepthStencilState *g_pDSNoZ;
IGXDepthStencilState *g_pDSDefault;

IGXTexture2D *g_pDashedMaterial = NULL;

IXGizmoRenderer *g_pSelectionRenderer = NULL;
IXGizmoRenderer *g_pUnselectedRenderer = NULL;

void XReleaseViewports();
void XInitViewports();
void XInitViewportLayout(X_VIEWPORT_LAYOUT layout);
void XExportToObj(const char *szMdl);
void XExportToDSE(const char *szMdl);
bool IsEditMessage();
bool IsButtonMessage();

class CEngineCallback: public IXEngineCallback
{
	struct SXIKD
	{
		UINT repeatCount : 16;
		UINT scanCode : 8;
		UINT isExtended : 1;
		UINT reserved : 4;
		UINT contextCode : 1;
		UINT previousState : 1;
		UINT transitionState : 1;
	};

public:
	CEngineCallback()
	{
		memset(m_aKeyStates, 0, sizeof(m_aKeyStates));
	}

	void XMETHODCALLTYPE onGraphicsResize(UINT uWidth, UINT uHeight, bool isFullscreen, bool isBorderless, IXEngine *pEngine) override
	{
		static const bool *terrax_detach_3d = NULL;
		if(!terrax_detach_3d && m_pCore)
		{
			terrax_detach_3d = m_pCore->getConsole()->getPCVarBool("terrax_detach_3d");
		}

		if(terrax_detach_3d && *terrax_detach_3d)
		{
			RECT rc = {0, 0, (int)uWidth, (int)uHeight};

			HMONITOR monitor = MonitorFromWindow(g_hTopLeftWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFO info;
			info.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo(monitor, &info);
			bool bForceNoBorder = (rc.right - rc.left == info.rcMonitor.right - info.rcMonitor.left
				&& rc.bottom - rc.top == info.rcMonitor.bottom - info.rcMonitor.top);


			SetClassLong(g_hTopLeftWnd, GCL_STYLE, GetClassLong(g_hTopLeftWnd, GCL_STYLE) | CS_NOCLOSE);

			DWORD wndStyle = GetWindowLong(g_hTopLeftWnd, GWL_STYLE) | WS_MINIMIZEBOX | WS_SYSMENU;

			if(isFullscreen || isBorderless || bForceNoBorder)
			{
				wndStyle &= ~(WS_SYSMENU | WS_CAPTION | WS_MAXIMIZE | WS_MINIMIZE | WS_THICKFRAME);
			}
			else
			{
				wndStyle |= WS_CAPTION | WS_THICKFRAME;
				wndStyle &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
			}

			SetWindowLong(g_hTopLeftWnd, GWL_STYLE, wndStyle);


			AdjustWindowRect(&rc, wndStyle, false);

			UINT uSWPflags = SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOOWNERZORDER;


			int iPosX = 0;
			int iPosY = 0;
			if(!(isFullscreen || isBorderless))
			{
				iPosX = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
				iPosY = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
			}
			RECT rcOld;
			GetWindowRect(g_hTopLeftWnd, &rcOld);


			if(rcOld.right - rcOld.left != rc.right - rc.left || rcOld.bottom - rcOld.top != rc.bottom - rc.top)
			{
				uSWPflags &= ~SWP_NOSIZE;
				//SetWindowPos(m_hWnd, HWND_TOP, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
			}

			SetWindowPos(g_hTopLeftWnd, HWND_TOP, iPosX, iPosY, rc.right - rc.left, rc.bottom - rc.top, uSWPflags);
		}
		else
		{
			XReleaseViewports();
			XInitViewports();
		}
	}

	bool XMETHODCALLTYPE processWindowMessages() override
	{
		MSG msg = {0};

		char className[32];

		while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
			{
				return(false);
			}

			if(IsEditMessage())
			{
				if(TranslateAccelerator(/*GetParent((HWND)SGCore_GetHWND())*/g_hWndMain, g_hAccelTableEdit, &msg))
				{
					continue;
				}
			}
			else if(GetActiveWindow() == g_hWndMain)
			{
				if(g_pCurrentTool)
				{
					if(msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN)
					{
						bool isHandled = false;
						SXIKD * pikd;
						pikd = (SXIKD*)&msg.lParam;

						UINT key = pikd->scanCode + (pikd->isExtended ? 128 : 0);

						if(key < SXI_KEYMAP_SIZE)
						{
							m_aKeyStates[key] = true;
							if(g_pCurrentTool->onKeyDown(key))
							{
								isHandled = true;
							}
						}
						if(
							((key == SIK_LSHIFT || key == SIK_RSHIFT) && (key = SIK_SHIFT))
							|| ((key == SIK_LALT || key == SIK_RALT) && (key = SIK_ALT))
							|| ((key == SIK_LCONTROL || key == SIK_RCONTROL) && (key = SIK_CONTROL))
							)
						{
							m_aKeyStates[key] = true;
							if(g_pCurrentTool->onKeyDown(key))
							{
								isHandled = true;
							}
						}
						if(isHandled)
						{
							continue;
						}
					}

					if(msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP)
					{
						SXIKD * pikd;
						pikd = (SXIKD*)&msg.lParam;

						UINT key = pikd->scanCode + (pikd->isExtended ? 128 : 0);

						if(key < SXI_KEYMAP_SIZE)
						{
							m_aKeyStates[key] = false;
							g_pCurrentTool->onKeyUp(key);
						}
						if(
							((key == SIK_LSHIFT || key == SIK_RSHIFT) && (key = SIK_SHIFT))
							|| ((key == SIK_LALT || key == SIK_RALT) && (key = SIK_ALT))
							|| ((key == SIK_LCONTROL || key == SIK_RCONTROL) && (key = SIK_CONTROL))
							)
						{
							m_aKeyStates[key] = false;
							g_pCurrentTool->onKeyUp(key);
						}
					}
				}

				if(TranslateAccelerator(/*GetParent((HWND)SGCore_GetHWND())*/g_hWndMain, g_hAccelTableMain, &msg))
				{
					continue;
				}
			}

			HWND hWnd = msg.hwnd;
			while(hWnd)
			{
				RealGetWindowClass(hWnd, className, sizeof(className));
				if(!strcmp(className, "#32770"))
				{
					break;
				}
				hWnd = GetParent(hWnd);
			}

			if(hWnd && IsDialogMessage(hWnd, &msg))
			{
				continue;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		static time_point tPrev = std::chrono::high_resolution_clock::now();
		time_point tNow = std::chrono::high_resolution_clock::now();
		float fDeltaTime = (float)std::chrono::duration_cast<std::chrono::microseconds>(tNow - tPrev).count() * 0.000001f;
		tPrev = tNow;

		if(g_is3DRotating || g_is3DPanning || g_is2DPanning)
		{
			int x, y;
			SSInput_GetMouseDelta(&x, &y);
			static const float * sense = GET_PCVAR_FLOAT("cl_mousesense");
			float fCoeff = SMToRadian(0.022) * *sense;
			float dx = (float)x * fCoeff;
			float dy = (float)y * fCoeff;
			IXCamera *pCamera = g_xConfig.m_pViewportCamera[XWP_TOP_LEFT];

			if(g_is3DRotating)
			{
				m_vPitchYawRoll.y -= dx;
				m_vPitchYawRoll.x -= dy;
				m_vPitchYawRoll.x = clampf(m_vPitchYawRoll.x, -SM_PIDIV2, SM_PIDIV2);
				while(m_vPitchYawRoll.y < 0.0f)
				{
					m_vPitchYawRoll.y += SM_2PI;
				}
				while(m_vPitchYawRoll.y > SM_2PI)
				{
					m_vPitchYawRoll.y -= SM_2PI;
				}

				pCamera->setOrientation(SMQuaternion(m_vPitchYawRoll.x, 'x') * SMQuaternion(m_vPitchYawRoll.z, 'z') * SMQuaternion(m_vPitchYawRoll.y, 'y'));
			}
			else if(g_is3DPanning)
			{
				float3 vPos = pCamera->getPosition();
				float3 vUp = pCamera->getUp();
				float3 vRight = pCamera->getRight();
				vPos += vUp * -dy * 10.0f + vRight * dx * 10.0f;
				pCamera->setPosition(vPos);
			}
			else if(g_is2DPanning)
			{
				// vWorldDelta
				IXCamera *pCamera = g_xConfig.m_pViewportCamera[g_xState.activeWindow];
				X_2D_VIEW xCurView = g_xConfig.m_x2DView[g_xState.activeWindow];
				float3 vWorldDelta = float2(-(float)x, (float)y) * g_xConfig.m_fViewportScale[g_xState.activeWindow];

				// printf("%5d, %5d\n", x, y);

				float3 vWorldDelta3D;
				switch(xCurView)
				{
				case X2D_TOP:
					vWorldDelta3D = float3(vWorldDelta.x, 0.0f, vWorldDelta.y);
					break;
				case X2D_FRONT:
					vWorldDelta3D = float3(vWorldDelta.x, vWorldDelta.y, 0.0f);
					break;
				case X2D_SIDE:
					vWorldDelta3D = float3(0.0f, vWorldDelta.y, vWorldDelta.x);
					break;
				}

				pCamera->setPosition(pCamera->getPosition() + vWorldDelta3D);
			}

			if(g_is3DPanning || g_is3DRotating)
			{
				float3 dir;
				static float s_fSpeed = 0;
				float fMaxSpeed = 10.0f; //@TODO: CVar this!
				float fMaxSpeedBoost = 40.0f; //@TODO: CVar this!
				float fAccelTime = 0.5f; //@TODO: CVar this!
				if(GetAsyncKeyState(VK_SHIFT) < 0)
				{
					fMaxSpeed = fMaxSpeedBoost;
				}
				float fAccel = fMaxSpeed / fAccelTime;
				bool mov = false;
				if(GetAsyncKeyState('W') < 0)
				{
					dir.z += 1.0f;
					mov = true;
				}
				if(GetAsyncKeyState('S') < 0)
				{
					dir.z -= 1.0f;
					mov = true;
				}
				if(GetAsyncKeyState('A') < 0)
				{
					dir.x -= 1.0f;
					mov = true;
				}
				if(GetAsyncKeyState('D') < 0)
				{
					dir.x += 1.0f;
					mov = true;
				}

				if(mov)
				{
					s_fSpeed += fAccel * fDeltaTime;
					if(s_fSpeed > fMaxSpeed)
					{
						s_fSpeed = fMaxSpeed;
					}
					float3 vPos = pCamera->getPosition();
					float3 vDir = pCamera->getLook();
					float3 vRight = pCamera->getRight();
					dir = SMVector3Normalize(dir) * fDeltaTime * s_fSpeed;
					vPos += vDir * dir.z + vRight * dir.x;
					pCamera->setPosition(vPos);
				}
				else
				{
					s_fSpeed = 0;
				}
			}
		}

		static const bool *terrax_detach_3d = m_pCore->getConsole()->getPCVarBool("terrax_detach_3d");
		if(!*terrax_detach_3d)
		{
			XFrameRun(fDeltaTime);
		}

		return(true);
	}

	IXCamera* XMETHODCALLTYPE getCameraForFrame() override
	{
		static const bool *terrax_detach_3d = m_pCore->getConsole()->getPCVarBool("terrax_detach_3d");
		if(*terrax_detach_3d)
		{
			return(SGame_GetActiveCamera());
		}
		
		return(g_xConfig.m_pViewportCamera[XWP_TOP_LEFT]);
	}

	void setCore(IXCore *pCore)
	{
		m_pCore = pCore;
	}

	bool isKeyPressed(UINT key)
	{
		assert(key < SXI_KEYMAP_SIZE);

		return(m_aKeyStates[key]);
	}

	void setCameraRotation(const float3 &vPitchYawRoll)
	{
		m_vPitchYawRoll = vPitchYawRoll;
	}

private:
	IXCore *m_pCore = NULL;

	bool m_aKeyStates[SXI_KEYMAP_SIZE];

	float3 m_vPitchYawRoll;
};

//##########################################################################

class CCVarEventListener: public IEventListener<XEventCvarChanged>
{
public:
	CCVarEventListener(IXCore *pCore):
		m_pCore(pCore)
	{}
	void onEvent(const XEventCvarChanged *pEvent) override
	{
		static const bool *terrax_detach_3d = m_pCore->getConsole()->getPCVarBool("terrax_detach_3d");

		if(terrax_detach_3d == pEvent->pCvar)
		{
			if(*terrax_detach_3d)
			{
				ShowWindow(g_hTopLeftWnd, SW_HIDE);
				m_lOldStyle = GetWindowLongA(g_hTopLeftWnd, GWL_STYLE);
				SetParent(g_hTopLeftWnd, NULL);
				SetWindowLongA(g_hTopLeftWnd, GWL_STYLE, WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
				SetWindowTextA(g_hTopLeftWnd, "SkyXEngine");

				m_lOldExStyle = GetWindowLongA(g_hTopLeftWnd, GWL_EXSTYLE);
				SetWindowLongA(g_hTopLeftWnd, GWL_EXSTYLE, 0);

				SetWindowPos(g_hTopLeftWnd, 
#ifdef _DEBUG
					HWND_TOP
#else
					HWND_TOPMOST
#endif
					, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);

				EnableWindow(g_hWndMain, FALSE);

				for(UINT i = 0, l = g_pLevelObjects.size(); i < l; ++i)
				{
					g_pLevelObjects[i]->setSimulationMode(true);
				}

				IXRenderGraph *pGraph;
				if(g_pRender->getGraph("xDefaultScene", &pGraph))
				{
					g_pTopLeftTarget->attachGraph(pGraph);
					mem_release(pGraph);
				}

				g_pTopLeftTarget->setCamera(SGame_GetActiveCamera());
			}
			else
			{
				EnableWindow(g_hWndMain, TRUE);

				ShowWindow(g_hTopLeftWnd, SW_HIDE);
				SetParent(g_hTopLeftWnd, g_hWndMain);
				SetWindowLongA(g_hTopLeftWnd, GWL_STYLE, m_lOldStyle);
				SetWindowLongA(g_hTopLeftWnd, GWL_EXSTYLE, m_lOldExStyle);
				SetWindowPos(g_hTopLeftWnd, HWND_TOP, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
				PostMessage(g_hWndMain, WM_SIZE, 0, 0);

				ShowCursor(TRUE);

				SetForegroundWindow(g_hWndMain);

				for(UINT i = 0, l = g_pLevelObjects.size(); i < l; ++i)
				{
					g_pLevelObjects[i]->setSimulationMode(false);
				}

				IXRenderGraph *pGraph;
				if(g_pRender->getGraph("xTerraX3D", &pGraph))
				{
					g_pTopLeftTarget->attachGraph(pGraph);
					mem_release(pGraph);
				}

				g_pTopLeftTarget->setCamera(g_xConfig.m_pViewportCamera[XWP_TOP_LEFT]);
			}
		}
	}

private:
	IXCore *m_pCore;
	LONG m_lOldStyle;
	LONG m_lOldExStyle;
};

//##########################################################################

CEngineCallback *g_pEngineCallback = NULL;

bool XIsKeyPressed(UINT uKey)
{
	return(g_pEngineCallback->isKeyPressed(uKey));
}

Array<XExtMenuItem> g_aExtMenuItems;

HMENU BuildExtMenu(const XEditorMenuItem *pMenu)
{
	UINT uMenus = pMenu->uItemCount;
	if(!uMenus)
	{
		return(NULL);
	}
	HMENU hMenu = CreatePopupMenu();
	for(UINT j = 0; j < uMenus; ++j)
	{
		const XEditorMenuItem *pMenuItem = &pMenu->aItems[j];
		if(!pMenuItem->szText)
		{
			AppendMenuW(hMenu, MF_SEPARATOR, NULL, NULL);
		}
		else
		{
			HMENU hSubMenu = BuildExtMenu(pMenuItem);

			if(hSubMenu)
			{
				AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, CMB2WC(pMenuItem->szText));
			}
			else
			{
				AppendMenuW(hMenu, MF_STRING, ID_EXT_MENU_FIRST + g_aExtMenuItems.size(), CMB2WC(pMenuItem->szText));
				g_aExtMenuItems.push_back({pMenuItem, hMenu});
			}
		}
	}
	return(hMenu);
}

#if defined(_WINDOWS)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	int argc = 0;
	char **argv = CommandLineToArgvA(lpCmdLine, &argc);
#else
int main(int argc, char **argv)
{
#endif

	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_BAR_CLASSES | ICC_TAB_CLASSES;
	InitCommonControlsEx(&icex);

	XRegisterClass(hInstance);
	g_pUndoManager = new CUndoManager();

	if(!XInitInstance(hInstance, nCmdShow))
	{
		return(1);
	}

	//SkyXEngine_Init(g_hTopLeftWnd, g_hWndMain, lpCmdLine);
	IXEngine *pEngine = XEngineInit(argc, argv, "TerraX");
	INIT_PROFILER_INTERNAL();

	IXWindowSystem *pWindowSystem = XWindowInit();
	pEngine->getCore()->getPluginManager()->registerInterface(IXWINDOWSYSTEM_GUID, pWindowSystem);

	g_pEngine = pEngine;
	CEngineCallback engineCb;
	g_pEngineCallback = &engineCb;
	C3DOverlayGraphNode *p3DOverlayGraphNode = new C3DOverlayGraphNode(pEngine->getCore());
	C2DGraphNode *p2DGraphNode = new C2DGraphNode(pEngine->getCore());
	pEngine->getCore()->getPluginManager()->registerInterface(IXRENDERGRAPHNODE_GUID, p3DOverlayGraphNode);
	pEngine->getCore()->getPluginManager()->registerInterface(IXRENDERGRAPHNODE_GUID, p2DGraphNode);
	pEngine->getCore()->getPluginManager()->registerInterface(IXRENDERGRAPHNODE_GUID, new CTextureViewGraphNode(pEngine->getCore()));
	pEngine->getCore()->getPluginManager()->registerInterface(IXRENDERGRAPHNODE_GUID, new CMaterialBrowserGraphNode(pEngine->getCore()));
	pEngine->getCore()->getPluginManager()->registerInterface(IXRENDERGRAPHNODE_GUID, new CCurveEditorGraphNode(pEngine->getCore()));
	pEngine->getCore()->getPluginManager()->registerInterface(IXRENDERGRAPHNODE_GUID, new CGradientPreviewGraphNode(pEngine->getCore()));
	pEngine->initGraphics((XWINDOW_OS_HANDLE)g_hTopLeftWnd, &engineCb);
	engineCb.setCore(pEngine->getCore());
	pEngine->getCore()->getConsole()->registerCVar("terrax_detach_3d", false, "", FCVAR_NOTIFY);
	pEngine->getCore()->getConsole()->execCommand("gmode editor");
	pEngine->getCore()->getConsole()->execCommand("exec ../config_editor.cfg");

	pEngine->getCore()->getConsole()->registerCommand("obj_dump", [](int argc, const char **argv){
		if(argc != 2)
		{
			printf("Usage: obj_dump <model>");
			return;
		}

		XExportToObj(argv[1]);
	}, "Export model to obj format");

	pEngine->getCore()->getConsole()->registerCommand("dse_dump", [](int argc, const char **argv){
		if(argc != 2)
		{
			printf("Usage: dse_dump <model>");
			return;
		}

		XExportToDSE(argv[1]);
	}, "Export model to dse format");
	
	//IXRenderTarget *g_pTopRightTarget = NULL;
	//IXRenderTarget *g_pBottomLeftTarget = NULL;
	//IXRenderTarget *g_pBottomRightTarget = NULL;
	g_pRender = (IXRender*)Core_GetIXCore()->getPluginManager()->getInterface(IXRENDER_GUID);

	g_pRender->getFinalTarget("xMainWindow", &g_pTopLeftTarget);
	g_pRender->newFinalTarget(g_hTopRightWnd, "xTopRightWindow", &g_pTopRightTarget);
	g_pRender->newFinalTarget(g_hBottomLeftWnd, "xBottomLeftWindow", &g_pBottomLeftTarget);
	g_pRender->newFinalTarget(g_hBottomRightWnd, "xBottomRightWindow", &g_pBottomRightTarget);

	IXRenderGraph *pGraph;
	if(g_pRender->getGraph("xTerraX2D", &pGraph))
	{
		g_pTopRightTarget->attachGraph(pGraph);
		g_pBottomLeftTarget->attachGraph(pGraph);
		g_pBottomRightTarget->attachGraph(pGraph);
		mem_release(pGraph);
	}

	if(g_pRender->getGraph("xTerraX3D", &pGraph))
	{
		g_pTopLeftTarget->attachGraph(pGraph);
		mem_release(pGraph);
	}

	IXRenderUtils *pUtils = (IXRenderUtils*)Core_GetIXCore()->getPluginManager()->getInterface(IXRENDERUTILS_GUID);
	pUtils->newGizmoRenderer(&g_pSelectionRenderer);
	pUtils->newGizmoRenderer(&g_pUnselectedRenderer);

	CTextureWindow::Setup(pEngine->getCore());

	g_pEditor = new CEditor(Core_GetIXCore());
	Core_GetIXCore()->getPluginManager()->registerInterface(IXEDITOR_GUID, g_pEditor);

	g_pEditor->newGizmoMove(&g_pGizmoMove);
	g_pEditor->newGizmoRotate(&g_pGizmoRotate);
	g_pGizmoMove->enable(false);
	g_pGizmoRotate->enable(false);
	g_pGizmoMove->setCallback(&g_gizmoMoveCallback);
	g_pGizmoRotate->setCallback(&g_gizmoRotateCallback);

	CCVarEventListener cvarListener(pEngine->getCore());
	auto *pChannel = pEngine->getCore()->getEventChannel<XEventCvarChanged>(EVENT_CVAR_CHANGED_GUID);
	pChannel->addListener(&cvarListener);

	g_pXformEventChannel = pEngine->getCore()->getEventChannel<XEventEditorXformType>(EVENT_EDITOR_XFORM_TYPE_GUID);

	XSetXformType(X2DXF_SCALE);

	RECT rcTopLeft;
	GetClientRect(g_hTopLeftWnd, &rcTopLeft);

	g_pEngine->getCore()->getConsole()->execCommand2("r_win_width %d\nr_win_height %d", rcTopLeft.right - rcTopLeft.left, rcTopLeft.bottom - rcTopLeft.top);

	IGXDevice *pDevice = g_pRender->getDevice();

	IPluginManager *pPluginManager = Core_GetIXCore()->getPluginManager();

	IXMaterialSystem *pMaterialSystem = (IXMaterialSystem*)pPluginManager->getInterface(IXMATERIALSYSTEM_GUID);

	pMaterialSystem->scanMaterials();

	g_matBrowserCallback.init(pMaterialSystem);

	GXCOLOR w = GX_COLOR_ARGB(255, 255, 255, 255);
	GXCOLOR t = GX_COLOR_ARGB(0, 255, 255, 255);
	GXCOLOR colorData[] = {
		t, t, t, w, w, w,
		w, t, t, t, w, w,
		w, w, t, t, t, w,
		w, w, w, t, t, t,
		t, w, w, w, t, t,
		t, t, w, w, w, t
	};
	g_pDashedMaterial = pDevice->createTexture2D(6, 6, 1, 0, GXFMT_A8B8G8R8, colorData);
	//pMaterialSystem->addTexture("dev_dashed", pDashedMaterial);
	//mem_release(pDashedMaterial);

	//GXCOLOR tmpColor = GX_COLOR_ARGB(255, 255, 255, 255);
	//IGXTexture2D* pWhiteMaterial = SGCore_GetDXDevice()->createTexture2D(1, 1, 1, 0, GXFMT_A8R8G8B8, &tmpColor);
	////pMaterialSystem->addTexture("dev_white", pWhiteMaterial);
	//mem_release(pDashedMaterial);


	IXGizmoRenderer *pAxesRenderer = NULL;
	pUtils->newGizmoRenderer(&pAxesRenderer);
	pAxesRenderer->setColor(float4(1.0f, 0.0f, 0.0f, 1.0f));
	pAxesRenderer->setLineWidth(0.04f);
	pAxesRenderer->setLineWidth(3.0f);
	pAxesRenderer->jumpTo(float3(0.0f, 0.0f, 0.0f));
	pAxesRenderer->lineTo(float3(1.0f, 0.0f, 0.0f));
	pAxesRenderer->setColor(float4(0.0f, 1.0f, 0.0f, 1.0f));
	pAxesRenderer->jumpTo(float3(0.0f, 0.0f, 0.0f));
	pAxesRenderer->lineTo(float3(0.0f, 1.0f, 0.0f));
	pAxesRenderer->setColor(float4(0.0f, 0.0f, 1.0f, 1.0f));
	pAxesRenderer->jumpTo(float3(0.0f, 0.0f, 0.0f));
	pAxesRenderer->lineTo(float3(0.0f, 0.0f, 1.0f));
	p3DOverlayGraphNode->setAxesRenderer(pAxesRenderer);
	p2DGraphNode->setAxesRenderer(pAxesRenderer);
	mem_release(pAxesRenderer);

	UINT ic = 0;
	IXEditable *pEditable;
	IXEditorTool *pTool;
	IXEditorResourceBrowser *pResourceBrowser;
	while((pEditable = (IXEditable*)pPluginManager->getInterface(IXEDITABLE_GUID, ic++)))
	{
		if(pEditable->getVersion() == IXEDITABLE_VERSION)
		{
			g_pEditableSystems.push_back(pEditable);
			pEditable->startup(pDevice);
			
			IXEditorExtension *pExt = pEditable->getEditorExtension();
			if(pExt)
			{
				g_apExtensions.push_back(pExt);

				for(UINT i = 0, l = pExt->getPropertyTabCount(); i < l; ++i)
				{
					g_pPropWindow->addCustomTab(pExt->getPropertyTab(i));
				}

				for(UINT i = 0, l = pExt->getToolCount(); i < l; ++i)
				{
					if(pExt->getTool(i, &pTool))
					{
						XInitTool(pTool, pEditable);
					}
				}

				for(UINT i = 0, l = pExt->getResourceBrowserCount(); i < l; ++i)
				{
					if(pExt->getResourceBrowser(i, &pResourceBrowser))
					{
						g_pEditor->registerResourceBrowser(pResourceBrowser);
						mem_release(pResourceBrowser);
						// XInitTool(pResourceBrowser, pEditable);
			}
				}
			}

			g_mEditableSystems[AAString(pEditable->getName())] = pEditable;
		}
	}

	HMENU hImportersMenu = NULL;
	IXEditorImporter *pImporter;
	ic = 0;
	while((pImporter = (IXEditorImporter*)pPluginManager->getInterface(IXEDITORIMPORTER_GUID, ic++)))
	{
		if(pImporter->getVersion() == IXEDITORIMPORTER_VERSION)
		{
			pImporter->startup(g_pEditor);

			if(!hImportersMenu)
			{
				hImportersMenu = CreatePopupMenu();
			}

			AppendMenuW(hImportersMenu, MF_STRING, IDC_FILE_IMPORT_FIRST + g_pEditorImporters.size(), CMB2WC(pImporter->getName()));

			g_pEditorImporters.push_back(pImporter);
		}
	}
	if(hImportersMenu)
	{
		HMENU hFileMenu = GetSubMenu(GetMenu(g_hWndMain), 0);
		MENUITEMINFOW mii = {sizeof(mii)};
		mii.fMask = MIIM_SUBMENU | MIIM_STATE;
		mii.fState = MFS_ENABLED;
		mii.hSubMenu = hImportersMenu;

		SetMenuItemInfoW(hFileMenu, ID_FILE_IMPORT, FALSE, &mii);
	}

	fora(i, g_apExtensions)
	{
		UINT uMenus = g_apExtensions[i]->getMenuCount();
		for(UINT j = 0; j < uMenus; ++j)
		{
			const XEditorMenuItem *pMenuItem;
			if((pMenuItem = g_apExtensions[i]->getMenu(j)))
			{
				HMENU hSubMenu = BuildExtMenu(pMenuItem);

				if(hSubMenu)
				{
					InsertMenuW(GetMenu(g_hWndMain), GetMenuItemCount(GetMenu(g_hWndMain)) - 1, MF_STRING | MF_POPUP | MF_BYPOSITION, (UINT_PTR)hSubMenu, CMB2WC(pMenuItem->szText));
				}
			}
		}
	}

	SetForegroundWindow(g_hWndMain);
	XInitCustomAccel();

	XInitTypesCombo();
	
	Core_GetIXCore()->getEventChannel<XEventLevel>(EVENT_LEVEL_GUID)->addListener([](const XEventLevel *pData)
	{
		switch(pData->type)
		{
		case XEventLevel::TYPE_LOAD:
			g_sLevelName = pData->szLevelName;
			XUpdateWindowTitle();
			{
				char szPathLevel[1024], szKey[64];
				sprintf(szPathLevel, "levels/%s/%s.lvl", pData->szLevelName, pData->szLevelName);
				if(Core_GetIXCore()->getFileSystem()->resolvePath(szPathLevel, szPathLevel, sizeof(szPathLevel)))
				{

					ISXConfig *pCfg = Core_OpConfig(szPathLevel);
					const char *szVal;

					szVal = pCfg->getKey("terrax", "vp_layout");
					if(szVal)
					{
						int iVal = 0;
						if(sscanf(szVal, "%d", &iVal) && iVal >= 0 && iVal <= 3)
						{
							XInitViewportLayout((X_VIEWPORT_LAYOUT)iVal);
						}
					}

					szVal = pCfg->getKey("terrax", "grid_step");
					if(szVal)
					{
						int iVal = 0;
						if(sscanf(szVal, "%d", &iVal) && iVal >= GRID_STEP_MINIMAL && iVal <= GRID_STEP_MAXIMAL)
						{
							g_xConfig.m_gridStep = (GRID_STEP)iVal;
						}
					}

					szVal = pCfg->getKey("terrax", "grid_show");
					if(szVal)
					{
						int iVal = 0;
						if(sscanf(szVal, "%d", &iVal))
						{
							g_xConfig.m_bShowGrid = iVal != 0;
						}
					}

					for(UINT i = 0; i < 4; ++i)
					{
						float3 vec;
						sprintf_s(szKey, "cam%u_pos", i);
						szVal = pCfg->getKey("terrax", szKey);
						if(szVal)
						{
							if(sscanf(szVal, "%f %f %f", &vec.x, &vec.y, &vec.z) == 3)
							{
								g_xConfig.m_pViewportCamera[i]->setPosition(vec);
							}
						}

						SMQuaternion q;
						sprintf_s(szKey, "cam%u_rot", i);
						szVal = pCfg->getKey("terrax", szKey);
						if(szVal)
						{
							if(sscanf(szVal, "%f %f %f %f", &q.x, &q.y, &q.z, &q.w) == 4)
							{
								g_xConfig.m_pViewportCamera[i]->setOrientation(q);
							}
						}

						sprintf_s(szKey, "cam%u_scale", i);
						szVal = pCfg->getKey("terrax", szKey);
						if(szVal)
						{
							float fVal = 0.0;
							if(sscanf(szVal, "%f", &fVal) && fVal > 0.0f)
							{
								g_xConfig.m_fViewportScale[i] = fVal;
								g_xConfig.m_pViewportCamera[i]->setScale(fVal);
							}
						}

						sprintf_s(szKey, "cam%u_view", i);
						szVal = pCfg->getKey("terrax", szKey);
						if(szVal)
						{
							int iVal = 0;
							if(sscanf(szVal, "%d", &iVal) && iVal >= -1 && iVal <= 2)
							{
								g_bViewportCaptionDirty[i] = true;
								g_xConfig.m_x2DView[i] = (X_2D_VIEW)iVal;
								if(g_xConfig.m_x2DView[i] == X2D_NONE)
								{
									g_pEngineCallback->setCameraRotation(g_xConfig.m_pViewportCamera[i]->getRotation());
								}
							}
						}
					}

					mem_release(pCfg);
				}
			}
			break;
		case XEventLevel::TYPE_UNLOAD:

			SendMessage(g_hWndMain, WM_COMMAND, MAKEWPARAM(ID_EDIT_CLEARSELECTION, 0), (LPARAM)0);

			g_sLevelName = "";

			fora(i, g_pLevelObjects)
			{
				mem_release(g_pLevelObjects[i]);
			}
			g_pLevelObjects.clear();

			fora(i, g_apProxies)
			{
				mem_release(g_apProxies[i]);
			}
			g_apProxies.clear();

			fora(i, g_apGroups)
			{
				mem_release(g_apGroups[i]);
			}
			g_apGroups.clear();
			
			for(Map<XGUID, IXEditorModel*>::Iterator i = g_apLevelModels.begin(); i; ++i)
			{
				mem_release(*(i.second))
			}
			g_apLevelModels.clear();

			SAFE_CALL(g_pUndoManager, reset);

			g_mObjectsLocation.clear();

			XUpdateWindowTitle();

			g_pEditor->onObjectsetChanged();
			break;

		case XEventLevel::TYPE_SAVE:
			g_sLevelName = pData->szLevelName;
			g_pUndoManager->makeClean();
			{
				char szPathLevel[1024], szKey[64], szVal[1024];
				sprintf(szPathLevel, "%s%s/%s.lvl", Core_RStringGet(G_RI_STRING_PATH_GS_LEVELS), pData->szLevelName, pData->szLevelName);

				IFileSystem *pFS = Core_GetIXCore()->getFileSystem();

				ISXConfig *pCfg = Core_OpConfig(szPathLevel);
				
				if(!pCfg->keyExists("level", "type"))
				{
					pCfg->set("level", "type", "indoor");
				}
				if(!pCfg->keyExists("level", "local_name"))
				{
					pCfg->set("level", "local_name", pData->szLevelName);
				}

				for(UINT i = 0; i < 4; ++i)
				{
					float3 vec;
					vec = g_xConfig.m_pViewportCamera[i]->getPosition();
					sprintf_s(szVal, "%f %f %f", vec.x, vec.y, vec.z);
					sprintf_s(szKey, "cam%u_pos", i);
					pCfg->set("terrax", szKey, szVal);

					SMQuaternion q = g_xConfig.m_pViewportCamera[i]->getOrientation();
					sprintf_s(szVal, "%f %f %f %f", q.x, q.y, q.z, q.w);
					sprintf_s(szKey, "cam%u_rot", i);
					pCfg->set("terrax", szKey, szVal);

					sprintf_s(szVal, "%f", g_xConfig.m_fViewportScale[i]);
					sprintf_s(szKey, "cam%u_scale", i);
					pCfg->set("terrax", szKey, szVal);

					sprintf_s(szVal, "%d", g_xConfig.m_x2DView[i]);
					sprintf_s(szKey, "cam%u_view", i);
					pCfg->set("terrax", szKey, szVal);
				}

				sprintf_s(szVal, "%d", g_xConfig.m_xViewportLayout);
				pCfg->set("terrax", "vp_layout", szVal);

				sprintf_s(szVal, "%d", g_xConfig.m_gridStep);
				pCfg->set("terrax", "grid_step", szVal);

				sprintf_s(szVal, "%d", g_xConfig.m_bShowGrid ? 1 : 0);
				pCfg->set("terrax", "grid_show", szVal);

				pCfg->save();
				mem_release(pCfg);


				// save groups
				sprintf(szPathLevel, "levels/%s/editor/groups.json", pData->szLevelName);

				IFile *pFile = pFS->openFile(szPathLevel, FILE_MODE_WRITE);
				if(!pFile)
				{
					LibReport(REPORT_MSG_LEVEL_ERROR, "Unable to save data '%s'\n", szPathLevel);
				}
				else
				{
					pFile->writeText("[\n");

					bool isFirst = true;
					char tmp[64];
					fora(i, g_apGroups)
					{
						CGroupObject *pGroup = g_apGroups[i];

						pFile->writeText("\t%s{\n", isFirst ? "" : ",");
						isFirst = false;

						XGUIDToSting(*pGroup->getGUID(), tmp, sizeof(tmp));
						pFile->writeText("\t\t\"guid\": \"%s\"\n", tmp);
						pFile->writeText("\t\t,\"name\": \"%s\"\n", pGroup->getKV("name"));

						pFile->writeText("\t\t,\"o\": [\n", tmp);

						for(UINT j = 0, jl = pGroup->getObjectCount(); j < jl; ++j)
						{
							XGUIDToSting(*pGroup->getObject(j)->getGUID(), tmp, sizeof(tmp));
							pFile->writeText("\t\t\t%s\"%s\"\n", j == 0 ? "" : ",", tmp);
						}

						pFile->writeText("\t\t]\n", tmp);

						pFile->writeText("\t}\n");
					}

					pFile->writeText("]\n");

					mem_release(pFile);
				}


				// save proxies
				sprintf(szPathLevel, "levels/%s/editor/proxies.json", pData->szLevelName);
				
				pFile = pFS->openFile(szPathLevel, FILE_MODE_WRITE);
				if(!pFile)
				{
					LibReport(REPORT_MSG_LEVEL_ERROR, "Unable to save data '%s'\n", szPathLevel);
				}
				else
				{
					pFile->writeText("[\n");

					sprintf(szPathLevel, "levels/%s/models", pData->szLevelName);
					pFS->deleteDirectory(szPathLevel);
					//IFileIterator *pIter = pFS->getFileList(szPathLevel, "dse");
					//const char *szFile;
					//while((szFile = pIter->next()))
					//{
					//	//printf("%s\n", szFile);
					//	pFS->deleteFile(szFile);
					//}

					bool isFirst = true;
					char tmp[64];
					fora(i, g_apProxies)
					{
						CProxyObject *pProxy = g_apProxies[i];
						if(pProxy->isRemoved())
						{
							continue;
						}
						pFile->writeText("\t%s{\n", isFirst ? "" : ",");
						isFirst = false;

						XGUIDToSting(*pProxy->getGUID(), tmp, sizeof(tmp));
						pFile->writeText("\t\t\"guid\": \"%s\"\n", tmp);
						XGUIDToSting(*pProxy->getTargetObject()->getGUID(), tmp, sizeof(tmp));
						pFile->writeText("\t\t,\"t\": \"%s\"\n", tmp);
						pFile->writeText("\t\t,\"s\": [\n", tmp);

						// TODO don't save empty models!

						for(UINT j = 0, jl = pProxy->getModelCount(); j < jl; ++j)
						{
							XGUIDToSting(*pProxy->getModel(j)->getGUID(), tmp, sizeof(tmp));
							pFile->writeText("\t\t\t%s\"%s\"\n", j == 0 ? "" : ",", tmp);
						}

						pFile->writeText("\t\t]\n\t\t,\"o\": [\n", tmp);

						for(UINT j = 0, jl = pProxy->getObjectCount(); j < jl; ++j)
						{
							XGUIDToSting(*pProxy->getObject(j)->getGUID(), tmp, sizeof(tmp));
							pFile->writeText("\t\t\t%s\"%s\"\n", j == 0 ? "" : ",", tmp);
						}

						pFile->writeText("\t\t]\n", tmp);

						pFile->writeText("\t}\n");

						pProxy->saveModel();
					}

					pFile->writeText("]\n");

					mem_release(pFile);
				}
			}
			break;

		case XEventLevel::TYPE_LOAD_BEGIN:
			EnableWindow(g_hWndMain, FALSE);
			break;

		case XEventLevel::TYPE_LOAD_END:
			fora(ic, g_pEditableSystems)
			{
				IXEditable *pEditable = g_pEditableSystems[ic];
				for(ID i = 0, l = pEditable->getObjectCount(); i < l; ++i)
				{
					g_pLevelObjects.push_back(pEditable->getObject(i));
				}

				for(ID i = 0, l = pEditable->getModelCount(); i < l; ++i)
				{
					IXEditorModel *pMdl;
					if(pEditable->getModel(i, &pMdl))
					{
						g_apLevelModels[*pMdl->getGUID()] = pMdl;
					}
				}
			}

			char szFile[1024];
			sprintf(szFile, "levels/%s/editor/proxies.json", pData->szLevelName);

			IFile *pFile = Core_GetIXCore()->getFileSystem()->openFile(szFile, FILE_MODE_READ);
			if(pFile)
			{
				size_t sizeFile = pFile->getSize();
				char *szJSON = new char[sizeFile + 1];
				pFile->readBin(szJSON, sizeFile);
				szJSON[sizeFile] = 0;

				bool isLoaded = false;

				IXJSON *pJSON = (IXJSON*)Core_GetIXCore()->getPluginManager()->getInterface(IXJSON_GUID);
				IXJSONItem *pRoot;
				if(pJSON->parse(szJSON, &pRoot))
				{
					IXJSONArray *pArr = pRoot->asArray();
					if(pArr)
					{
						for(UINT j = 0, jl = pArr->size(); j < jl; ++j)
						{
							IXJSONObject *pProxyObj = pArr->at(j)->asObject();
							if(pProxyObj)
							{
								// guid, t, s

								const char *szGUID = NULL;
								XGUID guid, guidTarget;
								IXJSONItem *pGuidItem = pProxyObj->getItem("guid");
								if(!pGuidItem || !(szGUID = pGuidItem->getString()) || !XGUIDFromString(&guid, szGUID))
								{
									LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid proxy '%u' in '%s'. Invalid GUID\n", j, szFile);
									continue;
								}

								pGuidItem = pProxyObj->getItem("t");
								if(!pGuidItem || !(szGUID = pGuidItem->getString()) || !XGUIDFromString(&guidTarget, szGUID))
								{
									LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid proxy '%u' in '%s'. Invalid target GUID\n", j, szFile);
									continue;
								}

								IXJSONItem *pSItem = pProxyObj->getItem("s");
								IXJSONArray *pSArr = NULL;
								if(!pSItem || !(pSArr = pSItem->asArray()))
								{
									LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid proxy '%u' in '%s'. Missing 's' key\n", j, szFile);
									continue;
								}

								if(!pSArr->size())
								{
									LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid proxy '%u' in '%s'. No child models\n", j, szFile);
									continue;
								}

								CProxyObject *pProxy = new CProxyObject(guid);
								pProxy->setDstObject(guidTarget);

								for(UINT k = 0, kl = pSArr->size(); k < kl; ++k)
								{
									szGUID = pSArr->at(k)->getString();
									if(!szGUID || !XGUIDFromString(&guid, szGUID))
									{
										LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid model '%u' guid in proxy '%u' in '%s'. '%s'\n", k, j, szFile, szGUID ? szGUID : "");
										continue;
									}

									pProxy->addSrcModel(guid);
								}

								g_apProxies.push_back(pProxy);

								//pProxy->build();

								add_ref(pProxy);
								g_pLevelObjects.push_back(pProxy);

								isLoaded = true;
							}
						}
					}

					mem_release(pRoot);
				}

				mem_delete_a(szJSON);
				mem_release(pFile);

				if(!isLoaded)
				{
					LibReport(REPORT_MSG_LEVEL_WARNING, "Unable to load '%s'\n", szFile);
				}
			}

			sprintf(szFile, "levels/%s/editor/groups.json", pData->szLevelName);

			pFile = Core_GetIXCore()->getFileSystem()->openFile(szFile, FILE_MODE_READ);
			if(pFile)
			{
				size_t sizeFile = pFile->getSize();
				char *szJSON = new char[sizeFile + 1];
				pFile->readBin(szJSON, sizeFile);
				szJSON[sizeFile] = 0;

				bool isLoaded = false;

				IXJSON *pJSON = (IXJSON*)Core_GetIXCore()->getPluginManager()->getInterface(IXJSON_GUID);
				IXJSONItem *pRoot;
				if(pJSON->parse(szJSON, &pRoot))
				{
					IXJSONArray *pArr = pRoot->asArray();
					if(pArr)
					{
						Array<CGroupObject*> aGroups(pArr->size());
						const char *szGUID = NULL;
						const char *szName = NULL;
						XGUID guid;

						for(UINT j = 0, jl = pArr->size(); j < jl; ++j)
						{
							aGroups[j] = NULL;

							IXJSONObject *pGroupObj = pArr->at(j)->asObject();
							if(pGroupObj)
							{
								IXJSONItem *pGuidItem = pGroupObj->getItem("guid");
								if(!pGuidItem || !(szGUID = pGuidItem->getString()) || !XGUIDFromString(&guid, szGUID))
								{
									LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid group '%u' in '%s'. Invalid GUID\n", j, szFile);
									continue;
								}

								IXJSONItem *pNameItem = pGroupObj->getItem("name");
								if(!pNameItem || !(szName = pNameItem->getString()))
								{
									LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid group '%u' in '%s'. Invalid name\n", j, szFile);
									continue;
								}

								IXJSONItem *pOItem = pGroupObj->getItem("o");
								IXJSONArray *pOArr = NULL;
								if(!pOItem || !(pOArr = pOItem->asArray()))
								{
									LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid group '%u' in '%s'. Missing 'o' key\n", j, szFile);
									continue;
								}

								if(!pOArr->size())
								{
									LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid group '%u' in '%s'. No child objects\n", j, szFile);
									continue;
								}

								CGroupObject *pGroup = new CGroupObject(guid);
								pGroup->setKV("name", szName);
								
								g_apGroups.push_back(pGroup);

								add_ref(pGroup);
								g_pLevelObjects.push_back(pGroup);

								aGroups[j] = pGroup;
							}
						}

						for(UINT j = 0, jl = pArr->size(); j < jl; ++j)
						{
							CGroupObject *pGroup = aGroups[j];
							if(pGroup)
							{								
								IXJSONArray *pOArr = pArr->at(j)->asObject()->getItem("o")->asArray();

								for(UINT k = 0, kl = pOArr->size(); k < kl; ++k)
								{
									szGUID = pOArr->at(k)->getString();
									if(!szGUID || !XGUIDFromString(&guid, szGUID))
									{
										LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid object '%u' guid in group '%u' in '%s'. '%s'\n", k, j, szFile, szGUID ? szGUID : "");
										continue;
									}

									IXEditorObject *pObj = XFindObjectByGUID(guid);
									if(!pObj)
									{
										LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid object '%u' in group '%u' in '%s'. '%s'. No object with GUID found.\n", k, j, szFile, szGUID ? szGUID : "");
										continue;
									}
									pGroup->addChildObject(pObj);

									isLoaded = true;
								}
							}
						}
					}

					mem_release(pRoot);
				}

				mem_delete_a(szJSON);
				mem_release(pFile);

				if(!isLoaded)
				{
					LibReport(REPORT_MSG_LEVEL_WARNING, "Unable to load '%s'\n", szFile);
				}
			}

			g_pEditor->onObjectsetChanged();

			EnableWindow(g_hWndMain, TRUE);

			// g_apLevelModels
			break;
		}
	});
	
	Core_SetOutPtr();

	//SGCore_SkyBoxLoadTex("sky_2_cube.dds");
	//SGCore_SkyBoxLoadTex("sky_hdr.dds");
	//SGCore_SkyBoxLoadTex("sky_test_cube.dds");
	//SGCore_SkyCloudsLoadTex("sky_oblaka.dds");
	//SGCore_SkyBoxSetUse(false);
	//SGCore_SkyCloudsSetUse(false);

	static const float *r_default_fov = GET_PCVAR_FLOAT("r_default_fov");

	IXRender *pRender = g_pRender;

	pRender->newCamera(&g_xConfig.m_pViewportCamera[XWP_TOP_LEFT]);
	g_xConfig.m_pViewportCamera[XWP_TOP_LEFT]->setFOV(SMToRadian(*r_default_fov));

	pRender->newCamera(&g_xConfig.m_pViewportCamera[XWP_TOP_RIGHT]);
	g_xConfig.m_pViewportCamera[XWP_TOP_RIGHT]->setPosition(X2D_TOP_POS);
	g_xConfig.m_pViewportCamera[XWP_TOP_RIGHT]->setOrientation(X2D_TOP_ROT);
	g_xConfig.m_pViewportCamera[XWP_TOP_RIGHT]->setProjectionMode(XCPM_ORTHOGONAL);
	g_xConfig.m_pViewportCamera[XWP_TOP_RIGHT]->setFar(2000.0f);
	g_xConfig.m_pViewportCamera[XWP_TOP_RIGHT]->setNear(1.0f);

	pRender->newCamera(&g_xConfig.m_pViewportCamera[XWP_BOTTOM_LEFT]);
	g_xConfig.m_pViewportCamera[XWP_BOTTOM_LEFT]->setPosition(X2D_SIDE_POS);
	g_xConfig.m_pViewportCamera[XWP_BOTTOM_LEFT]->setOrientation(X2D_SIDE_ROT);
	g_xConfig.m_pViewportCamera[XWP_BOTTOM_LEFT]->setProjectionMode(XCPM_ORTHOGONAL);
	g_xConfig.m_pViewportCamera[XWP_BOTTOM_LEFT]->setFar(2000.0f);
	g_xConfig.m_pViewportCamera[XWP_BOTTOM_LEFT]->setNear(1.0f);

	pRender->newCamera(&g_xConfig.m_pViewportCamera[XWP_BOTTOM_RIGHT]);
	g_xConfig.m_pViewportCamera[XWP_BOTTOM_RIGHT]->setPosition(X2D_FRONT_POS);
	g_xConfig.m_pViewportCamera[XWP_BOTTOM_RIGHT]->setOrientation(X2D_FRONT_ROT);
	g_xConfig.m_pViewportCamera[XWP_BOTTOM_RIGHT]->setProjectionMode(XCPM_ORTHOGONAL);
	g_xConfig.m_pViewportCamera[XWP_BOTTOM_RIGHT]->setFar(2000.0f);
	g_xConfig.m_pViewportCamera[XWP_BOTTOM_RIGHT]->setNear(1.0f);

	g_pTopLeftTarget->setCamera(g_xConfig.m_pViewportCamera[XWP_TOP_LEFT]);
	g_pTopRightTarget->setCamera(g_xConfig.m_pViewportCamera[XWP_TOP_RIGHT]);
	g_pBottomLeftTarget->setCamera(g_xConfig.m_pViewportCamera[XWP_BOTTOM_LEFT]);
	g_pBottomRightTarget->setCamera(g_xConfig.m_pViewportCamera[XWP_BOTTOM_RIGHT]);

//	SkyXEngine_RunGenPreview();
	//Core_0SetCVarInt("r_final_image", DS_RT_COLOR);
	Core_0SetCVarInt("r_final_image", DS_RT_SCENELIGHT);
	
	g_pGrid = new CGrid(pRender);
	g_pGrid->setOpacity(0.7f);
	
	g_xRenderStates.idColoredShaderVS = g_pRender->loadShader(SHADER_TYPE_VERTEX, "terrax_colored.vs");
	g_xRenderStates.idColoredShaderPS = g_pRender->loadShader(SHADER_TYPE_PIXEL, "terrax_colored.ps");
	g_xRenderStates.idColoredShaderKit = g_pRender->createShaderKit(g_xRenderStates.idColoredShaderVS, g_xRenderStates.idColoredShaderPS);
//	SkyXEngine_PreviewKill();

	GXBlendDesc blendDesc;
	blendDesc.renderTarget[0].useBlend = true;
	blendDesc.renderTarget[0].blendSrcColor = blendDesc.renderTarget[0].blendSrcAlpha = GXBLEND_BLEND_FACTOR;
	blendDesc.renderTarget[0].blendDestColor = blendDesc.renderTarget[0].blendDestAlpha = GXBLEND_INV_BLEND_FACTOR;
	g_xRenderStates.pBlendColorFactor = pDevice->createBlendState(&blendDesc);

	blendDesc.renderTarget[0].blendSrcColor = blendDesc.renderTarget[0].blendSrcAlpha = GXBLEND_SRC_ALPHA;
	blendDesc.renderTarget[0].blendDestColor = blendDesc.renderTarget[0].blendDestAlpha = GXBLEND_INV_SRC_ALPHA;
	g_xRenderStates.pBlendAlpha = pDevice->createBlendState(&blendDesc);

	GXRasterizerDesc rsDesc; 
	rsDesc.cullMode = GXCULL_NONE;
	g_xRenderStates.pRSSolidNoCull = pDevice->createRasterizerState(&rsDesc);

	rsDesc.fillMode = GXFILL_WIREFRAME;
	g_xRenderStates.pRSWireframe = pDevice->createRasterizerState(&rsDesc);

	g_xRenderStates.idTexturedShaderVS = g_pRender->loadShader(SHADER_TYPE_VERTEX, "terrax_textured.vs");
	g_xRenderStates.idTexturedShaderPS = g_pRender->loadShader(SHADER_TYPE_PIXEL, "terrax_textured.ps");
	g_xRenderStates.idTexturedShaderKit = g_pRender->createShaderKit(g_xRenderStates.idTexturedShaderVS, g_xRenderStates.idTexturedShaderPS);

	GXVertexElement oLayout[] =
	{
		{0, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_POSITION},
		{0, 12, GXDECLTYPE_FLOAT2, GXDECLUSAGE_TEXCOORD},
		GX_DECL_END()
	};
	IGXVertexDeclaration *pVD = pDevice->createVertexDeclaration(oLayout);
	g_pBorderVertexBuffer = pDevice->createVertexBuffer(sizeof(XBorderVertex) * 5, GXBUFFER_USAGE_STREAM);
	g_pBorderRenderBuffer = pDevice->createRenderBuffer(1, &g_pBorderVertexBuffer, pVD);
	mem_release(pVD);

	// Object handlers
	GXVertexElement oLayoutHandler[] =
	{
		{0, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_POSITION, GXDECLSPEC_PER_VERTEX_DATA},
		{1, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_TEXCOORD, GXDECLSPEC_PER_INSTANCE_DATA},
		GX_DECL_END()
	};
	pVD = pDevice->createVertexDeclaration(oLayoutHandler);
	g_xRenderStates.pHandlerVB = pDevice->createVertexBuffer(sizeof(float3_t) * 8, GXBUFFER_USAGE_STREAM);
	g_xRenderStates.pHandlerInstanceVB = pDevice->createVertexBuffer(sizeof(float3_t) * 2 * X_MAX_HANDLERS_PER_DIP, GXBUFFER_USAGE_STREAM);
	IGXVertexBuffer *ppVB[] = {g_xRenderStates.pHandlerVB, g_xRenderStates.pHandlerInstanceVB};
	g_xRenderStates.pHandlerRB = pDevice->createRenderBuffer(2, ppVB, pVD);
	g_xRenderStates.idHandlerShaderVS = g_pRender->loadShader(SHADER_TYPE_VERTEX, "terrax_handler.vs");
	g_xRenderStates.idHandlerShaderKit = g_pRender->createShaderKit(g_xRenderStates.idHandlerShaderVS, g_xRenderStates.idColoredShaderPS);
	g_xRenderStates.idBoundShaderKit = g_pRender->createShaderKit(g_pRender->loadShader(SHADER_TYPE_VERTEX, "terrax_bound.vs"), g_xRenderStates.idColoredShaderPS);
	USHORT pHandlerIndices[] = {0, 1, 2, 3, 4, 5, 6, 7};
	g_xRenderStates.pHandlerIB = pDevice->createIndexBuffer(sizeof(USHORT)* 8, GXBUFFER_USAGE_STATIC, GXIT_UINT16, pHandlerIndices);

	g_xRenderStates.idIconShaderKit = g_pRender->createShaderKit(g_pRender->loadShader(SHADER_TYPE_VERTEX, "terrax_icon.vs"), g_xRenderStates.idTexturedShaderPS);

	float3_t cubeData[] = {
		{-0.5f, -0.5f, -0.5f},
		{-0.5f, -0.5f, 0.5f},
		{-0.5f, 0.5f, -0.5f},
		{-0.5f, 0.5f, 0.5f},
		{0.5f, -0.5f, -0.5f},
		{0.5f, -0.5f, 0.5f},
		{0.5f, 0.5f, -0.5f},
		{0.5f, 0.5f, 0.5f}
	};
	// for(UINT i = 0; i < 8; ++i)
	// {
	// 	cubeData[i] = (float3)(cubeData[i] * 0.16f);
	// }

	GXVertexElement oLayoutHandler3D[] =
	{
		{0, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_POSITION, GXDECLSPEC_PER_VERTEX_DATA},
		{1, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_TEXCOORD, GXDECLSPEC_PER_INSTANCE_DATA},
		{1, 12, GXDECLTYPE_FLOAT3, GXDECLUSAGE_TEXCOORD2, GXDECLSPEC_PER_INSTANCE_DATA},
		GX_DECL_END()
	};

	mem_release(pVD);
	pVD = pDevice->createVertexDeclaration(oLayoutHandler3D);
	IGXVertexBuffer *pHandler3DVB = pDevice->createVertexBuffer(sizeof(float3_t) * 8, GXBUFFER_USAGE_STATIC, cubeData);
	IGXVertexBuffer *ppVB3D[] = {pHandler3DVB, g_xRenderStates.pHandlerInstanceVB};
	g_xRenderStates.pHandler3DRB = pDevice->createRenderBuffer(2, ppVB3D, pVD);
	mem_release(pVD);
	mem_release(pHandler3DVB);

	USHORT pHandler3DIndices[] = {0, 1, 0, 2, 0, 4, 6, 4, 6, 2, 6, 7, 5, 4, 5, 7, 5, 1, 3, 7, 3, 1, 3, 2};
	g_xRenderStates.pHandler3DIB = pDevice->createIndexBuffer(sizeof(USHORT) * 24, GXBUFFER_USAGE_STATIC, GXIT_UINT16, pHandler3DIndices);

	// Object icons
	GXVertexElement oLayoutIcon[] =
	{
		{0, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_POSITION, GXDECLSPEC_PER_VERTEX_DATA},
		{0, 12, GXDECLTYPE_FLOAT2, GXDECLUSAGE_TEXCOORD, GXDECLSPEC_PER_VERTEX_DATA},
		{1, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_TEXCOORD1, GXDECLSPEC_PER_INSTANCE_DATA},
		GX_DECL_END()
	};
	pVD = pDevice->createVertexDeclaration(oLayoutIcon);
	g_xRenderStates.pIconVB = pDevice->createVertexBuffer((sizeof(float3_t) + sizeof(float2_t)) * 4, GXBUFFER_USAGE_STREAM);
	IGXVertexBuffer *ppIconVB3D[] = {g_xRenderStates.pIconVB, g_xRenderStates.pHandlerInstanceVB};
	g_xRenderStates.pIcon3DRB = pDevice->createRenderBuffer(2, ppIconVB3D, pVD);
	mem_release(pVD);

	USHORT pIcon3DIndices[] = {0, 1, 2, 0, 2, 3};
	g_xRenderStates.pIcon3DIB = pDevice->createIndexBuffer(sizeof(USHORT) * 6, GXBUFFER_USAGE_STATIC, GXIT_UINT16, pIcon3DIndices);

	GXSamplerDesc samplerDesc;
	samplerDesc.addressU = samplerDesc.addressV = samplerDesc.addressW = GXTEXTURE_ADDRESS_CLAMP;
	samplerDesc.filter = GXFILTER_MIN_MAG_MIP_LINEAR;
	g_xRenderStates.pSamplerLinearClamp = pDevice->createSamplerState(&samplerDesc);

	{
		// Transform handlers
		GXVertexElement oLayoutHandler[] =
		{
			{0, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_POSITION, GXDECLSPEC_PER_VERTEX_DATA},
			GX_DECL_END()
		};
		pVD = pDevice->createVertexDeclaration(oLayoutHandler);
		g_xRenderStates.pTransformHandlerVB = pDevice->createVertexBuffer(sizeof(float3_t)* 32, GXBUFFER_USAGE_STREAM);
		g_xRenderStates.pTransformHandlerRB = pDevice->createRenderBuffer(1, &g_xRenderStates.pTransformHandlerVB, pVD);
		USHORT pHandlerIndices[] = {
			0, 1, 2, 0, 2, 3,
			4, 5, 6, 4, 6, 7,
			8, 9, 10, 8, 10, 11,
			12, 13, 14, 12, 14, 15,

			16, 17, 18, 16, 18, 19,
			20, 21, 22, 20, 22, 23,
			24, 25, 26, 24, 26, 27,
			28, 29, 30, 28, 30, 31
		};
		g_xRenderStates.pTransformHandlerScaleIB = pDevice->createIndexBuffer(sizeof(USHORT)* 48, GXBUFFER_USAGE_STATIC, GXIT_UINT16, pHandlerIndices);
		
		USHORT pHandlerRotateIndices[] = {
			0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 6, 0, 6, 7,
			8, 9, 10, 8, 10, 11, 8, 11, 12, 8, 12, 13, 8, 13, 14, 8, 14, 15,
			16, 17, 18, 16, 18, 19, 16, 19, 20, 16, 20, 21, 16, 21, 22, 16, 22, 23,
			24, 25, 26, 24, 26, 27, 24, 27, 28, 24, 28, 29, 24, 29, 30, 24, 30, 31
		};
		g_xRenderStates.pTransformHandlerRotateIB = pDevice->createIndexBuffer(sizeof(USHORT)* 72, GXBUFFER_USAGE_STATIC, GXIT_UINT16, pHandlerRotateIndices);

		float3_t pVertices[] = {
			float3_t(10000.0f, 0.0f, 0.0f), float3_t(-10000.0f, 0.0f, 0.0f),
			float3_t(0.0f, 10000.0f, 0.0f), float3_t(0.0f, -10000.0f, 0.0f),
			float3_t(0.0f, 0.0f, 10000.0f), float3_t(0.0f, 0.0f, -10000.0f)
		};
		IGXVertexBuffer *pCreateCrossVB = pDevice->createVertexBuffer(sizeof(float3_t) * 6, GXBUFFER_USAGE_STATIC, pVertices);
		g_xRenderStates.pCreateCrossRB = pDevice->createRenderBuffer(1, &pCreateCrossVB, pVD);

		mem_release(pVD);
	}

	GXDepthStencilDesc dsDesc;
	dsDesc.cmpFuncDepth = GXCMP_GREATER_EQUAL;
	g_pDSDefault = pDevice->createDepthStencilState(&dsDesc);

	dsDesc.useDepthTest = FALSE;
	dsDesc.useDepthWrite = FALSE;
	g_pDSNoZ = pDevice->createDepthStencilState(&dsDesc);

	XInitViewports();

	g_pMaterialBrowser->initGraphics(pRender);

	g_pCameraConstantBuffer = pDevice->createConstantBuffer(sizeof(SMMATRIX));

	for(UINT i = 0; i < 4; ++i)
	{
		g_bViewportCaptionDirty[i] = true;
	}

	int result = pEngine->start();

	fora(i, g_pEditorImporters)
	{
		g_pEditorImporters[i]->shutdown();
	}

	fora(i, g_pEditableSystems)
	{
		g_pEditableSystems[i]->shutdown();
	}
	XReleaseViewports();

	pChannel->removeListener(&cvarListener);

	mem_release(g_pTopLeftTarget);
	mem_release(g_pTopRightTarget);
	mem_release(g_pBottomLeftTarget);
	mem_release(g_pBottomRightTarget);

	mem_release(g_pSelectionRenderer);
	mem_release(g_pUnselectedRenderer);

	mem_release(g_pDashedMaterial);
	mem_release(g_pDSNoZ);
	mem_release(g_pDSDefault);
	mem_release(g_pCameraConstantBuffer);
	mem_delete(g_pGrid);
	//SkyXEngine_Kill();
	mem_delete(g_pUndoManager);
	mem_release(pEngine);
	mem_release(g_pEditor);
	return(result);
}

void XInitViewports()
{
	RECT rc;
	GetClientRect(g_hTopRightWnd, &rc);
	g_pTopRightTarget->resize(rc.right - rc.left, rc.bottom - rc.top);

	GetClientRect(g_hBottomLeftWnd, &rc);
	g_pBottomLeftTarget->resize(rc.right - rc.left, rc.bottom - rc.top);

	GetClientRect(g_hBottomRightWnd, &rc);
	g_pBottomRightTarget->resize(rc.right - rc.left, rc.bottom - rc.top);
	/*
	GetClientRect(g_pGuiWnd, &rc);
	g_pGuiSwapChain = pContext->createSwapChain(rc.right - rc.left, rc.bottom - rc.top, g_pGuiWnd);
	g_pGuiDepthStencilSurface = pContext->createDepthStencilSurface(rc.right - rc.left, rc.bottom - rc.top, GXFMT_D24S8, GXMULTISAMPLE_NONE);
	*/
}

void XReleaseViewports()
{
	mem_release(g_pGuiSwapChain);
	mem_release(g_pGuiDepthStencilSurface);
}


bool g_isRenderedSelection3D = false;
bool g_isRenderedUnselected3D = false;

void XRender3D()
{
	XPROFILE_FUNCTION();

	IGXDevice *pDevice = g_pRender->getDevice();
	IGXContext *pCtx = pDevice->getThreadContext();

	g_isRenderedSelection3D = true;
	g_isRenderedUnselected3D = true;

	if(!g_pCurrentTool || !g_pCurrentTool->wantDrawSelection(true))
	{
		g_pSelectionRenderer->reset();

		XEnumerateObjects([](IXEditorObject *pObj, bool isProxy, ICompoundObject *pParent){
			if(pObj->isSelected() && (g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent))
			{
				pObj->render(true, true, g_pSelectionRenderer);
			}
			return(XEOR_CONTINUE);
		});

		g_pSelectionRenderer->render(false, false);
	}

	{
		g_pUnselectedRenderer->reset();

		XEnumerateObjects([](IXEditorObject *pObj, bool isProxy, ICompoundObject *pParent){
			if(!pObj->isSelected() && (g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent))
			{
				pObj->render(true, false, g_pUnselectedRenderer);
			}
			return(XEOR_CONTINUE);
		});

		g_pUnselectedRenderer->render(false, false);
	}

	pCtx->setRasterizerState(NULL);

	if(g_xState.isFrameSelect)
	{
		X_2D_VIEW xCurView = g_xConfig.m_x2DView[g_xState.activeWindow];

		float3_t va, vb, vc, vd;

		switch(xCurView)
		{
		case X2D_TOP:
			va = float3_t(g_xState.vFrameSelectStart.x, 0.0f, g_xState.vFrameSelectStart.y);
			vb = float3_t(g_xState.vWorldMousePos.x, 0.0f, g_xState.vFrameSelectStart.y);
			vc = float3_t(g_xState.vWorldMousePos.x, 0.0f, g_xState.vWorldMousePos.y);
			vd = float3_t(g_xState.vFrameSelectStart.x, 0.0f, g_xState.vWorldMousePos.y);
			break;
		case X2D_FRONT:
			va = float3_t(g_xState.vFrameSelectStart.x, g_xState.vFrameSelectStart.y, 0.0f);
			vb = float3_t(g_xState.vWorldMousePos.x, g_xState.vFrameSelectStart.y, 0.0f);
			vc = float3_t(g_xState.vWorldMousePos.x, g_xState.vWorldMousePos.y, 0.0f);
			vd = float3_t(g_xState.vFrameSelectStart.x, g_xState.vWorldMousePos.y, 0.0f);
			break;
		case X2D_SIDE:
			va = float3_t(0.0f, g_xState.vFrameSelectStart.y, g_xState.vFrameSelectStart.x);
			vb = float3_t(0.0f, g_xState.vFrameSelectStart.y, g_xState.vWorldMousePos.x);
			vc = float3_t(0.0f, g_xState.vWorldMousePos.y, g_xState.vWorldMousePos.x);
			vd = float3_t(0.0f, g_xState.vWorldMousePos.y, g_xState.vFrameSelectStart.x);
			break;
		}

		XDrawBorder(GX_COLOR_ARGB(255, 255, 255, 0), va, vb, vc, vd);
	}

	static IGXConstantBuffer *s_pColorBuffer = pDevice->createConstantBuffer(sizeof(float4));
	static IGXConstantBuffer *s_mConstW = pDevice->createConstantBuffer(sizeof(SMMATRIX));

	if(g_xState.bCreateMode)
	{
		IGXRasterizerState *pOldRS = pCtx->getRasterizerState();
		pCtx->setRasterizerState(g_xRenderStates.pRSSolidNoCull);
		pCtx->setVSConstant(s_mConstW, SCR_OBJECT);
		pCtx->setPSConstant(s_pColorBuffer);
		g_pRender->bindShader(pCtx, g_xRenderStates.idColoredShaderKit);

		pCtx->setRenderBuffer(g_xRenderStates.pCreateCrossRB);
		s_mConstW->update(&SMMatrixTranspose(SMMatrixTranslation(g_xState.vCreateOrigin)));
		s_pColorBuffer->update(&float4(0.0f, 1.0f, 0.0f, 1.0f));
		pCtx->setPrimitiveTopology(GXPT_LINELIST);
		pCtx->drawPrimitive(0, 3);
		pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);

		pCtx->setRasterizerState(pOldRS);
		mem_release(pOldRS);
	}


	// Draw handlers
	if(g_pLevelObjects.size())
	{
		IGXRasterizerState *pOldRS = pCtx->getRasterizerState();
		pCtx->setRasterizerState(g_xRenderStates.pRSSolidNoCull);

		pCtx->setPSConstant(s_pColorBuffer);

		pCtx->setPrimitiveTopology(GXPT_LINELIST);
		g_pRender->bindShader(pCtx, g_xRenderStates.idBoundShaderKit);
		//SGCore_ShaderBind(g_xRenderStates.idHandlerShaderKit);
		pCtx->setIndexBuffer(g_xRenderStates.pHandler3DIB);

		s_pColorBuffer->update(&float4(1.0f, 0.0f, 1.0f, 1.0f));

		struct BoundVertex
		{
			float3_t vPos;
			float3_t vSize;
		};

		BoundVertex *pvData;

		pCtx->setRenderBuffer(g_xRenderStates.pHandler3DRB);
		for(bool isSelected = false;; isSelected = true)
		{
			UINT uHandlerCount = 0;
			pvData = NULL;

			XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, ICompoundObject *pParent){
				float3_t vPos = pObj->getPos();
				TODO("Add visibility check");
				/*if(fViewportBorders.x > vPos.x || fViewportBorders.z < vPos.x || fViewportBorders.y < vPos.z) // not visible
				{
					continue;
				}*/

				if(pObj->hasVisualModel() || isSelected != pObj->isSelected() || (!isSelected && pObj->getIcon()))
				{
					return(XEOR_CONTINUE);
				}
				if(!pvData && !g_xRenderStates.pHandlerInstanceVB->lock((void**)&pvData, GXBL_WRITE))
				{
					return(XEOR_STOP);
				}
				float3 vMin, vMax;
				pObj->getBound(&vMin, &vMax);

				pvData[uHandlerCount++] = {(float3)((vMax + vMin) * 0.5f), (float3)(vMax - vMin)};
				if(uHandlerCount == X_MAX_HANDLERS_PER_DIP)
				{
					g_xRenderStates.pHandlerInstanceVB->unlock();
					pCtx->drawIndexedInstanced(uHandlerCount, 8, 12, 0, 0);
					pvData = NULL;
					uHandlerCount = 0;
				}

				return(XEOR_CONTINUE);
			});

			if(pvData)
			{
				g_xRenderStates.pHandlerInstanceVB->unlock();
				pvData = NULL;
			}
			if(uHandlerCount)
			{
				pCtx->drawIndexedInstanced(uHandlerCount, 8, 12, 0, 0);
			}
			if(isSelected)
			{
				break;
			}
			s_pColorBuffer->update(&float4(1.0f, 0.0f, 0.0f, 1.0f));
		}

		g_pRender->unbindShader(pCtx);

		pCtx->setRasterizerState(pOldRS);
		mem_release(pOldRS);

		{
			struct IconVertex
			{
				float3_t vPos;
				float2_t vTex;
			};

			float3 vUp = g_xConfig.m_pViewportCamera[XWP_TOP_LEFT]->getUp() * 0.1f;
			float3 vRight = g_xConfig.m_pViewportCamera[XWP_TOP_LEFT]->getRight() * 0.1f;

			IconVertex *pvIconData;
			if(g_xRenderStates.pIconVB->lock((void**)&pvIconData, GXBL_WRITE))
			{
				pvIconData[0].vPos = float3(-vRight - vUp);
				pvIconData[0].vTex = float2_t(0.0f, 1.0f);
				pvIconData[1].vPos = float3(-vRight + vUp);
				pvIconData[1].vTex = float2_t(0.0f, 0.0f);
				pvIconData[2].vPos = float3(vRight + vUp);
				pvIconData[2].vTex = float2_t(1.0f, 0.0f);
				pvIconData[3].vPos = float3(vRight - vUp);
				pvIconData[3].vTex = float2_t(1.0f, 1.0f);
				g_xRenderStates.pIconVB->unlock();
			}


			s_pColorBuffer->update(&float4(1.0f, 1.0f, 1.0f, 1.0f));
			pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);
			g_pRender->bindShader(pCtx, g_xRenderStates.idIconShaderKit);
			pCtx->setIndexBuffer(g_xRenderStates.pIcon3DIB);
			pCtx->setRenderBuffer(g_xRenderStates.pIcon3DRB);
			pCtx->setSamplerState(g_xRenderStates.pSamplerLinearClamp, 0);

			struct Icon
			{
				IXTexture *pTexture;
				float3_t vPos;
			};
			Array<Icon> aIcons;
			Icon icon;
			XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, ICompoundObject *pParent){
				if((icon.pTexture = pObj->getIcon()))
				{
					icon.vPos = pObj->getPos();
					aIcons.push_back(icon);
				}
				return(XEOR_CONTINUE);
			});
			aIcons.quickSort([](const Icon &a, const Icon &b){
				return(a.pTexture < b.pTexture);
			});

			float3_t *pvData2 = NULL;

			UINT uIconCount = 0;
			IXTexture *pTexture = aIcons.size() ? aIcons[0].pTexture : NULL;
			bool bForceDraw = false;
			for(UINT i = 0, l = aIcons.size(); i < l; ++i)
			{
				if(!pvData2)
				{
					if(g_xRenderStates.pHandlerInstanceVB->lock((void**)&pvData2, GXBL_WRITE))
					{
						IGXBaseTexture *pTex = NULL;
						aIcons[i].pTexture->getAPITexture(&pTex);
						pCtx->setPSTexture(pTex);
						mem_release(pTex);
					}
					else
					{
						break;
					}
				}

				if(pTexture == aIcons[i].pTexture)
				{
					pvData2[uIconCount++] = aIcons[i].vPos;
				}
				else
				{
					pTexture = aIcons[i].pTexture;
					bForceDraw = true;
					--i;
				}
				if(bForceDraw || uIconCount == X_MAX_HANDLERS_PER_DIP * 2)
				{
					g_xRenderStates.pHandlerInstanceVB->unlock();
					pCtx->drawIndexedInstanced(uIconCount, 8, 12, 0, 0);
					pvData2 = NULL;
					uIconCount = 0;
				}
			}
			if(pvData2)
			{
				g_xRenderStates.pHandlerInstanceVB->unlock();
				pvData2 = NULL;
			}
			if(uIconCount)
			{
				pCtx->drawIndexedInstanced(uIconCount, 8, 12, 0, 0);
			}
			pCtx->setSamplerState(NULL, 0);
		}
	}
}

void XRender2D(IXCamera *pCamera, X_2D_VIEW view, float fScale, bool preScene, bool bRenderSelection)
{
	XPROFILE_FUNCTION();

	IGXDevice *pDevice = g_pRender->getDevice();
	IGXContext *pCtx = pDevice->getThreadContext();

	if(preScene)
	{
		if(g_xConfig.m_bShowGrid)
		{
			GRID_STEP step = g_xConfig.m_gridStep;
			GRID_STEP minStep = CGrid::GetMinStepForScale(fScale);
			if(step < minStep)
			{
				step = minStep;
			}
			SMMATRIX mWorld;
			switch(view)
			{
			case X2D_FRONT:
				mWorld = SMMatrixRotationX(SM_PIDIV2);
				break;
			case X2D_SIDE:
				mWorld = SMMatrixRotationZ(SM_PIDIV2);
				break;
			}
			g_pGrid->setOpacity(g_xConfig.m_fGridOpacity);
			g_pGrid->setDottedMode(g_xConfig.m_bDottedGrid);
			g_pGrid->render(mWorld, pCamera, step);
		}
	}
	else
	{
		static IGXConstantBuffer *s_pColorBuffer = pDevice->createConstantBuffer(sizeof(float4));

		if(bRenderSelection)
		{
			if(!g_pCurrentTool || !g_pCurrentTool->wantDrawSelection(false))
			{
				if(g_isRenderedSelection3D)
				{
					g_pSelectionRenderer->reset();

					XEnumerateObjects([](IXEditorObject *pObj, bool isProxy, ICompoundObject *pParent){
						if(pObj->isSelected() && !(g_xConfig.m_bIgnoreGroups && isProxy))
						{
							pObj->render(false, true, g_pSelectionRenderer);
						}
						return(XEOR_CONTINUE);
					});

					g_isRenderedSelection3D = false;
				}
				g_pSelectionRenderer->render(true);
			}
			return;
		}

		if(g_isRenderedUnselected3D)
		{
			g_pUnselectedRenderer->reset();

			XEnumerateObjects([](IXEditorObject *pObj, bool isProxy, ICompoundObject *pParent){
				if(!pObj->isSelected() && !(g_xConfig.m_bIgnoreGroups && isProxy))
				{
					pObj->render(false, false, g_pUnselectedRenderer);
				}
				return(XEOR_CONTINUE);
			});

			g_isRenderedUnselected3D = false;
		}
		g_pUnselectedRenderer->render(true);

		// Draw handlers
		if(g_pLevelObjects.size() || g_apProxies.size())
		{
			pCtx->setPSConstant(s_pColorBuffer);

			pCtx->setPrimitiveTopology(GXPT_LINELIST);
			g_pRender->bindShader(pCtx, g_xRenderStates.idHandlerShaderKit);
			pCtx->setIndexBuffer(g_xRenderStates.pHandlerIB);

			s_pColorBuffer->update(&float4(0.0f, 1.0f, 0.0f, 1.0f));

			float3_t *pvData;
			float fPtSize = MulDpiF(3.5f, g_uWndMainDpi) * fScale;
			if(g_xRenderStates.pHandlerVB->lock((void**)&pvData, GXBL_WRITE))
			{
				pvData[0] = float3_t(fPtSize, fPtSize, fPtSize);
				pvData[1] = (float3)(pvData[0] * -1.0f);

				pvData[2] = float3_t(fPtSize, fPtSize, -fPtSize);
				pvData[3] = (float3)(pvData[2] * -1.0f);

				pvData[4] = float3_t(fPtSize, -fPtSize, fPtSize);
				pvData[5] = (float3)(pvData[4] * -1.0f);

				pvData[6] = float3_t(-fPtSize, fPtSize, fPtSize);
				pvData[7] = (float3)(pvData[6] * -1.0f);

				g_xRenderStates.pHandlerVB->unlock();
			}
			pCtx->setRenderBuffer(g_xRenderStates.pHandlerRB);
			float4 fViewportBorders = g_xState.m_vViewportBorders[g_xState.activeWindow];
			for(bool isSelected = false;; isSelected = true)
			{
				UINT uHandlerCount = 0;
				pvData = NULL;
				XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, ICompoundObject *pParent){
					float3_t vPos = pObj->getPos();
					//@TODO: Add visibility check
					/*if(fViewportBorders.x > vPos.x || fViewportBorders.z < vPos.x || fViewportBorders.y < vPos.z) // not visible
					{
					continue;
					}*/
					if(isSelected != pObj->isSelected())
					{
						return(XEOR_CONTINUE);
					}
					if(isProxy)
					{
						return(XEOR_CONTINUE);
					}

					if(!pvData && !g_xRenderStates.pHandlerInstanceVB->lock((void**)&pvData, GXBL_WRITE))
					{
						return(XEOR_CONTINUE);
					}

					pvData[uHandlerCount++] = vPos;
					if(uHandlerCount == X_MAX_HANDLERS_PER_DIP * 2)
					{
						g_xRenderStates.pHandlerInstanceVB->unlock();
						pCtx->drawIndexedInstanced(uHandlerCount, 8, 4, 0, 0);
						pvData = NULL;
						uHandlerCount = 0;
					}
					return(XEOR_CONTINUE);
				});

				if(pvData)
				{
					g_xRenderStates.pHandlerInstanceVB->unlock();
				}
				if(uHandlerCount)
				{
					pCtx->drawIndexedInstanced(uHandlerCount, 8, 4, 0, 0);
				}
				if(isSelected)
				{
					break;
				}
				s_pColorBuffer->update(&float4(1.0f, 0.0f, 0.0f, 1.0f));
			}

			g_pRender->unbindShader(pCtx);
		}

		// Draw entity boxes
		if(g_pLevelObjects.size())
		{
			IGXRasterizerState *pOldRS = pCtx->getRasterizerState();
			pCtx->setRasterizerState(g_xRenderStates.pRSSolidNoCull);

			pCtx->setPSConstant(s_pColorBuffer);

			pCtx->setPrimitiveTopology(GXPT_LINELIST);
			g_pRender->bindShader(pCtx, g_xRenderStates.idBoundShaderKit);
			//SGCore_ShaderBind(g_xRenderStates.idHandlerShaderKit);
			pCtx->setIndexBuffer(g_xRenderStates.pHandler3DIB);

			s_pColorBuffer->update(&float4(1.0f, 0.0f, 1.0f, 1.0f));

			struct BoundVertex
			{
				float3_t vPos;
				float3_t vSize;
			};

			BoundVertex *pvData;

			pCtx->setRenderBuffer(g_xRenderStates.pHandler3DRB);
			for(bool isSelected = false;; isSelected = true)
			{
				UINT uHandlerCount = 0;
				pvData = NULL;
				XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, ICompoundObject *pParent){
					float3_t vPos = pObj->getPos();
					TODO("Add visibility check");
					/*if(fViewportBorders.x > vPos.x || fViewportBorders.z < vPos.x || fViewportBorders.y < vPos.z) // not visible
					{
					continue;
					}*/

					if(pObj->hasVisualModel() || isSelected != pObj->isSelected())
					{
						return(XEOR_CONTINUE);
					}
					if(!pvData && !g_xRenderStates.pHandlerInstanceVB->lock((void**)&pvData, GXBL_WRITE))
					{
						return(XEOR_STOP);
					}
					float3 vMin, vMax;
					pObj->getBound(&vMin, &vMax);

					pvData[uHandlerCount++] = {(float3)((vMax + vMin) * 0.5f), (float3)(vMax - vMin)};
					if(uHandlerCount == X_MAX_HANDLERS_PER_DIP)
					{
						g_xRenderStates.pHandlerInstanceVB->unlock();
						pCtx->drawIndexedInstanced(uHandlerCount, 8, 12, 0, 0);
						pvData = NULL;
						uHandlerCount = 0;
					}

					return(XEOR_CONTINUE);
				});
				
				if(pvData)
				{
					g_xRenderStates.pHandlerInstanceVB->unlock();
					pvData = NULL;
				}
				if(uHandlerCount)
				{
					pCtx->drawIndexedInstanced(uHandlerCount, 8, 12, 0, 0);
				}
				if(isSelected)
				{
					break;
				}
				s_pColorBuffer->update(&float4(1.0f, 0.0f, 0.0f, 1.0f));
			}

			g_pRender->unbindShader(pCtx);

			pCtx->setRasterizerState(pOldRS);
			mem_release(pOldRS);
		}

		if(g_xState.isFrameSelect)
		{
			X_2D_VIEW xCurView = g_xConfig.m_x2DView[g_xState.activeWindow];
			float3_t va, vb, vc, vd;
			switch(xCurView)
			{
			case X2D_TOP:
				va = float3_t(g_xState.vFrameSelectStart.x, 0.0f, g_xState.vFrameSelectStart.y);
				vb = float3_t(g_xState.vWorldMousePos.x, 0.0f, g_xState.vFrameSelectStart.y);
				vc = float3_t(g_xState.vWorldMousePos.x, 0.0f, g_xState.vWorldMousePos.y);
				vd = float3_t(g_xState.vFrameSelectStart.x, 0.0f, g_xState.vWorldMousePos.y);
				break;
			case X2D_FRONT:
				va = float3_t(g_xState.vFrameSelectStart.x, g_xState.vFrameSelectStart.y, 0.0f);
				vb = float3_t(g_xState.vWorldMousePos.x, g_xState.vFrameSelectStart.y, 0.0f);
				vc = float3_t(g_xState.vWorldMousePos.x, g_xState.vWorldMousePos.y, 0.0f);
				vd = float3_t(g_xState.vFrameSelectStart.x, g_xState.vWorldMousePos.y, 0.0f);
				break;
			case X2D_SIDE:
				va = float3_t(0.0f, g_xState.vFrameSelectStart.y, g_xState.vFrameSelectStart.x);
				vb = float3_t(0.0f, g_xState.vFrameSelectStart.y, g_xState.vWorldMousePos.x);
				vc = float3_t(0.0f, g_xState.vWorldMousePos.y, g_xState.vWorldMousePos.x);
				vd = float3_t(0.0f, g_xState.vWorldMousePos.y, g_xState.vFrameSelectStart.x);
				break;
			}
			XDrawBorder(GX_COLOR_ARGB(255, 255, 255, 0), va, vb, vc, vd, fScale);
		}

		static IGXConstantBuffer *s_mConstW = pDevice->createConstantBuffer(sizeof(SMMATRIX));
		extern HWND g_hABArrowButton;
		if(g_xState.bHasSelection && Button_GetCheck(g_hABArrowButton))
		{
			float3_t va, vb, vc, vd;
			float4 vBorder;
			float2 vSelectionCenter;
			switch(view)
			{
			case X2D_TOP:
				va = float3_t(g_xState.vSelectionBoundMin.x, 0.0f, g_xState.vSelectionBoundMin.z);
				vb = float3_t(g_xState.vSelectionBoundMax.x, 0.0f, g_xState.vSelectionBoundMin.z);
				vc = float3_t(g_xState.vSelectionBoundMax.x, 0.0f, g_xState.vSelectionBoundMax.z);
				vd = float3_t(g_xState.vSelectionBoundMin.x, 0.0f, g_xState.vSelectionBoundMax.z);
				vBorder = float4(g_xState.vSelectionBoundMin.x, g_xState.vSelectionBoundMin.z,
					g_xState.vSelectionBoundMax.x, g_xState.vSelectionBoundMax.z);
				vSelectionCenter = float2(g_xState.vSelectionBoundMin.x + g_xState.vSelectionBoundMax.x,
					g_xState.vSelectionBoundMin.z + g_xState.vSelectionBoundMax.z) * 0.5f;
				break;
			case X2D_FRONT:
				va = float3_t(g_xState.vSelectionBoundMin.x, g_xState.vSelectionBoundMin.y, 0.0f);
				vb = float3_t(g_xState.vSelectionBoundMax.x, g_xState.vSelectionBoundMin.y, 0.0f);
				vc = float3_t(g_xState.vSelectionBoundMax.x, g_xState.vSelectionBoundMax.y, 0.0f);
				vd = float3_t(g_xState.vSelectionBoundMin.x, g_xState.vSelectionBoundMax.y, 0.0f);
				vBorder = float4(g_xState.vSelectionBoundMin.x, g_xState.vSelectionBoundMin.y,
					g_xState.vSelectionBoundMax.x, g_xState.vSelectionBoundMax.y);
				vSelectionCenter = float2(g_xState.vSelectionBoundMin.x + g_xState.vSelectionBoundMax.x,
					g_xState.vSelectionBoundMin.y + g_xState.vSelectionBoundMax.y) * 0.5f;
				break;
			case X2D_SIDE:
				va = float3_t(0.0f, g_xState.vSelectionBoundMin.y, g_xState.vSelectionBoundMin.z);
				vb = float3_t(0.0f, g_xState.vSelectionBoundMin.y, g_xState.vSelectionBoundMax.z);
				vc = float3_t(0.0f, g_xState.vSelectionBoundMax.y, g_xState.vSelectionBoundMax.z);
				vd = float3_t(0.0f, g_xState.vSelectionBoundMax.y, g_xState.vSelectionBoundMin.z);
				vBorder = float4(g_xState.vSelectionBoundMin.z, g_xState.vSelectionBoundMin.y,
					g_xState.vSelectionBoundMax.z, g_xState.vSelectionBoundMax.y);
				vSelectionCenter = float2(g_xState.vSelectionBoundMin.z + g_xState.vSelectionBoundMax.z,
					g_xState.vSelectionBoundMin.y + g_xState.vSelectionBoundMax.y) * 0.5f;
				break;
			}
			XDrawBorder(GX_COLOR_ARGB(255, 255, 0, 0), va, vb, vc, vd, fScale);

			float3_t *pvData;
			float fPtSize = MulDpiF(3.0f, g_uWndMainDpi) * fScale;
			float fPtMargin = MulDpiF(7.0f, g_uWndMainDpi) * fScale;
			if(g_xRenderStates.pTransformHandlerVB->lock((void**)&pvData, GXBL_WRITE))
			{
				UINT uCV = 0;

				if(g_xState.xformType == X2DXF_SCALE || g_xState.xformType == X2DXF_NONE)
				{
					float3 vCenter(vSelectionCenter.x, 0.0f, vBorder.y - fPtMargin);
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, fPtSize));

					vCenter = float3(vBorder.z + fPtMargin, 0.0f, vSelectionCenter.y);
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, fPtSize));

					vCenter = float3(vSelectionCenter.x, 0.0f, vBorder.w + fPtMargin);
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, fPtSize));

					vCenter = float3(vBorder.x - fPtMargin, 0.0f, vSelectionCenter.y);
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, fPtSize));

					vCenter = float3(vBorder.x - fPtMargin, 0.0f, vBorder.y - fPtMargin);
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, fPtSize));

					vCenter = float3(vBorder.z + fPtMargin, 0.0f, vBorder.y - fPtMargin);
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, fPtSize));

					vCenter = float3(vBorder.z + fPtMargin, 0.0f, vBorder.w + fPtMargin);
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, fPtSize));

					vCenter = float3(vBorder.x - fPtMargin, 0.0f, vBorder.w + fPtMargin);
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, fPtSize));
				}
				else if(g_xState.xformType == X2DXF_ROTATE)
				{
					float fPtSize2 = fPtSize * sqrtf(2.0f) * 0.5f;

					float3 vCenter = float3(vBorder.x - fPtMargin, 0.0f, vBorder.y - fPtMargin);
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, 0.0f));
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize2, 0.0f, -fPtSize2));
					pvData[uCV++] = (float3)(vCenter + float3(0.0f, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize2, 0.0f, -fPtSize2));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, 0.0f));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize2, 0.0f, fPtSize2));
					pvData[uCV++] = (float3)(vCenter + float3(0.0f, 0.0f, fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize2, 0.0f, fPtSize2));

					vCenter = float3(vBorder.z + fPtMargin, 0.0f, vBorder.y - fPtMargin);
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, 0.0f));
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize2, 0.0f, -fPtSize2));
					pvData[uCV++] = (float3)(vCenter + float3(0.0f, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize2, 0.0f, -fPtSize2));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, 0.0f));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize2, 0.0f, fPtSize2));
					pvData[uCV++] = (float3)(vCenter + float3(0.0f, 0.0f, fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize2, 0.0f, fPtSize2));

					vCenter = float3(vBorder.z + fPtMargin, 0.0f, vBorder.w + fPtMargin);
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, 0.0f));
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize2, 0.0f, -fPtSize2));
					pvData[uCV++] = (float3)(vCenter + float3(0.0f, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize2, 0.0f, -fPtSize2));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, 0.0f));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize2, 0.0f, fPtSize2));
					pvData[uCV++] = (float3)(vCenter + float3(0.0f, 0.0f, fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize2, 0.0f, fPtSize2));

					vCenter = float3(vBorder.x - fPtMargin, 0.0f, vBorder.w + fPtMargin);
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize, 0.0f, 0.0f));
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize2, 0.0f, -fPtSize2));
					pvData[uCV++] = (float3)(vCenter + float3(0.0f, 0.0f, -fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize2, 0.0f, -fPtSize2));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize, 0.0f, 0.0f));
					pvData[uCV++] = (float3)(vCenter + float3(fPtSize2, 0.0f, fPtSize2));
					pvData[uCV++] = (float3)(vCenter + float3(0.0f, 0.0f, fPtSize));
					pvData[uCV++] = (float3)(vCenter + float3(-fPtSize2, 0.0f, fPtSize2));
				}

				g_xRenderStates.pTransformHandlerVB->unlock();
			}

			SMMATRIX mViewProj = pCamera->getViewMatrix() * pCamera->getProjMatrix();
			SMMATRIX mWorld;
			switch(view)
			{
			case X2D_FRONT:
				mWorld = SMMatrixRotationX(-SM_PIDIV2);
				break;
			case X2D_SIDE:
				mWorld = SMMatrixRotationY(-SM_PIDIV2) * SMMatrixRotationZ(-SM_PIDIV2);
				break;
			}
			
			s_mConstW->update(&SMMatrixTranspose(mWorld));
			pCtx->setVSConstant(s_mConstW, SCR_OBJECT);

			IGXRasterizerState *pOldRS = pCtx->getRasterizerState();
			pCtx->setRasterizerState(g_xRenderStates.pRSSolidNoCull);
			pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);
			g_pRender->bindShader(pCtx, g_xRenderStates.idColoredShaderKit);
			pCtx->setRenderBuffer(g_xRenderStates.pTransformHandlerRB);
			s_pColorBuffer->update(&float4(1.0f, 1.0f, 1.0f, 1.0f));
			pCtx->setPSConstant(s_pColorBuffer);
			if(g_xState.xformType == X2DXF_SCALE || g_xState.xformType == X2DXF_NONE)
			{
				pCtx->setIndexBuffer(g_xRenderStates.pTransformHandlerScaleIB);
				pCtx->drawIndexed(32, 16, 0, 0);
			}
			else if(g_xState.xformType == X2DXF_ROTATE)
			{
				pCtx->setIndexBuffer(g_xRenderStates.pTransformHandlerRotateIB);
				pCtx->drawIndexed(32, 24, 0, 0);
			}


			g_pRender->unbindShader(pCtx);
			pCtx->setRasterizerState(pOldRS);
			mem_release(pOldRS);
		}

		if(g_xState.bCreateMode)
		{
			IGXRasterizerState *pOldRS = pCtx->getRasterizerState();
			pCtx->setRasterizerState(g_xRenderStates.pRSSolidNoCull);
			pCtx->setVSConstant(s_mConstW, SCR_OBJECT);
			pCtx->setPSConstant(s_pColorBuffer);
			g_pRender->bindShader(pCtx, g_xRenderStates.idColoredShaderKit);

			pCtx->setRenderBuffer(g_xRenderStates.pCreateCrossRB);
			s_mConstW->update(&SMMatrixTranspose(SMMatrixTranslation(g_xState.vCreateOrigin)));
			s_pColorBuffer->update(&float4(0.0f, 1.0f, 0.0f, 1.0f));
			pCtx->setPrimitiveTopology(GXPT_LINELIST);
			pCtx->drawPrimitive(0, 3);
			pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);

			pCtx->setRasterizerState(pOldRS);
			mem_release(pOldRS);
		}
	}
}

void XLoadLevel(const char *szName)
{
	g_pEngine->getCore()->getConsole()->execCommand2("map %s", szName);
}

void XResetLevel()
{
	g_pEngine->getCore()->getConsole()->execCommand("endmap");
}

bool XSaveLevel(const char *szNewName, bool bForcePrompt)
{
	if(szNewName)
	{
		char szPathDirLevel[1024];
		sprintf(szPathDirLevel, "%s%s/", Core_RStringGet(G_RI_STRING_PATH_GS_LEVELS), szNewName);

		if(!FileExistsDir(szPathDirLevel))
		{
			FileCreateDir(szPathDirLevel);
		}

		g_sLevelName = szNewName;

		fora(i, g_apProxies)
		{
			g_apProxies[i]->build();
		}

		XEventLevel ev;
		ev.type = XEventLevel::TYPE_SAVE;
		ev.szLevelName = szNewName;
		Core_GetIXCore()->getEventChannel<XEventLevel>(EVENT_LEVEL_GUID)->broadcastEvent(&ev);
		return(true);
	}
	
	if(!bForcePrompt && g_sLevelName.length())
	{
		// g_sLevelName can changed during save process, so pointer will be broken
		String tmp = g_sLevelName;
		return(XSaveLevel(tmp.c_str()));
	}
		
	char szName[1024];
	int iMaxLength = sizeof(szName);
	if(Tools::DlgPrompt(szName, &iMaxLength, "Level name", "Save new level"))
	{
		return(XSaveLevel(szName));
	}
	
	return(false);
}

void XDrawBorder(GXCOLOR color, const float3_t &vA, const float3_t &vB, const float3_t &vC, const float3_t &vD, float fViewportScale)
{
	float2_t ta, tb, tc, td;
	if(vA.x == vB.x && vB.x == vC.x)
	{
		ta = float2_t(vA.y, vA.z);
		tb = float2_t(vB.y, vB.z);
		tc = float2_t(vC.y, vC.z);
		td = float2_t(vD.y, vD.z);
	}
	else if(vA.y == vB.y && vB.y == vC.y)
	{
		ta = float2_t(vA.x, vA.z);
		tb = float2_t(vB.x, vB.z);
		tc = float2_t(vC.x, vC.z);
		td = float2_t(vD.x, vD.z);
	}
	else
	{
		ta = float2_t(vA.x, vA.y);
		tb = float2_t(vB.x, vB.y);
		tc = float2_t(vC.x, vC.y);
		td = float2_t(vD.x, vD.y);
	}

	XBorderVertex *pData;
	if(g_pBorderVertexBuffer->lock((void**)&pData, GXBL_WRITE))
	{
		pData[0] = {vA, ta / fViewportScale / 6.0f};
		pData[1] = {vB, tb / fViewportScale / 6.0f};
		pData[2] = {vC, tc / fViewportScale / 6.0f};
		pData[3] = {vD, td / fViewportScale / 6.0f};
		pData[4] = pData[0];
		g_pBorderVertexBuffer->unlock();
	}

	IGXDevice *pDevice = g_pRender->getDevice();
	IGXContext *pCtx = pDevice->getThreadContext();

	IGXBlendState *pOldBlendState = pCtx->getBlendState();
	g_pRender->bindShader(pCtx, g_xRenderStates.idTexturedShaderKit);

	static IGXConstantBuffer *s_pColorBuffer = pDevice->createConstantBuffer(sizeof(float4));
	s_pColorBuffer->update(&GX_COLOR_COLOR_TO_F4(color));
	pCtx->setPSConstant(s_pColorBuffer);

	pCtx->setPSTexture(g_pDashedMaterial);
	pCtx->setBlendState(g_xRenderStates.pBlendAlpha);
	pCtx->setRenderBuffer(g_pBorderRenderBuffer);
	pCtx->setPrimitiveTopology(GXPT_LINESTRIP);
	pCtx->drawPrimitive(0, 4);
	g_pRender->unbindShader(pCtx);

	pCtx->setBlendState(pOldBlendState);
	mem_release(pOldBlendState);
}

void XUpdateUndoRedo();
bool XExecCommand(IXEditorCommand *pCommand)
{
	if(g_pUndoManager->execCommand(pCommand))
	{
		XUpdateUndoRedo();
		return(true);
	}
	return(false);
}

void XAttachCommand(IXEditorCommand *pCommand)
{
	g_pUndoManager->attachCommand(pCommand);
}

void XUpdateSelectionBound()
{
	g_xState.bHasSelection = false;
	float3 vMin, vMax;

	XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, ICompoundObject *pParent){
		if(pObj->isSelected()/* && !(g_xConfig.m_bIgnoreGroups && isProxy)*/)
		{
			pObj->getBound(&vMin, &vMax);
			if(!g_xState.bHasSelection)
			{
				g_xState.bHasSelection = true;
				g_xState.vSelectionBoundMax = vMax;
				g_xState.vSelectionBoundMin = vMin;
			}
			else
			{
				g_xState.vSelectionBoundMax = (float3)SMVectorMax(g_xState.vSelectionBoundMax, vMax);
				g_xState.vSelectionBoundMin = (float3)SMVectorMin(g_xState.vSelectionBoundMin, vMin);
			}
			return(XEOR_SKIP_CHILDREN);
		}
		return(XEOR_CONTINUE);
	});
	
	bool hasGroupSelected = false;
	if(g_xState.bHasSelection)
	{
		fora(i, g_apGroups)
		{
			if(g_apGroups[i]->isSelected())
			{
				hasGroupSelected = true;
				break;
			}
		}
	}
	EnableToolbarButton(ID_TOOLS_GROUP, g_xState.bHasSelection);
	EnableToolbarButton(ID_TOOLS_UNGROUP, hasGroupSelected);
}

bool XRayCast(X_WINDOW_POS wnd)
{
	float3 vStart, vEnd, vPos, vCamPos;
	if(!g_xConfig.m_pViewportCamera[wnd])
	{
		return(false);
	}
	vCamPos = g_xConfig.m_pViewportCamera[wnd]->getPosition();
	switch(g_xConfig.m_x2DView[wnd])
	{
	case X2D_NONE:
		return(false);
	case X2D_TOP:
		vStart = float3(g_xState.vWorldMousePos.x, vCamPos.y, g_xState.vWorldMousePos.y);
		vEnd = float3(0.0f, -2000.0f, 0.0f);
		break;
	case X2D_FRONT:
		vStart = float3(g_xState.vWorldMousePos.x, g_xState.vWorldMousePos.y, vCamPos.z);
		vEnd = float3(0.0f, 0.0f, 2000.0f);
		break;
	case X2D_SIDE:
		vStart = float3(vCamPos.x, g_xState.vWorldMousePos.y, g_xState.vWorldMousePos.x);
		vEnd = float3(-2000.0f, 0.0f, 0.0f);
		break;
	}
	vEnd += vStart;

	bool res = false;
	XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, ICompoundObject *pParent){
		if(!res && pObj->isSelected() && !(g_xConfig.m_bIgnoreGroups && isProxy) && pObj->rayTest(vStart, vEnd, &vPos, NULL))
		{
			res = true;
		}
		return(XEOR_CONTINUE);
	});
	return(false);
}

bool XIsMouseInBound(X_WINDOW_POS wnd, const float3_t &vBoundMin, const float3_t &vBoundMax)
{
	if(!g_xConfig.m_pViewportCamera[wnd])
	{
		return(false);
	}
	const float2_t &fMPos = g_xState.vWorldMousePos;
	float fMargin = g_xConfig.m_fViewportScale[wnd] * 5.0f;
	switch(g_xConfig.m_x2DView[wnd])
	{
	case X2D_TOP:
		return(fMPos.x >= vBoundMin.x - fMargin && fMPos.x <= vBoundMax.x + fMargin &&
			fMPos.y >= vBoundMin.z - fMargin && fMPos.y <= vBoundMax.z + fMargin);

	case X2D_FRONT:
		return(fMPos.x >= vBoundMin.x - fMargin && fMPos.x <= vBoundMax.x + fMargin &&
			fMPos.y >= vBoundMin.y - fMargin && fMPos.y <= vBoundMax.y + fMargin);

	case X2D_SIDE:
		return(fMPos.x >= vBoundMin.z - fMargin && fMPos.x <= vBoundMax.z + fMargin &&
			fMPos.y >= vBoundMin.y - fMargin && fMPos.y <= vBoundMax.y + fMargin);
	}
	return(false);
}

bool XIsMouseInSelection(X_WINDOW_POS wnd)
{
	if(!g_xState.bHasSelection)
	{
		return(false);
	}
	return(XIsMouseInBound(wnd, g_xState.vSelectionBoundMin, g_xState.vSelectionBoundMax));
}

void XUpdateWindowTitle()
{
	const char *szLevelName = g_sLevelName.c_str();
	static char szCaption[256];
	bool isDirty = g_pUndoManager ? g_pUndoManager->isDirty() : false;
	if(szLevelName && szLevelName[0])
	{
		sprintf(szCaption, "%s - [%s]%s | %s", MAIN_WINDOW_TITLE, szLevelName, isDirty ? "*" : "", SKYXENGINE_VERSION4EDITORS);
	}
	else
	{
		sprintf(szCaption, "%s%s | %s", MAIN_WINDOW_TITLE, isDirty ? " - *" : "", SKYXENGINE_VERSION4EDITORS);
	}
	PostMessageA(g_hWndMain, WM_SETTITLEASYNC, 0, (LPARAM)szCaption);
	// SetWindowText(g_hWndMain, szCaption);
}

void XExportToObj(const char *szMdl)
{
	IXCore *pCore = g_pEngine->getCore();
	IXResourceManager *pResourceManager = pCore->getResourceManager();
	IFileSystem *pFileSystem = pCore->getFileSystem();

	IXResourceModel *pResource;
	IXResourceModelStatic *pResourceStatic;
	if(!pResourceManager->getModel(szMdl, &pResource))
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Unable to load model '%s'!\n", szMdl);
		return;
	}
	if(!(pResourceStatic = pResource->asStatic()))
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Model '%s' is not a static model. Cannot export!\n", szMdl);
		mem_release(pResource);
		return;
	}

	const char *szBasename = basename(szMdl);
	char buf[128];
	
	sprintf(buf, "export/%s.mtl", szBasename);
	IFile *pMatFile = pFileSystem->openFile(buf, FILE_MODE_WRITE);

	sprintf(buf, "export/%s.obj", szBasename);
	IFile *pObjFile = pFileSystem->openFile(buf, FILE_MODE_WRITE);
	

	pObjFile->writeText("# File was written by " SKYXENGINE_VERSION4EDITORS "\n# Original model: %s\n\nmtllib %s.mtl\no model\n", szMdl, szBasename);

	UINT uSubsets = pResourceStatic->getSubsetCount(0);
	for(UINT i = 0; i < uSubsets; ++i)
	{
		auto *pSubset = pResourceStatic->getSubset(0, i);
		pObjFile->writeText("\n# Subset %u\n", i);
		for(UINT j = 0; j < pSubset->iVertexCount; ++j)
		{
			auto &v = pSubset->pVertices[j];

			pObjFile->writeText("v %f %f %f\nvt %f %f\nvn %f %f %f\n", 
				v.vPos.x, v.vPos.y, -v.vPos.z,
				v.vTex.x, 1.0f - v.vTex.y,
				v.vNorm.x, v.vNorm.y, v.vNorm.z
				);
		}
	}
	// XPT_TRIANGLELIST
	//XPT_TRIANGLESTRIP
	UINT uFaceOffset = 1;
	XPT_TOPOLOGY topology = pResourceStatic->getPrimitiveTopology();
	for(UINT i = 0; i < uSubsets; ++i)
	{
		auto *pSubset = pResourceStatic->getSubset(0, i);
		pObjFile->writeText("\n# Subset %u\ng subset_%u\nusemtl mtl_%u\ns off\n", i, i, pSubset->iMaterialID);
		
		if(topology == XPT_TRIANGLELIST)
		{
			for(UINT j = 0; j < pSubset->iIndexCount; j += 3)
			{
				UINT a = pSubset->pIndices[j] + uFaceOffset;
				UINT b = pSubset->pIndices[j + 1] + uFaceOffset;
				UINT c = pSubset->pIndices[j + 2] + uFaceOffset;
				pObjFile->writeText("f %u/%u/%u %u/%u/%u %u/%u/%u\n",
					a, a, a,
					c, c, c,
					b, b, b
					);
			}
		}
		else if(topology == XPT_TRIANGLESTRIP)
		{
			for(UINT j = 0; j < pSubset->iIndexCount - 2; ++j)
			{
				UINT a = pSubset->pIndices[j] + uFaceOffset;
				UINT b = pSubset->pIndices[j + 1] + uFaceOffset;
				UINT c = pSubset->pIndices[j + 2] + uFaceOffset;
				pObjFile->writeText("f %u/%u/%u %u/%u/%u %u/%u/%u\n",
					a, a, a,
					c, c, c,
					b, b, b
					);
			}
		}
		else
		{
			assert(!"Unknown topology!");
		}

		uFaceOffset += pSubset->iVertexCount;
	}

	UINT uMatCount = pResourceStatic->getMaterialCount();
	for(UINT i = 0; i < uMatCount; ++i)
	{
		const char *szMtl = pResourceStatic->getMaterial(i);

		pMatFile->writeText("newmtl mtl_%u\n", i);

		if(szMtl)
		{
			pMatFile->writeText("  map_Kd %s.dds\n", szMtl);
		}

		pMatFile->writeText("\n");
	}

	mem_release(pMatFile);
	mem_release(pObjFile);
	mem_release(pResource);

	LibReport(REPORT_MSG_LEVEL_NOTICE, "%s was written\n", buf);
}

void XExportToDSE(const char *szMdl)
{
	IXCore *pCore = g_pEngine->getCore();
	IXResourceManager *pResourceManager = pCore->getResourceManager();
	IFileSystem *pFileSystem = pCore->getFileSystem();

	IXResourceModel *pResource;
	//IXResourceModelStatic *pResourceStatic;
	if(!pResourceManager->getModel(szMdl, &pResource))
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Unable to load model '%s'!\n", szMdl);
		return;
	}
	//if(!(pResourceStatic = pResource->asStatic()))
	//{
	//	LibReport(REPORT_MSG_LEVEL_ERROR, "Model '%s' is not a static model. Cannot export!\n", szMdl);
	//	mem_release(pResource);
	//	return;
	//}

	const char *szBasename = basename(szMdl);
	char buf[128];

	sprintf(buf, "export/%s.dse", szBasename);
	IFile *pFile = pFileSystem->openFile(buf, FILE_MODE_WRITE);
	if(!pFile)
	{
		mem_release(pResource);
		LibReport(REPORT_MSG_LEVEL_ERROR, "Unable to write file '%s'!\n", buf);
		return;
	}

	IXModelWriter *pWriter = (IXModelWriter*)Core_GetIXCore()->getPluginManager()->getInterface(IXMODELWRITER_GUID);
	if(pWriter)
	{
		pWriter->writeModel(pResource, pFile);
	}

	mem_release(pFile);
	mem_release(pResource);

	LibReport(REPORT_MSG_LEVEL_NOTICE, "%s was written\n", buf);
}

ICompoundObject* XTakeObject(IXEditorObject *pObject, ICompoundObject *pWhere)
{
	int idx = g_pLevelObjects.indexOf(pObject);
	assert((idx >= 0) != (pWhere == NULL));
	if(idx >= 0)
	{
		g_pLevelObjects.erase(idx);
		g_mObjectsLocation[pObject] = pWhere;
		g_pEditor->onObjectRemoved(pObject);
		return(NULL);
	}

	const Map<IXEditorObject*, ICompoundObject*>::Node *pNode;
	if(g_mObjectsLocation.KeyExists(pObject, &pNode))
	{
		ICompoundObject *pProxy = *pNode->Val;
		//pProxy->removeChildObject(pObject);
		if(pWhere)
		{
			g_mObjectsLocation[pObject] = pWhere;
		}
		else
		{
			g_mObjectsLocation.erase(pObject);
			g_pLevelObjects.push_back(pObject);
			g_pEditor->onObjectAdded(pObject);
		}
		return(pProxy);
	}
	assert(!"Something is wrong!");
	return(NULL);
}

ICompoundObject* XGetObjectParent(IXEditorObject *pObject)
{
	const Map<IXEditorObject*, ICompoundObject*>::Node *pNode;
	if(g_mObjectsLocation.KeyExists(pObject, &pNode))
	{
		return(*pNode->Val);
	}
	return(NULL);
}

IXEditorObject* XFindObjectByGUID(const XGUID &guid)
{
	IXEditorObject *pFoundObj = NULL;
	XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, ICompoundObject *pParent){
		if(*pObj->getGUID() == guid)
		{
			pFoundObj = pObj;
			return(XEOR_STOP);
		}
		return(XEOR_CONTINUE);
	});

	return(pFoundObj);
}

void BeginMaterialEdit(const char *szMaterialName)
{
	extern HINSTANCE hInst;

	IXCore *pCore = g_pEngine->getCore();

	IXMaterial *pMat;
	((IXMaterialSystem*)pCore->getPluginManager()->getInterface(IXMATERIALSYSTEM_GUID))->loadMaterial(szMaterialName, &pMat);
	new CMaterialEditor(hInst, g_hWndMain, pCore, pMat);
}

void XSetCameraRotation(const float3 &vPitchYawRoll)
{
	g_pEngineCallback->setCameraRotation(vPitchYawRoll);
}
