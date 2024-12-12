#include "ShadowCache.h"
#include <core/sxcore.h>

class CRShadowSizeCvarListener: public IEventListener<XEventCvarChanged>
{
public:
	CRShadowSizeCvarListener(IXCore *pCore, CShadowCache *pShadowCache):
		m_pCore(pCore),
		m_pShadowCache(pShadowCache)
	{
	}

	virtual void onEvent(const XEventCvarChanged *pEvent)
	{
		static const float *s_pfRLSMQuality = m_pCore->getConsole()->getPCVarFloat("r_lsm_quality");
		static const float *s_pfRPSMQuality = m_pCore->getConsole()->getPCVarFloat("r_psm_quality");
		static const float *s_pfRPSSMQuality = m_pCore->getConsole()->getPCVarFloat("r_pssm_quality");
		if(pEvent->pCvar == s_pfRLSMQuality 
			|| pEvent->pCvar == s_pfRPSMQuality
			|| pEvent->pCvar == s_pfRPSSMQuality
			)
		{
			m_pShadowCache->dropCaches();
		}
	}

private:
	CShadowCache *m_pShadowCache;
	IXCore *m_pCore;
};

//##########################################################################

CShadowCache::CShadowCache(IXRender *pRender, IXMaterialSystem *pMaterialSystem):
	m_pRender(pRender),
	m_pMaterialSystem(pMaterialSystem),
	m_shadowMaps(m_aFrameLights, m_aReadyMaps, m_pRender, LRT_LIGHT),
	m_shadowCubeMaps(m_aFrameLights, m_aReadyMaps, m_pRender, LRT_LIGHT),
	m_reflectiveShadowMaps(m_aFrameRSMLights, m_aReadyReflectiveMaps, m_pRender, LRT_LPV),
	m_reflectiveShadowCubeMaps(m_aFrameRSMLights, m_aReadyReflectiveMaps, m_pRender, LRT_LPV)
{
	IXCore *pCore = Core_GetIXCore();
	m_pShadowSizeCvarListener = new CRShadowSizeCvarListener(pCore, this);
	pCore->getEventChannel<XEventCvarChanged>(EVENT_CVAR_CHANGED_GUID)->addListener(m_pShadowSizeCvarListener);

	IXConsole *pConsole = pCore->getConsole();

	pConsole->registerCVar("r_lsm_quality", 0.5f, "Коэфициент размера карты глубины для направленных источников света [0.5,4] (низкое, высокое)", FCVAR_NOTIFY);
	pConsole->registerCVar("r_psm_quality", 0.5f, "Коэфициент размера карты глубины для точечных источников света [0.5,4] (низкое, высокое)", FCVAR_NOTIFY);
	pConsole->registerCVar("r_pssm_quality", 0.5f, "Коэфициент размера карты глубины для солнца [0.5,4] (низкое, высокое)", FCVAR_NOTIFY);
	pConsole->registerCVar("r_sm_max_memory", 0.15f, "Максимальный процент от доступной видеопамяти, отводимый под кэш теней");
	pConsole->registerCVar("r_pssm_splits", 6, "Количество PSSM сплитов. Допустимые значения от 1 до 6");
	pConsole->registerCVar("r_pssm_max_distance", 800.0f, "Дальность прорисовки PSSM");
	
	const char *aszGSRequiredParams[] = {
		"vPosition",
		"vPos",
		NULL
	};
	GXMacro aCubemapDefines[] = {
		{"IS_CUBEMAP", "1"},
		GX_MACRO_END()
	};
	m_pCubemapGeometryShader = m_pMaterialSystem->registerGeometryShader("sm/rsmcube.gs", aszGSRequiredParams, aCubemapDefines);

	char tmp[16];
	for(int i = 0; i < PSSM_MAX_SPLITS; ++i)
	{
		sprintf(tmp, "%d", i + 1);
		GXMacro aPSSMDefines[] = {
			{"PSSM_MAX_SPLITS", tmp},
			GX_MACRO_END()
		};
		m_pPSSMGeometryShader[i] = m_pMaterialSystem->registerGeometryShader("sm/rsmcube.gs", aszGSRequiredParams, aPSSMDefines);
	}

	m_pRenderPassShadow = m_pMaterialSystem->getRenderPass("xShadow");

	GXRasterizerDesc rasterizerDesc;
	//rasterizerDesc.cullMode = GXCULL_FRONT;
	//rasterizerDesc.useConservativeRasterization = true;
	m_pRasterizerConservative = m_pRender->getDevice()->createRasterizerState(&rasterizerDesc);

	rasterizerDesc.cullMode = GXCULL_FRONT;
	m_pRasterizerCullFront = m_pRender->getDevice()->createRasterizerState(&rasterizerDesc);
}
CShadowCache::~CShadowCache()
{
	Core_GetIXCore()->getEventChannel<XEventCvarChanged>(EVENT_CVAR_CHANGED_GUID)->removeListener(m_pShadowSizeCvarListener);
	mem_delete(m_pShadowSizeCvarListener);
	mem_delete(m_pShadowPSSM);
	mem_delete(m_pReflectiveShadowSun);
	mem_release(m_pRasterizerCullFront);
	mem_release(m_pRasterizerConservative);
}

