
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "light.h"
#include "LightSystem.h"
#include <xcommon/IXRenderable.h>
#include <xcommon/physics/IXPhysics.h>
#include <core/sxcore.h>


//##########################################################################


CXLight::CXLight(CLightSystem *pLightSystem):
	m_pLightSystem(pLightSystem)
{
	IXRender *pRender = m_pLightSystem->getRender();
	pRender->newVisData(&m_pVisibility);
	pRender->newFrustum(&m_pFrustum);

	((IXPhysics*)Core_GetIXCore()->getPluginManager()->getInterface(IXPHYSICS_GUID))->newMutationObserver(&m_pMutationObserver);
}
CXLight::~CXLight()
{
	mem_release(m_pFrustum);
	mem_release(m_pShape);
	mem_release(m_pVSData);
	mem_release(m_pPSData);
	mem_release(m_pVisibility);
	mem_release(m_pMutationObserver);
}

LIGHT_TYPE CXLight::getType()
{
	return(m_type);
}

float3 CXLight::getColor()
{
	return(m_vColor);
}
void CXLight::setColor(const float3 &vColor)
{
	if((float3)m_vColor == vColor)
	{
		return;
	}
	m_vColor = vColor;
	m_pMutationObserver->setHalfExtents(float3(getMaxDistance() * 0.5f));
	m_isVSDataDirty = true;
	m_isPSDataDirty = true;
	m_dirty = LRT_ALL;
}

float3 CXLight::getPosition()
{
	return(m_vPosition);
}
void CXLight::setPosition(const float3 &vPosition)
{
	if(m_vPosition == vPosition)
	{
		return;
	}
	m_vPosition = vPosition;
	m_pMutationObserver->setPosition(vPosition);
	m_isVSDataDirty = true;
	m_isPSDataDirty = true;
	m_dirty = LRT_ALL;
}

float CXLight::getShadowIntencity()
{
	return(m_fShadowIntensity);
}
void CXLight::setShadowIntencity(float fShadowIntencity)
{
	if(m_fShadowIntensity == fShadowIntencity)
	{
		return;
	}
	m_fShadowIntensity = fShadowIntencity;
	m_isPSDataDirty = true;
}

bool CXLight::isEnabled()
{
	return(m_isEnable);
}
void CXLight::setEnabled(bool isEnabled)
{
	if(m_isEnable == isEnabled)
	{
		return;
	}
	m_isEnable = isEnabled;
	m_pMutationObserver->setEnabled(isEnabled);
	m_dirty = LRT_ALL;
}

bool CXLight::isShadowDynamic()
{
	return(m_isShadowDynamic);
}
void CXLight::setShadowDynamic(bool isDynamic)
{
	m_isShadowDynamic = isDynamic;
}

void CXLight::drawShape(IGXDevice *pDevice)
{
	assert(pDevice);

	if(m_pShape)
	{
		if(m_isVSDataDirty)
		{
			if(!m_pVSData)
			{
				m_pVSData = pDevice->createConstantBuffer(sizeof(SMMATRIX));
			}
			m_pVSData->update(&SMMatrixTranspose(getWorldTM()));
			m_isVSDataDirty = false;
		}

		IGXContext *pCtx = pDevice->getThreadContext();

		pCtx->setVSConstant(m_pVSData, SCR_OBJECT);

		m_pShape->draw();
	}
}

IGXConstantBuffer* CXLight::getConstants(IGXDevice *pDevice)
{
	if(m_isPSDataDirty)
	{
		updatePSConstants(pDevice);
		m_isPSDataDirty = false;
	}
	m_pPSData->AddRef();
	return(m_pPSData);
}

SMMATRIX CXLight::getWorldTM()
{
	return(SMMatrixScaling(float3(getMaxDistance())) * SMMatrixTranslation(m_vPosition));
}

IXLightSpot* CXLight::asSpot()
{
	if(m_type == LIGHT_TYPE_SPOT)
	{
		return((CXLightSpot*)this);
	}
	return(NULL);
}
IXLightSun* CXLight::asSun()
{
	if(m_type == LIGHT_TYPE_SUN)
	{
		return((CXLightSun*)this);
	}
	return(NULL);
}
IXLightPoint* CXLight::asPoint()
{
	if(m_type == LIGHT_TYPE_POINT)
	{
		return((CXLightPoint*)this);
	}
	return(NULL);
}

