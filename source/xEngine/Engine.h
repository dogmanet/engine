#ifndef __ENGINE_H
#define __ENGINE_H

#include "IXEngine.h"
#include <xUI/IXUI.h>
#include <xcommon/render/IXRender.h>

// #define USE_BREAKPAD

#ifdef USE_BREAKPAD
#	include <breakpad/src/client/windows/handler/exception_handler.h>
#endif

enum WANT_RESIZE
{
	WR_NONE = 0x0000,
	WR_WIDTH = 0x0001,
	WR_HEIGHT = 0x0002,
	WR_WINDOWED = 0x0004,
	WR_BORDERLESS = 0x0008,
	WR_VSYNC = 0x0010
};

class CEngine final: public IXUnknownImplementation<IXEngine>
{
	friend class CMainLoopTask;
public:
	CEngine(int argc, char **argv, const char *szName);
	~CEngine();

	int XMETHODCALLTYPE start() override;

	bool XMETHODCALLTYPE initGraphics(XWINDOW_OS_HANDLE hWindow, IXEngineCallback *pCallback) override;
	bool XMETHODCALLTYPE initServer() override;

	INT_PTR XMETHODCALLTYPE onMessage(UINT msg, WPARAM wParam, LPARAM lParam) override;
	IXCore* XMETHODCALLTYPE getCore() override;

private:

	void initPaths();

	bool runFrame();

	void onRWinWidthChanged();
	void onRWinHeightChanged();
	void onRWinWindowedChanged();
	void onRWinBorderlessChanged();
	void onRVSyncChanged();

	bool checkResize();

	void showProfile();

private:
	IXEngineCallback *m_pCallback = NULL;

	IXCore *m_pCore = NULL;

	IXRender *m_pRender = NULL;
	IXRenderTarget *m_pScreenTarget = NULL;

	WANT_RESIZE m_wantResize = WR_NONE;

	IXUI *m_pXUI = NULL;

	IEventChannel<XEventObserverChanged> *m_pObserverChangedEventChannel = NULL;

	std::chrono::high_resolution_clock::time_point m_fPrevTime = std::chrono::high_resolution_clock::now();
	float m_fDeltaTime = 0.0f;

#ifdef USE_BREAKPAD
	google_breakpad::ExceptionHandler *m_pBreakpadHandler = NULL;
#endif
};

DEFINE_ENUM_FLAG_OPERATORS(WANT_RESIZE);

#endif
