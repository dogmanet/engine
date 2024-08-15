#ifndef __PARTICLE_EFFECT_EMITTER_LIMIT_VELOCITY_LIFETIME_DATA_H
#define __PARTICLE_EFFECT_EMITTER_LIMIT_VELOCITY_LIFETIME_DATA_H

#include <xcommon/particles/IXParticleSystem.h>
#include <common/MinMaxCurve.h>


class CParticleEffectEmitterLimitVelocityLifetimeData final: public IXParticleEffectEmitterLimitVelocityLifetimeData
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

	XMETHOD_GETSET_IMPL(SeparateAxes, bool, yesNo, m_bSeparateAxes);
	XMETHOD_GETSET_IMPL(SimulationSpace, XPARTICLE_SIMULATION_SPACE, simulationSpace, m_simulationSpace);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getLimitCurve, &m_curveLimitX);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getLimitXCurve, &m_curveLimitX);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getLimitYCurve, &m_curveLimitY);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getLimitZCurve, &m_curveLimitZ);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getDampenCurve, &m_curveDampen);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getDragCurve, &m_curveDrag);
	XMETHOD_GETSET_IMPL(MultiplyBySize, bool, yesNo, m_bMultiplyBySize);
	XMETHOD_GETSET_IMPL(MultiplyByVelocity, bool, yesNo, m_bMultiplyByVelocity);

	float3 evaluateLimit(const float3 &vSpeed, float fLifetimeNormalized, float fRandomValue, const SMQuaternion &qWorldRot);

private:
	bool m_isEnabled = false;

	bool m_bSeparateAxes = false;
	XPARTICLE_SIMULATION_SPACE m_simulationSpace = XPSS_LOCAL;

	CMinMaxCurve m_curveLimitX;
	CMinMaxCurve m_curveLimitY;
	CMinMaxCurve m_curveLimitZ;
	CMinMaxCurve m_curveDampen = 0.0f;
	CMinMaxCurve m_curveDrag = 0.0f;

	bool m_bMultiplyBySize = false;
	bool m_bMultiplyByVelocity = false;
};

#endif
