#include "LightSystem.h"

#include <xcommon/resource/IXResourceManager.h>
#include <xcommon/IXAmbient.h>
#include <xcommon/IXRenderable.h>
#include <core/sxcore.h>

class CLevelLoadListener: public IEventListener<XEventLevel>
{
public:
	CLevelLoadListener(CLightSystem *pLightSystem, IXCore *pCore):
		m_pLightSystem(pLightSystem)
	{
		m_pLevelSizeChannel = pCore->getEventChannel<XEventLevelSize>(EVENT_LEVEL_GET_SIZE_GUID);
	}

	void onEvent(const XEventLevel *pEvent) override
	{
		switch(pEvent->type)
		{
		case XEventLevel::TYPE_LOAD_BEGIN:
			m_pLightSystem->setEnabled(false);
			break;

		case XEventLevel::TYPE_LOAD_END:
			XEventLevelSize levelSize;
			m_pLevelSizeChannel->broadcastEvent(&levelSize);

			m_pLightSystem->setLevelSize(levelSize.vMin, levelSize.vMax);

			m_pLightSystem->setEnabled(true);
			break;
		}
	}

private:
	CLightSystem *m_pLightSystem;
	IEventChannel<XEventLevelSize> *m_pLevelSizeChannel = NULL;
};

//##########################################################################

class CSkyboxChangedListener: public IEventListener<XEventSkyboxChanged>
{
public:
	CSkyboxChangedListener(CLightSystem *pLightSystem, IXCore *pCore):
		m_pLightSystem(pLightSystem)
	{

	}

	void onEvent(const XEventSkyboxChanged *pEvent) override
	{
		m_pLightSystem->setSkybox(pEvent->pTexture);
	}

private:
	CLightSystem *m_pLightSystem;
};

//##########################################################################

CLightSystem::CLightSystem(IXCore *pCore):
	m_pCore(pCore)
{
	IXConsole *pConsole = pCore->getConsole();
	pConsole->registerCVar("lpv_size_0", 1.0f, "Коэфициент размера первого каскада LPV");
	pConsole->registerCVar("lpv_size_1", 2.0f, "Коэфициент размера второго каскада LPV");
	pConsole->registerCVar("lpv_size_2", 4.0f, "Коэфициент размера третьего каскада LPV");
	pConsole->registerCVar("lpv_cascades_count", 3, "Количество активных каскадов LPV [0;3]");
	pConsole->registerCVar("pp_ssao", 3, "Рисовать ли эффект ssao? 0 - нет, 1 - на низком качестве, 2 - на среднем, 3 - на высоком");
	pConsole->registerCVar("pp_sslr", true, "Рисовать ли эффект sslr?");

	m_pLevelListener = new CLevelLoadListener(this, pCore);
	m_pLevelChannel = pCore->getEventChannel<XEventLevel>(EVENT_LEVEL_GUID);
	m_pLevelChannel->addListener(m_pLevelListener);

	m_pSkyboxChangedListener = new CSkyboxChangedListener(this, pCore);
	m_pSkyboxChangedChannel = pCore->getEventChannel<XEventSkyboxChanged>(EVENT_SKYBOX_CHANGED_GUID);
	m_pSkyboxChangedChannel->addListener(m_pSkyboxChangedListener);



	m_pRender = (IXRender*)pCore->getPluginManager()->getInterface(IXRENDER_GUID);
	m_pDevice = m_pRender->getDevice();

	m_pMaterialSystem = (IXMaterialSystem*)pCore->getPluginManager()->getInterface(IXMATERIALSYSTEM_GUID);
	m_pRenderPassIllumination = m_pMaterialSystem->getRenderPass("xIllumination");

	m_pShadowCache = new CShadowCache(m_pRender, m_pMaterialSystem);
	
	initShaders();



	if(m_pDevice)
	{
		{
			const UINT uSize = 32;
			m_uGICubesCount = uSize * uSize * uSize;
			float3_t *pData = new float3_t[m_uGICubesCount];
			float fHalf = (float)uSize / 2.0f;
			for(UINT x = 0; x < uSize; ++x)
			{
				for(UINT y = 0; y < uSize; ++y)
				{
					for(UINT z = 0; z < uSize; ++z)
					{
						pData[x + y * uSize + z * uSize * uSize] = (float3)(float3((float)x, (float)y, (float)z) - float3(fHalf));
					}
				}
			}
			IGXVertexBuffer *pVB = m_pDevice->createVertexBuffer(sizeof(float3_t) * m_uGICubesCount, GXBUFFER_USAGE_STATIC, pData);
			mem_delete_a(pData);

			GXVertexElement oLayout[] =
			{
				{0, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_POSITION},
				GX_DECL_END()
			};

			IGXVertexDeclaration *pVD = m_pDevice->createVertexDeclaration(oLayout);
			m_pGICubesRB = m_pDevice->createRenderBuffer(1, &pVB, pVD);
			mem_release(pVD);
			mem_release(pVB);
		}

		//static const int *r_win_width = GET_PCVAR_INT("r_win_width");
		//static const int *r_win_height = GET_PCVAR_INT("r_win_height");

		m_pLightLuminance = m_pDevice->createTexture2D(LUMINANCE_BUFFER_SIZE, LUMINANCE_BUFFER_SIZE, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_R16F);

		m_pLightLuminance32 = m_pDevice->createTexture2D(32, 32, 1, GX_TEXFLAG_RENDERTARGET | GX_TEXFLAG_UNORDERED_ACCESS, GXFMT_R16F);
		m_pLightLuminance1 = m_pDevice->createTexture2D(1, 1, 1, GX_TEXFLAG_RENDERTARGET | GX_TEXFLAG_UNORDERED_ACCESS, GXFMT_R16F);
		
		
		m_pLPVcentersShaderData = m_pDevice->createConstantBuffer(sizeof(m_lpvCentersShaderData.vs));
		m_pLPVcurrentCascadeShaderData = m_pDevice->createConstantBuffer(sizeof(float4_t));
		
		m_pToneMappingShaderData = m_pDevice->createConstantBuffer(sizeof(m_toneMappingShaderData));

		for(UINT i = 0; i < 3; ++i)
		{
			m_aLPVs[i].pGIAccumRed = m_pDevice->createTexture3D(32, 32, 32, 1, GX_TEXFLAG_RENDERTARGET | GX_TEXFLAG_UNORDERED_ACCESS, GXFMT_A32B32G32R32F);
			m_aLPVs[i].pGIAccumGreen = m_pDevice->createTexture3D(32, 32, 32, 1, GX_TEXFLAG_RENDERTARGET | GX_TEXFLAG_UNORDERED_ACCESS, GXFMT_A32B32G32R32F);
			m_aLPVs[i].pGIAccumBlue = m_pDevice->createTexture3D(32, 32, 32, 1, GX_TEXFLAG_RENDERTARGET | GX_TEXFLAG_UNORDERED_ACCESS, GXFMT_A32B32G32R32F);

			m_aLPVs[i].pGIAccumRed2 = m_pDevice->createTexture3D(32, 32, 32, 1, GX_TEXFLAG_RENDERTARGET | GX_TEXFLAG_UNORDERED_ACCESS, GXFMT_A32B32G32R32F);
			m_aLPVs[i].pGIAccumGreen2 = m_pDevice->createTexture3D(32, 32, 32, 1, GX_TEXFLAG_RENDERTARGET | GX_TEXFLAG_UNORDERED_ACCESS, GXFMT_A32B32G32R32F);
			m_aLPVs[i].pGIAccumBlue2 = m_pDevice->createTexture3D(32, 32, 32, 1, GX_TEXFLAG_RENDERTARGET | GX_TEXFLAG_UNORDERED_ACCESS, GXFMT_A32B32G32R32F);
		}

#if 0
		m_pSkyLightR = m_pDevice->createTexture3D(16, 16, 4, 1, GX_TEXFLAG_RENDERTARGET | GX_TEXFLAG_UNORDERED_ACCESS, GXFMT_A32B32G32R32F);
		m_pSkyLightG = m_pDevice->createTexture3D(16, 16, 4, 1, GX_TEXFLAG_RENDERTARGET | GX_TEXFLAG_UNORDERED_ACCESS, GXFMT_A32B32G32R32F);
		m_pSkyLightB = m_pDevice->createTexture3D(16, 16, 4, 1, GX_TEXFLAG_RENDERTARGET | GX_TEXFLAG_UNORDERED_ACCESS, GXFMT_A32B32G32R32F);
#else
		m_pSkyLightR = m_pDevice->createTexture3D(1, 1, 1, 1, GX_TEXFLAG_RENDERTARGET | GX_TEXFLAG_UNORDERED_ACCESS, GXFMT_A32B32G32R32F);
		m_pSkyLightG = m_pDevice->createTexture3D(1, 1, 1, 1, GX_TEXFLAG_RENDERTARGET | GX_TEXFLAG_UNORDERED_ACCESS, GXFMT_A32B32G32R32F);
		m_pSkyLightB = m_pDevice->createTexture3D(1, 1, 1, 1, GX_TEXFLAG_RENDERTARGET | GX_TEXFLAG_UNORDERED_ACCESS, GXFMT_A32B32G32R32F);
#endif

		m_pSkyLightBaseR = m_pDevice->createTexture2D(SKYLIGHT_AREA_COUNT, 1, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A32B32G32R32F);
		m_pSkyLightBaseG = m_pDevice->createTexture2D(SKYLIGHT_AREA_COUNT, 1, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A32B32G32R32F);
		m_pSkyLightBaseB = m_pDevice->createTexture2D(SKYLIGHT_AREA_COUNT, 1, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A32B32G32R32F);
		
		GXSamplerDesc samplerDesc;

		samplerDesc.addressU = samplerDesc.addressV = samplerDesc.addressW = GXTEXTURE_ADDRESS_CLAMP;
		m_pSamplerPointClamp = m_pDevice->createSamplerState(&samplerDesc);

		samplerDesc.addressU = samplerDesc.addressV = samplerDesc.addressW = GXTEXTURE_ADDRESS_WRAP;
		samplerDesc.filter = GXFILTER_MIN_MAG_MIP_LINEAR;
		m_pSamplerLinearWrap = m_pDevice->createSamplerState(&samplerDesc);

		samplerDesc.addressU = samplerDesc.addressV = samplerDesc.addressW = GXTEXTURE_ADDRESS_CLAMP;
		m_pSamplerLinearClamp = m_pDevice->createSamplerState(&samplerDesc);

		samplerDesc.addressU = samplerDesc.addressV = samplerDesc.addressW = GXTEXTURE_ADDRESS_BORDER;
		samplerDesc.filter = GXFILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.f4BorderColor = float4_t(0.0f, 0.0f, 0.0f, 0.0f);
		m_pSamplerLinearBorder = m_pDevice->createSamplerState(&samplerDesc);

		m_pShadowShaderDataVS = m_pDevice->createConstantBuffer(sizeof(m_shadowShaderData.vs));


		GXRasterizerDesc rasterizerDesc;

		//rasterizerDesc.useConservativeRasterization = true;
		m_pRasterizerConservative = m_pDevice->createRasterizerState(&rasterizerDesc);

		//rasterizerDesc.useConservativeRasterization = false;
		rasterizerDesc.cullMode = GXCULL_NONE;
		m_pRasterizerCullNone = m_pDevice->createRasterizerState(&rasterizerDesc);



		GXBlendDesc blendDesc;

		blendDesc.renderTarget[0].u8RenderTargetWriteMask = GXCOLOR_WRITE_ENABLE_RED;
		m_pBlendRed = m_pDevice->createBlendState(&blendDesc);

		blendDesc.renderTarget[0].u8RenderTargetWriteMask = GXCOLOR_WRITE_ENABLE_ALL;
		blendDesc.renderTarget[0].useBlend = TRUE;
		blendDesc.renderTarget[0].blendSrcColor = blendDesc.renderTarget[0].blendSrcAlpha = GXBLEND_ONE;
		blendDesc.renderTarget[0].blendDestColor = blendDesc.renderTarget[0].blendDestAlpha = GXBLEND_ONE;
		m_pBlendAlphaOneOne = m_pDevice->createBlendState(&blendDesc);




		GXDepthStencilDesc dsDesc;
		dsDesc.cmpFuncDepth = GXCMP_GREATER_EQUAL;
		m_pDepthStencilStateDefault = m_pDevice->createDepthStencilState(&dsDesc);

		dsDesc.useDepthTest = FALSE;
		dsDesc.useDepthWrite = FALSE;
		m_pDepthStencilStateNoZ = m_pDevice->createDepthStencilState(&dsDesc);

		dsDesc.useStencilTest = TRUE;
		dsDesc.useDepthTest = TRUE;
		dsDesc.stencilTestFront.cmpFunc = GXCMP_ALWAYS;
		dsDesc.stencilTestFront.opDepthFail = GXSTENCIL_OP_INCR;
		dsDesc.stencilTestFront.opPass = GXSTENCIL_OP_KEEP;
		dsDesc.stencilTestBack.opDepthFail = GXSTENCIL_OP_DECR;
		m_pDepthStencilStateLightBound = m_pDevice->createDepthStencilState(&dsDesc);

		dsDesc.useDepthTest = FALSE;
		dsDesc.stencilTestFront.cmpFunc = GXCMP_EQUAL;
		dsDesc.stencilTestFront.opDepthFail = GXSTENCIL_OP_ZERO;
		dsDesc.stencilTestFront.opFail = GXSTENCIL_OP_ZERO;
		dsDesc.stencilTestFront.opPass = GXSTENCIL_OP_KEEP;
		m_pDepthStencilStateLightShadowNonGlobal = m_pDevice->createDepthStencilState(&dsDesc);

		dsDesc.stencilTestFront.opPass = GXSTENCIL_OP_ZERO;
		m_pDepthStencilStateLightClear = m_pDevice->createDepthStencilState(&dsDesc);

		dsDesc.useStencilTest = FALSE;
		m_pDepthStencilStateLightShadowGlobal = m_pDevice->createDepthStencilState(&dsDesc);
		
		float3 rnd[24];
		for(int i = 0; i < 24; i++)
		{
			rnd[i] = SMVector3Normalize(float3(randf(-1.0f, 1.0f), randf(-1.0f, 1.0f), randf(0.0f, 1.0f))) * vlerp(0.1f, 1.0f, (float)i / 24.0f);
		}

		m_pSSAOrndCB = m_pDevice->createConstantBuffer(sizeof(rnd));
		m_pSSAOrndCB->update(&rnd);


		GXCOLOR aRandomTexture[4 * 4];
		float4 vRnd;
		for(UINT i = 0; i < 4 * 4; ++i)
		{
			vRnd = float4(SMVector3Normalize(float3(randf(0.0f, 1.0f), randf(0.0f, 1.0f), 0.0f)), 1.0f);

			aRandomTexture[i] = GX_COLOR_F4_TO_COLOR(vRnd);
		}

		//m_pRndTexture = m_pDevice->createTexture2D(32, 32, 1, 0, GXFMT_A8B8G8R8, aRandomTexture);
		m_pRndTexture = m_pDevice->createTexture2D(4, 4, 1, 0, GXFMT_A8B8G8R8, aRandomTexture);
		
		m_pSkyLightGenCB = m_pDevice->createConstantBuffer(sizeof(SkyLightGenSHParam));

		buildSkyLightMasks();

		IXAmbient *pAmbient = (IXAmbient*)m_pCore->getPluginManager()->getInterface(IXAMBIENT_GUID);
		if(pAmbient)
		{
			IXTexture *pTexture = NULL;
			pAmbient->getSkyboxTexture(&pTexture);
			setSkybox(pTexture);
			mem_release(pTexture);
		}
	}
	else
	{
		for(UINT i = 0; i < 3; ++i)
		{
			m_aLPVs[i].pGIAccumRed = NULL;
			m_aLPVs[i].pGIAccumGreen = NULL;
			m_aLPVs[i].pGIAccumBlue = NULL;

			m_aLPVs[i].pGIAccumRed2 = NULL;
			m_aLPVs[i].pGIAccumGreen2 = NULL;
			m_aLPVs[i].pGIAccumBlue2 = NULL;
		}
	}
}

