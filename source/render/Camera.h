
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#ifndef __CAMERA_H
#define __CAMERA_H

#include <xcommon/IXCamera.h>

//класс описывающий фрустум
class CFrustum final: public IXUnknownImplementation<IXFrustum>
{
public:
	SX_ALIGNED_OP_MEM();

	CFrustum();

	void update(const float4x4 &mView, const float4x4 &mProj) override;
	void update(const SMPLANE *pPlanes, bool isNormalized = false) override;

	bool pointInFrustum(const float3 &vPoint) const override;
	bool polyInFrustum(const float3 &vPount1, const float3 &vPount2, const float3 &vPount3) const override;
	bool polyInFrustumAbs(const float3 &vPount1, const float3 &vPount2, const float3 &vPount3) const override;

	bool sphereInFrustum(const float3 &vPount, float fRadius) const override;

	bool sphereInFrustumAbs(const float3 &vPount, float fRadius) const override;
	bool boxInFrustum(const float3 &vMin, const float3 &vMax, bool *pIsStrict = NULL) const override;
	bool boxInFrustum(const SMAABB &aabb, bool *pIsStrict = NULL) const override;

	bool frustumInFrustum(const IXFrustum *pOther) const override;

	float3 getPoint(int iNumPoint) const override;
	float3 getCenter() const override;

	UINT getPlaneCount() const override;
	const SMPLANE& getPlaneAt(UINT idx) const override;

	/*void setPoint(int iNumPoint, const float3 *pPoint);
	void setCenter(const float3 *pCenter);*/

private:

	SMPLANE m_aFrustumPlanes[6];

	mutable float3 m_aPoints[8];
	float3 m_vCenter;

	mutable bool m_isPointValid[8];
};

//**************************************************************************

class CCamera final: public IXUnknownImplementation<IXCamera>
{
public:
	CCamera(IXRenderableVisibility *pVisibility);
	~CCamera();

	SX_ALIGNED_OP_MEM();

	void XMETHODCALLTYPE setOrientation(const SMQuaternion &qOrientation) override;

	const SMQuaternion& XMETHODCALLTYPE getOrientation() override;

	const SMMATRIX& XMETHODCALLTYPE getViewMatrix() const override;//получаем матрицу вида

	const float3& XMETHODCALLTYPE getPosition() const override;
	void XMETHODCALLTYPE setPosition(const float3 &vPos) override;

	const float3& XMETHODCALLTYPE getRight() const override;
	const float3& XMETHODCALLTYPE getUp() const override;
	const float3& XMETHODCALLTYPE getLook() const override;

	const float3& XMETHODCALLTYPE getRotation() const override;//?

	void XMETHODCALLTYPE setFOV(float fFOV) override;
	void XMETHODCALLTYPE setFOVmultiplier(float fFOVmultiplier) override;

	float XMETHODCALLTYPE getFOV() const override;
	float XMETHODCALLTYPE getFOVmultiplier() const override;

	float XMETHODCALLTYPE getEffectiveFOV() const override;

	void XMETHODCALLTYPE updateFrustum(const SMMATRIX &mProjection) override;
	IXFrustum* XMETHODCALLTYPE getFrustum() override;

	void XMETHODCALLTYPE setNear(float fNear) override;
	void XMETHODCALLTYPE setFar(float fFar) override;
	float XMETHODCALLTYPE getNear() const override;
	float XMETHODCALLTYPE getFar() const override;

	void XMETHODCALLTYPE getVisibility(IXRenderableVisibility **ppVisibility) override;
	void XMETHODCALLTYPE updateVisibility() override;

	void XMETHODCALLTYPE setProjectionMode(XCAMERA_POJECTION_MODE mode) override;
	XCAMERA_POJECTION_MODE XMETHODCALLTYPE getProjectionMode() const override;

	void XMETHODCALLTYPE setScale(float fScale) override;
	float XMETHODCALLTYPE getScale() const override;

	const SMMATRIX& XMETHODCALLTYPE getProjMatrix() const override;
	void XMETHODCALLTYPE updateProjection(UINT uTargetWidth, UINT uTargetHeight) override;

	void XMETHODCALLTYPE setLayerMask(UINT bmLayerMask) override;
	UINT XMETHODCALLTYPE getLayerMask() override;

private:
	CFrustum *m_pFrustum;	//!< фрустум этой камеры

	IXRenderableVisibility *m_pVisibility = NULL;

	float3 m_vRight = float3(1.0f, 0.0f, 0.0f);
	float3 m_vUp = float3(0.0f, 1.0f, 0.0f);
	float3 m_vLook = float3(0.0f, 0.0f, 1.0f);

	float3 m_vPosition = float3(0.0f, 0.0f, 0.0f);

	float3 m_vPitchYawRoll = float3(0.0f, 0.0f, 0.0f);

	SMQuaternion m_qRotation;

	mutable float4x4 m_mView;
	float4x4 m_mProj;

	float m_fFOV = SM_PIDIV4;
	float m_fFOVmultiplier = 1.0f;

	float m_fNear = 0.025f;
	float m_fFar = 800.0f;

	float m_fScale = 1.0f;
	XCAMERA_POJECTION_MODE m_projectionMode = XCPM_PERSPECTIVE;

	UINT m_bmLayerMask = 0x1;
};

#endif
