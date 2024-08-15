#include "ParticleEffect.h"
#include "ParticleSystem.h"

CParticleEffect::CParticleEffect(CParticleSystem *pSystem):
	m_pSystem(pSystem)
{
}
CParticleEffect::~CParticleEffect()
{
	fora(i, m_apEmitters)
	{
		mem_release(m_apEmitters[i]);
	}
}

UINT XMETHODCALLTYPE CParticleEffect::getEmitterCount()
{
	return(m_apEmitters.size());
}
void XMETHODCALLTYPE CParticleEffect::setEmitterCount(UINT uCount)
{
	for(UINT i = uCount, l = m_apEmitters.size(); i < l; ++i)
	{
		mem_release(m_apEmitters[i]);
	}
	m_apEmitters.reserve(uCount);
	for(UINT i = m_apEmitters.size(); i < uCount; ++i)
	{
		m_apEmitters[i] = m_pSystem->allocEmitter();
	}
	m_apEmitters.resizeFast(uCount);
}
IXParticleEffectEmitter* XMETHODCALLTYPE CParticleEffect::getEmitterAt(UINT uIdx)
{
	return(getEmitterAtInternal(uIdx));
}
void XMETHODCALLTYPE CParticleEffect::removeEmitterAt(UINT uIdx)
{
	assert(m_apEmitters.size() > uIdx);

	if(m_apEmitters.size() > uIdx)
	{
		mem_release(m_apEmitters[uIdx]);
		m_apEmitters.erase(uIdx);
	}
}

CParticleEffectEmitter* CParticleEffect::getEmitterAtInternal(UINT uIdx)
{
	assert(m_apEmitters.size() > uIdx);

	if(m_apEmitters.size() > uIdx)
	{
		return(m_apEmitters[uIdx]);
	}

	return(NULL);
}

bool XMETHODCALLTYPE CParticleEffect::save()
{
	return(m_pSystem->saveEffect(this));
}

void CParticleEffect::setName(const char *szName)
{
	m_szName = szName;
}

const char* CParticleEffect::getName()
{
	return(m_szName);
}

void XMETHODCALLTYPE CParticleEffect::FinalRelease()
{
	m_pSystem->onEffectReleased(m_szName);
}
