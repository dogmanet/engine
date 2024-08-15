#include "FXAAGraphNode.h"
#include "FXAAGraphNodeData.h"
#include <xcommon/IXCamera.h>
#include <xcommon/IXRenderable.h>
#include <game/sxgame.h>

CFXAAGraphNode::CFXAAGraphNode(IXCore *pCore):
	m_pCore(pCore)
{
	pCore->getConsole()->registerCVar("pp_fxaa", 2, "Рисовать ли эффект fxaa? 0 - нет, 1 - на низком качестве, 2 - на среднем, 3 - на высоком");
}
CFXAAGraphNode::~CFXAAGraphNode()
{
	mem_release(m_pAnisotropicFXAA);
	mem_release(m_pDepthStencilStateNoZ);
}

void XMETHODCALLTYPE CFXAAGraphNode::startup(IXRender *pRender)
{
	m_pRender = pRender;
	m_pDevice = pRender->getDevice();


	GXSamplerDesc sampDesc;
	sampDesc.addressU = sampDesc.addressV = sampDesc.addressW = GXTEXTURE_ADDRESS_CLAMP;
	sampDesc.filter = GXFILTER_ANISOTROPIC;
	sampDesc.uMaxAnisotropy = 4;
	sampDesc.comparisonFunc = GXCMP_ALWAYS;
	sampDesc.fMaxLOD = 0;
	m_pAnisotropicFXAA = m_pDevice->createSamplerState(&sampDesc);


	GXDepthStencilDesc dsDesc;
	dsDesc.cmpFuncDepth = GXCMP_GREATER_EQUAL;
	dsDesc.useDepthWrite = FALSE;
	dsDesc.useDepthTest = FALSE;
	m_pDepthStencilStateNoZ = m_pDevice->createDepthStencilState(&dsDesc);


	// "FXAA_PRESET", "1"
	ID idQuadRenderVS = m_pRender->loadShader(SHADER_TYPE_VERTEX, "pp_quad_render.vs");

	GXMacro aFXAAPreset1[] = {{"FXAA_PRESET", "1"}, GX_MACRO_END()};
	GXMacro aFXAAPreset3[] = {{"FXAA_PRESET", "3"}, GX_MACRO_END()};
	GXMacro aFXAAPreset5[] = {{"FXAA_PRESET", "5"}, GX_MACRO_END()};

	m_aidFXAAShader[0] = m_pRender->createShaderKit(idQuadRenderVS, m_pRender->loadShader(SHADER_TYPE_PIXEL, "ppe_fxaa.ps", aFXAAPreset1));
	m_aidFXAAShader[1] = m_pRender->createShaderKit(idQuadRenderVS, m_pRender->loadShader(SHADER_TYPE_PIXEL, "ppe_fxaa.ps", aFXAAPreset3));
	m_aidFXAAShader[2] = m_pRender->createShaderKit(idQuadRenderVS, m_pRender->loadShader(SHADER_TYPE_PIXEL, "ppe_fxaa.ps", aFXAAPreset5));
}

void XMETHODCALLTYPE CFXAAGraphNode::newData(IXRenderGraphNodeData **ppData)
{
	*ppData = new CFXAAGraphNodeData(m_pDevice, m_pRender);
}

void XMETHODCALLTYPE CFXAAGraphNode::newOutputTarget(UINT uOutput, UINT uWidth, UINT uHeight, IGXBaseTexture **ppTarget)
{
	*ppTarget = m_pDevice->createTexture2D(uWidth, uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A8B8G8R8);
}

void XMETHODCALLTYPE CFXAAGraphNode::updateVisibility(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData *pData)
{
	CFXAAGraphNodeData *pTarget = (CFXAAGraphNodeData*)pData;
}

void XMETHODCALLTYPE CFXAAGraphNode::process(float fDeltaTime, IXRenderTarget *pFinalTarget, IGXBaseTexture **ppInputs, UINT uInputCount, IXRenderGraphNodeData *pData, IGXSurface **ppOutputs, UINT uOutputCount)
{
	XPROFILE_FUNCTION();

	assert(uInputCount == 1);
	assert(uOutputCount == 1);

	CFXAAGraphNodeData *pTarget = (CFXAAGraphNodeData*)pData;

	IGXContext *pCtx = m_pDevice->getThreadContext();

	pCtx->setColorTarget(ppOutputs[0]);

	static const int *pp_fxaa = m_pCore->getConsole()->getPCVarInt("pp_fxaa");
	int iFXAA = *pp_fxaa;
	if(iFXAA < 1)
	{
		iFXAA = 1;
	}
	else if(iFXAA > 3)
	{
		iFXAA = 3;
	}

	//if(iFXAA)
	{
		// render FXAA
		IGXDepthStencilState *pOldState = pCtx->getDepthStencilState();
		pCtx->setDepthStencilState(m_pDepthStencilStateNoZ);

		pCtx->setRasterizerState(NULL);

		pCtx->setSamplerState(m_pAnisotropicFXAA, 3);

		pCtx->setPSTexture(ppInputs[0]);
		m_pRender->bindShader(pCtx, m_aidFXAAShader[iFXAA - 1]);
		m_pRender->drawScreenQuad(pCtx, pFinalTarget);
		m_pRender->unbindShader(pCtx);

		pCtx->setDepthStencilState(pOldState);
		mem_release(pOldState);

		pCtx->addTimestamp("fxaa");
	}
	//else
	//{
	//	pTarget->setResultOverride(pSceneTexture);
	//}
}

const XRenderGraphNodeInputDesc* XMETHODCALLTYPE CFXAAGraphNode::getInputsDesc(UINT *puCount)
{
	static XRenderGraphNodeInputDesc aDesc[] = {
		{"litScene", GXTEXTURE_TYPE_2D}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}

const XRenderGraphNodeOutputDesc* XMETHODCALLTYPE CFXAAGraphNode::getOutputsDesc(UINT *puCount)
{
	static XRenderGraphNodeOutputDesc aDesc[] = {
		{"litScene", GXTEXTURE_TYPE_2D}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}
