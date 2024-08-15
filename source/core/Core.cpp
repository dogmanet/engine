#include "Core.h"
#include <common/file_utils.h>
#include "AsyncFileReader.h"
#include "AsyncTaskRunner.h"
#include "FileSystem.h"
#include "ModelProvider.h"

#include "concmd.h"
#include "TaskManager.h"

#include "time.h"

#include <GRegisterIndex.h>

#include <xcommon/IXTextureLoader.h>
#include <xcommon/IXTextureProxy.h>
#include "ResourceTexture.h"

#include "Config.h"
#include "Buffer.h"

extern CTimeManager *g_pTimers;
CProfiler *g_pProfilerInternal;
DECLARE_PROFILER_INTERNAL();
extern CCore *g_pCore;
extern CTaskManager *g_pTaskManager;

CCore::CCore(const char *szName)
{
	ConsoleConnect(szName);
	ConsoleRegisterCmds();
	CvarInitSystem(this);
	
	m_pConsole = new CConsole();

	Core_0RegisterCVarBool("g_time_run", true, "Запущено ли игровое время?", FCVAR_NOTIFY_OLD);
	Core_0RegisterCVarFloat("g_time_speed", 1.f, "Скорость/соотношение течения игрового времени", FCVAR_NOTIFY_OLD);
	Core_0RegisterCVarBool("dbg_config_save", false, "Отладочный вывод процесса сохранения конфига");
	Core_0RegisterCVarInt("r_stats", 0, "Показывать ли статистику? 0 - нет, 1 - fps и игровое время, 2 - показать полностью");

	Core_0RegisterConcmd("on_g_time_run_change", []()
	{
		static const bool * g_time_run = GET_PCVAR_BOOL("g_time_run");
		Core_TimeWorkingSet(Core_RIntGet(G_RI_INT_TIMER_GAME), *g_time_run);
	});
	Core_0RegisterConcmd("on_g_time_speed_change", []()
	{
		static const float * g_time_speed = GET_PCVAR_FLOAT("g_time_speed");
		Core_TimeSpeedSet(Core_RIntGet(G_RI_INT_TIMER_GAME), *g_time_speed);
	});

	m_pPluginManager = new CPluginManager();

	m_pFileSystem = new CFileSystem();
	m_pFileSystem->addRoot("engine");

	IXConfig *pConfig = newConfig();
	if(pConfig->open("../roots.ini"))
	{
		char sect[32];
		for(UINT i = 0;; ++i)
		{
			sprintf(sect, "path_%u", i);
			UINT uKeys = pConfig->getKeyCount(sect);
			if(!uKeys)
			{
				break;
			}

			int iPriority = -1;
			const char *szPriority = pConfig->getKey(sect, "priority");
			if(szPriority)
			{
				sscanf(szPriority, "%d", &iPriority);
			}

			const char *szPath = pConfig->getKey(sect, "path");
			if(szPath)
			{
				m_pFileSystem->addRoot(szPath, iPriority);
			}
		}
	}
	mem_release(pConfig);

	//m_pFileSystem->addRoot("gamesource");
	//m_pFileSystem->addRoot("demos");
	UINT uGamePathId = m_pFileSystem->addRoot("local");
	m_pFileSystem->setWritableRoot(uGamePathId);

	m_pModelProvider = new CModelProvider(this);
	m_pPluginManager->registerInterface(IXMODELPROVIDER_GUID, m_pModelProvider);
	
	m_pJSON = new CJSON();
	m_pPluginManager->registerInterface(IXJSON_GUID, m_pJSON);

	g_pTimers = m_pTimers = new CTimeManager();

	ID idTimerRender = Core_TimeAdd();
	ID idTimerGame = Core_TimeAdd();
	Core_RIntSet(G_RI_INT_TIMER_RENDER, idTimerRender);
	Core_RIntSet(G_RI_INT_TIMER_GAME, idTimerGame);

	tm ct = {0, 0, 10, 27, 5, 2030 - 1900, 0, 0, 0};
	Core_TimeUnixStartSet(idTimerGame, mktime(&ct));

	Core_TimeWorkingSet(idTimerRender, true);
	Core_TimeWorkingSet(idTimerGame, true);

	Core_TimeSpeedSet(idTimerGame, 10);

	int iThreadNum = 0;
	if(!sscanf(Core_0GetCommandLineArg("threads", "0"), "%d", &iThreadNum) || iThreadNum < 0)
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "Invalid -threads value! Defaulting to 0\n");
	}

	g_pTaskManager = m_pTaskManager = new CTaskManager(iThreadNum);
	if(strcasecmp(Core_0GetCommandLineArg("no-threads", "no"), "no"))
	{
		m_pTaskManager->forceSinglethreaded();
	}

	loadPlugins();

	//loadProfiler();

