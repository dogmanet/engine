
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "Camera.h"
#include <xcommon/IXRenderable.h>

CFrustum::CFrustum()
{
	memset(m_isPointValid, 0, sizeof(m_isPointValid));
}

void CFrustum::update(const float4x4 &mView, const float4x4 &mProj)
{
	float4x4 matComb = mView *mProj;

	// right (-x)
	m_aFrustumPlanes[0].x = matComb._14 - matComb._11;
	m_aFrustumPlanes[0].y = matComb._24 - matComb._21;
	m_aFrustumPlanes[0].z = matComb._34 - matComb._31;
	m_aFrustumPlanes[0].w = matComb._44 - matComb._41;

	// left (+x)
	m_aFrustumPlanes[1].x = matComb._14 + matComb._11;
	m_aFrustumPlanes[1].y = matComb._24 + matComb._21;
	m_aFrustumPlanes[1].z = matComb._34 + matComb._31;
	m_aFrustumPlanes[1].w = matComb._44 + matComb._41;

	// top (-y)
	m_aFrustumPlanes[2].x = matComb._14 - matComb._12;
	m_aFrustumPlanes[2].y = matComb._24 - matComb._22;
	m_aFrustumPlanes[2].z = matComb._34 - matComb._32;
	m_aFrustumPlanes[2].w = matComb._44 - matComb._42;

	// bottom (+y)
	m_aFrustumPlanes[3].x = matComb._14 + matComb._12;
	m_aFrustumPlanes[3].y = matComb._24 + matComb._22;
	m_aFrustumPlanes[3].z = matComb._34 + matComb._32;
	m_aFrustumPlanes[3].w = matComb._44 + matComb._42;
	
	// near (-z)
	m_aFrustumPlanes[4].x = /*matComb._14 - */matComb._13;
	m_aFrustumPlanes[4].y = /*matComb._24 - */matComb._23;
	m_aFrustumPlanes[4].z = /*matComb._34 - */matComb._33;
	m_aFrustumPlanes[4].w = /*matComb._44 - */matComb._43;

	// far (+z)
	m_aFrustumPlanes[5].x = matComb._14 - matComb._13;
	m_aFrustumPlanes[5].y = matComb._24 - matComb._23;
	m_aFrustumPlanes[5].z = matComb._34 - matComb._33;
	m_aFrustumPlanes[5].w = matComb._44 - matComb._43;

	//Нормализация плоскостей
	for(int i = 0; i < 6; ++i)
	{
		m_aFrustumPlanes[i] = SMPlaneNormalize(m_aFrustumPlanes[i]);
	}
	
	// printf("%.2f, %.2f, %.2f, %.2f\n", m_aFrustumPlanes[4].m_vNormal.x, m_aFrustumPlanes[4].m_vNormal.y, m_aFrustumPlanes[4].m_vNormal.z, m_aFrustumPlanes[4].m_fDistance);

	memset(m_isPointValid, 0, sizeof(m_isPointValid));
}

void CFrustum::update(const SMPLANE *pPlanes, bool isNormalized)
{
	for(int i = 0; i < 6; ++i)
	{
		m_aFrustumPlanes[i] = pPlanes[i];
		if(!isNormalized)
		{
			m_aFrustumPlanes[i] = SMPlaneNormalize(m_aFrustumPlanes[i]);
		}
	}

	memset(m_isPointValid, 0, sizeof(m_isPointValid));
}

bool CFrustum::pointInFrustum(const float3 &vPoint) const
{
	for(int i = 0; i < 6; ++i)
	{
		if(SMVector4Dot(m_aFrustumPlanes[i], float4(vPoint, 1.0f)) <= 0.0f)
		{
			return(false);
		}
	}
	return(true);
}

bool CFrustum::polyInFrustum(const float3 &p1, const float3 &p2, const float3 &p3) const
{
	/*if(pointInFrustum(p1) || pointInFrustum(p2) || pointInFrustum(p3))
	return true;*/

	for(int i = 0; i < 6; i++)
	{
		if(SMVector4Dot(m_aFrustumPlanes[i], float4(p1, 1.0f)) >= 0.0f) continue;
		if(SMVector4Dot(m_aFrustumPlanes[i], float4(p2, 1.0f)) >= 0.0f) continue;
		if(SMVector4Dot(m_aFrustumPlanes[i], float4(p3, 1.0f)) >= 0.0f) continue;
		return(false);
	}
	return(true);
}

