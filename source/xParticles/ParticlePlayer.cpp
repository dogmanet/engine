#include "ParticlePlayer.h"
#include "ParticleSystem.h"

CParticlePlayer::CParticlePlayer(CParticleEffect *pEffect, CParticleSystem *pSystem):
	m_pEffect(pEffect),
	m_pSystem(pSystem)
{
	add_ref(pEffect);
}
CParticlePlayer::~CParticlePlayer()
{
	mem_release(m_pEffect);
}

void XMETHODCALLTYPE CParticlePlayer::play()
{
}
void XMETHODCALLTYPE CParticlePlayer::pause()
{
}
void XMETHODCALLTYPE CParticlePlayer::stop(bool bClear)
{
}
void XMETHODCALLTYPE CParticlePlayer::clear()
{
}

void XMETHODCALLTYPE CParticlePlayer::simulate(float fTime, bool bRestart)
{
}

bool XMETHODCALLTYPE CParticlePlayer::isEmitting()
{
	assert(!"Not implemented");
	return(false);
}
bool XMETHODCALLTYPE CParticlePlayer::isPaused()
{
	assert(!"Not implemented");
	return(false);
}
bool XMETHODCALLTYPE CParticlePlayer::isPlaying()
{
	assert(!"Not implemented");
	return(false);
}
bool XMETHODCALLTYPE CParticlePlayer::isStopped()
{
	assert(!"Not implemented");
	return(false);
}
bool XMETHODCALLTYPE CParticlePlayer::isAlive()
{
	assert(!"Not implemented");
	return(false);
}

UINT XMETHODCALLTYPE CParticlePlayer::getParticleCount()
{
	assert(!"Not implemented");
	return(0);
}
float XMETHODCALLTYPE CParticlePlayer::getTime()
{
	assert(!"Not implemented");
	return(0.0f);
}

float3_t XMETHODCALLTYPE CParticlePlayer::getPos()
{
	assert(!"Not implemented");
	return(0.0f);
}
void XMETHODCALLTYPE CParticlePlayer::setPos(const float3_t &vPos)
{
}

SMQuaternion XMETHODCALLTYPE CParticlePlayer::getOrient()
{
	assert(!"Not implemented");
	return(SMQuaternion());
}
void XMETHODCALLTYPE CParticlePlayer::setOrient(const SMQuaternion &qRot)
{
}

void CParticlePlayer::update(float fDelta)
{
	UINT uEmCount = m_pEffect->getEmitterCount();
	if(uEmCount != m_aEmitters.size())
	{
		UINT i = m_aEmitters.size();
		m_aEmitters.resize(uEmCount);

		for(; i < uEmCount; ++i)
		{
			m_aEmitters[i].setData(m_pEffect->getEmitterAtInternal(i));

			if(m_pDevice)
			{
				m_aEmitters[i].setDevice(m_pDevice);
			}
		}
	}

	for(UINT i = 0; i < uEmCount; ++i)
	{
		m_aEmitters[i].update(fDelta);
	}
}

float CParticlePlayer::getDeltaPos()
{
	//assert(!"Not implemented");
	return(0.0f);
}

void CParticlePlayer::setDevice(IGXDevice *pDevice)
{
	m_pDevice = pDevice;

	fora(i, m_aEmitters)
	{
		m_aEmitters[i].setDevice(pDevice);
	}
}

void CParticlePlayer::render()
{
	fora(i, m_aEmitters)
	{
		m_aEmitters[i].render();
	}
}

void XMETHODCALLTYPE CParticlePlayer::FinalRelease()
{
	m_pSystem->onEffectPlayerReleased(this);
}

//#############################################################################

CParticlePlayerEmitter::~CParticlePlayerEmitter()
{
	mem_release(m_pData);

	mem_release(m_pRenderBuffer);
	mem_release(m_pVertexBuffer);

	if(m_pDevice)
	{
		ReleaseSharedData();
	}
}

