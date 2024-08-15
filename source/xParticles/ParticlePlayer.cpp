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
	fora(i, m_aEmitters)
	{
		m_pSystem->onPlayerEmitterReleased(m_aEmitters[i]);
	}
	mem_release(m_pEffect);
}

void XMETHODCALLTYPE CParticlePlayer::play()
{
	if(m_isPaused)
	{
		m_isPaused = false;
	}
	else
	{
		m_fTime = 0.0f;
		fora(i, m_aEmitters)
		{
			m_aEmitters[i]->restart();
		}
		m_isEmitting = true;
	}
	m_isUpdating = true;
}
void XMETHODCALLTYPE CParticlePlayer::pause()
{
	m_isUpdating = false;
	m_isPaused = true;
}
void XMETHODCALLTYPE CParticlePlayer::stop(bool bClear)
{
	m_isEmitting = false;
	SAFE_CALL(m_pCallback, onEvent, XPE_BURNOUT);

	if(bClear)
	{
		m_fTime = 0.0f;
		m_isUpdating = false;
		m_isPaused = false;

		fora(i, m_aEmitters)
		{
			m_aEmitters[i]->clear();
		}

		//updateSceneObject();
		SAFE_CALL(m_pCallback, onEvent, XPE_FINISH);
	}
}
void XMETHODCALLTYPE CParticlePlayer::clear()
{
	fora(i, m_aEmitters)
	{
		m_aEmitters[i]->clear();
	}
}

void XMETHODCALLTYPE CParticlePlayer::simulate(float fTime, bool bRestart)
{
}

bool XMETHODCALLTYPE CParticlePlayer::isEmitting()
{
	return(m_isEmitting);
}
bool XMETHODCALLTYPE CParticlePlayer::isPaused()
{
	return(m_isPaused);
}
bool XMETHODCALLTYPE CParticlePlayer::isPlaying()
{
	return(m_isUpdating);
}
bool XMETHODCALLTYPE CParticlePlayer::isStopped()
{
	return(!m_isUpdating && !m_isPaused);
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
	return(m_fTime);
}

float3_t XMETHODCALLTYPE CParticlePlayer::getPos()
{
	return(m_vPos);
}
void XMETHODCALLTYPE CParticlePlayer::setPos(const float3_t &vPos)
{
	m_vPos = vPos;
	m_isTransformDirty = true;
}

SMQuaternion XMETHODCALLTYPE CParticlePlayer::getOrient()
{
	return(m_qOrient);
}
void XMETHODCALLTYPE CParticlePlayer::setOrient(const SMQuaternion &qRot)
{
	m_qOrient = qRot;
	m_isTransformDirty = true;
}

void XMETHODCALLTYPE CParticlePlayer::setCallback(IXParticlePlayerCallback *pCallback)
{
	m_pCallback = pCallback;
}

void CParticlePlayer::syncEmitters()
{
	UINT uEmCount = m_pEffect->getEmitterCount();
	if(uEmCount != m_aEmitters.size())
	{
		UINT i = 0;
		for(UINT l = m_aEmitters.size(); i < l; ++i)
		{
			if(i < uEmCount)
			{
				m_aEmitters[i]->setData(m_pEffect->getEmitterAtInternal(i));
			}
			else
			{
				m_pSystem->onPlayerEmitterReleased(m_aEmitters[i]);
			}
		}
		i = m_aEmitters.size();
		m_aEmitters.resizeFast(uEmCount);

		for(; i < uEmCount; ++i)
		{
			m_aEmitters[i] = m_pSystem->allocPlayerEmitter(this);
			m_aEmitters[i]->setData(m_pEffect->getEmitterAtInternal(i));
			m_aEmitters[i]->updateTransform(true);

			if(m_pDevice)
			{
				m_aEmitters[i]->setDevice(m_pDevice);
			}
		}
	}
}