bool CFrustum::polyInFrustumAbs(const float3 &p1, const float3 &p2, const float3 &p3) const
{
	if(pointInFrustum(p1) && pointInFrustum(p2) && pointInFrustum(p3))
		return true;

	return false;
}

bool CFrustum::sphereInFrustum(const float3 &vPoint, float radius) const
{
	for(int i = 0; i<6; ++i)
	{
		if(SMVector4Dot(m_aFrustumPlanes[i], float4(vPoint, 1.0f)) <= -radius)
		{
			return(false);
		}
	}
	return(true);
}

bool CFrustum::sphereInFrustumAbs(const float3 &vPoint, float radius) const
{
	for(int i = 0; i<6; i++)
	{
		if(SMVector4Dot(m_aFrustumPlanes[i], float4(vPoint, 1.0f)) > -radius)
		{
			return(false);
		}
	}
	return(true);
}

bool CFrustum::boxInFrustum(const SMAABB &aabb, bool *pIsStrict) const
{
	return(boxInFrustum(aabb.vMin, aabb.vMax, pIsStrict));
}

#if 0
static bool BoxInFrustum1(const SMPLANE *m_aFrustumPlanes, const float3 &vMin, const float3 &vMax)
{
	for(register int p = 0; p < 6; ++p)
	{
		auto &plane = m_aFrustumPlanes[p];
		if(SMVector3Dot(plane, vMin) + plane.w > 0) continue;
		if(SMVector3Dot(plane, float3(vMin.x, vMin.y, vMax.z)) + plane.w > 0) continue;
		if(SMVector3Dot(plane, float3(vMin.x, vMax.y, vMin.z)) + plane.w > 0) continue;
		if(SMVector3Dot(plane, float3(vMin.x, vMax.y, vMax.z)) + plane.w > 0) continue;

		if(SMVector3Dot(plane, float3(vMax.x, vMin.y, vMin.z)) + plane.w > 0) continue;
		if(SMVector3Dot(plane, float3(vMax.x, vMin.y, vMax.z)) + plane.w > 0) continue;
		if(SMVector3Dot(plane, float3(vMax.x, vMax.y, vMin.z)) + plane.w > 0) continue;
		if(SMVector3Dot(plane, vMax) + plane.w > 0) continue;
		return(false);
	}
	return(true);
}

static bool BoxInFrustum2(const CFrustumPlane *m_aFrustumPlanes, const float3 &vMin, const float3 &vMax)
{
	bool inside = true;


	for(int p = 0; p < 6; ++p)
	{
		auto &plane = m_aFrustumPlanes[p];
		//находим ближайшую к плоскости вершину
		//проверяем, если она находится за плоскостью, то объект вне врустума
		float d = SMVector3Sum(SMVectorMax(vMin * plane.m_vNormal, vMax * plane.m_vNormal)) + plane.m_fDistance;

		float d2 = max(vMin.x * plane.m_vNormal.x, vMax.x * plane.m_vNormal.x)
			+ max(vMin.y * plane.m_vNormal.y, vMax.y * plane.m_vNormal.y)
			+ max(vMin.z * plane.m_vNormal.z, vMax.z * plane.m_vNormal.z)
			+ plane.m_fDistance;
		inside &= d > 0;

		/*if(d < 0.0f)
		{
		return(false);
		}*/
	}
	return inside;
}