CLightSystem::~CLightSystem()
{
	m_pLevelChannel->removeListener(m_pLevelListener);
	mem_delete(m_pLevelListener);
	m_pSkyboxChangedChannel->removeListener(m_pSkyboxChangedListener);
	mem_delete(m_pSkyboxChangedListener);

	mem_delete(m_pSun);
	mem_release(m_pShapeSphere);
	mem_release(m_pShapeCone);

	mem_release(m_pGICubesRB);

	mem_release(m_pLightLuminance);
	mem_release(m_pLightLuminance32);
	mem_release(m_pLightLuminance1);

	mem_release(m_pLPVcentersShaderData);
	mem_release(m_pLPVcurrentCascadeShaderData);
	mem_release(m_pToneMappingShaderData);


	for(UINT i = 0; i < 3; ++i)
	{
		mem_release(m_aLPVs[i].pGIAccumRed);
		mem_release(m_aLPVs[i].pGIAccumGreen);
		mem_release(m_aLPVs[i].pGIAccumBlue);

		mem_release(m_aLPVs[i].pGIAccumRed2);
		mem_release(m_aLPVs[i].pGIAccumGreen2);
		mem_release(m_aLPVs[i].pGIAccumBlue2);
	}

	mem_release(m_pDepthStencilStateDefault);

	mem_release(m_pSamplerPointClamp);
	mem_release(m_pSamplerLinearWrap);
	mem_release(m_pSamplerLinearClamp);
	mem_release(m_pSamplerLinearBorder);

	mem_release(m_pShadowShaderDataVS);

	mem_delete(m_pShadowCache);

	mem_release(m_pRasterizerConservative);
	mem_release(m_pRasterizerCullNone);

	mem_release(m_pBlendRed);
	mem_release(m_pBlendAlphaOneOne);

	mem_release(m_pDepthStencilStateNoZ);
	mem_release(m_pDepthStencilStateLightBound);
	mem_release(m_pDepthStencilStateLightClear);
	mem_release(m_pDepthStencilStateLightShadowNonGlobal);
	mem_release(m_pDepthStencilStateLightShadowGlobal);

	mem_release(m_pSkyboxTexture);

	for(UINT i = 0; i < SKYLIGHT_AREA_COUNT; ++i)
	{
		mem_release(m_apSkyboxMask[i]);
	}

	mem_release(m_pSkyLightR);
	mem_release(m_pSkyLightG);
	mem_release(m_pSkyLightB);

	mem_release(m_pSkyLightBaseR);
	mem_release(m_pSkyLightBaseG);
	mem_release(m_pSkyLightBaseB);

	mem_release(m_pSkyLightGenCB);

	mem_release(m_pSSAOrndCB);

	mem_release(m_pRndTexture);
}

