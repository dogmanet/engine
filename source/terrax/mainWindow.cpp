

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <windowsx.h>

#include "resource.h"
#include <commctrl.h>
#include <combaseapi.h>
#include <shellapi.h>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#include "LevelOpenDialog.h"

#include <uxtheme.h>
#pragma comment(lib, "UxTheme.lib")

#include <skyxengine.h>
#include <core/sxcore.h>
#include <gcore/sxgcore.h>
#include <render/sxrender.h>
#include <input/sxinput.h>
//#include <sxguiwinapi/sxgui.h>
//#include <level/sxlevel.h>

#include "terrax.h"
#include <xcommon/editor/IXEditorObject.h>
#include <xcommon/editor/IXEditable.h>
#include "UndoManager.h"

#include "CommandSelect.h"
#include "CommandMove.h"
#include "CommandScale.h"
#include "CommandRotate.h"
#include "CommandDelete.h"
#include "CommandCreate.h"
#include "CommandProperties.h"
#include "CommandPaste.h"
#include "CommandBuildModel.h"
#include "CommandDestroyModel.h"
#include "CommandModifyModel.h"

#include "PropertyWindow.h"

#include "Tools.h"

#define CLIPBOARD_FILE "TerraX.clipboard"
char g_szClipboardFile[MAX_PATH + sizeof(CLIPBOARD_FILE)];

#include <gui/guimain.h>

extern Array<IXEditorObject*> g_pLevelObjects;
extern AssotiativeArray<AAString, IXEditable*> g_mEditableSystems;

CMaterialBrowser *g_pMaterialBrowser = NULL;

// Global Variables:
HINSTANCE hInst;								// current instance
HWND g_hWndMain = NULL;
UINT g_uWndMainDpi = 96;
HFONT g_hWndMainFont = NULL;
HWND g_hTopRightWnd = NULL;
HWND g_hTopLeftWnd = NULL;
HWND g_hBottomRightWnd = NULL;
HWND g_hBottomLeftWnd = NULL;
HWND g_hStatusWnd = NULL;
HWND g_hStaticCurrentMatWnd = NULL;
HWND g_hStaticCurrentMatSizeWnd = NULL;
HWND g_hButtonCurrentMatBrowseWnd = NULL;
HWND g_hButtonToWorldWnd = NULL;
HWND g_hButtonToEntityWnd = NULL;
//HWND g_hObjectTreeWnd = NULL;
HWND g_hComboTypesWnd = NULL;
HWND g_hComboCurrentMatWnd = NULL;
HWND g_hStaticTypesWnd = NULL;
HWND g_hComboClassesWnd = NULL;
HWND g_hStaticClassesWnd = NULL;
HWND g_hStaticMoveSelectedToWnd = NULL;
HWND g_hCheckboxRandomScaleYawWnd = NULL;
HWND g_hCurMatWnd = NULL;

HWND g_hABArrowButton = NULL;
HWND g_hABCameraButton = NULL;
HWND g_hABCreateButton = NULL;

UINT g_uABNextTop = 0;
UINT g_uABNextLeft = 0;

BOOL g_isXResizeable = TRUE;
BOOL g_isYResizeable = TRUE;

BOOL g_is3DRotating = FALSE;
BOOL g_is3DPanning = FALSE;

BOOL g_is2DPanning = FALSE;

HWND g_hToolbarWnd = NULL;

BOOL g_isPropWindowVisible = FALSE;
CPropertyWindow *g_pPropWindow = NULL;

extern HACCEL g_hAccelTableMain;
extern HACCEL g_hAccelTableEdit;

HMENU g_hMenu = NULL;
HMENU g_hMenu2 = NULL;

HWND g_pGuiWnd = NULL;
gui::IGUI *g_pGUI = NULL;

CTerraXConfig g_xConfig;
CTerraXState g_xState;

bool g_bViewportCaptionDirty[4];

extern CUndoManager *g_pUndoManager;

extern Array<IXEditable*> g_pEditableSystems;

extern String g_sLevelName;

struct CustomTool
{
	IXEditorTool *pTool;
	XAccelItem accel;
	IXEditable *pEditable;
	String sSelectedClass;
};
Array<CustomTool> g_aTools;
String g_sSelectedType;
String g_sSelectedClass;
IXEditorTool *g_pCurrentTool = NULL;
UINT g_uCurrentTool = IDC_AB_ARROW;

bool g_isCurMatDirty = false;

CGizmoMoveCallback g_gizmoMoveCallback;
CGizmoRotateCallback g_gizmoRotateCallback;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RenderWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RenderNoninteractiveWndProc(HWND, UINT, WPARAM, LPARAM);
void DisplayContextMenu(HWND hwnd, POINT pt, HMENU hMenu, int iSubmenu, int iCheckItem = -1);
void XInitViewportLayout(X_VIEWPORT_LAYOUT layout);
BOOL XCheckMenuItem(HMENU hMenu, UINT uIDCheckItem, bool bCheck);
void XUpdateStatusGrid();
void XUpdateStatusMPos();
void XUpdateUndoRedo();
HWND CreateToolbar(HWND hWndParent);
HBITMAP StretchBitmap(HDC hDC, HBITMAP hBitmap);

UINT GetWindowDPI(HWND hWnd);
int DivDpi(int iUnscaled, UINT uCurrentDpi);
int MulDpi(int iUnscaled, UINT uCurrentDpi);
void DivDpiRect(RECT *pRc, UINT uCurrentDpi);
void MulDpiRect(RECT *pRc, UINT uCurrentDpi);

HFONT GetDefaultFont(UINT uDpi);
void ReleaseDefaultFont(HFONT hFont);

class CPropertyCallback: public CPropertyWindow::ICallback
{
public:
	void onClassChanged(const char *szNewClassName) override
	{

	}
	bool onPropertyChanged(const char *szKey, const char *szValue) override
	{
		m_pPropsCmd->setKV(szKey, szValue);
		return(true);
	}
	void onCancel() override
	{
		if(m_pPropsCmd)
		{
			CCommandProperties *pPropsCmd = m_pPropsCmd;
			m_pPropsCmd = NULL;
			pPropsCmd->undo();
			mem_delete(pPropsCmd);
		}
	}
	void onApply() override
	{
		if(m_pPropsCmd)
		{
			CCommandProperties *pPropsCmd = m_pPropsCmd;
			m_pPropsCmd = NULL;
			XExecCommand(pPropsCmd);
			//g_pUndoManager->execCommand(pPropsCmd);
		}
	}

	void start()
	{
		onApply();
		
		m_pPropsCmd = new CCommandProperties();
		XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
			if(pObj->isSelected() && (g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent))
			{
				m_pPropsCmd->addObject(pObj);
			}
		});
		
		for(UINT i = 0, l = g_pPropWindow->getCustomTabCount(); i < l; ++i)
		{
			auto *pTab = g_pPropWindow->getCustomTab(i);
			if(pTab->isEnabled())
			{
				m_pPropsCmd->addInnerCommand(pTab->startNewCommand());
			}
		}
	}
protected:
	CCommandProperties *m_pPropsCmd = NULL;
};

//#############################################################################

void CMatBrowserCallback::init(IXMaterialSystem *pMatSys)
{
	m_pMaterialSystem = pMatSys;
	if(m_pMaterialSystem->getScannedMaterialsCount())
	{
		onSelected(m_pMaterialSystem->getScannedMaterial(0, NULL));
	}
	else
	{
		onSelected("dev_null");
	}
}
void CMatBrowserCallback::onSelected(const char *szName)
{
	pushHistory(szName);

	m_sSelected = szName;

	mem_release(m_pMat);
	mem_release(m_pTex);
	IKeyIterator *pIter = NULL;

	m_pMaterialSystem->loadMaterial(szName, &m_pMat);

	const char *szTexture = m_pMat->getTextureName("txBase");
	if(!szTexture)
	{
		pIter = m_pMat->getTexturesIterator();
		if(pIter)
		{
			szTexture = pIter->getCurrent();
		}
	}

	if(szTexture)
	{
		m_pMaterialSystem->loadTexture(szTexture, &m_pTex);
	}

	mem_release(pIter);

	if(m_pTex)
	{
		UINT w = m_pTex->getWidth();
		UINT h = m_pTex->getHeight();

		char tmp[64];
		sprintf(tmp, "%ux%u", w, h);
		SetWindowText(g_hStaticCurrentMatSizeWnd, tmp);
	}

	g_isCurMatDirty = true;
}

void CMatBrowserCallback::getInfo(IXMaterial **ppMat, IXTexture **ppTex)
{
	add_ref(m_pMat);
	*ppMat = m_pMat;
	add_ref(m_pTex);
	*ppTex = m_pTex;
}

void CMatBrowserCallback::pushHistory(const char *szMat)
{
	int idx = ComboBox_FindStringExact(g_hComboCurrentMatWnd, -1, szMat);

	if(idx >= 0)
	{
		ComboBox_DeleteString(g_hComboCurrentMatWnd, idx);
	}
	ComboBox_InsertString(g_hComboCurrentMatWnd, 0, szMat);
	ComboBox_SetCurSel(g_hComboCurrentMatWnd, 0);

	if(ComboBox_GetCount(g_hComboCurrentMatWnd) > 20)
	{
		ComboBox_DeleteString(g_hComboCurrentMatWnd, 20);
	}
}

const char* CMatBrowserCallback::getSelection()
{
	return(m_sSelected.c_str());
}

UINT CMatBrowserCallback::getRecentMaterialCount()
{
	return(ComboBox_GetCount(g_hComboCurrentMatWnd));
}
const char* CMatBrowserCallback::getRecentMaterial(UINT idx)
{
	int iLen = ComboBox_GetLBTextLen(g_hComboCurrentMatWnd, idx);
	char *szMatName = (char*)alloca(sizeof(char)* (iLen + 1));
	ComboBox_GetLBText(g_hComboCurrentMatWnd, idx, szMatName);

	m_aTemp.clearFast();
	m_aTemp.reserve(iLen + 1);
	m_aTemp.append(szMatName);
	m_aTemp.push_back(0);
	return(m_aTemp);
}

//#############################################################################

CMatBrowserCallback g_matBrowserCallback;

CPropertyCallback g_propertyCallback;

ATOM XRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	HBRUSH	hBrush = NULL;

	// Resetting the structure members before use
	memset(&wcex, 0, sizeof(WNDCLASSEX));

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_LOGO));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);

	hBrush = CreateSolidBrush(RGB(240, 240, 240));


	wcex.hbrBackground = (HBRUSH)hBrush;
	//	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_SPLITTER_WND);
	wcex.lpszClassName = MAIN_WINDOW_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	if(!RegisterClassEx(&wcex))
	{
		return(FALSE);
	}


	wcex.lpfnWndProc = RenderNoninteractiveWndProc;
	hBrush = CreateSolidBrush(RGB(0, 0, 0));
	wcex.hbrBackground = (HBRUSH)hBrush;
	wcex.lpszClassName = RENDER_NONINTERACTIVE_WINDOW_CLASS;

	if(!RegisterClassEx(&wcex))
	{
		return(FALSE);
	}


	wcex.style |= CS_DBLCLKS;
	// Intilaizing the class for the viewport window
	wcex.lpfnWndProc = RenderWndProc;
	hBrush = CreateSolidBrush(RGB(255, 255, 255));
	wcex.hbrBackground = (HBRUSH)hBrush;
	wcex.lpszClassName = RENDER_WINDOW_CLASS;

	if(!RegisterClassEx(&wcex))
	{
		return(FALSE);
	}

	return(TRUE);
}

BOOL XInitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	// Varible used to postion the window, center to the desktop
	UINT nx_size = GetSystemMetrics(SM_CXSCREEN);
	UINT ny_size = GetSystemMetrics(SM_CYSCREEN);

	UINT nx_pos = (nx_size - WINDOW_WIDTH) / 2;
	UINT ny_pos = (ny_size - WINDOW_HEIGHT) / 2;

	g_hWndMain = CreateWindowA(MAIN_WINDOW_CLASS, MAIN_WINDOW_TITLE " | " SKYXENGINE_VERSION4EDITORS, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_OVERLAPPEDWINDOW, nx_pos, ny_pos, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, g_hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1)), hInstance, NULL);

	g_hMenu2 = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU2));

	if(!g_hWndMain)
	{
		UINT ret_val;

		ret_val = GetLastError();
		return FALSE;
	}

	WINDOWPLACEMENT wp = {sizeof(wp)};
	DWORD dwKeyLen = wp.length;
	if(ERROR_SUCCESS == RegGetValueA(HKEY_CURRENT_USER, "SOFTWARE\\DogmaNet\\TerraX", "WinPos", RRF_RT_REG_BINARY, NULL, (BYTE*)&wp, &dwKeyLen) && wp.length == sizeof(wp))
	{
		SetWindowPlacement(g_hWndMain, &wp);
	}
	else
	{
		ShowWindow(g_hWndMain, nCmdShow == SW_SHOWDEFAULT ? SW_MAXIMIZE : nCmdShow);
	}

	UpdateWindow(g_hWndMain);

	g_hAccelTableMain = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
	g_hAccelTableEdit = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR2));

	XCheckMenuItem(g_hMenu, ID_VIEW_GRID, g_xConfig.m_bShowGrid);

	GetTempPathA(sizeof(g_szClipboardFile), g_szClipboardFile);
	strcat(g_szClipboardFile, CLIPBOARD_FILE);

	return(TRUE);
}

bool IsEditMessage()
{
	HWND hFocused = GetFocus();
	char className[6];
	GetClassName(hFocused, className, 6);
	return(hFocused && !strcasecmp(className, "edit"));
}

bool IsButtonMessage()
{
	HWND hFocused = GetFocus();
	char className[8];
	GetClassName(hFocused, className, 8);
	return(hFocused && !strcasecmp(className, "button"));
}

WNDPROC g_pfnTreeOldWndproc;
LRESULT CALLBACK TreeViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message == WM_MOUSEWHEEL)
	{
		RECT rc;
		GetClientRect(hWnd, &rc);
		POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
		if(pt.x < rc.left || pt.x > rc.right || pt.y < rc.top || pt.y > rc.bottom)
		{
			SendMessage(g_hWndMain, message, wParam, lParam);
			return(0);
		}
	}
	return(CallWindowProc(g_pfnTreeOldWndproc, hWnd, message, wParam, lParam));
}

WNDPROC g_pfnClassesComboOldWndproc;
LRESULT CALLBACK ClassesComboWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message == WM_MOUSEWHEEL)
	{
		COMBOBOXINFO info = {sizeof(COMBOBOXINFO)};
		GetComboBoxInfo(hWnd, &info);

		if(!IsWindowVisible(info.hwndList))
		{
			RECT rc;
			GetClientRect(hWnd, &rc);
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			if(pt.x < rc.left || pt.x > rc.right || pt.y < rc.top || pt.y > rc.bottom)
			{
				SendMessage(g_hWndMain, message, wParam, lParam);
				return(0);
			}
		}
	}
	return(CallWindowProc(g_pfnClassesComboOldWndproc, hWnd, message, wParam, lParam));
}

WNDPROC g_pfnStatusBarOldWndproc;
LRESULT CALLBACK StatusBarWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message == WM_SIZE)
	{
		// 200 100 100 150
		// selection, world position, lwh, zoom, grid

		RECT rcClient;
		int iParts = 5;

		// Get the coordinates of the parent window's client area.
		GetClientRect(GetParent(hWnd), &rcClient);

		// Allocate an array for holding the right edge coordinates.
		HLOCAL hloc = LocalAlloc(LHND, sizeof(int) * iParts);
		int *paParts = (int*)LocalLock(hloc);

		// Calculate the right edge coordinate for each part, and
		// copy the coordinates to the array.
		paParts[0] = 0;
		paParts[1] = MulDpi(200, g_uWndMainDpi);
		paParts[2] = MulDpi(100, g_uWndMainDpi);
		paParts[3] = MulDpi(100, g_uWndMainDpi);
		paParts[4] = MulDpi(150, g_uWndMainDpi);
		int iTotal = 0;
		for(int i = 0; i < iParts; ++i)
		{
			iTotal += paParts[i];
		}
		paParts[0] = rcClient.right - iTotal;
		for(int i = 1; i < iParts; ++i)
		{
			paParts[i] += paParts[i - 1];
		}

		// Tell the status bar to create the window parts.
		SendMessage(hWnd, SB_SETPARTS, (WPARAM)iParts, (LPARAM)paParts);
		// Free the array, and return.
		LocalUnlock(hloc);
		LocalFree(hloc);
	}
	return(CallWindowProc(g_pfnStatusBarOldWndproc, hWnd, message, wParam, lParam));
}

static void DeleteSelection()
{
	CCommandDelete *pDelCmd = new CCommandDelete();
	XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
		if(pObj->isSelected() && (g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent))
		{
			pDelCmd->addObject(pObj);
		}
	});
	XExecCommand(pDelCmd);
}

static void ExportObject(IXConfig *pConfig, IXEditorObject *pObj, UINT &uCount)
{
	char szSection[64], szTmp[64];

	sprintf(szSection, "obj_%u_type", uCount);
	pConfig->set("meta", szSection, pObj->getTypeName());

	sprintf(szSection, "obj_%u_class", uCount);
	pConfig->set("meta", szSection, pObj->getClassName());

	float3_t vTmp = pObj->getPos();
	sprintf(szSection, "obj_%u_pos", uCount);
	sprintf(szTmp, "%f %f %f", vTmp.x, vTmp.y, vTmp.z);
	pConfig->set("meta", szSection, szTmp);

	float3 vMin, vMax;
	pObj->getBound(&vMin, &vMax);
	vTmp = (float3)(vMax - vMin);
	sprintf(szSection, "obj_%u_size", uCount);
	sprintf(szTmp, "%f %f %f", vTmp.x, vTmp.y, vTmp.z);
	pConfig->set("meta", szSection, szTmp);

	// vTmp = pObj->getScale();
	// sprintf(szSection, "obj_%u_scale", uCount);
	// sprintf(szTmp, "%f %f %f", vTmp.x, vTmp.y, vTmp.z);
	// pConfig->set("meta", szSection, szTmp);

	SMQuaternion qTmp = pObj->getOrient();
	sprintf(szSection, "obj_%u_orient", uCount);
	sprintf(szTmp, "%f %f %f %f", qTmp.x, qTmp.y, qTmp.z, qTmp.w);
	pConfig->set("meta", szSection, szTmp);

	sprintf(szSection, "obj_%u_guid", uCount);
	XGUIDToSting(*pObj->getGUID(), szTmp, sizeof(szTmp));
	pConfig->set("meta", szSection, szTmp);

	sprintf(szSection, "obj_%u", uCount);


	for(UINT i = 0, l = pObj->getProperyCount(); i < l; ++i)
	{
		const X_PROP_FIELD *pField = pObj->getPropertyMeta(i);

		if(!pField->isGeneric)
		{
			pConfig->set(szSection, pField->szKey, pObj->getKV(pField->szKey));
		}
	}

	++uCount;

	void *isProxy = NULL;
	pObj->getInternalData(&X_IS_PROXY_GUID, &isProxy);
	if(isProxy)
	{
		// export nested objects
		CProxyObject *pProxy = (CProxyObject*)pObj;
		for(UINT i = 0, l = pProxy->getObjectCount(); i < l; ++i)
		{
			ExportObject(pConfig, pProxy->getObject(i), uCount);
		}
	}
}

static void ToClipboard(bool isCut = false)
{
	IXConfig *pConfig = g_pEngine->getCore()->newConfig();
	pConfig->open(g_szClipboardFile);
	pConfig->clear();

	UINT uCount = 0;
	char szSection[64], szTmp[64], szKey[64];

	for(UINT i = 0, l = g_pLevelObjects.size(); i < l; ++i)
	{
		IXEditorObject *pObj = g_pLevelObjects[i];
		if(pObj->isSelected())
		{
			ExportObject(pConfig, pObj, uCount);
		}
	}

	sprintf(szSection, "%u", uCount);
	pConfig->set("meta", "count", szSection);


	uCount = 0;
	for(UINT i = 0, l = g_apProxies.size(); i < l; ++i)
	{
		CProxyObject *pObj = g_apProxies[i];
		if(pObj->isSelected())
		{
			sprintf(szSection, "proxy_%u", uCount);

			XGUIDToSting(*pObj->getGUID(), szTmp, sizeof(szTmp));
			pConfig->set(szSection, "guid", szTmp);

			UINT uObjCount = 0;
			sprintf(szTmp, "%u", pObj->getObjectCount());
			pConfig->set(szSection, "o_count", szTmp);

			for(UINT i = 0, l = pObj->getObjectCount(); i < l; ++i)
			{
				XGUIDToSting(*pObj->getObject(i)->getGUID(), szTmp, sizeof(szTmp));
				sprintf(szKey, "o_%u", uObjCount);
				pConfig->set(szSection, szKey, szTmp);
				++uObjCount;
			}

			++uCount;
		}
	}

	sprintf(szSection, "%u", uCount);
	pConfig->set("meta", "proxy_count", szSection);


	sprintf(szSection, "%f %f %f", g_xState.vSelectionBoundMin.x, g_xState.vSelectionBoundMin.y, g_xState.vSelectionBoundMin.z);
	pConfig->set("meta", "aabb_min", szSection);
	sprintf(szSection, "%f %f %f", g_xState.vSelectionBoundMax.x, g_xState.vSelectionBoundMax.y, g_xState.vSelectionBoundMax.z);
	pConfig->set("meta", "aabb_max", szSection);

	pConfig->save();
	mem_release(pConfig);

	if(isCut)
	{
		DeleteSelection();
	}
}

static float GetPasteCenter(char axis, X_WINDOW_POS skipPos)
{
	for(UINT i = 0; i < 4; ++i)
	{
		if(i != skipPos)
		{
			X_2D_VIEW x2dView = g_xConfig.m_x2DView[i];
			float3 vCamPos = g_xConfig.m_pViewportCamera[i]->getPosition();
			switch(axis)
			{
			case 'x':
				if(x2dView == X2D_TOP || x2dView == X2D_FRONT)
				{
					return(vCamPos.x);
				}
				break;

			case 'y':
				if(x2dView == X2D_FRONT || x2dView == X2D_SIDE)
				{
					return(vCamPos.y);
				}
				break;

			case 'z':
				if(x2dView == X2D_TOP || x2dView == X2D_SIDE)
				{
					return(vCamPos.z);
				}
				break;

			default:
				assert(!"Invalid axis");
			}
		}
	}

	return(0.0f);
}

