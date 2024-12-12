#ifndef __PARTICLE_EFFECT_EMITTER_GENERIC_DATA_H
#define __PARTICLE_EFFECT_EMITTER_GENERIC_DATA_H

#include <xcommon/particles/IXParticleSystem.h>
#include <common/MinMaxCurve.h>
#include <common/2ColorGradients.h>


class CParticleEffectEmitterGenericData final: public IXParticleEffectEmitterGenericData
{
	friend class CParticlePlayerEmitter;

public:
	XMETHOD_GETSET_IMPL(Duration, float, fValue, m_fDuration);
	XMETHOD_GETSET_IMPL(Looping, bool, yesNo, m_isLooping);
	XMETHOD_GETSET_IMPL(Prewarm, bool, yesNo, m_bPrewarm);
	XMETHOD_GETSET_IMPL(StartDelay, float, fValue, m_fStartDelay);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getStartLifetimeCurve, &m_curveStartLifetime);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getStartSpeedCurve, &m_curveStartSpeed);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getStartSizeCurve, &m_curveStartSizeX);
	XMETHOD_GETSET_IMPL(FlipRotation, float, fValue, m_fFlipRotation);
	XMETHOD_GETSET_IMPL(CullingMode, XPARTICLE_CULLING_MODE, mode, m_cullingMode);
	XMETHOD_GETSET_IMPL(GravityModifier, float, fValue, m_fGravityModifier);
	XMETHOD_GETSET_IMPL(MaxParticles, UINT, uCount, m_uMaxParticles);
	XMETHOD_GETSET_IMPL(RingBufferMode, XPARTICLE_RING_BUFFER_MODE, mode, m_ringBufferMode);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getRingBufferLoopRangeCurve, &m_curveRingBufferLoopRange);
	XMETHOD_GETSET_IMPL(SimulationSpace, XPARTICLE_SIMULATION_SPACE, simulationSpace, m_simulationSpace);
	XMETHOD_2CONST_IMPL(IX2ColorGradients*, getStartColorGradient, &m_gradStartColor);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getStartRotationCurve, &m_curveStartRotationX);
	XMETHOD_GETSET_IMPL(StartRotationSeparate, bool, yesNo, m_bStartRotationSeparate);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getStartRotationXCurve, &m_curveStartRotationX);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getStartRotationYCurve, &m_curveStartRotationY);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getStartRotationZCurve, &m_curveStartRotationZ);
	XMETHOD_GETSET_IMPL(StartSizeSeparate, bool, yesNo, m_bStartSizeSeparate);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getStartSizeXCurve, &m_curveStartSizeX);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getStartSizeYCurve, &m_curveStartSizeY);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getStartSizeZCurve, &m_curveStartSizeZ);

private:
	float m_fDuration = 1.0f;
	bool m_isLooping = true;
	bool m_bPrewarm = false;
	float m_fStartDelay = 0.0f;
	float m_fFlipRotation = 0.0f;
	XPARTICLE_CULLING_MODE m_cullingMode = XPCM_AUTO;
	float m_fGravityModifier = 1.0f;
	UINT m_uMaxParticles = 1000;
	XPARTICLE_RING_BUFFER_MODE m_ringBufferMode = XPRBM_DISABLED;
	XPARTICLE_SIMULATION_SPACE m_simulationSpace = XPSS_LOCAL;
	bool m_bStartRotationSeparate = false;
	bool m_bStartSizeSeparate = false;
	C2ColorGradients m_gradStartColor = float4_t(1.0f, 1.0f, 1.0f, 1.0f);

	CMinMaxCurve m_curveStartLifetime;
	CMinMaxCurve m_curveStartSpeed;
	CMinMaxCurve m_curveStartSizeX;
	CMinMaxCurve m_curveStartSizeY;
	CMinMaxCurve m_curveStartSizeZ;
	CMinMaxCurve m_curveRingBufferLoopRange;
	CMinMaxCurve m_curveStartRotationX;
	CMinMaxCurve m_curveStartRotationY;
	CMinMaxCurve m_curveStartRotationZ;
};

#endif
