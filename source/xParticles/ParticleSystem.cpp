#include "ParticleSystem.h"
#include "EffectLoader.h"

CParticleSystem::CParticleSystem(IXCore *pCore):
	m_pCore(pCore)
{
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

void CParticleSystem::update(float fDelta)
{
	fora(i, m_aPlayers)
	{
		m_aPlayers[i]->update(fDelta);
	}
}

void CParticleSystem::sync()
{

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

CParticleEffectEmitter* CParticleSystem::allocEmitter()
{
	ScopedSpinLock lock(m_slEmittersPool);
	return(m_poolEmitters.Alloc(this));
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
	m_pVertexShaderHandler = pMaterialSystem->registerVertexShader(pFormat, "base/particles.vs");
}

void CParticleSystem::newVisData(IXRenderableVisibility **ppVisibility)
{
	*ppVisibility = NULL;
}
void CParticleSystem::render(IXRenderableVisibility *pVisibility)
{
	TODO("Use pVisibility");

	if(m_pMaterialSystem)
	{
		static IXMaterial *s_pMat = NULL;
		if(!s_pMat)
		{
			m_pMaterialSystem->loadMaterial("dev_white", &s_pMat);
		}

		m_pMaterialSystem->bindVS(m_pVertexShaderHandler);

		m_pMaterialSystem->bindMaterial(s_pMat);

		fora(i, m_aPlayers)
		{
			m_aPlayers[i]->render();
		}
	}
}