void CParticlePlayerEmitter::setPlayer(CParticlePlayer *pPlayer)
{
	m_pPlayer = pPlayer;
}

void CParticlePlayerEmitter::setDevice(IGXDevice *pDevice)
{
	m_pDevice = pDevice;
	InitSharedData(m_pDevice);
}

void CParticlePlayerEmitter::setData(CParticleEffectEmitter *pData)
{
	if(m_pData != pData)
	{
		mem_release(m_pData);
		m_pData = pData;
		add_ref(m_pData);

		restart();
	}
}

void CParticlePlayerEmitter::update(float fDt)
{
	m_fTime += fDt;
	if(m_fTime < m_pData->m_dataGeneric.m_fStartDelay)
	{
		return;
	}

	m_aParticles.reserve(m_pData->m_dataGeneric.getMaxParticles());

	// collect dead particles
	fora(i, m_aParticles)
	{
		Particle &p = m_aParticles[i];
		p.fRemainingLifetime -= fDt;

		if(p.fRemainingLifetime <= 0.0f)
		{
			p = m_aParticles[il - 1];
			m_aParticles.erase(il - 1);
			--i; --il;
		}
	}

	// update movement
	fora(i, m_aParticles)
	{
		Particle &p = m_aParticles[i];
		p.vSpeed.y -= fDt * 9.8f * m_pData->m_dataGeneric.getGravityModifier();
		p.vPos = p.vPos + p.vSpeed * fDt;
	}

	// emission controlled by emission module
	CParticleEffectEmitterEmissionData &dataEmission = m_pData->m_dataEmission;
	float fEmitCount = evalCurve(dataEmission.m_curveRatePerSecond) * fDt;
	float fDeltaPos = m_pPlayer->getDeltaPos();
	if(fDeltaPos > SM_EPSILON)
	{
		fEmitCount += evalCurve(dataEmission.m_curveRatePerMeter) * fDeltaPos;
	}

	fora(i, m_aBursts)
	{
		CParticleBurst &burst = m_aBursts[i];
		burst.m_fTime -= fDt;
		if(burst.m_fTime < 0)
		{
			if(randf(0.0f, 1.0f) <= burst.m_fProbability)
			{
				fEmitCount += evalCurve(burst.m_curveCount);
			}

			burst.m_fTime += evalCurve(burst.m_curveInterval);
			if(burst.m_uCycles > 1)
			{
				--burst.m_uCycles;
			}
			else if(burst.m_uCycles == 1)
			{
				burst = m_aBursts[il - 1];
				m_aBursts.erase(il - 1);
				--i; --il;
			}
		}
	}
	
	emit(fEmitCount);
}

float CParticlePlayerEmitter::getTime()
{
	return(m_fTime - m_pData->m_dataGeneric.m_fStartDelay);
}

void CParticlePlayerEmitter::restart()
{
	m_fTime = 0.0f;
	m_fEmitFrac = 0.0f;

	m_aBursts = m_pData->m_dataEmission.m_aBursts;
}

void CParticlePlayerEmitter::clear()
{
	m_aParticles.clearFast();
}

void CParticlePlayerEmitter::emit(float fCount)
{
	m_fEmitFrac += fCount;
	if(m_fEmitFrac >= 1.0f)
	{
		UINT uCount = (UINT)m_fEmitFrac;
		m_fEmitFrac -= (float)uCount;

		for(UINT i = 0; i < uCount; ++i)
		{
			emitOne(uCount, i);
		}
	}
}