void CShadowCache::setLightsCount(UINT uPoints, UINT uSpots, bool hasGlobal)
{
	static const float *s_pfRLSMQuality = GET_PCVAR_FLOAT("r_lsm_quality");
	static const float *s_pfRPSMQuality = GET_PCVAR_FLOAT("r_psm_quality");
	static const float *s_pfRPSSMQuality = GET_PCVAR_FLOAT("r_pssm_quality");
	static const float *s_pfRSMMaxMemory = GET_PCVAR_FLOAT("r_sm_max_memory");
	const UINT uSpotShadowmapSize = (UINT)(512 * *s_pfRLSMQuality);
	const UINT uPointShadowmapSize = (UINT)(512 * *s_pfRPSMQuality);
	const UINT uSunShadowmapSize = (UINT)(512 * *s_pfRPSSMQuality);

	size_t stMaxMem = (size_t)(m_pRender->getDevice()->getAdapterDesc()->sizeTotalMemory * *s_pfRSMMaxMemory);

	size_t stPointsMemory = uPoints * CShadowCubeMap::GetMapMemory(uPointShadowmapSize);
	size_t stSpotsMemory = uSpots * CShadowMap::GetMapMemory(uSpotShadowmapSize);
	size_t stSunMemory = CShadowPSSM::GetMapMemory(uSunShadowmapSize);

	size_t stTotal = stPointsMemory + stSpotsMemory;
	stMaxMem -= stSunMemory;

	if(stTotal > stMaxMem)
	{
		float fCoeff = (float)stMaxMem / (float)stTotal;

		stPointsMemory = (size_t)(stPointsMemory * fCoeff);
		stSpotsMemory = (size_t)(stSpotsMemory * fCoeff);

		if(uSpots)
		{
			uSpots = (UINT)(stSpotsMemory / CShadowMap::GetMapMemory(uSpotShadowmapSize));
			if(!uSpots)
			{
				uSpots = 1;
			}
		}
		if(uPoints)
		{
			uPoints = (UINT)(stPointsMemory / CShadowCubeMap::GetMapMemory(uPointShadowmapSize));
			if(!uPoints)
			{
				uPoints = 1;
			}
		}
	}
	
	m_shadowMaps.setSize(uSpots, uSpotShadowmapSize);
	m_shadowCubeMaps.setSize(uPoints, uPointShadowmapSize);

	m_aReadyMaps.resizeFast(uSpots + uPoints + (hasGlobal ? 1 : 0));

	if(hasGlobal)
	{
		if(!m_pShadowPSSM)
		{
			m_pShadowPSSM = new ShadowPSSM();
			m_pShadowPSSM->map.init(m_pRender, uSunShadowmapSize);
		}
	}
	else
	{
		mem_delete(m_pShadowPSSM);
	}
}

