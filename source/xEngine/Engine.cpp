#include "Engine.h"
#include <core/task.h>
#include <GRegisterIndex.h>

#include <core/taskmanager.h>

#include "SkyXEngine.h"

#include <xcommon/IXSoundSystem.h>

#ifdef _DEBUG
#	pragma comment(lib, "sxcore_d.lib")
#else
#	pragma comment(lib, "sxcore.lib")
#endif

#ifdef USE_BREAKPAD
#	pragma comment(lib, "crash_generation_client.lib")
#	pragma comment(lib, "exception_handler.lib")
#	pragma comment(lib, "common.lib")
#endif

DECLARE_PROFILER_INTERNAL();

class CMainLoopTask: public ITaskImpl<ITask>
{
public:
	CMainLoopTask(CEngine *pEngine):
		m_pEngine(pEngine),
		ITaskImpl(CORE_TASK_FLAG_MAINTHREAD_REPEATING)
	{
	}

	void run() override
	{
		if(!m_pEngine->runFrame())
		{
			XCoreStop();
		}
	}

protected:
	CEngine *m_pEngine;
};

//##########################################################################

#ifdef USE_BREAKPAD
static bool HandleCrashDump(const wchar_t *wszDumpPath, const wchar_t* wszMinidumpId, void* pContext, EXCEPTION_POINTERS* pExInfo, MDRawAssertionInfo* pAssertion, bool isSucceeded)
{
	wchar_t *pCrashReporter = (wchar_t*)pContext;
	if(isSucceeded)
	{
		wcscat(pCrashReporter, wszMinidumpId);
		STARTUPINFOW info = {sizeof(info)};
		PROCESS_INFORMATION processInfo;
		if(CreateProcessW(NULL, pCrashReporter, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo))
		{
			CloseHandle(processInfo.hProcess);
			CloseHandle(processInfo.hThread);
		}
	}
	else
	{
		MessageBoxW(NULL, L"Program has crashed. Failed to write crashdump.", L"Crashed!", MB_OK | MB_ICONSTOP);
	}
	//MessageBoxW(NULL, wszMinidumpId, pCrashReporter, MB_OK | MB_ICONSTOP);
	return(true);
}
#endif

//##########################################################################

#define __LTEXT(text) L##text
#define LTEXT(text) __LTEXT(text)

static const wchar_t* dirnameW(wchar_t *str)
{
	wchar_t *pos = str, *ret = str;
	while(*str)
	{
		if((*str == '/' || *str == '\\') && *(str + 1))
		{
			pos = str + 1;
		}
		++str;
	}
	*pos = 0;
	return(ret);
}

