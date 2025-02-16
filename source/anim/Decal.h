#ifndef __DECAL_H
#define __DECAL_H

#include <xcommon/resource/IXDecal.h>
#include <xcommon/IXScene.h>
#include <xcommon/render/IXRender.h>
#include "ModelOverlay.h"

#define DECAL_POINTS 4

// #define DECAL_DEBUG_DRAW

class CDynamicModel;
class CDecalProvider;

class CDecal final: public IXUnknownImplementation<IXDecal>
{
public:
	CDecal(IXSceneQuery *pSceneQuery, IXRender *pRender, CDecalProvider *pProvider);
	~CDecal();

	bool XMETHODCALLTYPE isEnabled() const override;
	void XMETHODCALLTYPE enable(bool yesNo) override;

	float3 XMETHODCALLTYPE getPosition() const override;
	void XMETHODCALLTYPE setPosition(const float3 &vPos) override;

	SMQuaternion XMETHODCALLTYPE getOrientation() const override;
	void XMETHODCALLTYPE setOrientation(const SMQuaternion &qRot) override;

	float3 XMETHODCALLTYPE getLocalBoundMin() const override;
	float3 XMETHODCALLTYPE getLocalBoundMax() const override;
	SMAABB XMETHODCALLTYPE getLocalBound() const override;

	bool XMETHODCALLTYPE rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut = NULL, float3 *pvNormal = NULL, bool isRayInWorldSpace = true, bool bReturnNearestPoint = false) override;

	void XMETHODCALLTYPE setLayer(UINT uLayer) override;
	UINT XMETHODCALLTYPE getLayer() override;

	void XMETHODCALLTYPE setHeight(float fHeight) override;
	float XMETHODCALLTYPE getHeight() override;

	void XMETHODCALLTYPE setTextureRangeU(const float2_t &vRange) override;
	float2_t XMETHODCALLTYPE getTextureRangeU() override;

	void XMETHODCALLTYPE setTextureRangeV(const float2_t &vRange) override;
	float2_t XMETHODCALLTYPE getTextureRangeV() override;

	void setMaterial(IXMaterial *pMaterial);
	void setCorner(UINT uCorner, const float2_t &vCorner);

	void update(
#ifdef DECAL_DEBUG_DRAW
		IXGizmoRenderer *pDebugRenderer
#endif
	);

	// Must be called only for overlays removed with model
	void onOverlayRemoved(CModelOverlay *pOverlay);

private:
	IXSceneQuery *m_pSceneQuery = NULL;
	IXRender *m_pRender = NULL;
	CDecalProvider *m_pProvider = NULL;

	IXMaterial *m_pMaterial = NULL;
	
	float2_t m_avCorners[4];

	SMQuaternion m_qRot;
	float3_t m_vPos;
	
	float m_fHeight = 0.1f;

	UINT m_uLayer = 0;

	float2_t m_vTexRangeU = float2_t(0.0f, 1.0f);
	float2_t m_vTexRangeV = float2_t(0.0f, 1.0f);

	struct Overlay
	{
		CDynamicModel *pModel;
		CModelOverlay *pOverlay;
	};
	Array<Overlay> m_aOverlays;

	bool m_isEnabled = true;

	bool m_isDirty = false;

private:
	void XMETHODCALLTYPE FinalRelease() override;
	void spawnOverlayForModel(CDynamicModel *pModel, IXFrustum *pFrustum
#ifdef DECAL_DEBUG_DRAW
		, IXGizmoRenderer *pDebugRenderer
#endif
	);

	void removeOverlays();

	void setDirty();
};

#endif
