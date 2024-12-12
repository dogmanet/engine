#include "TonemappingGraphNode.h"
#include "TonemappingGraphNodeData.h"
#include <xcommon/IXCamera.h>
#include <xcommon/IXRenderable.h>
#include <game/sxgame.h>

CTonemappingGraphNode::CTonemappingGraphNode(IXCore *pCore, CLightSystem *pLightSystem):
	m_pCore(pCore),
	m_pLightSystem(pLightSystem)
{
	pCore->getConsole()->registerCVar("hdr_adapted_coef", 0.3f, "Коэфициент привыкания к освещению (0,1] (медлено, быстро)");
	pCore->getConsole()->registerCVar("hdr_base_value", 0.2f, "Базовое значение для тонмаппинга  (0,0.5] (темно, ярко)");
}
CTonemappingGraphNode::~CTonemappingGraphNode()
{
}

void XMETHODCALLTYPE CTonemappingGraphNode::startup(IXRender *pRender)
{
	m_pRender = pRender;
	m_pDevice = pRender->getDevice();
}

void XMETHODCALLTYPE CTonemappingGraphNode::newData(IXRenderGraphNodeData **ppData)
{
	*ppData = new CTonemappingGraphNodeData(m_pDevice, m_pRender);
}

void XMETHODCALLTYPE CTonemappingGraphNode::newOutputTarget(UINT uOutput, UINT uWidth, UINT uHeight, IGXBaseTexture **ppTarget)
{
	*ppTarget = m_pDevice->createTexture2D(uWidth, uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A8B8G8R8);
}

void XMETHODCALLTYPE CTonemappingGraphNode::updateVisibility(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData *pData)
{
	CTonemappingGraphNodeData *pTarget = (CTonemappingGraphNodeData*)pData;
}

void XMETHODCALLTYPE CTonemappingGraphNode::process(float fDeltaTime, IXRenderTarget *pFinalTarget, IGXBaseTexture **ppInputs, UINT uInputCount, IXRenderGraphNodeData *pData, IGXSurface **ppOutputs, UINT uOutputCount)
{
	XPROFILE_FUNCTION();

	assert(uInputCount == 1);
	assert(uOutputCount == 1);

	CTonemappingGraphNodeData *pTarget = (CTonemappingGraphNodeData*)pData;

	IGXContext *pCtx = m_pDevice->getThreadContext();

	pCtx->setColorTarget(ppOutputs[0]);
		
	m_pLightSystem->renderToneMapping(ppInputs[0], pTarget, pFinalTarget);

	pCtx->addTimestamp("tonemapping");
}

const XRenderGraphNodeInputDesc* XMETHODCALLTYPE CTonemappingGraphNode::getInputsDesc(UINT *puCount)
{
	static XRenderGraphNodeInputDesc aDesc[] = {
		{"litScene", GXTEXTURE_TYPE_2D}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}

const XRenderGraphNodeOutputDesc* XMETHODCALLTYPE CTonemappingGraphNode::getOutputsDesc(UINT *puCount)
{
	static XRenderGraphNodeOutputDesc aDesc[] = {
		{"litScene", GXTEXTURE_TYPE_2D}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}
