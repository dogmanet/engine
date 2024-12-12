#include "Render.h"
#include "RenderableVisibility.h"
#include <xcommon/IPluginManager.h>
#include "Camera.h"
#include "RenderUtils.h"
#include <core/sxcore.h>
#include "RenderGraph.h"

CRender::CRender(IXCore *pCore):
	m_pCore(pCore)
{
	pCore->getConsole()->registerCVar("r_texfilter_type", 2, "Тип фильтрации текстур, 0 - точечная, 1 - линейная, 2 - анизотропная");
	pCore->getConsole()->registerCVar("r_texfilter_max_anisotropy", 16, "Максимальное значение анизотропной фильтрации (если включена) [1,16]");
	pCore->getConsole()->registerCVar("r_texfilter_max_miplevel", 0, "Какой mip уровень текстур использовать? 0 - самый высокий, 1 - ниже на один уровень и т.д.");

	pCore->getConsole()->registerCVar("r_clear_color", false, "Очищать буфер перед выводом кадра");

	//setupPasses();
	//setupNodes();
}

CRender::~CRender()
{
	mem_release(m_pFrameShaderData);

	for(UINT i = 0, l = m_apRenderables.size(); i < l; ++i)
	{
		m_apRenderables[i]->shutdown();
	}

	mem_release(m_pScreenVertexDeclaration);

	mem_release(m_pSamplerScene);

	mem_release(m_pCameraShaderData);

	mem_release(m_pRenderUtils);

	mem_delete(m_pShaderManager);
}

bool XMETHODCALLTYPE CRender::newFinalTarget(SXWINDOW hWnd, const char *szName, IXRenderTarget **ppOut)
{
	AAString key;
	key.setName(szName);

	if(m_mapFinalTargets.KeyExists(key))
	{
		return(false);
	}

	CFinalTarget *pNewTarget = new CFinalTarget(this, m_pDevice, hWnd);
	*ppOut = pNewTarget;
	m_mapFinalTargets[key] = {pNewTarget};

	m_aFinalTargets.push_back(&m_mapFinalTargets[key]);

	return(true);
}

bool XMETHODCALLTYPE CRender::newTextureTarget(IXRenderTarget **ppOut, char *szTextureName, size_t sizeTextureNameBuffer)
{
	CTextureTarget *pTarget = new CTextureTarget(this, m_pDevice, m_pMaterialSystem);
	*ppOut = pTarget;

	m_aTextureTargets.push_back(pTarget);

	if(szTextureName)
	{
		pTarget->getTextureName(szTextureName, sizeTextureNameBuffer);
	}

	return(true);
}

bool XMETHODCALLTYPE CRender::getFinalTarget(const char *szName, IXRenderTarget **ppOut)
{
	const Map<AAString, FinalTarget>::Node *pNode;
	if(m_mapFinalTargets.KeyExists(szName, &pNode))
	{
		add_ref(pNode->Val->pFinalTarget);
		*ppOut = pNode->Val->pFinalTarget;
		return(true);
	}

	return(false);
}

bool XMETHODCALLTYPE CRender::getGraphNode(const char *szName, IXRenderGraphNode **ppOut)
{
	const Map<AAString, GraphNode>::Node *pNode;

	if(m_mapRenderNodes.KeyExists(szName, &pNode))
	{
		add_ref(pNode->Val->pNode);
		*ppOut = pNode->Val->pNode;
		return(true);
	}

	return(false);
}

bool XMETHODCALLTYPE CRender::getGraph(const char *szName, IXRenderGraph **ppOut)
{
	const Map<AAString, CRenderGraph*>::Node *pNode;

	if(m_mapRenderGraphs.KeyExists(szName, &pNode))
	{
		add_ref(*pNode->Val);
		*ppOut = *pNode->Val;
		return(true);
	}

	return(loadGraph(szName, ppOut));
}

void CRender::onFinalTargetReleased(CFinalTarget *pTarget)
{
	for(auto i = m_mapFinalTargets.begin(); i; ++i)
	{
		if(i.second->pFinalTarget == pTarget)
		{
			int idx = m_aFinalTargets.indexOf(i.second);
			assert(idx >= 0);
			if(idx >= 0)
			{
				m_aFinalTargets.erase(idx);
			}

			m_mapFinalTargets.erase(*i.first);
			break;
		}
	}

	mem_delete(pTarget);
}

void CRender::onTextureTargetReleased(CTextureTarget *pTarget)
{
	int idx = m_aTextureTargets.indexOf(pTarget);
	if(idx >= 0)
	{
		m_aTextureTargets.erase(idx);
	}
	mem_delete(pTarget);
}

void CRender::onGraphDestroyed(CRenderGraph *pGraph)
{
	m_mapRenderGraphs.erase(pGraph->getName());
}

void XMETHODCALLTYPE CRender::updateVisibility()
{
	fora(i, m_aFinalTargets)
	{
		m_aFinalTargets[i]->pFinalTarget->updateVisibility();
	}

	fora(i, m_aTextureTargets)
	{
		m_aTextureTargets[i]->updateVisibility();
	}
}

void XMETHODCALLTYPE CRender::renderFrame(float fDeltaTime)
{
	XPROFILE_FUNCTION();

	IGXContext *pCtx = m_pDevice->getThreadContext();


	m_frameShaderData.fFrameTime = fDeltaTime;
	m_pFrameShaderData->update(&m_frameShaderData);

	pCtx->setVSConstant(m_pFrameShaderData, SCR_FRAME);
	pCtx->setPSConstant(m_pFrameShaderData, SCR_FRAME);

	IXCamera *pCamera;
	UINT uWidth;
	UINT uHeight;
	static Array<IXRenderGraphNodeData*> s_aInputs;
	fora(i, m_aFinalTargets)
	{
		FinalTarget *pFinalTarget = m_aFinalTargets[i];

		m_pCurrentRenderTarget = pFinalTarget->pFinalTarget;
		add_ref(m_pCurrentRenderTarget);

		IGXConstantBuffer *pCamConstant = NULL;
		pFinalTarget->pFinalTarget->getCamera(&pCamera);
		if(pCamera)
		{
			pFinalTarget->pFinalTarget->getSize(&uWidth, &uHeight);
			updateMatrices(pCamera, uWidth, uHeight);
			mem_release(pCamera);

			pCamConstant = m_pCameraShaderData;
		}

		pCtx->setVSConstant(pCamConstant, SCR_CAMERA);
		pCtx->setGSConstant(pCamConstant, SCR_CAMERA);
		pCtx->setPSConstant(pCamConstant, SCR_CAMERA);
		pCtx->setVSConstant(pCamConstant, SCR_OBSERVER_CAMERA);
		pCtx->setGSConstant(pCamConstant, SCR_OBSERVER_CAMERA);
		pCtx->setPSConstant(pCamConstant, SCR_OBSERVER_CAMERA);

		pFinalTarget->pFinalTarget->render(fDeltaTime);
		
		mem_release(m_pCurrentRenderTarget);
	}

	fora(i, m_aTextureTargets)
	{
		CTextureTarget *pTextureTarget = m_aTextureTargets[i];

		m_pCurrentRenderTarget = pTextureTarget;
		add_ref(m_pCurrentRenderTarget);

		IGXConstantBuffer *pCamConstant = NULL;
		pTextureTarget->getCamera(&pCamera);
		if(pCamera)
		{
			pTextureTarget->getSize(&uWidth, &uHeight);
			updateMatrices(pCamera, uWidth, uHeight);
			mem_release(pCamera);

			pCamConstant = m_pCameraShaderData;
		}

		pCtx->setVSConstant(pCamConstant, SCR_CAMERA);
		pCtx->setGSConstant(pCamConstant, SCR_CAMERA);
		pCtx->setPSConstant(pCamConstant, SCR_CAMERA);
		pCtx->setVSConstant(pCamConstant, SCR_OBSERVER_CAMERA);
		pCtx->setGSConstant(pCamConstant, SCR_OBSERVER_CAMERA);
		pCtx->setPSConstant(pCamConstant, SCR_OBSERVER_CAMERA);

		pTextureTarget->render(fDeltaTime);

		mem_release(m_pCurrentRenderTarget);
	}
}

