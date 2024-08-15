#include "CurvePreviewGraphNode.h"
#include "CurvePreviewGraphNodeData.h"
//#include <xcommon/IXCamera.h>
//#include <xcommon/IXRenderable.h>
//#include "Editor.h"
//#include <xcommon/IPluginManager.h>


CCurvePreviewGraphNode::CCurvePreviewGraphNode(IXCore *pCore):
	m_pCore(pCore)
{
}
CCurvePreviewGraphNode::~CCurvePreviewGraphNode()
{
}

void XMETHODCALLTYPE CCurvePreviewGraphNode::startup(IXRender *pRender)
{
	m_pRender = pRender;
	m_pDevice = pRender->getDevice();
}

void XMETHODCALLTYPE CCurvePreviewGraphNode::newData(IXRenderGraphNodeData **ppData)
{
	*ppData = new CCurvePreviewGraphNodeData(m_pDevice, m_pRender, m_pCore);
}

void XMETHODCALLTYPE CCurvePreviewGraphNode::newOutputTarget(UINT uOutput, UINT uWidth, UINT uHeight, IGXBaseTexture **ppTarget)
{
	*ppTarget = m_pDevice->createTexture2D(uWidth, uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A8B8G8R8);
}

void XMETHODCALLTYPE CCurvePreviewGraphNode::updateVisibility(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData *pData)
{
	CCurvePreviewGraphNodeData *pTarget = (CCurvePreviewGraphNodeData*)pData;
}

void XMETHODCALLTYPE CCurvePreviewGraphNode::process(float fDeltaTime, IXRenderTarget *pFinalTarget, IGXBaseTexture **ppInputs, UINT uInputCount, IXRenderGraphNodeData *pNodeData, IGXSurface **ppOutputs, UINT uOutputCount)
{
	XPROFILE_FUNCTION();

	assert(uInputCount == 0);
	assert(uOutputCount == 1);

	CCurvePreviewGraphNodeData *pData = (CCurvePreviewGraphNodeData*)pNodeData;

	IGXContext *pCtx = m_pDevice->getThreadContext();

	pCtx->setColorTarget(pData->m_pSurface);
	pCtx->setDepthStencilSurface(NULL);
	pCtx->clear(GX_CLEAR_COLOR, float4(0.0f, 0.0f, 0.0f, 0.0f));

	pData->m_pLineRenderer->render(true, true, false);

	pCtx->downsampleColorTarget(pData->m_pSurface, ppOutputs[0]);
}

const XRenderGraphNodeOutputDesc* XMETHODCALLTYPE CCurvePreviewGraphNode::getOutputsDesc(UINT *puCount)
{
	static XRenderGraphNodeOutputDesc aDesc[] = {
		{"scene", GXTEXTURE_TYPE_2D}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}