static float3 GetPasteCenter()
{
	X_2D_VIEW x2dView = g_xConfig.m_x2DView[g_xState.activeWindow];

	float3 vPos;

	switch(x2dView)
	{
	case X2D_TOP:
		vPos.x = g_xState.vWorldMousePos.x;
		vPos.y = GetPasteCenter('y', g_xState.activeWindow);
		vPos.z = g_xState.vWorldMousePos.y;
		break;

	case X2D_FRONT:
		vPos.x = g_xState.vWorldMousePos.x;
		vPos.y = g_xState.vWorldMousePos.y;
		vPos.z = GetPasteCenter('z', g_xState.activeWindow);
		break;

	case X2D_SIDE:
		vPos.x = GetPasteCenter('x', g_xState.activeWindow);
		vPos.y = g_xState.vWorldMousePos.y;
		vPos.z = g_xState.vWorldMousePos.x;
		break;

	default:
		vPos.x = GetPasteCenter('x', XWP_TOP_LEFT);
		vPos.y = GetPasteCenter('y', XWP_TOP_LEFT);
		vPos.z = GetPasteCenter('z', XWP_TOP_LEFT);
		break;
	}

	return(XSnapToGrid(vPos));
}

static void FromClipboard()
{
	IXConfig *pConfig = g_pEngine->getCore()->newConfig();
	if(pConfig->open(g_szClipboardFile))
	{
		const char *szVal = pConfig->getKey("meta", "count");
		if(szVal)
		{
			UINT uCount = 0;
			sscanf(szVal, "%u", &uCount);

			SMAABB aabb;

			szVal = pConfig->getKey("meta", "aabb_min");
			if(szVal)
			{
				sscanf(szVal, "%f %f %f", &aabb.vMin.x, &aabb.vMin.y, &aabb.vMin.z);
			}
			szVal = pConfig->getKey("meta", "aabb_max");
			if(szVal)
			{
				sscanf(szVal, "%f %f %f", &aabb.vMax.x, &aabb.vMax.y, &aabb.vMax.z);
			}

			aabb.vMax = SMVectorMax(aabb.vMin, aabb.vMax);
			float3 vClipboardCenter = (aabb.vMin + aabb.vMax) * 0.5f;
			float3 vPasteCenter = GetPasteCenter();
			float3 vDelta = vPasteCenter - vClipboardCenter;

			float3 vPos, vScale;
			SMQuaternion qRot;

			CCommandPaste *pCmd = new CCommandPaste();

			//
			XGUID guid;

			char szSection[64];
			for(UINT i = 0; i < uCount; ++i)
			{
				sprintf(szSection, "obj_%u_type", i);
				const char *szTypeName = pConfig->getKey("meta", szSection);
				sprintf(szSection, "obj_%u_class", i);
				const char *szClassName = pConfig->getKey("meta", szSection);

				if(!szTypeName || !szClassName)
				{
					continue;
				}

				sprintf(szSection, "obj_%u_pos", i);
				const char *szTmp = pConfig->getKey("meta", szSection);
				if(szTmp)
				{
					sscanf(szTmp, "%f %f %f", &vPos.x, &vPos.y, &vPos.z);
				}

				//sprintf(szSection, "obj_%u_scale", i);
				//szTmp = pConfig->getKey("meta", szSection);
				//if(szTmp)
				//{
				//	sscanf(szTmp, "%f %f %f", &vScale.x, &vScale.y, &vScale.z);
				//}

				sprintf(szSection, "obj_%u_size", i);
				szTmp = pConfig->getKey("meta", szSection);
				if(szTmp)
				{
					sscanf(szTmp, "%f %f %f", &vScale.x, &vScale.y, &vScale.z);
				}

				sprintf(szSection, "obj_%u_orient", i);
				szTmp = pConfig->getKey("meta", szSection);
				if(szTmp)
				{
					sscanf(szTmp, "%f %f %f %f", &qRot.x, &qRot.y, &qRot.z, &qRot.w);
				}

				sprintf(szSection, "obj_%u_guid", i);
				szTmp = pConfig->getKey("meta", szSection);
				if(!szTmp || !XGUIDFromString(&guid, szTmp))
				{
					guid = XGUID();
				}

				sprintf(szSection, "obj_%u", i);


				UINT uObj = pCmd->addObject(szTypeName, szClassName, vPos + vDelta, vScale, qRot, guid);
				if(uObj != UINT_MAX)
				{
					for(UINT j = 0, jl = pConfig->getKeyCount(szSection); j < jl; ++j)
					{
						const char *szKey = pConfig->getKeyName(szSection, j);
						const char *szValue = pConfig->getKey(szSection, szKey);

						pCmd->setKV(uObj, szKey, szValue);
					}
				}
			}

			szVal = pConfig->getKey("meta", "proxy_count");
			if(szVal)
			{
				sscanf(szVal, "%u", &uCount);
				for(UINT i = 0; i < uCount; ++i)
				{
					sprintf(szSection, "proxy_%u", i);
					const char *szTmp;
					XGUID guid;
					UINT uObjCount;
					if(
						(szTmp = pConfig->getKey(szSection, "guid")) && XGUIDFromString(&guid, szTmp)
						&& (szTmp = pConfig->getKey(szSection, "o_count")) && sscanf(szTmp, "%u", &uObjCount)
						)
					{
						char szKey[64];
						//szTmp = pConfig->getKey(szSection, "o_count");
						UINT uProxy = pCmd->addProxy(guid);
						for(UINT j = 0; j < uObjCount; ++j)
						{
							sprintf(szKey, "o_%u", j);
							if((szTmp = pConfig->getKey(szSection, szKey)) && XGUIDFromString(&guid, szTmp))
							{
								pCmd->addProxyObject(uProxy, guid);
							}
						}
					}
					/*
					[proxy_0]
o_count = 2
o_1 = {4307AD0F-A496-4960-B1C8-0595380483E3}
o_0 = {4F36E63D-DC90-4F5A-B6B4-DB3ECB48E3FF}
guid = {9D7D2E62-24C7-42B7-8D83-8448FC4604F0}
					*/

				}
			}

			XExecCommand(pCmd);
		}
	}

	// create objects

	mem_release(pConfig);
}

