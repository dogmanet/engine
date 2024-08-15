#include "ParticleEffectEmitterRotationLifetimeData.h"

SMQuaternion CParticleEffectEmitterRotationLifetimeData::evaluate(float fLifetimeFrac, float fRandom, float fDt)
{
	SMQuaternion qDeltaRot;
	if(m_bSeparateAxes)
	{
		float fSpinX = m_curveAngularVelocityX.evaluate(fLifetimeFrac, fRandom);
		qDeltaRot = SMQuaternion(SMToRadian(fSpinX * fDt), 'x');
		float fSpinY = m_curveAngularVelocityY.evaluate(fLifetimeFrac, fRandom);
		qDeltaRot = qDeltaRot * SMQuaternion(SMToRadian(fSpinY * fDt), 'y');
	}
	float fSpinZ = m_curveAngularVelocityZ.evaluate(fLifetimeFrac, fRandom);
	qDeltaRot = qDeltaRot * SMQuaternion(SMToRadian(fSpinZ * fDt), 'z');
	
	return(qDeltaRot);
}
