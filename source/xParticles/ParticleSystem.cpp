#include "ParticleSystem.h"
#include "EffectLoader.h"
#include "RenderableVisibility.h"
#include <xcommon/IPluginManager.h>

CParticleSystem::CParticleSystem(IXCore *pCore):
	m_pCore(pCore),
	m_updateLoop(this)
{
	m_pMaterialChangedEventListener = new CMaterialChangedEventListener(this);
	pCore->getEventChannel<XEventMaterialChanged>(EVENT_MATERIAL_CHANGED_GUID)->addListener(m_pMaterialChangedEventListener);

	m_pScene = (IXScene*)pCore->getPluginManager()->getInterface(IXSCENE_GUID);
	m_pObjectType = m_pScene->registerObjectType("xParticles");
	m_pFeatureOpaque = m_pScene->registerObjectFeature("xOpaque");
	m_pFeatureTransparent = m_pScene->registerObjectFeature("xTransparent");
	m_pFeatureSelfillum = m_pScene->registerObjectFeature("xSelfillum");

	m_pOpaqueQuery = m_pObjectType->newQuery();
	m_pOpaqueQuery->setFeature(m_pFeatureOpaque, SQF_SET);

	m_pTransparentQuery = m_pObjectType->newQuery();
	m_pTransparentQuery->setFeature(m_pFeatureTransparent, SQF_SET);

	m_pSelfillumQuery = m_pObjectType->newQuery();
	m_pSelfillumQuery->setFeature(m_pFeatureSelfillum, SQF_SET);
}

CParticleSystem::~CParticleSystem()
{
	m_pCore->getEventChannel<XEventMaterialChanged>(EVENT_MATERIAL_CHANGED_GUID)->removeListener(m_pMaterialChangedEventListener);
	mem_delete(m_pMaterialChangedEventListener);
	mem_release(m_pOpaqueQuery);
	mem_release(m_pTransparentQuery);
	mem_release(m_pSelfillumQuery);
}

bool XMETHODCALLTYPE CParticleSystem::newEffect(const char *szName, IXParticleEffect **ppOut)
{
	CParticleEffect *pEffect;
	if(newEffect(szName, &pEffect))
	{
		*ppOut = pEffect;
		return(true);
	}

	return(false);
}
bool CParticleSystem::newEffect(const char *szName, CParticleEffect **ppOut)
{
	assert(szName && szName[0]);

	String key(szName);

	if(m_mapEffects.KeyExists(key))
	{
		return(false);
	}

	const Map<String, CParticleEffect>::Node *pNode = m_mapEffects.insert(key, this);
	CParticleEffect *pEffect = pNode->Val;
	pEffect->setName(pNode->Key.c_str());
	*ppOut = pEffect;

	return(true);
}
bool XMETHODCALLTYPE CParticleSystem::getEffect(const char *szName, IXParticleEffect **ppOut)
{
	String key(szName);

	CParticleEffect *pEffect;

	const Map<String, CParticleEffect>::Node *pNode;
	if(m_mapEffects.KeyExists(key, &pNode))
	{
		pEffect = pNode->Val;
		add_ref(pEffect);
		*ppOut = pEffect;
		return(true);
	}
	
	if(!newEffect(szName, &pEffect))
	{
		return(false);
	}

	//char szFile[MAX_PATH];

	//sprintf(szFile, "effects/%s.eff", szName);

	CEffectLoader loader(m_pCore);
	if(loader.loadFromFile(szName, pEffect))
	{
		*ppOut = pEffect;
		return(true);
	}

	mem_release(pEffect);

	return(false);
}

void XMETHODCALLTYPE CParticleSystem::newEffectPlayer(IXParticleEffect *pEffect, IXParticlePlayer **ppOut)
{
	CParticlePlayer *pPlayer;
	{
		ScopedSpinLock lock(m_slPlayerPool);
		pPlayer = m_poolPlayers.Alloc((CParticleEffect*)pEffect, this);
		*ppOut = pPlayer;
	}
	m_queuePlayers.emplace({PlayerQueueItem::PQI_ADD, pPlayer});
}

void CParticleSystem::onEffectReleased(const char *szName)
{
	m_mapEffects.erase(szName);
}

void CParticleSystem::onEffectPlayerReleased(CParticlePlayer *pPlayer)
{
	m_queuePlayers.emplace({PlayerQueueItem::PQI_REMOVE, pPlayer});
}

bool CParticleSystem::saveEffect(CParticleEffect *pEffect)
{
	//char szFile[MAX_PATH];

	//sprintf(szFile, "effects/%s.eff", pEffect->getName());

	CEffectLoader loader(m_pCore);
	return(loader.saveToFile(pEffect->getName(), pEffect));
}

ID CParticleSystem::update(float fDelta)
{
	XPROFILE_FUNCTION();

	m_updateLoop.setDelta(fDelta);

	return(m_pCore->forLoop(0, m_aPlayers.size(), &m_updateLoop, 1));
}