CEngine::CEngine(int argc, char **argv, const char *szName)
{
	srand((UINT)time(0));

	Core_0LoadCommandLine(argc, argv);
	//SkyXEngine_InitOutLog();
	initPaths();

	{
		wchar_t szPath[MAX_PATH];
		GetModuleFileNameW(NULL, szPath, MAX_PATH);
		dirnameW(szPath);
		dirnameW(szPath);
		wcscat(szPath, L"gamesource/");
		SetCurrentDirectoryW(szPath);
	}

#ifdef USE_BREAKPAD
	{
		static wchar_t szPath[MAX_PATH];
		GetModuleFileNameW(NULL, szPath, MAX_PATH);
		int iLastPos = -1;
		for(size_t i = 0, l = wcslen(szPath); i < l; ++i)
		{
			if(szPath[i] == L'/' || szPath[i] == L'\\')
			{
				iLastPos = (int)i;
			}
		}
		if(iLastPos >= 0)
		{
			szPath[iLastPos + 1] = 0;
		}
		wcscat(szPath, L"crashreporter.exe +release ");
		wcscat(szPath, CMB2WC(szName));
		wcscat(szPath, L"@" LTEXT(SKYXENGINE_VERSION) L"+" LTEXT(SKYXENGINE_BUILD_NUMBER) L" -dumpid "); // Keep last space!
		
		CreateDirectoryA("../crashdmp", NULL);
		m_pBreakpadHandler = new google_breakpad::ExceptionHandler(
			L"../crashdmp/",
			NULL,
			HandleCrashDump,
			szPath,
			google_breakpad::ExceptionHandler::HANDLER_ALL,
			MiniDumpNormal,
			L"",
			NULL);
	}
#endif


	m_pCore = XCoreInit(szName);
	INIT_OUTPUT_STREAM(m_pCore);
	INIT_PROFILER_INTERNAL();
	LibReport(REPORT_MSG_LEVEL_NOTICE, "LIB core initialized\n");

	printf(CONSOLE_TITLE "SkyXEngine %s version " SKYXENGINE_VERSION "+" SKYXENGINE_BUILD_NUMBER CONSOLE_TITLE_END, szName);

	m_pObserverChangedEventChannel = m_pCore->getEventChannel<XEventObserverChanged>(EVENT_OBSERVER_CHANGED_GUID);

	Core_0RegisterCVarString("engine_version", SKYXENGINE_VERSION, "Текущая версия движка", FCVAR_READONLY);
	Core_0RegisterCVarString("engine_build", SKYXENGINE_BUILD_NUMBER, "Текущая версия движка", FCVAR_READONLY);
}
CEngine::~CEngine()
{
	mem_release(m_pXUI);

	SGame_AKill();
	SPhysics_AKill();
	//SSInput_AKill();
	mem_delete(m_pCore);

#ifdef USE_BREAKPAD
	mem_delete(m_pBreakpadHandler);
#endif
}

