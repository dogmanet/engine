#include "Window.h"

#if defined(_WINDOWS)
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")
#endif

struct WINCOMPATTRDATA
{
	DWORD attribute; // the attribute to query, see below
	PVOID pData; // buffer to store the result
	ULONG dataSize; // size of the pData buffer
};

struct ACCENTPOLICY
{
	int AccentState;
	int AccentFlags;
	int GradientColor;
	int AnimationId;
};

// typedef BOOL (WINAPI *FNPTRSetWindowCompositionAttribute)(HWND hwnd, WINCOMPATTRDATA* pAttrData);

LRESULT CALLBACK CWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CWindow *pWindow = (CWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	
	if(pWindow)
	{
		if(msg == WM_ACTIVATE)
		{
			pWindow->AddRef();
			if(wParam == WA_INACTIVE)
			{
				BYTE abKeyboardState[256];
				if(GetKeyboardState(abKeyboardState))
				{
					for(UINT i = 0; i < ARRAYSIZE(abKeyboardState); ++i)
					{
						if(abKeyboardState[i] & 0x80)
						{
							pWindow->runCallback(WM_KEYUP, (WPARAM)i, 0);
						}
					}
				}
			}
			else
			{
				for(UINT i = 0; i < 256; ++i)
				{
					if(GetAsyncKeyState(i) < 0)
					{
						pWindow->runCallback(WM_KEYDOWN, (WPARAM)i, 0);
					}
				}
			}
			pWindow->Release();
		}

		if(msg == WM_CHAR || msg == WM_SYSCHAR)
		{
			char ch = LOWORD(wParam);
			WCHAR wch;
			MultiByteToWideChar(CP_ACP, 0, &ch, 1, &wch, 1);
			wParam = MAKEWPARAM(wch, HIWORD(wParam));
		}

		if(msg == WM_SHOWWINDOW)
		{
			pWindow->m_isVisible = (bool)wParam;
		}

		switch(msg)
		{
		case WM_MOUSEWHEEL:
			{
				POINT mc;
				mc.x = GET_X_LPARAM(lParam);
				mc.y = GET_Y_LPARAM(lParam);
				ScreenToClient(hWnd, &mc);

				lParam = MAKELPARAM(mc.x, mc.y);
			}
			// no break!;

		case WM_DESTROY:
		case WM_CLOSE:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_CHAR:
		case WM_ACTIVATE:
		case WM_SIZE:
		case WM_COMMAND:
		//case WM_KILLFOCUS:
		case WM_XBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_INPUT:
		case WM_SETCURSOR:
		case WM_MOUSEMOVE:
		case WM_ENTERSIZEMOVE:
		case WM_EXITSIZEMOVE:
			add_ref(pWindow);
			LRESULT res = pWindow->runCallback(msg, wParam, lParam);
			mem_release(pWindow);
			return(res);
		}
	}

	return(DefWindowProc(hWnd, msg, wParam, lParam));
}

