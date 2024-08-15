#include "ParticleEffectEmitterVelocityLifetimeData.h"

float3_t CParticleEffectEmitterVelocityLifetimeData::evaluateLinearSpeed(float fLifetimeNormalized, float fRandomValue, const SMQuaternion &qWorldRot)
{
	float3_t vSpeed = float3_t(
		m_curveLinearX.evaluate(fLifetimeNormalized, fRandomValue),
		m_curveLinearY.evaluate(fLifetimeNormalized, fRandomValue),
		m_curveLinearZ.evaluate(fLifetimeNormalized, fRandomValue)
	);

	if(m_simulationSpace == XPSS_LOCAL)
	{
		vSpeed = qWorldRot * vSpeed;
	}

	return(vSpeed);
}

float CParticleEffectEmitterVelocityLifetimeData::evaluateSpeedMultiplier(float fLifetimeNormalized, float fRandomValue)
{
	return(m_curveSpeedModifier.evaluate(fLifetimeNormalized, fRandomValue));
}
