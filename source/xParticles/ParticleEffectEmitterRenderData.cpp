#include "ParticleEffectEmitterRenderData.h"

void XMETHODCALLTYPE CParticleEffectEmitterRenderData::setMaterial(const char* szMaterial)
{
	m_sMaterialName = szMaterial;
}
const char* XMETHODCALLTYPE CParticleEffectEmitterRenderData::getMaterial() const
{
	return(m_sMaterialName.c_str());
}