void XMETHODCALLTYPE CRender::endFrame()
{
	fora(i, m_aFinalTargets)
	{
		m_aFinalTargets[i]->pFinalTarget->swapChain();
	}
}

void CRender::setupNodes()
{
	// scan for pipeline nodes, setup extensions
	IPluginManager *pPM = m_pCore->getPluginManager();

	Array<IXRenderGraphNode*> aExtensions;

	UINT uIFace = 0;
	IXRenderGraphNode *pRenderNode;
	while((pRenderNode = (IXRenderGraphNode*)pPM->getInterface(IXRENDERGRAPHNODE_GUID, uIFace++)))
	{
		if(pRenderNode->getVersion() == IXRENDERGRAPHNODE_VERSION)
		{
			pRenderNode->startup(this);
			if(pRenderNode->isExtension())
			{
				aExtensions.push_back(pRenderNode);
			}
			else
			{
				UINT uInputs;
				const XRenderGraphNodeInputDesc *pInputs = pRenderNode->getInputsDesc(&uInputs);

				UINT uOutputs;
				const XRenderGraphNodeOutputDesc *pOutputs = pRenderNode->getOutputsDesc(&uOutputs);

				bool isValid = true;

				for(UINT i = 0; i < uInputs; ++i)
				{
					const XRenderGraphNodeInputDesc &inp = pInputs[i];
					if(inp.isPassthrough)
					{
						// make sure there are output with the same name and of the same type
						bool isFound = false;
						for(UINT j = 0; j < uOutputs; ++j)
						{
							if(!fstrcmp(pOutputs[j].szName, inp.szName) && pOutputs[j].type == inp.type && pOutputs[j].format == inp.format)
							{
								isFound = true;
							}
						}

						if(!isFound)
						{
							isValid = false;
							LogError("[%s] Input '%s' flagged as 'passthrough' but there are no output with the same name, type and format\n");
						}
					}
				}

				if(isValid)
				{
					m_mapRenderNodes[pRenderNode->getName()].pNode = pRenderNode;
				}
			}
		}
	}

	auto fnComparator = [](IXRenderGraphNode *pA, IXRenderGraphNode *pB){
		return(pA->getExtensionDesc()->uPriority < pB->getExtensionDesc()->uPriority);
	};

	const Map<AAString, GraphNode>::Node *pNode;
	XRenderPipelineExtensionDesc *pDesc;
	fora(i, aExtensions)
	{
		pRenderNode = aExtensions[i];
		pDesc = pRenderNode->getExtensionDesc();
		assert(pDesc);

		if(pDesc && m_mapRenderNodes.KeyExists(pDesc->szTargetNodeName, &pNode))
		{
			if(pDesc->type == XRPET_PRE)
			{
				pNode->Val->aPreprocessors.push_back(pRenderNode);
			}
			else
			{
				pNode->Val->aPostprocessors.push_back(pRenderNode);
			}
		}
		else
		{
			mem_release(pRenderNode);
		}
	}
}

