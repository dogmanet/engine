
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#ifndef __SHADOW_H
#define __SHADOW_H

#include <gdefines.h>

#include <common/Math.h>
#include <common/Array.h>
#include <xcommon/IXCore.h>
#include <xcommon/render/IXRender.h>
#include <light/IXLight.h>
#include "light.h"

#define RSM_POINT_SIZE 32
#define RSM_SPOT_SIZE 32
#define RSM_SUN_SIZE 128
#define RSM_SUN_POINTS 128

class IXTexture;
class IBaseShadowMap
{
public:
	virtual void genShadow(IGXBaseTexture *pGBufferDepth, IGXBaseTexture *pGBufferNormals) = 0;
};

//##########################################################################

class IBaseReflectiveShadowMap
{
public:
	virtual void genLPV(bool isDebug = false) = 0;
};

//##########################################################################

class CShadowMap: public IBaseShadowMap
{
public:
	CShadowMap();
	~CShadowMap();

	SX_ALIGNED_OP_MEM();

	static UINT GetMapMemory(UINT uSize);

	void init(IXRender *pRender, UINT uSize);

	void setLight(CXLight *pLight);
	void process();
	void genShadow(IGXBaseTexture *pGBufferDepth, IGXBaseTexture *pGBufferNormals) override;

private:
	IXRender *m_pRender = NULL;
	IGXDevice *m_pDevice = NULL;

	static IGXDepthStencilSurface *ms_pDepthStencilSurface;
	static UINT ms_uDepthStencilSurfaceRefCount;

	static IGXSamplerState *ms_pSamplerPointWrap;
	static IGXSamplerState *ms_pSamplerPointClamp;
	static IGXSamplerState *ms_pSamplerLinearClamp;
	static IGXSamplerState *ms_pSamplerComparisonLinearClamp;

	static IGXTexture2D *ms_pRandomTexture;

	static void InitDepthStencilSurface(IXRender *pRender, UINT uSize);
	static void ReleaseDepthStencilSurface();

	IGXTexture2D *m_pDepthMap = NULL;

	float4x4 m_mScaleBiasMat;
	float m_fBias = 0.0001f;
	float m_fBlurPixel = 0.5f;

	float m_fSize;

	CXLight *m_pLight = NULL;

	float4x4 m_mView;
	float4x4 m_mProj;

	struct
	{
		struct
		{
			SMMATRIX mMatrixTexture;
			float3 vPixelMapSizeBias;
		} ps;
	} m_shaderData;
	IGXConstantBuffer *m_pShaderDataPS = NULL;
	IGXConstantBuffer *m_pShaderDataInjectVS = NULL;

	struct
	{
		struct
		{
			//SMMATRIX mV;
			SMMATRIX mVP;
			float3 vPosCam;
		} vs;
		//float4 vNearFarLayers;
	} m_cameraShaderData;
	IGXConstantBuffer *m_pCameraShaderDataVS = NULL;
};

//##########################################################################

class CReflectiveShadowMap: public IBaseReflectiveShadowMap
{
public:
	CReflectiveShadowMap();
	~CReflectiveShadowMap();

	SX_ALIGNED_OP_MEM();

	static UINT GetMapMemory(UINT uSize);

	void init(IXRender *pRender, UINT uSize);

	void setLight(CXLight *pLight);
	void process();
	void genLPV(bool isDebug = false) override;

private:
	IXRender *m_pRender = NULL;
	IGXDevice *m_pDevice = NULL;

	static IGXDepthStencilSurface *ms_pDepthStencilSurface32;
	static UINT ms_uDepthStencilSurfaceRefCount;

	static IGXSamplerState *ms_pSamplerPointWrap;
	static IGXSamplerState *ms_pSamplerPointClamp;
	static IGXSamplerState *ms_pSamplerLinearClamp;

	static ID ms_idInjectShader32;
	static ID ms_idInjectDebugShader32;

	static void InitDepthStencilSurface(IXRender *pRender, UINT uSize);
	static void ReleaseDepthStencilSurface();

	IGXTexture2D *m_pDepthMap32 = NULL;
	IGXTexture2D *m_pNormalMap32 = NULL;
	IGXTexture2D *m_pFluxMap32 = NULL;

	float4x4 m_mScaleBiasMat;
	float m_fBias = 0.0001f;
	float m_fBlurPixel = 0.5f;

	float m_fSize;

	CXLight *m_pLight = NULL;

	float4x4 m_mView;
	float4x4 m_mProj;

	struct
	{
		struct
		{
			SMMATRIX mMatrixTexture;
			float3 vPixelMapSizeBias;
		} ps;
	} m_shaderData;
	IGXConstantBuffer *m_pShaderDataPS = NULL;
	IGXConstantBuffer *m_pShaderDataInjectVS = NULL;