void CLightSystem::setLevelSize(const float3 &vMin, const float3 &vMax)
{
	m_vLevelMin = vMin;
	m_vLevelMax = vMax;
	m_fLevelDimensions = SMVector3Length(vMin - vMax);
	if(m_fLevelDimensions < 100.0f)
	{
		m_fLevelDimensions = 100.0f;
	}
	if(m_pSun)
	{
		m_pSun->setMaxDistance(m_fLevelDimensions);
	}
}

IXLightSun* XMETHODCALLTYPE CLightSystem::newSun()
{
	if(m_pSun)
	{
		m_pSun->AddRef();
	}
	else
	{
		m_pSun = new CXLightSun(this);
		m_pSun->setMaxDistance(m_fLevelDimensions);
		m_aLights.push_back(m_pSun);
	}
	return(m_pSun);
}

IXLightSun* XMETHODCALLTYPE CLightSystem::getSun()
{
	if(m_pSun)
	{
		m_pSun->AddRef();
	}
	return(m_pSun);
}

IXLightPoint* XMETHODCALLTYPE CLightSystem::newPoint()
{
	CXLightPoint *pLight = m_poolPoint.Alloc(this);
	m_aLights.push_back(pLight);
	return(pLight);
}
IXLightSpot* XMETHODCALLTYPE CLightSystem::newSpot()
{
	CXLightSpot *pLight = m_poolSpot.Alloc(this);
	m_aLights.push_back(pLight);
	return(pLight);
}

UINT XMETHODCALLTYPE CLightSystem::getCount()
{
	return(m_aLights.size());
}
IXLight* XMETHODCALLTYPE CLightSystem::getLight(ID id)
{
	assert(ID_VALID(id) && id < (ID)m_aLights.size());

	return(m_aLights[id]);
}

void CLightSystem::destroySun(IXLightSun *pLight)
{
	assert(m_pSun == pLight);
	_deleteLight(m_pSun);
	m_pSun = NULL;
}
void CLightSystem::destroyPoint(IXLightPoint *_pLight)
{
	CXLightPoint *pLight = dynamic_cast<CXLightPoint*>(_pLight);
	_deleteLight(pLight);
	m_poolPoint.Delete(pLight);
}
void CLightSystem::destroySpot(IXLightSpot *_pLight)
{
	CXLightSpot *pLight = dynamic_cast<CXLightSpot*>(_pLight);
	_deleteLight(pLight);
	m_poolSpot.Delete(pLight);
}

void CLightSystem::_deleteLight(CXLight *pLight)
{
	for(UINT i = 0, l = m_aLights.size(); i < l; ++i)
	{
		if(m_aLights[i] == pLight)
		{
			m_aLights.erase(i);
			break;
		}
	}
}

IXMesh* CLightSystem::getShapeSphere()
{
	if(!m_pShapeSphere)
	{
		m_pRender->getUtils()->createSphereMesh(1.0f, 16, 16, &m_pShapeSphere);
	}
	add_ref(m_pShapeSphere);
	return(m_pShapeSphere);
}

IXMesh* CLightSystem::getShapeCone()
{
	if(!m_pShapeCone)
	{
		m_pRender->getUtils()->createConeMesh(0.0f, 1.0f / cosf(SM_PI / 16.0f), 1.0f, &m_pShapeCone, 16);
	}
	add_ref(m_pShapeCone);
	return(m_pShapeCone);
}

void CLightSystem::updateVisibility(IXCamera *pCamera)
{
	XPROFILE_FUNCTION();
	assert(pCamera);

	if(!m_isEnabled)
	{
		return;
	}

	float3 vCamPos = pCamera->getPosition();
	float3 vCamDir = pCamera->getLook();

	static const float *lpv_size_2 = m_pCore->getConsole()->getPCVarFloat("lpv_size_2");
	static const int *lpv_cascades_count = m_pCore->getConsole()->getPCVarInt("lpv_cascades_count");

	int iCascades = *lpv_cascades_count;
	if(iCascades < 0)
	{
		iCascades = 0;
	}
	if(iCascades > LPV_CASCADES_COUNT)
	{
		iCascades = LPV_CASCADES_COUNT;
	}
	
	float3 vLPVmin;
	float3 vLPVmax;
	if(iCascades)
	{
		vLPVmin = vLPVmax = vCamPos + vCamDir * (LPV_GRID_SIZE / 2 - LPV_STEP_COUNT) * *lpv_size_2;

		vLPVmin += float3(-16.0f, -16.0f, -16.0f) * *lpv_size_2;
		vLPVmax += float3(16.0f, 16.0f, 16.0f) * *lpv_size_2;
	}

	for(UINT i = 0, l = m_aLights.size(); i < l; ++i)
	{
		if(m_aLights[i]->isEnabled())
		{
			m_aLights[i]->testDirty();

			//if(m_aLights[i]->isDirty(LRT_ALL))
			{
				m_aLights[i]->updateVisibility(pCamera, vLPVmin, vLPVmax, iCascades > 0);
			}
		}
	}
}

void CLightSystem::setFrameObserverCamera(IXCamera *pMainCamera)
{
	SAFE_CALL(m_pSun, setCamera, pMainCamera);
}