float CXLight::getMaxDistance()
{
	float fLum = SMVector3Dot(m_vColor, float3(0.298f, 0.585f, 0.117f));
	float fEps = 0.001f;

	return(sqrtf(fLum / fEps - 1.0f));
}

void CXLight::updateVisibility(IXCamera *pMainCamera, const float3 &vLPVmin, const float3 &vLPVmax, bool useLPV)
{
	updateFrustum();

	if(!(m_renderType & LRT_LPV))
	{
		if(useLPV && m_pFrustum->boxInFrustum(vLPVmin, vLPVmax))
		{
			m_renderType |= LRT_LPV;
		}
	}

	if(m_renderType != LRT_NONE)
	{
		m_pVisibility->updateForFrustum(m_pFrustum, 1 << m_uLayer);
	}
}

IXRenderableVisibility *CXLight::getVisibility()
{
	return(m_pVisibility);
}

//##########################################################################

CXLightPoint::CXLightPoint(CLightSystem *pLightSystem):
	CXLight(pLightSystem)
{
	m_type = LIGHT_TYPE_POINT;
	m_pShape = pLightSystem->getShapeSphere();
}

void XMETHODCALLTYPE CXLightPoint::FinalRelease()
{
	m_pLightSystem->destroyPoint(this);
}

void CXLightPoint::updatePSConstants(IGXDevice *pDevice)
{
	if(!m_pPSData)
	{
		m_pPSData = pDevice->createConstantBuffer(sizeof(m_dataPS));
	}
	m_dataPS.vLightColor = float4(m_vColor, getMaxDistance());
	m_dataPS.vLightPos = float4(m_vPosition, m_fShadowIntensity);
	m_pPSData->update(&m_dataPS);
}

void CXLightPoint::updateFrustum()
{
	//! @todo optimize me!
	float3 vPos = getPosition();
	float fRadius = getMaxDistance();

	// ax + by + cz + d = 0

	SMPLANE planes[] = {
		SMPLANE(-1.0f,  0.0f,  0.0f, fRadius + vPos.x),
		SMPLANE( 1.0f,  0.0f,  0.0f, fRadius - vPos.x),
		SMPLANE( 0.0f, -1.0f,  0.0f, fRadius + vPos.y),
		SMPLANE( 0.0f,  1.0f,  0.0f, fRadius - vPos.y),
		SMPLANE( 0.0f,  0.0f, -1.0f, fRadius + vPos.z),
		SMPLANE( 0.0f,  0.0f,  1.0f, fRadius - vPos.z),
	};

	m_pFrustum->update(planes, true);
}

void CXLightPoint::updateVisibility(IXCamera *pMainCamera, const float3 &vLPVmin, const float3 &vLPVmax, bool useLPV)
{
	if(!(m_renderType & LRT_LIGHT))
	{
		float3 vOrigin = getPosition();
		IXFrustum *pFrustum = pMainCamera->getFrustum();
		if(pFrustum->sphereInFrustum(vOrigin, getMaxDistance()))
		{
			m_renderType |= LRT_LIGHT;
		}
		mem_release(pFrustum);
	}

	CXLight::updateVisibility(pMainCamera, vLPVmin, vLPVmax, useLPV);
}

//##########################################################################

CXLightSun::CXLightSun(CLightSystem *pLightSystem):
	CXLight(pLightSystem)
{
	m_type = LIGHT_TYPE_SUN;


	IXRender *pRender = m_pLightSystem->getRender();
	pRender->newVisData(&m_pReflectiveVisibility);
	pRender->newVisData(&m_pTempVisibility);
	pRender->newFrustum(&m_pReflectiveFrustum);

	for(UINT i = 0; i < PSSM_MAX_SPLITS; ++i)
	{
		pRender->newFrustum(&m_pPSSMFrustum[i]);
	}
}

CXLightSun::~CXLightSun()
{
	mem_release(m_pReflectiveFrustum);
	mem_release(m_pReflectiveVisibility);
	mem_release(m_pTempVisibility);

	for(UINT i = 0; i < PSSM_MAX_SPLITS; ++i)
	{
		mem_release(m_pPSSMFrustum[i]);
	}

	m_pLightSystem->destroySun(this);
}

SMQuaternion CXLightSun::getDirection()
{
	return(m_qDirection);
}
void CXLightSun::setDirection(const SMQuaternion &qDirection)
{
	if(m_qDirection == qDirection)
	{
		return;
	}
	m_qDirection = qDirection;
	m_isVSDataDirty = true;
	m_dirty = LRT_ALL;
}

