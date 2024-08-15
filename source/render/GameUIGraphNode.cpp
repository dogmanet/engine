#include "GameUIGraphNode.h"
#include "GameUIGraphNodeData.h"
#include <xcommon/IXCamera.h>
#include <xcommon/IXRenderable.h>
#include <game/sxgame.h>

CGameUIGraphNode::CGameUIGraphNode(IXCore *pCore):
	m_pCore(pCore)
{
}
CGameUIGraphNode::~CGameUIGraphNode()
{
}

void XMETHODCALLTYPE CGameUIGraphNode::startup(IXRender *pRender)
{
	m_pRender = pRender;
	m_pDevice = pRender->getDevice();
}

void XMETHODCALLTYPE CGameUIGraphNode::newData(IXRenderGraphNodeData **ppData)
{
	*ppData = new CGameUIGraphNodeData(m_pDevice, m_pRender);
}

void XMETHODCALLTYPE CGameUIGraphNode::newOutputTarget(UINT uOutput, UINT uWidth, UINT uHeight, IGXBaseTexture **ppTarget)
{
	*ppTarget = m_pDevice->createTexture2D(uWidth, uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A8B8G8R8);
}

void XMETHODCALLTYPE CGameUIGraphNode::updateVisibility(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData *pData)
{
	CGameUIGraphNodeData *pTarget = (CGameUIGraphNodeData*)pData;
}

void XMETHODCALLTYPE CGameUIGraphNode::process(float fDeltaTime, IXRenderTarget *pFinalTarget, IGXBaseTexture **ppInputs, UINT uInputCount, IXRenderGraphNodeData *pData, IGXSurface **ppOutputs, UINT uOutputCount)
{
	XPROFILE_FUNCTION();

	assert(uInputCount == 1);
	assert(uOutputCount == 1);

	//IXRenderGraphNodeData *pLightInput = ppInputs[0];

	//CSceneFinalGraphNodeData *pTarget = (CSceneFinalGraphNodeData*)pOutput;

	IGXContext *pCtx = m_pDevice->getThreadContext();

	pCtx->setColorTarget(ppOutputs[0]);

	//@FIXME: пока так
	SGame_RenderHUD();
	pCtx->addTimestamp("hud");
}

const XRenderGraphNodeInputDesc* XMETHODCALLTYPE CGameUIGraphNode::getInputsDesc(UINT *puCount)
{
	static XRenderGraphNodeInputDesc aDesc[] = {
		{"litScene", GXTEXTURE_TYPE_2D, GXFMT_A8B8G8R8, true}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}

const XRenderGraphNodeOutputDesc* XMETHODCALLTYPE CGameUIGraphNode::getOutputsDesc(UINT *puCount)
{
	static XRenderGraphNodeOutputDesc aDesc[] = {
		{"litScene", GXTEXTURE_TYPE_2D, GXFMT_A8B8G8R8}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}
