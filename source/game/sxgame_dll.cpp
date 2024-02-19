
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "sxgame.h"
#include <core/sxcore.h>

#include "EntityManager.h"

#include "PathCorner.h"
#include "FuncTrain.h"

#include "GameData.h"

#if defined(_DEBUG)
#	pragma comment(lib, "sxcore_d.lib")
#	pragma comment(lib, "sxinput_d.lib")
#else
#	pragma comment(lib, "sxcore.lib")
#	pragma comment(lib, "sxinput.lib")
#endif

#if !defined(DEF_STD_REPORT)
#define DEF_STD_REPORT
report_func g_fnReportf = DefReport;
#endif


GameData * g_pGameData = NULL;
//IMesh* g_pFigureBox = 0;
DECLARE_PROFILER_INTERNAL();

HINSTANCE g_hInstance = NULL;

#define SG_PRECOND(ret) if(!g_pGameData){LibReport(REPORT_MSG_LEVEL_ERROR, "%s - sxgame is not init", GEN_MSG_LOCATION);return ret;}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch(ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		g_hInstance = hModule;
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


void ccmd_cam_pt()
{
	static int count = 0;
	char buf[64];
	sprintf(buf, "cam_pt_%d", count);
	CPathCorner * pc = (CPathCorner*)CREATE_ENTITY("path_corner", GameData::m_pMgr);
	pc->setKV("name", buf);
	pc->setPos(GameData::m_pActiveCamera->getPos());
	pc->setOrient(GameData::m_pActiveCamera->getOrient());

	sprintf(buf, "cam_pt_%d", count - 1);
	pc = (CPathCorner*)GameData::m_pMgr->findEntityByName(buf);
	sprintf(buf, "cam_pt_%d", count);
	pc->setKV("next", buf);
	
	++count;
}

void ccmd_save_as(int argc, const char ** argv)
{
	if(argc > 1)
	{
		GameData::m_pMgr->exportList(argv[1]);
	}
}

SX_LIB_API IXCamera* SGame_GetActiveCamera()
{
	SG_PRECOND(NULL);
	return(GameData::m_pActiveCamera->getCamera());
}

SX_LIB_API void SGame_0Create(HWND hWnd, bool isGame)
{
	if(g_pGameData)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "%s - sxgame double init", GEN_MSG_LOCATION);
		return;
	}
	Core_SetOutPtr();
	INIT_PROFILER_INTERNAL();

	g_pGameData = new GameData(hWnd, isGame);

	//g_pPlayer->spawn();
	//SGCore_FCreateBoundingBoxMesh(&float3(-0.5f, -0.5f, -0.5f), &float3(0.5f, 0.5f, 0.5f), &g_pFigureBox);
	//DX_CALL(D3DXCreateBox(SGCore_GetDXDevice(), 1, 1, 1, &g_pFigureBox, 0));

	Core_0RegisterConcmd("add_corner", ccmd_cam_pt);
	Core_0RegisterConcmdArg("ent_save", ccmd_save_as);
}
SX_LIB_API void SGame_AKill()
{
	SG_PRECOND(_VOID);
	mem_delete(g_pGameData);
	//mem_release(g_pFigureBox);
}

SX_LIB_API void SGame_Update(int thread)
{
	SG_PRECOND(_VOID);
	XPROFILE_FUNCTION();
	GameData::m_pMgr->update(thread);
	if(thread == 0)
	{
		g_pGameData->update();
	}
}
SX_LIB_API void SGame_UpdateSetThreadNum(int num)
{
	SG_PRECOND(_VOID);
	GameData::m_pMgr->setThreadNum(num);
}
SX_LIB_API void SGame_Sync()
{
	SG_PRECOND(_VOID);
	XPROFILE_FUNCTION();
	GameData::m_pMgr->sync();
	g_pGameData->sync();
}

SX_LIB_API gui::IGUI* SGame_GetGUI()
{
	SG_PRECOND(NULL);
	return(GameData::m_pGUI);
}

SX_LIB_API void SGame_Render(IXRenderTarget *pFinalTarget)
{
	SG_PRECOND(_VOID);
	//GameData::m_pMgr->render();
	g_pGameData->render(pFinalTarget);
}

SX_LIB_API void SGame_RenderHUD()
{
	SG_PRECOND(_VOID);
	g_pGameData->renderHUD();
}