void CRender::setupPasses()
{
	//! При изменении базовых шейдеров отредактировать https://docs.skyxengine.com/docs/old-wiki/stdshaders/

	IPluginManager *pPluginManager = m_pCore->getPluginManager();
	m_pMaterialSystem = (IXMaterialSystem*)pPluginManager->getInterface(IXMATERIALSYSTEM_GUID);

	XVertexOutputElement voelGeneric[] = {
		{"vPosition", GXDECLTYPE_FLOAT4, GXDECLUSAGE_POSITION},
		{"vTexUV", GXDECLTYPE_FLOAT2, GXDECLUSAGE_TEXCOORD},
		{"vNormal", GXDECLTYPE_FLOAT3, GXDECLUSAGE_TEXCOORD1},
		{"vPos", GXDECLTYPE_FLOAT4, GXDECLUSAGE_TEXCOORD2},
		{"vTangent", GXDECLTYPE_FLOAT3, GXDECLUSAGE_TEXCOORD3},
		{"vBinormal", GXDECLTYPE_FLOAT3, GXDECLUSAGE_TEXCOORD4},
		{"vColor", GXDECLTYPE_FLOAT4, GXDECLUSAGE_COLOR},
		{"uInstanceId", GXDECLTYPE_UBYTE4, GXDECLUSAGE_BLENDINDICES},
		XVERTEX_OUTPUT_DECL_END()
	};
	XVertexFormatHandler *pVertexFormatSceneGeneric = m_pMaterialSystem->registerVertexFormat("xSceneGeneric", voelGeneric);

	XVertexOutputElement voelPostprocess[] = {
		{"vPosition", GXDECLTYPE_FLOAT4, GXDECLUSAGE_POSITION},
		{"vTexUV", GXDECLTYPE_FLOAT2, GXDECLUSAGE_TEXCOORD},
		{"vEyeRay", GXDECLTYPE_FLOAT3, GXDECLUSAGE_TEXCOORD1},
		{"vWorldRay", GXDECLTYPE_FLOAT3, GXDECLUSAGE_TEXCOORD2},
		XVERTEX_OUTPUT_DECL_END()
	};
	XVertexFormatHandler *pVertexFormatPostprocess = m_pMaterialSystem->registerVertexFormat("xPostprocess", voelPostprocess);
	
	XVertexOutputElement voelSky[] = {
		{"vPosition", GXDECLTYPE_FLOAT4, GXDECLUSAGE_POSITION},
		{"vTexUV", GXDECLTYPE_FLOAT3, GXDECLUSAGE_TEXCOORD},
		{"vNormal", GXDECLTYPE_FLOAT3, GXDECLUSAGE_TEXCOORD1},
		{"vPos", GXDECLTYPE_FLOAT4, GXDECLUSAGE_TEXCOORD2},
		XVERTEX_OUTPUT_DECL_END()
	};
	XVertexFormatHandler *pVertexFormatSkyBox = m_pMaterialSystem->registerVertexFormat("xSky", voelSky);

	m_pMaterialSystem->registerVertexShader(pVertexFormatPostprocess, "base/post.vs");
	{}

	{
		XRenderPassSamplersElement pSamplers[] = {
			{"Scene default", "sScene", 0},
			XRENDER_PASS_SAMPLERS_LIST_END()
		};

		XRenderPassOutputElement pOutput[] = {
			{"Base color", "vBaseColor", GXDECLTYPE_FLOAT3, "float3(1.0f, 0.0f, 0.0f)"},
			{"Normal", "vNormal", GXDECLTYPE_FLOAT3, "float3(0.0f, 0.0f, 0.0f)"},
			{"AO", "fAO", GXDECLTYPE_FLOAT1, "0.0f"},
			{"f0", "f0", GXDECLTYPE_FLOAT1, "0.04f"},
			{"Roughness", "fRoughness", GXDECLTYPE_FLOAT1, "0.95f"},
			{"Metallic", "fMetallic", GXDECLTYPE_FLOAT1, "0.0f"},
			{"Thickness", "fThickness", GXDECLTYPE_FLOAT1, "1.0f"},
			{"Lighting Coefficient", "fLightCoeff", GXDECLTYPE_FLOAT1, "1.0f"},

			// {"Depth", "fDepth", GXDECLTYPE_FLOAT1, "IN.vPos.z"},

			{"Height", "fHeight", GXDECLTYPE_FLOAT1, "1.0f"},
			XRENDER_PASS_OUTPUT_LIST_END()
		};

		m_pRenderPassGBuffer = m_pMaterialSystem->registerRenderPass("xGBuffer", "material/gbuffer.ps", NULL, pSamplers, pOutput);
	}

	{
		XRenderPassTexturesElement pTextures[] = {
			{"GBuffer color(rgb) light(a)", "txGBufferC3L1", 0},
			// {"GBuffer normals(rgb) f0(a)", "txGBufferN3F1", 1},
			{"GBuffer depth(r)", "txGBufferD1", 2},
			// {"", "3", 3}, // reserved slot
			// {"GBuffer roughness(r) metallic(g) thickness(b) AO(a)", "g_txGBufferR1M1T1AO1", 3},
			{"Lighted scene", "txScene", 4},

			{"LPV Red cascades", "txLightVolumeRed[3]", 5, GXTEXTURE_TYPE_3D},
			{"", "6", 6},
			{"", "7", 7},
			{"LPV Green cascades", "txLightVolumeGreen[3]", 8, GXTEXTURE_TYPE_3D},
			{"", "9", 9},
			{"", "10", 10},
			{"LPV Red cascades", "txLightVolumeBlue[3]", 11, GXTEXTURE_TYPE_3D},
			{"", "12", 12},
			{"", "13", 13},
			XRENDER_PASS_TEXTURES_LIST_END()
		}; 
		
		XRenderPassSamplersElement pSamplers[] = {
			{"Scene default", "sScene", 0},
			{"Point clamp", "sPointClamp", 1},
			{"Linear clamp", "sLinearClamp", 2},
			XRENDER_PASS_SAMPLERS_LIST_END()
		};

		XRenderPassOutputElement pOutput[] = {
			{"Base color", "vBaseColor", GXDECLTYPE_FLOAT4, "float4(1.0f, 0.0f, 0.0f, 0.5f)"},
			{"Normal", "vNormal", GXDECLTYPE_FLOAT3, "float3(0.0f, 0.0f, 0.0f)"},
			{"AO", "fAO", GXDECLTYPE_FLOAT1, "0.0f"},
			{"f0", "f0", GXDECLTYPE_FLOAT1, "0.04f"},
			{"Roughness", "fRoughness", GXDECLTYPE_FLOAT1, "0.95f"},
			{"Metallic", "fMetallic", GXDECLTYPE_FLOAT1, "0.0f"},
			{"Thickness", "fThickness", GXDECLTYPE_FLOAT1, "1.0f"},
			{"Lighting Coefficient", "fLightCoeff", GXDECLTYPE_FLOAT1, "1.0f"},
			{"Emissive color", "fEmissiveColor", GXDECLTYPE_FLOAT3, "float3(0.0f, 0.0f, 0.0f)"},
			XRENDER_PASS_OUTPUT_LIST_END()
		};

		m_pRenderPassTransparency = m_pMaterialSystem->registerRenderPass("xTransparency", "material/transparent.ps", pTextures, pSamplers, pOutput);
	}

	{
		XRenderPassTexturesElement pTextures[] = {
		//	{"GBuffer color(rgb) light(a)", "g_txGBufferC3L1", 0},
		//	{"GBuffer normals(rgb) f0(a)", "g_txGBufferN3F1", 1},
		//	{"GBuffer depth(r)", "g_txGBufferD1", 2},
		//	{"", "", 3}, // reserved slot
		//	// {"GBuffer roughness(r) metallic(g) thickness(b) AO(a)", "g_txGBufferR1M1T1AO1", 3},
		//	{"Lighted scene", "g_txScene", 4},
			XRENDER_PASS_TEXTURES_LIST_END()
		};

		XRenderPassSamplersElement pSamplers[] = {
			{"Scene default", "sScene", 0},
		//	{"Point clamp", "sPointClamp", 1},
			XRENDER_PASS_SAMPLERS_LIST_END()
		};

		XRenderPassOutputElement pOutput[] = {
			{"Emissive color", "vEmissiveColor", GXDECLTYPE_FLOAT3, "float3(0.0f, 0.0f, 0.0f)"},
			{"Normal", "vNormal", GXDECLTYPE_FLOAT3, "float3(0.0f, 0.0f, 0.0f)"},
			XRENDER_PASS_OUTPUT_LIST_END()
		};

		m_pRenderPassIllumination = m_pMaterialSystem->registerRenderPass("xIllumination", "material/illum.ps", pTextures, pSamplers, pOutput);
	}

	{
		XRenderPassTexturesElement pTextures[] = {
			{"GBuffer color(rgb) light(a)", "g_txGBufferC3L1", 0},
			{"GBuffer normals(rgb) f0(a)", "g_txGBufferN3F1", 1},
			{"GBuffer depth(r)", "g_txGBufferD1", 2},
			{"", "", 3}, // reserved slot
			// {"GBuffer roughness(r) metallic(g) thickness(b) AO(a)", "g_txGBufferR1M1T1AO1", 3},
			{"Lighted scene", "g_txScene", 4},
			XRENDER_PASS_TEXTURES_LIST_END()
		};

		XRenderPassSamplersElement pSamplers[] = {
			{"Scene default", "sScene", 0},
			{"Point clamp", "sPointClamp", 1},
			XRENDER_PASS_SAMPLERS_LIST_END()
		};

		XRenderPassOutputElement pOutput[] = {
			{"Color", "vColor", GXDECLTYPE_FLOAT3, "float3(1.0f, 0.0f, 0.0f)"},
			XRENDER_PASS_OUTPUT_LIST_END()
		};

		m_pRenderPassPostprocess = m_pMaterialSystem->registerRenderPass("xPostprocess", "material/post.ps", pTextures, pSamplers, pOutput);
	}

	{
		XRenderPassSamplersElement pSamplers[] = {
			{"Scene default", "sScene", 0},
			// {"Point clamp", "sPointClamp", 1},
			XRENDER_PASS_SAMPLERS_LIST_END()
		};

		XRenderPassOutputElement pOutput[] = {
			{"Base color", "vBaseColor", GXDECLTYPE_FLOAT4, "float4(0.0f, 0.0f, 0.0f, 0.0f)"},
			{"Normal", "vNormal", GXDECLTYPE_FLOAT3, "float3(0.0f, 0.0f, 0.0f)"},
			XRENDER_PASS_OUTPUT_LIST_END()
		};

		GXMacro pVariant1[] = {
			{"IS_CUBEMAP", "1"},
			{NULL, NULL}
		}; 
		GXMacro pVariant2[] = {
			{"ENABLE_RSM", "1"},
			{NULL, NULL}
		};
		GXMacro pVariant3[] = {
			{"IS_CUBEMAP", "1"},
			{"ENABLE_RSM", "1"},
			{NULL, NULL}
		};
		XRenderPassVariantElement pVariants[] = {
			pVariant1,
			pVariant2,
			pVariant3,
			NULL
		};

		m_pRenderPassShadow = m_pMaterialSystem->registerRenderPass("xShadow", "material/shadow.ps", NULL, pSamplers, pOutput, pVariants);
	}

	{
		XMaterialShaderSampler pSamplers[] = {
			{"sDefault", GXSamplerDesc()},
			XMATERIAL_SHADER_SAMPLER_LIST_END()
		};
		
		XMaterialProperty pProperties[] = {
			XMATERIAL_PARAM_TEXTURE_OPT("Base texture", "txBase", NULL), // will override generic parameter if supplied
		//	XMATERIAL_PARAM_COLOR("Some color", "vColor", float4_t(1.0f, 1.0f, 1.0f, 1.0f)),
			XMATERIAL_PARAM_FLAG("Use f0 texture", "has_f0_texture", "HAS_F0MAP"),
			XMATERIAL_PARAM_GROUP(NULL, "HAS_F0MAP"),
				XMATERIAL_PARAM_TEXTURE("f0 texture", "txF0"),
			//	XMATERIAL_PARAM_FLAG("Use f0 texture", "has_f0_texture", "HAS_F0MAP2"),
			XMATERIAL_PARAM_GROUP_END(),
			XMATERIAL_PARAM_GROUP(NULL, "!HAS_F0MAP"),
				// XMATERIAL_PARAM_FLAG("Use f0 texture", "has_f0_texture", "HAS_F0MAP2"),
				XMATERIAL_PARAM_RANGE("f0", "pbr_f0", 0.0f, 1.0f, 0.04f),
			XMATERIAL_PARAM_GROUP_END(),
			XMATERIAL_PROPERTY_LIST_END()
		};

		XMaterialShaderPass pPasses[] = {
			{m_pRenderPassGBuffer, "default/default.ps", "MainGBuffer", NULL, pSamplers, pProperties},
			{m_pRenderPassIllumination, "default/default.ps", "MainIllimination", NULL, NULL, NULL},
			{m_pRenderPassShadow, "default/default.ps", "MainShadow", NULL, NULL, NULL},
			XMATERIAL_SHADER_PASS_LIST_END()
		};

		XMaterialProperty pGenericProperties[] = {
			// parameter name, texture name
			XMATERIAL_PARAM_TEXTURE("Base texture", "txBase"),
			// parameter name, define_if_supplied
			XMATERIAL_PARAM_TEXTURE_OPT("Normal map", "txNormals", "HAS_NORMALMAP"),
			XMATERIAL_PARAM_GROUP(NULL, "HAS_NORMALMAP"),
				XMATERIAL_PARAM_RANGE("Normalmap weight", "nm_weight", -1.0f, 1.0f, 1.0f),
			XMATERIAL_PARAM_GROUP_END(),
			// parameter name, material parameter name, define_if_set
			XMATERIAL_PARAM_FLAG("Use param texture", "has_parameter_texture", "HAS_PARAMMAP"),
			// group name (optional), condition (define)
			XMATERIAL_PARAM_GROUP(NULL, "HAS_PARAMMAP"),
				XMATERIAL_PARAM_TEXTURE("Param texture", "txParameters"),
			XMATERIAL_PARAM_GROUP_END(),
			XMATERIAL_PARAM_GROUP(NULL, "!HAS_PARAMMAP"),
				// name, min, max, material parameter name
				XMATERIAL_PARAM_RANGE("Roughness", "pbr_roughness", 0.0f, 1.0f, 0.95f),
				XMATERIAL_PARAM_RANGE("Metallic", "pbr_metallic", 0.0f, 1.0f, 0.0f),
				XMATERIAL_PARAM_RANGE("Thickness", "pbr_thickness", 0.0f, 1.0f, 1.0f),
				XMATERIAL_PARAM_RANGE("AO", "pbr_ao", 0.0f, 1.0f, 1.0f),
			XMATERIAL_PARAM_GROUP_END(),

			XMATERIAL_PARAM_FLAG("Enable alphatest", "has_alphatest", "HAS_ALPHATEST"),
			XMATERIAL_PARAM_GROUP(NULL, "HAS_ALPHATEST"),
				XMATERIAL_PARAM_RANGE("Alphatest threshold", "pbr_alphatest_threshold", 0.0f, 1.0f, 0.8f),
			XMATERIAL_PARAM_GROUP_END(),

			XMATERIAL_PARAM_FLAG("Emissive", "emissive", "HAS_EMISSION"),
			XMATERIAL_PARAM_GROUP(NULL, "HAS_EMISSION"),
				XMATERIAL_PARAM_TEXTURE_OPT("Emissive map", "txEmissive", "HAS_EMISSIVE_MAP"),
				XMATERIAL_PARAM_RANGE("Emissive multiplier", "em_multiplier", 0.0f, 1000.0f, 2.0f),
			XMATERIAL_PARAM_GROUP_END(),
			XMATERIAL_PROPERTY_LIST_END()
		};

		m_pMaterialSystem->registerMaterialShader("Default", pVertexFormatSceneGeneric, pPasses, pGenericProperties);
	}

	{
		XMaterialShaderSampler pSamplers[] = {
			{"sDefault", GXSamplerDesc()},
			XMATERIAL_SHADER_SAMPLER_LIST_END()
		};

		XMaterialShaderPass pPasses[] = {
			{m_pRenderPassGBuffer, "default/terrain.ps", "MainGBuffer", NULL, pSamplers, NULL},
			{m_pRenderPassShadow, "default/terrain.ps", "MainShadow", NULL, NULL, NULL},
			XMATERIAL_SHADER_PASS_LIST_END()
		};

		XMaterialProperty pGenericProperties[] = {
			// parameter name, texture name
			XMATERIAL_PARAM_TEXTURE("Base texture", "txBase"),

			XMATERIAL_PARAM_TEXTURE("Mask texture", "txMask"),
			XMATERIAL_PARAM_TEXTURE("Detail texture 0", "txDetail0"),
			XMATERIAL_PARAM_TEXTURE("Detail texture 1", "txDetail1"),
			XMATERIAL_PARAM_TEXTURE("Detail texture 2", "txDetail2"),
			XMATERIAL_PARAM_TEXTURE("Detail texture 3", "txDetail3"),

			XMATERIAL_PARAM_RANGE("Base scale", "terrain_base_scale", 0.0f, 100.0f, 2.0f),
			XMATERIAL_PARAM_RANGE("Detail scale", "terrain_detail_scale", 0.0f, 100.0f, 10.0f),
			XMATERIAL_PARAM_RANGE("Mask scale", "terrain_mask_scale", 0.0f, 100.0f, 1.0f),

			XMATERIAL_PARAM_RANGE("Detail texture 0 multiplier", "terrain_mask_mult_0", 0.0f, 1.0f, 1.0f),
			XMATERIAL_PARAM_RANGE("Detail texture 1 multiplier", "terrain_mask_mult_1", 0.0f, 1.0f, 1.0f),
			XMATERIAL_PARAM_RANGE("Detail texture 2 multiplier", "terrain_mask_mult_2", 0.0f, 1.0f, 1.0f),
			XMATERIAL_PARAM_RANGE("Detail texture 3 multiplier", "terrain_mask_mult_3", 0.0f, 1.0f, 1.0f),

			//XMATERIAL_PARAM_TEXTURE_OPT("Normal map", "txNormals", "HAS_NORMALMAP"),
			//XMATERIAL_PARAM_GROUP(NULL, "HAS_NORMALMAP"),
			//	XMATERIAL_PARAM_RANGE("Normalmap weight", "nm_weight", -1.0f, 1.0f, 1.0f),
			//XMATERIAL_PARAM_GROUP_END(),

		//	XMATERIAL_PARAM_FLAG("Use param texture", "has_parameter_texture", "HAS_PARAMMAP"),
		//	// group name (optional), condition (define)
		//	XMATERIAL_PARAM_GROUP(NULL, "HAS_PARAMMAP"),
		//		XMATERIAL_PARAM_TEXTURE("Param texture", "txParameters"),
		//	XMATERIAL_PARAM_GROUP_END(),
		//	XMATERIAL_PARAM_GROUP(NULL, "!HAS_PARAMMAP"),
				// name, min, max, material parameter name
				XMATERIAL_PARAM_RANGE("Roughness", "pbr_roughness", 0.0f, 1.0f, 0.95f),
				XMATERIAL_PARAM_RANGE("Metallic", "pbr_metallic", 0.0f, 1.0f, 0.0f),
				XMATERIAL_PARAM_RANGE("Thickness", "pbr_thickness", 0.0f, 1.0f, 1.0f),
				XMATERIAL_PARAM_RANGE("AO", "pbr_ao", 0.0f, 1.0f, 1.0f),
		//	XMATERIAL_PARAM_GROUP_END(),


			XMATERIAL_PARAM_TEXTURE_OPT("Normal map 0", "txNormals0", "HAS_NORMALMAP_0"),
			XMATERIAL_PARAM_GROUP(NULL, "HAS_NORMALMAP_0"),
				XMATERIAL_PARAM_RANGE("Normalmap 0 weight", "nm_weight_0", -1.0f, 1.0f, 1.0f),
			XMATERIAL_PARAM_GROUP_END(),

			// parameter name, material parameter name, define_if_set
		//	XMATERIAL_PARAM_FLAG("Use param texture 0", "has_parameter_texture_0", "HAS_PARAMMAP_0"),
		//	// group name (optional), condition (define)
		//	XMATERIAL_PARAM_GROUP(NULL, "HAS_PARAMMAP_0"),
		//		XMATERIAL_PARAM_TEXTURE("Param texture 0", "txParameters0"),
		//	XMATERIAL_PARAM_GROUP_END(),
		//	XMATERIAL_PARAM_GROUP(NULL, "!HAS_PARAMMAP_0"),
				// name, min, max, material parameter name
				XMATERIAL_PARAM_RANGE("Roughness 0", "pbr_roughness_0", 0.0f, 1.0f, 0.95f),
				XMATERIAL_PARAM_RANGE("Metallic 0", "pbr_metallic_0", 0.0f, 1.0f, 0.0f),
				XMATERIAL_PARAM_RANGE("Thickness 0", "pbr_thickness_0", 0.0f, 1.0f, 1.0f),
				XMATERIAL_PARAM_RANGE("AO 0", "pbr_ao_0", 0.0f, 1.0f, 1.0f),
		//	XMATERIAL_PARAM_GROUP_END(),


			XMATERIAL_PARAM_TEXTURE_OPT("Normal map 1", "txNormals1", "HAS_NORMALMAP_1"),
			XMATERIAL_PARAM_GROUP(NULL, "HAS_NORMALMAP_1"),
				XMATERIAL_PARAM_RANGE("Normalmap 1 weight", "nm_weight_1", -1.0f, 1.0f, 1.0f),
			XMATERIAL_PARAM_GROUP_END(),

			// parameter name, material parameter name, define_if_set
		//	XMATERIAL_PARAM_FLAG("Use param texture 1", "has_parameter_texture_1", "HAS_PARAMMAP_1"),
		//	// group name (optional), condition (define)
		//	XMATERIAL_PARAM_GROUP(NULL, "HAS_PARAMMAP_1"),
		//		XMATERIAL_PARAM_TEXTURE("Param texture 1", "txParameters1"),
		//	XMATERIAL_PARAM_GROUP_END(),
		//	XMATERIAL_PARAM_GROUP(NULL, "!HAS_PARAMMAP_1"),
				// name, min, max, material parameter name
				XMATERIAL_PARAM_RANGE("Roughness 1", "pbr_roughness_1", 0.0f, 1.0f, 0.95f),
				XMATERIAL_PARAM_RANGE("Metallic 1", "pbr_metallic_1", 0.0f, 1.0f, 0.0f),
				XMATERIAL_PARAM_RANGE("Thickness 1", "pbr_thickness_1", 0.0f, 1.0f, 1.0f),
				XMATERIAL_PARAM_RANGE("AO 1", "pbr_ao_1", 0.0f, 1.0f, 1.0f),
		//	XMATERIAL_PARAM_GROUP_END(),



			XMATERIAL_PARAM_TEXTURE_OPT("Normal map 2", "txNormals2", "HAS_NORMALMAP_2"),
			XMATERIAL_PARAM_GROUP(NULL, "HAS_NORMALMAP_2"),
				XMATERIAL_PARAM_RANGE("Normalmap 2 weight", "nm_weight_2", -1.0f, 1.0f, 1.0f),
			XMATERIAL_PARAM_GROUP_END(),
			
			// parameter name, material parameter name, define_if_set
		//	XMATERIAL_PARAM_FLAG("Use param texture 2", "has_parameter_texture_2", "HAS_PARAMMAP_2"),
		//	// group name (optional), condition (define)
		//	XMATERIAL_PARAM_GROUP(NULL, "HAS_PARAMMAP_2"),
		//		XMATERIAL_PARAM_TEXTURE("Param texture 2", "txParameters2"),
		//	XMATERIAL_PARAM_GROUP_END(),
		//	XMATERIAL_PARAM_GROUP(NULL, "!HAS_PARAMMAP_2"),
				// name, min, max, material parameter name
				XMATERIAL_PARAM_RANGE("Roughness 2", "pbr_roughness_2", 0.0f, 1.0f, 0.95f),
				XMATERIAL_PARAM_RANGE("Metallic 2", "pbr_metallic_2", 0.0f, 1.0f, 0.0f),
				XMATERIAL_PARAM_RANGE("Thickness 2", "pbr_thickness_2", 0.0f, 1.0f, 1.0f),
				XMATERIAL_PARAM_RANGE("AO 2", "pbr_ao_2", 0.0f, 1.0f, 1.0f),
		//	XMATERIAL_PARAM_GROUP_END(),
			

			XMATERIAL_PARAM_TEXTURE_OPT("Normal map 3", "txNormals3", "HAS_NORMALMAP_3"),
			XMATERIAL_PARAM_GROUP(NULL, "HAS_NORMALMAP_3"),
				XMATERIAL_PARAM_RANGE("Normalmap 3 weight", "nm_weight_3", -1.0f, 1.0f, 1.0f),
			XMATERIAL_PARAM_GROUP_END(),
			
			// parameter name, material parameter name, define_if_set
		//	XMATERIAL_PARAM_FLAG("Use param texture 3", "has_parameter_texture_3", "HAS_PARAMMAP_3"),
		//	// group name (optional), condition (define)
		//	XMATERIAL_PARAM_GROUP(NULL, "HAS_PARAMMAP_3"),
		//		XMATERIAL_PARAM_TEXTURE("Param texture 3", "txParameters3"),
		//	XMATERIAL_PARAM_GROUP_END(),
		//	XMATERIAL_PARAM_GROUP(NULL, "!HAS_PARAMMAP_3"),
				// name, min, max, material parameter name
				XMATERIAL_PARAM_RANGE("Roughness 3", "pbr_roughness_3", 0.0f, 1.0f, 0.95f),
				XMATERIAL_PARAM_RANGE("Metallic 3", "pbr_metallic_3", 0.0f, 1.0f, 0.0f),
				XMATERIAL_PARAM_RANGE("Thickness 3", "pbr_thickness_3", 0.0f, 1.0f, 1.0f),
				XMATERIAL_PARAM_RANGE("AO 3", "pbr_ao_3", 0.0f, 1.0f, 1.0f),
		//	XMATERIAL_PARAM_GROUP_END(),

			XMATERIAL_PROPERTY_LIST_END()
		};

		m_pMaterialSystem->registerMaterialShader("Terrain", pVertexFormatSceneGeneric, pPasses, pGenericProperties);
	}

	{
		GXMacro pMacro[] = {
			{"WANT_WRITE_DEPTH", "1"},
			GX_MACRO_END()
		};

		XMaterialShaderPass pPasses[] = {
			{m_pRenderPassGBuffer, "default/sky.ps", "MainGBuffer", pMacro, NULL, NULL},
			{m_pRenderPassIllumination, "default/sky.ps", "MainIllimination", NULL, NULL, NULL},
			XMATERIAL_SHADER_PASS_LIST_END()
		};

		XMaterialProperty pGenericProperties[] = {
			// parameter name, texture name
			XMATERIAL_PARAM_TEXTURE_CUBE("Base texture", "txBase"),
			// parameter name, define_if_supplied
			XMATERIAL_PROPERTY_LIST_END()
		};

		m_pMaterialSystem->registerMaterialShader("Sky", pVertexFormatSkyBox, pPasses, pGenericProperties);
	}

	{
		XMaterialShaderPass pPasses[] = {
			{m_pRenderPassTransparency, "default/transparent.ps", "MainTransparency", NULL, NULL, NULL},
			{m_pRenderPassIllumination, "default/transparent.ps", "MainIllimination", NULL, NULL, NULL},
			{m_pRenderPassShadow, "default/transparent.ps", "MainShadow", NULL, NULL, NULL},
			XMATERIAL_SHADER_PASS_LIST_END()
		};

		XMaterialProperty pGenericProperties[] = {
			XMATERIAL_STATIC_FLAG("transparent", true),
			// parameter name, texture name
			XMATERIAL_PARAM_TEXTURE("Base texture", "txBase"),
			XMATERIAL_PARAM_RANGE("Transparency multiplier", "tp_multiplier", 0.0f, 1.0f, 1.0f),
			// parameter name, define_if_supplied
			XMATERIAL_PARAM_TEXTURE_OPT("Normal map", "txNormals", "HAS_NORMALMAP"),
			XMATERIAL_PARAM_GROUP(NULL, "HAS_NORMALMAP"),
				XMATERIAL_PARAM_RANGE("Normalmap weight", "nm_weight", -1.0f, 1.0f, 1.0f),
			XMATERIAL_PARAM_GROUP_END(),
			// parameter name, material parameter name, define_if_set
			XMATERIAL_PARAM_FLAG("Refraction", "refractive", "HAS_REFRACTION"),
			XMATERIAL_PARAM_FLAG("Use param texture", "has_parameter_texture", "HAS_PARAMMAP"),
			// group name (optional), condition (define)
			XMATERIAL_PARAM_GROUP(NULL, "HAS_PARAMMAP"),
				XMATERIAL_PARAM_TEXTURE("Param texture", "txParameters"),
			XMATERIAL_PARAM_GROUP_END(),
			XMATERIAL_PARAM_GROUP(NULL, "!HAS_PARAMMAP"),
				// name, min, max, material parameter name
				XMATERIAL_PARAM_RANGE("Roughness", "pbr_roughness", 0.0f, 1.0f, 0.95f),
				XMATERIAL_PARAM_RANGE("Metallic", "pbr_metallic", 0.0f, 1.0f, 0.0f),
				XMATERIAL_PARAM_RANGE("Thickness", "pbr_thickness", 0.0f, 1.0f, 1.0f),
				XMATERIAL_PARAM_RANGE("AO", "pbr_ao", 0.0f, 1.0f, 1.0f),
			XMATERIAL_PARAM_GROUP_END(),

			XMATERIAL_PROPERTY_LIST_END()
		};

		m_pMaterialSystem->registerMaterialShader("Transparent", pVertexFormatSceneGeneric, pPasses, pGenericProperties);
	}

	{
		XMaterialShaderPass pPasses[] = {
			{m_pRenderPassPostprocess, "default/postprocess.ps", "MainPostprocess", NULL, NULL, NULL},
			XMATERIAL_SHADER_PASS_LIST_END()
		};

		m_pMaterialSystem->registerMaterialShader("Postprocess", pVertexFormatPostprocess, pPasses, NULL);
	}

	IXRenderable *pRenderable;
	UINT ic = 0;
	while((pRenderable = (IXRenderable*)pPluginManager->getInterface(IXRENDERABLE_GUID, ic++)))
	{
		if(pRenderable->getVersion() == IXRENDERABLE_VERSION)
		{
			pRenderable->startup(this, m_pMaterialSystem);
			m_apRenderables.push_back(pRenderable);

			X_RENDER_STAGE stages = pRenderable->getStages();

			for(UINT idx = 0; idx < sizeof(UINT) * 8 /* bits in byte */; ++idx)
			{
				X_RENDER_STAGE stage = (X_RENDER_STAGE)(1 << idx);

				if(stages & stage)
				{
					RenderSystem rs;
					rs.pRenderable = pRenderable;
					rs.uPriority = pRenderable->getPriorityForStage(stage);
					rs.uRenderableId = m_apRenderables.size() - 1;

					m_apRenderStages[idx].aSystems.push_back(rs);
				}
			}
		}
	}

	for(UINT i = 0, l = m_apRenderStages.size(); i < l; ++i)
	{
		m_apRenderStages[i].aSystems.quickSort([](const RenderSystem &a, const RenderSystem &b)
		{
			return(a.uPriority < b.uPriority);
		});
	}

}