void CXLightSun::updatePSConstants(IGXDevice *pDevice)
{
	if(!m_pPSData)
	{
		m_pPSData = pDevice->createConstantBuffer(sizeof(m_dataPS));
	}

	m_dataPS.vLightColor = float4(m_vColor, getMaxDistance());
	m_dataPS.vLightPos = float4(m_qDirection * -LIGHTS_DIR_BASE, m_fShadowIntensity);
	m_pPSData->update(&m_dataPS);
}

float CXLightSun::getMaxDistance()
{
	return(m_fMaxDistance);
}

void CXLightSun::setMaxDistance(float fMax)
{
	m_fMaxDistance = fMax;
}

void CXLightSun::updateVisibility(IXCamera *pMainCamera, const float3 &vLPVmin, const float3 &vLPVmax, bool useLPV)
{
	m_renderType = LRT_LIGHT | LRT_LPV;

	CXLight::updateVisibility(pMainCamera, vLPVmin, vLPVmax, useLPV);
	
	static const int *lpv_cascades_count = GET_PCVAR_INT("lpv_cascades_count");

	if(lpv_cascades_count > 0)
	{
		m_pReflectiveVisibility->updateForFrustum(m_pReflectiveFrustum, 1 << m_uLayer);
	}

	static const int *r_pssm_splits = GET_PCVAR_INT("r_pssm_splits");

	for(int i = 1; i < *r_pssm_splits; ++i)
	{
		m_pTempVisibility->updateForFrustum(m_pPSSMFrustum[i - 1], 1 << m_uLayer);
		m_pVisibility->append(m_pTempVisibility);
	}
}

