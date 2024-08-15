#ifndef __PARTICLE_EFFECT_EMITTER_RENDER_DATA_H
#define __PARTICLE_EFFECT_EMITTER_RENDER_DATA_H

#include <xcommon/particles/IXParticleSystem.h>
#include <common/MinMaxCurve.h>
#include <common/string.h>


class CParticleEffectEmitterRenderData final: public IXParticleEffectEmitterRenderData
{
public:
	void XMETHODCALLTYPE setMaterial(const char* szMaterial) override;
	const char* XMETHODCALLTYPE getMaterial() const override;

private:
	String m_sMaterialName = "dev_glare";
};

#endif