void XMETHODCALLTYPE CRender::newVisData(IXRenderableVisibility **ppVisibility)
{
	CRenderableVisibility *pVisibility = new CRenderableVisibility(-1, m_apRenderables.size());
	for(UINT i = 0, l = m_apRenderables.size(); i < l; ++i)
	{
		IXRenderableVisibility *pVis = NULL;
		m_apRenderables[i]->newVisData(&pVis);
		pVisibility->setVisibility(i, pVis);
	}
	*ppVisibility = pVisibility;
}

UINT CRender::getIndexForStage(X_RENDER_STAGE stage)
{
	UINT idx = 0;
	UINT stg = stage;
	while(stg >>= 1)
	{
		idx++;
	}
	return(idx);
}

void XMETHODCALLTYPE CRender::renderStage(X_RENDER_STAGE stage, IXRenderableVisibility *pVisibility)
{
	CRenderableVisibility *pVis = NULL;
	if(pVisibility)
	{
		assert(((IXRenderableVisibility*)pVisibility)->getPluginId() == -1);
		pVis = (CRenderableVisibility*)pVisibility;
	}

	auto &list = m_apRenderStages[getIndexForStage(stage)].aSystems;

	for(UINT i = 0, l = list.size(); i < l; ++i)
	{
		list[i].pRenderable->renderStage(stage, pVis ? pVis->getVisibility(list[i].uRenderableId) : NULL);
	}
}

