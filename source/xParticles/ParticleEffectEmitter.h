#ifndef __PARTICLEEFFECTEMITTER_H
#define __PARTICLEEFFECTEMITTER_H

#include <common/string.h>
#include <xcommon/particles/IXParticleSystem.h>
#include "ParticleEffectEmitterGenericData.h"
#include "ParticleEffectEmitterEmissionData.h"
#include "ParticleEffectEmitterShapeData.h"
#include "ParticleEffectEmitterVelocityLifetimeData.h"
#include "ParticleEffectEmitterLimitVelocityLifetimeData.h"
#include "ParticleEffectEmitterForceLifetimeData.h"
#include "ParticleEffectEmitterSizeLifetimeData.h"
#include "ParticleEffectEmitterSizeSpeedData.h"
#include "ParticleEffectEmitterRotationLifetimeData.h"
#include "ParticleEffectEmitterRotationSpeedData.h"
#include "ParticleEffectEmitterLifetimeEmitterSpeedData.h"
#include "ParticleEffectEmitterColorLifetimeData.h"
#include "ParticleEffectEmitterColorSpeedData.h"
#include "ParticleEffectEmitterRenderData.h"

class CParticleSystem;
class CParticleEffectEmitter final: public IXUnknownImplementation<IXParticleEffectEmitter>
{
	friend class CParticlePlayerEmitter;
public:
	CParticleEffectEmitter(CParticleSystem *pSystem);

	const char* XMETHODCALLTYPE getName() const override;
	void XMETHODCALLTYPE setName(const char *szName) override;

	XMETHOD_GETSET_REF_IMPL(Pos, float3_t, vPos, m_vPos);
	XMETHOD_GETSET_REF_IMPL(Orient, SMQuaternion, qOrient, m_qOrient);

	XMETHOD_2CONST_IMPL(IXParticleEffectEmitterGenericData*, getGenericData, &m_dataGeneric);
	XMETHOD_2CONST_IMPL(IXParticleEffectEmitterEmissionData*, getEmissionData, &m_dataEmission);
	XMETHOD_2CONST_IMPL(IXParticleEffectEmitterShapeData*, getShapeData, &m_dataShape);
	XMETHOD_2CONST_IMPL(IXParticleEffectEmitterVelocityLifetimeData*, getVelocityLifetimeData, &m_dataVelocityLifetime);
	XMETHOD_2CONST_IMPL(IXParticleEffectEmitterLimitVelocityLifetimeData*, getLimitVelocityLifetimeData, &m_dataLimitVelocityLifetime);
	XMETHOD_2CONST_IMPL(IXParticleEffectEmitterForceLifetimeData*, getForceLifetimeData, &m_dataForceLifetime);
	XMETHOD_2CONST_IMPL(IXParticleEffectEmitterSizeLifetimeData*, getSizeLifetimeData, &m_dataSizeLifetime);
	XMETHOD_2CONST_IMPL(IXParticleEffectEmitterSizeSpeedData*, getSizeSpeedData, &m_dataSizeSpeed);
	XMETHOD_2CONST_IMPL(IXParticleEffectEmitterRotationLifetimeData*, getRotationLifetimeData, &m_dataRotationLifetime);
	XMETHOD_2CONST_IMPL(IXParticleEffectEmitterRotationSpeedData*, getRotationSpeedData, &m_dataRotationSpeed);
	XMETHOD_2CONST_IMPL(IXParticleEffectEmitterLifetimeEmitterSpeedData*, getLifetimeEmitterSpeedData, &m_dataLifetimeEmitterSpeed);
	XMETHOD_2CONST_IMPL(IXParticleEffectEmitterColorLifetimeData*, getColorLifetimeData, &m_dataColorLifetime);
	XMETHOD_2CONST_IMPL(IXParticleEffectEmitterColorSpeedData*, getColorSpeedData, &m_dataColorSpeed);
	XMETHOD_2CONST_IMPL(IXParticleEffectEmitterRenderData*, getRenderData, &m_dataRender);

private:
	void XMETHODCALLTYPE FinalRelease() override;

private:
	CParticleSystem *m_pSystem;

	String m_sName;

	float3_t m_vPos;
	SMQuaternion m_qOrient;

	CParticleEffectEmitterGenericData m_dataGeneric;
	CParticleEffectEmitterEmissionData m_dataEmission;
	CParticleEffectEmitterShapeData m_dataShape;
	CParticleEffectEmitterVelocityLifetimeData m_dataVelocityLifetime;
	CParticleEffectEmitterLimitVelocityLifetimeData m_dataLimitVelocityLifetime;
	CParticleEffectEmitterForceLifetimeData m_dataForceLifetime;
	CParticleEffectEmitterSizeLifetimeData m_dataSizeLifetime;
	CParticleEffectEmitterSizeSpeedData m_dataSizeSpeed;
	CParticleEffectEmitterRotationLifetimeData m_dataRotationLifetime;
	CParticleEffectEmitterRotationSpeedData m_dataRotationSpeed;
	CParticleEffectEmitterLifetimeEmitterSpeedData m_dataLifetimeEmitterSpeed;
	CParticleEffectEmitterColorLifetimeData m_dataColorLifetime;
	CParticleEffectEmitterColorSpeedData m_dataColorSpeed;
	CParticleEffectEmitterRenderData m_dataRender;
};

#endif
