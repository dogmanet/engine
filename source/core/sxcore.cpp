
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#define CORE_VERSION 1

#include "sxcore.h"

#include "Config.h"

#include "File.h"

#include "Task.h"
#include "TaskManager.h"

#include "concmd.h"
#include "cvars.h"

#include "Time.h"

#include <shellapi.h>

#include <common/file_utils.h>

#include "GRegisterIndex.h"

#include "Core.h"

//##########################################################################

#if !defined(DEF_STD_REPORT)
#define DEF_STD_REPORT
report_func g_fnReportf = DefReport;
#endif

//**************************************************************************

CTaskManager *g_pTaskManager = 0;

#define SXCORE_PRECOND(retval) if(!g_pTaskManager){LibReport(REPORT_MSG_LEVEL_ERROR, "%s - sxcore is not init", GEN_MSG_LOCATION); return retval;}

//**************************************************************************

//РЕГИСТРЫ
bool g_aGRegistersBool[CORE_REGISTRY_SIZE];
int32_t g_aGRegistersInt[CORE_REGISTRY_SIZE];
float32_t g_aGRegistersFloat[CORE_REGISTRY_SIZE];
float3 g_aGRegistersFloat3[CORE_REGISTRY_SIZE];
String g_aGRegistersString[CORE_REGISTRY_SIZE];

#define CORE_REGUSTRY_PRE_COND_ID(id,stdval) \
if (!(id >= 0 && id < CORE_REGISTRY_SIZE))\
{LibReport(REPORT_MSG_LEVEL_ERROR, "%s: %s - unresolved index '%d' of access for registry", SX_LIB_NAME, GEN_MSG_LOCATION, id); return stdval; }

//**************************************************************************

CTimeManager *g_pTimers = NULL;
CCore *g_pCore = NULL;
#define CORE_TIME_PRECOND(retval) if(!g_pTimers){LibReport(REPORT_MSG_LEVEL_ERROR, "%s - sxcore is not init", GEN_MSG_LOCATION); return retval;}

//##########################################################################

//функции обертки
long Core_0GetVersion()
{
	return CORE_VERSION;
}

void Core_Dbg_Set(report_func fnReportFunc)
{
	g_fnReportf = fnReportFunc;
}

/*bool Core_0FileExists(const char *szNameFunc)
{
	WIN32_FIND_DATA wfd;
	HANDLE hFind = ::FindFirstFile(szNameFunc, &wfd);
		if (INVALID_HANDLE_VALUE != hFind)
		{
			::FindClose(hFind);
			return true;
		}
	return false;
}*/

/*SX_LIB_API UINT Core_0GetTimeLastModify(const char *szPath)
{
	HANDLE hFile = CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	
	if (hFile == INVALID_HANDLE_VALUE)
		return 0;

	SYSTEMTIME stUTC;
	FILETIME ftCreate, ftAccess, ftWrite;
	GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite);
	FileTimeToSystemTime(&ftWrite, &stUTC);

	tm tmObj;
	ZeroMemory(&tmObj, sizeof(tm));
	tmObj.tm_year = stUTC.wYear - 1900;
	tmObj.tm_mon = stUTC.wMonth;
	tmObj.tm_mday = stUTC.wDay;
	tmObj.tm_hour = stUTC.wHour;
	tmObj.tm_min = stUTC.wMinute;
	tmObj.tm_sec = stUTC.wSecond;

	uint32_t tLastModify = mktime(&tmObj);

	return tLastModify;
}*/

bool Core_0ClipBoardCopy(const char *szStr)
{
    HGLOBAL hglb;
    char *s;
	size_t len = strlen(szStr) + 1;
 
	if(!(hglb = GlobalAlloc(GHND, len)))
		return false;

	if(!(s = (char *)GlobalLock(hglb)))
		return false;

	strcpy(s, szStr);
	GlobalUnlock(hglb);

	if(!OpenClipboard(NULL) || !EmptyClipboard()) 
	{
		GlobalFree(hglb);
		return false;
	}
	SetClipboardData(CF_TEXT, hglb);
	CloseClipboard();
	return true;
}

bool Core_0IsProcessRun(const char* process)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if(hSnapshot == INVALID_HANDLE_VALUE)
	{
		return(false);
	}

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);
	Process32First(hSnapshot, &pe);

	bool isFound = false;
	do
	{
		if(strcasecmp(pe.szExeFile, process) == 0)
		{
			isFound = true;
			break;
		}
	}
	while(Process32Next(hSnapshot, &pe));

	CloseHandle(hSnapshot);

	return(isFound);
}


SX_LIB_API XDEPRECATED IXCore *Core_GetIXCore()
{
	return(g_pCore);
}

//##########################################################################

SX_LIB_API XDEPRECATED IFile* Core_CrFile()
{
	return new CFile();
}

SX_LIB_API XDEPRECATED IFile* Core_OpFile(const char *szPath, int iType)
{
	CFile* pFile = new CFile();
	pFile->open(szPath, iType);
	return pFile;
}