bool XMETHODCALLTYPE CEngine::initGraphics(XWINDOW_OS_HANDLE hWindow, IXEngineCallback *pCallback)
{
	XPROFILE_FUNCTION();

	m_pCallback = pCallback;

	// init input
	SSInput_0Create("sxinput", (HWND)hWindow, false);
	LibReport(REPORT_MSG_LEVEL_NOTICE, "LIB input initialized\n");



	// init sound
	AudioRawDesc oAudioDesc;
	oAudioDesc.u8Channels = 2;
	oAudioDesc.fmtSample = AUDIO_SAMPLE_FMT_SINT16;
	oAudioDesc.uSampleRate = 44100;
	oAudioDesc.calc();

	IXSoundSystem *pSound = (IXSoundSystem*)(m_pCore->getPluginManager()->getInterface(IXSOUNDSYSTEM_GUID));
	IXSoundLayer *pMasterLayer = pSound->createMasterLayer(&oAudioDesc, "master");
	if(pMasterLayer)
	{
		pMasterLayer->play(true);
		IXSoundLayer *pGameLayer = pMasterLayer->newSoundLayer("xGame");
		IXSoundLayer *pGuiLayer = pMasterLayer->newSoundLayer("xGUI");
		pGameLayer->play(false);
		pGuiLayer->play(false);
	}
	/*IXSoundPlayer *pPlayer = pMasterLayer->newSoundPlayer("sounds/guitar_10.ogg", SOUND_DTYPE_3D);
	pPlayer->setWorldPos(float3(-11.084, 0.435, -18.707));
	pPlayer->setLoop(SOUND_LOOP_SIMPLE);
	pPlayer->play();*/
	/*IXSoundEmitter *pEmitter = pMasterLayer->newSoundEmitter("sounds/ak74_shoot.ogg", SOUND_DTYPE_2D);
	//pEmitter->play();

	IXSoundEmitter *pEmitter2 = pMasterLayer->newSoundEmitter("sounds/ak74_shoot.ogg", SOUND_DTYPE_2D);
	//pEmitter2->play();

	while (1)
	{
		if (GetAsyncKeyState('I'))
		{
			pEmitter2->play();
			Sleep(100);
		}

		if (GetAsyncKeyState('Q'))
			pMasterLayer->play(false);
		
		if (GetAsyncKeyState('W'))
			pMasterLayer->play(true);

		float3 v;
		pSound->update(v,v,v);
	}*/
	LibReport(REPORT_MSG_LEVEL_NOTICE, "LIB sound initialized\n");



	// init graphics
	Core_0RegisterCVarInt("r_win_width", 800, "Размер окна по горизонтали (в пикселях)", FCVAR_NOTIFY_OLD);
	Core_0RegisterCVarInt("r_win_height", 600, "Размер окна по вертикали (в пикселях)", FCVAR_NOTIFY_OLD);
	Core_0RegisterCVarBool("r_win_windowed", true, "Режим рендера true - оконный, false - полноэкранный", FCVAR_NOTIFY_OLD);
	Core_0RegisterCVarBool("r_win_borderless", false, "Режим без рамки", FCVAR_NOTIFY_OLD);
	Core_0RegisterCVarBool("r_vsync", false, "Включить вертикальную синхронизацию", FCVAR_NOTIFY_OLD);
	Core_0RegisterCVarFloat("r_default_fov", 45.0f /*SM_PI * 0.25f*/, "Дефолтный fov в градусах");
	Core_0RegisterCVarFloat("r_near", 0.025f, "Ближняя плоскость отсчечения", FCVAR_NOTIFY);
	Core_0RegisterCVarFloat("r_far", 800.0f, "Дальняя плоскость отсечения (дальность видимости)", FCVAR_NOTIFY);
	Core_0RegisterCVarInt("dev_gpu_profile_framerate", 0, "Раз в сколько кадров выводить данные профайлинга (0 отключает эту возможность)", FCVAR_NOTIFY);

	Core_0RegisterCVarInt("r_final_image", DS_RT_SCENELIGHT, "Тип финального (выводимого в окно рендера) изображения из перечисления DS_RT");

	Core_0RegisterConcmdCls("on_r_win_width_change", this, (SXCONCMDCLS)&CEngine::onRWinWidthChanged);
	Core_0RegisterConcmdCls("on_r_win_height_change", this, (SXCONCMDCLS)&CEngine::onRWinHeightChanged);
	Core_0RegisterConcmdCls("on_r_win_windowed_change", this, (SXCONCMDCLS)&CEngine::onRWinWindowedChanged);
	Core_0RegisterConcmdCls("on_r_win_borderless_change", this, (SXCONCMDCLS)&CEngine::onRWinBorderlessChanged);
	Core_0RegisterConcmdCls("on_r_vsync_change", this, (SXCONCMDCLS)&CEngine::onRVSyncChanged);

	static const int *r_win_width = GET_PCVAR_INT("r_win_width");
	static const int *r_win_height = GET_PCVAR_INT("r_win_height");
	static const bool *r_win_windowed = GET_PCVAR_BOOL("r_win_windowed");

	//SGCore_0Create("sxgcore", (HWND)hWindow, *r_win_width, *r_win_height, *r_win_windowed, false);
	
	//LibReport(REPORT_MSG_LEVEL_NOTICE, "LIB gcore initialized\n");

#if 1
	// init physics
	SPhysics_0Create();
	LibReport(REPORT_MSG_LEVEL_NOTICE, "LIB physics initialized\n");
#endif


#if 1
	m_pRender = (IXRender*)m_pCore->getPluginManager()->getInterface(IXRENDER_GUID);

	m_pRender->initGraphics();

	m_pRender->newFinalTarget(hWindow, "xMainWindow", &m_pScreenTarget);

	m_pScreenTarget->resize(*r_win_width, *r_win_height);

	IXRenderGraph *pGraph;
	if(m_pRender->getGraph("xDefaultScene", &pGraph))
	{
		m_pScreenTarget->attachGraph(pGraph);
		mem_release(pGraph);
	}
	else
	{
		LogFatal("Couldn't load default render graph 'xDefaultScene'\n");
	}
#endif

	// init game
	SGame_0Create((HWND)hWindow, true);
	LibReport(REPORT_MSG_LEVEL_NOTICE, "LIB game initialized\n");

#if 0
	//m_pXUI = 
	HMODULE hDLL = LoadLibrary("xUI"
#ifdef _DEBUG
		"_d"
#endif
		".dll");
	if(!hDLL)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Unable to load xUI"
#ifdef _DEBUG
			"_d"
#endif
			".dll\n");
	}
	else
	{
		PFNXUIINIT pfnXGUIInit;
		pfnXGUIInit = (PFNXUIINIT)GetProcAddress(hDLL, "InitInstance");

		if(!pfnXGUIInit)
		{
			LibReport(REPORT_MSG_LEVEL_ERROR, "The procedure entry point InitInstance could not be located in the dynamic link library xUI.dll\n");
		}
		else
		{
			IXWindowSystem *pWindowSystem = (IXWindowSystem*)m_pCore->getPluginManager()->getInterface(IXWINDOWSYSTEM_GUID);
			m_pXUI = pfnXGUIInit(m_pRender, pWindowSystem, SGame_GetGUI());
			m_pCore->getPluginManager()->registerInterface(IXUI_GUID, m_pXUI);
		}
	}
