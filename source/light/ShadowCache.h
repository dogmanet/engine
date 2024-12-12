#ifndef __SHADOW_CACHE_H
#define __SHADOW_CACHE_H

#include <gdefines.h>

#include <mtrl/IXMaterialSystem.h>
#include <common/array.h>
#include "shadow.h"

#include "light.h"

enum SHADOW_TYPE
{
	SHADOW_TYPE_PSSM,
	SHADOW_TYPE_OMNI,
	SHADOW_TYPE_SPOT
};

class CShadowCache
{
	friend class CRShadowSizeCvarListener;
public:
	CShadowCache(IXRender *pRender, IXMaterialSystem *pMaterialSystem);
	~CShadowCache();

	SX_ALIGNED_OP_MEM();

	//! Установка количества лампочек, инициализация кэша
	void setLightsCount(UINT iPoints, UINT iSpots, bool hasGlobal);
	void setRSMLightsCount(UINT iPoints, UINT iSpots, bool hasGlobal);

	//! Указывает, что начался новый кадр
	void nextFrame();

	void setObserverCamera(IXCamera *pCamera);

	//! Добавляет источник к текущему проходу, В случае отсутствия свободных слотов, возвращает false
	void addLight(CXLight *pLight);
	void addRSMLight(CXLight *pLight);

	UINT processNextBunch();
	UINT processNextRSMBunch();
	CXLight *getLight(ID id);
	CXLight *getRSMLight(ID id);
	IBaseShadowMap *getShadow(ID id);
	IBaseReflectiveShadowMap *getRSMShadow(ID id);

protected:
	IXRender *m_pRender;
	IXMaterialSystem *m_pMaterialSystem;
	CRShadowSizeCvarListener *m_pShadowSizeCvarListener;

	IXCamera *m_pCamera = NULL;

	IGXRasterizerState *m_pRasterizerConservative = NULL;
	IGXRasterizerState *m_pRasterizerCullFront = NULL;

	UINT m_uCurrentFrame = 0;
	Array<CXLight*> m_aFrameLights;
	Array<CXLight*> m_aFrameRSMLights;
	bool m_isFirstBunch = true;
	bool m_isFirstRSMBunch = true;

	struct ShadowMap
	{
		CShadowMap map;
		bool isDirty = false;
		bool shouldProcess = false;
		CXLight *pLight = NULL;
		UINT uLastUsed = UINT_MAX;
	};

	struct ShadowCubeMap
	{
		CShadowCubeMap map;
		bool isDirty = false;
		bool shouldProcess = false;
		CXLight *pLight = NULL;
		UINT uLastUsed = UINT_MAX;
	};

	struct ShadowPSSM
	{
		CShadowPSSM map;
		bool isDirty = false;
		bool shouldProcess = false;
		CXLight *pLight = NULL;

		SX_ALIGNED_OP_MEM();
	};
	
	struct ReflectiveShadowMap
	{
		CReflectiveShadowMap map;
		bool isDirty = false;
		bool shouldProcess = false;
		CXLight *pLight = NULL;
		UINT uLastUsed = UINT_MAX;
	};

	struct ReflectiveShadowCubeMap
	{
		CReflectiveShadowCubeMap map;
		bool isDirty = false;
		bool shouldProcess = false;
		CXLight *pLight = NULL;
		UINT uLastUsed = UINT_MAX;
	};

	struct ReflectiveShadowSun
	{
		CReflectiveShadowSun map;
		bool isDirty = false;
		bool shouldProcess = false;
		CXLight *pLight = NULL;

		SX_ALIGNED_OP_MEM();
	};