void CParticlePlayerEmitter::emitOne(UINT uCountInGen, UINT uIdInGen)
{
	if(m_aParticles.size() == m_aParticles.GetAllocSize())
	{
		// limit is reached
		return;
	}

	CParticleEffectEmitterGenericData &dataGeneric = m_pData->m_dataGeneric;

	Particle &newParticle = m_aParticles[m_aParticles.size()];
	newParticle.fRemainingLifetime = evalCurve(dataGeneric.m_curveStartLifetime);
	newParticle.vColor = dataGeneric.m_vStartColor;

	float3 vBasePos, vBaseDir, vDir;
	m_pData->m_dataShape.evaluate(getTime() / m_pData->m_dataGeneric.m_fDuration, &vBasePos, &vBaseDir, uCountInGen, uIdInGen);
	if(dataGeneric.m_simulationSpace == XPSS_WORLD)
	{
		// add player pos and orient
		vBasePos = m_pPlayer->getOrient() * vBasePos + m_pPlayer->getPos();
		vDir = m_pPlayer->getOrient() * vBaseDir;
	}
	else
	{
		vDir = vBaseDir;
	}
	newParticle.vPos = vBasePos;
	newParticle.vSpeed = (float3)(vDir * evalCurve(dataGeneric.m_curveStartSpeed));

	if(dataGeneric.m_bStartSizeSeparate)
	{
		newParticle.vSize.x = evalCurve(dataGeneric.m_curveStartSizeX);
		newParticle.vSize.y = evalCurve(dataGeneric.m_curveStartSizeY);
		newParticle.vSize.z = evalCurve(dataGeneric.m_curveStartSizeZ);
	}
	else
	{
		newParticle.vSize = (float3)evalCurve(dataGeneric.m_curveStartSizeX);
	}

	if(dataGeneric.m_bStartRotationSeparate)
	{
		newParticle.qRot = SMQuaternion(SMToRadian(evalCurve(dataGeneric.m_curveStartRotationX)), 'x') *
			SMQuaternion(SMToRadian(evalCurve(dataGeneric.m_curveStartRotationY)), 'y') *
			SMQuaternion(SMToRadian(evalCurve(dataGeneric.m_curveStartRotationZ)), 'z');
	}
	else
	{
		newParticle.qRot = SMQuaternion(SMToRadian(evalCurve(dataGeneric.m_curveStartRotationX)), 'z');
	}

	if(dataGeneric.m_fFlipRotation > 0.0f && randf(0.0f, 1.0f) <= dataGeneric.m_fFlipRotation)
	{
		newParticle.qRot.w *= -1.0f;
	}

	if(m_pData->m_dataShape.getAlignToDirection())
	{
		newParticle.qRotWorld = SMQuaternion(float3(0.0f, 1.0f, 0.0f), vBaseDir);
	}

	// m_curveStartRotationX    //! The initial rotation of particles around the x-axis when emitted.
	// m_bStartRotationSeparate //! A flag to enable 3D particle rotation.

	// set other properties
	//+ speed
	//+ size
	//+ rotation
	// spin
	// space
	//+ pos

	// shape mod:
	// pos
	// velocity dir
	// rotation
}

float CParticlePlayerEmitter::evalCurve(const CMinMaxCurve &curve)
{
	float fLerpFactor = 1.0f;
	if(curve.getMode() == XMCM_TWO_CONSTANTS || curve.getMode() == XMCM_TWO_CURVES)
	{
		fLerpFactor = randf(0.0f, 1.0f);
	}
	return(curve.evaluate(getTime() / m_pData->m_dataGeneric.m_fDuration, fLerpFactor));
}

void CParticlePlayerEmitter::updateRenderBuffer()
{
	if(m_pDevice && m_uAllocatedInstanceCount != m_aParticles.size())
	{
		m_uAllocatedInstanceCount = m_aParticles.size();
		
		mem_release(m_pRenderBuffer);
		mem_release(m_pVertexBuffer);

		m_pVertexBuffer = m_pDevice->createVertexBuffer(sizeof(GpuParticle) * m_uAllocatedInstanceCount, GXBUFFER_USAGE_STREAM);

		IGXVertexBuffer *ppVBs[] = {ms_pVertexBuffer, m_pVertexBuffer};
		m_pRenderBuffer = m_pDevice->createRenderBuffer(2, ppVBs, ms_pVertexDeclaration);
	}
}