//##########################################################################
/*
Array<String> g_aResourcePathes;

SX_LIB_API void Core_ResPathAdd(const char *szPath)
{
	if (szPath)
		g_aResourcePathes.push_back(String(Core_RStringGet(G_RI_STRING_PATH_EXE)) + szPath + "/");
}

SX_LIB_API void Core_ResPathClear()
{
	g_aResourcePathes.clear();
}

SX_LIB_API const char* Core_ResPathGetLast(int iRegisterPath)
{
	if (g_aResourcePathes.size() > 0)
	{
		if (iRegisterPath >= 0)
			return (g_aResourcePathes[g_aResourcePathes.size() - 1] + Core_RStringGet(iRegisterPath)).c_str();
		else
			return g_aResourcePathes[g_aResourcePathes.size() - 1].c_str();
	}

	return 0;
}

SX_LIB_API const char* Core_ResPathGetFullPathByRel(const char *szRelPath)
{
	for (int i = 0, il = g_aResourcePathes.size(); i < il; ++i)
	{
		String sResPath = g_aResourcePathes[(il - 1) - i];
		String sFullPath = sResPath + szRelPath;

		if (FileExistsFile(sFullPath.c_str()))
			return sFullPath.c_str();
	}

	return 0;
}

SX_LIB_API const char* Core_ResPathGetFullPathByRel2(const char *szRelPathPart1, const char *szRelPathPart2)
{
	return Core_ResPathGetFullPathByRel((String(szRelPathPart1) + szRelPathPart2).c_str());
}

SX_LIB_API const char* Core_ResPathGetFullPathByRelIndex(int iRegisterPath, const char *szRelPath)
{
	return Core_ResPathGetFullPathByRel((String(Core_RStringGet(G_RI_STRING_PATH_GS_CONFIGS)) + szRelPath).c_str());
}

SX_LIB_API const char* Core_ResPathGetFullPathByRelIndex2(int iRegisterPath, const char *szRelPathPart1, const char *szRelPathPart2)
{
	return Core_ResPathGetFullPathByRelIndex(G_RI_STRING_PATH_GS_CONFIGS, (String(szRelPathPart1) + szRelPathPart2).c_str());
}
*/
//##########################################################################


SX_LIB_API XDEPRECATED ISXConfig* Core_CrConfig()
{
	return new CConfig(g_pCore->getFileSystem());
}

SX_LIB_API XDEPRECATED ISXConfig* Core_OpConfig(const char* path)
{
	CConfig* pConfig = new CConfig(g_pCore->getFileSystem());
	pConfig->open(path);
	return pConfig;
}

//##########################################################################

SX_LIB_API void Core_MTaskAdd(THREAD_UPDATE_FUNCTION func, DWORD flag)
{
	SXCORE_PRECOND(_VOID);
	assert(!(flag & CORE_TASK_FLAG_FOR_LOOP));
	g_pTaskManager->add(func, flag);
}

SX_LIB_API void Core_MForceSinglethreaded()
{
	SXCORE_PRECOND(_VOID);
	g_pTaskManager->forceSinglethreaded();
}

SX_LIB_API void Core_MTaskStart()
{
	SXCORE_PRECOND(_VOID);
	g_pTaskManager->start();
}

SX_LIB_API void Core_MTaskStop()
{
	SXCORE_PRECOND(_VOID);
	g_pTaskManager->stop();
}

SX_LIB_API ID Core_MForLoop(int iStart, int iEnd, const IParallelForBody *pBody, int iMaxChunkSize)
{
	SXCORE_PRECOND(-1);
	return(g_pTaskManager->forLoop(iStart, iEnd, pBody, iMaxChunkSize));
}

SX_LIB_API void Core_MWaitFor(ID id)
{
	SXCORE_PRECOND(_VOID);
	if(Core_MGetThreadID() != 0)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Core_MWaitFor() must be called from main thread!");
		return;
	}

	g_pTaskManager->waitFor(id);
}

SX_LIB_API int Core_MGetThreadCount()
{
	SXCORE_PRECOND(1);
	int iTC = g_pTaskManager->getThreadCount();
	return(max(iTC, 1));
}

//##########################################################################

void Core_RBoolSet(int id, bool val)
{
	CORE_REGUSTRY_PRE_COND_ID(id, _VOID);
	g_aGRegistersBool[id] = val;
}

bool Core_RBoolGet(int id)
{
	CORE_REGUSTRY_PRE_COND_ID(id, 0);
	return g_aGRegistersBool[id];
}

void Core_RIntSet(int id, int32_t val)
{
	CORE_REGUSTRY_PRE_COND_ID(id, _VOID);
	g_aGRegistersInt[id] = val;
}

int32_t Core_RIntGet(int id)
{
	CORE_REGUSTRY_PRE_COND_ID(id,0);
	return g_aGRegistersInt[id];
}