void CXLightSun::updateFrustum()
{
	assert(m_pCamera);
	
	float3 vLightDir = getDirection() * LIGHTS_DIR_BASE;
	float3 vUpDir = getDirection() * float3(1.0f, 0.0f, 0.0f);

	float3 vStart = m_pCamera->getPosition();
	float3 vDir = SMVector3Normalize(m_pCamera->getLook());
	float3 vRight = m_pCamera->getRight();
	float3 vUp = m_pCamera->getUp();

	static const int *r_win_width = GET_PCVAR_INT("r_win_width");
	static const int *r_win_height = GET_PCVAR_INT("r_win_height");
	//static const float *r_effective_fov = GET_PCVAR_FLOAT("r_default_fov");

	float fAspectRatio = (float)*r_win_width / (float)*r_win_height;
	float fFovTan = tanf(m_pCamera->getEffectiveFOV() * 0.5f);

	{
		static const float *s_pfRPSSMQuality = GET_PCVAR_FLOAT("r_pssm_quality");
		const float fSize = 512.0f * *s_pfRPSSMQuality;


		
		float r_near = m_pCamera->getNear();
		float r_far = m_pCamera->getFar();
		static const float *r_pssm_max_distance = GET_PCVAR_FLOAT("r_pssm_max_distance");

		static const int *r_pssm_splits = GET_PCVAR_INT("r_pssm_splits");
		if(*r_pssm_splits < 1)
		{
			Core_0SetCVarInt("r_pssm_splits", 1);
		}
		else if(*r_pssm_splits > PSSM_MAX_SPLITS)
		{
			Core_0SetCVarInt("r_pssm_splits", PSSM_MAX_SPLITS);
		}

		float fSplitWeight = 0.8f;
		float fShadowDistance = min(*r_pssm_max_distance, r_far);

		float fMaxDistanceSun = getMaxDistance();
		if(fShadowDistance > fMaxDistanceSun)
		{
			fShadowDistance = fMaxDistanceSun;
		}

		float aSplitDistances[PSSM_MAX_SPLITS];
		for(int i = 0; i < *r_pssm_splits; ++i)
		{
			float f = (i + 1.0f) / *r_pssm_splits;
			float fLogDistance = r_near * pow(fShadowDistance / r_near, f);
			float fUniformDistance = r_near + (fShadowDistance - r_near) * f;
			aSplitDistances[i] = lerpf(fUniformDistance, fLogDistance, fSplitWeight);

			if(i == 0)
			{
				m_splits[i].vNearFar = float2(r_near, aSplitDistances[i]);
			}
			else
			{
				m_splits[i].vNearFar = float2(aSplitDistances[i - 1], aSplitDistances[i]);
			}
		}


		
		for(int i = 0; i < *r_pssm_splits; ++i)
		{
			Split &split = m_splits[i];

			float3 vNearCenter = vStart + vDir * split.vNearFar.x;
			float3 vFarCenter = vStart + vDir * split.vNearFar.y;

			float fNearHalfHeight = fFovTan * split.vNearFar.x;
			float fFarHalfHeight = fFovTan * split.vNearFar.y;

			float fNearHalfWidth = fNearHalfHeight * fAspectRatio;
			float fFarHalfWidth = fFarHalfHeight * fAspectRatio;

			float3 vA = vNearCenter - vUp * fNearHalfHeight + vRight * fNearHalfWidth;
			float3 vB = vFarCenter + vUp * fFarHalfHeight - vRight * fFarHalfWidth;
			float3 vC = vFarCenter - vUp * fFarHalfHeight + vRight * fFarHalfWidth;

			float3 vCenter = SMTriangleCircumcenter3(vA, vB, vC);


			float fRadius = SMVector3Length(vCenter - vA);

			SMMATRIX mLight(SMMatrixTranspose(SMMATRIX(
				float4(SMVector3Normalize(SMVector3Cross(vUpDir, vLightDir))),
				float4(vUpDir),
				float4(vLightDir),
				float4(0.0f, 0.0f, 0.0f, 1.0f)
				)));
			SMMATRIX mLightInv = SMMatrixInverse(NULL, mLight);

			vCenter = SMVector3Transform(vCenter, mLight);
			float fStep = (fRadius * 2.0f / fSize);
			vCenter.x -= fmodf(vCenter.x, fStep);
			vCenter.y -= fmodf(vCenter.y, fStep);

			vCenter = SMVector3Transform(vCenter, mLightInv);

			float fMaxDistance = PSSM_LIGHT_FAR;

			split.mProj = SMMatrixOrthographicLH(fRadius * 2.0f, fRadius * 2.0f, PSSM_LIGHT_NEAR, fMaxDistance);
			split.mView = SMMatrixLookToLH(vCenter - vLightDir * (fMaxDistance /*- fRadius * 2*/ * 0.5f), vLightDir, vUpDir);

			m_mVPs[i] = SMMatrixTranspose(split.mView * split.mProj);

			if(i == 0)
			{
				m_pFrustum->update(split.mView, split.mProj);
			}
			else
			{
				m_pPSSMFrustum[i - 1]->update(split.mView, split.mProj);
			}
		}
	}

	static const int *lpv_cascades_count = GET_PCVAR_INT("lpv_cascades_count");
	int iCascades = *lpv_cascades_count;
	if(iCascades < 0)
	{
		iCascades = 0;
	}
	if(iCascades > LPV_CASCADES_COUNT)
	{
		iCascades = LPV_CASCADES_COUNT;
	}

	if(iCascades)
	{
		static const float *lpv_size_2 = GET_PCVAR_FLOAT("lpv_size_2");
		
		float3 vGridCenter = vStart + vDir * (LPV_GRID_SIZE / 2 - LPV_STEP_COUNT) * *lpv_size_2;
		//float fGridRadius = sqrtf(16.0f * 16.0f * 3.0f);
		float fDim = 16.0f * *lpv_size_2;
		float fGridRadius = sqrtf(fDim * fDim * 3.0f);


		SMMATRIX mLight(SMMatrixTranspose(SMMATRIX(
			float4(SMVector3Normalize(SMVector3Cross(vUpDir, vLightDir))),
			float4(vUpDir),
			float4(vLightDir),
			float4(0.0f, 0.0f, 0.0f, 1.0f)
			)));
		SMMATRIX mLightInv = SMMatrixInverse(NULL, mLight);

		vGridCenter = SMVector3Transform(vGridCenter, mLight);
		float fStep = (fGridRadius * 2.0f / (float)RSM_SUN_SIZE);
		vGridCenter.x -= fmodf(vGridCenter.x, fStep);
		vGridCenter.y -= fmodf(vGridCenter.y, fStep);

		vGridCenter = SMVector3Transform(vGridCenter, mLightInv);

		float fMaxDistance = PSSM_LIGHT_FAR;

		m_mReflectiveProj = SMMatrixOrthographicLH(fGridRadius * 2.0f, fGridRadius * 2.0f, PSSM_LIGHT_NEAR, fMaxDistance);
		m_mReflectiveView = SMMatrixLookToLH(vGridCenter - vLightDir * (fMaxDistance /*- fRadius * 2*/ * 0.5f), vLightDir, vUpDir);

		m_pReflectiveFrustum->update(m_mReflectiveView, m_mReflectiveProj);
	}
}