static CCommandCreate* CreateObjectAtPosition(const float3 &vPos, bool bDeselectAll)
{
	int iSel1 = ComboBox_GetCurSel(g_hComboTypesWnd);
	int iLen1 = ComboBox_GetLBTextLen(g_hComboTypesWnd, iSel1);
	char *szTypeName = (char*)alloca(sizeof(char)* (iLen1 + 1));
	ComboBox_GetLBText(g_hComboTypesWnd, iSel1, szTypeName);

	int iSel2 = ComboBox_GetCurSel(g_hComboClassesWnd);
	int iLen2 = ComboBox_GetLBTextLen(g_hComboClassesWnd, iSel2);
	char *szClassName = (char*)alloca(sizeof(char)* (iLen2 + 1));
	ComboBox_GetLBText(g_hComboClassesWnd, iSel2, szClassName);

	if(bDeselectAll)
	{
		SendMessage(g_hWndMain, WM_COMMAND, MAKEWPARAM(ID_EDIT_CLEARSELECTION, 0), (LPARAM)0);
	}

	CCommandCreate *pCmd = new CCommandCreate(vPos, szTypeName, szClassName, Button_GetCheck(g_hCheckboxRandomScaleYawWnd));
	g_pUndoManager->execCommand(pCmd);

	return(pCmd);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT rect;
	PAINTSTRUCT ps;
	HDC hdc;
	static HCURSOR	hcSizeEW = NULL;
	static HCURSOR	hcSizeNS = NULL;
	static HCURSOR	hcSizeNESW = NULL;

	

	static BOOL xSizing;
	static BOOL ySizing;

	static bool shouldShowProps = false;

	static float fCoeffWidth = 0.0f;
	static float fCoeffHeight = 0.0f;
	static int iLeftWidth = 0;
	static int iTopHeight = 0;
	
	IMSG msg;
	msg.lParam = lParam;
	msg.wParam = wParam;
	msg.message = message;

	SSInput_AddMsg(msg);

	switch(message)
	{
	case WM_CREATE:
	{
		g_uWndMainDpi = GetWindowDPI(hWnd);
		g_hWndMainFont = GetDefaultFont(g_uWndMainDpi);

		hcSizeEW = LoadCursor(NULL, IDC_SIZEWE);
		hcSizeNS = LoadCursor(NULL, IDC_SIZENS);
		hcSizeNESW = LoadCursor(NULL, IDC_SIZEALL);

		//	g_hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU1));
		//	SetMenu(g_hWndMain, g_hMenu); 
		//	DrawMenuBar(g_hWndMain);


		GetClientRect(hWnd, &rect);

		rect.top += MARGIN_TOP;
		rect.bottom -= MARGIN_BOTTOM;
		rect.left += MARGIN_LEFT;
		rect.right -= MARGIN_RIGHT;

		iLeftWidth = (rect.right - rect.left - SPLITTER_BAR_WIDTH) / 2;
		iTopHeight = (rect.bottom - rect.top - SPLITTER_BAR_WIDTH) / 2;

		fCoeffWidth = (float)iLeftWidth / (float)(rect.right - rect.left);
		fCoeffHeight = (float)iTopHeight / (float)(rect.bottom - rect.top);

		// Creates the left window using the width and height read from the XML 
		// files
		g_hTopLeftWnd = CreateWindowExA(WS_EX_CLIENTEDGE, RENDER_WINDOW_CLASS, "", WS_CHILD | WS_VISIBLE | SS_SUNKEN, MulDpi(rect.left, g_uWndMainDpi), MulDpi(rect.top, g_uWndMainDpi), MulDpi(iLeftWidth, g_uWndMainDpi), MulDpi(iTopHeight, g_uWndMainDpi), hWnd, NULL, hInst, NULL);
		if(g_hTopLeftWnd)
		{
			ShowWindow(g_hTopLeftWnd, SW_SHOW);
			//	UpdateWindow(g_hTopLeftWnd);
		}

		g_hBottomLeftWnd = CreateWindowExA(WS_EX_CLIENTEDGE, RENDER_WINDOW_CLASS, "", WS_CHILD | WS_VISIBLE | SS_SUNKEN, MulDpi(rect.left, g_uWndMainDpi), MulDpi(rect.top + iTopHeight + SPLITTER_BAR_WIDTH, g_uWndMainDpi), MulDpi(iLeftWidth, g_uWndMainDpi), MulDpi(iTopHeight, g_uWndMainDpi), hWnd, NULL, hInst, NULL);
		if(g_hTopLeftWnd)
		{
			ShowWindow(g_hTopLeftWnd, SW_SHOW);
			//	UpdateWindow(g_hTopLeftWnd);
		}

		g_hTopRightWnd = CreateWindowExA(WS_EX_CLIENTEDGE, RENDER_WINDOW_CLASS, "", WS_CHILD | WS_VISIBLE | SS_SUNKEN, MulDpi(rect.left + iLeftWidth + SPLITTER_BAR_WIDTH, g_uWndMainDpi), MulDpi(rect.top, g_uWndMainDpi), MulDpi(iLeftWidth, g_uWndMainDpi), MulDpi(iTopHeight, g_uWndMainDpi), hWnd, NULL, hInst, NULL);
		if(g_hTopRightWnd)
		{
			ShowWindow(g_hTopRightWnd, SW_SHOW);
			//	UpdateWindow(g_hTopLeftWnd);
		}

		g_hBottomRightWnd = CreateWindowExA(WS_EX_CLIENTEDGE, RENDER_WINDOW_CLASS, "", WS_CHILD | WS_VISIBLE | SS_SUNKEN, MulDpi(rect.left + iLeftWidth + SPLITTER_BAR_WIDTH, g_uWndMainDpi), MulDpi(rect.top + iTopHeight + SPLITTER_BAR_WIDTH, g_uWndMainDpi), MulDpi(iLeftWidth, g_uWndMainDpi), MulDpi(iTopHeight, g_uWndMainDpi), hWnd, NULL, hInst, NULL);
		if(g_hBottomRightWnd)
		{
			ShowWindow(g_hBottomRightWnd, SW_SHOW);
			//	UpdateWindow(g_hTopLeftWnd);
		}

		g_hStatusWnd = CreateWindowEx(0, STATUSCLASSNAME, "For help, press F1", SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, (HMENU)IDC_STATUSBAR, hInst, NULL);                   // no window creation data
		{
			g_pfnStatusBarOldWndproc = (WNDPROC)GetWindowLongPtr(g_hStatusWnd, GWLP_WNDPROC);
			SetWindowLongPtr(g_hStatusWnd, GWLP_WNDPROC, (LONG_PTR)StatusBarWndProc);
			SendMessage(g_hStatusWnd, WM_SIZE, 0, 0);
			XUpdateStatusGrid();
		}
		g_uABNextTop = rect.top;
		g_uABNextLeft = rect.left - MARGIN_LEFT;
		g_hABArrowButton = CreateWindow("Button", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_BITMAP | BS_PUSHLIKE | BS_CHECKBOX, MulDpi(g_uABNextLeft, g_uWndMainDpi), MulDpi(g_uABNextTop, g_uWndMainDpi), MulDpi(AB_BUTTON_WIDTH, g_uWndMainDpi), MulDpi(AB_BUTTON_HEIGHT, g_uWndMainDpi), hWnd, (HMENU)IDC_AB_ARROW, hInst, NULL);
		{
			SetWindowTheme(g_hABArrowButton, L" ", L" ");
			HBITMAP hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP30));
			SendMessage(g_hABArrowButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
			Button_SetCheck(g_hABArrowButton, TRUE);
		}

		//WS_EX_DLGMODALFRAME

		g_uABNextTop += AB_BUTTON_HEIGHT;
		g_hABCameraButton = CreateWindow("Button", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_BITMAP | BS_PUSHLIKE | BS_CHECKBOX, MulDpi(g_uABNextLeft, g_uWndMainDpi), MulDpi(g_uABNextTop, g_uWndMainDpi), MulDpi(AB_BUTTON_WIDTH, g_uWndMainDpi), MulDpi(AB_BUTTON_HEIGHT, g_uWndMainDpi), hWnd, (HMENU)IDC_AB_CAMERA, hInst, NULL);
		{
			SetWindowTheme(g_hABCameraButton, L" ", L" ");
			HBITMAP hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP31));
			SendMessage(g_hABCameraButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
		}

		g_uABNextTop += AB_BUTTON_HEIGHT;
		g_hABCreateButton = CreateWindow("Button", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_BITMAP | BS_PUSHLIKE | BS_CHECKBOX, MulDpi(g_uABNextLeft, g_uWndMainDpi), MulDpi(g_uABNextTop, g_uWndMainDpi), MulDpi(AB_BUTTON_WIDTH, g_uWndMainDpi), MulDpi(AB_BUTTON_HEIGHT, g_uWndMainDpi), hWnd, (HMENU)IDC_AB_CREATE, hInst, NULL);
		{
			SetWindowTheme(g_hABCreateButton, L" ", L" ");
			HBITMAP hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP34));
			SendMessage(g_hABCreateButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
		}
		g_uABNextTop += AB_BUTTON_HEIGHT;


		g_hStaticCurrentMatWnd = CreateWindowExA(0, WC_STATIC, "Current material", WS_VISIBLE | WS_CHILD, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(15, g_uWndMainDpi), hWnd, 0, hInst, NULL);
		{
			SetWindowFont(g_hStaticCurrentMatWnd, g_hWndMainFont, FALSE);
		}

		g_hComboCurrentMatWnd = CreateWindowExA(WS_EX_RIGHT, WC_COMBOBOX, "", WS_VISIBLE | WS_CHILD | WS_BORDER | /*CBS_SORT | */CBS_DROPDOWNLIST | CBS_HASSTRINGS, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(OBJECT_TREE_HEIGHT, g_uWndMainDpi), hWnd, (HMENU)ID_CMB_MAT, hInst, NULL);
		{
			SetWindowFont(g_hComboCurrentMatWnd, g_hWndMainFont, FALSE);

			//g_pfnClassesComboOldWndproc = (WNDPROC)GetWindowLongPtr(g_hComboTypesWnd, GWLP_WNDPROC);
			//SetWindowLongPtr(g_hComboCurrentMatWnd, GWLP_WNDPROC, (LONG_PTR)ClassesComboWndProc);
		}

		g_hCurMatWnd = CreateWindowExA(WS_EX_CLIENTEDGE, RENDER_NONINTERACTIVE_WINDOW_CLASS, "", WS_CHILD | WS_VISIBLE | SS_SUNKEN, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), hWnd, NULL, hInst, NULL);
		if(g_hCurMatWnd)
		{
			ShowWindow(g_hCurMatWnd, SW_SHOW);
			//	UpdateWindow(g_hTopLeftWnd);
		}

		g_hStaticCurrentMatSizeWnd = CreateWindowExA(0, WC_STATIC, "16384x16384", WS_VISIBLE | WS_CHILD, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(15, g_uWndMainDpi), hWnd, 0, hInst, NULL);
		{
			SetWindowFont(g_hStaticCurrentMatSizeWnd, g_hWndMainFont, FALSE);
		}

		g_hButtonCurrentMatBrowseWnd = CreateWindowExA(0, WC_BUTTON, "Browse", WS_VISIBLE | WS_CHILD, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(25, g_uWndMainDpi), hWnd, (HMENU)ID_MAT_BROWSER, hInst, NULL);
		{
			SetWindowFont(g_hButtonCurrentMatBrowseWnd, g_hWndMainFont, FALSE);
		}

		/*g_hObjectTreeWnd = CreateWindowExA(0, WC_TREEVIEW, "", WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP | TVS_CHECKBOXES | TVS_NOHSCROLL, rect.right, rect.top, MARGIN_RIGHT, OBJECT_TREE_HEIGHT, hWnd, 0, hInst, NULL);
		{
			g_pfnTreeOldWndproc = (WNDPROC)GetWindowLongPtr(g_hObjectTreeWnd, GWLP_WNDPROC);
			SetWindowLongPtr(g_hObjectTreeWnd, GWLP_WNDPROC, (LONG_PTR)TreeViewWndProc);
		}*/
		
		g_hStaticTypesWnd = CreateWindowExA(0, WC_STATIC, "Object type", WS_VISIBLE | WS_CHILD, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15 + 25 + 10, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(15, g_uWndMainDpi), hWnd, 0, hInst, NULL);
		{
			SetWindowFont(g_hStaticTypesWnd, g_hWndMainFont, FALSE);
		}

		g_hComboTypesWnd = CreateWindowExA(0, WC_COMBOBOX, "", WS_VISIBLE | WS_CHILD | WS_BORDER | CBS_SORT | CBS_DROPDOWNLIST | CBS_HASSTRINGS, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15 + 25 + 10 + 15, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(OBJECT_TREE_HEIGHT, g_uWndMainDpi), hWnd, (HMENU)IDC_CMB_TYPE, hInst, NULL);
		{
			SetWindowFont(g_hComboTypesWnd, g_hWndMainFont, FALSE);

			//g_pfnClassesComboOldWndproc = (WNDPROC)GetWindowLongPtr(g_hComboTypesWnd, GWLP_WNDPROC);
			SetWindowLongPtr(g_hComboTypesWnd, GWLP_WNDPROC, (LONG_PTR)ClassesComboWndProc);
		}

		g_hStaticClassesWnd = CreateWindowExA(0, WC_STATIC, "Object class", WS_VISIBLE | WS_CHILD, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15 + 25 + 10 + 15 + 25, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(15, g_uWndMainDpi), hWnd, 0, hInst, NULL);
		{
			SetWindowFont(g_hStaticClassesWnd, g_hWndMainFont, FALSE);
		}

		g_hComboClassesWnd = CreateWindowExA(0, WC_COMBOBOX, "", WS_VISIBLE | WS_CHILD | WS_BORDER | CBS_SORT | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15 + 25 + 10 + 15 + 15 + 25, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(OBJECT_TREE_HEIGHT, g_uWndMainDpi), hWnd, (HMENU)IDC_CMB_CLASS, hInst, NULL);
		{
			SetWindowFont(g_hComboClassesWnd, g_hWndMainFont, FALSE);

			g_pfnClassesComboOldWndproc = (WNDPROC)GetWindowLongPtr(g_hComboClassesWnd, GWLP_WNDPROC);
			SetWindowLongPtr(g_hComboClassesWnd, GWLP_WNDPROC, (LONG_PTR)ClassesComboWndProc);
		}

		//g_hCheckboxRandomScaleYawWnd
		g_hCheckboxRandomScaleYawWnd = CreateWindowExA(0, WC_BUTTON, "Random scale and yaw", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15 + 25 + 10 + 15 + 15 + 25 + 25, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(15, g_uWndMainDpi), hWnd, (HMENU)IDC_RAND_SCALE_YAW, hInst, NULL);
		{
			SetWindowFont(g_hCheckboxRandomScaleYawWnd, g_hWndMainFont, FALSE);
		}

		g_hStaticMoveSelectedToWnd = CreateWindowExA(0, WC_STATIC, "Move selected", WS_VISIBLE | WS_CHILD, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15 + 25 + 10 + 15 + 15 + 25 + 25 + 25, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(15, g_uWndMainDpi), hWnd, 0, hInst, NULL);
		{
			SetWindowFont(g_hStaticMoveSelectedToWnd, g_hWndMainFont, FALSE);
		}

		g_hButtonToWorldWnd = CreateWindowExA(0, WC_BUTTON, "To World", WS_VISIBLE | WS_CHILD, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15 + 25 + 10 + 15 + 15 + 25 + 25 + 25 + 15, g_uWndMainDpi), MulDpi(MARGIN_RIGHT / 2, g_uWndMainDpi), MulDpi(25, g_uWndMainDpi), hWnd, (HMENU)IDC_BACK_TO_WORLD, hInst, NULL);
		{
			SetWindowFont(g_hButtonToWorldWnd, g_hWndMainFont, FALSE);
		}

		g_hButtonToEntityWnd = CreateWindowExA(0, WC_BUTTON, "To Object", WS_VISIBLE | WS_CHILD, MulDpi(rect.right + MARGIN_RIGHT / 2, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15 + 25 + 10 + 15 + 15 + 25 + 25 + 25 + 15, g_uWndMainDpi), MulDpi(MARGIN_RIGHT / 2, g_uWndMainDpi), MulDpi(25, g_uWndMainDpi), hWnd, (HMENU)IDC_TIE_TO_OBJECT, hInst, NULL);
		{
			SetWindowFont(g_hButtonToEntityWnd, g_hWndMainFont, FALSE);
			EnableWindow(g_hButtonToEntityWnd, FALSE);
		}

		/*{
			TV_INSERTSTRUCT tvis;
			memset(&tvis, 0, sizeof(tvis));
			tvis.hParent = TVI_ROOT;
			tvis.hInsertAfter = TVI_LAST;
			tvis.item.mask = TVIF_TEXT;
			tvis.item.pszText = "Test item";
			tvis.item.cchTextMax = lstrlen(tvis.item.pszText);
			HTREEITEM hItem = TreeView_InsertItem(g_hObjectTreeWnd, &tvis);

			tvis.hParent = hItem;
			tvis.item.pszText = "Object 1";
			tvis.item.cchTextMax = lstrlen(tvis.item.pszText);
			TreeView_InsertItem(g_hObjectTreeWnd, &tvis);
			tvis.item.pszText = "Object 2";
			tvis.item.cchTextMax = lstrlen(tvis.item.pszText);
			TreeView_InsertItem(g_hObjectTreeWnd, &tvis);
		}*/

		g_hToolbarWnd = CreateToolbar(hWnd);



		g_pPropWindow = new CPropertyWindow(hInst, hWnd);
		g_pPropWindow->clearClassList();
		g_pPropWindow->setCallback(&g_propertyCallback);

		g_pMaterialBrowser = new CMaterialBrowser(hInst, hWnd);
		
		return FALSE;
	}
		break;

	case WM_ENTERSIZEMOVE:
	{
		GetClientRect(hWnd, &rect);

		rect.top += MARGIN_TOP;
		rect.bottom -= MARGIN_BOTTOM;
		rect.left += MARGIN_LEFT;
		rect.right -= MARGIN_RIGHT;

		fCoeffWidth = (float)iLeftWidth / (float)(rect.right - rect.left);
		fCoeffHeight = (float)iTopHeight / (float)(rect.bottom - rect.top);
	}
		break;

	case WM_DPICHANGED:
		g_uWndMainDpi = LOWORD(wParam);
		ReleaseDefaultFont(g_hWndMainFont);
		g_hWndMainFont = GetDefaultFont(g_uWndMainDpi);
		// no break!
	case WM_SIZE:
	{
#if 0
		if(g_isPropWindowVisible)
		{
			if(wParam == SIZE_MINIMIZED)
			{
				g_isPropWindowVisible = g_pPropWindow->isVisible();
				g_pPropWindow->hide();
				shouldShowProps = g_isPropWindowVisible;
			}
			else if(wParam == SIZE_RESTORED && shouldShowProps)
			{
				g_pPropWindow->show();
				shouldShowProps = false;
			}
		}
#endif
		GetClientRect(hWnd, &rect);
		DivDpiRect(&rect, g_uWndMainDpi);

		rect.top += MARGIN_TOP;
		rect.bottom -= MARGIN_BOTTOM;
		rect.left += MARGIN_LEFT;
		rect.right -= MARGIN_RIGHT;

		if(!(xSizing || ySizing))
		{
			RECT rcTopLeft, rcBottomRight;
			GetClientRect(g_hTopLeftWnd, &rcTopLeft);
			DivDpiRect(&rcTopLeft, g_uWndMainDpi);
			GetClientRect(g_hBottomRightWnd, &rcBottomRight);
			DivDpiRect(&rcBottomRight, g_uWndMainDpi);

			UINT uLeftWidth = rcTopLeft.right - rcTopLeft.left;
			UINT uRightWidth = rcBottomRight.right - rcBottomRight.left;
			UINT uTopHeight = rcTopLeft.bottom - rcTopLeft.top;
			UINT uBottomHeight = rcBottomRight.bottom - rcBottomRight.top;

			if(fCoeffHeight && uTopHeight + uBottomHeight + SPLITTER_BAR_WIDTH + WIDTH_ADJUST * 4 != rect.bottom - rect.top)
			{
				iTopHeight = (UINT)(fCoeffHeight * (float)(rect.bottom - rect.top));
			}
			if(fCoeffWidth && uLeftWidth + uRightWidth + SPLITTER_BAR_WIDTH + WIDTH_ADJUST * 4 != rect.right - rect.left)
			{
				iLeftWidth = (UINT)(fCoeffWidth * (float)(rect.right - rect.left));
			}
		}
		MoveWindow(g_hTopLeftWnd, MulDpi(rect.left, g_uWndMainDpi), MulDpi(rect.top, g_uWndMainDpi), MulDpi(iLeftWidth, g_uWndMainDpi), MulDpi(iTopHeight, g_uWndMainDpi), FALSE);
		MoveWindow(g_hBottomLeftWnd, MulDpi(rect.left, g_uWndMainDpi), MulDpi(rect.top + iTopHeight + SPLITTER_BAR_WIDTH, g_uWndMainDpi), MulDpi(iLeftWidth, g_uWndMainDpi), MulDpi(rect.bottom - rect.top - SPLITTER_BAR_WIDTH - iTopHeight, g_uWndMainDpi), FALSE);
		MoveWindow(g_hTopRightWnd, MulDpi(rect.left + iLeftWidth + SPLITTER_BAR_WIDTH, g_uWndMainDpi), MulDpi(rect.top, g_uWndMainDpi), MulDpi(rect.right - rect.left - iLeftWidth - SPLITTER_BAR_WIDTH, g_uWndMainDpi), MulDpi(iTopHeight, g_uWndMainDpi), FALSE);
		MoveWindow(g_hBottomRightWnd, MulDpi(rect.left + iLeftWidth + SPLITTER_BAR_WIDTH, g_uWndMainDpi), MulDpi(rect.top + iTopHeight + SPLITTER_BAR_WIDTH, g_uWndMainDpi), MulDpi(rect.right - rect.left - iLeftWidth - SPLITTER_BAR_WIDTH, g_uWndMainDpi), MulDpi(rect.bottom - rect.top - SPLITTER_BAR_WIDTH - iTopHeight, g_uWndMainDpi), FALSE);
		//MoveWindow(g_hObjectTreeWnd, rect.right, rect.top, MARGIN_RIGHT, OBJECT_TREE_HEIGHT, FALSE);
		MoveWindow(g_hStaticCurrentMatWnd, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(15, g_uWndMainDpi), FALSE);
		MoveWindow(g_hComboCurrentMatWnd, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(15, g_uWndMainDpi), FALSE);
		MoveWindow(g_hCurMatWnd, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), FALSE);
		MoveWindow(g_hStaticCurrentMatSizeWnd, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(15, g_uWndMainDpi), FALSE);
		MoveWindow(g_hButtonCurrentMatBrowseWnd, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(25, g_uWndMainDpi), FALSE);

		MoveWindow(g_hStaticTypesWnd, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15 + 25 + 10, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(15, g_uWndMainDpi), FALSE);
		MoveWindow(g_hComboTypesWnd, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15 + 25 + 10 + 15, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(OBJECT_TREE_HEIGHT, g_uWndMainDpi), FALSE);
		MoveWindow(g_hStaticClassesWnd, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15 + 25 + 10 + 15 + 25, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(15, g_uWndMainDpi), FALSE);
		MoveWindow(g_hComboClassesWnd, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15 + 25 + 10 + 15 + 15 + 25, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(OBJECT_TREE_HEIGHT, g_uWndMainDpi), FALSE);
		MoveWindow(g_hCheckboxRandomScaleYawWnd, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15 + 25 + 10 + 15 + 15 + 25 + 25, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(15, g_uWndMainDpi), FALSE);
		MoveWindow(g_hStaticMoveSelectedToWnd, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15 + 25 + 10 + 15 + 15 + 25 + 25 + 25, g_uWndMainDpi), MulDpi(MARGIN_RIGHT, g_uWndMainDpi), MulDpi(15, g_uWndMainDpi), FALSE);
		MoveWindow(g_hButtonToWorldWnd, MulDpi(rect.right, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15 + 25 + 10 + 15 + 15 + 25 + 25 + 25 + 15, g_uWndMainDpi), MulDpi(MARGIN_RIGHT / 2, g_uWndMainDpi), MulDpi(25, g_uWndMainDpi), FALSE);
		MoveWindow(g_hButtonToEntityWnd, MulDpi(rect.right + MARGIN_RIGHT / 2, g_uWndMainDpi), MulDpi(rect.top + 15 + 25 + MARGIN_RIGHT + 15 + 25 + 10 + 15 + 15 + 25 + 25 + 25 + 15, g_uWndMainDpi), MulDpi(MARGIN_RIGHT / 2, g_uWndMainDpi), MulDpi(25, g_uWndMainDpi), FALSE);
		

		GetClientRect(hWnd, &rect);

		if(message == WM_DPICHANGED)
		{
			g_uABNextTop = rect.top + MARGIN_TOP;
			g_uABNextLeft = rect.left;
			MoveWindow(g_hABArrowButton, MulDpi(g_uABNextLeft, g_uWndMainDpi), MulDpi(g_uABNextTop, g_uWndMainDpi), MulDpi(AB_BUTTON_WIDTH, g_uWndMainDpi), MulDpi(AB_BUTTON_HEIGHT, g_uWndMainDpi), FALSE);
			g_uABNextTop += AB_BUTTON_HEIGHT;

			MoveWindow(g_hABCameraButton, MulDpi(g_uABNextLeft, g_uWndMainDpi), MulDpi(g_uABNextTop, g_uWndMainDpi), MulDpi(AB_BUTTON_WIDTH, g_uWndMainDpi), MulDpi(AB_BUTTON_HEIGHT, g_uWndMainDpi), FALSE);
			g_uABNextTop += AB_BUTTON_HEIGHT;

			MoveWindow(g_hABCreateButton, MulDpi(g_uABNextLeft, g_uWndMainDpi), MulDpi(g_uABNextTop, g_uWndMainDpi), MulDpi(AB_BUTTON_WIDTH, g_uWndMainDpi), MulDpi(AB_BUTTON_HEIGHT, g_uWndMainDpi), FALSE);
			g_uABNextTop += AB_BUTTON_HEIGHT;

			for(UINT i = 0, l = g_aTools.size(); i < l; ++i)
			{
				HWND hBtn = GetDlgItem(hWnd, IDC_AB_FIRST + i);

				MoveWindow(hBtn, MulDpi(g_uABNextLeft, g_uWndMainDpi), MulDpi(g_uABNextTop, g_uWndMainDpi), MulDpi(AB_BUTTON_WIDTH, g_uWndMainDpi), MulDpi(AB_BUTTON_HEIGHT, g_uWndMainDpi), FALSE);
				g_uABNextTop += AB_BUTTON_HEIGHT;
			}


			SetWindowFont(g_hStaticCurrentMatWnd, g_hWndMainFont, FALSE);
			SetWindowFont(g_hComboCurrentMatWnd, g_hWndMainFont, FALSE);
			SetWindowFont(g_hStaticCurrentMatSizeWnd, g_hWndMainFont, FALSE);
			SetWindowFont(g_hButtonCurrentMatBrowseWnd, g_hWndMainFont, FALSE);
			SetWindowFont(g_hStaticTypesWnd, g_hWndMainFont, FALSE);
			SetWindowFont(g_hComboTypesWnd, g_hWndMainFont, FALSE);
			SetWindowFont(g_hStaticClassesWnd, g_hWndMainFont, FALSE);
			SetWindowFont(g_hComboClassesWnd, g_hWndMainFont, FALSE);
			SetWindowFont(g_hCheckboxRandomScaleYawWnd, g_hWndMainFont, FALSE);
			SetWindowFont(g_hStaticMoveSelectedToWnd, g_hWndMainFont, FALSE);
			SetWindowFont(g_hButtonToWorldWnd, g_hWndMainFont, FALSE);
			SetWindowFont(g_hButtonToEntityWnd, g_hWndMainFont, FALSE);
		}

		InvalidateRect(hWnd, &rect, TRUE);

		if(g_pEngine)
		{
			RECT rcTopLeft;
			GetClientRect(g_hTopLeftWnd, &rcTopLeft);
			g_pEngine->getCore()->getConsole()->execCommand2("r_win_width %d\nr_win_height %d", rcTopLeft.right - rcTopLeft.left, rcTopLeft.bottom - rcTopLeft.top);
		}

		SendMessage(g_hStatusWnd, WM_SIZE, wParam, lParam);
	}
		break;

	case WM_INITMENU:
		if(IsEditMessage())	
		{
			HMENU hMenu = (HMENU)wParam;
			EnableMenuItem(hMenu, ID_EDIT_CUT, MF_ENABLED);
			EnableMenuItem(hMenu, ID_EDIT_COPY, MF_ENABLED);
			EnableMenuItem(hMenu, ID_EDIT_PASTE, MF_ENABLED);
			EnableMenuItem(hMenu, ID_EDIT_DELETE, MF_ENABLED);
			EnableMenuItem(hMenu, ID_EDIT_UNDO, MF_ENABLED);
			EnableMenuItem(hMenu, ID_EDIT_REDO, MF_ENABLED);
		}
		else
		{
			bool hasSelection = g_xState.bHasSelection;

			HMENU hMenu = (HMENU)wParam;
			EnableMenuItem(hMenu, ID_EDIT_CUT, hasSelection ? MF_ENABLED : MF_DISABLED);
			EnableMenuItem(hMenu, ID_EDIT_COPY, hasSelection ? MF_ENABLED : MF_DISABLED);
			EnableMenuItem(hMenu, ID_EDIT_DELETE, hasSelection ? MF_ENABLED : MF_DISABLED);
			EnableMenuItem(hMenu, ID_EDIT_PASTE, GetFileAttributesA(g_szClipboardFile) != ~0 ? MF_ENABLED : MF_DISABLED);
		}
		XUpdateUndoRedo();

		{
			const XEditorMenuItem *pItem;
			HMENU hMenu;
			MENUITEMINFOA mii = {sizeof(mii)};
			mii.fMask = MIIM_STATE;
			fora(i, g_aExtMenuItems)
			{
				pItem = g_aExtMenuItems[i].pItem;
				hMenu = g_aExtMenuItems[i].hMenu;
				mii.fState = (pItem->isChecked ? MFS_CHECKED : MFS_UNCHECKED) | 
					(pItem->isDisabled ? MFS_DISABLED : MFS_ENABLED);

				SetMenuItemInfoA(hMenu, ID_EXT_MENU_FIRST + i, FALSE, &mii);
			}
		}
		break;

	case WM_COMMAND:
		if(LOWORD(wParam) >= IDC_AB_FIRST && LOWORD(wParam) < IDC_AB_FIRST + g_aTools.size())
		{
			if(!g_is3DRotating && !g_is3DPanning)
			{
				Button_SetCheck(g_hABCameraButton, BST_UNCHECKED);

				IXEditorTool *pNewTool = g_aTools[LOWORD(wParam) - IDC_AB_FIRST].pTool;
				if(pNewTool != g_pCurrentTool)
				{
					SAFE_CALL(g_pCurrentTool, deactivate);
					mem_release(g_pCurrentTool);
					add_ref(pNewTool);
					g_pCurrentTool = pNewTool;
					XInitTypesCombo();
					SAFE_CALL(g_pCurrentTool, activate);

					CheckDlgButton(hWnd, g_uCurrentTool, FALSE);
					g_uCurrentTool = LOWORD(wParam);
					CheckDlgButton(hWnd, g_uCurrentTool, TRUE);

					g_xState.bCreateMode = false;
					XUpdateGizmos();

				}
				else
				{
					SAFE_CALL(g_pCurrentTool, onNextMode);
				}
			}

			if(GetFocus() == (HWND)lParam)
			{
				SetFocus(g_hWndMain);
			}
		}
		if(LOWORD(wParam) >= IDC_FILE_IMPORT_FIRST && LOWORD(wParam) < IDC_FILE_IMPORT_FIRST + g_pEditorImporters.size())
		{
			IXEditorImporter *pImporter = g_pEditorImporters[LOWORD(wParam) - IDC_FILE_IMPORT_FIRST];
			pImporter->startImport();
		}
		if(LOWORD(wParam) >= ID_EXT_MENU_FIRST && LOWORD(wParam) < ID_EXT_MENU_FIRST + g_aExtMenuItems.size())
		{
			const XEditorMenuItem *pMenuItem = g_aExtMenuItems[LOWORD(wParam) - ID_EXT_MENU_FIRST].pItem;
			if(pMenuItem->pfnCallback)
			{
				pMenuItem->pfnCallback(pMenuItem->pContext);
			}
		}
		switch(LOWORD(wParam))
		{
		case ID_FILE_OPEN:
		{
			if(g_pUndoManager->isDirty())
			{
				const char *szLevelName = g_sLevelName.c_str();
				char szPrompt[128];
				if(szLevelName[0])
				{
					sprintf_s(szPrompt, "Save changes to %s?", szLevelName);
				}
				UINT mb = MessageBoxA(hWnd, szLevelName[0] ? szPrompt : "Save changes?", MAIN_WINDOW_TITLE, MB_YESNOCANCEL);
				if(mb == IDYES)
				{
					if(!XSaveLevel())
					{
						break;
					}
				}
				else if(mb == IDCANCEL)
				{
					break;
				}
			}
			char szSelName[MAX_PATH];
			char szSelPath[1024];
			szSelName[0] = szSelPath[0] = 0;
			{
				CLevelOpenDialog dlg(hInst, g_hWndMain);
				const char *szLevelName = dlg.getLevelName();
				if(szLevelName)
				{
					XLoadLevel(szLevelName);
				}
			}
			break;
		}
		case ID_FILE_NEW:
		{
			if(g_pUndoManager->isDirty())
			{
				const char *szLevelName = g_sLevelName.c_str();
				char szPrompt[128];
				if(szLevelName[0])
				{
					sprintf_s(szPrompt, "Save changes to %s?", szLevelName);
				}
				UINT mb = MessageBoxA(hWnd, szLevelName[0] ? szPrompt : "Save changes?", MAIN_WINDOW_TITLE, MB_YESNOCANCEL);
				if(mb == IDYES)
				{
					if(!XSaveLevel())
					{
						break;
					}
				}
				else if(mb == IDCANCEL)
				{
					break;
				}
			}

			XResetLevel();
			break;
		}
		
		case ID_FILE_SAVE:
			if(g_pUndoManager->isDirty())
			{
				XSaveLevel();
			}
			break;

		case ID_FILE_SAVEAS:
			//if(g_pUndoManager->isDirty())
			{
				XSaveLevel(NULL, true);
			}
			break;

		case IDC_CTRL_BACK:
			if(HIWORD(wParam) == 1)
			{
				if(IsEditMessage())
				{
					HWND hEdit = GetFocus();
					SendMessage(hEdit, WM_KEYDOWN, VK_CONTROL, 0);
					SendMessage(hEdit, WM_KEYDOWN, VK_SHIFT, 0);
					SendMessage(hEdit, WM_KEYDOWN, VK_LEFT, 0);
					SendMessage(hEdit, WM_KEYUP, VK_LEFT, 0);
					SendMessage(hEdit, WM_KEYUP, VK_SHIFT, 0);
					SendMessage(hEdit, WM_KEYUP, VK_CONTROL, 0);
					SendMessage(hEdit, WM_KEYDOWN, VK_DELETE, 0);
					SendMessage(hEdit, WM_KEYUP, VK_DELETE, 0);
					break;
				}
			}
			break;

		case ID_VIEW_AUTOSIZEVIEWS:
			GetClientRect(hWnd, &rect);
			DivDpiRect(&rect, g_uWndMainDpi);

			rect.top += MARGIN_TOP;
			rect.bottom -= MARGIN_BOTTOM;
			rect.left += MARGIN_LEFT;
			rect.right -= MARGIN_RIGHT;

			if(g_isXResizeable)
			{
				iLeftWidth = (rect.right - rect.left - SPLITTER_BAR_WIDTH) / 2;
			}
			else
			{
				iLeftWidth = rect.right - rect.left;
			}
			if(g_isYResizeable)
			{
				iTopHeight = (rect.bottom - rect.top - SPLITTER_BAR_WIDTH) / 2;
			}
			else
			{
				iTopHeight = rect.bottom - rect.top;
			}

			fCoeffWidth = (float)iLeftWidth / (float)(rect.right - rect.left);
			fCoeffHeight = (float)iTopHeight / (float)(rect.bottom - rect.top);
			SendMessage(hWnd, WM_SIZE, 0, 0);
			break;

		case ID_VEWPORTCONFIGURATION_1X2:
			XInitViewportLayout(XVIEW_1X2);
			break;
		case ID_VEWPORTCONFIGURATION_2X2:
			XInitViewportLayout(XVIEW_2X2);
			break;
		case ID_VEWPORTCONFIGURATION_2X1:
			XInitViewportLayout(XVIEW_2X1);
			break;
		case ID_VEWPORTCONFIGURATION_3DONLY:
			XInitViewportLayout(XVIEW_3DONLY);
			break;

		case IDC_ESCAPE:
		case IDC_AB_ARROW:
			if(!g_is3DRotating && !g_is3DPanning && g_uCurrentTool != IDC_AB_ARROW)
			{
				if(g_uCurrentTool != IDC_AB_CAMERA && Button_GetCheck(g_hABCameraButton) == BST_CHECKED)
				{
					CheckDlgButton(hWnd, IDC_AB_CAMERA, BST_UNCHECKED);
				}
				else
				{
					SAFE_CALL(g_pCurrentTool, deactivate);
					mem_release(g_pCurrentTool);
					CheckDlgButton(hWnd, g_uCurrentTool, FALSE);
					g_uCurrentTool = IDC_AB_ARROW;
					CheckDlgButton(hWnd, g_uCurrentTool, TRUE);
					g_xState.bCreateMode = false;
					XUpdateGizmos();
					XInitTypesCombo();
				}
			}
			if(GetFocus() == (HWND)lParam)
			{
				SetFocus(g_hWndMain);
			}
			break;
		case IDC_AB_CAMERA:
			if(!g_is3DRotating && !g_is3DPanning && g_uCurrentTool != IDC_AB_CAMERA)
			{
				if(g_pCurrentTool && g_pCurrentTool->allowUseCamera())
				{
					// toggle camera
					bool isCamera = Button_GetCheck(g_hABCameraButton) == BST_CHECKED;
					Button_SetCheck(g_hABCameraButton, isCamera ? BST_UNCHECKED : BST_CHECKED);
				}
				else
				{
					SAFE_CALL(g_pCurrentTool, deactivate);
					mem_release(g_pCurrentTool);
					CheckDlgButton(hWnd, g_uCurrentTool, BST_UNCHECKED);
					g_uCurrentTool = IDC_AB_CAMERA;
					CheckDlgButton(hWnd, g_uCurrentTool, BST_CHECKED);
					g_xState.bCreateMode = false;
					XUpdateGizmos();
				}				
			}
			if(GetFocus() == (HWND)lParam)
			{
				SetFocus(g_hWndMain);
			}
			break;
		case IDC_AB_CREATE:
			if(!g_is3DRotating && !g_is3DPanning)
			{
				Button_SetCheck(g_hABCameraButton, BST_UNCHECKED);
				if(g_uCurrentTool != IDC_AB_CREATE)
				{
					SAFE_CALL(g_pCurrentTool, deactivate);
					mem_release(g_pCurrentTool);
					CheckDlgButton(hWnd, g_uCurrentTool, FALSE);
					g_uCurrentTool = IDC_AB_CREATE;
					CheckDlgButton(hWnd, g_uCurrentTool, TRUE);
					XUpdateGizmos();
					XInitTypesCombo();
				}
			}
			if(GetFocus() == (HWND)lParam)
			{
				SetFocus(g_hWndMain);
			}
			break;

		case ID_GRIDSIZE_SMALLER:
			if(g_xConfig.m_gridStep > GRID_STEP_MINIMAL)
			{
				g_xConfig.m_gridStep = (GRID_STEP)(g_xConfig.m_gridStep - 1);
				XUpdateStatusGrid();
			}
			break;
		case ID_GRIDSIZE_BIGGER:
			if(g_xConfig.m_gridStep < GRID_STEP_MAXIMAL)
			{
				g_xConfig.m_gridStep = (GRID_STEP)(g_xConfig.m_gridStep + 1);
				XUpdateStatusGrid();
			}
			break;
		case ID_VIEW_GRID:
			g_xConfig.m_bShowGrid = !g_xConfig.m_bShowGrid;
			XCheckMenuItem(g_hMenu, ID_VIEW_GRID, g_xConfig.m_bShowGrid);
			break;

		case ID_LEVEL_SNAPTOGRID:
			g_xConfig.m_bSnapGrid = !g_xConfig.m_bSnapGrid;
			XUpdateStatusGrid();
			break;

		case ID_EDIT_UNDO:
			g_pUndoManager->undo();
			break;

		case ID_EDIT_REDO:
			g_pUndoManager->redo();
			break;

		case ID_EDIT_COPY:
			ToClipboard();
			break;

		case ID_EDIT_CUT:
			ToClipboard(true);
			break;

		case ID_EDIT_PASTE:
			FromClipboard();
			break;

		case ID_EDIT_DELETE:
			DeleteSelection();
			break;

		case ID_EDIT_PROPERTIES:
			g_pPropWindow->show();

			XUpdatePropWindow();
			g_isPropWindowVisible = TRUE;
			break;

		case ID_MAT_BROWSER:
			g_pMaterialBrowser->browse(&g_matBrowserCallback);
			if(GetFocus() == (HWND)lParam)
			{
				SetFocus(g_hWndMain);
			}
			break;


		case ID_VIEW_CENTERONSELECTION:
			if(g_xState.bHasSelection)
			{
				float3 vCenterPos = (g_xState.vSelectionBoundMax + g_xState.vSelectionBoundMin) * 0.5f;
				for(UINT i = 1; i < 4; ++i)
				{
					ICamera *pCamera = g_xConfig.m_pViewportCamera[i];
					float3 vCamPos = pCamera->getPosition();

					switch(g_xConfig.m_x2DView[i])
					{
					case X2D_TOP:
						vCamPos = float3(vCenterPos.x, vCamPos.y, vCenterPos.z);
						break;
					case X2D_FRONT:
						vCamPos = float3(vCenterPos.x, vCenterPos.y, vCamPos.z);
						break;
					case X2D_SIDE:
						vCamPos = float3(vCamPos.x, vCenterPos.y, vCenterPos.z);
						break;
					}

					pCamera->setPosition(vCamPos);
				}
			}
			break;

		case IDC_CMB_TYPE:
			{
				int iSel = ComboBox_GetCurSel(g_hComboTypesWnd);
				int iLen = ComboBox_GetLBTextLen(g_hComboTypesWnd, iSel);
				char *szTypeName = (char*)alloca(sizeof(char)* (iLen + 1));
				ComboBox_GetLBText(g_hComboTypesWnd, iSel, szTypeName);

				ComboBox_ResetContent(g_hComboClassesWnd);

				if(g_pCurrentTool && g_pCurrentTool->wantUseClassSelector())
				{
					UINT uClassCount = g_pCurrentTool->getClassCount();
					for(UINT i = 0; i < uClassCount; ++i)
					{
						ComboBox_AddString(g_hComboClassesWnd, g_pCurrentTool->getClassAt(i));
					}
					ComboBox_Enable(g_hComboClassesWnd, uClassCount > 1);

					int idx = g_aTools.indexOf(g_pCurrentTool, [](const CustomTool &a, IXEditorTool *b){
						return(a.pTool == b);
					});
					assert(idx >= 0);
					if(idx >= 0)
					{
						iSel = ComboBox_FindStringExact(g_hComboClassesWnd, -1, g_aTools[idx].sSelectedClass.c_str());
					}
					else
					{
						iSel = -1;
					}

					ComboBox_SetCurSel(g_hComboClassesWnd, iSel >= 0 ? iSel : 0);
					SendMessage(GetParent(g_hComboClassesWnd), WM_COMMAND, MAKEWPARAM(IDC_CMB_CLASS, CBN_SELCHANGE), (LPARAM)g_hComboClassesWnd);
				}
				else
				{
					g_sSelectedType = szTypeName;

					const AssotiativeArray<AAString, IXEditable*>::Node *pNode;
					if(g_mEditableSystems.KeyExists(AAString(szTypeName), &pNode))
					{
						IXEditable *pEditable = *pNode->Val;
						UINT uClassCount = pEditable->getClassCount();
						for(UINT i = 0; i < uClassCount; ++i)
						{
							ComboBox_AddString(g_hComboClassesWnd, pEditable->getClass(i));
						}
						ComboBox_Enable(g_hComboClassesWnd, uClassCount > 1);

						iSel = ComboBox_FindStringExact(g_hComboClassesWnd, -1, g_sSelectedClass.c_str());

						ComboBox_SetCurSel(g_hComboClassesWnd, iSel >= 0 ? iSel : 0);
						SendMessage(GetParent(g_hComboClassesWnd), WM_COMMAND, MAKEWPARAM(IDC_CMB_CLASS, CBN_SELCHANGE), (LPARAM)g_hComboClassesWnd);
					}
				}
			}
			break;

		case IDC_CMB_CLASS:
			{
				int iSel = ComboBox_GetCurSel(g_hComboTypesWnd);
				int iLen = ComboBox_GetLBTextLen(g_hComboTypesWnd, iSel);
				char *szTypeName = (char*)alloca(sizeof(char)* (iLen + 1));
				ComboBox_GetLBText(g_hComboTypesWnd, iSel, szTypeName);

				iSel = ComboBox_GetCurSel(g_hComboClassesWnd);
				iLen = ComboBox_GetLBTextLen(g_hComboClassesWnd, iSel);
				char *szClassName = (char*)alloca(sizeof(char)* (iLen + 1));
				ComboBox_GetLBText(g_hComboClassesWnd, iSel, szClassName);

				bool can = false;

				const AssotiativeArray<AAString, IXEditable*>::Node *pNode;
				if(g_mEditableSystems.KeyExists(AAString(szTypeName), &pNode))
				{
					IXEditable *pEditable = *pNode->Val;
					can = pEditable->canUseModel(szClassName);
				}
				EnableWindow(g_hButtonToEntityWnd, can);

				if(g_pCurrentTool && g_pCurrentTool->wantUseClassSelector())
				{
					int idx = g_aTools.indexOf(g_pCurrentTool, [](const CustomTool &a, IXEditorTool *b){
						return(a.pTool == b);
					});
					assert(idx >= 0);
					if(g_pCurrentTool->useClass(szClassName))
					{
						if(idx >= 0)
						{
							g_aTools[idx].sSelectedClass = szClassName;
						}
					}
					else
					{
						if(idx >= 0)
						{
							iSel = ComboBox_FindStringExact(g_hComboClassesWnd, -1, g_aTools[idx].sSelectedClass.c_str());
							if(iSel >= 0)
							{
								ComboBox_SetCurSel(g_hComboClassesWnd, iSel);
							}
							else
							{
								ComboBox_SetText(g_hComboClassesWnd, "");
							}
						}
					}
				}
				else
				{
					g_sSelectedClass = szClassName;
				}
			}
			break;

		case ID_CMB_MAT:
			{
				int iSel = ComboBox_GetCurSel(g_hComboCurrentMatWnd);
				if(iSel >= 0)
				{
					int iLen = ComboBox_GetLBTextLen(g_hComboCurrentMatWnd, iSel);
					char *szMatName = (char*)alloca(sizeof(char)* (iLen + 1));
					ComboBox_GetLBText(g_hComboCurrentMatWnd, iSel, szMatName);

					g_matBrowserCallback.onSelected(szMatName);
				}
			}
			break;

		case IDC_RETURN:
		case IDC_CTRL_RETURN:
			if(g_xState.bCreateMode)
			{
				CreateObjectAtPosition(g_xState.vCreateOrigin, GetKeyState(VK_CONTROL) >= 0);

				g_xState.bCreateMode = false;
			}
			break;

		case ID_FILE_EXIT:
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;

		case ID_LEVEL_RUN:
			g_pEngine->getCore()->getConsole()->execCommand("run_level");
			break;

		case ID_TOOLS_RELOADSHADERS:
			g_pEngine->getCore()->getConsole()->execCommand("shader_reload");
			break;

		case ID_TOOLS_RELOADMATERIALS:
			g_pEngine->getCore()->getConsole()->execCommand("mtl_reload");
			break;

		case ID_TOOLS_NEWMATERIAL:
			{
				IXMaterialSystem *pMS = (IXMaterialSystem*)Core_GetIXCore()->getPluginManager()->getInterface(IXMATERIALSYSTEM_GUID);
				char tmp[1024];
				tmp[0] = 0;
				int iMaxLen = sizeof(tmp);
				IXMaterial *pMat;
				while(Tools::DlgPrompt(tmp, &iMaxLen, "Material name", "Edit material", tmp))
				{
					if(pMS->testMaterialName(tmp))
					{
						pMS->loadMaterial(tmp, &pMat);
						new CMaterialEditor(hInst, g_hWndMain, pMat);
						break;
					}
					else
					{
						MessageBoxA(hWnd, "Cannot save material with that name!", "Edit material", MB_OK | MB_ICONERROR);
					}
					iMaxLen = sizeof(tmp);
				}
			}
			break;

		case ID_HELP_SKYXENGINEWIKI:
			ShellExecute(0, 0, "https://wiki.skyxengine.com", 0, 0, SW_SHOW);
			break;

		case ID_HELP_ABOUT:
			MessageBox(g_hWndMain, SKYXENGINE_VERSION4EDITORS "\nhttps://skyxengine.com\nLicensed under the AGPL-3.0 License\nCopyright (c) DogmaNet Team, 2020", "TerraX Editor " SKYXENGINE_VERSION, MB_OK | MB_ICONINFORMATION);
			break;

		case ID_EDIT_CLEARSELECTION:
			if(g_pUndoManager)
			{
				CCommandSelect *pCmdUnselect = new CCommandSelect();
				XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
					if(pObj->isSelected() && (g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent))
					{
						pCmdUnselect->addDeselected(pObj);
					}
				});
				g_pUndoManager->execCommand(pCmdUnselect);
			}
			break;

		case ID_EDIT_SELECTALL:
			{
				CCommandSelect *pCmdSelect = new CCommandSelect();
				XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
					if(!pObj->isSelected() && (g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent))
					{
						pCmdSelect->addSelected(pObj);
					}
				});
				g_pUndoManager->execCommand(pCmdSelect);
			}
			break;

		case ID_XFORM_TRANSLATE:
			if(!GetCapture())
			{
				XSetXformType(X2DXF_SCALE);
			}
			break;
		case ID_XFORM_NONE:
			XSetXformType(X2DXF_NONE);
			break;
		case ID_XFORM_ROTATE:
			XSetXformType(X2DXF_ROTATE);
			break;

		case IDC_TIE_TO_OBJECT:
			if(IsWindowEnabled(g_hButtonToEntityWnd))
			{
				// TODO check for selected proxies!

				Array<CProxyObject*> aProxies;

				fora(i, g_apProxies)
				{
					if(g_apProxies[i]->isSelected())
					{
						aProxies.push_back(g_apProxies[i]);
					}
				}

				if(aProxies.size() > 1)
				{
					MessageBoxA(g_hWndMain, "You have selected several existing objects.\n"
						"It's not allowed", "TerraX", MB_OK | MB_ICONSTOP);
					break;
				}

				int resp = IDNO;
				if(aProxies.size() != 0)
				{
					char tmp[256];
					sprintf(tmp, "You have selected an existing object (%s/%s)\n"
						"Would you like to add the selected brushed to the existing object?\n"
						"If you select 'No', a new object will be created.", aProxies[0]->getTypeName(), aProxies[0]->getClassName());
					resp = MessageBoxA(g_hWndMain, tmp, "TerraX", MB_YESNOCANCEL | MB_ICONEXCLAMATION);
					if(resp == IDCANCEL)
					{
						break;
					}
				}

				IXEditorCommand *pCmd;
				if(resp == IDNO)
				{
					int iSel = ComboBox_GetCurSel(g_hComboTypesWnd);
					int iLen = ComboBox_GetLBTextLen(g_hComboTypesWnd, iSel);
					char *szTypeName = (char*)alloca(sizeof(char) * (iLen + 1));
					ComboBox_GetLBText(g_hComboTypesWnd, iSel, szTypeName);

					iSel = ComboBox_GetCurSel(g_hComboClassesWnd);
					iLen = ComboBox_GetLBTextLen(g_hComboClassesWnd, iSel);
					char *szClassName = (char*)alloca(sizeof(char) * (iLen + 1));
					ComboBox_GetLBText(g_hComboClassesWnd, iSel, szClassName);

					pCmd = new CCommandBuildModel(szTypeName, szClassName);
				}
				else
				{
					pCmd = new CCommandModifyModel(aProxies[0]);
				}
				if(XExecCommand(pCmd))
				{
					SendMessage(g_hWndMain, WM_COMMAND, MAKEWPARAM(ID_EDIT_PROPERTIES, 0), NULL);
				}

				if(GetFocus() == (HWND)lParam)
				{
					SetFocus(g_hWndMain);
				}
			}
			break;

		case IDC_BACK_TO_WORLD:
			if(IsWindowEnabled(g_hButtonToWorldWnd))
			{
				if(!g_xConfig.m_bIgnoreGroups)
				{
					CCommandContainer *pContainer = NULL;
					fora(i, g_apProxies)
					{
						CProxyObject *pProxy = g_apProxies[i];
						if(pProxy->isSelected())
						{
							if(!pContainer)
							{
								pContainer = new CCommandContainer();
							}
							pContainer->addCommand(new CCommandDestroyModel(pProxy));
						}
					}
					if(pContainer)
					{
						XExecCommand(pContainer);
					}
				}
				if(GetFocus() == (HWND)lParam)
				{
					SetFocus(g_hWndMain);
				}
			}
			
			break;

		case IDC_RAND_SCALE_YAW:
			if(GetFocus() == (HWND)lParam)
			{
				SetFocus(g_hWndMain);
			}
			break;

		case ID_ROTATE_90:
		case ID_ROTATE_M90:
			{
				CCommandRotate *pCmd = new CCommandRotate(GetKeyState(VK_SHIFT) < 0);
				XEnumerateObjects([pCmd](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
					if(pObj->isSelected() && (g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent))
					{
						pCmd->addObject(pObj);
					}
				});

				X_2D_VIEW xCurView = g_xConfig.m_x2DView[g_xState.activeWindow];
				float fViewScale = g_xConfig.m_fViewportScale[g_xState.activeWindow];

				float3 vCenter = (g_xState.vSelectionBoundMax + g_xState.vSelectionBoundMin) * 0.5f;

				float fTo = LOWORD(wParam) == ID_ROTATE_90 ? 1.0f : -1.0f;
				switch(xCurView)
				{
				case X2D_TOP:
					pCmd->setStartOrigin(vCenter, float3(0.0f, 1.0f, 0.0f));
					pCmd->setStartPos(vCenter + float3(0.0f, 0.0f, 1.0f));
					pCmd->setCurrentPos(vCenter + float3(fTo, 0.0f, 0.0f));
					break;
				case X2D_FRONT:
					pCmd->setStartOrigin(vCenter, float3(0.0f, 0.0f, 1.0f));
					pCmd->setStartPos(vCenter + float3(0.0f, 1.0f, 0.0f));
					pCmd->setCurrentPos(vCenter + float3(fTo, 0.0f, 0.0f));
					break;
				case X2D_SIDE:
					pCmd->setStartOrigin(vCenter, float3(1.0f, 0.0f, 0.0f));
					pCmd->setStartPos(vCenter + float3(0.0f, 1.0f, 0.0f));
					pCmd->setCurrentPos(vCenter + float3(0.0f, 0.0f, fTo));
					break;
				}

				XExecCommand(pCmd);
			}
			break;

		case ID_IGNORE_GROUPS:
			g_xConfig.m_bIgnoreGroups = !g_xConfig.m_bIgnoreGroups;
			CheckToolbarButton(ID_IGNORE_GROUPS, g_xConfig.m_bIgnoreGroups);
			CCommandSelect *pCmdUnselect = new CCommandSelect();
			if(!g_xConfig.m_bIgnoreGroups)
			{
				//! Deselect partially selected groups
				XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
					if(isProxy && !pParent)
					{
						bool hasUnselectedChild = false;
						bool hasUnselectedProxies = false;
						XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
							if(!pObj->isSelected())
							{
								if(isProxy)
								{
									hasUnselectedProxies = true;
								}
								else
								{
									hasUnselectedChild = true;
								}
							}
						}, (CProxyObject*)pObj);
						if(hasUnselectedChild)
						{
							XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
								if(!isProxy)
								{
									bool hasSelectedParent = pParent->isSelected();
									CProxyObject *pCur = pParent;
									while(!hasSelectedParent && (pCur = XGetObjectParent(pCur)))
									{
										hasSelectedParent = pCur->isSelected();
									}
									if(hasSelectedParent)
									{
										if(!pObj->isSelected())
										{
											pCmdUnselect->addSelected(pObj);
										}
									}
									else if(pObj->isSelected())
									{
										pCmdUnselect->addDeselected(pObj);
									}
								}
							}, (CProxyObject*)pObj);
							if(pObj->isSelected())
							{
								pCmdUnselect->addDeselected(pObj);
							}
							else
							{
								XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
									if(isProxy)
									{
										bool hasSelectedParent = pParent->isSelected();
										CProxyObject *pCur = pParent;
										while((pCur = XGetObjectParent(pCur)))
										{
											if(pCur->isSelected())
											{
												hasSelectedParent = true;
												pParent = pCur;
											}
										}
										if(hasSelectedParent)
										{
											pCmdUnselect->addDeselected(pParent);
										}
									}
								}, (CProxyObject*)pObj);
							}
						}
						else
						{
							if(!pObj->isSelected() || hasUnselectedProxies)
							{
								// deselect leaves, select root
								XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
									if(!isProxy)
									{
										pCmdUnselect->addDeselected(pObj);
									}
								}, (CProxyObject*)pObj);
								pCmdUnselect->addSelected(pObj);
							}
							
						}
					}
				});
				
				pCmdUnselect->setIGMode(CCommandSelect::IGM_ENABLE);
			}
			else
			{
				pCmdUnselect->setIGMode(CCommandSelect::IGM_DISABLE);
			}
			g_pUndoManager->execCommand(pCmdUnselect);
			XUpdatePropWindow();
			break;
		}
		break;

	case WM_LBUTTONDOWN:
	{
		int                 xPos;
		int                 yPos;

		// Varible used to get the mouse cursor x and y co-ordinates
		xPos = DivDpi(GET_X_LPARAM(lParam), g_uWndMainDpi);
		yPos = DivDpi(GET_Y_LPARAM(lParam), g_uWndMainDpi);

		// Checks whether the mouse is over the splitter window
		xSizing = g_isXResizeable && (xPos > iLeftWidth + MARGIN_LEFT - SPLITTER_BAR_WIDTH && xPos < iLeftWidth + MARGIN_LEFT + SPLITTER_BAR_WIDTH);
		ySizing = g_isYResizeable && (yPos > iTopHeight + MARGIN_TOP - SPLITTER_BAR_WIDTH && yPos < iTopHeight + MARGIN_TOP + SPLITTER_BAR_WIDTH);

		if(xSizing || ySizing)
		{
			// Api to capture mouse input
			SetCapture(hWnd);
			if(xSizing && ySizing)
			{
				SetCursor(hcSizeNESW);
			}
			else if(xSizing)
			{
				SetCursor(hcSizeEW);
			}
			else if(ySizing)
			{
				SetCursor(hcSizeNS);
			}
		}
	}
	break;

	case WM_LBUTTONUP:
		if(xSizing || ySizing)
		{
			RECT    focusrect;
			HDC     hdc;

			// Releases the captured mouse input
			ReleaseCapture();
			// Get the main window dc to draw a focus rectangle
			hdc = GetDC(hWnd);
			GetClientRect(hWnd, &rect);
			DivDpiRect(&rect, g_uWndMainDpi);

			rect.top += MARGIN_TOP;
			rect.bottom -= MARGIN_BOTTOM;
			rect.left += MARGIN_LEFT;
			rect.right -= MARGIN_RIGHT;

			fCoeffWidth = (float)iLeftWidth / (float)(rect.right - rect.left);
			fCoeffHeight = (float)iTopHeight / (float)(rect.bottom - rect.top);

			if(xSizing)
			{
				SetRect(&focusrect, iLeftWidth - (WIDTH_ADJUST * 2), rect.top, iLeftWidth + WIDTH_ADJUST, rect.bottom);
				MulDpiRect(&focusrect, g_uWndMainDpi);

				// Call api to vanish the dragging rectangle 
				DrawFocusRect(hdc, &focusrect);

				xSizing = FALSE;
			}
			if(ySizing)
			{
				SetRect(&focusrect, rect.left, iTopHeight - (WIDTH_ADJUST * 2), rect.right, iTopHeight + WIDTH_ADJUST);
				MulDpiRect(&focusrect, g_uWndMainDpi);

				// Call api to vanish the dragging rectangle 
				DrawFocusRect(hdc, &focusrect);

				ySizing = FALSE;
			}
			// Release the dc once done 
			ReleaseDC(hWnd, hdc);
			// Post a WM_SIZE message to redraw the windows
			PostMessage(hWnd, WM_SIZE, 0, 0);
		}
		break;

	case WM_MOUSEMOVE:
	{
		int   xPos;
		int   yPos;

		// Get the x and y co-ordinates of the mouse

		xPos = DivDpi(GET_X_LPARAM(lParam), g_uWndMainDpi);
		yPos = DivDpi(GET_Y_LPARAM(lParam), g_uWndMainDpi);
		
		GetClientRect(hWnd, &rect);
		DivDpiRect(&rect, g_uWndMainDpi);

		rect.top += MARGIN_TOP;
		rect.bottom -= MARGIN_BOTTOM;
		rect.left += MARGIN_LEFT;
		rect.right -= MARGIN_RIGHT;

		if(xPos < rect.left + SPLITTER_BAR_WIDTH * 3)
		{
			xPos = rect.left + SPLITTER_BAR_WIDTH * 3;
		}
		if(xPos > rect.right - SPLITTER_BAR_WIDTH * 3)
		{
			xPos = rect.right - SPLITTER_BAR_WIDTH * 3;
		}
		if(yPos < rect.top + SPLITTER_BAR_WIDTH * 3)
		{
			yPos = rect.top + SPLITTER_BAR_WIDTH * 3;
		}
		if(yPos > rect.bottom - SPLITTER_BAR_WIDTH * 3)
		{
			yPos = rect.bottom - SPLITTER_BAR_WIDTH * 3;
		}

		// Checks if the left button is pressed during dragging the splitter
		if(wParam == MK_LBUTTON)
		{
			// If the window is d`agged using the splitter, get the
			// cursors current postion and draws a focus rectangle 
			if(xSizing)
			{
				RECT    focusrect;
				HDC     hdc;

				hdc = GetDC(hWnd);


				SetRect(&focusrect, iLeftWidth + MARGIN_LEFT - (WIDTH_ADJUST * 2), rect.top, iLeftWidth + MARGIN_LEFT + WIDTH_ADJUST, rect.bottom);
				MulDpiRect(&focusrect, g_uWndMainDpi);

				DrawFocusRect(hdc, &focusrect);

				// Get the size of the left window to increase
				iLeftWidth = xPos - MARGIN_LEFT;

				// Draws a focus rectangle
				SetRect(&focusrect, iLeftWidth + MARGIN_LEFT - (SPLITTER_BAR_WIDTH * 2), rect.top, iLeftWidth + MARGIN_LEFT + SPLITTER_BAR_WIDTH, rect.bottom);
				MulDpiRect(&focusrect, g_uWndMainDpi);

				DrawFocusRect(hdc, &focusrect);

				ReleaseDC(hWnd, hdc);
			}

			if(ySizing)
			{
				RECT focusrect;
				HDC hdc;
				hdc = GetDC(hWnd);
				SetRect(&focusrect, rect.left, iTopHeight + MARGIN_TOP - (WIDTH_ADJUST * 2), rect.right, iTopHeight + MARGIN_TOP + WIDTH_ADJUST);
				MulDpiRect(&focusrect, g_uWndMainDpi);
				DrawFocusRect(hdc, &focusrect);
				iTopHeight = yPos - MARGIN_TOP;
				SetRect(&focusrect, rect.left, iTopHeight + MARGIN_TOP - (SPLITTER_BAR_WIDTH * 2), rect.right, iTopHeight + MARGIN_TOP + SPLITTER_BAR_WIDTH);
				MulDpiRect(&focusrect, g_uWndMainDpi);
				DrawFocusRect(hdc, &focusrect);
				ReleaseDC(hWnd, hdc);
			}
		}
		// Set the cursor image to east west direction when the mouse is over 
		// the splitter window
		BOOL x = g_isXResizeable && (xPos > iLeftWidth + MARGIN_LEFT - SPLITTER_BAR_WIDTH && xPos < iLeftWidth + MARGIN_LEFT + SPLITTER_BAR_WIDTH);
		BOOL y = g_isYResizeable && (yPos > iTopHeight + MARGIN_TOP - SPLITTER_BAR_WIDTH && yPos < iTopHeight + MARGIN_TOP + SPLITTER_BAR_WIDTH);
		if(x && y)
		{
			SetCursor(hcSizeNESW);
		}
		else if(x)
		{
			SetCursor(hcSizeEW);
		}
		else if(y)
		{
			SetCursor(hcSizeNS);
		}
	}
	break;

	case WM_MOUSEWHEEL:
	{
		POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
		ScreenToClient(hWnd, &pt);

		GetClientRect(hWnd, &rect);

		rect.top += MulDpi(MARGIN_TOP, g_uWndMainDpi);
		rect.bottom -= MulDpi(MARGIN_BOTTOM, g_uWndMainDpi);
		rect.left += MulDpi(MARGIN_LEFT, g_uWndMainDpi);
		rect.right -= MulDpi(MARGIN_RIGHT, g_uWndMainDpi);
		if(pt.x < rect.left || pt.x > rect.right || pt.y < rect.top || pt.y > rect.bottom)
		{
			return(TRUE);
		}
		BOOL isLeft = (pt.x < rect.left + MulDpi(iLeftWidth, g_uWndMainDpi)),
			isTop = (pt.y < rect.top + MulDpi(iTopHeight, g_uWndMainDpi));
		ICamera *pCamera = NULL;
		X_2D_VIEW x2dView;
		HWND hTargetWnd;
		float *pfOldScale = NULL;
		if(isLeft)
		{
			if(isTop)
			{
				// pCamera = g_pTopLeftCamera;
				pCamera = g_xConfig.m_pViewportCamera[XWP_TOP_LEFT];
				float3 vLook = pCamera->getLook();
				float3 vPos = pCamera->getPosition();
				pCamera->setPosition(vPos + vLook * (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA * 0.5f);
				pCamera = NULL;
				hTargetWnd = g_hTopLeftWnd;
			}
			else
			{
				pCamera = g_xConfig.m_pViewportCamera[XWP_BOTTOM_LEFT];
				x2dView = g_xConfig.m_x2DView[XWP_BOTTOM_LEFT];
				hTargetWnd = g_hBottomLeftWnd;
				pfOldScale = &g_xConfig.m_fViewportScale[XWP_BOTTOM_LEFT];
			}
		}
		else
		{
			if(isTop)
			{
				pCamera = g_xConfig.m_pViewportCamera[XWP_TOP_RIGHT];
				x2dView = g_xConfig.m_x2DView[XWP_TOP_RIGHT];
				hTargetWnd = g_hTopRightWnd;
				pfOldScale = &g_xConfig.m_fViewportScale[XWP_TOP_RIGHT];
			}
			else
			{
				pCamera = g_xConfig.m_pViewportCamera[XWP_BOTTOM_RIGHT];
				x2dView = g_xConfig.m_x2DView[XWP_BOTTOM_RIGHT];
				hTargetWnd = g_hBottomRightWnd;
				pfOldScale = &g_xConfig.m_fViewportScale[XWP_BOTTOM_RIGHT];
			}
		}
		if(pCamera)
		{
			float fNewScale = *pfOldScale - *pfOldScale * (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA * 0.1f;
			RECT rc;
			GetWindowRect(hTargetWnd, &rc);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&rc, 2);
			float2 vCenter((float)(rc.left + rc.right) * 0.5f, (float)(rc.top + rc.bottom) * 0.5f);
			float2 vDelta = (float2((float)pt.x, (float)pt.y) - vCenter) * float2(1.0f, -1.0f);
			float2 vWorldPt;
			float3 vCamPos = pCamera->getPosition();
			switch(x2dView)
			{
			case X2D_TOP:
				vWorldPt = float2(vCamPos.x, vCamPos.z);
				break;
			case X2D_FRONT:
				vWorldPt = float2(vCamPos.x, vCamPos.y);
				break;
			case X2D_SIDE:
				vWorldPt = float2(vCamPos.z, vCamPos.y);
			//	vDelta.x *= -1;
				break;
			}
			vWorldPt += vDelta * (*pfOldScale - fNewScale);
			switch(x2dView)
			{
			case X2D_TOP:
				pCamera->setPosition(float3(vWorldPt.x, vCamPos.y, vWorldPt.y));
				break;
			case X2D_FRONT:
				pCamera->setPosition(float3(vWorldPt.x, vWorldPt.y, vCamPos.z));
				break;
			case X2D_SIDE:
				pCamera->setPosition(float3(vCamPos.x, vWorldPt.y, vWorldPt.x));
				break;
			}
			*pfOldScale = fNewScale;
		}
		else
		{
			// 3d
		}
		break;
	}

	case WM_DESTROY:
		{
			WINDOWPLACEMENT wp = {sizeof(wp)};
			BOOL res = GetWindowPlacement(hWnd, &wp);

			HKEY hKey;
			if(ERROR_SUCCESS == RegCreateKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\DogmaNet\\TerraX", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_QUERY_VALUE, NULL, &hKey, NULL))
			{
				RegSetValueExA(hKey, "WinPos", 0, REG_BINARY, (BYTE*)&wp, wp.length);
			}
			RegCloseKey(hKey);
		}

		mem_delete(g_pPropWindow);
		DestroyMenu(g_hMenu);
		ReleaseDefaultFont(g_hWndMainFont);
		DeleteObject(hcSizeEW);
		DeleteObject(hcSizeNS);
		DeleteObject(hcSizeNESW);
		PostQuitMessage(0);
		break;

	case WM_SETTITLEASYNC:
		SetWindowTextA(hWnd, (LPCSTR)lParam);
		break;

	case WM_CLOSE:
		if(g_pUndoManager->isDirty())
		{
			const char *szLevelName = g_sLevelName.c_str();
			char szPrompt[128];
			if(szLevelName[0])
			{
				sprintf_s(szPrompt, "Save changes to %s?", szLevelName);
			}
			UINT mb = MessageBoxA(hWnd, szLevelName[0] ? szPrompt : "Save changes?", MAIN_WINDOW_TITLE, MB_YESNOCANCEL);
			if(mb == IDYES)
			{
				if(!XSaveLevel())
				{
					break;
				}
			}
			else if(mb == IDCANCEL)
			{
				break;
			}
		}
		DestroyWindow(hWnd);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return(0);
}

struct SelectItem
{
	float fDist2;
	IXEditorObject *pObj;
};
Array<SelectItem> g_aRaytracedItems;
CCommandSelect *g_pSelectCmd = NULL;
float g_fSelectDeltaTime = 0.0f;
UINT g_uSelectedIndex = 0;
bool g_isSelectionCtrl = false;

static void XTrackMouse(HWND hWnd, LPARAM lParam)
{
	HWND hCapWnd = GetCapture();
	if(hCapWnd)
	{
		hWnd = hCapWnd;
	}
	else
	{
		g_xState.hActiveWnd = hWnd;

		if(hWnd == g_hTopRightWnd)
		{
			g_xState.activeWindow = XWP_TOP_RIGHT;
		}
		else if(hWnd == g_hBottomLeftWnd)
		{
			g_xState.activeWindow = XWP_BOTTOM_LEFT;
		}
		else if(hWnd == g_hBottomRightWnd)
		{
			g_xState.activeWindow = XWP_BOTTOM_RIGHT;
		}
		else
		{
			g_xState.activeWindow = XWP_TOP_LEFT;
		}
	}

	g_xState.vMousePos = {(float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam)};

	ICamera *pCamera = g_xConfig.m_pViewportCamera[g_xState.activeWindow];
	if(!pCamera)
	{
		return;
	}

	RECT rc;
	GetWindowRect(hWnd, &rc);
	float2 vWinSize((float)(rc.right - rc.left), (float)(rc.bottom - rc.top));
	g_xState.vWinSize = vWinSize;

	if(g_xState.activeWindow == XWP_TOP_LEFT)
	{
		// transform by matrix
		SMMATRIX mViewProj;
		Core_RMatrixGet(G_RI_MATRIX_OBSERVER_VIEWPROJ, &mViewProj);
		SMMATRIX mInvVP = SMMatrixInverse(NULL, mViewProj);

		float3 vScreenPos(g_xState.vMousePos / vWinSize, 0.0f);
		vScreenPos = vScreenPos * float3(2.0f, -2.0f, 1.0f) - float3(1.0f, -1.0f, 0.0f);
		vScreenPos.z = -1.0f;
		vScreenPos.w = 1.0f;


		float3 vRayStart = vScreenPos * mInvVP;
		vRayStart /= vRayStart.w;

		vScreenPos.z = 1.0f;
		float3 vRayDir = vScreenPos * mInvVP;
		vRayDir /= vRayDir.w;
		vRayDir = SMVector3Normalize(vRayDir - vRayStart);

		g_xState.vWorldRayStart = vRayStart;
		g_xState.vWorldRayDir = vRayDir;

		float3 avNormals[] = {
			float3(1.0f, 0.0f, 0.0f),
			float3(0.0f, 1.0f, 0.0f),
			float3(0.0f, 0.0f, 1.0f)
		};

		float fBestCos = -1.0f;
		for(int i = 0; i < ARRAYSIZE(avNormals); ++i)
		{
			float fCos = fabsf(SMVector3Dot(avNormals[i], vRayDir));
			if(fCos > fBestCos)
			{
				fBestCos = fCos;
				g_xState.vBestPlaneNormal = avNormals[i];
			}
		}
	}
	else
	{
		float3 fCamWorld = pCamera->getPosition();

		X_2D_VIEW xCurView = g_xConfig.m_x2DView[g_xState.activeWindow];
		float fViewScale = g_xConfig.m_fViewportScale[g_xState.activeWindow];

		switch(xCurView)
		{
		case X2D_TOP:
			fCamWorld = float3(fCamWorld.x, fCamWorld.z, 0.0f);
			g_xState.vWorldRayDir = float3(0.0f, -1.0f, 0.0f);
			g_xState.vBestPlaneNormal = -g_xState.vWorldRayDir;
			break;
		case X2D_FRONT:
			fCamWorld = float3(fCamWorld.x, fCamWorld.y, 0.0f);
			g_xState.vWorldRayDir = float3(0.0f, 0.0f, 1.0f);
			g_xState.vBestPlaneNormal = g_xState.vWorldRayDir;
			break;
		case X2D_SIDE:
			fCamWorld = float3(fCamWorld.z, fCamWorld.y, 0.0f);
			g_xState.vWorldRayDir = float3(-1.0f, 0.0f, 0.0f);
			g_xState.vBestPlaneNormal = -g_xState.vWorldRayDir;
			break;
		}

		RECT rc;
		GetClientRect(hWnd, &rc);

		float2 vCenter((float)(rc.left + rc.right) * 0.5f, (float)(rc.top + rc.bottom) * 0.5f);
		float2 vDelta = (g_xState.vMousePos - vCenter) * float2(1.0f, -1.0f);

		float2 vSize = vWinSize;
		vSize *= 0.5f * fViewScale;
		float2 vTopLeft = fCamWorld - vSize;
		float2 vBottomRight = fCamWorld + vSize;
		g_xState.m_vViewportBorders[g_xState.activeWindow] = float4(vTopLeft.x, vTopLeft.y, vBottomRight.x, vBottomRight.y);

		g_xState.vWorldMousePos = (float2)(fCamWorld + vDelta * fViewScale);

		switch(xCurView)
		{
		case X2D_TOP:
			g_xState.vWorldRayStart = float3(g_xState.vWorldMousePos.x, fCamWorld.y, g_xState.vWorldMousePos.y);
			break;
		case X2D_FRONT:
			g_xState.vWorldRayStart = float3(g_xState.vWorldMousePos.x, g_xState.vWorldMousePos.y, fCamWorld.z);
			break;
		case X2D_SIDE:
			g_xState.vWorldRayStart = float3(fCamWorld.x, g_xState.vWorldMousePos.y, g_xState.vWorldMousePos.x);
			break;
		}

		g_xState.vResolvedWorldMousePos = GetPasteCenter();
	}
}

void XDisableCurrentTool()
{
	Button_SetCheck(g_hABCameraButton, BST_UNCHECKED);
	SAFE_CALL(g_pCurrentTool, deactivate);
	mem_release(g_pCurrentTool);
	CheckDlgButton(g_hWndMain, g_uCurrentTool, FALSE);
	g_uCurrentTool = IDC_AB_ARROW;
	CheckDlgButton(g_hWndMain, g_uCurrentTool, TRUE);
	g_xState.bCreateMode = false;
	XUpdateGizmos();
	XInitTypesCombo();
}

static bool XIsClicked(const float3 &vPos)
{
	X_2D_VIEW xCurView = g_xConfig.m_x2DView[g_xState.activeWindow];
	float fViewScale = g_xConfig.m_fViewportScale[g_xState.activeWindow];

	const float fWorldSize = 3.5f * fViewScale;

	bool sel = false;
	switch(xCurView)
	{
	case X2D_TOP:
		sel = fabsf(vPos.x - g_xState.vWorldMousePos.x) < fWorldSize && fabsf(vPos.z - g_xState.vWorldMousePos.y) < fWorldSize;
		break;
	case X2D_FRONT:
		sel = fabsf(vPos.x - g_xState.vWorldMousePos.x) < fWorldSize && fabsf(vPos.y - g_xState.vWorldMousePos.y) < fWorldSize;
		break;
	case X2D_SIDE:
		sel = fabsf(vPos.z - g_xState.vWorldMousePos.x) < fWorldSize && fabsf(vPos.y - g_xState.vWorldMousePos.y) < fWorldSize;
		break;
	}

	return(sel);
}

static bool XIsCameraActive()
{
	return(Button_GetCheck(g_hABCameraButton) || GetKeyState(VK_SPACE) < 0);
}

LRESULT CALLBACK RenderNoninteractiveWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//return(DefWindowProc(hWnd, message, wParam, lParam));


	switch(message)
	{
	case WM_PAINT:
		g_isCurMatDirty = true;
		break;
	}
	return(DefWindowProc(hWnd, message, wParam, lParam));
}

