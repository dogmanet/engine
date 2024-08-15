#include "GBufferGraphNode.h"
#include "GBufferGraphNodeData.h"
#include <xcommon/IPluginManager.h>
#include <xcommon/IXCamera.h>
#include <xcommon/IXRenderable.h>

CGBufferGraphNode::CGBufferGraphNode(IXCore *pCore):
	m_pCore(pCore)
{
	IPluginManager *pPluginManager = pCore->getPluginManager();
	m_pMaterialSystem = (IXMaterialSystem*)pPluginManager->getInterface(IXMATERIALSYSTEM_GUID);
}
CGBufferGraphNode::~CGBufferGraphNode()
{
	mem_release(m_pDepthStencilStateDefault);
	mem_release(m_pDepthStencilStateNoZWrite);
	mem_release(m_pDepthStencilStateNoZ);
}

void XMETHODCALLTYPE CGBufferGraphNode::startup(IXRender *pRender)
{
	m_pRenderPassGBuffer = m_pMaterialSystem->getRenderPass("xGBuffer");

	m_pRender = pRender;
	m_pDevice = pRender->getDevice();

	GXDepthStencilDesc dsDesc;
	dsDesc.cmpFuncDepth = GXCMP_GREATER_EQUAL;
	m_pDepthStencilStateDefault = m_pDevice->createDepthStencilState(&dsDesc);

	dsDesc.useDepthWrite = FALSE;
	m_pDepthStencilStateNoZWrite = m_pDevice->createDepthStencilState(&dsDesc);

	dsDesc.useDepthTest = FALSE;
	m_pDepthStencilStateNoZ = m_pDevice->createDepthStencilState(&dsDesc);

	m_idShaderScreenOut = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "pp_quad_render.vs"), m_pRender->loadShader(SHADER_TYPE_PIXEL, "pp_quad_render.ps"));
}

void XMETHODCALLTYPE CGBufferGraphNode::newData(IXRenderGraphNodeData **ppData)
{
	*ppData = new CGBufferGraphNodeData(m_pDevice, m_pRender);
}

void XMETHODCALLTYPE CGBufferGraphNode::newOutputTarget(UINT uOutput, UINT uWidth, UINT uHeight, IGXBaseTexture **ppTarget)
{
	switch(uOutput)
	{
	case 0:
	case 1:
	case 2:
		*ppTarget = m_pDevice->createTexture2D(uWidth, uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A8B8G8R8);
		break;

	case 3:
		*ppTarget = m_pDevice->createTexture2D(uWidth, uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_R32F);
		break;
	}
}

void XMETHODCALLTYPE CGBufferGraphNode::updateVisibility(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData *pData)
{
	CGBufferGraphNodeData *pTarget = (CGBufferGraphNodeData*)pData;

	IXCamera *pCamera;
	pFinalTarget->getCamera(&pCamera);

	if(pCamera)
	{
		pCamera->updateVisibility();
	}
	mem_release(pCamera);
}

void XMETHODCALLTYPE CGBufferGraphNode::process(float fDeltaTime, IXRenderTarget *pFinalTarget, IGXBaseTexture **ppInputs, UINT uInputCount, IXRenderGraphNodeData *pData, IGXSurface **ppOutputs, UINT uOutputCount)
{
	assert(uInputCount == 0);
	assert(uOutputCount == 4);


	CGBufferGraphNodeData *pTarget = (CGBufferGraphNodeData*)pData;

	IXCamera *pCamera;
	pFinalTarget->getCamera(&pCamera);
	if(!pCamera)
	{
		return;
	}

	static const int *r_final_image = m_pCore->getConsole()->getPCVarInt("r_final_image");

	IGXContext *pCtx = m_pDevice->getThreadContext();

	pCtx->setDepthStencilSurface(pTarget->m_pDepthStencilSurface);


	m_pRender->renderStage(XRS_PREPARE);

	renderGBuffer(pCamera, ppOutputs);
	pCtx->addTimestamp("gbuffer");

	/*if(pTargetOverride)
	{
		pCtx->setColorTarget(pTargetOverride);
		showTexture(pTarget->m_pGBufferColor, pFinalTarget);
	}*/

#ifdef __TRANSIENT
	switch(*r_final_image)
	{
	case DS_RT_COLOR:
		showTexture(pTarget->m_pGBufferColor, pFinalTarget);
		goto end;
	case DS_RT_NORMAL:
		showTexture(pTarget->m_pGBufferNormals, pFinalTarget);
		goto end;
	case DS_RT_PARAM:
		showTexture(pTarget->m_pGBufferParams, pFinalTarget);
		goto end;
	case DS_RT_DEPTH:
		showTexture(pTarget->m_pGBufferDepth, pFinalTarget);
		goto end;
	}
#endif
	
//end:
	mem_release(pCamera);

#ifdef __TRANSIENT
	if(m_pLightSystem)
	{
		m_pLightSystem->renderDebug();
		pCtx->addTimestamp("debug");
	}

	//@FIXME: пока так
	SGame_RenderHUD();
	pCtx->addTimestamp("hud");

	showFrameStats();

	getXUI()->render();
	pCtx->addTimestamp("ui");
#endif
}