SX_LIB_API void SGame_Dbg_Set(report_func rf)
{
	g_fnReportf = rf;
}

SX_LIB_API void SGame_PlayerSpawn()
{
	SG_PRECOND(_VOID);
	GameData::m_pPlayer->spawn();
}

SX_LIB_API void SGame_EditorRender(ID id, ID id_sel_tex, const float3 *pvRenderPos)
{
	SG_PRECOND(_VOID);
#ifdef _GRAPHIX_API
	CBaseEntity* pEnt = SGame_EntGet(id);
	if (!pEnt)
		return;

	SMMATRIX mView, mProj;
	Core_RMatrixGet(G_RI_MATRIX_VIEW, &mView);
	Core_RMatrixGet(G_RI_MATRIX_PROJECTION, &mProj);

	SGCore_GetDXDevice()->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&mView);
	SGCore_GetDXDevice()->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&mProj);

	float3 vBoxSize = pEnt->getEditorBoxSize();

	if(strcmp(pEnt->getClassName(), "path_corner") == 0)
	{
		CPathCorner * pStartPoint = (CPathCorner*)pEnt;
		if (!pStartPoint)
			return;

		CPathCorner * pCur = pStartPoint;
		while ((pCur = pCur->getPrev()))
		{
			pStartPoint = pCur;
		}

		float len = 0;
		int count = 0;
		pCur = pStartPoint;
		do
		{
			len += pCur->getLength();
			++count;
			SGCore_GetDXDevice()->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&(SMMatrixScaling(vBoxSize) * pEnt->getOrient().GetMatrix() * SMMatrixTranslation(pCur->getPos())));

			if (id == pCur->getId())
				SGCore_GetDXDevice()->setTexture(SGCore_LoadTexGetTex(id_sel_tex));
			else
				SGCore_GetDXDevice()->setTexture(NULL);

			g_pFigureBox->draw();
		} while ((pCur = pCur->getNext()));


		int npoints = count * 10;
		static Array<float3_t> pts;

		if ((int)pts.size() <= npoints)
		{
			pts.clear();
			pts.resize(npoints*2);
		}
		
		for (int i = 0; i < npoints; ++i)
		{
			pts[i * 2] = pStartPoint->getPoint((float)i * (len / (float)npoints));
		}
		for (int i = 0; i < npoints - 1; ++i)
		{
			pts[i * 2 + 1] = pts[(i + 1) * 2];
		}
		pts[npoints * 2 - 1] = pStartPoint->getPoint(len);

		//pts[npoints * 2] = pTrain->getPos();
		//pts[npoints * 2 + 1] = (float3)(pTrain->getPos() + (pTrain->getOrient() * float3(0.0f, 1.0f, 0.0f)));

		SGCore_ShaderUnBind();

		SGCore_GetDXDevice()->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&SMMatrixIdentity());

		SGCore_GetDXDevice()->SetFVF(D3DFVF_XYZ);
		SGCore_GetDXDevice()->setTexture(NULL);
		DX_CALL(SGCore_GetDXDevice()->DrawPrimitiveUP(D3DPT_LINELIST, npoints, &(pts[0]), sizeof(float3_t)));
	}
	else
	{
		SMMATRIX mView, mProj;
		Core_RMatrixGet(G_RI_MATRIX_VIEW, &mView);
		Core_RMatrixGet(G_RI_MATRIX_PROJECTION, &mProj);

		SGCore_GetDXDevice()->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&mView);
		SGCore_GetDXDevice()->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&mProj);
		SGCore_GetDXDevice()->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&(SMMatrixScaling(vBoxSize) * pEnt->getOrient().GetMatrix() * SMMatrixTranslation(pvRenderPos ? *pvRenderPos : pEnt->getPos()))/*bEnt->getWorldTM()*/);

		DX_CALL(SGCore_GetDXDevice()->SetTexture(0, SGCore_LoadTexGetTex(id_sel_tex)));
		DX_CALL(g_pFigureBox->DrawSubset(0));
	}
#endif
}

SX_LIB_API int SGame_EntGetClassListCount()
{
	SG_PRECOND(-1);

	return(CEntityFactoryMap::GetInstance()->getListCount());
}