LRESULT CALLBACK RenderWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if(g_pEngine && g_pEngine->onMessage(message, wParam, lParam))
	{
		return(TRUE);
	}

	static const bool *terrax_detach_3d = GET_PCVAR_BOOL("terrax_detach_3d");
	if(!terrax_detach_3d)
	{
		terrax_detach_3d = GET_PCVAR_BOOL("terrax_detach_3d");
	}
	else if(*terrax_detach_3d)
	{
		return(DefWindowProc(hWnd, message, wParam, lParam));
	}

	PAINTSTRUCT ps;
	HDC hdc;
	RECT rect;
	static HCURSOR hcPtr = NULL;
	static HCURSOR hcSizeNS = NULL;
	static HCURSOR hcSizeWE = NULL;
	static HCURSOR hcSizeNESW = NULL;
	static HCURSOR hcSizeNWSE = NULL;
	static HCURSOR hcRotate = NULL;

	static bool s_wasSelectionChanged = false;

	static const int *r_final_image = GET_PCVAR_INT("r_final_image");
	if(!r_final_image)
	{
		r_final_image = GET_PCVAR_INT("r_final_image");
	}

	static CCommandMove *s_pMoveCmd = NULL;
	static CCommandScale *s_pScaleCmd = NULL;
	static CCommandRotate *s_pRotateCmd = NULL;

	switch(message)
	{
	case WM_CREATE:
		hcPtr = LoadCursor(NULL, IDC_CROSS);
		hcSizeNS = LoadCursor(NULL, IDC_SIZENS);
		hcSizeWE = LoadCursor(NULL, IDC_SIZEWE);
		hcSizeNWSE = LoadCursor(NULL, IDC_SIZENWSE);
		hcSizeNESW = LoadCursor(NULL, IDC_SIZENESW);
		hcRotate = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR1));
		break;

	case WM_RBUTTONUP:
		if(g_is3DPanning)
		{
			SSInput_SetEnable(false);
			ReleaseCapture();
			g_is3DPanning = FALSE;
		}

		SAFE_CALL(g_pCurrentTool, onMouseUp, false);

		if(hWnd != g_hTopLeftWnd && XIsMouseInSelection(g_xState.activeWindow))
		{
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			MapWindowPoints(hWnd, g_hWndMain, &pt, 1);
			DisplayContextMenu(g_hWndMain, pt, g_hMenu2, 2);
		}

		break;

	case WM_LBUTTONUP:
	{
		if(g_is2DPanning)
		{
			g_is2DPanning = FALSE;
			SSInput_SetEnable(false);
			ReleaseCapture();
			break;
		}
		SAFE_CALL(g_pCurrentTool, onMouseUp, true);

		if(g_is3DRotating)
		{
			g_is3DRotating = FALSE;
			SSInput_SetEnable(false);
			ReleaseCapture();
			break;
		}
		if(g_xState.isFrameSelectInternal)
		{
			float2_t vSelectStartPos, vSelectEndPos;
			X_2D_VIEW xCurView;
			g_pEditor->endFrameSelect(&xCurView, &vSelectStartPos, &vSelectEndPos);
			g_xState.isFrameSelectInternal = false;

			if(!SMIsZero(SMVector2Length(vSelectEndPos - vSelectStartPos)))
			{
				CCommandSelect *pCmd = new CCommandSelect();
				bool bUse = false;
				XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
					if(!(g_xConfig.m_bIgnoreGroups && isProxy))
					{
						float3_t vPos = pObj->getPos();
						bool sel = g_pEditor->isPointInFrame(vPos, vSelectStartPos, vSelectEndPos, xCurView);
						if(!g_xConfig.m_bIgnoreGroups && pParent)
						{
							CProxyObject *pCur;
							while((pCur = XGetObjectParent(pParent)))
							{
								pParent = pCur;
							}
							pObj = pParent;
						}
						if(wParam & MK_CONTROL)
						{
							if(sel && !pObj->isSelected())
							{
								//pObj->setSelected(true);
								bUse = true;
								pCmd->addSelected(pObj);
							}
						}
						else
						{
							if(!pObj->isSelected() && sel)
							{
								bUse = true;
								pCmd->addSelected(pObj);
								//pObj->setSelected(sel);
							}
							else if(pObj->isSelected() && !sel)
							{
								bUse = true;
								pCmd->addDeselected(pObj);
								//pObj->setSelected(sel);
							}
						}
					}
				});

				if(bUse)
				{
					if(!g_xState.bHasSelection)
					{
						XSetXformType(X2DXF_SCALE);
					}
					XExecCommand(pCmd);
				}
			}
		}
		else if(s_pMoveCmd)
		{
			ReleaseCapture();
			if(!XExecCommand(s_pMoveCmd) && g_xState.bHasSelection && !s_wasSelectionChanged)
			{
				XSetXformType((X_2DXFORM_TYPE)((g_xState.xformType + 1) % X2DXF__LAST));
			}
			s_pMoveCmd = NULL;
		}
		else if(s_pScaleCmd)
		{
			ReleaseCapture();
			XExecCommand(s_pScaleCmd);
			s_pScaleCmd = NULL;
		}
		else if(s_pRotateCmd)
		{
			ReleaseCapture();
			XExecCommand(s_pRotateCmd);
			s_pRotateCmd = NULL;
		}
		else if(g_pSelectCmd)
		{
			ReleaseCapture();
			XExecCommand(g_pSelectCmd);
			g_pSelectCmd = NULL;
		}

		g_pEditor->onMouseUp();

		POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
		GetClientRect(hWnd, &rect);
		if(pt.x > rect.left && pt.x < rect.left + 40
			&& pt.y > rect.top && pt.y < rect.top + 20)
		{
			int iActiveMenu = -1;
			X_2D_VIEW x2dView = g_xConfig.m_x2DView[g_xState.activeWindow];
			if(hWnd == g_hTopLeftWnd)
			{
				switch(*r_final_image)
				{
				case DS_RT_COLOR:
					iActiveMenu = ID_3D_TEXTURED;
					break;
				case DS_RT_NORMAL:
					iActiveMenu = ID_3D_NORMALS;
					break;
				case DS_RT_PARAM:
					iActiveMenu = ID_3D_PARAMETERS;
					break;
				case DS_RT_AMBIENTDIFF:
					iActiveMenu = ID_3D_AMBIENTDIFFUSE;
					break;
				case DS_RT_LUMINANCE:
					iActiveMenu = ID_3D_SPECULAR;
					break;
				case DS_RT_SCENELIGHT:
					iActiveMenu = ID_3D_LIGHTINGSCENE;
					break;
				case DS_RT_DEPTH:
					iActiveMenu = ID_3D_3DDEPTH;
					break;
				}
			}
			else
			{
				switch(x2dView)
				{
				case X2D_TOP:
					iActiveMenu = ID_2D_TOP;
					break;
				case X2D_FRONT:
					iActiveMenu = ID_2D_FRONT;
					break;
				case X2D_SIDE:
					iActiveMenu = ID_2D_SIDE;
					break;
				}
			}
			DisplayContextMenu(hWnd, pt, g_hMenu2, hWnd == g_hTopLeftWnd ? 0 : 1, iActiveMenu);
		}
	}
	break;

	case WM_LBUTTONDBLCLK:
		if(hWnd == g_hTopLeftWnd && Button_GetCheck(g_hABArrowButton))
		{
			SendMessage(g_hWndMain, WM_COMMAND, MAKEWPARAM(ID_EDIT_PROPERTIES, 0), NULL);
			break;
		}
		// NO BREAK!
	case WM_LBUTTONDOWN:
	{
		s_wasSelectionChanged = false;
		POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
		GetClientRect(hWnd, &rect);
		if(pt.x > rect.left && pt.x < rect.left + 20
			&& pt.y > rect.top && pt.y < rect.top + 20)
		{
			break;
		}

		XTrackMouse(hWnd, lParam);



		if(hWnd == g_hTopLeftWnd)
		{
			RECT rc;
			GetWindowRect(hWnd, &rc);
			float2 vWinSize((float)(rc.right - rc.left), (float)(rc.bottom - rc.top));

			// transform by matrix
			SMMATRIX mViewProj;
			Core_RMatrixGet(G_RI_MATRIX_OBSERVER_VIEWPROJ, &mViewProj);
			SMMATRIX mInvVP = SMMatrixInverse(NULL, mViewProj);

			float3 vScreenPos(g_xState.vMousePos / vWinSize, 0.0f);
			vScreenPos = vScreenPos * float3(2.0f, -2.0f, 1.0f) - float3(1.0f, -1.0f, 0.0f);
			vScreenPos.z = -1.0f;
			vScreenPos.w = 1.0f;


			float3 vRayStart = vScreenPos * mInvVP;
			vRayStart /= vRayStart.w;

			vScreenPos.z = 1.0f;
			float3 vRayDir = vScreenPos * mInvVP;
			vRayDir /= vRayDir.w;
			vRayDir = SMVector3Normalize(vRayDir - vRayStart);
			// printf("%.2f, %.2f, %.2f\n", vRayStart.x, vRayStart.y, vRayStart.z);
			// printf("%.2f, %.2f, %.2f\n\n", vRayEnd.x, vRayEnd.y, vRayEnd.z);

			float3 vRayEnd = vRayStart + vRayDir * 1000.0f;


			if(Button_GetCheck(g_hABArrowButton) && !XIsCameraActive())
			{
				if(g_pEditor->onMouseDown())
				{
					break;
				}

				g_aRaytracedItems.clearFast();
				XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
					if(g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent)
					{
						float fDist2 = -1.0f;
						if(!pObj->hasVisualModel())
						{
							float3 vPos = pObj->getPos();
							if(SMDistancePointBeam2(vPos, vRayStart, vRayDir) < 0.1f * 0.1f)
							{
								fDist2 = SMVector3Length2(vRayStart - vPos);
							}
						}
						else
						{
							float3 vPos;
							if(pObj->rayTest(vRayStart, vRayEnd, &vPos))
							{
								fDist2 = SMVector3Length2(vRayStart - vPos);
							}
						}

						if(fDist2 >= 0.0f)
						{
							if(!g_xConfig.m_bIgnoreGroups && pParent)
							{
								CProxyObject *pCur;
								while((pCur = XGetObjectParent(pParent)))
								{
									pParent = pCur;
								}
								pObj = pParent;
							}

							g_aRaytracedItems.push_back({fDist2, pObj});
						}
					}
				});
				
				g_aRaytracedItems.quickSort([](const SelectItem &a, const SelectItem &b){
					return(a.fDist2 < b.fDist2);
				});

				g_pSelectCmd = new CCommandSelect();
				SetCapture(hWnd);
				g_fSelectDeltaTime = XSELECT_STEP_DELAY;
				g_uSelectedIndex = ~0;
				g_isSelectionCtrl = GetKeyState(VK_CONTROL) < 0;

				XFrameRun(0.0f);
			}
			else if(Button_GetCheck(g_hABCreateButton) && !XIsCameraActive())
			{
				struct SelectItem2
				{
					float fDist2;
					float3 vPos;
					float3 vNorm;
				};
				static Array<SelectItem2> s_aRaytracedItems;

				float3 vPos, vNorm;
				XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
					if(g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent)
					{
						if(pObj->hasVisualModel())
						{
							if(pObj->rayTest(vRayStart, vRayEnd, &vPos, &vNorm, NULL, true))
							{
								s_aRaytracedItems.push_back({SMVector3Length2(vRayStart - vPos), vPos, vNorm});
							}
						}
					}
				});
				
				s_aRaytracedItems.quickSort([](const SelectItem2 &a, const SelectItem2 &b){
					return(a.fDist2 < b.fDist2);
				});

				if(s_aRaytracedItems.size())
				{
					vPos = s_aRaytracedItems[0].vPos;
					vNorm = SMVector3Normalize(s_aRaytracedItems[0].vNorm);

					IXEditorObject *pObj = CreateObjectAtPosition(vPos, GetKeyState(VK_CONTROL) >= 0)->getObject();
					float3 vMin, vMax;
					pObj->getBound(&vMin, &vMax);

					float3 avPoints[] = {
						float3(vMin),
						float3(vMin.x, vMin.y, vMax.z),
						float3(vMin.x, vMax.y, vMin.z),
						float3(vMin.x, vMax.y, vMax.z),
						float3(vMax.x, vMin.y, vMin.z),
						float3(vMax.x, vMin.y, vMax.z),
						float3(vMax.x, vMax.y, vMin.z),
						float3(vMax),
					};
					//printf("%.2f, %.2f, %.2f\n", vNorm.x, vNorm.y, vNorm.z);

					float fProj = FLT_MAX;
					for(UINT i = 0, l = ARRAYSIZE(avPoints); i < l; ++i)
					{
						float fTmp = SMVector3Dot(avPoints[i], vNorm);
						//printf("%.2f\n", fTmp);
						if(fTmp < fProj)
						{
							fProj = fTmp;
						}
					}

					fProj = SMVector3Dot(vPos, vNorm) - fProj;
					//printf("%.2f, %.2f, %.2f\n%.2f, %.2f, %.2f\n%.2f, %.2f, %.2f\n%f\n\n", vPos.x, vPos.y, vPos.z, vMin.x, vMin.y, vMin.z, vMax.x, vMax.y, vMax.z, fProj);
					if(fProj > 0)
					{
						pObj->setPos((float3)(vPos + vNorm * fProj));
					}

					s_aRaytracedItems.clearFast();
				}
			}

			if(!XIsCameraActive() && g_pEditor->onMouseDown())
			{
				break;
			}
		}
		else
		{
			if(XIsCameraActive())
			{
				g_is2DPanning = TRUE;
				SetCapture(hWnd);
				SSInput_SetEnable(true);
			}
			else if(Button_GetCheck(g_hABArrowButton))
			{
				X_2D_VIEW xCurView = g_xConfig.m_x2DView[g_xState.activeWindow];

				if(g_xState.bHasSelection)
				{
					float fScale = g_xConfig.m_fViewportScale[g_xState.activeWindow];
					float fPtSize = 3.0f * fScale;
					float fPtMargin = 7.0f * fScale;
					float2 vSelectionCenter;
					float4 vBorder;

					const float2_t &fMPos = g_xState.vWorldMousePos;
					float3 vStartPos;
					float3 vMask;

					switch(g_xConfig.m_x2DView[g_xState.activeWindow])
					{
					case X2D_TOP:
						vBorder = float4(g_xState.vSelectionBoundMin.x, g_xState.vSelectionBoundMin.z,
							g_xState.vSelectionBoundMax.x, g_xState.vSelectionBoundMax.z);
						vSelectionCenter = float2(g_xState.vSelectionBoundMin.x + g_xState.vSelectionBoundMax.x,
							g_xState.vSelectionBoundMin.z + g_xState.vSelectionBoundMax.z) * 0.5f;
						vStartPos = float3(fMPos.x, 0.0f, fMPos.y);
						vMask = float3(1.0f, 0.0f, 1.0f);
						break;
					case X2D_FRONT:
						vBorder = float4(g_xState.vSelectionBoundMin.x, g_xState.vSelectionBoundMin.y,
							g_xState.vSelectionBoundMax.x, g_xState.vSelectionBoundMax.y);
						vSelectionCenter = float2(g_xState.vSelectionBoundMin.x + g_xState.vSelectionBoundMax.x,
							g_xState.vSelectionBoundMin.y + g_xState.vSelectionBoundMax.y) * 0.5f;
						vStartPos = float3(fMPos.x, fMPos.y, 0.0f);
						vMask = float3(1.0f, 1.0f, 0.0f);
						break;
					case X2D_SIDE:
						vBorder = float4(g_xState.vSelectionBoundMin.z, g_xState.vSelectionBoundMin.y,
							g_xState.vSelectionBoundMax.z, g_xState.vSelectionBoundMax.y);
						vSelectionCenter = float2(g_xState.vSelectionBoundMin.z + g_xState.vSelectionBoundMax.z,
							g_xState.vSelectionBoundMin.y + g_xState.vSelectionBoundMax.y) * 0.5f;
						vStartPos = float3(0.0f, fMPos.y, fMPos.x);
						vMask = float3(0.0f, 1.0f, 1.0f);
						break;
					}

					//vStartPos = XSnapToGrid(vStartPos);

					// bottom center
					float2_t vCenters[] = {
						{vSelectionCenter.x, vBorder.y - fPtMargin}, // bottom center
						{vBorder.z + fPtMargin, vSelectionCenter.y}, // center right
						{vSelectionCenter.x, vBorder.w + fPtMargin}, // top center
						{vBorder.x - fPtMargin, vSelectionCenter.y}, // center left
						{vBorder.x - fPtMargin, vBorder.y - fPtMargin}, // bottom left
						{vBorder.z + fPtMargin, vBorder.y - fPtMargin}, // bottom right
						{vBorder.z + fPtMargin, vBorder.w + fPtMargin}, // top right
						{vBorder.x - fPtMargin, vBorder.w + fPtMargin} // top left
					};
					bool bHandled = false;
					HCURSOR hcs[] = {hcSizeNS, hcSizeWE, hcSizeNS, hcSizeWE, hcSizeNESW, hcSizeNWSE, hcSizeNESW, hcSizeNWSE};
					X_DIR dirs[][8] = {
						// X2D_TOP (x/z)
						{XDIR_Z_NEG, XDIR_X_POS, XDIR_Z_POS, XDIR_X_NEG, XDIR_Z_NEG | XDIR_X_NEG, XDIR_Z_NEG | XDIR_X_POS, XDIR_Z_POS | XDIR_X_POS, XDIR_Z_POS | XDIR_X_NEG},
						// X2D_FRONT (x/y)
						{XDIR_Y_NEG, XDIR_X_POS, XDIR_Y_POS, XDIR_X_NEG, XDIR_Y_NEG | XDIR_X_NEG, XDIR_Y_NEG | XDIR_X_POS, XDIR_Y_POS | XDIR_X_POS, XDIR_Y_POS | XDIR_X_NEG},
						// X2D_SIDE (z/y)
						{XDIR_Y_NEG, XDIR_Z_POS, XDIR_Y_POS, XDIR_Z_NEG, XDIR_Y_NEG | XDIR_Z_NEG, XDIR_Y_NEG | XDIR_Z_POS, XDIR_Y_POS | XDIR_Z_POS, XDIR_Y_POS | XDIR_Z_NEG}
					};
					for(int i = g_xState.xformType == X2DXF_SCALE || g_xState.xformType == X2DXF_NONE ? 0 : 4; i < 8; ++i)
					{
						if(fabsf(fMPos.x - vCenters[i].x) <= fPtSize && fabsf(fMPos.y - vCenters[i].y) <= fPtSize)
						{
							SetCapture(hWnd);

							if(g_xState.xformType == X2DXF_SCALE || g_xState.xformType == X2DXF_NONE)
							{
								// create scale command
								s_pScaleCmd = new CCommandScale();
								s_pScaleCmd->setStartAABB(g_xState.vSelectionBoundMin, g_xState.vSelectionBoundMax);
								s_pScaleCmd->setTransformDir(dirs[g_xConfig.m_x2DView[g_xState.activeWindow]][i]);
								s_pScaleCmd->setStartPos(vStartPos);
								XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
									if(pObj->isSelected() && (g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent))
									{
										s_pScaleCmd->addObject(pObj);
									}
								});
							}
							else if(g_xState.xformType == X2DXF_ROTATE)
							{
								// create rotate command
								s_pRotateCmd = new CCommandRotate(GetKeyState(VK_SHIFT) < 0);
								s_pRotateCmd->setStartOrigin((g_xState.vSelectionBoundMax + g_xState.vSelectionBoundMin) * 0.5f * vMask, float3(1.0f) - vMask);
								s_pRotateCmd->setStartPos(vStartPos);
								XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
									if(pObj->isSelected() && (g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent))
									{
										s_pRotateCmd->addObject(pObj);
									}
								});
							}
							bHandled = true;
							break;
						}
					}
					if(bHandled)
					{
						break;
					}
				}

				if(g_pEditor->onMouseDown())
				{
					break;
				}

				if(!XIsMouseInSelection(g_xState.activeWindow) || (wParam & MK_CONTROL))
				{
					CCommandSelect *pCmd = new CCommandSelect();
					bool bUse = false;
					bool wasSel = false;

					XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
						if(!(g_xConfig.m_bIgnoreGroups && isProxy))
						{
							bool sel = XIsClicked(pObj->getPos());

							if(!g_xConfig.m_bIgnoreGroups && pParent)
							{
								CProxyObject *pCur;
								while((pCur = XGetObjectParent(pParent)))
								{
									pParent = pCur;
								}
								pObj = pParent;
							}

							if(wParam & MK_CONTROL)
							{
								if(sel && !wasSel)
								{
									if(pObj->isSelected())
									{
										pCmd->addDeselected(pObj);
									}
									else
									{
										pCmd->addSelected(pObj);
									}
									bUse = true;
									wasSel = true;
								}
							}
							else
							{
								if(pObj->isSelected() && !sel)
								{
									pCmd->addDeselected(pObj);
									bUse = true;
								}
								else if(!pObj->isSelected() && sel && !wasSel)
								{
									pCmd->addSelected(pObj);
									bUse = true;
									wasSel = true;
								}
							}
						}
					});

					if(bUse)
					{
						s_wasSelectionChanged = true;
						XExecCommand(pCmd);
						XUpdateSelectionBound();
					}
				}

				if(XIsMouseInSelection(g_xState.activeWindow))
				{
					SetCapture(hWnd);
					SetCursor(hcPtr);
					s_pMoveCmd = new CCommandMove(GetKeyState(VK_SHIFT) < 0);
					float3 vStartPos;
					switch(xCurView)
					{
					case X2D_TOP:
						vStartPos = float3(g_xState.vWorldMousePos.x, 0.0f, g_xState.vWorldMousePos.y);
						break;
					case X2D_FRONT:
						vStartPos = float3(g_xState.vWorldMousePos.x, g_xState.vWorldMousePos.y, 0.0f);
						break;
					case X2D_SIDE:
						vStartPos = float3(0.0f, g_xState.vWorldMousePos.y, g_xState.vWorldMousePos.x);
						break;
					}
					s_pMoveCmd->setStartPos(XSnapToGrid(vStartPos));

					XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
						if(pObj->isSelected() && (g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent))
						{
							s_pMoveCmd->addObject(pObj);
						}
					});
					// if mouse in selected object
					// start move
				}
				else
				{ // start frame select
					g_xState.isFrameSelectInternal = true;
					g_pEditor->beginFrameSelect();
				}
			}
			else if(Button_GetCheck(g_hABCreateButton))
			{
				X_2D_VIEW xCurView = g_xConfig.m_x2DView[g_xState.activeWindow];

				g_xState.bCreateMode = true;

				switch(xCurView)
				{
				case X2D_TOP:
					g_xState.vCreateOrigin = float3(g_xState.vWorldMousePos.x, g_xState.vCreateOrigin.y, g_xState.vWorldMousePos.y);
					break;
				case X2D_FRONT:
					g_xState.vCreateOrigin = float3(g_xState.vWorldMousePos.x, g_xState.vWorldMousePos.y, g_xState.vCreateOrigin.z);
					break;
				case X2D_SIDE:
					g_xState.vCreateOrigin = float3(g_xState.vCreateOrigin.x, g_xState.vWorldMousePos.y, g_xState.vWorldMousePos.x);
					break;
				}

				g_xState.vCreateOrigin = XSnapToGrid(g_xState.vCreateOrigin);
			}
			else if(g_pEditor->onMouseDown())
			{
				break;
			}
			//break;
		}

		if(!XIsCameraActive() && g_pCurrentTool && g_pCurrentTool->onMouseDown(true))
		{
			break;
		}

		if(hWnd == g_hTopLeftWnd)
		{
			if(g_is3DPanning)
			{
				break;
			}
			if(XIsCameraActive())
			{
				g_is3DRotating = TRUE;
				SetCapture(hWnd);
				SSInput_SetEnable(true);
				break;
			}
		}
		//else
	}
	break;

	case WM_RBUTTONDOWN:
	{
		if(!XIsCameraActive() && g_pCurrentTool && g_pCurrentTool->onMouseDown(false))
		{
			break;
		}

		if(hWnd != g_hTopLeftWnd || g_is3DRotating)
		{
			break;
		}
		if(XIsCameraActive())
		{
			g_is3DPanning = TRUE;
			SetCapture(hWnd);
			SSInput_SetEnable(true);
		}
		break;
	}

	case WM_MBUTTONDOWN:
		if(hWnd != g_hTopLeftWnd)
		{
			g_is2DPanning = TRUE;
			SetCapture(hWnd);
			SSInput_SetEnable(true);
		}
		break;

	case WM_MBUTTONUP:
		if(hWnd != g_hTopLeftWnd)
		{
			g_is2DPanning = FALSE;
			SSInput_SetEnable(false);
			ReleaseCapture();
		}
		break;
		
	case WM_MOUSEMOVE:
		SetFocus(g_hWndMain);

		if(!g_is3DRotating && !g_is3DPanning && !g_is2DPanning)
		{
			XTrackMouse(hWnd, lParam);




			if(g_xState.activeWindow != XWP_TOP_LEFT)
			{
				X_2D_VIEW xCurView = g_xConfig.m_x2DView[g_xState.activeWindow];
#if 0
				ICamera *pCamera = g_xConfig.m_pViewportCamera[g_xState.activeWindow];
				if(!pCamera)
				{
					break;
				}

				float fViewScale = g_xConfig.m_fViewportScale[g_xState.activeWindow];

				float3 fCamWorld = pCamera->getPosition();
				switch(xCurView)
				{
				case X2D_TOP:
					fCamWorld = float3(fCamWorld.x, fCamWorld.z, 0.0f);
					break;
				case X2D_FRONT:
					fCamWorld = float3(fCamWorld.x, fCamWorld.y, 0.0f);
					break;
				case X2D_SIDE:
					fCamWorld = float3(fCamWorld.z, fCamWorld.y, 0.0f);
					break;
				}

				RECT rc;
				GetClientRect(hWnd, &rc);

				float2 vCenter((float)(rc.left + rc.right) * 0.5f, (float)(rc.top + rc.bottom) * 0.5f);
				float2 vDelta = (g_xState.vMousePos - vCenter) * float2(1.0f, -1.0f);

				float2 vSize((float)(rc.right - rc.left), (float)(rc.bottom - rc.top));
				vSize *= 0.5f * fViewScale;
				float2 vTopLeft = fCamWorld - vSize;
				float2 vBottomRight = fCamWorld + vSize;
				g_xState.m_vViewportBorders[g_xState.activeWindow] = float4(vTopLeft.x, vTopLeft.y, vBottomRight.x, vBottomRight.y);

				g_xState.vWorldMousePos = (float2)(fCamWorld + vDelta * fViewScale);
#endif
				/*if(g_is2DPanning)
				{
				// vWorldDelta
				ICamera *pCamera = g_xConfig.m_pViewportCamera[g_xState.activeWindow];

				float3 vWorldDelta = (g_xState.vMousePos - g_v2DPanningStartMouse) * fViewScale;

				float3 vWorldDelta3D;
				switch(xCurView)
				{
				case X2D_TOP:
				vWorldDelta3D = float3(vWorldDelta.x, 0.0f, -vWorldDelta.y);
				break;
				case X2D_FRONT:
				vWorldDelta3D = float3(vWorldDelta.x, -vWorldDelta.y, 0.0f);
				break;
				case X2D_SIDE:
				vWorldDelta3D = float3(0.0f, -vWorldDelta.y, vWorldDelta.x);
				break;
				}
				float3 vPos = g_v2DPanningStart - vWorldDelta3D;
				pCamera->setPosition(&vPos);
				}*/

				if(s_pMoveCmd || s_pScaleCmd || s_pRotateCmd)
				{
					float3 vCurPos;
					switch(xCurView)
					{
					case X2D_TOP:
						vCurPos = float3(g_xState.vWorldMousePos.x, 0.0f, g_xState.vWorldMousePos.y);
						break;
					case X2D_FRONT:
						vCurPos = float3(g_xState.vWorldMousePos.x, g_xState.vWorldMousePos.y, 0.0f);
						break;
					case X2D_SIDE:
						vCurPos = float3(0.0f, g_xState.vWorldMousePos.y, g_xState.vWorldMousePos.x);
						break;
					}

					//vCurPos = XSnapToGrid(vCurPos);

					if(s_pMoveCmd)
					{
						s_pMoveCmd->setCurrentPos(XSnapToGrid(vCurPos));
					}
					if(s_pScaleCmd)
					{
						s_pScaleCmd->setCurrentPos(vCurPos);
					}
					if(s_pRotateCmd)
					{
						s_pRotateCmd->setCurrentPos(vCurPos);
					}
				}
			}
			XUpdateStatusMPos();

			if(Button_GetCheck(g_hABArrowButton) && !s_pMoveCmd && !s_pScaleCmd && !s_pRotateCmd && g_xState.activeWindow != XWP_TOP_LEFT)
			{
				if(XIsMouseInSelection(g_xState.activeWindow))
				{
					SetCursor(hcPtr);
				}
				else if(g_xState.bHasSelection)
				{
					float fScale = g_xConfig.m_fViewportScale[g_xState.activeWindow];
					float fPtSize = 3.0f * fScale;
					float fPtMargin = 7.0f * fScale;
					float2 vSelectionCenter;
					float4 vBorder;

					const float2_t &fMPos = g_xState.vWorldMousePos;

					switch(g_xConfig.m_x2DView[g_xState.activeWindow])
					{
					case X2D_TOP:
						vBorder = float4(g_xState.vSelectionBoundMin.x, g_xState.vSelectionBoundMin.z,
							g_xState.vSelectionBoundMax.x, g_xState.vSelectionBoundMax.z);
						vSelectionCenter = float2(g_xState.vSelectionBoundMin.x + g_xState.vSelectionBoundMax.x,
							g_xState.vSelectionBoundMin.z + g_xState.vSelectionBoundMax.z) * 0.5f;
						break;
					case X2D_FRONT:
						vBorder = float4(g_xState.vSelectionBoundMin.x, g_xState.vSelectionBoundMin.y,
							g_xState.vSelectionBoundMax.x, g_xState.vSelectionBoundMax.y);
						vSelectionCenter = float2(g_xState.vSelectionBoundMin.x + g_xState.vSelectionBoundMax.x,
							g_xState.vSelectionBoundMin.y + g_xState.vSelectionBoundMax.y) * 0.5f;
						break;
					case X2D_SIDE:
						vBorder = float4(g_xState.vSelectionBoundMin.z, g_xState.vSelectionBoundMin.y,
							g_xState.vSelectionBoundMax.z, g_xState.vSelectionBoundMax.y);
						vSelectionCenter = float2(g_xState.vSelectionBoundMin.z + g_xState.vSelectionBoundMax.z,
							g_xState.vSelectionBoundMin.y + g_xState.vSelectionBoundMax.y) * 0.5f;
						break;
					}

					// bottom center
					float2_t vCenters[] = {
						{vSelectionCenter.x, vBorder.y - fPtMargin}, // bottom center
						{vBorder.z + fPtMargin, vSelectionCenter.y}, // center right
						{vSelectionCenter.x, vBorder.w + fPtMargin}, // top center
						{vBorder.x - fPtMargin, vSelectionCenter.y}, // center left

						{vBorder.x - fPtMargin, vBorder.y - fPtMargin}, // bottom left
						{vBorder.z + fPtMargin, vBorder.y - fPtMargin}, // bottom right
						{vBorder.z + fPtMargin, vBorder.w + fPtMargin}, // top right
						{vBorder.x - fPtMargin, vBorder.w + fPtMargin} // top left
					};
					HCURSOR hcs[] = {hcSizeNS, hcSizeWE, hcSizeNS, hcSizeWE, hcSizeNESW, hcSizeNWSE, hcSizeNESW, hcSizeNWSE};
					for(int i = (g_xState.xformType == X2DXF_SCALE || g_xState.xformType == X2DXF_NONE) ? 0 : 4; i < 8; ++i)
					{
						if(fabsf(fMPos.x - vCenters[i].x) <= fPtSize && fabsf(fMPos.y - vCenters[i].y) <= fPtSize)
						{
							if(g_xState.xformType == X2DXF_SCALE || g_xState.xformType == X2DXF_NONE)
							{
								SetCursor(hcs[i]);
							}
							else if(g_xState.xformType == X2DXF_ROTATE)
							{
								SetCursor(hcRotate);
							}
							break;
						}
					}
				}
			}
			if(s_pMoveCmd)
			{
				SetCursor(hcPtr);
			}
			if(s_pScaleCmd)
			{
				//@TODO:SetCursor(???);
			}
			if(s_pRotateCmd)
			{
				SetCursor(hcRotate);
			}

			if(Button_GetCheck(g_hABCreateButton) && g_xState.bCreateMode && GetAsyncKeyState(VK_LBUTTON) < 0)
			{
				switch(g_xConfig.m_x2DView[g_xState.activeWindow])
				{
				case X2D_TOP:
					g_xState.vCreateOrigin = float3(g_xState.vWorldMousePos.x, g_xState.vCreateOrigin.y, g_xState.vWorldMousePos.y);
					break;
				case X2D_FRONT:
					g_xState.vCreateOrigin = float3(g_xState.vWorldMousePos.x, g_xState.vWorldMousePos.y, g_xState.vCreateOrigin.z);
					break;
				case X2D_SIDE:
					g_xState.vCreateOrigin = float3(g_xState.vCreateOrigin.x, g_xState.vWorldMousePos.y, g_xState.vWorldMousePos.x);
					break;
				}
				g_xState.vCreateOrigin = XSnapToGrid(g_xState.vCreateOrigin);
			}

			SAFE_CALL(g_pEditor, onMouseMove);

			if(g_pCurrentTool && g_pCurrentTool->onMouseMove())
			{
				return(TRUE);
			}

			return(TRUE);
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case ID_3D_TEXTURED:
			Core_0SetCVarInt("r_final_image", DS_RT_COLOR);
			break;
		case ID_3D_NORMALS:
			Core_0SetCVarInt("r_final_image", DS_RT_NORMAL);
			break;
		case ID_3D_PARAMETERS:
			Core_0SetCVarInt("r_final_image", DS_RT_PARAM);
			break;
		case ID_3D_AMBIENTDIFFUSE:
			Core_0SetCVarInt("r_final_image", DS_RT_AMBIENTDIFF);
			break;
		case ID_3D_SPECULAR:
			Core_0SetCVarInt("r_final_image", DS_RT_LUMINANCE);
			break;
		case ID_3D_LIGHTINGSCENE:
			Core_0SetCVarInt("r_final_image", DS_RT_SCENELIGHT);
			break;
		case ID_3D_3DDEPTH:
			Core_0SetCVarInt("r_final_image", DS_RT_DEPTH);
			break;


		case ID_2D_TOP:
		case ID_2D_FRONT:
		case ID_2D_SIDE:
		{
			ICamera *pTargetCam = g_xConfig.m_pViewportCamera[g_xState.activeWindow];
			X_2D_VIEW *pX2DView = &g_xConfig.m_x2DView[g_xState.activeWindow];
			g_bViewportCaptionDirty[g_xState.activeWindow] = true;
			switch(LOWORD(wParam))
			{
			case ID_2D_TOP:
				pTargetCam->setPosition(X2D_TOP_POS);
				pTargetCam->setOrientation(X2D_TOP_ROT);
				*pX2DView = X2D_TOP;
				break;
			case ID_2D_FRONT:
				pTargetCam->setPosition(X2D_FRONT_POS);
				pTargetCam->setOrientation(X2D_FRONT_ROT);
				*pX2DView = X2D_FRONT;
				break;
			case ID_2D_SIDE:
				pTargetCam->setPosition(X2D_SIDE_POS);
				pTargetCam->setOrientation(X2D_SIDE_ROT);
				*pX2DView = X2D_SIDE;
				break;
			}
		}
		}
		break;

	case WM_DESTROY:
		DeleteObject(hcPtr);
		DeleteObject(hcSizeNS);
		DeleteObject(hcSizeWE);
		DeleteObject(hcSizeNWSE);
		DeleteObject(hcSizeNESW);
		DeleteObject(hcRotate);
		break;

	default:
		return(DefWindowProc(hWnd, message, wParam, lParam));
	}
	return(0);
}


