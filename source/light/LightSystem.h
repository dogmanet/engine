#ifndef __LIGHTSYSTEM_H
#define __LIGHTSYSTEM_H

#include "IXLightSystem.h"
#include <common/memalloc.h>
#include "light.h"
#include "ShadowCache.h"
#include "GIGraphNodeData.h"
#include "TonemappingGraphNodeData.h"

#define LPV_CASCADES_COUNT 3
#define LPV_GRID_SIZE 32
#define LPV_STEP_COUNT 6
#define LUMINANCE_BUFFER_SIZE 1024

#define SKYLIGHT_AREA_COUNT (sizeof(uint16_t) << 3)

class CLevelLoadListener;
class CSkyboxChangedListener;
class CLightSystem final: public IXUnknownImplementation<IXLightSystem>
{
public:
	CLightSystem(IXCore *pCore);
	~CLightSystem();

	SX_ALIGNED_OP_MEM();

	IXLightSun* XMETHODCALLTYPE newSun() override;
	IXLightSun* XMETHODCALLTYPE getSun() override;
	void destroySun(IXLightSun *pLight);

	IXLightPoint* XMETHODCALLTYPE newPoint() override;
	void destroyPoint(IXLightPoint *pLight);
	IXLightSpot* XMETHODCALLTYPE newSpot() override;
	void destroySpot(IXLightSpot *pLight);

	UINT XMETHODCALLTYPE getCount() override;
	IXLight* XMETHODCALLTYPE getLight(ID id) override;

	IXMesh* getShapeSphere();
	IXMesh* getShapeCone();

	void updateVisibility(IXCamera *pCamera);

	void setFrameObserverCamera(IXCamera *pMainCamera);

	void setLevelSize(const float3 &vMin, const float3 &vMax);

	void renderGI(CGIGraphNodeData *pNodeData, IXRenderTarget *pFinalTarget);
	void renderToneMapping(IGXBaseTexture *pSourceLight, CTonemappingGraphNodeData *pNodeData, IXRenderTarget *pFinalTarget);
	void renderDebug();

	void setEnabled(bool set)
	{
		m_isEnabled = set;
	}

	void setSkybox(IXTexture *pTexture);

	IXRender* getRender()
	{
		return(m_pRender);
	}

	void sync();

private:
	void showGICubes();

	void buildSkyLightMasks();

	void updateSkylight();

	void updateSkylightGrid();

	void initShaders();
private:
	CXLightSun *m_pSun = NULL;
	MemAlloc<CXLightPoint, 256, 16, 16> m_poolPoint;
	MemAlloc<CXLightSpot, 256, 16, 16> m_poolSpot;

	IXCore *m_pCore;
	IXRender *m_pRender = NULL;
	IXMaterialSystem *m_pMaterialSystem = NULL;
	IGXDevice *m_pDevice = NULL;

	XRenderPassHandler *m_pRenderPassIllumination = NULL;

	CShadowCache *m_pShadowCache = NULL;

	float3_t m_vLevelMin, m_vLevelMax;
	float m_fLevelDimensions = 0.0f;

	IEventChannel<XEventLevel> *m_pLevelChannel = NULL;
	CLevelLoadListener *m_pLevelListener = NULL;

	IEventChannel<XEventSkyboxChanged> *m_pSkyboxChangedChannel = NULL;
	CSkyboxChangedListener *m_pSkyboxChangedListener = NULL;

	Array<CXLight*> m_aLights;

	void _deleteLight(CXLight *pLight);

	IXMesh *m_pShapeSphere = NULL;
	IXMesh *m_pShapeCone = NULL;


	IGXRenderBuffer *m_pGICubesRB = NULL;
	UINT m_uGICubesCount = 0;
	ID m_idGICubesShader = -1;

	//###################################

	IGXDepthStencilState *m_pDepthStencilStateDefault = NULL;
	IGXSamplerState *m_pSamplerPointClamp = NULL;
	IGXSamplerState *m_pSamplerLinearWrap = NULL;
	IGXSamplerState *m_pSamplerLinearClamp = NULL;
	IGXSamplerState *m_pSamplerLinearBorder = NULL;

	IGXRasterizerState *m_pRasterizerCullNone = NULL;
	IGXRasterizerState *m_pRasterizerConservative = NULL;