#endif
	m_pXUI = (IXUI*)m_pCore->getPluginManager()->getInterface(IXUI_GUID);
	// init updatable
	m_pCore->initUpdatable();

	getCore()->getConsole()->execCommand("exec ../config_sys.cfg");

	return(true);
}
bool XMETHODCALLTYPE CEngine::initServer()
{
	XPROFILE_FUNCTION();

#if 1
	// init physics
	SPhysics_0Create();
	LibReport(REPORT_MSG_LEVEL_NOTICE, "LIB physics initialized\n");
#endif

	// init game
	SGame_0Create(NULL, true);
	LibReport(REPORT_MSG_LEVEL_NOTICE, "LIB game initialized\n");

	// init updatable
	Core_GetIXCore()->initUpdatable();

	getCore()->getConsole()->execCommand("exec ../config_sys.cfg");

	return(true);
}

INT_PTR XMETHODCALLTYPE CEngine::onMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	IMSG imsg;
	imsg.lParam = lParam;
	imsg.wParam = wParam;
	imsg.message = msg;

	SSInput_AddMsg(imsg);

	if(!SGame_AddWMsg(msg, wParam, lParam))
	{
		return(TRUE);
	}

	return(FALSE);
}
IXCore* XMETHODCALLTYPE CEngine::getCore()
{
	return(m_pCore);
}

int XMETHODCALLTYPE CEngine::start()
{
	Core_0ExecCommandLine();

	CTaskManager::TaskPtr pTask = new CMainLoopTask(this);
	XCoreAddTask(pTask);
	XCoreStart();
	return(0);
}

bool CEngine::runFrame()
{
	XPROFILE_FUNCTION();
	Core_0ConsoleUpdate();

	if(m_pCallback && !m_pCallback->processWindowMessages())
	{
		return(false);
	}
	
	Core_TimesUpdate();

	SSInput_Update();
	
	// draw frame
	{
		static IGXDevice *pRenderDevice = m_pRender->getDevice();
		if(pRenderDevice && (/*!pRenderContext->canBeginFrame() || */!checkResize()))
		{
			goto end;
		}

		//#############################################################################

		SGame_Update();

		SPhysics_Update();

		//#############################################################################
		
		SGame_Sync();

		m_pCore->runUpdate();
		
		//#############################################################################


		if(pRenderDevice)
		{
			auto *pCtx = pRenderDevice->getThreadContext();

			IXCamera *pCamera = m_pCallback->getCameraForFrame();
			m_pScreenTarget->setCamera(pCamera);

			XEventObserverChanged ev;
			ev.pCamera = pCamera;
			m_pObserverChangedEventChannel->broadcastEvent(&ev);

			m_pRender->updateVisibility();

			m_pRender->endFrame();
			//m_pDevice->swapBuffers();
			SAFE_CALL(m_pXUI, present);

			showProfile();

			std::chrono::high_resolution_clock::time_point tNow = std::chrono::high_resolution_clock::now();
			m_fDeltaTime = (float)std::chrono::duration_cast<std::chrono::microseconds>(tNow - m_fPrevTime).count() / 1000000.0f;
			m_fPrevTime = tNow;

			pCtx->beginFrame();
			pCtx->addTimestamp("begin");
			m_pRender->renderFrame(m_fDeltaTime);

			SAFE_CALL(m_pXUI, render);
			pCtx->addTimestamp("ui");

			pCtx->addTimestamp("end");
			pCtx->endFrame();
		}

		//#############################################################################
		
	}
	goto finish;

end:
	Sleep(10);
finish:
	return(true);
}