SX_LIB_API void SGame_EntGetClassList(const char ** pszClassList, int count)
{
	SG_PRECOND(_VOID);

	CEntityFactoryMap::GetInstance()->getListing(pszClassList, count);
}

SX_LIB_API CBaseEntity * SGame_CreateEntity(const char * classname)
{
	SG_PRECOND(NULL);

	return(CREATE_ENTITY(classname, GameData::m_pMgr));
}

SX_LIB_API void SGame_RemoveEntity(CBaseEntity * pEnt)
{
	SG_PRECOND(_VOID);

	REMOVE_ENTITY(pEnt);
}

SX_LIB_API proptable_t * SGame_EntGetProptable(const char * classname)
{
	SG_PRECOND(NULL);

	return(CEntityFactoryMap::GetInstance()->getPropTable(classname));
}

SX_LIB_API const char * SGame_EntGetDefault(const char * classname, const char * key)
{
	SG_PRECOND(NULL);

	EntDefaultsMap * pMap = CEntityFactoryMap::GetInstance()->getDefaults(classname);
	if(!pMap)
	{
		return(NULL);
	}

	const EntDefaultsMap::Node * pNode;
	if(!pMap->KeyExists(AAString(key), &pNode))
	{
		return(NULL);
	}
	return(*pNode->Val);
}

SX_LIB_API int SGame_EntGetCount()
{
	SG_PRECOND(0);
	return(GameData::m_pMgr->getCount());
}

SX_LIB_API CBaseEntity *SGame_EntGet(ID id)
{
	SG_PRECOND(NULL);
	return(GameData::m_pMgr->getById(id));
}

SX_LIB_API CBaseEntity *SGame_EntGetByName(const char *szName, ID idStart)
{
	SG_PRECOND(NULL);

	CBaseEntity *pEnt = 0;

	for (int i = 0, il = SGame_EntGetCount(); i < il; ++i)
	{
		pEnt = SGame_EntGet(i);
		if (pEnt && strcmp(szName, pEnt->getName()) == 0)
		{
			return pEnt;
		}
	}

	return 0;
}

SX_LIB_API BOOL SGame_AddWMsg(UINT message, WPARAM wParam, LPARAM lParam)
{
	static const bool * pbHudDraw = GET_PCVAR_BOOL("hud_draw");
	if(!pbHudDraw)
	{
		pbHudDraw = GET_PCVAR_BOOL("hud_draw");
	}
	if(!GameData::m_pGUI || (pbHudDraw && !*pbHudDraw))
	{
		return(TRUE);
	}

	static const bool *s_pisWindowed = GET_PCVAR_BOOL("r_win_windowed");
	if(!*s_pisWindowed && message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
	{
		POINT pt;
		if(GetCursorPos(&pt))
		{
			short x = (short)pt.x;
			short y = (short)pt.y;
			lParam = (*((unsigned short*)&x)) | (*((unsigned short*)&y) << 16);
		}
	}

	static const bool *s_pGrabCursor = GET_PCVAR_BOOL("cl_grab_cursor");
	if(message == WM_ACTIVATE && s_pGrabCursor && *s_pGrabCursor && LOWORD(wParam) == WA_INACTIVE)
	{
		Core_0ConsoleExecCmd("game_menu");
	}

	return(GameData::m_pGUIStack->putMessage(message, wParam, lParam));
}

#if 0
SX_LIB_API ID SGame_EntGetByRay(const float3 &vStart, const float3 &vDir, float3 *pHitPos)
{
	SG_PRECOND(-1);
	float3 vEnd = vStart + vDir * 10000.0f;
	btCollisionWorld::ClosestRayResultCallback cb(F3_BTVEC(vStart), F3_BTVEC(vEnd));
	cb.m_collisionFilterGroup = CG_BULLETFIRE;
	cb.m_collisionFilterMask = CG_ALL;
	cb.m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;
	SPhysics_GetDynWorld()->rayTest(F3_BTVEC(vStart), F3_BTVEC(vEnd), cb);

	if(cb.hasHit())
	{
		CBaseEntity *pEnt = (CBaseEntity*)cb.m_collisionObject->getUserPointer();
		if(pEnt)
		{
			if(pHitPos)
			{
				*pHitPos = BTVEC_F3(cb.m_hitPointWorld);
			}
			return(pEnt->getId());
		}
	}
	return(-1);
}
#endif