//void CRender::renderPrepare()
//{
//	renderStage(XRS_PREPARE);
//}

IGXDevice* XMETHODCALLTYPE CRender::getDevice()
{
	return(m_pDevice);
}

void XMETHODCALLTYPE CRender::drawScreenQuad(IGXContext *pCtx, IXRenderTarget *pRenderTarget)
{
	IGXRenderBuffer *pRB = NULL;

	if(pRenderTarget)
	{
		pRB = ((CBaseTarget*)pRenderTarget)->getScreenRB();
	}
	else
	{
		pRB = m_pCurrentRenderTarget->getScreenRB();
	}

	pCtx->setRenderBuffer(pRB);
	pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);
	pCtx->drawPrimitive(0, 2);
}

void XMETHODCALLTYPE CRender::bindShader(IGXContext *pCtx, ID idShaderKit)
{
	m_pShaderManager->bind(idShaderKit, pCtx);
}

void XMETHODCALLTYPE CRender::unbindShader(IGXContext *pCtx)
{
	m_pShaderManager->unbind(pCtx);
}

ID XMETHODCALLTYPE CRender::loadShader(SHADER_TYPE type_shader, const char *szPath, GXMacro *pMacro)
{
	return(m_pShaderManager->preLoad(type_shader, szPath, pMacro));
}