CWindow::CWindow(HINSTANCE hInst, UINT uId, const XWINDOW_DESC *pWindowDesc, IXWindowCallback *pCallback, IXWindow *pParent):
	m_hInst(hInst),
	m_uId(uId),
	m_pCallback(pCallback),
	m_pParent(pParent)
{
	//assert(pCallback);
	add_ref(pCallback);

	m_windowDesc = *pWindowDesc;
	m_windowDesc.szTitle = NULL;

#if defined(_WINDOWS)
	UINT style = CS_HREDRAW | CS_VREDRAW;

	if(!(pWindowDesc->flags & XWF_BUTTON_CLOSE))
	{
		style |= CS_NOCLOSE;
	}

	char szClassName[64];
	sprintf_s(szClassName, "XWindowClass_%u", uId);

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = style;
	wcex.lpfnWndProc = (WNDPROC)WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szClassName;
	wcex.hIconSm = NULL;
	//wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	if(!RegisterClassEx(&wcex))
	{
		LibReport(REPORT_MSG_LEVEL_FATAL, "Unable to register window class!\n");
	}

	//#############################################################################


	DWORD wndStyle = WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME;

	if(pWindowDesc->flags & XWF_BUTTON_MINIMIZE)
	{
		wndStyle |= WS_MINIMIZEBOX | WS_SYSMENU;
	}
	if(pWindowDesc->flags & XWF_BUTTON_MAXIMIZE)
	{
		wndStyle |= WS_MAXIMIZEBOX | WS_SYSMENU;
	}
	if(pWindowDesc->flags & XWF_BUTTON_CLOSE)
	{
		wndStyle |= WS_SYSMENU;
	}
	if(pWindowDesc->flags & XWF_NOBORDER)
	{
		wndStyle &= ~(WS_SYSMENU | WS_CAPTION | WS_MAXIMIZE | WS_MINIMIZE | WS_THICKFRAME);
	}
	else if(pWindowDesc->flags & XWF_NORESIZE)
	{
		wndStyle &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
	}
	RECT rc = {0, 0, pWindowDesc->iSizeX, pWindowDesc->iSizeY};
	AdjustWindowRect(&rc, wndStyle, false);

	int iPosX = pWindowDesc->iPosX;
	int iPosY = pWindowDesc->iPosY;

	if(iPosX == XCW_USEDEFAULT)
	{
		iPosX = CW_USEDEFAULT;
	}
	if(iPosY == XCW_USEDEFAULT)
	{
		iPosY = CW_USEDEFAULT;
	}
	if(iPosX == XCW_CENTER)
	{
		iPosX = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
	}
	if(iPosY == XCW_CENTER)
	{
		iPosY = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
	}

	HWND hParentWnd = NULL;
	if(pParent)
	{
		FIXME("HACK: Remove that as soon as all windows is turned to xWindow");
		if(IsWindow((HWND)pParent))
		{
			hParentWnd = (HWND)pParent;
		}
		else
		{
			hParentWnd = (HWND)pParent->getOSHandle();
		}
	}

	m_hWnd = CreateWindowExA(0/*WS_EX_APPWINDOW*/, szClassName, pWindowDesc->szTitle, wndStyle, iPosX, iPosY, rc.right - rc.left, rc.bottom - rc.top, hParentWnd, NULL, wcex.hInstance, NULL);
	if(!m_hWnd)
	{
		LibReport(REPORT_MSG_LEVEL_FATAL, "Unable to create window!\n");
	}

	if(pWindowDesc->flags & XWF_TRANSPARENT)
	{
		DWM_BLURBEHIND dwmBlur = {0};
		dwmBlur.dwFlags = DWM_BB_ENABLE;
		dwmBlur.fEnable = TRUE;

		DwmEnableBlurBehindWindow(m_hWnd, &dwmBlur);

		ACCENTPOLICY policy = {3, 0/*32 | 64 | 128 | 256*/, 0, 0};
		WINCOMPATTRDATA data = {19, &policy, sizeof(ACCENTPOLICY)};

		/*
		// windows 10 dwm is buggy with overlapped windows :(
		HMODULE hUser32Mod = LoadLibrary("user32.dll");
		if(hUser32Mod)
		{
			FNPTRSetWindowCompositionAttribute SetWindowCompositionAttribute = (FNPTRSetWindowCompositionAttribute)GetProcAddress(hUser32Mod, "SetWindowCompositionAttribute");
			if(SetWindowCompositionAttribute)
			{
				SetWindowCompositionAttribute(m_hWnd, &data);
			}
		}*/

		// Extend the frame across the whole window.
		// MARGINS margins = {-1};
		// DwmExtendFrameIntoClientArea(m_hWnd, &margins);
	}

	SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);

	if(!(pWindowDesc->flags & XWF_INIT_HIDDEN))
	{
		ShowWindow(m_hWnd, SW_NORMAL);
	}	

	if(pWindowDesc->flags & XWF_NOBORDER)
	{
		LONG lStyle = GetWindowLong(m_hWnd, GWL_STYLE);
		lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
		SetWindowLong(m_hWnd, GWL_STYLE, lStyle);
	}
#endif
}
CWindow::~CWindow()
{
#if defined(_WINDOWS)
	char szClassName[64];
	sprintf_s(szClassName, "XWindowClass_%u", m_uId);

	SetWindowLongPtr(m_hWnd, GWLP_USERDATA, NULL);
	DestroyWindow(m_hWnd);
	UnregisterClassA(szClassName, m_hInst);
#endif
	mem_release(m_pCallback);
}

XWINDOW_OS_HANDLE XMETHODCALLTYPE CWindow::getOSHandle()
{
	return(m_hWnd);
}

