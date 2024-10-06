#ifndef __IXWINDOWSYSTEM_H
#define __IXWINDOWSYSTEM_H

#include <gdefines.h>

#if defined(_WINDOWS)
#	if defined(XWINDOW_EXPORTS)
#		define XWAPI __declspec(dllexport)
#	else
#		define XWAPI __declspec(dllimport)
#	endif
#else
#	define XWAPI
#endif
#define EXTERN_C extern "C"

enum XWINDOW_FLAG
{
	XWF_DEFAULT = 0x0000,
	XWF_NOBORDER = 0x0001,
	XWF_BUTTON_CLOSE = 0x0002,
	XWF_BUTTON_MINIMIZE = 0x0004,
	XWF_BUTTON_MAXIMIZE = 0x0008,
	XWF_TRANSPARENT = 0x0010,
	XWF_NORESIZE = 0x0020,
	XWF_INIT_HIDDEN = 0x0040
};
DEFINE_ENUM_FLAG_OPERATORS(XWINDOW_FLAG);

#define XCW_USEDEFAULT ((int)0x80000000)
#define XCW_CENTER ((int)0x80000001)

struct XWINDOW_DESC
{
	int iSizeX = 800;
	int iSizeY = 600;
	int iPosX = XCW_USEDEFAULT;
	int iPosY = XCW_USEDEFAULT;
	XWINDOW_FLAG flags = XWF_DEFAULT;
	const char *szTitle = NULL;
};

typedef WINDOWPLACEMENT XWindowPlacement;


/*struct XWindowPlacement
{
	byte _private[sizeof(WINDOWPLACEMENT)];
};*/

//#############################################################################

class IXWindow: public IXUnknown
{
public:
	//! 
	virtual XWINDOW_OS_HANDLE XMETHODCALLTYPE getOSHandle() = 0;

	//! Скрывает окно (остается в таскбаре)
	virtual void XMETHODCALLTYPE hide() = 0;

	//! Закрывает окно (исчезает из таскбара)
	virtual void XMETHODCALLTYPE close() = 0;

	//! Открывает окно
	virtual void XMETHODCALLTYPE show() = 0;

	//! Видимо ли окно (открыто и не свернуто)
	virtual bool XMETHODCALLTYPE isVisible() = 0;

	//! 
	virtual void XMETHODCALLTYPE setTitle(const char *szTitle) = 0;

	//! 
	virtual void XMETHODCALLTYPE update(const XWINDOW_DESC *pWindowDesc) = 0;

	//! 
	virtual INT_PTR XMETHODCALLTYPE runDefaultCallback(UINT msg, WPARAM wParam, LPARAM lParam) = 0;

	//! 
	virtual const XWINDOW_DESC* XMETHODCALLTYPE getDesc() = 0;

	virtual IXWindow* XMETHODCALLTYPE getParent() = 0;

	virtual bool XMETHODCALLTYPE getPlacement(XWindowPlacement *pPlacement) = 0;
	virtual void XMETHODCALLTYPE setPlacement(const XWindowPlacement &placement, bool bSkipVisibility = false) = 0;
};

class IXWindowCallback: public IXUnknown
{
public:
	virtual INT_PTR XMETHODCALLTYPE onMessage(UINT msg, WPARAM wParam, LPARAM lParam, IXWindow *pWindow) = 0;
};


// {DD8E8BED-5471-44E8-A7E8-F4135BFD76B6}
#define IXWINDOWSYSTEM_GUID DEFINE_XGUID(0xdd8e8bed, 0x5471, 0x44e8, 0xa7, 0xe8, 0xf4, 0x13, 0x5b, 0xfd, 0x76, 0xb6)

class IXWindowSystem: public IXUnknown
{
public:
	//! 
	virtual IXWindow* XMETHODCALLTYPE createWindow(const XWINDOW_DESC *pWindowDesc, IXWindowCallback *pCallback = NULL, IXWindow *pParent = NULL) = 0;

	//! 
	virtual bool XMETHODCALLTYPE processMessages(UINT uMaxMessages = 0) = 0;
};

EXTERN_C XWAPI IXWindowSystem* XWindowInit();

#endif