#if 0
	IXTextureLoader *pLoader = (IXTextureLoader*)m_pPluginManager->getInterface(IXTEXTURELOADER_GUID);
	/*if(pLoader->open("textures/tp/tp_sga_window_center_up.dds", ""))
	{
		CResourceTexture2D *pTex2D = new CResourceTexture2D();
		XTextureInfo texInfo;
		pLoader->getInfo(&texInfo);
		if(!pLoader->loadAs2D(pTex2D))
		{
			printf(COLOR_LRED "Unable to load texture!" COLOR_RESET "\n");
		}
		pLoader->close();

		pTex2D->Release();
	}*/

	//if(pLoader->open("textures/fire/test.dds", ""))
	if(pLoader->open("textures/fire/fire_tiled_a.dds", ""))
	{
		CResourceTexture2D *pTex2D = new CResourceTexture2D();
		XTextureInfo texInfo;
		pLoader->getInfo(&texInfo);
		if(!pLoader->loadAs2D(pTex2D))
		{
			printf(COLOR_LRED "Unable to load texture!" COLOR_RESET "\n");
		}
		pLoader->close();

		pTex2D->Release();
	}

	if(pLoader->open("textures/sky/sky_hdr_hl2.dds", ""))
	{
		CResourceTextureCube *pTexCube = new CResourceTextureCube();
		XTextureInfo texInfo;
		pLoader->getInfo(&texInfo);
		if(!pLoader->loadAsCube(pTexCube))
		{
			printf(COLOR_LRED "Unable to load texture!" COLOR_RESET "\n");
		}
		pLoader->close();

		pTexCube->Release();
	}

#endif
#if 0
	IXTextureProxy *pProxy = (IXTextureProxy*)m_pPluginManager->getInterface(IXTEXTUREPROXY_GUID);
	if(pProxy)
	{
		char szBuf[1024];
		UINT uBufSize = sizeof(szBuf);
		if(pProxy->resolveName("sky_hdr_hl2", szBuf, &uBufSize))
		{
			printf(COLOR_LGREEN "Name is resolved to '%s'!" COLOR_RESET "\n", szBuf);
		}
		else
		{
			printf(COLOR_LRED "Name is not resolved!" COLOR_RESET "\n");
		}

		if(pProxy->resolveName("test_byte", szBuf, &uBufSize))
		{
			printf(COLOR_LGREEN "Name is resolved to '%s'!" COLOR_RESET "\n", szBuf);
		}
		else
		{
			printf(COLOR_LRED "Name is not resolved!" COLOR_RESET "\n");
		}
	}
#endif
#if 0
	while(1)
	{
		IXResourceTexture2D *pTex = NULL;
		if(getResourceManager()->getTexture2D("textures/tp/tp_sga_window_center_up1.dds#+0button", &pTex))
		{
			printf(COLOR_LGREEN "Loaded!" COLOR_RESET "\n");
			mem_release(pTex);
		}
		Sleep(10);
	}
#endif
}
CCore::~CCore()
{
	shutdownUpdatable();

	mem_delete(m_pTaskManager);

	mem_delete(m_pTimers);
	mem_delete(m_pPerfMon);

	mem_delete(m_pJSON);

	mem_delete(m_pModelProvider);
	mem_delete(m_pResourceManager);
	mem_delete(m_pFileSystem);
	//mem_delete(m_pProfiler);
	mem_delete(m_pPluginManager);
	for(AssotiativeArray<XGUID, IBaseEventChannel*>::Iterator i = m_mEventChannels.begin(); i; ++i)
	{
		mem_delete(*i.second);
	}

	mem_delete(m_pConsole);

	ConsoleDisconnect();
}

IPluginManager* XMETHODCALLTYPE CCore::getPluginManager()
{
	return(m_pPluginManager);
}
IFileSystem* XMETHODCALLTYPE CCore::getFileSystem()
{
	return(m_pFileSystem);
}
IXResourceManager* XMETHODCALLTYPE CCore::getResourceManager()
{
	return(m_pResourceManager);
}

IAsyncFileReader* XMETHODCALLTYPE CCore::newAsyncFileReader()
{
	return(new CAsyncFileReader(this));
}
IAsyncTaskRunner* XMETHODCALLTYPE CCore::newAsyncTaskRunner()
{
	return(new CAsyncTaskRunner(this));
}

IBaseEventChannel* CCore::getEventChannelInternal(const XGUID &guid)
{
	const AssotiativeArray<XGUID, IBaseEventChannel*>::Node *pNode;
	if(m_mEventChannels.KeyExists(guid, &pNode))
	{
		return(*pNode->Val);
	}
	//@HACK
	IBaseEventChannel *pOut = new IEventChannel<int>();
	m_mEventChannels[guid] = pOut;
	return(pOut);
}