void CParticleSystem::updateRange(int iStart, int iEnd, float fDelta)
{
	XPROFILE_FUNCTION();

	for(int i = iStart; i < iEnd; ++i)
	{
		m_aPlayers[i]->update(fDelta);
	}
}

void CParticleSystem::sync()
{
	XPROFILE_FUNCTION();

	PlayerQueueItem item;
	while(m_queuePlayers.pop(&item))
	{
		switch(item.cmd)
		{
		case PlayerQueueItem::PQI_ADD:
			if(m_pDevice)
			{
				item.pPlayer->setDevice(m_pDevice);
			}
			m_aPlayers.push_back(item.pPlayer);
			break;

		case PlayerQueueItem::PQI_REMOVE:
			int idx = m_aPlayers.indexOf(item.pPlayer);
			assert(idx >= 0);
			if(idx >= 0)
			{
				m_aPlayers.erase(idx);
			}

			{
				ScopedSpinLock lock(m_slPlayerPool);
				m_poolPlayers.Delete(item.pPlayer);
			}
			break;
		}
	}
}

void CParticleSystem::onEmitterReleased(CParticleEffectEmitter *pEmitter)
{
	ScopedSpinLock lock(m_slEmittersPool);
	m_poolEmitters.Delete(pEmitter);
}

void CParticleSystem::onPlayerEmitterReleased(CParticlePlayerEmitter *pEmitter)
{
	ScopedSpinLock lock(m_slPlayerEmitterPool);
	m_poolPlayerEmitters.Delete(pEmitter);
}

CParticleEffectEmitter* CParticleSystem::allocEmitter()
{
	ScopedSpinLock lock(m_slEmittersPool);
	return(m_poolEmitters.Alloc(this));
}

CParticlePlayerEmitter* CParticleSystem::allocPlayerEmitter(CParticlePlayer *pPlayer)
{
	ScopedSpinLock lock(m_slPlayerEmitterPool);
	return(m_poolPlayerEmitters.Alloc(pPlayer));
}

void CParticleSystem::setDevice(IGXDevice *pDevice)
{
	m_pDevice = pDevice;

	if(pDevice)
	{
		fora(i, m_aPlayers)
		{
			m_aPlayers[i]->setDevice(pDevice);
		}
	}
}
void CParticleSystem::setMaterialSystem(IXMaterialSystem *pMaterialSystem)
{
	m_pMaterialSystem = pMaterialSystem;
	XVertexFormatHandler *pFormat = pMaterialSystem->getVertexFormat("xSceneGeneric");
	m_pVertexShaderNoTransformHandler = pMaterialSystem->registerVertexShader(pFormat, "base/particles.vs");
	GXMacro aMacro[] = {
		{"WITH_TRANSFORM", "1"},
		GX_MACRO_END()
	};
	m_pVertexShaderWithTransformHandler = pMaterialSystem->registerVertexShader(pFormat, "base/particles.vs", aMacro);
}

void CParticleSystem::render(CRenderableVisibility *pVisibility)
{
	/*if(m_pMaterialSystem)
	{
		fora(i, m_aPlayers)
		{
			m_aPlayers[i]->render(m_pMaterialSystem);
		}
	}*/
	if(!pVisibility)
	{
		return;
	}

	XPROFILE_FUNCTION();

	render(pVisibility->getRenderList(), MF_OPAQUE);
}
void CParticleSystem::renderEmissive(CRenderableVisibility *pVisibility)
{
	if(!pVisibility)
	{
		return;
	}

	render(pVisibility->getSelfillumList(), MF_SELFILLUM);
}

void CParticleSystem::render(Array<CParticlePlayerEmitter*> &aRenderList, XMODEL_FEATURE bmWhat)
{
	auto &rl = aRenderList;
	for(UINT i = 0, l = rl.size(); i < l; ++i)
	{
		rl[i]->render(m_pMaterialSystem/*, bmWhat*/);
	}
}

void CParticleSystem::renderTransparentObject(CRenderableVisibility *pVisibility, UINT uIndex, UINT uSplitPlanes)
{
	//m_pMaterialSystem->bindVS(m_pVertexShaderHandler);
	CRenderableVisibility::TransparentModel *pMdl = pVisibility->getItemTransparentDynamic(uIndex);
	pMdl->pReferenceMdl->render(m_pMaterialSystem/*, pMdl->uLod*/);
}

void CParticleSystem::setVertexShader(bool useTransform)
{
	m_pMaterialSystem->bindVS(useTransform ? m_pVertexShaderWithTransformHandler : m_pVertexShaderNoTransformHandler);
}

IXSceneObjectType* CParticleSystem::getSceneObjectType()
{
	return(m_pObjectType);
}