const XRenderGraphNodeOutputDesc* XMETHODCALLTYPE CGBufferGraphNode::getOutputsDesc(UINT *puCount)
{
	static XRenderGraphNodeOutputDesc aDesc[] = {
		{"color", GXTEXTURE_TYPE_2D, GXFMT_A8B8G8R8},
		{"normals", GXTEXTURE_TYPE_2D, GXFMT_A8B8G8R8},
		{"params", GXTEXTURE_TYPE_2D, GXFMT_A8B8G8R8},
		{"depth", GXTEXTURE_TYPE_2D, GXFMT_R32F}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}

void CGBufferGraphNode::renderGBuffer(IXCamera *pCamera, IGXSurface **ppOutputs)
{
	XPROFILE_FUNCTION();
	IGXContext *pCtx = m_pDevice->getThreadContext();

	static const bool *r_clear_color = m_pCore->getConsole()->getPCVarBool("r_clear_color");

	m_pMaterialSystem->bindRenderPass(m_pRenderPassGBuffer);

	m_pMaterialSystem->setFillMode(GXFILL_SOLID);
	m_pMaterialSystem->setCullMode(GXCULL_BACK);

	//	pCtx->setRasterizerState(NULL);
	pCtx->setDepthStencilState(m_pDepthStencilStateDefault);
	pCtx->setBlendState(NULL);
	m_pRender->setSceneFilter(pCtx);

	//?
	//Core_RIntSet(G_RI_INT_RENDERSTATE, RENDER_STATE_MATERIAL);

	IGXSurface *pColorSurf, *pNormalSurf, *pParamSurf, *pDepthMapLinearSurf;

	pColorSurf = ppOutputs[0];
	pNormalSurf = ppOutputs[1];
	pParamSurf = ppOutputs[2];
	pDepthMapLinearSurf = ppOutputs[3];

	//очищаем рт глубины  максимальным значением
	//чтобы там где нет окружения к примеру был скайбокс, а значит в рт глубины было максимальное значение - максимальная отдаленность
	pCtx->setColorTarget(pDepthMapLinearSurf);
	//	pCtx->clear(GX_CLEAR_COLOR, GX_COLOR_ARGB(255, 255, 255, 255));

	pCtx->setColorTarget(pParamSurf);
	pCtx->setColorTarget(pNormalSurf, 1);
	//	pCtx->clear(GX_CLEAR_COLOR);
	pCtx->setColorTarget(NULL, 1);

	pCtx->setColorTarget(pColorSurf);
	//pCtx->clear(GX_CLEAR_COLOR | GX_CLEAR_DEPTH | GX_CLEAR_STENCIL, RENDER_DEFAUL_BACKGROUND_COLOR, 0.0f);
	pCtx->clear(GX_CLEAR_DEPTH | GX_CLEAR_STENCIL | (*r_clear_color ? GX_CLEAR_COLOR : 0), XRENDER_DEFAULT_BACKGROUND_COLOR, 0.0f);

	pCtx->setColorTarget(pNormalSurf, 1);
	pCtx->setColorTarget(pParamSurf, 2);
	pCtx->setColorTarget(pDepthMapLinearSurf, 3);	//ставим рт глубины

	IXRenderableVisibility *pVis;
	pCamera->getVisibility(&pVis);
	m_pRender->renderStage(XRS_GBUFFER, pVis);
	mem_release(pVis);
	
	//SXAnim_Render();
	//SXDecals_Render();

	pCtx->setColorTarget(NULL, 1);
	pCtx->setColorTarget(NULL, 2);
	pCtx->setColorTarget(NULL, 3);


	/*if(GetAsyncKeyState('U') < 0)
	{
		m_pDevice->saveTextureToFile("sm_color.dds", m_pGBufferColor);
		m_pDevice->saveTextureToFile("sm_shadow.dds", m_pGBufferDepth);
	}*/
}

void CGBufferGraphNode::showTexture(IGXTexture2D *pTexture, IXRenderTarget *pFinalTarget)
{
	IGXContext *pCtx = m_pDevice->getThreadContext();

	IGXDepthStencilState *pOldState = pCtx->getDepthStencilState();
	pCtx->setDepthStencilState(m_pDepthStencilStateNoZ);

	pCtx->setRasterizerState(NULL);

	pCtx->setPSTexture(pTexture);
	m_pRender->bindShader(pCtx, m_idShaderScreenOut);

	m_pRender->drawScreenQuad(pCtx, pFinalTarget);

	pCtx->setDepthStencilState(pOldState);
	mem_release(pOldState);
}
