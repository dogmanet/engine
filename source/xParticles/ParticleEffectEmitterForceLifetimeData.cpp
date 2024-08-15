#include "ParticleEffectEmitterForceLifetimeData.h"

float3_t CParticleEffectEmitterForceLifetimeData::evaluateForce(float fLifetimeNormalized, float fRandomValue, const SMQuaternion &qWorldRot)
{
	if(m_bRandomize)
	{
		fRandomValue = randf(0.0f, 1.0f);
	}
	float3_t vForce(
		m_curveForceX.evaluate(fLifetimeNormalized, fRandomValue),
		m_curveForceY.evaluate(fLifetimeNormalized, fRandomValue),
		m_curveForceZ.evaluate(fLifetimeNormalized, fRandomValue)
	);

	if(m_simulationSpace == XPSS_LOCAL)
	{
		vForce = qWorldRot * vForce;
	}

	return(vForce);
}
