#include "GradientPreviewGraphNode.h"
#include "GradientPreviewGraphNodeData.h"
//#include <xcommon/IXCamera.h>
//#include <xcommon/IXRenderable.h>
//#include "Editor.h"
//#include <xcommon/IPluginManager.h>


CGradientPreviewGraphNode::CGradientPreviewGraphNode(IXCore *pCore):
	m_pCore(pCore)
{
}
CGradientPreviewGraphNode::~CGradientPreviewGraphNode()
{
}

void XMETHODCALLTYPE CGradientPreviewGraphNode::startup(IXRender *pRender)
{
	m_pRender = pRender;
	m_pDevice = pRender->getDevice();
}

void XMETHODCALLTYPE CGradientPreviewGraphNode::newData(IXRenderGraphNodeData **ppData)
{
	*ppData = new CGradientPreviewGraphNodeData(m_pDevice, m_pRender, m_pCore);
}

void XMETHODCALLTYPE CGradientPreviewGraphNode::newOutputTarget(UINT uOutput, UINT uWidth, UINT uHeight, IGXBaseTexture **ppTarget)
{
	*ppTarget = m_pDevice->createTexture2D(uWidth, uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A8B8G8R8);
}

void XMETHODCALLTYPE CGradientPreviewGraphNode::updateVisibility(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData *pData)
{
	CGradientPreviewGraphNodeData *pTarget = (CGradientPreviewGraphNodeData*)pData;
}

void XMETHODCALLTYPE CGradientPreviewGraphNode::process(float fDeltaTime, IXRenderTarget *pFinalTarget, IGXBaseTexture **ppInputs, UINT uInputCount, IXRenderGraphNodeData *pNodeData, IGXSurface **ppOutputs, UINT uOutputCount)
{
	XPROFILE_FUNCTION();

	assert(uInputCount == 0);
	assert(uOutputCount == 1);

	CGradientPreviewGraphNodeData *pData = (CGradientPreviewGraphNodeData*)pNodeData;

	IGXContext *pCtx = m_pDevice->getThreadContext();

	pCtx->setColorTarget(pData->m_pSurface);
	pCtx->setDepthStencilSurface(NULL);
	pCtx->clear(GX_CLEAR_COLOR, float4(0.0f, 0.0f, 0.0f, 0.0f));

	pData->m_pRenderer->render(true, true, false);

	pCtx->downsampleColorTarget(pData->m_pSurface, ppOutputs[0]);
}

const XRenderGraphNodeOutputDesc* XMETHODCALLTYPE CGradientPreviewGraphNode::getOutputsDesc(UINT *puCount)
{
	static XRenderGraphNodeOutputDesc aDesc[] = {
		{"scene", GXTEXTURE_TYPE_2D}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}
