#ifndef __DYNAMICMODEL_H
#define __DYNAMICMODEL_H

#include <xcommon/resource/IXModel.h>
#include <xcommon/IXScene.h>
#include "DynamicModelShared.h"
#include "ModelOverlay.h"

class CDynamicModel final: public IXUnknownImplementation<IXDynamicModel>
{
public:
	CDynamicModel(CDynamicModelProvider *pProvider, CDynamicModelShared *pShared);
	~CDynamicModel();

	bool XMETHODCALLTYPE isEnabled() const override;
	void XMETHODCALLTYPE enable(bool yesNo) override;

	IXAnimatedModel* XMETHODCALLTYPE asAnimatedModel() override;
	IXDynamicModel* XMETHODCALLTYPE asDynamicModel() override;
	IXStaticModel* XMETHODCALLTYPE asStaticModel() override;


	float3 XMETHODCALLTYPE getPosition() const override;
	void XMETHODCALLTYPE setPosition(const float3 &vPos) override;

	SMQuaternion XMETHODCALLTYPE getOrientation() const override;
	void XMETHODCALLTYPE setOrientation(const SMQuaternion &qRot) override;

	float XMETHODCALLTYPE getScale() const override;
	void XMETHODCALLTYPE setScale(float fScale) override;

	UINT XMETHODCALLTYPE getSkin() const override;
	void XMETHODCALLTYPE setSkin(UINT uSkin) override;

	float3 XMETHODCALLTYPE getLocalBoundMin() const override;
	float3 XMETHODCALLTYPE getLocalBoundMax() const override;
	SMAABB XMETHODCALLTYPE getLocalBound() const override;

	float4 XMETHODCALLTYPE getColor() const override;
	void XMETHODCALLTYPE setColor(const float4 &vColor) override;

	UINT XMETHODCALLTYPE getPhysboxCount(UINT uPartIndex = 0) const override;
	const IModelPhysbox* XMETHODCALLTYPE getPhysBox(UINT id, UINT uPartIndex = 0) const override;
	const IXResourceModel* XMETHODCALLTYPE getResource(UINT uIndex = 0) override;

	void XMETHODCALLTYPE render(UINT uLod, XMODEL_FEATURE bmFeatures) override;

	CDynamicModelShared* getShared();
	void initGPUresources();

	UINT getPSPcount(UINT uLod) const;
	SMPLANE getPSP(UINT uLod, UINT uPlane) const;
	XMODEL_FEATURE getFeatures(UINT uLod) const;
	IXMaterial* getTransparentMaterial(UINT uLod);

	void onFeaturesChanged();

	bool XMETHODCALLTYPE rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut = NULL, float3 *pvNormal = NULL, bool isRayInWorldSpace = true, bool bReturnNearestPoint = false) override;

	void XMETHODCALLTYPE setLayer(UINT uLayer) override;
	UINT XMETHODCALLTYPE getLayer() override;

	CModelOverlay* getOverlay();
	void setOverlay(CModelOverlay *pOverlay);

	bool isStatic();

private:
	CDynamicModelProvider *m_pProvider;
	CDynamicModelShared *m_pShared;
	IGXDevice *m_pDevice;
	IXSceneObject *m_pSceneObject = NULL;

	IGXConstantBuffer *m_pWorldBuffer = NULL;
	bool m_isWorldDirty = true;

	IGXConstantBuffer *m_pColorBuffer = NULL;
	bool m_isColorDirty = true;

	float3_t m_vPosition;
	SMQuaternion m_qRotation;
	UINT m_uSkin = 0;
	float4_t m_vColor{1.0f, 1.0f, 1.0f, 1.0f};
	bool m_isEnabled = true;
	float m_fScale = 1.0f;

	mutable bool m_isLocalAABBvalid = false;
	mutable float3_t m_vLocalMin;
	mutable float3_t m_vLocalMax;

	UINT m_uLayer = 0;

	CModelOverlay *m_pOverlay = NULL;

private:
	void _updateAABB() const;

	void XMETHODCALLTYPE FinalRelease() override;
};

#endif