void CCore::loadPlugins()
{
#ifdef WIN64
	auto list = FileGetListFiles("../bin64/plugins/*.dll");
#else
	auto list = FileGetListFiles("../bin/plugins/*.dll");
#endif
		
	IXPlugin *pPlugin = NULL;
	for(UINT i = 0, l = list.size(); i < l; ++i)
	{
		printf("Loading plugin '%s'... ", list[i].c_str());
		pPlugin = m_pPluginManager->loadPlugin((String("../bin"
#ifdef WIN64
			"64"
#endif
			"/plugins/") + list[i]).c_str());
		if(pPlugin)
		{
			printf(COLOR_GREEN "DONE!" COLOR_RESET "\n");
		}
		else
		{
			printf(COLOR_LRED "ERROR!" COLOR_RESET "\n");
		}
	}

	loadProfiler();

	m_pResourceManager = new CResourceManager(this);

	m_pPluginManager->invokeStartup(this);

	m_pResourceManager->initPlugins();
}

void CCore::initUpdatable()
{
	IXCore *pCore = Core_GetIXCore();
	IPluginManager *pPluginManager = pCore->getPluginManager();

	IXUpdatable *pUpdatable;
	UINT ic = 0;
	while((pUpdatable = (IXUpdatable*)pPluginManager->getInterface(IXUPDATABLE_GUID, ic++)))
	{
		// if(pUpdatable->getVersion() == IXRENDERABLE_VERSION)
		{
			_update_sys rs;
			rs.pUpdatable = pUpdatable;
			rs.uPriority = pUpdatable->startup();

			m_aUpdatables.push_back(rs);
		}
	}

	m_aUpdatables.quickSort([](const _update_sys &a, const _update_sys &b)
	{
		return(a.uPriority < b.uPriority);
	});

	m_tLastUpdateTime = std::chrono::high_resolution_clock::now();
}

void CCore::runUpdate()
{
	XPROFILE_FUNCTION();

	static Array<ID> s_aIdToWait;
	ID idTask;
	time_point tNow = std::chrono::high_resolution_clock::now();
	float fDeltaTime = (float)std::chrono::duration_cast<std::chrono::microseconds>(tNow - m_tLastUpdateTime).count() / 1000000.0f;
	m_tLastUpdateTime = tNow;

	for(UINT i = 0, l = m_aUpdatables.size(); i < l; ++i)
	{
		idTask = m_aUpdatables[i].pUpdatable->run(fDeltaTime);
		if(ID_VALID(idTask))
		{
			s_aIdToWait.push_back(idTask);
		}
	}

	for(UINT i = 0, l = s_aIdToWait.size(); i < l; ++i)
	{
		Core_MWaitFor(s_aIdToWait[i]);
	}

	s_aIdToWait.clearFast();

	for(UINT i = 0, l = m_aUpdatables.size(); i < l; ++i)
	{
		m_aUpdatables[i].pUpdatable->sync();
	}
}

void CCore::shutdownUpdatable()
{
	for(UINT i = 0, l = m_aUpdatables.size(); i < l; ++i)
	{
		m_aUpdatables[i].pUpdatable->shutdown();
	}
}

UINT_PTR XMETHODCALLTYPE CCore::getCrtOutputHandler()
{
	return(Core_ConsoleGetOutHandler());
}

ID XMETHODCALLTYPE CCore::getThreadId()
{
	return(Core_MGetThreadID());
}

bool XMETHODCALLTYPE CCore::isOnMainThread()
{
	return(getThreadId() == 0);
}

IXConfig* XMETHODCALLTYPE CCore::newConfig()
{
	return(new CXConfig(m_pFileSystem));
}

IXBuffer* XMETHODCALLTYPE CCore::newBuffer()
{
	return(new CBuffer());
}

ID XMETHODCALLTYPE CCore::forLoop(int iStart, int iEnd, const IParallelForBody *pBody, int iMaxChunkSize)
{
	return(g_pTaskManager->forLoop(iStart, iEnd, pBody, iMaxChunkSize));
}
void XMETHODCALLTYPE CCore::waitForLoop(ID id)
{
	g_pTaskManager->waitFor(id);
}

void XMETHODCALLTYPE CCore::addTask(ITask *pTask)
{
	g_pTaskManager->addTask(pTask);
}

IXConsole* XMETHODCALLTYPE CCore::getConsole()
{
	return(m_pConsole);
}

void CCore::loadProfiler()
{
	g_pProfiler = g_pProfilerInternal = m_pProfiler = new CProfiler(this, Core_0GetCommandLineArg("profiler"), Core_0GetCommandLineArg("start-profiling") != NULL);
	m_pPluginManager->registerInterface(IXPROFILER_GUID, m_pProfiler);
}

//##########################################################################

EXTERN_C SXCORE_API IXCore* XCoreInit(const char *szName)
{
	return(g_pCore = new CCore(szName));
}

EXTERN_C SXCORE_API void XCoreStart()
{
	g_pTaskManager->start();
}
EXTERN_C SXCORE_API void XCoreStop()
{
	g_pTaskManager->stop();
}
EXTERN_C SXCORE_API void XCoreAddTask(ITask *pTask)
{
	g_pTaskManager->addTask(pTask);
}