void CEngine::showProfile()
{
	static IGXDevice *pRenderDevice = m_pRender->getDevice();
	static int *dev_gpu_profile_framerate = (int*)GET_PCVAR_INT("dev_gpu_profile_framerate");

	if(*dev_gpu_profile_framerate)
	{
		pRenderDevice->enableProfiling(true);

		const GXTimeStamp *pTimeStamps;
		UINT uTimeStampCount;
		static UINT uFrames = 0;
		if(pRenderDevice->getProfilingResults(&pTimeStamps, &uTimeStampCount))
		{
			if(++uFrames >= (UINT)*dev_gpu_profile_framerate && uTimeStampCount >= 2)
			{
				uFrames = 0;

				uint64_t uTotalTicks = pTimeStamps[uTimeStampCount - 1].uTicks - pTimeStamps[0].uTicks;
				float fTotalTime = (float)uTotalTicks / (float)(pTimeStamps[uTimeStampCount - 1].uDenominator / 1000);

				uint64_t uPrev = 0;
				float fTime = 0.0f;
				float fDeltaTime;
				uint64_t uDeltaTicks;
				const char *pMarker;
				for(UINT i = 0; i < uTimeStampCount; ++i)
				{
					uDeltaTicks = pTimeStamps[i].uTicks - uPrev;
					pMarker = (const char*)pTimeStamps[i].pMarker;
					fDeltaTime = (float)uDeltaTicks / (float)(pTimeStamps[i].uDenominator / 1000);

					bool isInner = pMarker[strlen(pMarker) - 1] == '-';
					if(isInner)
					{
						fTime += fDeltaTime;
					}
					else if(fTime > 0.0f)
					{
						fDeltaTime += fTime;
						fTime = 0.0f;
					}

					LogInfo("%20s = %f ms %5.2f%%  --  %llu ticks\n", pMarker, fDeltaTime, fDeltaTime / fTotalTime * 100, uDeltaTicks);
					uPrev = pTimeStamps[i].uTicks;
				}
				LogInfo("%20s = %f ms %5.2f%%  --  %llu ticks\n", "total", fTotalTime, 100.0f, uTotalTicks);
				LogInfo(" \n");
			}
		}
	}
	else
	{
		pRenderDevice->enableProfiling(false);
	}
}

