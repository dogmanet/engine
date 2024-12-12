#include "ParticleEffectEmitterLimitVelocityLifetimeData.h"

static float DampenOutsideLimit(float v, float fLimit, float fDampen)
{
	float fAbs = fabsf(v);
	if(fAbs > fLimit)
	{
		fAbs = copysignf(lerpf(fAbs, fLimit, fDampen), v);
	}
	return(fAbs);
}

float3 CParticleEffectEmitterLimitVelocityLifetimeData::evaluateLimit(const float3 &vSpeed, float fLifetimeNormalized, float fRandomValue, const SMQuaternion &qWorldRot)
{
	float fDampen = m_curveDampen.evaluate(fLifetimeNormalized, fRandomValue);

	if(!m_bSeparateAxes)
	{
		float fLimit = m_curveLimitX.evaluate(fLifetimeNormalized, fRandomValue);
		return(SMVector3Normalize(vSpeed) * DampenOutsideLimit(SMVector3Length(vSpeed), fLimit, fDampen));
	}
	else
	{
		float3 vLocalSpeed = vSpeed;
		if(m_simulationSpace == XPSS_LOCAL)
		{
			vLocalSpeed = qWorldRot.Conjugate() * vLocalSpeed;
		}

		vLocalSpeed = float3(
			DampenOutsideLimit(vLocalSpeed.x, m_curveLimitX.evaluate(fLifetimeNormalized, fRandomValue), fDampen),
			DampenOutsideLimit(vLocalSpeed.y, m_curveLimitY.evaluate(fLifetimeNormalized, fRandomValue), fDampen),
			DampenOutsideLimit(vLocalSpeed.z, m_curveLimitZ.evaluate(fLifetimeNormalized, fRandomValue), fDampen)
		);

		if(m_simulationSpace == XPSS_LOCAL)
		{
			vLocalSpeed = qWorldRot * vLocalSpeed;
		}

		return(vLocalSpeed);
	}
}
