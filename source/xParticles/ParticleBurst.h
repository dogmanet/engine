#ifndef __PARTICLE_BURST_H
#define __PARTICLE_BURST_H

#include <xcommon/particles/IXParticleSystem.h>
#include <common/MinMaxCurve.h>


class CParticleBurst final: public IXParticleBurst
{
	friend class CParticlePlayerEmitter;

public:
	XMETHOD_GETSET_IMPL(Time, float, fValue, m_fTime);
	XMETHOD_GETSET_IMPL(Cycles, UINT, uValue, m_uCycles);
	XMETHOD_GETSET_IMPL(Probability, float, fValue, m_fProbability);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getCountCurve, &m_curveCount);
	XMETHOD_2CONST_IMPL(IXMinMaxCurve*, getIntervalCurve, &m_curveInterval);

private:
	float m_fTime = 0.0f;
	UINT m_uCycles = 0;
	float m_fProbability = 1.0f;

	CMinMaxCurve m_curveCount;
	CMinMaxCurve m_curveInterval;
};

#endif