void CXLightSun::setCamera(IXCamera *pCamera)
{
	m_pCamera = pCamera;
}

const SMMATRIX* CXLightSun::getPSSMVPs() const
{
	return(m_mVPs);
}

const CXLightSun::Split* CXLightSun::getPSSMsplits() const
{
	return(m_splits);
}

IXRenderableVisibility* CXLightSun::getReflectiveVisibility()
{
	return(m_pReflectiveVisibility);
}

void CXLightSun::getReflectiveVP(SMMATRIX *pView, SMMATRIX *pProj) const
{
	*pView = m_mReflectiveView;
	*pProj = m_mReflectiveProj;
}

//##########################################################################

CXLightSpot::CXLightSpot(CLightSystem *pLightSystem):CXLight(pLightSystem)
{
	m_type = LIGHT_TYPE_SPOT;
	m_pShape = pLightSystem->getShapeCone();
}

void XMETHODCALLTYPE CXLightSpot::FinalRelease()
{
	m_pLightSystem->destroySpot(this);
}

float CXLightSpot::getInnerAngle()
{
	return(m_fInnerAngle);
}
void CXLightSpot::setInnerAngle(float fAngle)
{
	if(m_fInnerAngle == fAngle)
	{
		return;
	}
	m_fInnerAngle = fAngle;
	m_isPSDataDirty = true;
}
float CXLightSpot::getOuterAngle()
{
	return(m_fOuterAngle);
}
void CXLightSpot::setOuterAngle(float fAngle)
{
	if(m_fOuterAngle == fAngle)
	{
		return;
	}
	m_fOuterAngle = fAngle;
	m_isVSDataDirty = true;
	m_isPSDataDirty = true;
	m_dirty = LRT_ALL;
}
SMQuaternion CXLightSpot::getDirection()
{
	return(m_qDirection);
}
void CXLightSpot::setDirection(const SMQuaternion &qDirection)
{
	if(m_qDirection == qDirection)
	{
		return;
	}
	m_qDirection = qDirection;
	m_isVSDataDirty = true;
	m_isPSDataDirty = true;
	m_dirty = LRT_ALL;
}

void CXLightSpot::updatePSConstants(IGXDevice *pDevice)
{
	if(!m_pPSData)
	{
		m_pPSData = pDevice->createConstantBuffer(sizeof(m_dataPS));
	}
	m_dataPS.baseData.vLightColor = float4(m_vColor, getMaxDistance());
	m_dataPS.baseData.vLightPos = float4(m_vPosition, m_fShadowIntensity);
	m_dataPS.vInnerOuterAngle = float2(cosf(m_fInnerAngle * 0.5f), cosf(m_fOuterAngle * 0.5f));
	m_dataPS.vDir = m_qDirection * LIGHTS_DIR_BASE;
	m_pPSData->update(&m_dataPS);
}

SMMATRIX CXLightSpot::getWorldTM()
{
	float fAngleScale = tanf(m_fOuterAngle * 0.5f);

	return(SMMatrixScaling(float3(fAngleScale, 1.0f, fAngleScale)) * SMMatrixScaling(float3(getMaxDistance())) * m_qDirection.GetMatrix() * SMMatrixTranslation(m_vPosition));
}

void CXLightSpot::updateFrustum()
{
	//! @todo optimize me!
	float3 vPos = getPosition();
	float3 vDir = getDirection() * LIGHTS_DIR_BASE;
	float3 vUp = getDirection() * float3(0.0f, 0.0f, 1.0f);

	SMMATRIX mView = SMMatrixLookAtLH(vPos, vPos + vDir, vUp);
	SMMATRIX mProj = SMMatrixPerspectiveFovLH(getOuterAngle(), 1.0f, 0.025f, getMaxDistance());

	m_pFrustum->update(mView, mProj);
}

void CXLightSpot::updateVisibility(IXCamera *pMainCamera, const float3 &vLPVmin, const float3 &vLPVmax, bool useLPV)
{
	if(!(m_renderType & LRT_LIGHT))
	{
		updateFrustum();
		float3 vOrigin = getPosition();
		IXFrustum *pFrustum = pMainCamera->getFrustum();
		if(pFrustum->frustumInFrustum(m_pFrustum))
		{
			m_renderType |= LRT_LIGHT;
		}
		mem_release(pFrustum);
	}

	CXLight::updateVisibility(pMainCamera, vLPVmin, vLPVmax, useLPV);
}