IXSceneFeature* CParticleSystem::getFeature(XMODEL_FEATURE bmFeature)
{
	switch(bmFeature)
	{
	case MF_OPAQUE:
		return(m_pFeatureOpaque);
	case MF_TRANSPARENT:
		return(m_pFeatureTransparent);
	case MF_SELFILLUM:
		return(m_pFeatureSelfillum);
	}
	return(NULL);
}

void CParticleSystem::computeVisibility(const IXFrustum *pFrustum, const float3 &vHintDir, CRenderableVisibility *pVisibility, UINT bmLayerMask, CRenderableVisibility *pReference, IXCamera *pCamera)
{
	XPROFILE_FUNCTION();

	TODO("Setup screen size culling");
	/*if(pCamera)
	{
		FIXME("Use actual target width!");
		static const int *r_win_width = GET_PCVAR_INT("r_win_width");
		//static const float *r_default_fov = GET_PCVAR_FLOAT("r_default_fov");
		m_pOpaqueQuery->setScreenSizeCulling(pCamera->getPosition(), pCamera->getEffectiveFOV(), *r_win_width, 4.0f);
	}
	else
	{
		m_pOpaqueQuery->unsetScreenSizeCulling();
	}
	*/

	void **ppData;
	m_pOpaqueQuery->setLayerMask(bmLayerMask);
	UINT uCount = m_pOpaqueQuery->execute(pFrustum, vHintDir, &ppData, pVisibility->getCuller());
	pVisibility->setRenderList(ppData, uCount);

	m_pSelfillumQuery->setLayerMask(bmLayerMask);
	uCount = m_pSelfillumQuery->execute(pFrustum, vHintDir, &ppData, pVisibility->getCuller());
	pVisibility->setSelfillumList(ppData, uCount);

	m_pTransparentQuery->setLayerMask(bmLayerMask);
	uCount = m_pTransparentQuery->execute(pFrustum, vHintDir, &ppData, pVisibility->getCuller());
	//pVisibility->setTransparentList(ppData, uCount);

	{
		//pVisibility->resetItemTransparentDynamic();
		pVisibility->setItemTransparentCountDynamic(uCount);
		//auto &rl = pVisibility->getTransparentList();
		CParticlePlayerEmitter **rl = (CParticlePlayerEmitter**)ppData;
		CParticlePlayerEmitter *pMdl;
		for(UINT i = 0, l = uCount; i < l; ++i)
		{
			pMdl = rl[i];

			//float3 vDelta = pMdl->getPosition();

			IXMaterial *pMaterial;
			pMdl->getMaterial(&pMaterial);
			//UINT uPSPcount = pMdl->getPSPcount(0);
			auto *pItem = pVisibility->getItemTransparentDynamic(i);
			pItem->pReferenceMdl = pMdl;
			pItem->uLod = 0;
			pItem->pMaterial = pMaterial;
			/*if(uPSPcount)
			{
				pItem->hasPSP = true;
				pItem->psp = pMdl->getPSP(0, 0);
			}
			else
			{
				pItem->hasPSP = false;
			}*/
			mem_release(pMaterial);
		}
	}
}

IXMaterialSystem* CParticleSystem::getMaterialSystem()
{
	return(m_pMaterialSystem);
}

void CParticleSystem::onMaterialTransparencyChanged(const IXMaterial *pMaterial)
{
	//ScopedSpinLock lock(m_slModels);

	fora(i, m_aPlayers)
	{
		m_aPlayers[i]->onMaterialTransparencyChanged(pMaterial);
	}
}

void CParticleSystem::onMaterialEmissivityChanged(const IXMaterial *pMaterial)
{
	//ScopedSpinLock lock(m_slModels);

	fora(i, m_aPlayers)
	{
		m_aPlayers[i]->onMaterialEmissivityChanged(pMaterial);
	}
}

//##########################################################################

CMaterialChangedEventListener::CMaterialChangedEventListener(CParticleSystem *pSystem):
	m_pSystem(pSystem)
{}
void CMaterialChangedEventListener::onEvent(const XEventMaterialChanged *pData)
{
	if(pData->type == XEventMaterialChanged::TYPE_TRANSPARENCY)
	{
		m_pSystem->onMaterialTransparencyChanged(pData->pMaterial);
	}
	if(pData->type == XEventMaterialChanged::TYPE_EMISSIVITY)
	{
		m_pSystem->onMaterialEmissivityChanged(pData->pMaterial);
	}
}

//##########################################################################

CUpdateForLoop::CUpdateForLoop(CParticleSystem *pSystem):
	m_pSystem(pSystem)
{
}

void CUpdateForLoop::forLoop(int iStart, int iEnd) const
{
	m_pSystem->updateRange(iStart, iEnd, m_fDelta);
}

void CUpdateForLoop::setDelta(float fDelta)
{
	m_fDelta = fDelta;
}