void CShadowCache::setRSMLightsCount(UINT uPoints, UINT uSpots, bool hasGlobal)
{
	//static const float *s_pfRRSMQuality = GET_PCVAR_FLOAT("r_rsm_quality");
	const UINT uSpotShadowmapSize = (UINT)(RSM_SPOT_SIZE);
	const UINT uPointShadowmapSize = (UINT)(RSM_POINT_SIZE);
	const UINT uSunShadowmapSize = (UINT)(RSM_SUN_SIZE);

	//size_t stMaxMem = (size_t)(m_pRenderPipeline->getDevice()->getAdapterDesc()->sizeTotalMemory * *s_pfRSMMaxMemory);
	size_t stMaxMem = (size_t)(m_pRender->getDevice()->getAdapterDesc()->sizeTotalMemory * 0.3f);

	size_t stPointsMemory = uPoints * CReflectiveShadowCubeMap::GetMapMemory(uPointShadowmapSize);
	size_t stSpotsMemory = uSpots * CReflectiveShadowMap::GetMapMemory(uSpotShadowmapSize);
	size_t stSunMemory = CReflectiveShadowSun::GetMapMemory(uSunShadowmapSize);

	size_t stTotal = stPointsMemory + stSpotsMemory;
	stMaxMem -= stSunMemory;

	if(stTotal > stMaxMem)
	{
		float fCoeff = (float)stMaxMem / (float)stTotal;

		stPointsMemory = (size_t)(stPointsMemory * fCoeff);
		stSpotsMemory = (size_t)(stSpotsMemory * fCoeff);

		if(uSpots)
		{
			uSpots = (UINT)(stSpotsMemory / CReflectiveShadowMap::GetMapMemory(uSpotShadowmapSize));
			if(!uSpots)
			{
				uSpots = 1;
			}
		}
		if(uPoints)
		{
			uPoints = (UINT)(stPointsMemory / CReflectiveShadowCubeMap::GetMapMemory(uPointShadowmapSize));
			if(!uPoints)
			{
				uPoints = 1;
			}
		}
	}

	m_reflectiveShadowMaps.setSize(uSpots, uSpotShadowmapSize);
	m_reflectiveShadowCubeMaps.setSize(uPoints, uPointShadowmapSize);

	m_aReadyReflectiveMaps.resizeFast(uSpots + uPoints + (hasGlobal ? 1 : 0));

	if(hasGlobal)
	{
		if(!m_pReflectiveShadowSun)
		{
			m_pReflectiveShadowSun = new ReflectiveShadowSun();
			m_pReflectiveShadowSun->map.init(m_pRender, uSunShadowmapSize);
		}
	}
	else
	{
		mem_delete(m_pReflectiveShadowSun);
	}
}

void CShadowCache::nextFrame()
{
	++m_uCurrentFrame;
	m_aFrameLights.clearFast();
	m_aFrameRSMLights.clearFast();
	m_isFirstBunch = true;
	
	m_shadowMaps.updateLastUsed();
	m_shadowCubeMaps.updateLastUsed();
	m_reflectiveShadowMaps.updateLastUsed();
	m_reflectiveShadowCubeMaps.updateLastUsed();
}

void CShadowCache::addLight(CXLight *pLight)
{
	m_aFrameLights.push_back(pLight);
}

void CShadowCache::addRSMLight(CXLight *pLight)
{
	m_aFrameRSMLights.push_back(pLight);
}