	template<class T, class R> struct Cache
	{
	private:
		IXRender *m_pRender;
		Array<T> m_aMaps;
		Array<T*> m_aMapsQueue;
		LIGHT_RENDER_TYPE m_renderType;
		Array<CXLight*> &m_aFrameLights;
		Array<R> &m_aReadyMaps;
	public:
		Cache(Array<CXLight*> &aFrameLights, Array<R> &aReadyMaps, IXRender *pRender, LIGHT_RENDER_TYPE renderType):
			m_aFrameLights(aFrameLights),
			m_aReadyMaps(aReadyMaps),
			m_pRender(pRender),
			m_renderType(renderType)
		{
		}
		void setRenderType(LIGHT_RENDER_TYPE renderType)
		{
			m_renderType = renderType;
		}
		void setSize(UINT uSize, UINT uMapSize)
		{
			if(m_aMaps.size() != uSize)
			{
				UINT i = m_aMaps.size();
				m_aMaps.resizeFast(uSize);
				m_aMapsQueue.resizeFast(uSize);
				for(; i < uSize; ++i)
				{
					m_aMaps[i].map.init(m_pRender, uMapSize);
				}

				for(i = 0; i < uSize; ++i)
				{
					m_aMapsQueue[i] = &m_aMaps[i];
				}
			}
		}
		void updateLastUsed()
		{
			T *pSM;
			for(UINT i = 0, l = m_aMaps.size(); i < l; ++i)
			{
				pSM = &m_aMaps[i];

				if(pSM->uLastUsed != UINT_MAX)
				{
					++pSM->uLastUsed;
				}
			}
		}

		bool processFirstBunch()
		{
			T *pSM;
			ID id;
			bool isSomeFound = false;
			for(UINT i = 0, l = m_aMaps.size(); i < l; ++i)
			{
				pSM = &m_aMaps[i];
				if(!pSM->isDirty && (id = m_aFrameLights.indexOf(pSM->pLight)) >= 0)
				{
					if(pSM->pLight->isDirty(m_renderType))
					{
						pSM->pLight->markClean(m_renderType);
						pSM->isDirty = true;
					}
					m_aFrameLights.erase(id);
					pSM->shouldProcess = true;
					isSomeFound = true;
				}
			}
			return(isSomeFound);
		}
		void sortQueue()
		{
			m_aMapsQueue.quickSort([](const T *a, const T *b){
				return(a->uLastUsed > b->uLastUsed);
			});
		}
		bool checkIthLight(UINT uMap, int i)
		{
			if(m_aMapsQueue.size() > uMap)
			{
				m_aMapsQueue[uMap]->isDirty = true;
				m_aMapsQueue[uMap]->shouldProcess = true;
				if(m_aMapsQueue[uMap]->pLight)
				{
					m_aMapsQueue[uMap]->pLight->markDirty(m_renderType);
				}
				m_aMapsQueue[uMap]->pLight = m_aFrameLights[i];
				m_aFrameLights.erase(i);
				return(true);
			}
			return(false);
		}
		void processTheRest()
		{
			T *pSM;
			for(UINT i = 0, l = m_aMaps.size(); i < l; ++i)
			{
				pSM = &m_aMaps[i];
				if(pSM->shouldProcess)
				{
					pSM->uLastUsed = 0;
					pSM->shouldProcess = false;
					if(pSM->isDirty)
					{
						pSM->map.setLight(pSM->pLight);
						pSM->map.process();
						pSM->isDirty = false;
					}

					m_aReadyMaps.push_back({&pSM->map, pSM->pLight});
				}
			}
		}
		void dropCaches()
		{
			m_aMaps.resizeFast(0);
			m_aMapsQueue.resizeFast(0);
		}
	};

	struct ReadyShadows
	{
		IBaseShadowMap *pShadowMap;
		CXLight *pLight;
	};

	Array<ReadyShadows> m_aReadyMaps;

	struct ReadyReflectiveShadows
	{
		IBaseReflectiveShadowMap *pShadowMap;
		CXLight *pLight;
	};

	Array<ReadyReflectiveShadows> m_aReadyReflectiveMaps;

	Cache<ShadowMap, ReadyShadows> m_shadowMaps;
	Cache<ShadowCubeMap, ReadyShadows> m_shadowCubeMaps;
	Cache<ReflectiveShadowMap, ReadyReflectiveShadows> m_reflectiveShadowMaps;
	Cache<ReflectiveShadowCubeMap, ReadyReflectiveShadows> m_reflectiveShadowCubeMaps;

	ShadowPSSM *m_pShadowPSSM = NULL;
	ReflectiveShadowSun *m_pReflectiveShadowSun = NULL;

	//XGeometryShaderHandler *m_pRSMGeometryShader = NULL;
	XGeometryShaderHandler *m_pCubemapGeometryShader = NULL;
	XGeometryShaderHandler *m_pPSSMGeometryShader[PSSM_MAX_SPLITS];
	XRenderPassHandler *m_pRenderPassShadow = NULL;

	void dropCaches();
};

#endif
