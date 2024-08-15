#ifndef __PARTICLE_EFFECT_EMITTER_FORCE_LIFETIME_DATA_H
#define __PARTICLE_EFFECT_EMITTER_FORCE_LIFETIME_DATA_H

#include <xcommon/particles/IXParticleSystem.h>
#include <common/MinMaxCurve.h>


class CParticleEffectEmitterForceLifetimeData final: public IXParticleEffectEmitterForceLifetimeData
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

	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getForceXCurve, &m_curveForceX);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getForceYCurve, &m_curveForceY);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getForceZCurve, &m_curveForceZ);
	XMETHOD_GETSET_IMPL(SimulationSpace, XPARTICLE_SIMULATION_SPACE, simulationSpace, m_simulationSpace);
	XMETHOD_GETSET_IMPL(Randomize, bool, yesNo, m_bRandomize);

	float3_t evaluateForce(float fLifetimeNormalized, float fRandomValue, const SMQuaternion &qWorldRot);
private:
	bool m_isEnabled = false;

	bool m_bRandomize = false;
	XPARTICLE_SIMULATION_SPACE m_simulationSpace = XPSS_LOCAL;

	CMinMaxCurve m_curveForceX = 0.0f;
	CMinMaxCurve m_curveForceY = 0.0f;
	CMinMaxCurve m_curveForceZ = 0.0f;
};

#endif