UINT CShadowCache::processNextBunch()
{
	if(!m_aFrameLights.size())
	{
		return(0);
	}

	static const int *r_pssm_splits = GET_PCVAR_INT("r_pssm_splits");

	m_pRender->getDevice()->getThreadContext()->setDepthStencilState(NULL);
	m_pMaterialSystem->setCullMode(GXCULL_FRONT);
//	m_pRenderPipeline->getDevice()->getThreadContext()->setRasterizerState(m_pRasterizerCullFront);

	bool isSomeFound = false;
	if(m_isFirstBunch)
	{
		if(m_shadowMaps.processFirstBunch())
		{
			isSomeFound = true;
		}
		if(m_shadowCubeMaps.processFirstBunch())
		{
			isSomeFound = true;
		}

		ID id;
		if(false && m_pShadowPSSM && !m_pShadowPSSM->isDirty && (id = m_aFrameLights.indexOf(m_pShadowPSSM->pLight)) >= 0)
		{
			if(m_pShadowPSSM->pLight->isDirty(LRT_LIGHT))
			{
				m_pShadowPSSM->pLight->markClean(LRT_LIGHT);
				m_pShadowPSSM->isDirty = true;
			}
			m_aFrameLights.erase(id);
			m_pShadowPSSM->shouldProcess = true;
			isSomeFound = true;
		}

		m_isFirstBunch = false;
	}
	
	if(!isSomeFound)
	{
		UINT uPoints = 0, uSpots = 0;
		bool isFound = false;

		m_shadowMaps.sortQueue();
		m_shadowCubeMaps.sortQueue();

		for(int i = (int)m_aFrameLights.size() - 1; i >= 0; --i)
		{
			switch(m_aFrameLights[i]->getType())
			{
			case LIGHT_TYPE_SPOT:
				if(m_shadowMaps.checkIthLight(uSpots, i))
				{
					++uSpots;
					isFound = true;
				}
				break;
			case LIGHT_TYPE_POINT:
				if(m_shadowCubeMaps.checkIthLight(uSpots, i))
				{
					++uPoints;
					isFound = true;
				}
				break;
			case LIGHT_TYPE_SUN:
				m_pShadowPSSM->isDirty = true;
				m_pShadowPSSM->shouldProcess = true;
				m_pShadowPSSM->pLight = m_aFrameLights[i];
				m_aFrameLights.erase(i);
				isFound = true;
				break;
			}
			if(!isFound)
			{
				break;
			}
		}
	}
	m_aReadyMaps.clearFast();

	// render shadows
	m_pMaterialSystem->bindRenderPass(m_pRenderPassShadow);

	m_shadowMaps.processTheRest();

	m_pMaterialSystem->bindGS(m_pPSSMGeometryShader[*r_pssm_splits - 1]);

	if(m_pShadowPSSM && m_pShadowPSSM->shouldProcess && m_pCamera)
	{
		m_pShadowPSSM->shouldProcess = false;

		if(m_pShadowPSSM->isDirty)
		{
			m_pShadowPSSM->map.setObserverCamera(m_pCamera);
			m_pShadowPSSM->map.setLight(m_pShadowPSSM->pLight);
			m_pShadowPSSM->map.process();
			m_pShadowPSSM->isDirty = false;
		}

		m_aReadyMaps.push_back({&m_pShadowPSSM->map, m_pShadowPSSM->pLight});
	}

	m_pMaterialSystem->bindGS(m_pCubemapGeometryShader);

	m_pMaterialSystem->bindRenderPass(m_pRenderPassShadow, 1);

	m_shadowCubeMaps.processTheRest();

	m_pMaterialSystem->bindGS(NULL);

	m_pMaterialSystem->setCullMode(GXCULL_BACK);

	return(m_aReadyMaps.size());
}

