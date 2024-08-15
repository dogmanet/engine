#ifndef __PARTICLESYSTEM_H
#define __PARTICLESYSTEM_H

#include <xcommon/particles/IXParticleSystem.h>
#include <xcommon/IXCore.h>
#include <xcommon/resource/IXModel.h>
#include "ParticleEffect.h"
#include "ParticlePlayer.h"
#include <common/queue.h>
#include <mtrl/IXMaterialSystem.h>
#include <xcommon/IXRenderable.h>
#include <xcommon/IXScene.h>

class CRenderableVisibility;

class CParticleSystem;
class CMaterialChangedEventListener final: public IEventListener<XEventMaterialChanged>
{
public:
	CMaterialChangedEventListener(CParticleSystem *pSystem);
	void onEvent(const XEventMaterialChanged *pData) override;

protected:
	CParticleSystem *m_pSystem;
};

class CParticleSystem final: public IXUnknownImplementation<IXParticleSystem>
{
	friend class CMaterialChangedEventListener;
public:
	CParticleSystem(IXCore *pCore);
	~CParticleSystem();

	XIMPLEMENT_VERSION(IXPARTICLESYSTEM_VERSION);

	bool XMETHODCALLTYPE newEffect(const char *szName, IXParticleEffect **ppOut) override;
	bool newEffect(const char *szName, CParticleEffect **ppOut);
	bool XMETHODCALLTYPE getEffect(const char *szName, IXParticleEffect **ppOut) override;

	void XMETHODCALLTYPE newEffectPlayer(IXParticleEffect *pEffect, IXParticlePlayer **ppOut) override;

	void onEffectReleased(const char *szName);
	void onEffectPlayerReleased(CParticlePlayer *pPlayer);
	void onEmitterReleased(CParticleEffectEmitter *pEmitter);
	void onPlayerEmitterReleased(CParticlePlayerEmitter *pEmitter);

	bool saveEffect(CParticleEffect *pEffect);

	void update(float fDelta);
	void sync();

	CParticleEffectEmitter* allocEmitter();


	void setDevice(IGXDevice *pDevice);
	void setMaterialSystem(IXMaterialSystem *pMaterialSystem);

	void render(CRenderableVisibility *pVisibility);
	void renderEmissive(CRenderableVisibility *pVisibility);
	void render(Array<CParticlePlayerEmitter*> &aRenderList, XMODEL_FEATURE bmWhat);
	void renderTransparentObject(CRenderableVisibility *pVisibility, UINT uIndex, UINT uSplitPlanes);

	void setVertexShader(bool useTransform);

	IXSceneObjectType* getSceneObjectType();

	IXSceneFeature* getFeature(XMODEL_FEATURE bmFeature);

	void computeVisibility(const IXFrustum *pFrustum, const float3 &vHintDir, CRenderableVisibility *pVisibility, UINT bmLayerMask, CRenderableVisibility *pReference = NULL, IXCamera *pCamera = NULL);

	IXMaterialSystem* getMaterialSystem();

	CParticlePlayerEmitter* allocPlayerEmitter(CParticlePlayer *pPlayer);

private:
	void onMaterialEmissivityChanged(const IXMaterial *pMaterial);
	void onMaterialTransparencyChanged(const IXMaterial *pMaterial);

protected:
	IXCore *m_pCore;

	Map<String, CParticleEffect> m_mapEffects;

	CMaterialChangedEventListener *m_pMaterialChangedEventListener; 

	struct PlayerQueueItem
	{
		enum
		{
			PQI_ADD,
			PQI_REMOVE
		} cmd;
		CParticlePlayer *pPlayer;
	};
	SpinLock m_slPlayerPool;
	MemAlloc<CParticlePlayer> m_poolPlayers;
	Queue<PlayerQueueItem> m_queuePlayers;
	Array<CParticlePlayer*> m_aPlayers;


	SpinLock m_slPlayerEmitterPool;
	MemAlloc<CParticlePlayerEmitter> m_poolPlayerEmitters;

	SpinLock m_slEmittersPool;
	MemAlloc<CParticleEffectEmitter> m_poolEmitters;

	XVertexShaderHandler *m_pVertexShaderWithTransformHandler = NULL;
	XVertexShaderHandler *m_pVertexShaderNoTransformHandler = NULL;
	IGXDevice *m_pDevice = NULL;
	IXMaterialSystem *m_pMaterialSystem = NULL;


	IXScene *m_pScene = NULL;
	IXSceneObjectType *m_pObjectType = NULL;
	IXSceneFeature *m_pFeatureOpaque = NULL;
	IXSceneFeature *m_pFeatureTransparent = NULL;
	IXSceneFeature *m_pFeatureSelfillum = NULL;
	IXSceneQuery *m_pOpaqueQuery = NULL;
	IXSceneQuery *m_pTransparentQuery = NULL;
	IXSceneQuery *m_pSelfillumQuery = NULL;
};

#endif