void XFrameRun(float fDeltaTime)
{
	if(g_pSelectCmd)
	{
		g_fSelectDeltaTime += fDeltaTime;

		if(g_fSelectDeltaTime >= XSELECT_STEP_DELAY)
		{
			g_fSelectDeltaTime = fmodf(g_fSelectDeltaTime, XSELECT_STEP_DELAY);

			if(g_uSelectedIndex == ~0 && !g_isSelectionCtrl)
			{
				XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
					if(pObj->isSelected() && (g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent))
					{
						pObj->setSelected(false);
						g_pSelectCmd->addDeselected(pObj);
					}
				});
			}

			if(g_aRaytracedItems.size())
			{
				if(g_uSelectedIndex != ~0)
				{
					if(g_aRaytracedItems[g_uSelectedIndex].pObj->isSelected())
					{
						g_aRaytracedItems[g_uSelectedIndex].pObj->setSelected(false);
						g_pSelectCmd->addDeselected(g_aRaytracedItems[g_uSelectedIndex].pObj);
					}
					else
					{
						g_aRaytracedItems[g_uSelectedIndex].pObj->setSelected(true);
						g_pSelectCmd->addSelected(g_aRaytracedItems[g_uSelectedIndex].pObj);
					}
				}

				++g_uSelectedIndex;
				g_uSelectedIndex %= g_aRaytracedItems.size();

				if(g_aRaytracedItems[g_uSelectedIndex].pObj->isSelected())
				{
					g_aRaytracedItems[g_uSelectedIndex].pObj->setSelected(false);
					g_pSelectCmd->addDeselected(g_aRaytracedItems[g_uSelectedIndex].pObj);
				}
				else
				{
					g_aRaytracedItems[g_uSelectedIndex].pObj->setSelected(true);
					g_pSelectCmd->addSelected(g_aRaytracedItems[g_uSelectedIndex].pObj);
				}
			}
		}
	}

	g_pMaterialBrowser->update(fDeltaTime);
}

