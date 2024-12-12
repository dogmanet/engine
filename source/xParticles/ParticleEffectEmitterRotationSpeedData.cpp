#include "ParticleEffectEmitterRotationSpeedData.h"

SMQuaternion CParticleEffectEmitterRotationSpeedData::evaluate(float fSpeed, float fRandom, float fDt)
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

	SMQuaternion qDeltaRot;
	if(m_bSeparateAxes)
	{
		float fSpinX = m_curveAngularVelocityX.evaluate(fPos, fRandom);
		qDeltaRot = SMQuaternion(SMToRadian(fSpinX * fDt), 'x');
		float fSpinY = m_curveAngularVelocityY.evaluate(fPos, fRandom);
		qDeltaRot = qDeltaRot * SMQuaternion(SMToRadian(fSpinY * fDt), 'y');
	}
	float fSpinZ = m_curveAngularVelocityZ.evaluate(fPos, fRandom);
	qDeltaRot = qDeltaRot * SMQuaternion(SMToRadian(fSpinZ * fDt), 'z');
	
	return(qDeltaRot);
}