ID XMETHODCALLTYPE CRender::createShaderKit(ID idVertexShader, ID idPixelShader, ID idGeometryShader, ID idComputeShader)
{
	return(m_pShaderManager->createKit(idVertexShader, idPixelShader, idGeometryShader, idComputeShader));
}

IGXVertexDeclaration* CRender::getScreenQuadVertexDeclaration()
{
	return(m_pScreenVertexDeclaration);
}

void XMETHODCALLTYPE CRender::setSceneFilter(IGXContext *pCtx, UINT uSlot)
{
	IXConsole *pConsole = m_pCore->getConsole();
	static const int *r_texfilter_type = pConsole->getPCVarInt("r_texfilter_type");
	static const int *r_texfilter_max_anisotropy = pConsole->getPCVarInt("r_texfilter_max_anisotropy");
	static const int *r_texfilter_max_miplevel = pConsole->getPCVarInt("r_texfilter_max_miplevel");

	static int r_texfilter_type2 = r_texfilter_type ? *r_texfilter_type : 1;
	static int r_texfilter_max_anisotropy2 = r_texfilter_max_anisotropy ? *r_texfilter_max_anisotropy : 1;
	static int r_texfilter_max_miplevel2 = r_texfilter_max_miplevel ? *r_texfilter_max_miplevel : 1;

	bool bChanged = false;

	if(r_texfilter_type && *r_texfilter_type != r_texfilter_type2)
	{
		r_texfilter_type2 = (*r_texfilter_type);
		bChanged = true;
	}

	if(r_texfilter_max_anisotropy && *r_texfilter_max_anisotropy != r_texfilter_max_anisotropy2)
	{
		r_texfilter_max_anisotropy2 = (*r_texfilter_max_anisotropy);
		bChanged = true;
	}

	if(r_texfilter_max_miplevel && *r_texfilter_max_miplevel != r_texfilter_max_miplevel2)
	{
		r_texfilter_max_miplevel2 = (*r_texfilter_max_miplevel);
		bChanged = true;
	}

	if(bChanged)
	{
		mem_release(m_pSamplerScene);
	}

	if(!m_pSamplerScene)
	{
		GXSamplerDesc samplerDesc;
		samplerDesc.uMaxAnisotropy = r_texfilter_max_anisotropy2;
		switch(r_texfilter_type2)
		{
		case 1:
			samplerDesc.filter = GXFILTER_MIN_MAG_MIP_LINEAR;
			break;
		case 2:
			samplerDesc.filter = GXFILTER_ANISOTROPIC;
			break;
		default:
			samplerDesc.filter = GXFILTER_MIN_MAG_MIP_POINT;
			break;
		}
		samplerDesc.fMinLOD = (float)r_texfilter_max_miplevel2;
		m_pSamplerScene = m_pDevice->createSamplerState(&samplerDesc);
	}

	pCtx->setSamplerState(m_pSamplerScene, uSlot);
}