void DisplayContextMenu(HWND hwnd, POINT pt, HMENU hMenu, int iSubmenu, int iCheckItem)
{
	//HMENU hmenu;            // top-level menu 
	HMENU hmenuTrackPopup;  // shortcut menu 

	// Load the menu resource. 

	//if((hmenu = LoadMenu(hInst, MAKEINTRESOURCE(iMenu))) == NULL)
	//	return;
	hmenuTrackPopup = GetSubMenu(hMenu, iSubmenu);

	MENUITEMINFOA mii;
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	mii.fState = MFS_UNCHECKED;
	for(UINT i = 0, l = GetMenuItemCount(hmenuTrackPopup); i < l; ++i)
	{
		SetMenuItemInfoA(hmenuTrackPopup, i, TRUE, &mii);
	}
	if(iCheckItem > 0)
	{
		mii.fState = MFS_CHECKED;
		SetMenuItemInfoA(hmenuTrackPopup, iCheckItem, FALSE, &mii);
	}

	ClientToScreen(hwnd, (LPPOINT)&pt);
	// Display the shortcut menu. Track the right mouse 
	// button. 

	TrackPopupMenu(hmenuTrackPopup,
		TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		pt.x, pt.y, 0, hwnd, NULL);

	// Destroy the menu. 

	//DestroyMenu(hmenu);
}