void CParticlePlayer::update(float fDelta)
{
	if(!m_isUpdating)
	{
		return;
	}

	XPROFILE_FUNCTION();

	m_fTime += fDelta;

	UINT uEmCount = m_pEffect->getEmitterCount();
	/*if(uEmCount != m_aEmitters.size())
	{
		for(UINT i = uEmCount, l = m_aEmitters.size(); i < l; ++i)
		{
			m_aEmitters[i].clear();
		}
		m_pSystem->onEffectPlayerSyncEmittersReq(this);

		return;
	}*/

	syncEmitters();

	bool isEmitting = false;
	bool isAlive = false;
	for(UINT i = 0; i < uEmCount; ++i)
	{
		if(m_isTransformDirty)
		{
			m_aEmitters[i]->updateTransform(m_isFirstTransformUpdate);
		}
		m_aEmitters[i]->update(fDelta, m_fTime, m_isEmitting);

		if(!isEmitting && m_aEmitters[i]->isEmitting())
		{
			isEmitting = true;
		}
		if(!isAlive && m_aEmitters[i]->isAlive())
		{
			isAlive = true;
		}
	}

	m_isFirstTransformUpdate = false;
	m_isTransformDirty = false;

	if(m_isEmitting && !isEmitting)
	{
		m_isEmitting = false;
		SAFE_CALL(m_pCallback, onEvent, XPE_BURNOUT);
	}

	if(!isAlive)
	{
		m_isUpdating = false;
		SAFE_CALL(m_pCallback, onEvent, XPE_FINISH);
	}
}

void CParticlePlayer::setDevice(IGXDevice *pDevice)
{
	m_pDevice = pDevice;

	fora(i, m_aEmitters)
	{
		m_aEmitters[i]->setDevice(pDevice);
	}
}

void CParticlePlayer::render(IXMaterialSystem *pMaterialSystem)
{
	fora(i, m_aEmitters)
	{
		m_aEmitters[i]->render(pMaterialSystem);
	}
}

void XMETHODCALLTYPE CParticlePlayer::FinalRelease()
{
	m_pSystem->onEffectPlayerReleased(this);
}

void CParticlePlayer::setVertexShader(bool useTransform)
{
	m_pSystem->setVertexShader(useTransform);
}

CParticleSystem* CParticlePlayer::getSystem()
{
	return(m_pSystem);
}

void CParticlePlayer::onMaterialTransparencyChanged(const IXMaterial *pMaterial)
{
	//ScopedSpinLock lock(m_slModels);

	fora(i, m_aEmitters)
	{
		m_aEmitters[i]->onMaterialTransparencyChanged(pMaterial);
	}
}

void CParticlePlayer::onMaterialEmissivityChanged(const IXMaterial *pMaterial)
{
	//ScopedSpinLock lock(m_slModels);

	fora(i, m_aEmitters)
	{
		m_aEmitters[i]->onMaterialEmissivityChanged(pMaterial);
	}
}

void XMETHODCALLTYPE CParticlePlayer::setLayer(UINT uLayer)
{
	m_uLayer = uLayer;
	fora(i, m_aEmitters)
	{
		m_aEmitters[i]->setLayer(uLayer);
	}
}
UINT XMETHODCALLTYPE CParticlePlayer::getLayer()
{
	return(m_uLayer);
}


//#############################################################################

CParticlePlayerEmitter::CParticlePlayerEmitter(CParticlePlayer *pPlayer):
	m_pPlayer(pPlayer)
{
}

CParticlePlayerEmitter::~CParticlePlayerEmitter()
{
	mem_release(m_pData);

	mem_release(m_pRenderBuffer);
	mem_release(m_pVertexBuffer);

	mem_release(m_pObjectConstant);

	mem_release(m_pSceneObject);

	mem_release(m_pMaterial);

	if(m_pDevice)
	{
		ReleaseSharedData();
	}
}

