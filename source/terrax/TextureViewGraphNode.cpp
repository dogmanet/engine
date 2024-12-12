#include "TextureViewGraphNode.h"
#include "TextureViewGraphNodeData.h"
//#include <xcommon/IPluginManager.h>
//#include <xcommon/IXCamera.h>
//#include <xcommon/IXRenderable.h>
#include "terrax.h"

CTextureViewGraphNode::CTextureViewGraphNode(IXCore *pCore):
	m_pCore(pCore)
{
}
CTextureViewGraphNode::~CTextureViewGraphNode()
{
}

void XMETHODCALLTYPE CTextureViewGraphNode::startup(IXRender *pRender)
{
	m_pRender = pRender;
	m_pDevice = pRender->getDevice();

	m_idScreenOutShader = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "pp_quad_render.vs"), m_pRender->loadShader(SHADER_TYPE_PIXEL, "pp_quad_render.ps"));
}

void XMETHODCALLTYPE CTextureViewGraphNode::newData(IXRenderGraphNodeData **ppData)
{
	*ppData = new CTextureViewGraphNodeData(m_pDevice, m_pRender);
}

void XMETHODCALLTYPE CTextureViewGraphNode::newOutputTarget(UINT uOutput, UINT uWidth, UINT uHeight, IGXBaseTexture **ppTarget)
{
	*ppTarget = m_pDevice->createTexture2D(uWidth, uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A8B8G8R8);
}

void XMETHODCALLTYPE CTextureViewGraphNode::updateVisibility(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData *pData)
{
}

void XMETHODCALLTYPE CTextureViewGraphNode::process(float fDeltaTime, IXRenderTarget *pFinalTarget, IGXBaseTexture **ppInputs, UINT uInputCount, IXRenderGraphNodeData *pData, IGXSurface **ppOutputs, UINT uOutputCount)
{
	assert(uInputCount == 0);
	assert(uOutputCount == 1);

	CTextureViewGraphNodeData *pTarget = (CTextureViewGraphNodeData*)pData;

	IGXContext *pCtx = m_pDevice->getThreadContext();
	pCtx->setColorTarget(ppOutputs[0]);

	pCtx->setDepthStencilSurface(NULL);

	pCtx->clear(GX_CLEAR_COLOR, float4(0, 0, 0, 0));

	pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);
	pCtx->setRasterizerState(NULL);
	//pCtx->setBlendState(g_xRenderStates.pBlendAlpha);


	//IXMaterial *pMat = NULL;
	IXTexture *pTex = pTarget->m_pTexture;

	if(pTex)
	{
		//g_matBrowserCallback.getInfo(&pMat, &pTex);

		//if(!pMat || !pMat->isTransparent())
		//{
		pCtx->setBlendState(NULL);
		//}

		m_pRender->bindShader(pCtx, m_idScreenOutShader);

		IGXBaseTexture *pTexture = NULL;

		pTex->getAPITexture(&pTexture, /*item.uCurrentFrame*/ 0);
		pCtx->setPSTexture(pTexture);
		mem_release(pTexture);

		m_pRender->drawScreenQuad(pCtx, pFinalTarget);
		m_pRender->unbindShader(pCtx);

	}
	//mem_release(pMat);
	//mem_release(pTex);
}

const XRenderGraphNodeOutputDesc* XMETHODCALLTYPE CTextureViewGraphNode::getOutputsDesc(UINT *puCount)
{
	static XRenderGraphNodeOutputDesc aDesc[] = {
		{"textureView", GXTEXTURE_TYPE_2D}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}

const XRenderGraphNodeTextureDesc* XMETHODCALLTYPE CTextureViewGraphNode::getTexturesDesc(UINT *puCount)
{
	static XRenderGraphNodeTextureDesc aDesc[] = {
		{"texture", GXTEXTURE_TYPE_2D}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}

void XMETHODCALLTYPE CTextureViewGraphNode::setTexture(IXRenderGraphNodeData *pNodeData, UINT uIdx, IXTexture *pTexture)
{
	CTextureViewGraphNodeData *pTarget = (CTextureViewGraphNodeData*)pNodeData;
	pTarget->setTexture(pTexture);
}