void CLightSystem::renderGI(CGIGraphNodeData *pNodeData, IXRenderTarget *pFinalTarget)
{
	if(!m_isEnabled)
	{
		return;
	}
	//IGXTexture2D *pLightTotal = pNodeData->m_pLightTotal;
	IGXTexture2D *pTempBuffer = pNodeData->m_pLightAmbientDiffuse;

	IXCamera *pCamera;
	pFinalTarget->getCamera(&pCamera);

	updateSkylight();

	IGXContext *pCtx = m_pDevice->getThreadContext();

	IGXDepthStencilSurface *pOldDSSurface = pCtx->getDepthStencilSurface();

	UINT uCounts[LIGHT_TYPE__COUNT] = {0};
	for(int i = 0, l = m_aLights.size(); i < l; ++i)
	{
		++uCounts[m_aLights[i]->getType()];
	}
	m_pShadowCache->setLightsCount(uCounts[LIGHT_TYPE_POINT], uCounts[LIGHT_TYPE_SPOT], uCounts[LIGHT_TYPE_SUN] != 0);
	m_pShadowCache->setRSMLightsCount(uCounts[LIGHT_TYPE_POINT], uCounts[LIGHT_TYPE_SPOT], uCounts[LIGHT_TYPE_SUN] != 0);

	m_pShadowCache->setObserverCamera(pCamera);

	m_pShadowCache->nextFrame();

	IGXSurface *pAmbientSurf, *pBackBuf;
	pAmbientSurf = pTempBuffer->getMipmap();

	pBackBuf = pCtx->getColorTarget();

	pCtx->setColorTarget(pAmbientSurf);

	//очищаем рт и стенсил
	pCtx->clear(GX_CLEAR_COLOR);
	//	pCtx->clear(GX_CLEAR_COLOR | GX_CLEAR_STENCIL);

	IXRenderableVisibility *pVis;
	pCamera->getVisibility(&pVis);

	m_pRender->setSceneFilter(pCtx);
	m_pMaterialSystem->bindRenderPass(m_pRenderPassIllumination);
	m_pRender->renderStage(XRS_GI, pVis);
	mem_release(pVis);
	pCtx->addTimestamp("selfillum -");

	float3 vCamDir = pCamera->getLook();
	float3 vCamPos = pCamera->getPosition();

	static const float *lpv_size_0 = m_pCore->getConsole()->getPCVarFloat("lpv_size_0");
	static const float *lpv_size_1 = m_pCore->getConsole()->getPCVarFloat("lpv_size_1");
	static const float *lpv_size_2 = m_pCore->getConsole()->getPCVarFloat("lpv_size_2");
	static const int *lpv_cascades_count = m_pCore->getConsole()->getPCVarInt("lpv_cascades_count");

	int iCascades = *lpv_cascades_count;
	if(iCascades < 0)
	{
		iCascades = 0;
	}
	if(iCascades > LPV_CASCADES_COUNT)
	{
		iCascades = LPV_CASCADES_COUNT;
	}

	const float c_aLPVsizes[] = {
		*lpv_size_0,
		*lpv_size_1,
		*lpv_size_2
	};

	if(iCascades)
	{
		m_lpvCentersShaderData.vs.vCenterSize[0] = float4(vCamPos + vCamDir * (LPV_GRID_SIZE / 2 - LPV_STEP_COUNT) * c_aLPVsizes[0], c_aLPVsizes[0]);
		m_lpvCentersShaderData.vs.vCenterSize[1] = float4(vCamPos + vCamDir * (LPV_GRID_SIZE / 2 - LPV_STEP_COUNT) * c_aLPVsizes[1], c_aLPVsizes[1]);
		m_lpvCentersShaderData.vs.vCenterSize[2] = float4(vCamPos + vCamDir * (LPV_GRID_SIZE / 2 - LPV_STEP_COUNT) * c_aLPVsizes[2], c_aLPVsizes[2]);
		m_pLPVcentersShaderData->update(&m_lpvCentersShaderData.vs);
	}

	bool useAO = false;

	{
		static const int *pp_ssao = m_pCore->getConsole()->getPCVarInt("pp_ssao");
		int iSSAO = *pp_ssao;
		if(iSSAO < 0)
		{
			iSSAO = 0;
		}
		else if(iSSAO > 3)
		{
			iSSAO = 3;
		}
		if(iSSAO && (iCascades || m_pSkyboxTexture))
		{
			IGXDepthStencilSurface *pOldDS = pCtx->getDepthStencilSurface();
			pCtx->setDepthStencilSurface(NULL);

			IGXSurface *pSurf = pNodeData->m_pSSAOTexture->getMipmap();
			pCtx->setColorTarget(pSurf);
			mem_release(pSurf);

			pCtx->setPSTexture(m_pRndTexture, 2);
			pCtx->setPSTexture(pNodeData->m_pGBufferNormals, 3);
			pCtx->setPSTexture(pNodeData->m_pGBufferDepth, 4);

			pCtx->setSamplerState(m_pSamplerPointClamp, 0);
			pCtx->setSamplerState(m_pSamplerLinearWrap, 2);

			m_pDevice->getThreadContext()->setPSConstant(m_pSSAOrndCB, 7);

			m_pRender->bindShader(pCtx, m_idSSAOShader[iSSAO - 1]);
			m_pRender->drawScreenQuad(pCtx, pFinalTarget);

			pSurf = pNodeData->m_pSSAOTextureBlur->getMipmap();
			pCtx->setColorTarget(pSurf);
			mem_release(pSurf);
			pCtx->setPSTexture(pNodeData->m_pSSAOTexture, 0);

			m_pRender->bindShader(pCtx, m_idSSAOBlendShader);
			m_pRender->drawScreenQuad(pCtx, pFinalTarget);

			std::swap(pNodeData->m_pSSAOTexture, pNodeData->m_pSSAOTextureBlur);

			useAO = true;

			pCtx->setDepthStencilSurface(pOldDS);
			mem_release(pOldDS);


			pCtx->addTimestamp("ssao -");
		}
	}

	// Определим список лампочек, которые будут участвовать в текущем кадре
	CXLight *pLight;
	for(int i = 0, l = m_aLights.size(); i < l; ++i)
	{
		pLight = m_aLights[i];

		//если свет виден фрустуму камеры (это надо было заранее просчитать) и если свет включен
		if(pLight->isEnabled() && pLight->getRenderType() != LRT_NONE && ((1 << pLight->getLayer()) & pCamera->getLayerMask()))
		{
			if(pLight->getRenderType() & LRT_LPV)
			{
				m_pShadowCache->addRSMLight(pLight);
			}
			if(pLight->getRenderType() & LRT_LIGHT)
			{
				m_pShadowCache->addLight(pLight);
			}
		}
	}

	//static const float *r_default_fov = GET_PCVAR_FLOAT("r_default_fov");
	//static const float *r_near = GET_PCVAR_FLOAT("r_near");
	//static const float *r_far = GET_PCVAR_FLOAT("r_far");

	m_pMaterialSystem->setCullMode(GXCULL_BACK);
//	pCtx->setRasterizerState(m_pRasterizerConservative);

	IGXConstantBuffer *pOldVSCameraConst = pCtx->getVSConstant(SCR_CAMERA);
	IGXConstantBuffer *pOldPSCameraConst = pCtx->getPSConstant(SCR_CAMERA);
	IGXConstantBuffer *pOldGSCameraConst = pCtx->getGSConstant(SCR_CAMERA);
	IGXConstantBuffer *pOldCSCameraConst = pCtx->getCSConstant(SCR_CAMERA);

	UINT uShadowCount = 0;
	while((uShadowCount = m_pShadowCache->processNextBunch()))
	{
		pCtx->addTimestamp("shadows -");
		pCtx->setColorTarget(pAmbientSurf);
		pCtx->setDepthStencilSurface(pOldDSSurface);
		pCtx->setBlendState(m_pBlendAlphaOneOne);

		IBaseShadowMap *pShadow = NULL;

		//render direct light with shadows
		for(UINT i = 0; i < uShadowCount; ++i)
		{
			pLight = m_pShadowCache->getLight(i);
			pShadow = m_pShadowCache->getShadow(i);

			//если не глобальный источник
			if(pLight->getType() != LIGHT_TYPE_SUN)
			{
				//помечаем в стенсил буфере пиксели  которые входят в ограничивающий объем света, чтобы их осветить
				pCtx->setRasterizerState(m_pRasterizerCullNone);
				pCtx->setStencilRef(0);
				pCtx->setDepthStencilState(m_pDepthStencilStateLightBound);

				//отрисовка ограничивающего объема
				m_pRender->bindShader(pCtx, m_idLightBoundShader);
				pLight->drawShape(m_pDevice);

				pCtx->setStencilRef(255);
				pCtx->setDepthStencilState(m_pDepthStencilStateLightShadowNonGlobal);
			}
			else
			{
				//иначе это глобальный источник, отключаем стенсил тест
				pCtx->setDepthStencilState(m_pDepthStencilStateLightShadowGlobal);
			}

			pCtx->setRasterizerState(NULL);


			pCtx->setBlendState(m_pBlendRed);
			pShadow->genShadow(pNodeData->m_pGBufferDepth, pNodeData->m_pGBufferNormals);
			pCtx->setBlendState(m_pBlendAlphaOneOne);
			pCtx->setColorTarget(pAmbientSurf);

			//	pCtx->setPSTexture(m_pShadow, 4);
			//idshaderkit = gdata::shaders_id::kit::idComLightingShadow;
			ID idshaderkit = -1;
			switch(pLight->getType())
			{
			case LIGHT_TYPE_SPOT:
				idshaderkit = m_idComLightingSpotShadow;
				break;
			case LIGHT_TYPE_POINT:
				idshaderkit = m_idComLightingShadow;
				break;
			case LIGHT_TYPE_SUN:
				idshaderkit = m_idComLightingPSSMShadow;
				break;
			default:
				assert(!"Unknown light type!");
			}

			//теперь когда будем считать освещение надо сбросить значения в стенсил буфере, чтобы каждый кадр не чистить
			//если стенсил тест прошел успешно, устанавливаем значнеие в нуль
			if(pLight->getType() != LIGHT_TYPE_SUN)
			{
				pCtx->setDepthStencilState(m_pDepthStencilStateLightClear);
			}

			IGXConstantBuffer *pLightConstants = pLight->getConstants(m_pDevice);
			pCtx->setPSConstant(pLightConstants);
			mem_release(pLightConstants);

			m_pRender->bindShader(pCtx, idshaderkit);

			pCtx->setSamplerState(m_pSamplerPointClamp, 0);

			pCtx->setPSTexture(pNodeData->m_pGBufferColor);
			pCtx->setPSTexture(pNodeData->m_pGBufferNormals, 1);
			pCtx->setPSTexture(pNodeData->m_pGBufferParams, 2);
			pCtx->setPSTexture(pNodeData->m_pGBufferDepth, 3);
			//m_pDevice->setTexture(SGCore_GbufferGetRT(DS_RT_ADAPTEDLUM), 5);

			m_pRender->drawScreenQuad(pCtx, pFinalTarget);
		}
		pCtx->addTimestamp("lights -");
	}

	pCtx->setVSConstant(m_pLPVcentersShaderData, 9);
	pCtx->setPSConstant(m_pLPVcentersShaderData, 9);

	bool isFirstRun[3] = {true, true, true};
	while(iCascades && (uShadowCount = m_pShadowCache->processNextRSMBunch()))
	{
		pCtx->addTimestamp("rsm -");

		pCtx->setVSConstant(m_pLPVcurrentCascadeShaderData, 10);
		pCtx->setPSConstant(m_pLPVcurrentCascadeShaderData, 10);

		pCtx->setDepthStencilSurface(pOldDSSurface);
		pCtx->setBlendState(m_pBlendAlphaOneOne);

		//	pCtx->setVSConstant(m_pLightingShaderDataVS, 1);
		//	pCtx->setPSConstant(m_pLightingShaderDataPS, 1);

		IGXDepthStencilSurface *pOldSurface = pCtx->getDepthStencilSurface();
		pCtx->setDepthStencilSurface(NULL);

		IBaseReflectiveShadowMap *pShadow = NULL;

		for(UINT i = LPV_CASCADES_COUNT - iCascades; i < LPV_CASCADES_COUNT; ++i)
		{
			float4_t vTmp((float)i + 0.5f); // just to be sure
			m_pLPVcurrentCascadeShaderData->update(&vTmp);

			IGXSurface *pLPVRed = m_aLPVs[i].pGIAccumRed->asRenderTarget();
			IGXSurface *pLPVGreen = m_aLPVs[i].pGIAccumGreen->asRenderTarget();
			IGXSurface *pLPVBlue = m_aLPVs[i].pGIAccumBlue->asRenderTarget();

			//m_pDevice->setColorTarget(pAmbientSurf);
			pCtx->setColorTarget(pLPVRed);
			pCtx->setColorTarget(pLPVGreen, 1);
			pCtx->setColorTarget(pLPVBlue, 2);

			mem_release(pLPVRed);
			mem_release(pLPVGreen);
			mem_release(pLPVBlue);

			if(isFirstRun[i])
			{
				pCtx->clear(GX_CLEAR_COLOR);
				isFirstRun[i] = false;
			}

			//inject VPLs into LPV grid
			for(UINT j = 0; j < uShadowCount; ++j)
			{
				pShadow = m_pShadowCache->getRSMShadow(j);
				pShadow->genLPV();
			}

			pCtx->setColorTarget(NULL);
			pCtx->setColorTarget(NULL, 1);
			pCtx->setColorTarget(NULL, 2);
		}

		pCtx->addTimestamp("lpv_inject -");

		static const bool *dev_lpv_points = m_pCore->getConsole()->getPCVarBool("dev_lpv_points");
		if(*dev_lpv_points)
		{
			auto pTarget = pNodeData->m_pGBufferColor->asRenderTarget();
			pCtx->setColorTarget(pAmbientSurf);
			mem_release(pTarget);
			for(UINT i = 0; i < uShadowCount; ++i)
			{
				pShadow = m_pShadowCache->getRSMShadow(i);
				pShadow->genLPV(true);
			}
			pCtx->setColorTarget(NULL);
		}

		pCtx->setDepthStencilSurface(pOldSurface);
		mem_release(pOldSurface);

		//break;
	}
	//pCtx->addTimestamp("rsm -");

	bool isClean[3] = {0};
	if(iCascades)
	{
		for(UINT i = 0; i < 3; ++i)
		{
			if(isFirstRun[i])
			{
				IGXSurface *pLPVRed = m_aLPVs[i].pGIAccumRed->asRenderTarget();
				IGXSurface *pLPVGreen = m_aLPVs[i].pGIAccumGreen->asRenderTarget();
				IGXSurface *pLPVBlue = m_aLPVs[i].pGIAccumBlue->asRenderTarget();

				pCtx->setColorTarget(pLPVRed);
				pCtx->setColorTarget(pLPVGreen, 1);
				pCtx->setColorTarget(pLPVBlue, 2);

				mem_release(pLPVRed);
				mem_release(pLPVGreen);
				mem_release(pLPVBlue);

				pCtx->clear(GX_CLEAR_COLOR);
				isFirstRun[i] = false;

				pCtx->setColorTarget(NULL);
				pCtx->setColorTarget(NULL, 1);
				pCtx->setColorTarget(NULL, 2);

				isClean[i] = true;
			}
		}
	}
	m_pRender->unbindShader(pCtx);

	mem_release(pOldDSSurface);

	if(iCascades)
	{
		m_pRender->bindShader(pCtx, m_idLPVPropagateShader);
		UINT uStepCount[] = {4, 6, 8};

		//for(UINT k = 0; k < 1000; ++k)
		//{
		for(UINT j = 0; j < 3; ++j)
		{
			if(isClean[j])
			{
				continue;
			}
			for(UINT i = 0; i < uStepCount[j]; ++i)
			{
				pCtx->setCSTexture(m_aLPVs[j].pGIAccumRed, 0);
				pCtx->setCSTexture(m_aLPVs[j].pGIAccumGreen, 1);
				pCtx->setCSTexture(m_aLPVs[j].pGIAccumBlue, 2);

				pCtx->setCSUnorderedAccessView(m_aLPVs[j].pGIAccumRed2, 0);
				pCtx->setCSUnorderedAccessView(m_aLPVs[j].pGIAccumGreen2, 1);
				pCtx->setCSUnorderedAccessView(m_aLPVs[j].pGIAccumBlue2, 2);

				pCtx->computeDispatch(4, 4, 32);

				pCtx->setCSUnorderedAccessView(NULL, 0);
				pCtx->setCSUnorderedAccessView(NULL, 1);
				pCtx->setCSUnorderedAccessView(NULL, 2);

				pCtx->setCSTexture(NULL, 0);
				pCtx->setCSTexture(NULL, 1);
				pCtx->setCSTexture(NULL, 2);


				pCtx->setCSTexture(m_aLPVs[j].pGIAccumRed2, 0);
				pCtx->setCSTexture(m_aLPVs[j].pGIAccumGreen2, 1);
				pCtx->setCSTexture(m_aLPVs[j].pGIAccumBlue2, 2);

				pCtx->setCSUnorderedAccessView(m_aLPVs[j].pGIAccumRed, 0);
				pCtx->setCSUnorderedAccessView(m_aLPVs[j].pGIAccumGreen, 1);
				pCtx->setCSUnorderedAccessView(m_aLPVs[j].pGIAccumBlue, 2);

				pCtx->computeDispatch(4, 4, 32);

				pCtx->setCSUnorderedAccessView(NULL, 0);
				pCtx->setCSUnorderedAccessView(NULL, 1);
				pCtx->setCSUnorderedAccessView(NULL, 2);

				pCtx->setCSTexture(NULL, 0);
				pCtx->setCSTexture(NULL, 1);
				pCtx->setCSTexture(NULL, 2);
			}
		}
		//}

		m_pRender->unbindShader(pCtx);
		pCtx->addTimestamp("lpv_propagate -");
	}


	if(iCascades)
	{
		pCtx->setColorTarget(pAmbientSurf);
		//gdata::pDXDevice->setRasterizerState(NULL);
		pCtx->setRasterizerState(m_pRasterizerCullNone);
		pCtx->setBlendState(m_pBlendAlphaOneOne);
		pCtx->setDepthStencilState(m_pDepthStencilStateLightShadowGlobal);

		m_pRender->bindShader(pCtx, m_idComLightingGI[iCascades - 1][useAO ? 1 : 0]);

		pCtx->setSamplerState(m_pSamplerPointClamp, 0);
		pCtx->setSamplerState(m_pSamplerLinearBorder, 1);

		pCtx->setPSTexture(pNodeData->m_pGBufferDepth);
		pCtx->setPSTexture(pNodeData->m_pGBufferNormals, 1);
		pCtx->setPSTexture(pNodeData->m_pGBufferColor, 2);
		pCtx->setPSTexture(pNodeData->m_pGBufferParams, 3);
		pCtx->setPSTexture(pNodeData->m_pSSAOTexture, 13);
		for(UINT i = 0; i < 3; ++i)
		{
			pCtx->setPSTexture(m_aLPVs[i].pGIAccumRed, 4 + i);
			pCtx->setPSTexture(m_aLPVs[i].pGIAccumGreen, 7 + i);
			pCtx->setPSTexture(m_aLPVs[i].pGIAccumBlue, 10 + i);
		}

		m_pRender->drawScreenQuad(pCtx, pFinalTarget);

		pCtx->setPSTexture(NULL, 2);
		pCtx->setPSTexture(NULL, 3);
		pCtx->setPSTexture(NULL, 4);

		m_pRender->unbindShader(pCtx);
		pCtx->setSamplerState(m_pSamplerLinearClamp, 1); 
		
		pCtx->addTimestamp("lpv_apply -");
	}

	pCtx->setVSConstant(pOldVSCameraConst, SCR_CAMERA);
	pCtx->setPSConstant(pOldPSCameraConst, SCR_CAMERA);
	pCtx->setGSConstant(pOldGSCameraConst, SCR_CAMERA);
	pCtx->setCSConstant(pOldCSCameraConst, SCR_CAMERA);

	mem_release(pOldVSCameraConst);
	mem_release(pOldPSCameraConst);
	mem_release(pOldGSCameraConst);
	mem_release(pOldCSCameraConst);

	pCtx->setDepthStencilState(m_pDepthStencilStateNoZ);
	pCtx->setRasterizerState(NULL);
	pCtx->setBlendState(NULL);

	//pCtx->setColorTarget(NULL, 1);

	mem_release(pAmbientSurf);

	//-------------------------------

	//теперь необходимо все смешать чтобы получить итоговую освещенную картинку
	//{{

	static const bool *pp_sslr = m_pCore->getConsole()->getPCVarBool("pp_sslr");
	bool useSSLR = *pp_sslr;


	pCtx->setSamplerState(m_pSamplerPointClamp, 0);
	pCtx->setSamplerState(m_pSamplerLinearClamp, 1);
	pCtx->setSamplerState(m_pSamplerLinearWrap, 2);

	//m_pSSAOTexture
	//очищаем рт (в старой версии было многопроходное смешивание)
	//	pCtx->clear(GX_CLEAR_COLOR);

	pCtx->setPSTexture(pNodeData->m_pGBufferColor);
	pCtx->setPSTexture(pTempBuffer, 1);
	//pCtx->setPSTexture(m_pRndTexture, 2);
	//pCtx->setPSTexture(m_pLightSpecular, 2);
	pCtx->setPSTexture(pNodeData->m_pGBufferNormals, 3);
	pCtx->setPSTexture(pNodeData->m_pGBufferDepth, 4);
	pCtx->setPSTexture(pNodeData->m_pGBufferParams, 5);

	//

	pCtx->setColorTarget(pBackBuf);
	mem_release(pBackBuf);
	//IGXSurface *pComLightSurf = pLightTotal->getMipmap();
	//pCtx->setColorTarget(pComLightSurf);

	pCtx->setPSTexture(pNodeData->m_pSSAOTexture, 2);

	pCtx->setPSTexture(m_pSkyLightR, 6);
	pCtx->setPSTexture(m_pSkyLightG, 7);
	pCtx->setPSTexture(m_pSkyLightB, 8);

	if(!useSSLR)
	{
		IGXBaseTexture *pTex = NULL;
		if(m_pSkyboxTexture)
		{
			m_pSkyboxTexture->getAPITexture(&pTex);
		}
		pCtx->setPSTexture(pTex, 9);
		mem_release(pTex);
	}

	m_pRender->bindShader(pCtx, m_idBlendAmbientSpecDiffColor[useAO ? 1 : 0][useSSLR ? 1 : 0]);

	m_pRender->drawScreenQuad(pCtx, pFinalTarget);

	//mem_release(pComLightSurf);
	//}}

	m_pRender->unbindShader(pCtx);


	mem_release(pCamera);

	pCtx->addTimestamp("final -");
	//Sleep(16);
}
void CLightSystem::renderToneMapping(IGXBaseTexture *pSourceLight, CTonemappingGraphNodeData *pNodeData, IXRenderTarget *pFinalTarget)
{
	XPROFILE_FUNCTION();
	
	static const float *hdr_adapted_coef = m_pCore->getConsole()->getPCVarFloat("hdr_adapted_coef");
	static const float *hdr_base_value = m_pCore->getConsole()->getPCVarFloat("hdr_base_value");

	//! @todo update only on change
	m_toneMappingShaderData.fAdaptationSpeed = *hdr_adapted_coef;
	m_toneMappingShaderData.fBaseValue = *hdr_base_value;
	m_pToneMappingShaderData->update(&m_toneMappingShaderData);

	//	SGCore_ToneMappingCom(timeDelta, (hdr_adapted_coef ? (*hdr_adapted_coef) : 0.03f));
	IGXContext *pCtx = m_pDevice->getThreadContext();

	pCtx->setPSConstant(m_pToneMappingShaderData, 8);

	IGXSurface *pBackBuf = pCtx->getColorTarget();

	IGXSurface *pRT = m_pLightLuminance->asRenderTarget();
	pCtx->setColorTarget(pRT);
	mem_release(pRT);

	pCtx->setPSTexture(pSourceLight);
	pCtx->setSamplerState(m_pSamplerLinearClamp, 0);

	IGXDepthStencilSurface *pDSSurface = pCtx->getDepthStencilSurface();
	pCtx->setDepthStencilSurface(NULL);

	m_pRender->bindShader(pCtx, m_idHDRinitLuminance);
	m_pRender->drawScreenQuad(pCtx, pFinalTarget);
	m_pRender->unbindShader(pCtx);

	pCtx->setColorTarget(pBackBuf);

	// Reduction
	{
		m_pRender->bindShader(pCtx, m_idLuminanceReductionShader);

		pCtx->setCSTexture(m_pLightLuminance, 0);
		pCtx->setCSUnorderedAccessView(m_pLightLuminance32, 0);
		pCtx->computeDispatch(32, 32, 1);
		pCtx->setCSUnorderedAccessView(NULL, 0);
		pCtx->setCSTexture(NULL, 0);

		m_pRender->bindShader(pCtx, m_idLuminanceReductionMaskedShader);
		pCtx->setCSTexture(m_pLightLuminance32, 0);
		pCtx->setCSUnorderedAccessView(m_pLightLuminance1, 0);
		pCtx->computeDispatch(1, 1, 1);
		pCtx->setCSUnorderedAccessView(NULL, 0);
		pCtx->setCSTexture(NULL, 0);

		m_pRender->unbindShader(pCtx);
	}

	// Adaptation
	{
		// m_pLightLuminance1
		pCtx->setPSTexture(pNodeData->m_pAdaptedLuminance[!pNodeData->m_uCurrAdaptedLuminanceTarget]);
		pCtx->setPSTexture(m_pLightLuminance1, 1);
		IGXSurface *pAdaptedLuminanceBuf = pNodeData->m_pAdaptedLuminance[pNodeData->m_uCurrAdaptedLuminanceTarget]->asRenderTarget();
		pCtx->setColorTarget(pAdaptedLuminanceBuf);
		mem_release(pAdaptedLuminanceBuf);

		m_pRender->bindShader(pCtx, m_idHDRAdaptLuminance);

		m_pRender->drawScreenQuad(pCtx, pFinalTarget);

		m_pRender->unbindShader(pCtx);
	}


	// Bloom
	{
		m_pRender->bindShader(pCtx, m_idBloomShader);

		pCtx->setPSTexture(pSourceLight);
		pCtx->setPSTexture(pNodeData->m_pAdaptedLuminance[pNodeData->m_uCurrAdaptedLuminanceTarget], 1);
		IGXSurface *pRT = pNodeData->m_pBloom[0]->asRenderTarget();
		pCtx->setColorTarget(pRT);
		mem_release(pRT);

		pCtx->setSamplerState(m_pSamplerPointClamp, 1);

		m_pRender->drawScreenQuad(pCtx, pFinalTarget);
		m_pRender->unbindShader(pCtx);

		pCtx->setPSConstant(pNodeData->m_pGaussBlurShaderData, 8);

		m_pRender->bindShader(pCtx, m_idGaussShader[0]);
		pCtx->setPSTexture(pNodeData->m_pBloom[0], 0);
		pRT = pNodeData->m_pBloom[1]->asRenderTarget();
		pCtx->setColorTarget(pRT);
		mem_release(pRT);
		m_pRender->drawScreenQuad(pCtx, pFinalTarget);


		m_pRender->bindShader(pCtx, m_idGaussShader[1]);
		pCtx->setPSTexture(pNodeData->m_pBloom[1], 0);
		pRT = pNodeData->m_pBloom[0]->asRenderTarget();
		pCtx->setColorTarget(pRT);
		mem_release(pRT);
		m_pRender->drawScreenQuad(pCtx, pFinalTarget);


		m_pRender->unbindShader(pCtx);
		pCtx->setColorTarget(pBackBuf);
	}

	pCtx->setPSConstant(m_pToneMappingShaderData, 8);

	
	pCtx->setColorTarget(pBackBuf);
	mem_release(pBackBuf);

	{
		pCtx->setSamplerState(m_pSamplerPointClamp, 0);
		pCtx->setSamplerState(m_pSamplerLinearClamp, 1);
		pCtx->setPSTexture(pSourceLight);
		pCtx->setPSTexture(pNodeData->m_pAdaptedLuminance[pNodeData->m_uCurrAdaptedLuminanceTarget], 1);
		pCtx->setPSTexture(m_pLightLuminance1, 2);
		pCtx->setPSTexture(pNodeData->m_pBloom[0], 3);

		m_pRender->bindShader(pCtx, m_idHDRToneMapping);

		m_pRender->drawScreenQuad(pCtx, pFinalTarget);

		m_pRender->unbindShader(pCtx);
	}

	pCtx->setDepthStencilSurface(pDSSurface);
	pNodeData->m_uCurrAdaptedLuminanceTarget = !pNodeData->m_uCurrAdaptedLuminanceTarget;
}
void CLightSystem::renderDebug()
{
	const bool *dev_lpv_cubes = m_pCore->getConsole()->getPCVarBool("dev_lpv_cubes");
	if(*dev_lpv_cubes)
	{
		showGICubes();
	}
}

