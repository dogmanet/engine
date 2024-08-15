
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "sxphysics.h"
#include "PhyWorld.h"
#include "Physics.h"

#include <core/sxcore.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>

#if defined(_DEBUG)
#	pragma comment(lib, "sxcore_d.lib")
#	pragma comment(lib, "BulletDynamics_vs2010_debug.lib")
#	pragma comment(lib, "BulletCollision_vs2010_debug.lib")
#	pragma comment(lib, "LinearMath_vs2010_debug.lib")
#	pragma comment(lib, "BulletWorldImporter_vs2010_debug.lib")
#	pragma comment(lib, "BulletWorldImporter_vs2010_debug.lib")
#	pragma comment(lib, "BulletFileLoader_vs2010_debug.lib")
#else
#	pragma comment(lib, "sxcore.lib")
#	pragma comment(lib, "BulletDynamics_vs2010.lib")
#	pragma comment(lib, "BulletCollision_vs2010.lib")
#	pragma comment(lib, "LinearMath_vs2010.lib")
#	pragma comment(lib, "BulletWorldImporter_vs2010.lib")
#	pragma comment(lib, "BulletFileLoader_vs2010.lib")
#endif

#if !defined(DEF_STD_REPORT)
#define DEF_STD_REPORT
report_func g_fnReportf = DefReport;
#endif

DECLARE_PROFILER_INTERNAL();

CPhyWorld *g_pWorld = NULL;
CPhysics *g_pPhysics = NULL;

#define SP_PRECOND(ret) if(!g_pWorld){LibReport(REPORT_MSG_LEVEL_ERROR, "%s - sxphysics is not init", GEN_MSG_LOCATION);return ret;}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

SX_LIB_API void SPhysics_DumpStats()
{
	CProfileManager::dumpAll();
}

SX_LIB_API void SPhysics_0Create()
{
	INIT_PROFILER_INTERNAL();
	if(g_pWorld)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "%s - sxphysics double init", GEN_MSG_LOCATION);
		return;
	}
	Core_SetOutPtr();

	g_pWorld = new CPhyWorld();
	g_pPhysics = new CPhysics(g_pWorld);

	Core_GetIXCore()->getPluginManager()->registerInterface(IXPHYSICS_GUID, g_pPhysics);

	Core_0RegisterConcmd("perf_physics", SPhysics_DumpStats);
}
SX_LIB_API void SPhysics_AKill()
{
	SP_PRECOND(_VOID);
	mem_delete(g_pPhysics);
	mem_delete(g_pWorld);
}

SX_LIB_API void SPhysics_Update()
{
	SP_PRECOND(_VOID);
	g_pWorld->update();
}

SX_LIB_API btDiscreteDynamicsWorldMt* SPhysics_GetDynWorld()
{
	SP_PRECOND(NULL);
	return(g_pWorld->getBtWorld());
}
