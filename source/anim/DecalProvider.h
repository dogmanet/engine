#ifndef __DECALPROVIDER_H
#define __DECALPROVIDER_H

#include <xcommon/resource/IXDecalProvider.h>
#include <xcommon/IXCore.h>
#include "RenderableVisibility.h"
//#include <mtrl/IXMaterialSystem.h>
//#include <common/ConcurrentQueue.h>
//#include <xcommon/XEvents.h>
#include <xcommon/IXScene.h>
#include <common/queue.h>
#include "Decal.h"

#define SIN_45_DEGREES 0.70710678118654752440084436210485f

class CDecalProvider: public IXUnknownImplementation<IXDecalProvider>
{
public:
	CDecalProvider(IXCore *pCore, CDynamicModelProvider *pProviderDynamic);
	~CDecalProvider();

	void render(CRenderableVisibility *pVisibility);

	void XMETHODCALLTYPE shootDecal(XDECAL_TYPE type, const float3 &vWorldPos, const float3 &vNormal, float fScale = 1.0f, const float3 *pvSAxis = NULL) override;

	void computeVisibility(const float3 &vHintDir, CRenderableVisibility *pVisibility);

	void setDevice(IGXDevice *pDevice);

	void onDecalReleased(CDecal *pDecal);

	void onDecalEmptied(CDecal *pDecal);

	void updateDecal(CDecal *pDecal);

	void update();

	CModelOverlay* allocOverlay(CDecal *pDecal, IXMaterial *pMaterial, Array<XResourceModelStaticVertex> &aVertices, const float3_t &vNormal);
	void freeOverlay(CModelOverlay *pOverlay);

private:
	struct DecalTexRange
	{
		int xmin;
		int ymin;
		int xmax;
		int ymax;
	};

	struct DecalType
	{
		Array<DecalTexRange> aTextures;
		float fBaseScale = 1.0f;
		IXMaterial *pMaterial = NULL;
	};

	/*struct TempDecal
	{
		CDecal *pDecal;
	};*/

private:
	IXCore *m_pCore = NULL;
	CDynamicModelProvider *m_pProviderDynamic = NULL;
	IGXDevice *m_pDevice = NULL;
	IXMaterialSystem *m_pMaterialSystem = NULL;
	IXRender *m_pRender = NULL;
	IXRenderUtils *m_pRenderUtils = NULL;
	IXSceneQuery *m_pQuery = NULL;

	IGXConstantBuffer *m_pWorldBuffer = NULL;

	IGXBlendState *m_pBlendState = NULL;
	IGXDepthStencilState *m_pDSState = NULL;

	DecalType m_aDecalTypes[XDT__COUNT];

#ifdef DECAL_DEBUG_DRAW
	IXGizmoRenderer *m_pDebugRenderer = NULL;
#endif

	MemAlloc<CDecal> m_memDecals;
	MemAlloc<CModelOverlay> m_memOverlays;
	SpinLock m_slMemDecals;
	SpinLock m_slMemOverlays;

	Array<CDecal*> m_aTempDecals;
	SpinLock m_slTempDecals;

	Queue<CDecal*> m_qDecalsForUpdate;

private:
	void loadDecals();

	const DecalType* getDecalType(XDECAL_TYPE type);

	void computeBasis(SMMATRIX *pmOut, const float3_t &vSurfaceNormal, const float3 *pvSAxis = NULL);

	CDecal* allocDecal();

	void addTempDecal(CDecal *pDecal);
};

#endif
