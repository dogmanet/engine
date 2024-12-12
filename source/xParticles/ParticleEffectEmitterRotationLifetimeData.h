#ifndef __PARTICLE_EFFECT_EMITTER_ROTATION_LIFETIME_DATA_H
#define __PARTICLE_EFFECT_EMITTER_ROTATION_LIFETIME_DATA_H

#include <xcommon/particles/IXParticleSystem.h>
#include <common/MinMaxCurve.h>


class CParticleEffectEmitterRotationLifetimeData final: public IXParticleEffectEmitterRotationLifetimeData
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

	XMETHOD_GETSET_IMPL(SeparateAxes, bool, yesNo, m_bSeparateAxes);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getAngularVelocityCurve, &m_curveAngularVelocityZ);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getAngularVelocityXCurve, &m_curveAngularVelocityX);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getAngularVelocityYCurve, &m_curveAngularVelocityY);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getAngularVelocityZCurve, &m_curveAngularVelocityZ);

	SMQuaternion evaluate(float fLifetimeFrac, float fRandom, float fDt);

private:
	bool m_isEnabled = false;

	bool m_bSeparateAxes = false;

	CMinMaxCurve m_curveAngularVelocityX = 0.0f;
	CMinMaxCurve m_curveAngularVelocityY = 0.0f;
	CMinMaxCurve m_curveAngularVelocityZ = 45.0f;
};

#endif