void XMETHODCALLTYPE CRender::newCamera(IXCamera **ppCamera)
{
	IXRenderableVisibility *pVisibility;
	newVisData(&pVisibility);

	*ppCamera = new CCamera(pVisibility);

	mem_release(pVisibility);
}
void XMETHODCALLTYPE CRender::newFrustum(IXFrustum **ppFrustum)
{
	*ppFrustum = new CFrustum();
}

UINT XMETHODCALLTYPE CRender::getTransparentObjectsCount(IXRenderableVisibility *pVisibility)
{
	assert(pVisibility && pVisibility->getPluginId() == -1);
	CRenderableVisibility *pVis = (CRenderableVisibility*)pVisibility;

	auto &list = m_apRenderStages[getIndexForStage(XRS_TRANSPARENT)].aSystems;

	UINT uCount = 0;

	for(UINT i = 0, l = list.size(); i < l; ++i)
	{
		IXRenderableVisibility *pVisibility = pVis ? pVis->getVisibility(list[i].uRenderableId) : NULL;
		uCount += list[i].pRenderable->getTransparentCount(pVisibility);
	}

	return(uCount);
}
void XMETHODCALLTYPE CRender::getTransparentObjects(IXRenderableVisibility *pVisibility, XRenderTransparentObject *pObjects)
{
	assert(pVisibility && pVisibility->getPluginId() == -1);
	CRenderableVisibility *pVis = (CRenderableVisibility*)pVisibility;

	auto &list = m_apRenderStages[getIndexForStage(XRS_TRANSPARENT)].aSystems;

	UINT uTotal = 0;

	for(UINT i = 0, l = list.size(); i < l; ++i)
	{
		IXRenderableVisibility *pVisibility = pVis ? pVis->getVisibility(list[i].uRenderableId) : NULL;
		UINT uCount = list[i].pRenderable->getTransparentCount(pVisibility);
		for(UINT j = 0; j < uCount; ++j)
		{
			XRenderTransparentObject &dst = pObjects[uTotal++];
			list[i].pRenderable->getTransparentObject(pVisibility, j, &dst.obj);

			dst.uRenderable = i;
			dst.uObjectID = j;
		}
	}
}

void XMETHODCALLTYPE CRender::renderTransparentObject(IXRenderableVisibility *pMainVisibility, XRenderTransparentObject *pObject, UINT uSplitPlanes)
{
	assert(pMainVisibility && pMainVisibility->getPluginId() == -1);
	CRenderableVisibility *pVis = (CRenderableVisibility*)pMainVisibility;

	auto &list = m_apRenderStages[getIndexForStage(XRS_TRANSPARENT)].aSystems;
	IXRenderableVisibility *pVisibility = pVis ? pVis->getVisibility(list[pObject->uRenderable].uRenderableId) : NULL;

	list[pObject->uRenderable].pRenderable->renderTransparentObject(pVisibility, pObject->uObjectID, uSplitPlanes);
}

void CRender::updateMatrices(IXCamera *pCamera, UINT uTargetWidth, UINT uTargetHeight)
{
	static float2_t vWinSize;
	vWinSize = float2((float)uTargetWidth, (float)uTargetHeight);

	pCamera->updateProjection(uTargetWidth, uTargetHeight);

	float2_t vNearFar(pCamera->getNear(), pCamera->getFar());
	float fProjFov = pCamera->getEffectiveFOV();

	const SMMATRIX &mCamProj = pCamera->getProjMatrix();
	const SMMATRIX &mCamView = pCamera->getViewMatrix();

	m_cameraShaderData.mVP = SMMatrixTranspose(mCamView * mCamProj);
	m_cameraShaderData.mInvVP = SMMatrixInverse(NULL, m_cameraShaderData.mVP);
	m_cameraShaderData.mInvV = SMMatrixTranspose(SMMatrixInverse(NULL, mCamView));
	m_cameraShaderData.vPosCam = pCamera->getPosition();
	m_cameraShaderData.vNearFarInvWinSize = float4(vNearFar, 1.0f / vWinSize.x, 1.0f / vWinSize.y);
	m_cameraShaderData.vParamProj = float3(vWinSize, fProjFov);
	m_pCameraShaderData->update(&m_cameraShaderData);
}

IXRenderUtils* XMETHODCALLTYPE CRender::getUtils()
{
	return(m_pRenderUtils);
}