void CParticlePlayerEmitter::setDevice(IGXDevice *pDevice)
{
	m_pDevice = pDevice;
	InitSharedData(m_pDevice);

	m_pObjectConstant = pDevice->createConstantBuffer(sizeof(m_objectData));
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

bool CParticlePlayerEmitter::isEmitting()
{
	return(m_pData->m_dataGeneric.m_isLooping || (m_fTime - m_pData->m_dataGeneric.m_fStartDelay) < m_pData->m_dataGeneric.m_fDuration);
}

bool CParticlePlayerEmitter::isAlive()
{
	return(isEmitting() || m_aParticles.size());
}

TODO("Move to sxmath.h");
static SMAABB SMAABBTransform(const SMAABB &aabb, const transform_t &xform)
{
	float3 v0 = xform * aabb.vMin;
	SMAABB aabbOut(v0, v0);
	v0 = xform * float3(aabb.vMin.x, aabb.vMin.y, aabb.vMax.z);
	aabbOut.vMin = SMVectorMin(aabbOut.vMin, v0);
	aabbOut.vMax = SMVectorMax(aabbOut.vMax, v0);
	v0 = xform * float3(aabb.vMin.x, aabb.vMax.y, aabb.vMin.z);
	aabbOut.vMin = SMVectorMin(aabbOut.vMin, v0);
	aabbOut.vMax = SMVectorMax(aabbOut.vMax, v0);
	v0 = xform * float3(aabb.vMin.x, aabb.vMax.y, aabb.vMax.z);
	aabbOut.vMin = SMVectorMin(aabbOut.vMin, v0);
	aabbOut.vMax = SMVectorMax(aabbOut.vMax, v0);
	v0 = xform * float3(aabb.vMax.x, aabb.vMin.y, aabb.vMin.z);
	aabbOut.vMin = SMVectorMin(aabbOut.vMin, v0);
	aabbOut.vMax = SMVectorMax(aabbOut.vMax, v0);
	v0 = xform * float3(aabb.vMax.x, aabb.vMin.y, aabb.vMax.z);
	aabbOut.vMin = SMVectorMin(aabbOut.vMin, v0);
	aabbOut.vMax = SMVectorMax(aabbOut.vMax, v0);
	v0 = xform * float3(aabb.vMax.x, aabb.vMax.y, aabb.vMin.z);
	aabbOut.vMin = SMVectorMin(aabbOut.vMin, v0);
	aabbOut.vMax = SMVectorMax(aabbOut.vMax, v0);
	v0 = xform * aabb.vMax;
	aabbOut.vMin = SMVectorMin(aabbOut.vMin, v0);
	aabbOut.vMax = SMVectorMax(aabbOut.vMax, v0);

	return(aabbOut);
}

void CParticlePlayerEmitter::update(float fDt, float fPlaybackTime, bool bAllowEmission)
{
	if(m_szMaterial != m_pData->getRenderData()->getMaterial())
	{
		m_szMaterial = m_pData->getRenderData()->getMaterial();
		mem_release(m_pMaterial);
		m_pPlayer->getSystem()->getMaterialSystem()->loadMaterial(m_szMaterial[0] ? m_szMaterial : "dev_glare", &m_pMaterial);
		onFeaturesChanged();
	}

	m_fTime = fPlaybackTime;
	if(m_fTime < m_pData->m_dataGeneric.m_fStartDelay)
	{
		return;
	}
	m_fTimeFrac = fmodf(getTime() / m_pData->m_dataGeneric.m_fDuration, 1.0f);

	m_aParticles.reserve(m_pData->m_dataGeneric.getMaxParticles());

	float fDeltaPos = SMVector3Distance(m_objectData.vPos, m_vPrevWorldPos);
	m_fMomentalSpeed = fDeltaPos / fDt;
	m_vPrevWorldPos = m_objectData.vPos;

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

	if(bAllowEmission && (m_pData->m_dataGeneric.m_isLooping || getTime() <= m_pData->m_dataGeneric.m_fDuration))
	{
		// emission controlled by emission module
		CParticleEffectEmitterEmissionData &dataEmission = m_pData->m_dataEmission;
		float fEmitCount = evalCurve(dataEmission.m_curveRatePerSecond) * fDt;
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

	//float2_t v2SpeedRange;
	//bool bReportSpeed = false;
	//static float s_fReportTime = 0.0f;
	//s_fReportTime += fDt;
	//if(s_fReportTime > 1.0f)
	//{
	//	s_fReportTime = fmodf(s_fReportTime, 1.0f);
	//	bReportSpeed = true;
	//}

	SMAABB aabb;

	float fGravitySpeed = fDt * 9.8f * m_pData->m_dataGeneric.getGravityModifier();
	// update movement
	fora(i, m_aParticles)
	{
		Particle &p = m_aParticles[i];
		p.vSpeed.y -= fGravitySpeed;

		float fLifetimeFrac = 1.0f - (p.fRemainingLifetime / p.fStartLifetime);
		float fSpeedMul = 1.0f;
		if(m_pData->m_dataVelocityLifetime.isEnabled())
		{
			p.vAddedSpeed = m_pData->m_dataVelocityLifetime.evaluateLinearSpeed(fLifetimeFrac, p.fRandom, m_objectData.qRot);
			p.fSpeedMultiplier = m_pData->m_dataVelocityLifetime.evaluateSpeedMultiplier(fLifetimeFrac, p.fRandom);

			p.qOrbitBase = SMQuaternion();
			// rotate speed [rad/s]
			float fOrbitalSpeedX = m_pData->m_dataVelocityLifetime.m_curveOrbitalX.evaluate(fLifetimeFrac, p.fRandom) * p.fSpeedMultiplier;
			float fOrbitalSpeedY = m_pData->m_dataVelocityLifetime.m_curveOrbitalY.evaluate(fLifetimeFrac, p.fRandom) * p.fSpeedMultiplier;
			float fOrbitalSpeedZ = m_pData->m_dataVelocityLifetime.m_curveOrbitalZ.evaluate(fLifetimeFrac, p.fRandom) * p.fSpeedMultiplier;
			bool bOSX = !SMIsZero(fOrbitalSpeedX);
			bool bOSY = !SMIsZero(fOrbitalSpeedY);
			bool bOSZ = !SMIsZero(fOrbitalSpeedZ);

#if 0
			bool bOffX = false;
			bool bOffY = false;
			bool bOffZ = false;

			float fOffsetX, fOffsetY, fOffsetZ;
			if(bOSX || bOSY || bOSZ)
			{
				fOffsetX = m_pData->m_dataVelocityLifetime.m_curveOffsetX.evaluate(fLifetimeFrac, p.fRandom);
				fOffsetY = m_pData->m_dataVelocityLifetime.m_curveOffsetY.evaluate(fLifetimeFrac, p.fRandom);
				fOffsetZ = m_pData->m_dataVelocityLifetime.m_curveOffsetZ.evaluate(fLifetimeFrac, p.fRandom);

				bOffX = !SMIsZero(fOffsetX);
				bOffY = !SMIsZero(fOffsetY);
				bOffZ = !SMIsZero(fOffsetZ);
			}
#endif

			if(bOSX)
			{
				p.qOrbit = p.qOrbit * SMQuaternion(fOrbitalSpeedX * fDt, 'x');
#if 0
				if(bOffY || bOffZ)
				{
					float2 r = (SMVector2Length(float2(p.vSpeed.y, p.vSpeed.z)) / fOrbitalSpeedX) * SMVector2Normalize(float2(p.vSpeed.z, -p.vSpeed.y));
					float2 roff = r + float2(fOffsetY, fOffsetZ);

					//fSpeedMul = SMVector2Length(roff) * fOrbitalSpeedX / SMVector3Length(p.vSpeed);
					float fSpeedDelta = SMVector2Length(roff) * fOrbitalSpeedX - SMVector2Length(float2(p.vSpeed.y, p.vSpeed.z));
					p.vAddedSpeed = p.vAddedSpeed + SMVector3Normalize(float3(0.0f, p.vSpeed.y, p.vSpeed.z)) * fSpeedDelta;

					p.qOrbitBase = p.qOrbitBase * SMQuaternion(float3(0.0f, r.x, r.y), float3(0.0f, roff.x, roff.y));
				}
#endif
			}

			if(bOSY)
			{
				p.qOrbit = p.qOrbit * SMQuaternion(fOrbitalSpeedY * fDt, 'y');
#if 0
				if(bOffX || bOffZ)
				{
					float2 r = (SMVector2Length(float2(p.vSpeed.x, p.vSpeed.z)) / fOrbitalSpeedY) * SMVector2Normalize(float2(p.vSpeed.z, -p.vSpeed.x));
					float2 roff = r + float2(fOffsetX, fOffsetZ);

					//fSpeedMul = SMVector2Length(roff) * fOrbitalSpeedX / SMVector3Length(p.vSpeed);
					float fSpeedDelta = SMVector2Length(roff) * fOrbitalSpeedY - SMVector2Length(float2(p.vSpeed.x, p.vSpeed.z));
					p.vAddedSpeed = p.vAddedSpeed + SMVector3Normalize(float3(p.vSpeed.x, 0.0f, p.vSpeed.z)) * fSpeedDelta;

					p.qOrbitBase = p.qOrbitBase * SMQuaternion(float3(r.x, 0.0f, r.y), float3(roff.x, 0.0f, roff.y));
				}
#endif
			}

			if(bOSZ)
			{
				p.qOrbit = p.qOrbit * SMQuaternion(fOrbitalSpeedZ * fDt, 'z');
#if 0
				if(bOffX || bOffY)
				{
					float2 r = (SMVector2Length(float2(p.vSpeed.x, p.vSpeed.y)) / fOrbitalSpeedZ) * SMVector2Normalize(float2(p.vSpeed.y, -p.vSpeed.x));
					float2 roff = r + float2(fOffsetX, fOffsetY);

					//fSpeedMul = SMVector2Length(roff) * fOrbitalSpeedX / SMVector3Length(p.vSpeed);
					float fSpeedDelta = SMVector2Length(roff) * fOrbitalSpeedZ - SMVector2Length(float2(p.vSpeed.x, p.vSpeed.y));
					p.vAddedSpeed = p.vAddedSpeed + SMVector3Normalize(float3(p.vSpeed.x, p.vSpeed.z, 0.0f)) * fSpeedDelta;

					p.qOrbitBase = p.qOrbitBase * SMQuaternion(float3(r.x, r.y, 0.0f), float3(roff.x, roff.y, 0.0f));
				}
#endif
			}

			float fRadial = m_pData->m_dataVelocityLifetime.m_curveRadial.evaluate(fLifetimeFrac, p.fRandom) * fDt * p.fSpeedMultiplier;
			if(!SMIsZero(fRadial))
			{
				p.fRadialSpeed += fRadial;
				p.vAddedSpeed = p.vAddedSpeed + SMVector3Normalize(p.vSpeed) * p.fRadialSpeed;
			}
		}

		if(m_pData->m_dataForceLifetime.isEnabled())
		{
			p.vSpeed = p.vSpeed + m_pData->m_dataForceLifetime.evaluateForce(fLifetimeFrac, p.fRandom, m_objectData.qRot) * fDt;
		}


		float fSpeed = -1.0f;

		// clamp velocity
		if(m_pData->m_dataLimitVelocityLifetime.isEnabled())
		{
			float3 vSpeed = (p.vSpeed + p.vAddedSpeed) * p.fSpeedMultiplier;
			// clamp

			vSpeed = m_pData->m_dataLimitVelocityLifetime.evaluateLimit(vSpeed, fLifetimeFrac, p.fRandom, m_objectData.qRot);

			p.vSpeed = vSpeed / p.fSpeedMultiplier - p.vAddedSpeed;
			
			// apply drag
			float fDrag = m_pData->m_dataLimitVelocityLifetime.m_curveDrag.evaluate(fLifetimeFrac, p.fRandom);


			if(m_pData->m_dataLimitVelocityLifetime.m_bMultiplyBySize){
				fDrag *= SMVector3Length(p.vSize);
			}

			if(m_pData->m_dataLimitVelocityLifetime.m_bMultiplyByVelocity){
				if(fSpeed < 0.0f){
					fSpeed = SMVector3Length(p.vSpeed);
				}
				fDrag *= fSpeed;
			}

			if(fDrag >= 0.001f){
				if(fSpeed < 0.0f){
					fSpeed = SMVector3Length(p.vSpeed);
				}
				fSpeed -= fDrag * fDt;

				p.vSpeed = SMVector3Normalize(p.vSpeed) * fSpeed;
			}
		}

		if(m_pData->m_dataColorLifetime.isEnabled() || m_pData->m_dataColorSpeed.isEnabled())
		{
			p.vColor = p.vStartColor;
			if(m_pData->m_dataColorLifetime.isEnabled())
			{
				p.vColor = p.vColor * evalGradient(m_pData->m_dataColorLifetime.m_gradColor, p.fRandom, fLifetimeFrac);
			}
			if(m_pData->m_dataColorSpeed.isEnabled())
			{
				p.vColor = p.vColor * m_pData->m_dataColorSpeed.evaluate(SMVector3Length(p.vSpeed + p.vAddedSpeed), p.fRandom);
			}
		}

		if(m_pData->m_dataSizeLifetime.isEnabled() || m_pData->m_dataSizeSpeed.isEnabled())
		{
			p.vSize = p.vStartSize;
			if(m_pData->m_dataSizeLifetime.isEnabled())
			{
				float3 vSizeCoeff = m_pData->m_dataSizeLifetime.m_curveSizeX.evaluate(fLifetimeFrac, p.fRandom);
				if(m_pData->m_dataSizeLifetime.getSeparateAxes())
				{
					vSizeCoeff.y = m_pData->m_dataSizeLifetime.m_curveSizeY.evaluate(fLifetimeFrac, p.fRandom);
					vSizeCoeff.z = m_pData->m_dataSizeLifetime.m_curveSizeZ.evaluate(fLifetimeFrac, p.fRandom);
				}

				p.vSize = p.vSize * vSizeCoeff;
			}

			if(m_pData->m_dataSizeSpeed.isEnabled())
			{
				p.vSize = p.vSize * m_pData->m_dataSizeSpeed.evaluate(SMVector3Length(p.vSpeed + p.vAddedSpeed), p.fRandom);
			}
		}


		float3 vTotalSpeed = (p.vSpeed * fSpeedMul + p.vAddedSpeed) * p.fSpeedMultiplier;
		p.vPos = p.vPos + p.qOrbitBase * p.qOrbit * vTotalSpeed * fDt;

		/*if(bReportSpeed)
		{
			float fTotalSpeed = SMVector3Length2(vTotalSpeed);
			if(i == 0)
			{
				v2SpeedRange = float2_t(fTotalSpeed, fTotalSpeed);
			}
			else
			{
				if(fTotalSpeed < v2SpeedRange.x)
				{
					v2SpeedRange.x = fTotalSpeed;
				}
				if(fTotalSpeed > v2SpeedRange.y)
				{
					v2SpeedRange.y = fTotalSpeed;
				}
			}
		}*/

		if(m_pData->m_dataRotationLifetime.isEnabled() || m_pData->m_dataRotationSpeed.isEnabled())
		{
			SMQuaternion qDeltaRot;
			if(m_pData->m_dataRotationLifetime.isEnabled())
			{
				qDeltaRot = m_pData->m_dataRotationLifetime.evaluate(fLifetimeFrac, p.fRandom, fDt);
			}

			if(m_pData->m_dataRotationSpeed.isEnabled())
			{
				qDeltaRot = qDeltaRot * m_pData->m_dataRotationSpeed.evaluate(SMVector3Length(p.vSpeed + p.vAddedSpeed), p.fRandom, fDt);
			}

			p.qRot = p.qRot * qDeltaRot;
		}

		if(i == 0)
		{
			aabb.vMin = aabb.vMax = p.vPos;
		}
		else
		{
			aabb.vMin = SMVectorMin(aabb.vMin, p.vPos);
			aabb.vMax = SMVectorMax(aabb.vMax, p.vPos);
		}
	}

	if(m_pData->getGenericData()->getSimulationSpace() != XPSS_WORLD)
	{
		aabb = SMAABBTransform(aabb, transform_t(m_pPlayer->getPos(), m_pPlayer->getOrient()));
	}

	if(!m_aParticles.size() || SMVector3Length2(aabb.vMin - m_aabb.vMin) > 0.01f || SMVector3Length2(aabb.vMax - m_aabb.vMax) > 0.01f)
	{
		m_aabb = aabb;
		updateSceneObject();
	}

	/*if(bReportSpeed)
	{
		LogInfo("Speed: %.2f-%.2f\n", v2SpeedRange.x, v2SpeedRange.y);
	}*/
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
	updateSceneObject();
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
	newParticle.fRandom = randf(0.0f, 1.0f);

	float fLifetime = evalCurve(dataGeneric.m_curveStartLifetime, newParticle.fRandom);
	if(m_pData->m_dataLifetimeEmitterSpeed.isEnabled())
	{
		fLifetime *= m_pData->m_dataLifetimeEmitterSpeed.evaluate(m_fMomentalSpeed, newParticle.fRandom);
	}

	newParticle.fRemainingLifetime = newParticle.fStartLifetime = fLifetime;
	newParticle.vStartColor = newParticle.vColor = evalGradient(dataGeneric.m_gradStartColor, newParticle.fRandom);
	newParticle.fSpeedMultiplier = 1.0f;

	float3 vBasePos, vBaseDir, vDir;
	if(m_pData->m_dataShape.isEnabled())
	{
		m_pData->m_dataShape.evaluate(getTime() / m_pData->m_dataGeneric.m_fDuration, &vBasePos, &vBaseDir, uCountInGen, uIdInGen);
	}
	else
	{
		vBasePos = 0.0f;
		vBaseDir = float3(0.0f, 1.0f, 0.0f);
	}
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
	newParticle.vSpeed = (float3)(vDir * evalCurve(dataGeneric.m_curveStartSpeed, newParticle.fRandom));

	if(dataGeneric.m_bStartSizeSeparate)
	{
		newParticle.vStartSize.x = newParticle.vSize.x = evalCurve(dataGeneric.m_curveStartSizeX, newParticle.fRandom);
		newParticle.vStartSize.y = newParticle.vSize.y = evalCurve(dataGeneric.m_curveStartSizeY, newParticle.fRandom);
		newParticle.vStartSize.z = newParticle.vSize.z = evalCurve(dataGeneric.m_curveStartSizeZ, newParticle.fRandom);
	}
	else
	{
		newParticle.vStartSize = newParticle.vSize = (float3)evalCurve(dataGeneric.m_curveStartSizeX, newParticle.fRandom);
	}

	if(dataGeneric.m_bStartRotationSeparate)
	{
		newParticle.qRot = SMQuaternion(SMToRadian(evalCurve(dataGeneric.m_curveStartRotationX, newParticle.fRandom)), 'x') *
			SMQuaternion(SMToRadian(evalCurve(dataGeneric.m_curveStartRotationY, newParticle.fRandom)), 'y') *
			SMQuaternion(SMToRadian(evalCurve(dataGeneric.m_curveStartRotationZ, newParticle.fRandom)), 'z');
	}
	else
	{
		newParticle.qRot = SMQuaternion(SMToRadian(evalCurve(dataGeneric.m_curveStartRotationX, newParticle.fRandom)), 'z');
	}

	if(dataGeneric.m_fFlipRotation > 0.0f && randf(0.0f, 1.0f) <= dataGeneric.m_fFlipRotation)
	{
		newParticle.qRot.w *= -1.0f;
	}

	if(m_pData->m_dataShape.isEnabled() && m_pData->m_dataShape.getAlignToDirection())
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

	return(evalCurve(curve, fLerpFactor));
}
float CParticlePlayerEmitter::evalCurve(const CMinMaxCurve &curve, float fLerpFactor)
{	
	return(evalCurve(curve, fLerpFactor, m_fTimeFrac));
}
float CParticlePlayerEmitter::evalCurve(const CMinMaxCurve &curve, float fLerpFactor, float fTime)
{
	return(curve.evaluate(fTime, fLerpFactor));
}

float4_t CParticlePlayerEmitter::evalGradient(const C2ColorGradients &grad)
{
	float fLerpFactor = 1.0f;
	if(grad.getMode() == X2CGM_TWO_COLORS || grad.getMode() == X2CGM_TWO_GRADIENTS)
	{
		fLerpFactor = randf(0.0f, 1.0f);
	}
	return(evalGradient(grad, fLerpFactor, m_fTimeFrac));
}
float4_t CParticlePlayerEmitter::evalGradient(const C2ColorGradients &grad, float fLerpFactor)
{
	return(evalGradient(grad, fLerpFactor, m_fTimeFrac));
}
float4_t CParticlePlayerEmitter::evalGradient(const C2ColorGradients &grad, float fLerpFactor, float fTime)
{
	return(grad.evaluate(fTime, fLerpFactor));
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

void CParticlePlayerEmitter::render(IXMaterialSystem *pMaterialSystem)
{
	if(!m_pDevice || !m_aParticles.size())
	{
		return;
	}

	updateRenderBuffer();

	if(m_isTransformConstandDirty && m_pData->m_dataGeneric.getSimulationSpace() == XPSS_LOCAL)
	{
		m_pObjectConstant->update(&m_objectData);
		m_isTransformConstandDirty = false;
	}

	if(m_pData->m_dataGeneric.getSimulationSpace() == XPSS_LOCAL)
	{
		m_pDevice->getThreadContext()->setVSConstant(m_pObjectConstant, 1 /* SCR_OBJECT */);
	}
	m_pPlayer->setVertexShader(m_pData->m_dataGeneric.getSimulationSpace() == XPSS_LOCAL);
	
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
	pMaterialSystem->bindMaterial(m_pMaterial);
		
	pCtx->drawIndexedInstanced(m_aParticles.size(), 4, 2);
}

float3_t CParticlePlayerEmitter::getLocalPos()
{
	return(m_pData->getPos());
}
SMQuaternion CParticlePlayerEmitter::getLocalOrient()
{
	return(m_pData->getOrient());
}

void CParticlePlayerEmitter::updateTransform(bool bFirstUpdate)
{
	m_objectData.vPos = m_pPlayer->getPos() + m_pPlayer->getOrient() * getLocalPos();
	m_objectData.qRot = m_pPlayer->getOrient() * getLocalOrient();

	if(bFirstUpdate)
	{
		m_vPrevWorldPos = m_objectData.vPos;
	}

	m_isTransformConstandDirty = true;
}

void CParticlePlayerEmitter::updateSceneObject()
{
	if(m_aParticles.size())
	{
		if(m_pSceneObject)
		{
			m_pSceneObject->update(m_aabb);
		}
		else
		{
			m_pSceneObject = m_pPlayer->getSystem()->getSceneObjectType()->newObject(m_aabb, this);
			m_pSceneObject->setLayer(m_pPlayer->getLayer());
			onFeaturesChanged();
		}
	}
	else
	{
		mem_release(m_pSceneObject);
	}
}

void CParticlePlayerEmitter::onFeaturesChanged()
{
	if(m_pSceneObject)
	{
		XMODEL_FEATURE bmFeatures = {};
		if(m_pMaterial->isTransparent())
		{
			bmFeatures |= MF_TRANSPARENT;
		}
		else
		{
			bmFeatures |= MF_OPAQUE;
		}
		if(m_pMaterial->isEmissive())
		{
			bmFeatures |= MF_SELFILLUM;
		}

		IXSceneFeature *ppFeatures[4] = {0};
		int i = 0;

		if(bmFeatures & MF_OPAQUE)
		{
			ppFeatures[i++] = m_pPlayer->getSystem()->getFeature(MF_OPAQUE);
		}
		if(bmFeatures & MF_TRANSPARENT)
		{
			ppFeatures[i++] = m_pPlayer->getSystem()->getFeature(MF_TRANSPARENT);
		}
		if(bmFeatures & MF_SELFILLUM)
		{
			ppFeatures[i++] = m_pPlayer->getSystem()->getFeature(MF_SELFILLUM);
		}

		assert(i < ARRAYSIZE(ppFeatures));

		m_pSceneObject->setFeatures(ppFeatures);
	}
}

void CParticlePlayerEmitter::onMaterialTransparencyChanged(const IXMaterial *pMaterial)
{
	if(m_pMaterial == pMaterial)
	{
		onFeaturesChanged();
	}
}

void CParticlePlayerEmitter::onMaterialEmissivityChanged(const IXMaterial *pMaterial)
{
	if(m_pMaterial == pMaterial)
	{
		onFeaturesChanged();
	}
}

const SMAABB& CParticlePlayerEmitter::getBounds() const
{
	return(m_aabb);
}

void CParticlePlayerEmitter::getMaterial(IXMaterial **ppMaterial)
{
	add_ref(m_pMaterial);
	*ppMaterial = m_pMaterial;
}

void CParticlePlayerEmitter::setLayer(UINT uLayer)
{
	SAFE_CALL(m_pSceneObject, setLayer, uLayer);
}
