#ifndef __PARTICLE_EFFECT_EMITTER_LIFETIME_EMITTER_SPEED_DATA_H
#define __PARTICLE_EFFECT_EMITTER_LIFETIME_EMITTER_SPEED_DATA_H

#include <xcommon/particles/IXParticleSystem.h>
#include <common/MinMaxCurve.h>


class CParticleEffectEmitterLifetimeEmitterSpeedData final: public IXParticleEffectEmitterLifetimeEmitterSpeedData
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

	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getMultiplierCurve, &m_curveMultiplier);
	XMETHOD_GETSET_REF_IMPL(SpeedRange, float2_t, vRange, m_vRange);

	float evaluate(float fSpeed, float fRandom);

private:
	bool m_isEnabled = false;

	CMinMaxCurve m_curveMultiplier = 1.0f;

	float2_t m_vRange = float2_t(0.0f, 1.0f);
};

#endif