void CLightSystem::showGICubes()
{
	IGXContext *pCtx = m_pDevice->getThreadContext();

	pCtx->setPrimitiveTopology(GXPT_POINTLIST);
	pCtx->setRenderBuffer(m_pGICubesRB);
	m_pRender->bindShader(pCtx, m_idGICubesShader);
	//pCtx->setGSConstant(m_pLightingShaderDataVS, 1);
	pCtx->setDepthStencilState(m_pDepthStencilStateDefault);
	pCtx->setSamplerState(m_pSamplerPointClamp, 0);

	pCtx->setVSConstant(m_pLPVcurrentCascadeShaderData, 10);
	pCtx->setGSConstant(m_pLPVcurrentCascadeShaderData, 10);
	pCtx->setPSConstant(m_pLPVcurrentCascadeShaderData, 10);

	for(UINT i = 2; i < 3; ++i)
	{
		float4_t vTmp((float)i + 0.5f);  // just to be sure
		m_pLPVcurrentCascadeShaderData->update(&vTmp);

		pCtx->setPSTexture(m_aLPVs[i].pGIAccumRed2, 0);
		pCtx->setPSTexture(m_aLPVs[i].pGIAccumGreen2, 1);
		pCtx->setPSTexture(m_aLPVs[i].pGIAccumBlue2, 2);

		pCtx->drawPrimitive(0, m_uGICubesCount);
	}


	pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);

	pCtx->setPSTexture(NULL, 0);
	pCtx->setPSTexture(NULL, 1);
	pCtx->setPSTexture(NULL, 2);
}