void XInitViewportLayout(X_VIEWPORT_LAYOUT layout)
{
	MENUITEMINFOA mii;
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	mii.fState = MFS_UNCHECKED;
	SetMenuItemInfoA(g_hMenu, ID_VEWPORTCONFIGURATION_1X2, FALSE, &mii);
	SetMenuItemInfoA(g_hMenu, ID_VEWPORTCONFIGURATION_2X2, FALSE, &mii);
	SetMenuItemInfoA(g_hMenu, ID_VEWPORTCONFIGURATION_2X1, FALSE, &mii);
	SetMenuItemInfoA(g_hMenu, ID_VEWPORTCONFIGURATION_3DONLY, FALSE, &mii);
	UINT uMenuId = 0;
	mii.fState = MFS_CHECKED;
	switch(layout)
	{
	case XVIEW_2X2:
		uMenuId = ID_VEWPORTCONFIGURATION_2X2;
		ShowWindow(g_hTopRightWnd, TRUE);
		ShowWindow(g_hBottomLeftWnd, TRUE);
		ShowWindow(g_hBottomRightWnd, TRUE);
		g_isXResizeable = TRUE;
		g_isYResizeable = TRUE;
		break;
	case XVIEW_1X2:
		uMenuId = ID_VEWPORTCONFIGURATION_1X2;
		ShowWindow(g_hTopRightWnd, TRUE);
		ShowWindow(g_hBottomLeftWnd, FALSE);
		ShowWindow(g_hBottomRightWnd, FALSE);
		g_isXResizeable = TRUE;
		g_isYResizeable = FALSE;
		break;
	case XVIEW_2X1:
		uMenuId = ID_VEWPORTCONFIGURATION_2X1;
		ShowWindow(g_hTopRightWnd, FALSE);
		ShowWindow(g_hBottomLeftWnd, TRUE);
		ShowWindow(g_hBottomRightWnd, FALSE);
		g_isXResizeable = FALSE;
		g_isYResizeable = TRUE;
		break;
	case XVIEW_3DONLY:
		uMenuId = ID_VEWPORTCONFIGURATION_3DONLY;
		ShowWindow(g_hTopRightWnd, FALSE);
		ShowWindow(g_hBottomLeftWnd, FALSE);
		ShowWindow(g_hBottomRightWnd, FALSE);
		g_isXResizeable = FALSE;
		g_isYResizeable = FALSE;
		break;
	}
	
	PostMessage(g_hWndMain, WM_COMMAND, MAKEWPARAM(ID_VIEW_AUTOSIZEVIEWS, 0), 0);

	g_xConfig.m_xViewportLayout = layout;
	SetMenuItemInfoA(g_hMenu, uMenuId, FALSE, &mii);
}

BOOL XCheckMenuItem(HMENU hMenu, UINT uIDCheckItem, bool bCheck)
{
	MENUITEMINFOA mii;
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	mii.fState = bCheck ? MFS_CHECKED : MFS_UNCHECKED;
	return(SetMenuItemInfoA(hMenu, uIDCheckItem, FALSE, &mii));
}

float XGetGridStep()
{
	float fGridStep = -1.0f;
	switch(g_xConfig.m_gridStep)
	{
	case GRID_STEP_1CM:
		fGridStep = 0.01f;
		break;
	case GRID_STEP_2CM:
		fGridStep = 0.02f;
		break;
	case GRID_STEP_5CM:
		fGridStep = 0.05f;
		break;
	case GRID_STEP_10CM:
		fGridStep = 0.1f;
		break;
	case GRID_STEP_20CM:
		fGridStep = 0.2f;
		break;
	case GRID_STEP_50CM:
		fGridStep = 0.5f;
		break;
	case GRID_STEP_1M:
		fGridStep = 1.0f;
		break;
	case GRID_STEP_2M:
		fGridStep = 2.0f;
		break;
	case GRID_STEP_5M:
		fGridStep = 5.0f;
		break;
	case GRID_STEP_10M:
		fGridStep = 10.0f;
		break;
	case GRID_STEP_20M:
		fGridStep = 20.0f;
		break;
	case GRID_STEP_50M:
		fGridStep = 50.0f;
		break;
	}

	return(fGridStep);
}

float3 XSnapToGrid(const float3 &vPos)
{
	if(g_xConfig.m_bSnapGrid)
	{
		float fGridStep = XGetGridStep();

		if(fGridStep > 0.0f)
		{
			float3 vDelta(
				fmodf(vPos.x, fGridStep),
				fmodf(vPos.y, fGridStep),
				fmodf(vPos.z, fGridStep)
				);
			float fHalfStep = fGridStep * 0.5f;
			if(vDelta.x > fHalfStep)
			{
				vDelta.x -= fGridStep;
			}
			else if(vDelta.x < -fHalfStep)
			{
				vDelta.x += fGridStep;
			}

			if(vDelta.y > fHalfStep)
			{
				vDelta.y -= fGridStep;
			}
			else if(vDelta.y < -fHalfStep)
			{
				vDelta.y += fGridStep;
			}

			if(vDelta.z > fHalfStep)
			{
				vDelta.z -= fGridStep;
			}
			else if(vDelta.z < -fHalfStep)
			{
				vDelta.z += fGridStep;
			}

			return(vPos - vDelta);
		}
	}
	return(vPos);
}

void XUpdateStatusGrid()
{
	char szMsg[64];
	const char *szGrid = "";
	switch(g_xConfig.m_gridStep)
	{
	case GRID_STEP_1CM:
		szGrid = "1cm";
		break;
	case GRID_STEP_2CM:
		szGrid = "2cm";
		break;
	case GRID_STEP_5CM:
		szGrid = "5cm";
		break;
	case GRID_STEP_10CM:
		szGrid = "10cm";
		break;
	case GRID_STEP_20CM:
		szGrid = "20cm";
		break;
	case GRID_STEP_50CM:
		szGrid = "50cm";
		break;
	case GRID_STEP_1M:
		szGrid = "1m";
		break;
	case GRID_STEP_2M:
		szGrid = "2m";
		break;
	case GRID_STEP_5M:
		szGrid = "5m";
		break;
	case GRID_STEP_10M:
		szGrid = "10m";
		break;
	case GRID_STEP_20M:
		szGrid = "20m";
		break;
	case GRID_STEP_50M:
		szGrid = "50m";
		break;
	case GRID_STEP_100M:
		szGrid = "100m";
		break;
	case GRID_STEP_AXES:
		szGrid = "axes";
		break;
	}

	sprintf_s(szMsg, "Snap: %s; Grid: %s", g_xConfig.m_bSnapGrid ? "on" : "off", szGrid);
	SendMessage(g_hStatusWnd, SB_SETTEXT, MAKEWPARAM(4, 0), (LPARAM)szMsg);

	XCheckMenuItem(g_hMenu, ID_LEVEL_SNAPTOGRID, g_xConfig.m_bSnapGrid);
}

void XUpdateStatusMPos()
{
	if(g_xState.activeWindow == XWP_TOP_LEFT)
	{
		SendMessage(g_hStatusWnd, SB_SETTEXT, MAKEWPARAM(2, 0), (LPARAM)"");
	}
	else
	{
		char szMsg[32];
		sprintf_s(szMsg, "@%.2f, %.2f", g_xState.vWorldMousePos.x, g_xState.vWorldMousePos.y);
		SendMessage(g_hStatusWnd, SB_SETTEXT, MAKEWPARAM(2, 0), (LPARAM)szMsg);
	}
}

void XUpdateUndoRedo()
{
	MENUITEMINFOA mii;
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE | MIIM_STRING;
	char str[64]; 
	mii.dwTypeData = str;

	mii.fState = g_pUndoManager->canUndo() ? MFS_ENABLED : MFS_DISABLED;
	if(g_pUndoManager->canUndo())
	{
		mii.cch = sprintf(str, "Undo %s\tCtrl+Z", g_pUndoManager->getUndoText());
	}
	else
	{
		mii.cch = sprintf(str, "Can't undo\tCtrl+Z");
	}
	SetMenuItemInfoA(g_hMenu, ID_EDIT_UNDO, FALSE, &mii);
	SetMenuItemInfoA(g_hMenu2, ID_EDIT_UNDO, FALSE, &mii);

	mii.fState = g_pUndoManager->canRedo() ? MFS_ENABLED : MFS_DISABLED;
	if(g_pUndoManager->canRedo())
	{
		mii.cch = sprintf(str, "Redo %s\tCtrl+Y", g_pUndoManager->getRedoText());
	}
	else
	{
		mii.cch = sprintf(str, "Can't redo\tCtrl+Y");
	}
	SetMenuItemInfoA(g_hMenu, ID_EDIT_REDO, FALSE, &mii);
	SetMenuItemInfoA(g_hMenu2, ID_EDIT_REDO, FALSE, &mii);
}

//##########################################################################

