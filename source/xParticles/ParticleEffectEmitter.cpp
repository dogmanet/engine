#include "ParticleEffectEmitter.h"
#include "ParticleSystem.h"

CParticleEffectEmitter::CParticleEffectEmitter(CParticleSystem *pSystem):
	m_pSystem(pSystem),
	m_sName("New emitter")
{
}

TODO("Collisions");
TODO("Sub emitters");
TODO("Lights");
TODO("Trails?");

const char* XMETHODCALLTYPE CParticleEffectEmitter::getName() const
{
	return(m_sName.c_str());
}
void XMETHODCALLTYPE CParticleEffectEmitter::setName(const char *szName)
{
	m_sName = szName;
}

void XMETHODCALLTYPE CParticleEffectEmitter::FinalRelease()
{
	m_pSystem->onEmitterReleased(this);
}
