
/***********************************************************
Copyright � Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "sxdecals.h"

#include <core/sxcore.h>
#include "DecalManager.h"

#if defined(_DEBUG)
#	pragma comment(lib, "sxcore_d.lib")
#	pragma comment(lib, "sxgcore_d.lib")
#	pragma comment(lib, "sxphysics_d.lib")
#else
#	pragma comment(lib, "sxcore.lib")
#	pragma comment(lib, "sxgcore.lib")
#	pragma comment(lib, "sxphysics.lib")
#endif

#if !defined(DEF_STD_REPORT)
#define DEF_STD_REPORT
report_func g_fnReportf = DefReport;
#endif

DecalManager * g_pMgr = NULL;

#define SP_PRECOND(ret) if(!g_pMgr){LibReport(REPORT_MSG_LEVEL_ERROR, "%s - sxdecals is not init", GEN_MSG_LOCATION);return ret;}

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

SX_LIB_API void SXDecals_0Create()
{
	if(g_pMgr)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "%s - sxdecals double init", GEN_MSG_LOCATION);
		return;
	}
	Core_SetOutPtr();

	g_pMgr = new DecalManager();
}
SX_LIB_API void SXDecals_AKill()
{
	SP_PRECOND(_VOID);
	mem_delete(g_pMgr);
}

SX_LIB_API void SXDecals_Update()
{
	SP_PRECOND(_VOID);
	g_pMgr->update();
}

SX_LIB_API void SXDecals_Sync()
{
	SP_PRECOND(_VOID);
	g_pMgr->sync();
}

SX_LIB_API void SXDecals_Dbg_Set(report_func rf)
{
	g_fnReportf = rf;
}

SX_LIB_API void SXDecals_Render()
{
	SP_PRECOND(_VOID);
	g_pMgr->render();
}

SX_LIB_API void SXDecals_ShootDecal(DECAL_TYPE type, const float3 & fWorldPos, const float3 & normal)
{
	SP_PRECOND(_VOID);
	if(type == DECAL_TYPE_CUSTOM)
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "Unable to use DECAL_TYPE_CUSTOM with SXDecals_ShootDecal\n");
		return;
	}
	g_pMgr->shootDecal(type, fWorldPos, 0, 0, 0, 1.0f, 0, &normal);
}

SX_LIB_API void SXDecals_ShootDecalEx(DECAL_TYPE type, const float3 & fWorldPos, const float3 & normal, int flags, ID material, float fScale, const float3 * saxis)
{
	SP_PRECOND(_VOID);
	g_pMgr->shootDecal(type, fWorldPos, material, 0, saxis, fScale, flags, &normal);
}

SX_LIB_API void SXDecals_Clear()
{
	SP_PRECOND(_VOID);
	g_pMgr->clear();
}
