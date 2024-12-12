#ifndef __PARTICLE_EFFECT_EMITTER_COLOR_LIFETIME_DATA_H
#define __PARTICLE_EFFECT_EMITTER_COLOR_LIFETIME_DATA_H

#include <xcommon/particles/IXParticleSystem.h>
#include <common/2ColorGradients.h>


class CParticleEffectEmitterColorLifetimeData final: public IXParticleEffectEmitterColorLifetimeData
{
	friend class CParticlePlayerEmitter;
public:
	bool XMETHODCALLTYPE isEnabled() const override
	{
		return(m_isEnabled);
	}
	void XMETHODCALLTYPE enable(bool yesNo) override
	{
		m_isEnabled = yesNo;
	}

	XMETHOD_2CONST_IMPL(IX2ColorGradients*, getColor, &m_gradColor);

private:
	bool m_isEnabled = false;

	C2ColorGradients m_gradColor = float4_t(1.0f, 1.0f, 1.0f, 1.0f);
};

#endif
