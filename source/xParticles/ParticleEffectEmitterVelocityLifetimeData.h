#ifndef __PARTICLE_EFFECT_EMITTER_VELOCITY_LIFETIME_DATA_H
#define __PARTICLE_EFFECT_EMITTER_VELOCITY_LIFETIME_DATA_H

#include <xcommon/particles/IXParticleSystem.h>
#include <common/MinMaxCurve.h>


class CParticleEffectEmitterVelocityLifetimeData final: public IXParticleEffectEmitterVelocityLifetimeData
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

	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getLinearXCurve, &m_curveLinearX);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getLinearYCurve, &m_curveLinearY);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getLinearZCurve, &m_curveLinearZ);

	XMETHOD_GETSET_IMPL(SimulationSpace, XPARTICLE_SIMULATION_SPACE, simulationSpace, m_simulationSpace);

	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getOrbitalXCurve, &m_curveOrbitalX);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getOrbitalYCurve, &m_curveOrbitalY);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getOrbitalZCurve, &m_curveOrbitalZ);
	
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getOffsetXCurve, &m_curveOffsetX);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getOffsetYCurve, &m_curveOffsetY);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getOffsetZCurve, &m_curveOffsetZ);

	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getRadialCurve, &m_curveRadial);

	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getSpeedModifierCurve, &m_curveSpeedModifier);

	float3_t evaluateLinearSpeed(float fLifetimeNormalized, float fRandomValue, const SMQuaternion &qWorldRot);
	float evaluateSpeedMultiplier(float fLifetimeNormalized, float fRandomValue);

private:
	bool m_isEnabled = false;

	CMinMaxCurve m_curveLinearX = 0.0f;
	CMinMaxCurve m_curveLinearY = 0.0f;
	CMinMaxCurve m_curveLinearZ = 0.0f;

	XPARTICLE_SIMULATION_SPACE m_simulationSpace = XPSS_LOCAL;

	CMinMaxCurve m_curveOrbitalX = 0.0f;
	CMinMaxCurve m_curveOrbitalY = 0.0f;
	CMinMaxCurve m_curveOrbitalZ = 0.0f;

	CMinMaxCurve m_curveOffsetX = 0.0f;
	CMinMaxCurve m_curveOffsetY = 0.0f;
	CMinMaxCurve m_curveOffsetZ = 0.0f;

	CMinMaxCurve m_curveRadial = 0.0f;

	CMinMaxCurve m_curveSpeedModifier = 1.0f;

};

#endif