bool CFrustum::boxInFrustum(const float3 &vMin, const float3 &vMax, bool isStrict) const
{
	if(isStrict)
	{
		return(false);
	}

	bool b0 = BoxInFrustum1(m_aFrustumPlanes, vMin, vMax);
	//bool b1 = BoxInFrustum2(m_aFrustumPlanes, vMin, vMax);

	//assert(b0 == b1);

	//return(b0);

	/*for(register int p = 0; p < 6; ++p)
	{
		auto &plane = m_aFrustumPlanes[p];
		if(SMVector3Dot(plane.m_vNormal, vMin) + plane.m_fDistance > 0) continue;
		if(SMVector3Dot(plane.m_vNormal, float3(vMin.x, vMin.y, vMax.z)) + plane.m_fDistance > 0) continue;
		if(SMVector3Dot(plane.m_vNormal, float3(vMin.x, vMax.y, vMin.z)) + plane.m_fDistance > 0) continue;
		if(SMVector3Dot(plane.m_vNormal, float3(vMin.x, vMax.y, vMax.z)) + plane.m_fDistance > 0) continue;

		if(SMVector3Dot(plane.m_vNormal, float3(vMax.x, vMin.y, vMin.z)) + plane.m_fDistance > 0) continue;
		if(SMVector3Dot(plane.m_vNormal, float3(vMax.x, vMin.y, vMax.z)) + plane.m_fDistance > 0) continue;
		if(SMVector3Dot(plane.m_vNormal, float3(vMax.x, vMax.y, vMin.z)) + plane.m_fDistance > 0) continue;
		if(SMVector3Dot(plane.m_vNormal, vMax) + plane.m_fDistance > 0) continue;
		return(false);
	}
	return(true);*/
	auto fn = isStrict ? SMVectorMin : SMVectorMax;

	//bool inside = true;


	for(int p = 0; p < 6; ++p)
	{
		auto &plane = m_aFrustumPlanes[p];
		//находим ближайшую к плоскости вершину
		//проверяем, если она находится за плоскостью, то объект вне врустума
		float d = SMVector3Sum(fn(vMin * plane.m_vNormal, vMax * plane.m_vNormal)) + plane.m_fDistance;

		float d2 = max(vMin.x * plane.m_vNormal.x, vMax.x * plane.m_vNormal.x)
			+ max(vMin.y * plane.m_vNormal.y, vMax.y * plane.m_vNormal.y)
			+ max(vMin.z * plane.m_vNormal.z, vMax.z * plane.m_vNormal.z)
			+ plane.m_fDistance;
		//inside &= d > 0;

		if(d <= 0.0f)
		{
			assert(!b0);
			return(false);
		}
	}
	//assert(inside == b0);
	//return inside;

	assert(b0);
	return(true);
}
#endif
bool CFrustum::boxInFrustum(const float3 &vMin, const float3 &vMax, bool *pIsStrict) const
{
	bool isVisible = true;
	bool isVisibleStrict = true;

	for(int p = 0; p < 6; ++p)
	{
		auto &plane = m_aFrustumPlanes[p];
		//находим ближайшую к плоскости вершину
		//проверяем, если она находится за плоскостью, то объект вне врустума
		float3 vTmpMin = vMin * plane;
		float3 vTmpMax = vMax * plane;
		float d = SMVector4Dot(float4(1.0f, 1.0f, 1.0f, plane.w), float4(SMVectorMax(vTmpMin, vTmpMax), 1.0f));

		isVisible &= d > 0.0f;

		if(isVisible && pIsStrict)
		{
			d = SMVector4Dot(float4(1.0f, 1.0f, 1.0f, plane.w), float4(SMVectorMin(vTmpMin, vTmpMax), 1.0f));
			isVisibleStrict &= d > 0.0f;
		}
	}

	//bool b0 = BoxInFrustum1(m_aFrustumPlanes, vMin, vMax);

	//assert(isVisible == b0);

	if(pIsStrict)
	{
		*pIsStrict = isVisible && isVisibleStrict;
	}

	return(isVisible);
}

bool CFrustum::frustumInFrustum(const IXFrustum *pOther) const
{
	for(register int p = 0; p < 6; ++p)
	{
		auto &plane = m_aFrustumPlanes[p];
		bool isFound = false;
		for(register int j = 0; j < 8; ++j)
		{
			if(SMVector4Dot(plane, float4(pOther->getPoint(j), 1.0f)) > 0.0f)
			{
				isFound = true;
				break;
			}
		}
		if(isFound)
		{
			continue;
		}
		return(false);
	}

	return(true);
}