const GXModeDesc* XMETHODCALLTYPE CRender::getModes(UINT *puCount, IXRenderTarget *pTarget)
{
	if(pTarget)
	{
		void *isFinalTarget = NULL;
		pTarget->getInternalData(&X_IS_FINAL_TARGET_GUID, &isFinalTarget);
		if(!isFinalTarget)
		{
			return(NULL);
		}
	}

	CFinalTarget *pFT = (CFinalTarget*)pTarget;
	if(!pFT)
	{
		if(!m_aFinalTargets.size())
		{
			return(NULL);
		}

		pFT = m_aFinalTargets[0]->pFinalTarget;
	}


	IGXSwapChain *pSwapChain = pFT->getSwapChain();
	assert(pSwapChain);

	if(!pSwapChain)
	{
		return(NULL);
	}

	return(pSwapChain->getModes(puCount));
}

void XMETHODCALLTYPE CRender::initGraphics(void *pReserved)
{
	char szModuleName[64];
	sprintf_s(szModuleName, "gxgapi%s.dll", Core_0GetCommandLineArg("gapi", "dx11"));
	HMODULE m_hLibGXAPI = LoadLibrary(szModuleName);
	if(!m_hLibGXAPI)
	{
		const char *szMsg = "";
		if(GetLastError() == ERROR_MOD_NOT_FOUND && !fstrcmp(szModuleName, "gxgapidx11.dll"))
		{
			szMsg = "Please check you have D3DCompiler_47.dll in your system!\n";
		}
		LibReport(REPORT_MSG_LEVEL_FATAL, "%s - unable to load GX: %s; Error: %lu\n%s", GEN_MSG_LOCATION, szModuleName, GetLastError(), szMsg);
		return;
	}

	IGXDevice* (*libGXGetInstance)();
	libGXGetInstance = (IGXDevice*(*)())GetProcAddress(m_hLibGXAPI, "GetInstance");
	if(!libGXGetInstance)
	{
		LibReport(REPORT_MSG_LEVEL_FATAL, "%s - %s: Not a GX module!; Error: %lu\n", GEN_MSG_LOCATION, szModuleName, GetLastError());
		return;
	}

	m_pDevice = libGXGetInstance();
	if(!m_pDevice)
	{
		LibReport(REPORT_MSG_LEVEL_FATAL, "%s - %s: Cannot spawn GX context!\n", GEN_MSG_LOCATION, szModuleName);
		return;
	}
	m_pDevice->setLogger(&m_gxLogger);

	if(!m_pDevice->initDevice())
	{
		LibReport(REPORT_MSG_LEVEL_FATAL, "%s - %s: Cannot init GX context!\n", GEN_MSG_LOCATION, szModuleName);
		return;
	}

	m_pShaderManager = new CShaderManager(m_pCore, m_pDevice);

	m_pRenderUtils = new CRenderUtils(this);

	m_pFrameShaderData = m_pDevice->createConstantBuffer(sizeof(m_frameShaderData));
	m_pCameraShaderData = m_pDevice->createConstantBuffer(sizeof(m_cameraShaderData));

	GXVertexElement oLayoutQuad[] =
	{
		{0, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_POSITION},
		{0, 12, GXDECLTYPE_FLOAT3, GXDECLUSAGE_TEXCOORD},
		GX_DECL_END()
	};

	m_pScreenVertexDeclaration = m_pDevice->createVertexDeclaration(oLayoutQuad);


	setupPasses();
	setupNodes();
	setupGraphs();
}

void CRender::update(float fDelta)
{
	m_pShaderManager->allLoad();
}

void CRender::setupGraphs()
{
#if 0
	IFileSystem *pFS = m_pCore->getFileSystem();
	IFileIterator *pIter = pFS->getFileList("config/render/graph.d", "cfg");
	if(pIter)
	{
		const char *szFileName;
		while((szFileName = pIter->next()))
		{
			const char *szBasename = basename(szFileName);
			int iLen = (int)strlen(szBasename);
			char *szTmp = (char*)alloca(sizeof(char) * (iLen + 1));
			strcpy(szTmp, szBasename);
			szTmp[iLen - 4] = 0;

			IXRenderGraph *pGraph;
			loadGraph(szTmp, &pGraph);
		}
		
		mem_release(pIter);
	}
#endif
}

bool CRender::loadGraph(const char *szName, IXRenderGraph **ppOut)
{
	const char *szFormat = "config/render/graph.d/%s.cfg";
	int iLen = (int)strlen(szFormat) + (int)strlen(szName);
	char *szFileName = (char*)alloca(sizeof(char) * (iLen + 1));
	sprintf(szFileName, szFormat, szName);

	CRenderGraph *pRG = NULL;

	LogInfo("Parsing %s\n", szFileName);
	IXConfig *pConfig = m_pCore->newConfig();
	if(pConfig->open(szFileName))
	{
		pRG = new CRenderGraph(this);

		//const char *szBasename = basename(szFileName);
		//int iLen = (int)strlen(szBasename);
		//char *szTmp = (char*)alloca(sizeof(char) * (iLen + 1));
		//strcpy(szTmp, szBasename);
		//szTmp[iLen - 4] = 0;
		pRG->setName(szName);

		for(UINT i = 0, l = pConfig->getSectionCount(); i < l; ++i)
		{
			const char *szSection = pConfig->getSectionName(i);
			if(strcmp(szSection, "__output__"))
			{
				const char *szNodeName = pConfig->getKey(szSection, "node");
				if(!szNodeName)
				{
					LogError("[%s] Node instance %s has no 'node' property\n", szFileName, szSection);
					continue;
				}

				pRG->addNode(szSection, szNodeName);
			}
		}

		for(UINT i = 0, l = pConfig->getSectionCount(); i < l; ++i)
		{
			const char *szSection = pConfig->getSectionName(i);
			if(!strcmp(szSection, "__output__"))
			{
				const char *szOutput = pConfig->getKey(szSection, "in");
				if(!szOutput)
				{
					LogError("[%s] Graph output has no 'in' property\n", szFileName);
					continue;
				}

				int iLen = (int)strlen(szOutput);
				char *szTmp = (char*)alloca(sizeof(char)* (iLen + 1));
				strcpy(szTmp, szOutput);

				char *aszParts[2];

				if(parse_str(szTmp, aszParts, 2, '.') != 2)
				{
					LogError("[%s] Couldn't parse output '%s' for graph output\n", szFileName, szOutput);
					continue;
				}

				pRG->setOutput(aszParts[0], aszParts[1]);
			}
			else
			{
				for(UINT j = 0, jl = pConfig->getKeyCount(szSection); j < jl; ++j)
				{
					const char *szKey = pConfig->getKeyName(szSection, j);
					if(!memcmp(szKey, "in.", 3))
					{
						const char *szInputName = szKey + 3;
						const char *szOutput = pConfig->getKey(szSection, szKey);

						int iLen = (int)strlen(szOutput);
						char *szTmp = (char*)alloca(sizeof(char)* (iLen + 1));
						strcpy(szTmp, szOutput);

						char *aszParts[2];

						if(parse_str(szTmp, aszParts, 2, '.') != 2)
						{
							LogError("[%s] Couldn't parse output '%s' for '%s.%s'\n", szFileName, szOutput, szSection, szKey);
							continue;
						}

						pRG->connect(aszParts[0], aszParts[1], szSection, szInputName);
					}
				}
			}
		}

		if(!pRG->isValid())
		{
			mem_delete(pRG);
		}
		else
		{
			m_mapRenderGraphs[pRG->getName()] = pRG;
		}
	}
	else
	{
		LogError("[%s] File is not found\n", szFileName);
	}

	mem_release(pConfig);

	*ppOut = pRG;
	return(pRG != NULL);
}
