#ifndef __PARTICLEPLAYER_H
#define __PARTICLEPLAYER_H

#include <xcommon/particles/IXParticleSystem.h>
#include "ParticleEffect.h"
#include <xcommon/render/IXRender.h>

class IXMaterialSystem;

class CParticlePlayer;
class CParticlePlayerEmitter final
{
public:
	~CParticlePlayerEmitter();

	void setPlayer(CParticlePlayer *pPlayer);
	void setData(CParticleEffectEmitter *pData);
	void setDevice(IGXDevice *pDevice);
	//bool isEmitting();

	//void reset();

	void update(float fDt);

	float getTime();

	void restart();
	void clear();

	void render();

private:
	void emit(float fCount);
	void emitOne(UINT uCountInGen = 1, UINT uIdInGen = 0);

	float evalCurve(const CMinMaxCurve &curve);

	void updateRenderBuffer();

	static void InitSharedData(IGXDevice *pDev);
	static void ReleaseSharedData();

private:
	CParticleEffectEmitter *m_pData = NULL;
	CParticlePlayer *m_pPlayer = NULL;
	IGXDevice *m_pDevice = NULL;

	float m_fEmitFrac = 0.0f;

	float m_fTime = 0.0f;

	struct Particle
	{
		float fRemainingLifetime;
		float3_t vPos;
		float3_t vSpeed;
		float4_t vColor;
		float3_t vSize;
		SMQuaternion qRot;
		SMQuaternion qRotWorld;
		float3_t vSpin;
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

	void update(float fDelta);

	float getDeltaPos();

	void setDevice(IGXDevice *pDevice);

	void render();

private:
	//void simulateEmitter(UINT i, float fTime)

	void XMETHODCALLTYPE FinalRelease() override;

private:
	CParticleEffect *m_pEffect;
	CParticleSystem *m_pSystem;

	Array<CParticlePlayerEmitter> m_aEmitters;

	IGXDevice *m_pDevice = NULL;
	//IXMaterialSystem *m_pMaterialSystem = NULL;
};

#endif
