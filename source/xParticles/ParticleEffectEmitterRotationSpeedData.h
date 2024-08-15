#ifndef __PARTICLE_EFFECT_EMITTER_ROTATION_SPEED_DATA_H
#define __PARTICLE_EFFECT_EMITTER_ROTATION_SPEED_DATA_H

#include <xcommon/particles/IXParticleSystem.h>
#include <common/MinMaxCurve.h>


class CParticleEffectEmitterRotationSpeedData final: public IXParticleEffectEmitterRotationSpeedData
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
	XMETHOD_GETSET_REF_IMPL(SpeedRange, float2_t, vRange, m_vRange);

	SMQuaternion evaluate(float fSpeed, float fRandom, float fDt);

private:
	bool m_isEnabled = false;

	bool m_bSeparateAxes = false;

	CMinMaxCurve m_curveAngularVelocityX = 0.0f;
	CMinMaxCurve m_curveAngularVelocityY = 0.0f;
	CMinMaxCurve m_curveAngularVelocityZ = 45.0f;

	float2_t m_vRange = float2_t(0.0f, 1.0f);
};

#endif