void CEngine::initPaths()
{
	char szPath[MAX_PATH];
	/*GetModuleFileName(NULL, szPath, MAX_PATH);

	String sPathExe = FileCanonizePathS(FileGetPrevDir(szPath).c_str());
	Core_RStringSet(G_RI_STRING_PATH_EXE, sPathExe.c_str());

	String sPrevPathExe = FileGetPrevDir(sPathExe.c_str());
	*/
	String sPrevPathExe = "../";

	sprintf(szPath, "%s%s", sPrevPathExe.c_str(), "/worktex/");
	Core_RStringSet(G_RI_STRING_PATH_WORKTEX, szPath);
	//FileCreateDir(szPath);

	sprintf(szPath, "%s/%s/", sPrevPathExe.c_str(), "gamesource");
	Core_RStringSet(G_RI_STRING_PATH_GAMESOURCE, szPath);
	SetCurrentDirectoryA(szPath);

	sprintf(szPath, "%s/%s/", sPrevPathExe.c_str(), "editors_cache");
	Core_RStringSet(G_RI_STRING_PATH_EDITOR_CACHE, szPath);

	sprintf(szPath, "%s%s", sPrevPathExe.c_str(), "/screenshots/");
	Core_RStringSet(G_RI_STRING_PATH_SCREENSHOTS, szPath);
	// FileCreateDir(szPath);

	sprintf(szPath, "%s%s/", Core_RStringGet(G_RI_STRING_PATH_GAMESOURCE), "config");
	Core_RStringSet(G_RI_STRING_PATH_GS_CONFIGS, szPath);
	// FileCreateDir(szPath);

	sprintf(szPath, "%s%s/", Core_RStringGet(G_RI_STRING_PATH_GAMESOURCE), "levels");
	Core_RStringSet(G_RI_STRING_PATH_GS_LEVELS, szPath);
	// FileCreateDir(szPath);

	sprintf(szPath, "%s%s/", Core_RStringGet(G_RI_STRING_PATH_GAMESOURCE), "meshes");
	Core_RStringSet(G_RI_STRING_PATH_GS_MESHES, szPath);
	// FileCreateDir(szPath);

	sprintf(szPath, "%s%s/", Core_RStringGet(G_RI_STRING_PATH_GAMESOURCE), "models");
	Core_RStringSet(G_RI_STRING_PATH_GS_MODELS, szPath);
	// FileCreateDir(szPath);

	sprintf(szPath, "%s%s/", Core_RStringGet(G_RI_STRING_PATH_GAMESOURCE), "shaders");
	Core_RStringSet(G_RI_STRING_PATH_GS_SHADERS, szPath);
	// FileCreateDir(szPath);

	sprintf(szPath, "%s%s/", Core_RStringGet(G_RI_STRING_PATH_GAMESOURCE), "sounds");
	Core_RStringSet(G_RI_STRING_PATH_GS_SOUNDS, szPath);
	// FileCreateDir(szPath);

	sprintf(szPath, "%s%s/", Core_RStringGet(G_RI_STRING_PATH_GAMESOURCE), "scripts");
	Core_RStringSet(G_RI_STRING_PATH_GS_SCRIPTS, szPath);
	// FileCreateDir(szPath);

	sprintf(szPath, "%s%s/", Core_RStringGet(G_RI_STRING_PATH_GAMESOURCE), "textures");
	Core_RStringSet(G_RI_STRING_PATH_GS_TEXTURES, szPath);
	// FileCreateDir(szPath);

	sprintf(szPath, "%s%s/", Core_RStringGet(G_RI_STRING_PATH_GAMESOURCE), "materials");
	Core_RStringSet(G_RI_STRING_PATH_GS_MTRLS, szPath);
	// FileCreateDir(szPath);

	sprintf(szPath, "%s%s/", Core_RStringGet(G_RI_STRING_PATH_GAMESOURCE), "gui");
	Core_RStringSet(G_RI_STRING_PATH_GS_GUI, szPath);
	// FileCreateDir(szPath);
}