float3 CFrustum::getPoint(int iPoint) const
{
	assert(iPoint >= 0 && iPoint < 8);
	if(iPoint >= 0 && iPoint < 8)
	{
		if(!m_isPointValid[iPoint])
		{
			int p0 = (iPoint & 1);
			int p1 = (iPoint & 2) ? 2 : 3;
			int p2 = (iPoint & 4) ? 4 : 5;

			float fDet = SMMatrix3x3Determinant(SMMATRIX(
				float4(m_aFrustumPlanes[p0], 0.0f),
				float4(m_aFrustumPlanes[p1], 0.0f),
				float4(m_aFrustumPlanes[p2], 0.0f),
				float4(0.0f, 0.0f, 0.0f, 1.0f)
				));
			float3 &vPoint = m_aPoints[iPoint];
			vPoint.x = SMMatrix3x3Determinant(SMMATRIX(
				float4(-m_aFrustumPlanes[p0].w, m_aFrustumPlanes[p0].y, m_aFrustumPlanes[p0].z, 0.0f),
				float4(-m_aFrustumPlanes[p1].w, m_aFrustumPlanes[p1].y, m_aFrustumPlanes[p1].z, 0.0f),
				float4(-m_aFrustumPlanes[p2].w, m_aFrustumPlanes[p2].y, m_aFrustumPlanes[p2].z, 0.0f),
				float4(0.0f, 0.0f, 0.0f, 1.0f)
				));
			vPoint.y = SMMatrix3x3Determinant(SMMATRIX(
				float4(m_aFrustumPlanes[p0].x, -m_aFrustumPlanes[p0].w, m_aFrustumPlanes[p0].z, 0.0f),
				float4(m_aFrustumPlanes[p1].x, -m_aFrustumPlanes[p1].w, m_aFrustumPlanes[p1].z, 0.0f),
				float4(m_aFrustumPlanes[p2].x, -m_aFrustumPlanes[p2].w, m_aFrustumPlanes[p2].z, 0.0f),
				float4(0.0f, 0.0f, 0.0f, 1.0f)
				));
			vPoint.z = SMMatrix3x3Determinant(SMMATRIX(
				float4(m_aFrustumPlanes[p0].x, m_aFrustumPlanes[p0].y, -m_aFrustumPlanes[p0].w, 0.0f),
				float4(m_aFrustumPlanes[p1].x, m_aFrustumPlanes[p1].y, -m_aFrustumPlanes[p1].w, 0.0f),
				float4(m_aFrustumPlanes[p2].x, m_aFrustumPlanes[p2].y, -m_aFrustumPlanes[p2].w, 0.0f),
				float4(0.0f, 0.0f, 0.0f, 1.0f)
				));

			vPoint /= fDet;

			m_isPointValid[iPoint] = true;
		}
		return(m_aPoints[iPoint]);
	}
	
	return(float3());
}

float3 CFrustum::getCenter() const
{
	assert(!"Not implemented!");
	return m_vCenter;
}

UINT CFrustum::getPlaneCount() const
{
	return(6);
}
const SMPLANE& CFrustum::getPlaneAt(UINT idx) const
{
	return(m_aFrustumPlanes[idx]);
}

//void CFrustum::setPoint(int iNumPoint, const float3 *pPoint)
//{
//	if(iNumPoint >= 0 && iNumPoint < 8)
//		m_aPoints[iNumPoint] = *pPoint;
//}
//
//void CFrustum::setCenter(const float3 *pCenter)
//{
//	m_vCenter = *pCenter;
//}

//##########################################################################

CCamera::CCamera(IXRenderableVisibility *pVisibility):
	m_pFrustum(new CFrustum()),
	m_pVisibility(pVisibility)
{
	add_ref(m_pVisibility);
}
CCamera::~CCamera()
{
	mem_release(m_pFrustum);
	mem_release(m_pVisibility);
}

void XMETHODCALLTYPE CCamera::setOrientation(const SMQuaternion &q)
{
	m_qRotation = q;

	m_vRight = q * float3(1.0f, 0.0f, 0.0f);
	m_vUp = q * float3(0.0f, 1.0f, 0.0f);
	m_vLook = q * float3(0.0f, 0.0f, 1.0f);

	m_vPitchYawRoll.x = SMRightAngleBetweenVectors(m_vLook, SMVector3Normalize(float3(1.0f, 0.0f, 1.0f) * m_vLook), m_vRight);
	if(m_vPitchYawRoll.x >= SM_PI)
	{
		m_vPitchYawRoll.x -= SM_2PI;
	}
	m_vPitchYawRoll.y = -SMRightAngleBetweenVectors(float3(0.0f, 0.0f, 1.0f), SMVector3Normalize(float3(1.0f, 0.0f, 1.0f) * m_vLook), float3(0.0f, 1.0f, 0.0f));
	m_vPitchYawRoll.z = 0.0f;
}

const SMQuaternion& XMETHODCALLTYPE CCamera::getOrientation()
{
	return(m_qRotation);
}

