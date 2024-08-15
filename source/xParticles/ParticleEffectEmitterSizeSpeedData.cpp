#include "ParticleEffectEmitterSizeSpeedData.h"

float3_t CParticleEffectEmitterSizeSpeedData::evaluate(float fSpeed, float fRandom)
{
	float fPos;
	if(fSpeed <= m_vRange.x)
	{
		fPos = 0.0f;
	}
	else if(fSpeed >= m_vRange.y)
	{
		fPos = 1.0f;
	}
	else
	{
		fPos = (fSpeed - m_vRange.x) / (m_vRange.y - m_vRange.x);
	}

	float3_t vSizeCoeff = m_curveSizeX.evaluate(fPos, fRandom);
	if(m_bSeparateAxes)
	{
		vSizeCoeff.y = m_curveSizeY.evaluate(fPos, fRandom);
		vSizeCoeff.z = m_curveSizeZ.evaluate(fPos, fRandom);
	}

	return(vSizeCoeff);
}
