#ifndef __PARTICLE_EFFECT_EMITTER_SHAPE_DATA_H
#define __PARTICLE_EFFECT_EMITTER_SHAPE_DATA_H

#include <xcommon/particles/IXParticleSystem.h>
#include <common/MinMaxCurve.h>


class CParticleEffectEmitterShapeData final: public IXParticleEffectEmitterShapeData
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

	XMETHOD_GETSET_IMPL(Shape, XPARTICLE_SHAPE, shape, m_shape);
	XMETHOD_GETSET_IMPL(Radius, float, fValue, m_fRadius);
	XMETHOD_GETSET_IMPL(RadiusThickness, float, fValue, m_fRadiusThickness);
	XMETHOD_GETSET_IMPL(Angle, float, fValue, m_fAngle);
	XMETHOD_GETSET_IMPL(Arc, float, fValue, m_fArc);
	XMETHOD_GETSET_IMPL(ArcMode, XPARTICLE_SHAPE_ARC_MODE, mode, m_arcMode);
	XMETHOD_GETSET_IMPL(ArcSpread, float, fValue, m_fArcSpread);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getArcSpeedCurve, &m_curveArcSpeed);
	XMETHOD_GETSET_IMPL(Length, float, fValue, m_fLength);
	XMETHOD_GETSET_IMPL(ConeEmitFrom, XPARTICLE_SHAPE_CONE_EMIT_FROM, emitFrom, m_coneEmitFrom);
	XMETHOD_GETSET_IMPL(BoxEmitFrom, XPARTICLE_SHAPE_BOX_EMIT_FROM, emitFrom, m_boxEmitFrom);
	XMETHOD_GETSET_IMPL(DonutRadius, float, fValue, m_fDonutRadius);
	XMETHOD_GETSET_IMPL(AlignToDirection, bool, yesNo, m_bAlignToDirection);
	XMETHOD_GETSET_IMPL(RandomizeDirection, float, fValue, m_fRandomizeDirection);
	XMETHOD_GETSET_IMPL(SpherizeDirection, float, fValue, m_fSpherizeDirection);
	XMETHOD_GETSET_IMPL(RandomizePosition, float, fValue, m_fRandomizePosition);
	XMETHOD_GETSET_REF_IMPL(Size, float3_t, vSize, m_vSize);

	void evaluate(float fTime, float3 *pvPos, float3 *pvDir, UINT uCountInGen = 1, UINT uIdInGen = 0);

private:
	void evalSphere(float fRadius, float fRadiusThickness, bool isSemi, float3 *pvPos, float3 *pvDir);

	float evalArc(float fMaxAngle, float fTime, UINT uCountInGen, UINT uIdInGen);

private:
	bool m_isEnabled = false;

	XPARTICLE_SHAPE m_shape = XPS_SPHERE;
	float m_fRadius = 1.0f;
	float m_fRadiusThickness = 1.0f;
	float m_fAngle = 45.0f;
	float m_fArc = 1.0f;
	XPARTICLE_SHAPE_ARC_MODE m_arcMode = XPSAM_RANDOM;
	float m_fArcSpread = 0.0f;
	CMinMaxCurve m_curveArcSpeed;
	float m_fLength = 0.0f;
	XPARTICLE_SHAPE_CONE_EMIT_FROM m_coneEmitFrom = XPSCEF_BASE;
	XPARTICLE_SHAPE_BOX_EMIT_FROM m_boxEmitFrom = XPSBEF_EDGE;
	float m_fDonutRadius = 0.0f;
	bool m_bAlignToDirection = false;
	float m_fRandomizeDirection = 0.0f;
	float m_fSpherizeDirection = 0.0f;
	float m_fRandomizePosition = 0.0f;
	float3_t m_vSize = 1.0f;
};

#endif