const SMMATRIX& XMETHODCALLTYPE CCamera::getViewMatrix() const
{
	m_mView = SMMatrixLookToLH(m_vPosition, m_vLook, m_vUp);
	return(m_mView);
}


const float3& XMETHODCALLTYPE CCamera::getPosition() const
{
	return(m_vPosition);
}

void XMETHODCALLTYPE CCamera::setPosition(const float3 &vPos)
{
	m_vPosition = vPos;
}

const float3& XMETHODCALLTYPE CCamera::getRight() const
{
	return(m_vRight);
}

const float3& XMETHODCALLTYPE CCamera::getUp() const
{
	return(m_vUp);
}

const float3& XMETHODCALLTYPE CCamera::getLook() const
{
	return(m_vLook);
}

const float3& XMETHODCALLTYPE CCamera::getRotation() const
{
	return(m_vPitchYawRoll);
}

void XMETHODCALLTYPE CCamera::setFOV(float fov)
{
	m_fFOV = fov;
}

void XMETHODCALLTYPE CCamera::setFOVmultiplier(float fFOVmultiplier)
{
	m_fFOVmultiplier = fFOVmultiplier;
}

float XMETHODCALLTYPE CCamera::getFOV() const
{
	return(m_fFOV);
}

float XMETHODCALLTYPE CCamera::getFOVmultiplier() const
{
	return(m_fFOVmultiplier);
}

float XMETHODCALLTYPE CCamera::getEffectiveFOV() const
{
	return(m_fFOV * m_fFOVmultiplier);
}

void XMETHODCALLTYPE CCamera::updateFrustum(const SMMATRIX &mProjection)
{
	m_pFrustum->update(getViewMatrix(), mProjection);
}

IXFrustum* XMETHODCALLTYPE CCamera::getFrustum()
{
	m_pFrustum->AddRef();
	return(m_pFrustum);
}

void XMETHODCALLTYPE CCamera::setNear(float fNear)
{
	m_fNear = fNear;
}
void XMETHODCALLTYPE CCamera::setFar(float fFar)
{
	m_fFar = fFar;
}
float XMETHODCALLTYPE CCamera::getNear() const
{
	return(m_fNear);
}
float XMETHODCALLTYPE CCamera::getFar() const
{
	return(m_fFar);
}

void XMETHODCALLTYPE CCamera::getVisibility(IXRenderableVisibility **ppVisibility)
{
	add_ref(m_pVisibility);
	*ppVisibility = m_pVisibility;
}

void XMETHODCALLTYPE CCamera::updateVisibility()
{
	m_pVisibility->updateForCamera(this);
}

void XMETHODCALLTYPE CCamera::setProjectionMode(XCAMERA_POJECTION_MODE mode)
{
	m_projectionMode = mode;
}
XCAMERA_POJECTION_MODE XMETHODCALLTYPE CCamera::getProjectionMode() const
{
	return(m_projectionMode);
}

void XMETHODCALLTYPE CCamera::setScale(float fScale)
{
	m_fScale = fScale;
}
float XMETHODCALLTYPE CCamera::getScale() const
{
	return(m_fScale);
}

const SMMATRIX& XMETHODCALLTYPE CCamera::getProjMatrix() const
{
	return(m_mProj);
}

void XMETHODCALLTYPE CCamera::updateProjection(UINT uTargetWidth, UINT uTargetHeight)
{
	float fWinWidth = (float)uTargetWidth;
	float fWinHeight = (float)uTargetHeight;

	if(m_projectionMode == XCPM_PERSPECTIVE)
	{
		m_mProj = SMMatrixPerspectiveFovLH(getEffectiveFOV(), fWinWidth / fWinHeight, getFar(), getNear());
	}
	else if(m_projectionMode == XCPM_ORTHOGONAL)
	{
		m_mProj = /*SMMatrixTranslation(-0.5f, -0.5f, 0.0f) * */SMMatrixOrthographicLH(fWinWidth * getScale(), fWinHeight * getScale(), getNear(), getFar());
	}

	updateFrustum(m_mProj);
}

void XMETHODCALLTYPE CCamera::setLayerMask(UINT bmLayerMask)
{
	m_bmLayerMask = bmLayerMask;
}
UINT XMETHODCALLTYPE CCamera::getLayerMask()
{
	return(m_bmLayerMask);
}