	IGXBlendState *m_pBlendRed = NULL;
	IGXBlendState *m_pBlendAlphaOneOne = NULL;

	IGXDepthStencilState *m_pDepthStencilStateNoZ = NULL;
	IGXDepthStencilState *m_pDepthStencilStateLightBound = NULL;
	IGXDepthStencilState *m_pDepthStencilStateLightClear = NULL;
	IGXDepthStencilState *m_pDepthStencilStateLightShadowNonGlobal = NULL;
	IGXDepthStencilState *m_pDepthStencilStateLightShadowGlobal = NULL;

	//###################################

	ID m_idBlendAmbientSpecDiffColor[2][2];
	ID m_idComLightingShadow = -1;
	ID m_idComLightingSpotShadow = -1;
	ID m_idComLightingPSSMShadow = -1;
	ID m_idComLightingGI[LPV_CASCADES_COUNT][2];

	ID m_idHDRinitLuminance = -1;
	ID m_idHDRAdaptLuminance = -1;
	ID m_idHDRToneMapping = -1;

	//###################################

	struct LPVcascade
	{
		IGXTexture3D *pGIAccumRed = NULL;
		IGXTexture3D *pGIAccumGreen = NULL;
		IGXTexture3D *pGIAccumBlue = NULL;

		IGXTexture3D *pGIAccumRed2 = NULL;
		IGXTexture3D *pGIAccumGreen2 = NULL;
		IGXTexture3D *pGIAccumBlue2 = NULL;
	} m_aLPVs[3];

	ID m_idLightBoundShader = -1;
	ID m_idLPVPropagateShader = -1;
	ID m_idLuminanceReductionShader = -1;
	ID m_idLuminanceReductionMaskedShader = -1;
	ID m_idBloomShader = -1;
	ID m_idGaussShader[2];

	struct
	{
		struct
		{
			float4 vCenterSize[LPV_CASCADES_COUNT]; // xyz: center / size; w: world size of cell in meters
		} vs;
		//float4 vNearFarLayers;
	} m_lpvCentersShaderData;
	IGXConstantBuffer *m_pLPVcentersShaderData = NULL;
	IGXConstantBuffer *m_pLPVcurrentCascadeShaderData = NULL;

	struct
	{
		float fAdaptationSpeed;
		float fBaseValue;
		float _padding[2];
	} m_toneMappingShaderData;
	IGXConstantBuffer *m_pToneMappingShaderData = NULL;

	struct
	{
		struct
		{
			SMMATRIX mVP; // dummy
			SMMATRIX mViewInv;
			float2 vNearFar;
			float3 vParamProj;
		} vs;
	} m_shadowShaderData;
	IGXConstantBuffer *m_pShadowShaderDataVS = NULL;

	//###################################

	//! Буфер яркости
	IGXTexture2D *m_pLightLuminance = NULL;
	IGXTexture2D *m_pLightLuminance32 = NULL;
	IGXTexture2D *m_pLightLuminance1 = NULL;
	
	bool m_isEnabled = true;

	//###################################

	IXTexture *m_pSkyboxTexture = NULL;
	bool m_isSkyboxDirty = true;
	IGXBaseTexture *m_apSkyboxMask[SKYLIGHT_AREA_COUNT];

	ID m_idSkylightGenSHShader = -1;
	ID m_idSkylightGenGridShader = -1;

	IGXTexture2D *m_pSkyLightBaseR = NULL;
	IGXTexture2D *m_pSkyLightBaseG = NULL;
	IGXTexture2D *m_pSkyLightBaseB = NULL;

	IGXTexture3D *m_pSkyLightR = NULL;
	IGXTexture3D *m_pSkyLightG = NULL;
	IGXTexture3D *m_pSkyLightB = NULL;

	struct SkyLightGenSHParam
	{
		uint32_t uSkyboxSize;
		float fWeight;
		float fTexOffset;
		float _3;
	};

	IGXConstantBuffer *m_pSkyLightGenCB = NULL;
	IGXConstantBuffer *m_pSkyLightOcclusionCB = NULL;

	//###################################

	ID m_idSSAOShader[3];
	ID m_idSSAOBlendShader = -1;

	IGXTexture2D *m_pRndTexture = NULL;
	IGXConstantBuffer *m_pSSAOrndCB = NULL;
};

#endif