void CLightSystem::setSkybox(IXTexture *pTexture)
{
	mem_release(m_pSkyboxTexture);
	m_pSkyboxTexture = pTexture;
	if(pTexture)
	{
		m_pSkyboxTexture->AddRef();
	}
	m_isSkyboxDirty = true;
}

void CLightSystem::buildSkyLightMasks()
{
	static const float3 s_avDirs[] = {
		float3( 0.000000f,  1.000000f,  0.000000f),
		float3( 0.000000f,  0.623191f, -0.782070f),
		float3(-0.611460f,  0.623208f, -0.487574f),
		float3(-0.762399f,  0.623228f, -0.174169f),
		float3(-0.339310f,  0.623245f,  0.704581f),
		float3( 0.339310f,  0.623245f,  0.704581f),
		float3( 0.762399f,  0.623228f,  0.174169f),
		float3( 0.611460f,  0.623208f, -0.487574f),
		float3( 0.000000f, -0.222892f, -0.974843f),
		float3(-0.762183f, -0.222906f, -0.607775f),
		float3(-0.950429f, -0.222886f,  0.216809f),
		float3(-0.422970f, -0.222901f,  0.878300f),
		float3( 0.422970f, -0.222901f,  0.878300f),
		float3( 0.950429f, -0.222886f,  0.216809f),
		float3( 0.762183f, -0.222906f, -0.607775f),
		float3( 0.000000f, -1.000000f,  0.000000f),
	};

	static_assert(ARRAYSIZE(s_avDirs) == SKYLIGHT_AREA_COUNT, "Invalid count!");

	IXResourceTextureCube *apTextures[SKYLIGHT_AREA_COUNT] = {0};

	const int MASK_SIZE = 32;

	for(UINT i = 0; i < SKYLIGHT_AREA_COUNT; ++i)
	{
		apTextures[i] = m_pCore->getResourceManager()->newResourceTextureCube();

		apTextures[i]->init(MASK_SIZE, GXFMT_R16F, 1);
	}

	float fB = -1.0f + 1.0f / MASK_SIZE;
	float fS = (MASK_SIZE > 1) ? (2.0f * (1.0f - 1.0f / MASK_SIZE) / (MASK_SIZE - 1.0f)) : 0.f;
	float fPicSize = 1.0f / MASK_SIZE;


	float16_t *apData[SKYLIGHT_AREA_COUNT];

	for(UINT face = 0; face < 6; ++face)
	{
		for(UINT i = 0; i < ARRAYSIZE(s_avDirs); ++i)
		{
			XImageMip *pMip = apTextures[i]->getMip((GXCUBEMAP_FACES)face, 0);
			pMip->isWritten = true;
			memset(pMip->pData, 0, pMip->sizeData);
			apData[i] = (float16_t*)pMip->pData;
		}

		//const byte *pSrc = pMip->pData;
		for(UINT y = 0; y < MASK_SIZE; ++y)
		{
			const float v = float(y) * fS + fB;

			for(UINT x = 0; x < MASK_SIZE; ++x)
			{
				const float u = float(x) * fS + fB;

				float ix, iy, iz;
				switch(face)
				{
				case GXCUBEMAP_FACE_POSITIVE_X:
					iz = 1.0f - (2.0f * float(x) + 1.0f) * fPicSize;
					iy = 1.0f - (2.0f * float(y) + 1.0f) * fPicSize;
					ix = 1.0f;
					break;

				case GXCUBEMAP_FACE_NEGATIVE_X:
					iz = -1.0f + (2.0f * float(x) + 1.0f) * fPicSize;
					iy = 1.0f - (2.0f * float(y) + 1.0f) * fPicSize;
					ix = -1;
					break;

				case GXCUBEMAP_FACE_POSITIVE_Y:
					iz = -1.0f + (2.0f * float(y) + 1.0f) * fPicSize;
					iy = 1.0f;
					ix = -1.0f + (2.0f * float(x) + 1.0f) * fPicSize;
					break;

				case GXCUBEMAP_FACE_NEGATIVE_Y:
					iz = 1.0f - (2.0f * float(y) + 1.0f) * fPicSize;
					iy = -1.0f;
					ix = -1.0f + (2.0f * float(x) + 1.0f) * fPicSize;
					break;

				case GXCUBEMAP_FACE_POSITIVE_Z:
					iz = 1.0f;
					iy = 1.0f - (2.0f * float(y) + 1.0f) * fPicSize;
					ix = -1.0f + (2.0f * float(x) + 1.0f) * fPicSize;
					break;

				case GXCUBEMAP_FACE_NEGATIVE_Z:
					iz = -1.0f;
					iy = 1.0f - (2.0f * float(y) + 1.0f) * fPicSize;
					ix = 1.0f - (2.0f * float(x) + 1.0f) * fPicSize;
					break;
				}

				float3 dir(ix, iy, iz);
				dir = SMVector3Normalize(dir);

				float fMax = -1.0f;
				int iMax = -1;
				for(UINT i = 0; i < ARRAYSIZE(s_avDirs); ++i)
				{
					float fDot = SMVector3Dot(dir, s_avDirs[i]);
					if(iMax < 0 || fMax < fDot)
					{
						iMax = i;
						fMax = fDot;
					}
				}

				apData[iMax][y * MASK_SIZE + x] = 1.0f;
			}
		}
	}

	char tmp[32];
	IXTexture *pTex;
	for(UINT i = 0; i < SKYLIGHT_AREA_COUNT; ++i)
	{
		sprintf(tmp, "dev_skybox_mask#%u", i);
		m_pCore->getResourceManager()->addTexture(tmp, apTextures[i]);

		m_pMaterialSystem->loadTexture(tmp, &pTex);
		pTex->getAPITexture(&m_apSkyboxMask[i]);
		mem_release(pTex);
	}
}