UINT CParticlePlayerEmitter::ms_uSharedDataRefCount = 0;
IGXVertexDeclaration *CParticlePlayerEmitter::ms_pVertexDeclaration = NULL;
IGXIndexBuffer *CParticlePlayerEmitter::ms_pIndexBuffer = NULL;
IGXVertexBuffer *CParticlePlayerEmitter::ms_pVertexBuffer = NULL;

void CParticlePlayerEmitter::InitSharedData(IGXDevice *pDev)
{
	if(!ms_uSharedDataRefCount)
	{
		GXVertexElement oLayout[] =
		{
			{0, 0, GXDECLTYPE_FLOAT2, GXDECLUSAGE_TEXCOORD, GXDECLSPEC_PER_VERTEX_DATA},

			{1, 0, GXDECLTYPE_FLOAT4, GXDECLUSAGE_COLOR, GXDECLSPEC_PER_INSTANCE_DATA},
			{1, 16, GXDECLTYPE_FLOAT4, GXDECLUSAGE_TEXCOORD1, GXDECLSPEC_PER_INSTANCE_DATA},
			{1, 32, GXDECLTYPE_FLOAT4, GXDECLUSAGE_TEXCOORD2, GXDECLSPEC_PER_INSTANCE_DATA},
			{1, 48, GXDECLTYPE_FLOAT3, GXDECLUSAGE_POSITION, GXDECLSPEC_PER_INSTANCE_DATA},
			{1, 60, GXDECLTYPE_FLOAT3, GXDECLUSAGE_TEXCOORD3, GXDECLSPEC_PER_INSTANCE_DATA},
			GX_DECL_END()
		};

		float2_t aVertexData[] = {
			float2_t(0.0f, 1.0f),
			float2_t(0.0f, 0.0f),
			float2_t(1.0f, 0.0f),
			float2_t(1.0f, 1.0f)
		};

		ms_pVertexDeclaration = pDev->createVertexDeclaration(oLayout);
		ms_pVertexBuffer = pDev->createVertexBuffer(sizeof(aVertexData), GXBUFFER_USAGE_STATIC, aVertexData);
		
		USHORT pIndices[] = {0, 2, 1, 0, 3, 2};
		ms_pIndexBuffer = pDev->createIndexBuffer(sizeof(USHORT) * 6, GXBUFFER_USAGE_STATIC, GXIT_UINT16, pIndices);
	}
	++ms_uSharedDataRefCount;
}
void CParticlePlayerEmitter::ReleaseSharedData()
{
	if(--ms_uSharedDataRefCount == 0)
	{
		mem_release(ms_pIndexBuffer);
		mem_release(ms_pVertexDeclaration);
		mem_release(ms_pVertexBuffer);
	}
}

void CParticlePlayerEmitter::render()
{
	if(!m_pDevice || !m_aParticles.size())
	{
		return;
	}

	updateRenderBuffer();
	
	GpuParticle *pvData;
	if(m_pVertexBuffer->lock((void**)&pvData, GXBL_WRITE))
	{
		fora(i, m_aParticles)
		{
			GpuParticle &gp = pvData[i];
			Particle &p = m_aParticles[i];

			gp.vPos = p.vPos;
			gp.vSize = p.vSize;
			gp.vColor = p.vColor;
			gp.qRotLocal = p.qRot;
			gp.qRotGlobal = p.qRotWorld;
		}
		m_pVertexBuffer->unlock();
	}

	IGXContext *pCtx = m_pDevice->getThreadContext();

	pCtx->setIndexBuffer(ms_pIndexBuffer);
	pCtx->setRenderBuffer(m_pRenderBuffer);

	// setMaterial
	
	pCtx->drawIndexedInstanced(m_aParticles.size(), 4, 2);
}
