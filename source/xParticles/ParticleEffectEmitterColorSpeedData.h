#ifndef __PARTICLE_EFFECT_EMITTER_COLOR_SPEED_DATA_H
#define __PARTICLE_EFFECT_EMITTER_COLOR_SPEED_DATA_H

#include <xcommon/particles/IXParticleSystem.h>
#include <common/2ColorGradients.h>


class CParticleEffectEmitterColorSpeedData final: public IXParticleEffectEmitterColorSpeedData
{
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
	XMETHOD_GETSET_REF_IMPL(SpeedRange, float2_t, vRange, m_vRange);

	float4_t evaluate(float fSpeed, float fRandom);

private:
	bool m_isEnabled = false;

	C2ColorGradients m_gradColor = float4_t(1.0f, 1.0f, 1.0f, 1.0f);

	float2_t m_vRange = float2_t(0.0f, 1.0f);
};

#endif