UINT CShadowCache::processNextRSMBunch()
{
	if(!m_aFrameRSMLights.size())
	{
		return(0);
	}

	m_pRender->getDevice()->getThreadContext()->setDepthStencilState(NULL);
	m_pMaterialSystem->setCullMode(GXCULL_BACK);
//	m_pRenderPipeline->getDevice()->getThreadContext()->setRasterizerState(m_pRasterizerConservative);

	bool isSomeFound = false;
	if(m_isFirstRSMBunch)
	{
		if(m_reflectiveShadowMaps.processFirstBunch())
		{
			isSomeFound = true;
		}
		if(m_reflectiveShadowCubeMaps.processFirstBunch())
		{
			isSomeFound = true;
		}

		ID id;
		if(false && m_pReflectiveShadowSun && !m_pReflectiveShadowSun->isDirty && (id = m_aFrameRSMLights.indexOf(m_pReflectiveShadowSun->pLight)) >= 0)
		{
			if(m_pReflectiveShadowSun->pLight->isDirty(LRT_LPV))
			{
				m_pReflectiveShadowSun->pLight->markClean(LRT_LPV);
				m_pReflectiveShadowSun->isDirty = true;
			}
			m_aFrameRSMLights.erase(id);
			m_pReflectiveShadowSun->shouldProcess = true;
			isSomeFound = true;
		}

		m_isFirstBunch = false;
	}

	if(!isSomeFound)
	{
		UINT uPoints = 0, uSpots = 0;
		bool isFound = false;

		m_reflectiveShadowMaps.sortQueue();
		m_reflectiveShadowCubeMaps.sortQueue();

		for(int i = (int)m_aFrameRSMLights.size() - 1; i >= 0; --i)
		{
			switch(m_aFrameRSMLights[i]->getType())
			{
			case LIGHT_TYPE_SPOT:
				if(m_reflectiveShadowMaps.checkIthLight(uSpots, i))
				{
					++uSpots;
					isFound = true;
				}
				break;
			case LIGHT_TYPE_POINT:
				if(m_reflectiveShadowCubeMaps.checkIthLight(uSpots, i))
				{
					++uPoints;
					isFound = true;
				}
				break;
			case LIGHT_TYPE_SUN:
				m_pReflectiveShadowSun->isDirty = true;
				m_pReflectiveShadowSun->shouldProcess = true;
				m_pReflectiveShadowSun->pLight = m_aFrameRSMLights[i];
				m_aFrameRSMLights.erase(i);
				isFound = true;
				break;
			}
			if(!isFound)
			{
				break;
			}
		}
	}
	m_aReadyReflectiveMaps.clearFast();

	// render shadows
	m_pMaterialSystem->bindRenderPass(m_pRenderPassShadow, 2);

	m_reflectiveShadowMaps.processTheRest();

	//m_pMaterialSystem->bindGS(m_pPSSMGeometryShader[*r_pssm_splits - 1]);

	if(m_pReflectiveShadowSun && m_pReflectiveShadowSun->shouldProcess && m_pCamera)
	{
		m_pReflectiveShadowSun->shouldProcess = false;

		if(m_pReflectiveShadowSun->isDirty)
		{
			m_pReflectiveShadowSun->map.setObserverCamera(m_pCamera);
			m_pReflectiveShadowSun->map.setLight(m_pReflectiveShadowSun->pLight);
			m_pReflectiveShadowSun->map.process();
			m_pReflectiveShadowSun->isDirty = false;
		}

		m_aReadyReflectiveMaps.push_back({&m_pReflectiveShadowSun->map, m_pReflectiveShadowSun->pLight});
	}

	m_pMaterialSystem->bindGS(m_pCubemapGeometryShader);

	m_pMaterialSystem->bindRenderPass(m_pRenderPassShadow, 3);

	m_reflectiveShadowCubeMaps.processTheRest();

	m_pMaterialSystem->bindGS(NULL);

	return(m_aReadyReflectiveMaps.size());
}

CXLight* CShadowCache::getLight(ID id)
{
	assert(ID_VALID(id) && m_aReadyMaps.size() > (UINT)id);
	return(m_aReadyMaps[id].pLight);
}

CXLight* CShadowCache::getRSMLight(ID id)
{
	assert(ID_VALID(id) && m_aReadyReflectiveMaps.size() > (UINT)id);
	return(m_aReadyReflectiveMaps[id].pLight);
}

IBaseShadowMap* CShadowCache::getShadow(ID id)
{
	assert(ID_VALID(id) && m_aReadyMaps.size() > (UINT)id);
	return(m_aReadyMaps[id].pShadowMap);
}

IBaseReflectiveShadowMap* CShadowCache::getRSMShadow(ID id)
{
	assert(ID_VALID(id) && m_aReadyReflectiveMaps.size() > (UINT)id);
	return(m_aReadyReflectiveMaps[id].pShadowMap);
}

void CShadowCache::setObserverCamera(IXCamera *pCamera)
{
	m_pCamera = pCamera;
}

void CShadowCache::dropCaches()
{
	m_shadowMaps.dropCaches();
	m_shadowCubeMaps.dropCaches();
	mem_delete(m_pShadowPSSM);
}
