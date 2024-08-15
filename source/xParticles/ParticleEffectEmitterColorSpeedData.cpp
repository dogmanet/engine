#include "ParticleEffectEmitterColorSpeedData.h"

float4_t CParticleEffectEmitterColorSpeedData::evaluate(float fSpeed, float fRandom)
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

	return(m_gradColor.evaluate(fPos, fRandom));
}