static float GetWeightForSize(UINT uSize)
{
	float fSize = (float)uSize;

	float fB = -1.0f + 1.0f / fSize;
	float fS = (uSize > 1) ? (2.0f*(1.0f - 1.0f / fSize) / (fSize - 1.0f)) : 0.0f;

	float fWt = 0.0f;

	for(UINT face = 0; face < 6; ++face)
	{
		for(UINT y = 0; y < uSize; ++y)
		{
			const float v = (float)y * fS + fB;

			for(UINT x = 0; x < uSize; ++x)
			{
				const float u = (float)x * fS + fB;

				fWt += 1.0f / ((1.0f + u * u + v * v) * sqrtf(1.0f + u * u + v * v));
			}
		}
	}

	return(SM_PI / fWt);
}

void CLightSystem::updateSkylight()
{
	if(!m_isSkyboxDirty)
	{
		return;
	}
	IGXContext *pCtx = m_pDevice->getThreadContext();

	IGXBaseTexture *pTex;
	if(m_pSkyboxTexture)
	{
		m_pSkyboxTexture->getAPITexture(&pTex);

		if(!pTex)
		{
			return;
		}
	}

	IGXDepthStencilSurface *pOldDS = pCtx->getDepthStencilSurface();
	pCtx->setDepthStencilSurface(NULL);


	IGXSurface *pOldRT = pCtx->getColorTarget(0);

	IGXSurface *pRenderTarget = m_pSkyLightBaseR->getMipmap();
	pCtx->setColorTarget(pRenderTarget, 0);
	mem_release(pRenderTarget);
	pRenderTarget = m_pSkyLightBaseG->getMipmap();
	pCtx->setColorTarget(pRenderTarget, 1);
	mem_release(pRenderTarget);
	pRenderTarget = m_pSkyLightBaseB->getMipmap();
	pCtx->setColorTarget(pRenderTarget, 2);
	mem_release(pRenderTarget);

	pCtx->clear(GX_CLEAR_COLOR);


	if(m_pSkyboxTexture)
	{
		SkyLightGenSHParam params = {0};
		params.uSkyboxSize = m_pSkyboxTexture->getWidth();
		params.fWeight = GetWeightForSize(params.uSkyboxSize);
		pCtx->setVSConstant(m_pSkyLightGenCB);
		pCtx->setPSConstant(m_pSkyLightGenCB);

		m_pRender->bindShader(pCtx, m_idSkylightGenSHShader);

		pCtx->setRenderBuffer(NULL);
		pCtx->setIndexBuffer(NULL);
		pCtx->setPrimitiveTopology(GXPT_POINTLIST);

		pCtx->setSamplerState(m_pSamplerLinearClamp, 0);

		pCtx->setBlendState(m_pBlendAlphaOneOne);

		pCtx->setPSTexture(pTex, 0);
		mem_release(pTex);
		for(UINT i = 0; i < SKYLIGHT_AREA_COUNT; ++i)
		{
			pCtx->setPSTexture(m_apSkyboxMask[i], 1);

			params.fTexOffset = (1.0f + (float)(i * 2)) / (float)SKYLIGHT_AREA_COUNT - 1.0f;
			m_pSkyLightGenCB->update(&params);

			pCtx->drawPrimitive(0, params.uSkyboxSize * params.uSkyboxSize * 6);
		}

		pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);

		//	m_pDevice->saveTextureToFile("skylight_r.dds", m_pSkyLightBaseR);
		//	m_pDevice->saveTextureToFile("skylight_g.dds", m_pSkyLightBaseG);
		//	m_pDevice->saveTextureToFile("skylight_b.dds", m_pSkyLightBaseB);

		// 16 probes (rgb(vec4))
		// 32 x 32 x 6 points additive

		// 3 tex (RGBA32) 16 x 1 (mrt)

		// vtx:
		// - normal
		// - pos
		// - mask?


		// 3 tex (RGBA32) 16 x 16 x 4 (probes) from mask

	}

	pCtx->setColorTarget(pOldRT, 0);
	pCtx->setColorTarget(NULL, 1);
	pCtx->setColorTarget(NULL, 2);
	mem_release(pOldRT);

	pCtx->setDepthStencilSurface(pOldDS);
	mem_release(pOldDS);

	m_isSkyboxDirty = false;

	updateSkylightGrid();
}

void CLightSystem::updateSkylightGrid()
{
	IGXContext *pCtx = m_pDevice->getThreadContext();

	uint16_t buf[16][16][4];
	if(!m_pSkyLightOcclusionCB)
	{
		m_pSkyLightOcclusionCB = m_pDevice->createConstantBuffer(sizeof(buf));
	}

	for(UINT i = 0; i < 16; ++i)
	{
		uint16_t uVal = 1 << i;
		for(UINT j = 0; j < 16; ++j)
		{
			for(UINT k = 0; k < 4; ++k)
			{
				buf[i][j][k] = 1024; // rand() & 0xFFFF;
			}
		}
	}

	m_pSkyLightOcclusionCB->update(buf);

	m_pRender->bindShader(pCtx, m_idSkylightGenGridShader);

	pCtx->setCSConstant(m_pSkyLightOcclusionCB);

	pCtx->setCSTexture(m_pSkyLightBaseR, 0);
	pCtx->setCSTexture(m_pSkyLightBaseG, 1);
	pCtx->setCSTexture(m_pSkyLightBaseB, 2);

	pCtx->setCSUnorderedAccessView(m_pSkyLightR, 0);
	pCtx->setCSUnorderedAccessView(m_pSkyLightG, 1);
	pCtx->setCSUnorderedAccessView(m_pSkyLightB, 2);

#if 0
	pCtx->computeDispatch(1, 1, 4);
#else
	pCtx->computeDispatch(1, 1, 1);
#endif

	pCtx->setCSUnorderedAccessView(NULL, 0);
	pCtx->setCSUnorderedAccessView(NULL, 1);
	pCtx->setCSUnorderedAccessView(NULL, 2);

	pCtx->setCSTexture(NULL, 0);
	pCtx->setCSTexture(NULL, 1);
	pCtx->setCSTexture(NULL, 2);
}