bool CEngine::checkResize()
{
	if(m_wantResize == WR_NONE)
	{
		return(true);
	}

	static const bool *r_win_windowed = GET_PCVAR_BOOL("r_win_windowed");
	static const bool *r_win_borderless = GET_PCVAR_BOOL("r_win_borderless");
	static int *r_win_width = const_cast<int*>(GET_PCVAR_INT("r_win_width"));
	static int *r_win_height = const_cast<int*>(GET_PCVAR_INT("r_win_height"));
	static const bool *r_vsync = GET_PCVAR_BOOL("r_vsync");

	if(m_wantResize & WR_VSYNC)
	{
		m_pRender->getDevice()->enableVSync(*r_vsync);
	}

	UINT uModeCount = 0;
	const GXModeDesc *pModes = m_pRender->getModes(&uModeCount, m_pScreenTarget);

	// only one dimension has been changed, select the second from available modes
	if((m_wantResize & (WR_WIDTH | WR_HEIGHT)) != (WR_WIDTH | WR_HEIGHT))
	{
		if(m_wantResize & WR_WIDTH)
		{
			int iNewHeight = 0;
			for(UINT i = 0; i < uModeCount; ++i)
			{
				if(pModes[i].uWidth == *r_win_width)
				{
					if(pModes[i].uHeight == *r_win_height)
					{
						iNewHeight = pModes[i].uHeight;
						break;
					}
					if(iNewHeight == 0)
					{
						iNewHeight = pModes[i].uHeight;
					}
				}
			}
			if(iNewHeight && iNewHeight != *r_win_height)
			{
				*r_win_height = iNewHeight;
			}
		}
		else
		{
			int iNewWidth = 0;
			for(UINT i = 0; i < uModeCount; ++i)
			{
				if(pModes[i].uHeight == *r_win_height)
				{
					if(pModes[i].uWidth == *r_win_width)
					{
						iNewWidth = pModes[i].uWidth;
						break;
					}
					if(iNewWidth == 0)
					{
						iNewWidth = pModes[i].uWidth;
					}
				}
			}
			if(iNewWidth && iNewWidth != *r_win_width)
			{
				*r_win_width = iNewWidth;
			}
		}
	}
	// fullscreen -- test available modes
	if(!*r_win_windowed) 
	{
		bool isValid = false;
		for(UINT i = 0; i < uModeCount; ++i)
		{
			if(pModes[i].uWidth == *r_win_width && pModes[i].uHeight == *r_win_height)
			{
				isValid = true;
				break;
			}
		}

		if(!isValid)
		{
			int iNewWidthL = 0, iNewHeightL = 0, iDeltaL = 0;
			int iNewWidthG = 0, iNewHeightG = 0, iDeltaG = 0;

			for(UINT i = 0; i < uModeCount; ++i)
			{
				int iDelta = pModes[i].uWidth - *r_win_width + pModes[i].uHeight - *r_win_height;

				if((int)pModes[i].uWidth >= *r_win_width && (int)pModes[i].uHeight >= *r_win_height)
				{
					if(iDelta < iDeltaG)
					{
						iDeltaG = iDelta;
						iNewWidthG = pModes[i].uWidth;
						iNewHeightG = pModes[i].uHeight;
					}
				}
				else if((int)pModes[i].uWidth <= *r_win_width && (int)pModes[i].uHeight <= *r_win_height)
				{
					if(-iDelta < iDeltaL)
					{
						iDeltaL = -iDelta;
						iNewWidthL = pModes[i].uWidth;
						iNewHeightL = pModes[i].uHeight;
					}
				}
			}

			if(iDeltaL && iDeltaG)
			{
				if(iDeltaL > iDeltaG)
				{
					iDeltaL = 0;
				}
				else
				{
					iDeltaG = 0;
				}
			}

			if(iDeltaG)
			{
				*r_win_width = iNewWidthG;
				*r_win_height = iNewHeightG;
			}
			else if(iDeltaL)
			{
				*r_win_width = iNewWidthL;
				*r_win_height = iNewHeightL;
			}
			else
			{
				*r_win_width = 800;
				*r_win_height = 600;
			}
		}
	}

	if(*r_win_width <= 0)
	{
		*r_win_width = 1;
	}
	if(*r_win_height <= 0)
	{
		*r_win_height = 1;
	}

	LibReport(REPORT_MSG_LEVEL_WARNING, "r_win_width %d, r_win_height %d, r_win_windowed %d\n", *r_win_width, *r_win_height, (int)*r_win_windowed);
	if(m_pCallback)
	{
		m_pCallback->onGraphicsResize(*r_win_width, *r_win_height, !*r_win_windowed, *r_win_borderless, this);
	}
	m_pScreenTarget->resize(*r_win_width, *r_win_height/*, *r_win_windowed*/);
	m_wantResize = WR_NONE;

	return(false);
}

void CEngine::onRWinWidthChanged()
{
	m_wantResize |= WR_WIDTH;
}
void CEngine::onRWinHeightChanged()
{
	m_wantResize |= WR_HEIGHT;
}
void CEngine::onRWinWindowedChanged()
{
	m_wantResize |= WR_WINDOWED;
}
void CEngine::onRWinBorderlessChanged()
{
	m_wantResize |= WR_BORDERLESS;
}
void CEngine::onRVSyncChanged()
{
	m_wantResize |= WR_VSYNC;
}

//##########################################################################

EXTERN_C XAPI IXEngine* XEngineInit(int argc, char **argv, const char *szName)
{
	return(new CEngine(argc, argv, szName));
}
