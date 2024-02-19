#include "SceneFinalGraphNode.h"
#include "SceneFinalGraphNodeData.h"
#include <xcommon/IXCamera.h>
#include <xcommon/IXRenderable.h>
#include <game/sxgame.h>

CSceneFinalGraphNode::CSceneFinalGraphNode(IXCore *pCore):
	m_pCore(pCore)
{
}
CSceneFinalGraphNode::~CSceneFinalGraphNode()
{
}

void XMETHODCALLTYPE CSceneFinalGraphNode::startup(IXRender *pRender)
{
	m_pRender = pRender;
	m_pDevice = pRender->getDevice();
}

void XMETHODCALLTYPE CSceneFinalGraphNode::newData(IXRenderGraphNodeData **ppData)
{
	*ppData = new CSceneFinalGraphNodeData(m_pDevice, m_pRender);
}

void XMETHODCALLTYPE CSceneFinalGraphNode::newOutputTarget(UINT uOutput, UINT uWidth, UINT uHeight, IGXBaseTexture **ppTarget)
{
	*ppTarget = m_pDevice->createTexture2D(uWidth, uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A8B8G8R8);
}

void XMETHODCALLTYPE CSceneFinalGraphNode::updateVisibility(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData *pData)
{
	CSceneFinalGraphNodeData *pTarget = (CSceneFinalGraphNodeData*)pData;
}

void XMETHODCALLTYPE CSceneFinalGraphNode::process(float fDeltaTime, IXRenderTarget *pFinalTarget, IGXBaseTexture **ppInputs, UINT uInputCount, IXRenderGraphNodeData *pData, IGXSurface **ppOutputs, UINT uOutputCount)
{
	XPROFILE_FUNCTION();

	assert(uInputCount == 1);
	assert(uOutputCount == 1);

	//IXRenderGraphNodeData *pLightInput = ppInputs[0];

	//CSceneFinalGraphNodeData *pTarget = (CSceneFinalGraphNodeData*)pOutput;

	IGXContext *pCtx = m_pDevice->getThreadContext();

	pCtx->setColorTarget(ppOutputs[0]);

	//@FIXME: Пока так
	SGame_Render(pFinalTarget);
}

const XRenderGraphNodeInputDesc* XMETHODCALLTYPE CSceneFinalGraphNode::getInputsDesc(UINT *puCount)
{
	static XRenderGraphNodeInputDesc aDesc[] = {
		{"litScene", GXTEXTURE_TYPE_2D, GXFMT_A8B8G8R8, true}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}

const XRenderGraphNodeOutputDesc* XMETHODCALLTYPE CSceneFinalGraphNode::getOutputsDesc(UINT *puCount)
{
	static XRenderGraphNodeOutputDesc aDesc[] = {
		{"litScene", GXTEXTURE_TYPE_2D, GXFMT_A8B8G8R8}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}