void CLightSystem::initShaders()
{
	ID idVS = m_pRender->loadShader(SHADER_TYPE_VERTEX, "gi_cubes.vs");
	ID idPS = m_pRender->loadShader(SHADER_TYPE_PIXEL, "gi_cubes.ps");
	ID idGS = m_pRender->loadShader(SHADER_TYPE_GEOMETRY, "gi_cubes.gs");
	m_idGICubesShader = m_pRender->createShaderKit(idVS, idPS, idGS);

	m_idLightBoundShader = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "lighting_bound.vs"), -1);

	m_idLPVPropagateShader = m_pRender->createShaderKit(-1, -1, -1, m_pRender->loadShader(SHADER_TYPE_COMPUTE, "gi_propagation.cs"));

	GXMacro aMaskedReductionMacro[] = {{"APPLY_MASK", ""}, GX_MACRO_END()};
	m_idLuminanceReductionShader = m_pRender->createShaderKit(-1, -1, -1, m_pRender->loadShader(SHADER_TYPE_COMPUTE, "hdr_reduction.cs"));
	m_idLuminanceReductionMaskedShader = m_pRender->createShaderKit(-1, -1, -1, m_pRender->loadShader(SHADER_TYPE_COMPUTE, "hdr_reduction.cs", aMaskedReductionMacro));

	m_idSkylightGenGridShader = m_pRender->createShaderKit(-1, -1, -1, m_pRender->loadShader(SHADER_TYPE_COMPUTE, "gi_skylight_grid.cs"));

	ID idScreenOut = m_pRender->loadShader(SHADER_TYPE_VERTEX, "pp_quad_render.vs");
	ID idResPos = m_pRender->loadShader(SHADER_TYPE_VERTEX, "pp_res_pos.vs");

	GXMacro Defines_IS_SHADOWED[] = {{"IS_SHADOWED", "1"}, GX_MACRO_END()};
	GXMacro Defines_IS_SPOT_SHADOWED[] = {{"IS_SHADOWED", "1"}, {"IS_SPOT", "1"}, GX_MACRO_END()};
	GXMacro Defines_IS_PSSM_SHADOWED[] = {{"IS_SHADOWED", "1"}, {"IS_PSSM", "1"}, GX_MACRO_END()};

	GXMacro Defines_USE_AO[] = {{"USE_AO", "1"}, GX_MACRO_END()};
	m_idBlendAmbientSpecDiffColor[0][0] = m_pRender->createShaderKit(idResPos, m_pRender->loadShader(SHADER_TYPE_PIXEL, "lighting_blend.ps"));
	m_idBlendAmbientSpecDiffColor[1][0] = m_pRender->createShaderKit(idResPos, m_pRender->loadShader(SHADER_TYPE_PIXEL, "lighting_blend.ps", Defines_USE_AO));
	GXMacro Defines_USE_SSLR[] = {{"USE_SSLR", "1"}, GX_MACRO_END()};
	m_idBlendAmbientSpecDiffColor[0][1] = m_pRender->createShaderKit(idResPos, m_pRender->loadShader(SHADER_TYPE_PIXEL, "lighting_blend.ps", Defines_USE_SSLR));
	GXMacro Defines_USE_AO_SSLR[] = {{"USE_SSLR", "1"}, {"USE_AO", "1"}, GX_MACRO_END()};
	m_idBlendAmbientSpecDiffColor[1][1] = m_pRender->createShaderKit(idResPos, m_pRender->loadShader(SHADER_TYPE_PIXEL, "lighting_blend.ps", Defines_USE_AO_SSLR));

	m_idComLightingShadow = m_pRender->createShaderKit(idResPos, m_pRender->loadShader(SHADER_TYPE_PIXEL, "lighting_com.ps", Defines_IS_SHADOWED));
	m_idComLightingSpotShadow = m_pRender->createShaderKit(idResPos, m_pRender->loadShader(SHADER_TYPE_PIXEL, "lighting_com.ps", Defines_IS_SPOT_SHADOWED));
	m_idComLightingPSSMShadow = m_pRender->createShaderKit(idResPos, m_pRender->loadShader(SHADER_TYPE_PIXEL, "lighting_com.ps", Defines_IS_PSSM_SHADOWED));

	// LPV_CASCADES_COUNT - iCascades
	GXMacro Defines_LPV_CASCADE_3[] = {{"LPV_START_CASCADE", "0"}, GX_MACRO_END()};
	GXMacro Defines_LPV_CASCADE_3_AO[] = {{"LPV_START_CASCADE", "0"}, {"USE_AO", "1"}, GX_MACRO_END()};
	m_idComLightingGI[2][0] = m_pRender->createShaderKit(idResPos, m_pRender->loadShader(SHADER_TYPE_PIXEL, "lighting_gi.ps", Defines_LPV_CASCADE_3));
	m_idComLightingGI[2][1] = m_pRender->createShaderKit(idResPos, m_pRender->loadShader(SHADER_TYPE_PIXEL, "lighting_gi.ps", Defines_LPV_CASCADE_3_AO));
	GXMacro Defines_LPV_CASCADE_2[] = {{"LPV_START_CASCADE", "1"}, GX_MACRO_END()};
	GXMacro Defines_LPV_CASCADE_2_AO[] = {{"LPV_START_CASCADE", "1"}, {"USE_AO", "1"}, GX_MACRO_END()};
	m_idComLightingGI[1][0] = m_pRender->createShaderKit(idResPos, m_pRender->loadShader(SHADER_TYPE_PIXEL, "lighting_gi.ps", Defines_LPV_CASCADE_2));
	m_idComLightingGI[1][1] = m_pRender->createShaderKit(idResPos, m_pRender->loadShader(SHADER_TYPE_PIXEL, "lighting_gi.ps", Defines_LPV_CASCADE_2_AO));
	GXMacro Defines_LPV_CASCADE_1[] = {{"LPV_START_CASCADE", "2"}, GX_MACRO_END()};
	GXMacro Defines_LPV_CASCADE_1_AO[] = {{"LPV_START_CASCADE", "2"}, {"USE_AO", "1"}, GX_MACRO_END()};
	m_idComLightingGI[0][0] = m_pRender->createShaderKit(idResPos, m_pRender->loadShader(SHADER_TYPE_PIXEL, "lighting_gi.ps", Defines_LPV_CASCADE_1));
	m_idComLightingGI[0][1] = m_pRender->createShaderKit(idResPos, m_pRender->loadShader(SHADER_TYPE_PIXEL, "lighting_gi.ps", Defines_LPV_CASCADE_1_AO));

	m_idHDRinitLuminance = m_pRender->createShaderKit(idScreenOut, m_pRender->loadShader(SHADER_TYPE_PIXEL, "hdr_luminance.ps"));
	m_idHDRAdaptLuminance = m_pRender->createShaderKit(idScreenOut, m_pRender->loadShader(SHADER_TYPE_PIXEL, "hdr_adapt.ps"));
	m_idHDRToneMapping = m_pRender->createShaderKit(idScreenOut, m_pRender->loadShader(SHADER_TYPE_PIXEL, "hdr_tonemapping.ps"));
	m_idBloomShader = m_pRender->createShaderKit(idScreenOut, m_pRender->loadShader(SHADER_TYPE_PIXEL, "hdr_bloom.ps"));
	m_idGaussShader[0] = m_pRender->createShaderKit(idScreenOut, m_pRender->loadShader(SHADER_TYPE_PIXEL, "ppe_gauss_blur.ps"));
	GXMacro Defines_PPE_GAUSS_HORIZONTAL[] = {{"_H_", "1"}, GX_MACRO_END()};
	m_idGaussShader[1] = m_pRender->createShaderKit(idScreenOut, m_pRender->loadShader(SHADER_TYPE_PIXEL, "ppe_gauss_blur.ps", Defines_PPE_GAUSS_HORIZONTAL));


	m_idSkylightGenSHShader = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "gi_skylight_gen_sh.vs"), m_pRender->loadShader(SHADER_TYPE_PIXEL, "gi_skylight_gen_sh.ps"));


	GXMacro Defines_SSAO_Q_3[] = {{"SSAO_Q_3", ""}, GX_MACRO_END()};
	m_idSSAOShader[2] = m_pRender->createShaderKit(idResPos, m_pRender->loadShader(SHADER_TYPE_PIXEL, "ppe_ssao.ps", Defines_SSAO_Q_3));

	GXMacro Defines_SSAO_Q_2[] = {{"SSAO_Q_2", ""}, GX_MACRO_END()};
	m_idSSAOShader[1] = m_pRender->createShaderKit(idResPos, m_pRender->loadShader(SHADER_TYPE_PIXEL, "ppe_ssao.ps", Defines_SSAO_Q_2));

	GXMacro Defines_SSAO_Q_1[] = {{"SSAO_Q_1", ""}, GX_MACRO_END()};
	m_idSSAOShader[0] = m_pRender->createShaderKit(idResPos, m_pRender->loadShader(SHADER_TYPE_PIXEL, "ppe_ssao.ps", Defines_SSAO_Q_1));

	m_idSSAOBlendShader = m_pRender->createShaderKit(idScreenOut, m_pRender->loadShader(SHADER_TYPE_PIXEL, "ppe_ssao_blur.ps"));
}

void CLightSystem::sync()
{
	for(UINT i = 0, l = m_aLights.size(); i < l; ++i)
	{
		m_aLights[i]->resetVisibility();
	}
}