	struct
	{
		struct
		{
			//SMMATRIX mV;
			SMMATRIX mVP;
			float3 vPosCam;
		} vs;
		//float4 vNearFarLayers;
	} m_cameraShaderData;
	IGXConstantBuffer *m_pCameraShaderDataVS = NULL;
};

//##########################################################################

class CShadowPSSM: public IBaseShadowMap
{
public:
	CShadowPSSM();
	~CShadowPSSM();

	SX_ALIGNED_OP_MEM();

	static UINT GetMapMemory(UINT uSize);

	void init(IXRender *pRender, UINT uSize);

	void setObserverCamera(IXCamera *pCamera);
	void setLight(CXLight *pLight);
	void process();
	void genShadow(IGXBaseTexture *pGBufferDepth, IGXBaseTexture *pGBufferNormals) override;

protected:
	void updateFrustums();
private:
	IXRender *m_pRender = NULL;
	IGXDevice *m_pDevice = NULL;
	IXCamera *m_pCamera = NULL;

	IGXDepthStencilSurface *m_pDepthStencilSurface = NULL;

	IGXSamplerState *m_pSamplerPointWrap = NULL;
	IGXSamplerState *m_pSamplerPointClamp = NULL;
	IGXSamplerState *m_pSamplerLinearClamp = NULL;
	IGXSamplerState *m_pSamplerComparisonLinearClamp = NULL;

	IGXTextureCube *m_pDepthMap = NULL;

#if 0
	struct Split
	{
		SX_ALIGNED_OP_MEM2();

		float2 vNearFar;
		float4x4 mView;
		float4x4 mProj;
	};
	Split m_splits[PSSM_MAX_SPLITS];
#endif

	float4x4 m_mScaleBiasMat;
	float m_fBias = 0.0001f;
	float m_fBlurPixel = 0.5f;

	float m_fSize;

	CXLight *m_pLight = NULL;

	struct
	{
		struct
		{
			SMMATRIX mMatrixTextureV[PSSM_MAX_SPLITS];
			SMMATRIX mMatrixTextureP[PSSM_MAX_SPLITS];
			float4 vSizeBoundNearFar;
		} ps;
	} m_shaderData;
	IGXConstantBuffer *m_pShaderDataPS = NULL;
	IGXConstantBuffer *m_pShaderDataGS = NULL;

	struct
	{
		struct
		{
			//SMMATRIX mV;
			SMMATRIX mVP;
			float3 vPosCam;
		} vs;
		//float4 vNearFarLayers;
	} m_cameraShaderData;
	IGXConstantBuffer *m_pCameraShaderDataVS = NULL;
};

//##########################################################################

class CReflectiveShadowSun: public IBaseReflectiveShadowMap
{
public:
	CReflectiveShadowSun();
	~CReflectiveShadowSun();

	SX_ALIGNED_OP_MEM();

	static UINT GetMapMemory(UINT uSize);

	void init(IXRender *pRender, UINT uSize);

	void setObserverCamera(IXCamera *pCamera);
	void setLight(CXLight *pLight);
	void process();
	void genLPV(bool isDebug = false) override;
protected:
	void updateFrustum();
private:
	IXRender *m_pRender = NULL;
	IGXDevice *m_pDevice = NULL;
	IXCamera *m_pCamera = NULL;

	IGXDepthStencilSurface *m_pDepthStencilSurface = NULL;

	IGXTexture2D *m_pDepthMap = NULL;
	IGXTexture2D *m_pNormalMap = NULL;
	IGXTexture2D *m_pFluxMap = NULL;

	float4x4 m_mView;
	float4x4 m_mProj;

	IGXSamplerState *m_pSamplerPointWrap = NULL;
	IGXSamplerState *m_pSamplerPointClamp = NULL;
	IGXSamplerState *m_pSamplerLinearClamp = NULL;
	IGXSamplerState *m_pSamplerComparisonLinearClamp = NULL;

	ID m_idInjectShader = -1;
	ID m_idInjectDebugShader = -1;

	float4x4 m_mScaleBiasMat;
	float m_fBias = 0.0001f;
	float m_fBlurPixel = 0.5f;

	float m_fSize;

	CXLight *m_pLight = NULL;

	struct
	{
		struct
		{
			SMMATRIX mMatrixTexture;
			float3 vPixelMapSizeBias;
		} ps;
	} m_shaderData;
	IGXConstantBuffer *m_pShaderDataPS = NULL;
	IGXConstantBuffer *m_pShaderDataInjectVS = NULL;

	struct
	{
		struct
		{
			//SMMATRIX mV;
			SMMATRIX mVP;
			float3 vPosCam;
		} vs;
		//float4 vNearFarLayers;
	} m_cameraShaderData;
	IGXConstantBuffer *m_pCameraShaderDataVS = NULL;
};

//##########################################################################