void Core_RFloatSet(int id, float32_t val)
{
	CORE_REGUSTRY_PRE_COND_ID(id, _VOID);
	g_aGRegistersFloat[id] = val;
}

float32_t Core_RFloatGet(int id)
{
	CORE_REGUSTRY_PRE_COND_ID(id,0);
	return g_aGRegistersFloat[id];
}


void Core_RFloat3Set(int id, float3* val)
{
	CORE_REGUSTRY_PRE_COND_ID(id, _VOID);
	if (val)
		g_aGRegistersFloat3[id] = *val;
}

void Core_RFloat3Get(int id, float3* val)
{
	CORE_REGUSTRY_PRE_COND_ID(id, _VOID);

	if (val)
		memcpy(val, &g_aGRegistersFloat3[id], sizeof(float3));
}

void Core_RStringSet(int id, const char *val)
{
	CORE_REGUSTRY_PRE_COND_ID(id, _VOID);
	if (val)
		g_aGRegistersString[id] = val;
}

const char* Core_RStringGet(int id)
{
	CORE_REGUSTRY_PRE_COND_ID(id, 0);
	return g_aGRegistersString[id].c_str();
}

//##########################################################################

ID Core_TimeAdd()
{
	CORE_TIME_PRECOND(-1);

	return g_pTimers->timeAdd();
}

void Core_TimesUpdate()
{
	CORE_TIME_PRECOND(_VOID);

	g_pTimers->update();
}


void Core_TimeSpeedSet(ID id, float speed)
{
	CORE_TIME_PRECOND(_VOID);

	g_pTimers->timeSpeedSet(id, speed);
}

float Core_TimeSpeedGet(ID id)
{
	CORE_TIME_PRECOND(0);

	return g_pTimers->timeSpeedGet(id);
}


void Core_TimeWorkingSet(ID id, bool working)
{
	CORE_TIME_PRECOND(_VOID);

	g_pTimers->timeWorkingSet(id, working);
}

bool Core_TimeWorkingGet(ID id)
{
	CORE_TIME_PRECOND(false);

	return g_pTimers->timeWorkingGet(id);
}


void Core_TimeUnixStartSet(ID id, int64_t start_time)
{
	CORE_TIME_PRECOND(_VOID);

	g_pTimers->timeUnixStartSet(id, start_time);
}

int64_t Core_TimeUnixStartGet(ID id)
{
	CORE_TIME_PRECOND(0);

	return g_pTimers->timeUnixStartGet(id);
}

int64_t Core_TimeUnixCurrGet(ID id)
{
	CORE_TIME_PRECOND(0);

	return g_pTimers->timeUnixCurrGet(id);
}


int64_t Core_TimeTotalMcsGet(ID id)
{
	CORE_TIME_PRECOND(0);

	return g_pTimers->timeTotalMcsGet(id);
}

int64_t Core_TimeTotalMcsGetU(ID id)
{
	CORE_TIME_PRECOND(0);

	return g_pTimers->timeTotalMcsGetU(id);
}


//##########################################################################

static AssotiativeArray<String, String> g_mCommandLine;
static Array<String> g_aConsoleLine;

void Core_0LoadCommandLine(int argc, char **argv)
{
	const char * key = NULL;
	bool isCvar = true;
	for(int i = 0; i < argc; ++i)
	{
		if(argv[i][0] == L'-') ///< startup param
		{
			if(key)
			{
				if(isCvar)
				{
					g_aConsoleLine.push_back(key);
				}
				else
				{
					g_mCommandLine[key] = "";
				}
			}
			key = &argv[i][1];
			isCvar = false;
		}
		else if(argv[i][0] == L'+') ///< cvar param (or cmd)
		{
			if(key)
			{
				if(isCvar)
				{
					g_aConsoleLine.push_back(key);
				}
				else
				{
					g_mCommandLine[key] = "";
				}
			}
			key = &argv[i][1];
			isCvar = true;
		}
		else if(key != NULL) ///< arg
		{
			if(isCvar)
			{
				g_aConsoleLine.push_back(String(key) + " " + argv[i]);
			}
			else
			{
				g_mCommandLine[key] = argv[i];
			}
			//store val
			key = NULL;
		}
	}
	if(key != NULL) ///< arg
	{
		if(isCvar)
		{
			g_aConsoleLine.push_back(key);
		}
		else
		{
			g_mCommandLine[key] = "";
		}
	}
}

void Core_0ExecCommandLine()
{
	for(int i = 0, l = g_aConsoleLine.size(); i < l; ++i)
	{
		Core_0ConsoleExecCmd("%s\n", g_aConsoleLine[i].c_str());
	}
}

const char *Core_0GetCommandLineArg(const char *szArg, const char *szDefault)
{
	const AssotiativeArray<String, String>::Node *pNode;
	if(g_mCommandLine.KeyExists(szArg, &pNode))
	{
		return(pNode->Val->c_str());
	}
	return(szDefault);
}
