#ifndef __PARTICLEPLAYER_H
#define __PARTICLEPLAYER_H

#include <xcommon/particles/IXParticleSystem.h>
#include "ParticleEffect.h"
#include <xcommon/render/IXRender.h>
#include <xcommon/IXScene.h>

class IXMaterialSystem;

struct CEmitterTransformData
{
	SMQuaternion qRot;
	float3 vPos;
};

class CParticlePlayer;
class CParticlePlayerEmitter final
{
public:
	CParticlePlayerEmitter(CParticlePlayer *pPlayer);
	~CParticlePlayerEmitter();

	void setData(CParticleEffectEmitter *pData);
	void setDevice(IGXDevice *pDevice);
	bool isEmitting();
	bool isAlive();

	//void reset();

	void update(float fDt, float fPlaybackTime, bool bAllowEmission);

	float getTime();

	void restart();
	void clear();

	void render(IXMaterialSystem *pMaterialSystem);

	float3_t getLocalPos();
	SMQuaternion getLocalOrient();

	void updateTransform(bool bFirstUpdate);

	void onMaterialEmissivityChanged(const IXMaterial *pMaterial);
	void onMaterialTransparencyChanged(const IXMaterial *pMaterial);

	const SMAABB& getBounds() const;

	void getMaterial(IXMaterial **ppMaterial);

	void setLayer(UINT uLayer);

private:
	void emit(float fCount);
	void emitOne(UINT uCountInGen = 1, UINT uIdInGen = 0);

	float evalCurve(const CMinMaxCurve &curve);
	float evalCurve(const CMinMaxCurve &curve, float fLerpFactor);
	float evalCurve(const CMinMaxCurve &curve, float fLerpFactor, float fTime);
	float4_t evalGradient(const C2ColorGradients &grad);
	float4_t evalGradient(const C2ColorGradients &grad, float fLerpFactor);
	float4_t evalGradient(const C2ColorGradients &grad, float fLerpFactor, float fTime);

	void updateRenderBuffer();

	static void InitSharedData(IGXDevice *pDev);
	static void ReleaseSharedData();

	void updateSceneObject();

	void onFeaturesChanged();

private:
	CParticleEffectEmitter *m_pData = NULL;
	CParticlePlayer *m_pPlayer = NULL;
	IGXDevice *m_pDevice = NULL;

	IXSceneObject *m_pSceneObject = NULL;

	SMAABB m_aabb;

	float m_fEmitFrac = 0.0f;

	float m_fTime = 0.0f;
	float m_fTimeFrac = 0.0f;

	struct Particle
	{
		float fStartLifetime;
		float fRemainingLifetime;
		float fRandom;
		float3_t vPos;
		float3_t vSpeed;
		float3_t vAddedSpeed;
		float fSpeedMultiplier;
		float4_t vStartColor;
		float4_t vColor;
		float3_t vStartSize;
		float3_t vSize;
		SMQuaternion qRot;
		SMQuaternion qRotWorld;
		//float3_t vSpin;
		SMQuaternion qOrbitBase;
		SMQuaternion qOrbit;
		float fRadialSpeed = 0.0f;
		//float3_t vOrbitOriginDir;
		//float fOrbitOriginDistance;
	};
	Array<Particle> m_aParticles;

	Array<CParticleBurst> m_aBursts;

	struct GpuParticle
	{
		float4_t vColor;
		SMQuaternion qRotLocal;
		SMQuaternion qRotGlobal;
		float3_t vPos;
		float3_t vSize;
	};

	static UINT ms_uSharedDataRefCount;
	static IGXVertexDeclaration *ms_pVertexDeclaration;
	static IGXIndexBuffer *ms_pIndexBuffer;
	static IGXVertexBuffer *ms_pVertexBuffer;

	IGXRenderBuffer *m_pRenderBuffer = NULL;
	IGXVertexBuffer *m_pVertexBuffer = NULL;
	UINT m_uAllocatedInstanceCount = 0;

	CEmitterTransformData m_objectData;
	IGXConstantBuffer *m_pObjectConstant = NULL;

	float3_t m_vPrevWorldPos;

	float m_fMomentalSpeed = 0.0f;

	bool m_isTransformConstandDirty = false;

	IXMaterial *m_pMaterial = NULL;
	const char *m_szMaterial = NULL;
};

//#############################################################################

class CParticlePlayer final: public IXUnknownImplementation<IXParticlePlayer>
{
public:
	CParticlePlayer(CParticleEffect *pEffect, CParticleSystem *pSystem);
	~CParticlePlayer();

	void XMETHODCALLTYPE play() override;
	void XMETHODCALLTYPE pause() override;
	void XMETHODCALLTYPE stop(bool bClear = false) override;
	void XMETHODCALLTYPE clear() override;

	void XMETHODCALLTYPE simulate(float fTime, bool bRestart = false) override;

	bool XMETHODCALLTYPE isEmitting() override;
	bool XMETHODCALLTYPE isPaused() override;
	bool XMETHODCALLTYPE isPlaying() override;
	bool XMETHODCALLTYPE isStopped() override;
	bool XMETHODCALLTYPE isAlive() override;

	UINT XMETHODCALLTYPE getParticleCount() override;
	float XMETHODCALLTYPE getTime() override;

	float3_t XMETHODCALLTYPE getPos() override;
	void XMETHODCALLTYPE setPos(const float3_t &vPos) override;

	SMQuaternion XMETHODCALLTYPE getOrient() override;
	void XMETHODCALLTYPE setOrient(const SMQuaternion &qRot) override;

	void XMETHODCALLTYPE setCallback(IXParticlePlayerCallback *pCallback) override;

	void update(float fDelta);

	void setDevice(IGXDevice *pDevice);

	void render(IXMaterialSystem *pMaterialSystem);
	
	void setVertexShader(bool useTransform);

	CParticleSystem* getSystem();

	void onMaterialEmissivityChanged(const IXMaterial *pMaterial);
	void onMaterialTransparencyChanged(const IXMaterial *pMaterial);

	void XMETHODCALLTYPE setLayer(UINT uLayer) override;
	UINT XMETHODCALLTYPE getLayer() override;

	void syncEmitters();

private:
	//void simulateEmitter(UINT i, float fTime)

	void XMETHODCALLTYPE FinalRelease() override;

private:
	CParticleEffect *m_pEffect;
	CParticleSystem *m_pSystem;

	IXParticlePlayerCallback *m_pCallback = NULL;

	Array<CParticlePlayerEmitter*> m_aEmitters;

	IGXDevice *m_pDevice = NULL;
	//IXMaterialSystem *m_pMaterialSystem = NULL;

	float3_t m_vPos;
	SMQuaternion m_qOrient;

	float m_fTime = 0.0f;

	UINT m_uLayer = 0;

	bool m_isUpdating = false;
	bool m_isEmitting = false;
	bool m_isPaused = false;

	bool m_isTransformDirty = false;
	bool m_isFirstTransformUpdate = true;
};

#endif