void XMETHODCALLTYPE CWindow::hide()
{
	ShowWindow(m_hWnd, SW_MINIMIZE);
}
void XMETHODCALLTYPE CWindow::close()
{
	SendMessage(m_hWnd, WM_CLOSE, 0, 0);
}
void XMETHODCALLTYPE CWindow::show()
{
	ShowWindow(m_hWnd, SW_SHOW);
}
bool XMETHODCALLTYPE CWindow::isVisible()
{
	return(!!IsWindowVisible(m_hWnd));
}
void XMETHODCALLTYPE CWindow::setTitle(const char *szTitle)
{
	SetWindowTextA(m_hWnd, szTitle);
}
void XMETHODCALLTYPE CWindow::update(const XWINDOW_DESC *pWindowDesc)
{
	HMONITOR monitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info;
	info.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(monitor, &info);
	bool bForceNoBorder = (pWindowDesc->iSizeX == info.rcMonitor.right - info.rcMonitor.left
		&& pWindowDesc->iSizeY == info.rcMonitor.bottom - info.rcMonitor.top);


	if(pWindowDesc->flags & XWF_BUTTON_CLOSE)
	{
		SetClassLong(m_hWnd, GCL_STYLE, GetClassLong(m_hWnd, GCL_STYLE) & ~CS_NOCLOSE);
	}
	else
	{
		SetClassLong(m_hWnd, GCL_STYLE, GetClassLong(m_hWnd, GCL_STYLE) | CS_NOCLOSE);
	}

	DWORD wndStyle = GetWindowLong(m_hWnd, GWL_STYLE);

	if(pWindowDesc->flags & XWF_BUTTON_MINIMIZE)
	{
		wndStyle |= WS_MINIMIZEBOX | WS_SYSMENU;
	}
	if(pWindowDesc->flags & XWF_BUTTON_MAXIMIZE)
	{
		wndStyle |= WS_MAXIMIZEBOX | WS_SYSMENU;
	}
	if(pWindowDesc->flags & XWF_BUTTON_CLOSE)
	{
		wndStyle |= WS_SYSMENU;
	}
	if((pWindowDesc->flags & XWF_NOBORDER) || bForceNoBorder)
	{
		wndStyle &= ~(WS_SYSMENU | WS_CAPTION | WS_MAXIMIZE | WS_MINIMIZE | WS_THICKFRAME);
	}
	else
	{
		wndStyle |= WS_CAPTION | WS_THICKFRAME;

		if(pWindowDesc->flags & XWF_NORESIZE)
		{
			wndStyle &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
		}
	}

	SetWindowLong(m_hWnd, GWL_STYLE, wndStyle);

	UINT uSWPflags = SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOOWNERZORDER;
	//SetWindowPos(m_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOOWNERZORDER);

	RECT rc = {0, 0, pWindowDesc->iSizeX, pWindowDesc->iSizeY};
	AdjustWindowRect(&rc, wndStyle, false);

	RECT rcOld;
	GetWindowRect(m_hWnd, &rcOld);


	if(rcOld.right - rcOld.left != rc.right - rc.left || rcOld.bottom - rcOld.top != rc.bottom - rc.top)
	{
		uSWPflags &= ~SWP_NOSIZE;
		//SetWindowPos(m_hWnd, HWND_TOP, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	}

	int iPosX = pWindowDesc->iPosX;
	int iPosY = pWindowDesc->iPosY;

	if(bForceNoBorder)
	{
		iPosX = 0;
		iPosY = 0;

		uSWPflags &= ~SWP_NOMOVE;
	}
	else if(pWindowDesc->iPosX != XCW_USEDEFAULT && pWindowDesc->iPosY != XCW_USEDEFAULT)
	{
		if(iPosX == XCW_CENTER)
		{
			iPosX = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
		}
		if(iPosY == XCW_CENTER)
		{
			iPosY = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
		}

		uSWPflags &= ~SWP_NOMOVE;
		//SetWindowPos(m_hWnd, HWND_TOP, iPosX, iPosY, 0, 0, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	}
	SetWindowPos(m_hWnd, HWND_TOP, iPosX, iPosY, rc.right - rc.left, rc.bottom - rc.top, uSWPflags);

	if(pWindowDesc->szTitle)
	{
		SetWindowTextA(m_hWnd, pWindowDesc->szTitle);
	}

	m_windowDesc = *pWindowDesc;
	m_windowDesc.szTitle = NULL;
}

INT_PTR XMETHODCALLTYPE CWindow::runDefaultCallback(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_CLOSE:
		ShowWindow(m_hWnd, SW_HIDE);
		return(TRUE);
	}
	return(DefWindowProcA(m_hWnd, msg, wParam, lParam));
}

const XWINDOW_DESC* XMETHODCALLTYPE CWindow::getDesc()
{
	return(&m_windowDesc);
}

IXWindow* XMETHODCALLTYPE CWindow::getParent()
{
	return(m_pParent);
}

bool XMETHODCALLTYPE CWindow::getPlacement(XWindowPlacement *pPlacement)
{
	pPlacement->length = sizeof(*pPlacement);
	bool res = GetWindowPlacement(m_hWnd, pPlacement);

	if(res && !m_isVisible)
	{
		pPlacement->showCmd = SW_HIDE;
	}

	return(res);
}
void XMETHODCALLTYPE CWindow::setPlacement(const XWindowPlacement &placement, bool bSkipVisibility)
{
	if(bSkipVisibility)
	{
		XWindowPlacement p = placement;
		p.showCmd = isVisible() ? SW_SHOWNORMAL : SW_HIDE;
		SetWindowPlacement(m_hWnd, &p);
	}
	else
	{
		SetWindowPlacement(m_hWnd, &placement);
	}
}

INT_PTR CWindow::runCallback(UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(msg == WM_EXITSIZEMOVE || msg == WM_SIZE)
	{
		RECT rc;
		if(GetClientRect(m_hWnd, &rc))
		{
			m_windowDesc.iSizeX = rc.right - rc.left;
			m_windowDesc.iSizeY = rc.bottom - rc.top;
		}

		if(GetWindowRect(m_hWnd, &rc))
		{
			m_windowDesc.iPosX = rc.left;
			m_windowDesc.iPosY = rc.top;
		}
	}

	if(m_pCallback)
	{
		return(m_pCallback->onMessage(msg, wParam, lParam, this));
	}
	return(runDefaultCallback(msg, wParam, lParam));
}
