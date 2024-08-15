#ifndef __IXENGINE_H
#define __IXENGINE_H

#include <gdefines.h>
#include <xcommon/IXCore.h>


#if defined(_WINDOWS)
#	if defined(XENGINE_EXPORTS)
#		define XAPI __declspec(dllexport)
#	else
#		define XAPI __declspec(dllimport)
#	endif
#else
#	define XAPI
#endif
#define EXTERN_C extern "C"

class IXEngine;
class IXCamera;

class IXEngineCallback
{
public:
	virtual void XMETHODCALLTYPE onGraphicsResize(UINT uWidth, UINT uHeight, bool isFullscreen, bool isBorderless, IXEngine *pEngine) = 0;

	virtual bool XMETHODCALLTYPE processWindowMessages() = 0;

	virtual IXCamera* XMETHODCALLTYPE getCameraForFrame() = 0;
};

class IXEngine: public IXUnknown
{
public:
	virtual int XMETHODCALLTYPE start() = 0;

	virtual bool XMETHODCALLTYPE initGraphics(XWINDOW_OS_HANDLE hWindow, IXEngineCallback *pCallback) = 0;
	virtual bool XMETHODCALLTYPE initServer() = 0;

	virtual INT_PTR XMETHODCALLTYPE onMessage(UINT msg, WPARAM wParam, LPARAM lParam) = 0;
	virtual IXCore* XMETHODCALLTYPE getCore() = 0;
};

EXTERN_C XAPI LPSTR* CommandLineToArgvA(LPSTR lpCmdline, int *numargs);
EXTERN_C XAPI IXEngine* XEngineInit(int argc, char **argv, const char *szName);

#endif