class CShadowCubeMap: public IBaseShadowMap
{
public:
	CShadowCubeMap();
	~CShadowCubeMap();

	SX_ALIGNED_OP_MEM();

	static UINT GetMapMemory(UINT uSize);

	void init(IXRender *pRender, UINT uSize);

	void setLight(CXLight *pLight);
	void process();
	void genShadow(IGXBaseTexture *pGBufferDepth, IGXBaseTexture *pGBufferNormals) override;

private:
	IXRender *m_pRender = NULL;
	IGXDevice *m_pDevice = NULL;

	static IGXDepthStencilSurface *ms_pDepthStencilSurface;
	static UINT ms_uDepthStencilSurfaceRefCount;

	static IGXSamplerState *ms_pSamplerPointWrap;
	static IGXSamplerState *ms_pSamplerPointClamp;
	static IGXSamplerState *ms_pSamplerLinearClamp;
	static IGXSamplerState *ms_pSamplerComparisonLinearClamp;

	static void InitDepthStencilSurface(IXRender *pRender, UINT uSize);
	static void ReleaseDepthStencilSurface();

	IGXTextureCube *m_pDepthMap = NULL;

	float4x4 m_mScaleBiasMat;
	float m_fBias = 0.005f;
	float m_fBlurPixel = 0.5f;

	float m_fSize;

	CXLight *m_pLight = NULL;

	float4x4 m_mView[6];
	float4x4 m_mProj;

	struct
	{
		struct
		{
			SMMATRIX mVP[6];
		} gs;
		struct
		{
			float2 vPixelSizeBias;
		} ps;
	} m_shaderData;
	IGXConstantBuffer *m_pShaderDataPS = NULL;
	IGXConstantBuffer *m_pShaderDataGS = NULL;



	struct
	{
		struct
		{
			//SMMATRIX mV;
			SMMATRIX mVP;
			float3 vPosCam;
		} vs;
		//float4 vNearFarLayers;
	} m_cameraShaderData;
	IGXConstantBuffer *m_pCameraShaderDataVS = NULL;


	struct
	{
		struct
		{
			SMMATRIX mVP;
			SMMATRIX mViewInv;
			float2 vNearFar;
			float3 vParamProj;
		} vs;
	} m_lightingShaderData;
	IGXConstantBuffer *m_pLightingShaderDataVS = NULL;
};

//##########################################################################

class CReflectiveShadowCubeMap: public IBaseReflectiveShadowMap
{
public:
	CReflectiveShadowCubeMap();
	~CReflectiveShadowCubeMap();

	SX_ALIGNED_OP_MEM();

	static UINT GetMapMemory(UINT uSize);

	void init(IXRender *pRender, UINT uSize);

	void setLight(CXLight *pLight);
	void process();
	void genLPV(bool isDebug = false) override;

private:
	IXRender *m_pRender = NULL;
	IGXDevice *m_pDevice = NULL;

	static IGXDepthStencilSurface *ms_pDepthStencilSurface32;
	static UINT ms_uDepthStencilSurfaceRefCount;

	static IGXSamplerState *ms_pSamplerPointWrap;
	static IGXSamplerState *ms_pSamplerPointClamp;
	static IGXSamplerState *ms_pSamplerLinearClamp;

	static ID ms_idInjectShader32;
	static ID ms_idInjectDebugShader32;

	static void InitDepthStencilSurface(IXRender *pRender, UINT uSize);
	static void ReleaseDepthStencilSurface();

	IGXTextureCube *m_pDepthMap32 = NULL;
	IGXTextureCube *m_pNormalMap32 = NULL;
	IGXTextureCube *m_pFluxMap32 = NULL;

	float4x4 m_mScaleBiasMat;
	float m_fBias = 0.005f;
	float m_fBlurPixel = 0.5f;

	float m_fSize;

	CXLight *m_pLight = NULL;

	float4x4 m_mView[6];
	float4x4 m_mProj;

	struct
	{
		struct
		{
			SMMATRIX mVP[6];
		} gs;
		struct
		{
			float2 vPixelSizeBias;
		} ps;
	} m_shaderData;
	IGXConstantBuffer *m_pShaderDataPS = NULL;
	IGXConstantBuffer *m_pShaderDataGS = NULL;



	struct
	{
		struct
		{
			//SMMATRIX mV;
			SMMATRIX mVP;
			float3 vPosCam;
		} vs;
		//float4 vNearFarLayers;
	} m_cameraShaderData;
	IGXConstantBuffer *m_pCameraShaderDataVS = NULL;


	struct
	{
		struct
		{
			SMMATRIX mVP;
			SMMATRIX mViewInv;
			float2 vNearFar;
			float3 vParamProj;
		} vs;
	} m_lightingShaderData;
	IGXConstantBuffer *m_pLightingShaderDataVS = NULL;
};

#endif
