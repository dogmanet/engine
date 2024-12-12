#include "CurveEditorGraphNode.h"
#include "CurveEditorGraphNodeData.h"
//#include <xcommon/IXCamera.h>
//#include <xcommon/IXRenderable.h>
//#include "Editor.h"
//#include <xcommon/IPluginManager.h>
#include "terrax.h"


CCurveEditorGraphNode::CCurveEditorGraphNode(IXCore *pCore):
	m_pCore(pCore)
{
}
CCurveEditorGraphNode::~CCurveEditorGraphNode()
{
}

void XMETHODCALLTYPE CCurveEditorGraphNode::startup(IXRender *pRender)
{
	m_pRender = pRender;
	m_pDevice = pRender->getDevice();
}

void XMETHODCALLTYPE CCurveEditorGraphNode::newData(IXRenderGraphNodeData **ppData)
{
	*ppData = new CCurveEditorGraphNodeData(m_pDevice, m_pRender, m_pCore);
}

void XMETHODCALLTYPE CCurveEditorGraphNode::newOutputTarget(UINT uOutput, UINT uWidth, UINT uHeight, IGXBaseTexture **ppTarget)
{
	*ppTarget = m_pDevice->createTexture2D(uWidth, uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A8B8G8R8);
}

void XMETHODCALLTYPE CCurveEditorGraphNode::updateVisibility(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData *pData)
{
	CCurveEditorGraphNodeData *pTarget = (CCurveEditorGraphNodeData*)pData;
}

void XMETHODCALLTYPE CCurveEditorGraphNode::process(float fDeltaTime, IXRenderTarget *pFinalTarget, IGXBaseTexture **ppInputs, UINT uInputCount, IXRenderGraphNodeData *pNodeData, IGXSurface **ppOutputs, UINT uOutputCount)
{
	XPROFILE_FUNCTION();

	assert(uInputCount == 0);
	assert(uOutputCount == 1);

	CCurveEditorGraphNodeData *pData = (CCurveEditorGraphNodeData*)pNodeData;

	IGXContext *pCtx = m_pDevice->getThreadContext();

	pCtx->setColorTarget(pData->m_pSurface);
	pCtx->setDepthStencilSurface(NULL);
	pCtx->clear(GX_CLEAR_COLOR, float4(0.0f, 0.0f, 0.0f, 0.0f));

	pData->m_pRenderer->render(true, true, false);
	pData->m_pLineRenderer->render(true, true, false);

	pCtx->downsampleColorTarget(pData->m_pSurface, ppOutputs[0]);
}

const XRenderGraphNodeOutputDesc* XMETHODCALLTYPE CCurveEditorGraphNode::getOutputsDesc(UINT *puCount)
{
	static XRenderGraphNodeOutputDesc aDesc[] = {
		{"scene", GXTEXTURE_TYPE_2D}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}
