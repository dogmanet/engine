#ifndef __PARTICLE_EFFECT_EMITTER_SIZE_LIFETIME_DATA_H
#define __PARTICLE_EFFECT_EMITTER_SIZE_LIFETIME_DATA_H

#include <xcommon/particles/IXParticleSystem.h>
#include <common/MinMaxCurve.h>


class CParticleEffectEmitterSizeLifetimeData final: public IXParticleEffectEmitterSizeLifetimeData
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
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getSizeCurve, &m_curveSizeX);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getSizeXCurve, &m_curveSizeX);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getSizeYCurve, &m_curveSizeY);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getSizeZCurve, &m_curveSizeZ);

private:
	bool m_isEnabled = false;

	bool m_bSeparateAxes = false;

	CMinMaxCurve m_curveSizeX = XMCM_CURVE;
	CMinMaxCurve m_curveSizeY = XMCM_CURVE;
	CMinMaxCurve m_curveSizeZ = XMCM_CURVE;
};

#endif