#define XDECLARE_PROP_GIZMO(gizmo_type, callback, init) 						         \
class CPropGizmo##gizmo_type##Callback final: public IXEditorGizmo##gizmo_type##Callback \
{																				         \
public:																			         \
	CPropGizmo##gizmo_type##Callback()											         \
	{																			         \
		g_pEditor->newGizmo##gizmo_type(&m_pGizmo);								         \
		m_pGizmo->setCallback(this);											         \
	}																			         \
	~CPropGizmo##gizmo_type##Callback()											         \
	{																			         \
		mem_release(m_pGizmo);													         \
	}																			         \
																				         \
	callback																	         \
	init																	             \
																				         \
	void XMETHODCALLTYPE onStart(IXEditorGizmo##gizmo_type *pGizmo) override	         \
	{																			         \
		m_pCommand = new CCommandProperties();									         \
		m_pCommand->addObject(m_pObj);											         \
	}																			         \
	void XMETHODCALLTYPE onEnd(IXEditorGizmo##gizmo_type *pGizmo) override		         \
	{																			         \
		XExecCommand(m_pCommand);												         \
		m_pCommand = NULL;														         \
	}																			         \
																				         \
	IXEditorObject *m_pObj;														         \
	X_PROP_FIELD m_field;														         \
	UINT m_uUpdateRevision;														         \
																				         \
private:																		         \
	IXEditorGizmo##gizmo_type *m_pGizmo;										         \
	CCommandProperties *m_pCommand = NULL;										         \
};																						 \
Array<CPropGizmo##gizmo_type##Callback*> g_aPropGizmo##gizmo_type##Items;				 \
MemAlloc<CPropGizmo##gizmo_type##Callback> g_poolPropGizmo##gizmo_type##Items;

#define XCREATE_PROP_GIZMO(editor_type, gizmo)                                           \
case editor_type:                                                                        \
	for(UINT i = 0, l = g_aPropGizmo##gizmo##Items.size(); i < l; ++i)                   \
	{                                                                                    \
		auto *pItem = g_aPropGizmo##gizmo##Items[i];                                     \
		if(pItem->m_pObj == pObj && !fstrcmp(pItem->m_field.szKey, pField->szKey))       \
		{                                                                                \
			pItem->m_uUpdateRevision = g_uPropGizmoUpdateRevision;                       \
			pItem->init();                                                               \
			return;                                                                      \
		}                                                                                \
	}                                                                                    \
	{                                                                                    \
		auto *pCb = g_poolPropGizmo##gizmo##Items.Alloc();                               \
		pCb->m_pObj = pObj;                                                              \
		pCb->m_field = *pField;                                                          \
		pCb->m_uUpdateRevision = g_uPropGizmoUpdateRevision;                             \
		pCb->init();                                                                     \
		g_aPropGizmo##gizmo##Items.push_back(pCb);                                       \
	}                                                                                    \
	break

#define XCLEANUP_PROP_GIZMO(gizmo)                                                       \
	for(UINT i = 0, l = g_aPropGizmo##gizmo##Items.size(); i < l; ++i)                   \
	{                                                                                    \
		auto *pItem = g_aPropGizmo##gizmo##Items[i];                                     \
		if(pItem->m_uUpdateRevision != g_uPropGizmoUpdateRevision)                       \
		{                                                                                \
			g_aPropGizmo##gizmo##Items.erase(i); --i; --l;                               \
			g_poolPropGizmo##gizmo##Items.Delete(pItem);                                 \
		}                                                                                \
	}


XDECLARE_PROP_GIZMO(Radius, void XMETHODCALLTYPE onChange(float fNewRadius, IXEditorGizmoRadius *pGizmo) override
{
	pGizmo->setRadius(fNewRadius);
	char tmp[16];
	sprintf(tmp, "%f", fNewRadius);
	m_pCommand->setKV(m_field.szKey, tmp);
}, void init()
{
	m_pGizmo->setPos(m_pObj->getPos());
	float fRadius;
	if(sscanf(m_pObj->getKV(m_field.szKey), "%f", &fRadius))
	{
		m_pGizmo->setRadius(fRadius);
	}
});

XDECLARE_PROP_GIZMO(Handle, void XMETHODCALLTYPE moveTo(const float3 &vNewPos, IXEditorGizmoHandle *pGizmo) override
{
	pGizmo->setPos(vNewPos);
	char tmp[64];
	sprintf(tmp, "%f %f %f", vNewPos.x, vNewPos.y, vNewPos.z);
	m_pCommand->setKV(m_field.szKey, tmp);
}, void init()
{
	float3_t vec;
	if(sscanf(m_pObj->getKV(m_field.szKey), "%f %f %f", &vec.x, &vec.y, &vec.z))
	{
		m_pGizmo->setPos(vec);
	}
});

UINT g_uPropGizmoUpdateRevision = 0;

void XInitPropGizmo(IXEditorObject *pObj, const X_PROP_FIELD *pField)
{
	assert(pField->useGizmo);

	switch(pField->editorType)
	{
		XCREATE_PROP_GIZMO(XPET_POINTCOORD, Handle);
		XCREATE_PROP_GIZMO(XPET_RADIUS, Radius);

	default:
		return;
	}
}

void XCleanupUnreferencedPropGizmos()
{
	XCLEANUP_PROP_GIZMO(Handle);
	XCLEANUP_PROP_GIZMO(Radius);
	++g_uPropGizmoUpdateRevision;
}

#undef XCREATE_PROP_GIZMO
#undef XCLEANUP_PROP_GIZMO

//##########################################################################

void XUpdatePropWindow()
{
	g_propertyCallback.start();

	const char *szFirstType = NULL;
	const char *szFirstClass = NULL;
	bool bDifferentClasses = false;
	bool bDifferentTypes = false;
	struct prop_item_s
	{
		X_PROP_FIELD xPropField;
		bool isEnabled;
		const char *szValue;
	};
	AssotiativeArray<AAString, prop_item_s> mProps;
	g_pPropWindow->clearClassList();
	g_pPropWindow->clearProps();

	UINT uSelectedCount = 0;

	XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
		if(pObj->isSelected() && (g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent))
		{
			++uSelectedCount;
			if(!szFirstType)
			{
				szFirstType = pObj->getTypeName();
				szFirstClass = pObj->getClassName();
			}
			else
			{
				if(!bDifferentTypes)
				{
					bDifferentTypes = fstrcmp(szFirstType, pObj->getTypeName()) != 0;
				}
				if(!bDifferentTypes && !bDifferentClasses)
				{
					bDifferentClasses = fstrcmp(szFirstClass, pObj->getClassName()) != 0;
				}
			}

			for(UINT i = 0, l = pObj->getProperyCount(); i < l; ++i)
			{
				const X_PROP_FIELD *pField = pObj->getPropertyMeta(i);

				if(pField->useGizmo)
				{
					XInitPropGizmo(pObj, pField);
				}

				const AssotiativeArray<AAString, prop_item_s>::Node *pNode;
				if(mProps.KeyExists(AAString(pField->szKey), &pNode))
				{
					if(pNode->Val->isEnabled)
					{
						if(pField->editorType != pNode->Val->xPropField.editorType
							|| fstrcmp(pField->szName, pNode->Val->xPropField.szName)
							|| pField->pEditorData != pNode->Val->xPropField.pEditorData
							)
						{
							pNode->Val->isEnabled = false;
						}
						else if(fstrcmp(pField->szHelp, pNode->Val->xPropField.szHelp))
						{
							pNode->Val->xPropField.szHelp = "";
						}

						if(pNode->Val->isEnabled)
						{
							if(fstrcmp(pObj->getKV(pField->szKey), pNode->Val->szValue))
							{
								pNode->Val->szValue = "";
							}
						}
					}
				}
				else
				{
					mProps[AAString(pField->szKey)] = {*pField, true, pObj->getKV(pField->szKey)};
				}
			}
		}
	});

	XCleanupUnreferencedPropGizmos();

	// Nothing selected
	if(!szFirstType)
	{
		SendMessage(g_hStatusWnd, SB_SETTEXT, MAKEWPARAM(1, 0), (LPARAM)"no selection");
		return;
	}

	char szStatus[256];
	szStatus[0] = 0;
	if(!bDifferentTypes)
	{
		// add classes for type
		const AssotiativeArray<AAString, IXEditable*>::Node *pNode;
		if(g_mEditableSystems.KeyExists(AAString(szFirstType), &pNode))
		{
			for(UINT i = 0, l = (*pNode->Val)->getClassCount(); i < l; ++i)
			{
				g_pPropWindow->addClass((*pNode->Val)->getClass(i));
			}
		}

		if(!bDifferentClasses)
		{
			//@TODO: uncomment this
			g_pPropWindow->allowClassChange(true);
			g_pPropWindow->setClassName(szFirstClass);

			if(uSelectedCount > 1)
			{
				sprintf(szStatus, "%ux %s", uSelectedCount, szFirstClass);
			}
			else
			{
				sprintf(szStatus, "%s", szFirstClass);
			}
		}
		else
		{
			g_pPropWindow->allowClassChange(false);
		}
	}
	else
	{
		g_pPropWindow->addClass("");
		g_pPropWindow->allowClassChange(false);
		g_pPropWindow->setClassName("");
	}

	if(!szStatus[0])
	{
		sprintf(szStatus, "%u objects selected", uSelectedCount);
	}

	SendMessage(g_hStatusWnd, SB_SETTEXT, MAKEWPARAM(1, 0), (LPARAM)szStatus);

	for(AssotiativeArray<AAString, prop_item_s>::Iterator i = mProps.begin(); i; ++i)
	{
		g_pPropWindow->addPropField(&i.second->xPropField, i.second->szValue);
	}

}

void XUpdateGizmos()
{
	if(Button_GetCheck(g_hABArrowButton) && g_xState.bHasSelection)
	{
		float3 vPos = (g_xState.vSelectionBoundMin + g_xState.vSelectionBoundMax) * 0.5f;
		g_pGizmoMove->setPos(vPos);
		if(!g_gizmoRotateCallback.isActive())
		{
			g_pGizmoRotate->setPos(vPos);
		}
		
		g_pGizmoMove->enable(g_xState.xformType == X2DXF_SCALE);
		g_pGizmoRotate->enable(g_xState.xformType == X2DXF_ROTATE);
	}
	else
	{
		g_pGizmoMove->enable(false);
		g_pGizmoRotate->enable(false);
	}
}

//#############################################################################

void XMETHODCALLTYPE CGizmoMoveCallback::moveTo(const float3 &vNewPos, IXEditorGizmoMove *pGizmo)
{
	m_pCmd->setCurrentPos(vNewPos);
	pGizmo->setPos(vNewPos);
}
void XMETHODCALLTYPE CGizmoMoveCallback::onStart(IXEditorGizmoMove *pGizmo)
{
	m_pCmd = new CCommandMove(GetKeyState(VK_SHIFT) < 0);
	m_pCmd->setStartPos(pGizmo->getPos());
	XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
		if(pObj->isSelected() && (g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent))
		{
			m_pCmd->addObject(pObj);
		}
	});
}
void XMETHODCALLTYPE CGizmoMoveCallback::onEnd(IXEditorGizmoMove *pGizmo)
{
	XExecCommand(m_pCmd);
	m_pCmd = NULL;
}

//#############################################################################

void XMETHODCALLTYPE CGizmoRotateCallback::onRotate(const float3_t &vAxis, float fAngle, IXEditorGizmoRotate *pGizmo)
{
	pGizmo->setDeltaAngle(fAngle);
	m_pCmd->setCurrentPos(pGizmo->getPos() + pGizmo->getOrient() * m_vOffset);
}
void XMETHODCALLTYPE CGizmoRotateCallback::onStart(const float3_t &vAxis, IXEditorGizmoRotate *pGizmo)
{
	float3 vStartOffset;
	if(fabsf(SMVector3Dot(vAxis, float3(0.0f, 1.0f, 0.0))) > 0.9f)
	{
		vStartOffset = float3(1.0f, 0.0f, 0.0f);
	}
	else
	{
		vStartOffset = float3(0.0f, 1.0f, 0.0f);
	}
	m_vOffset = vStartOffset = SMVector3Normalize(SMVector3Cross(vAxis, vStartOffset));

	m_pCmd = new CCommandRotate(GetKeyState(VK_SHIFT) < 0);
	m_pCmd->setStartOrigin(pGizmo->getPos(), vAxis);
	m_pCmd->setStartPos(pGizmo->getPos() + vStartOffset);
	XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
		if(pObj->isSelected() && (g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent))
		{
			m_pCmd->addObject(pObj);
		}
	});

	pGizmo->setOrient(SMQuaternion());
}
void XMETHODCALLTYPE CGizmoRotateCallback::onEnd(IXEditorGizmoRotate *pGizmo)
{
	XExecCommand(m_pCmd);
	m_pCmd = NULL;
}

//#############################################################################

HIMAGELIST g_hImageList = NULL;

void CheckToolbarButton(int iCmd, BOOL isChecked)
{
	SendMessage(g_hToolbarWnd, TB_CHECKBUTTON, iCmd, MAKELPARAM(isChecked, 0));
}

void CheckXformButton(X_2DXFORM_TYPE type, bool isChecked)
{
	int iCmd = 0;
	switch(type)
	{
	case X2DXF_NONE:
		iCmd = ID_XFORM_NONE;
		break;
	case X2DXF_SCALE:
		iCmd = ID_XFORM_TRANSLATE;
		break;
	case X2DXF_ROTATE:
		iCmd = ID_XFORM_ROTATE;
		break;
	}

	CheckToolbarButton(iCmd, isChecked);
}

HWND CreateToolbar(HWND hWndParent)
{
	// Declare and initialize local constants.
	const int ImageListID = 0;
	const int numButtons = 4;
	const int bitmapSize = 16;

	const DWORD buttonStyles = BTNS_AUTOSIZE;

	// Create the toolbar.
	HWND hWndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
		WS_CHILD | TBSTYLE_WRAPABLE | TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS, 0, 0, 0, 0,
		hWndParent, NULL, hInst, NULL);

	//SetWindowLong(hWndToolbar, GWL_EXSTYLE, GetWindowLong(hWndToolbar, GWL_EXSTYLE) | TBSTYLE_EX_MIXEDBUTTONS);

	if(hWndToolbar == NULL)
		return NULL;

	// Create the image list.
	g_hImageList = ImageList_Create(MulDpi(bitmapSize, g_uWndMainDpi), MulDpi(bitmapSize - 1, g_uWndMainDpi),   // Dimensions of individual bitmaps.
		ILC_COLOR16 | ILC_MASK,   // Ensures transparent background.
		numButtons, 0);

	HBITMAP hbToolbar = LoadBitmap(hInst, MAKEINTRESOURCE(IDR_TOOLBAR1));
	HBITMAP hbToolbarA = LoadBitmap(hInst, MAKEINTRESOURCE(IDR_TOOLBAR_A));
	
	if(g_uWndMainDpi != USER_DEFAULT_SCREEN_DPI)
	{
		HDC hDC = GetDC(hWndToolbar);

		HBITMAP hbToolbarScaled = StretchBitmap(hDC, hbToolbar);
		HBITMAP hbToolbarAScaled = StretchBitmap(hDC, hbToolbarA);

		DeleteBitmap(hbToolbar);
		DeleteBitmap(hbToolbarA);

		hbToolbar = hbToolbarScaled;
		hbToolbarA = hbToolbarAScaled;

		ReleaseDC(hWndToolbar, hDC);
	}

	ImageList_Add(g_hImageList, hbToolbar, hbToolbarA);
	DeleteBitmap(hbToolbar);
	DeleteBitmap(hbToolbarA);

	// Set the image list.
	SendMessage(hWndToolbar, TB_SETIMAGELIST,
		(WPARAM)ImageListID,
		(LPARAM)g_hImageList);


	// Load the button images.
	//SendMessage(hWndToolbar, TB_LOADIMAGES,
	//	(WPARAM)TB_LOADIMAGES,
	//	(LPARAM)HINST_COMMCTRL);

	TBBUTTON tbButtons[] =
	{
		{MAKELONG(0, ImageListID), ID_XFORM_NONE, TBSTATE_ENABLED, buttonStyles, {0}, 0, (INT_PTR)"Handle [Q]"},
		{MAKELONG(1, ImageListID), ID_XFORM_TRANSLATE, TBSTATE_ENABLED, buttonStyles, {0}, 0, (INT_PTR)"Move [W]"},
		{MAKELONG(2, ImageListID), ID_XFORM_ROTATE, TBSTATE_ENABLED, buttonStyles, {0}, 0, (INT_PTR)"Rotate [R]"},
		{0, 0, TBSTATE_ENABLED, BTNS_SEP, 0L, 0},
		{MAKELONG(5, ImageListID), ID_IGNORE_GROUPS, TBSTATE_ENABLED, buttonStyles, {0}, 0, (INT_PTR)"Toggle group ignore [Ctrl+W]"},
		{0, 0, TBSTATE_ENABLED, BTNS_SEP, 0L, 0},
		{MAKELONG(3, ImageListID), ID_LEVEL_RUN, TBSTATE_ENABLED, buttonStyles, {0}, 0, (INT_PTR)"Run [F5]"}
	};

	// Add buttons.
	SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
	SendMessage(hWndToolbar, TB_ADDBUTTONS, (WPARAM)ARRAYSIZE(tbButtons), (LPARAM)&tbButtons);

	// Resize the toolbar, and then show it.
	SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0);
	SendMessage(hWndToolbar, TB_SETEXTENDEDSTYLE, 0, (LPARAM)TBSTYLE_EX_MIXEDBUTTONS);
	ShowWindow(hWndToolbar, TRUE);

	return hWndToolbar;
}

void XSetXformType(X_2DXFORM_TYPE type)
{
	CheckXformButton(g_xState.xformType, false);
	g_xState.xformType = type;
	CheckXformButton(g_xState.xformType, true);
	// emit event

	XEventEditorXformType ev = {};
	ev.newXformType = type;
	g_pXformEventChannel->broadcastEvent(&ev);

	XUpdateGizmos();
}

void XInitTool(IXEditorTool *pTool, IXEditable *pEditable)
{
	HWND hBtn = CreateWindow("Button", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_BITMAP | BS_PUSHLIKE | BS_CHECKBOX,
		MulDpi(g_uABNextLeft, g_uWndMainDpi), MulDpi(g_uABNextTop, g_uWndMainDpi), MulDpi(AB_BUTTON_WIDTH, g_uWndMainDpi), MulDpi(AB_BUTTON_HEIGHT, g_uWndMainDpi), g_hWndMain, (HMENU)(IDC_AB_FIRST + g_aTools.size()), hInst, NULL
	);
	{
		SetWindowTheme(hBtn, L" ", L" ");
		HBITMAP hBitmap = (HBITMAP)pTool->getIcon();
		if(hBitmap) 
		{
			SendMessage(hBtn, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
		}
	}
	g_aTools.push_back({pTool, pTool->getAccel(), pEditable});
	g_uABNextTop += AB_BUTTON_HEIGHT;
}

void XInitCustomAccel()
{
	if(!g_aTools.size())
	{
		return;
	}
	int nAccel = CopyAcceleratorTable(g_hAccelTableMain, NULL, 0);
	Array<ACCEL> aAccel;
	aAccel.resize(nAccel);
	CopyAcceleratorTable(g_hAccelTableMain, aAccel, nAccel);

	fora(i, g_aTools)
	{
		ACCEL a = {};
		XAccelItem &itm = g_aTools[i].accel;
		if(itm.flags & XAF_ALT)
		{
			a.fVirt |= FALT;
		}
		if(itm.flags & XAF_CTRL)
		{
			a.fVirt |= FCONTROL;
		}
		if(itm.flags & XAF_SHIFT)
		{
			a.fVirt |= FSHIFT;
		}
		if(itm.flags & XAF_VIRTKEY)
		{
			a.fVirt |= FVIRTKEY;
		}
		a.key = itm.uKey;
		a.cmd = IDC_AB_FIRST + i;

		aAccel.push_back(a);
	}

	DestroyAcceleratorTable(g_hAccelTableMain);
	g_hAccelTableMain = CreateAcceleratorTable(aAccel, aAccel.size());
}

void XInitTypesCombo()
{
	static bool s_isCustomList = true;

	if(g_pCurrentTool && g_pCurrentTool->wantUseClassSelector())
	{
		if(s_isCustomList)
		{
			return;
		}
		s_isCustomList = true;
	}
	else
	{
		if(!s_isCustomList)
		{
			return;
		}
		s_isCustomList = false;
	}
		
	ComboBox_ResetContent(g_hComboTypesWnd);

	int iTypes = 0;

	if(g_pCurrentTool && g_pCurrentTool->wantUseClassSelector())
	{
		int idx = g_aTools.indexOf(g_pCurrentTool, [](const CustomTool &a, IXEditorTool *b){
			return(a.pTool == b);
		});
		assert(idx >= 0);
		if(idx >= 0)
		{
			ComboBox_AddString(g_hComboTypesWnd, g_aTools[idx].pEditable->getName());
			++iTypes;
		}

		EnableWindow(g_hCheckboxRandomScaleYawWnd, g_pCurrentTool->isRandomScaleOrYawSupported());
	}
	else
	{
		IXEditable *pEditable;
		fora(i, g_pEditableSystems)
		{
			pEditable = g_pEditableSystems[i];
			if(pEditable->getClassCount() != 0)
			{
				ComboBox_AddString(g_hComboTypesWnd, pEditable->getName());
				++iTypes;
			}
		}
		EnableWindow(g_hCheckboxRandomScaleYawWnd, TRUE);
	}

	if(iTypes > 0)
	{
		int iSel = ComboBox_FindStringExact(g_hComboTypesWnd, -1, g_sSelectedType.c_str());
		ComboBox_SetCurSel(g_hComboTypesWnd, iSel >= 0 ? iSel : 0);
		SendMessage(GetParent(g_hComboTypesWnd), WM_COMMAND, MAKEWPARAM(IDC_CMB_TYPE, CBN_SELCHANGE), (LPARAM)g_hComboTypesWnd);
	}

	ComboBox_Enable(g_hComboTypesWnd, iTypes > 1);
}

UINT GetWindowDPI(HWND hWnd)
{
	typedef HRESULT(WINAPI *PGetDpiForMonitor)(HMONITOR hmonitor, int dpiType, UINT* dpiX, UINT* dpiY);

	// Try to get the DPI setting for the monitor where the given window is located.
	// This API is Windows 8.1+.
	HMODULE hSHcore = LoadLibraryW(L"shcore");
	if(hSHcore)
	{
		PGetDpiForMonitor pGetDpiForMonitor = (PGetDpiForMonitor)GetProcAddress(hSHcore, "GetDpiForMonitor");
		if(pGetDpiForMonitor)
		{
			HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
			UINT uDpiX;
			UINT uDpiY;
			HRESULT hr = pGetDpiForMonitor(hMonitor, /*MDT_EFFECTIVE_DPI*/ 0, &uDpiX, &uDpiY);
			if(SUCCEEDED(hr))
			{
				return(uDpiX);
			}
		}
	}

	// We couldn't get the window's DPI above, so get the DPI of the primary monitor
	// using an API that is available in all Windows versions.
	HDC hScreenDC = GetDC(0);
	int iDpiX = GetDeviceCaps(hScreenDC, LOGPIXELSX);
	ReleaseDC(0, hScreenDC);

	return((UINT)iDpiX);
}

int MulDpi(int iUnscaled, UINT uCurrentDpi)
{
	return(MulDiv(iUnscaled, uCurrentDpi, USER_DEFAULT_SCREEN_DPI));
}

int DivDpi(int iUnscaled, UINT uCurrentDpi)
{
	return(MulDiv(iUnscaled, USER_DEFAULT_SCREEN_DPI, uCurrentDpi));
}

void DivDpiRect(RECT *pRc, UINT uCurrentDpi)
{
	pRc->top = DivDpi(pRc->top, uCurrentDpi);
	pRc->bottom = DivDpi(pRc->bottom, uCurrentDpi);
	pRc->left = DivDpi(pRc->left, uCurrentDpi);
	pRc->right = DivDpi(pRc->right, uCurrentDpi);
}

void MulDpiRect(RECT *pRc, UINT uCurrentDpi)
{
	pRc->top = MulDpi(pRc->top, uCurrentDpi);
	pRc->bottom = MulDpi(pRc->bottom, uCurrentDpi);
	pRc->left = MulDpi(pRc->left, uCurrentDpi);
	pRc->right = MulDpi(pRc->right, uCurrentDpi);
}

HFONT GetDefaultFont(UINT uDpi)
{
	NONCLIENTMETRICSW nonClientMetrics = {sizeof(nonClientMetrics)};
	if(SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(nonClientMetrics), &nonClientMetrics, 0))
	{
		HDC hScreenDC = GetDC(0);
		int iDpiX = GetDeviceCaps(hScreenDC, LOGPIXELSX);
		ReleaseDC(0, hScreenDC);

		//nonClientMetrics.lfMenuFont.lfHeight = -12 * (int)uDpi / 72;
		nonClientMetrics.lfMenuFont.lfHeight = MulDiv(nonClientMetrics.lfMenuFont.lfHeight, uDpi, iDpiX);
		HFONT hFont = CreateFontIndirectW(&nonClientMetrics.lfMenuFont);
		if(hFont)
		{
			return(hFont);
		}
	}

	return((HFONT)GetStockObject(DEFAULT_GUI_FONT));
}

void ReleaseDefaultFont(HFONT hFont)
{
	if(hFont != GetStockObject(DEFAULT_GUI_FONT))
	{
		DeleteObject(hFont);
	}
}

HBITMAP StretchBitmap(HDC hDC, HBITMAP hBitmap)
{
	BITMAP bm1, bm2;
	HBITMAP hBitmapNew;
	HDC hDC1, hDC2;
	hDC1 = CreateCompatibleDC(hDC);
	hDC2 = CreateCompatibleDC(hDC);
	
	GetObject(hBitmap, sizeof(BITMAP), (PSTR)&bm1);

	bm2 = bm1;
	bm2.bmWidth = MulDpi(bm1.bmWidth, g_uWndMainDpi);
	bm2.bmHeight = MulDpi(bm1.bmHeight, g_uWndMainDpi);
	bm2.bmWidthBytes = MulDpi(bm1.bmWidthBytes, g_uWndMainDpi);
	hBitmapNew = CreateBitmapIndirect(&bm2);

	SelectObject(hDC1, hBitmap);
	SelectObject(hDC2, hBitmapNew);

	SetStretchBltMode(hDC1, HALFTONE);
	StretchBlt(hDC2, 0, 0, bm2.bmWidth, bm2.bmHeight, hDC1, 0, 0, bm1.bmWidth, bm1.bmHeight, SRCCOPY);

	DeleteDC(hDC1);
	DeleteDC(hDC2);

	return(hBitmapNew);
}
