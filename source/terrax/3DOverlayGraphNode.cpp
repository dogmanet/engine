#include "3DOverlayGraphNode.h"
#include "3DOverlayGraphNodeData.h"
#include <xcommon/IXCamera.h>
#include <xcommon/IXRenderable.h>
#include "Editor.h"
#include <xcommon/IPluginManager.h>

void XRender3D();
extern CEditor *g_pEditor;

C3DOverlayGraphNode::C3DOverlayGraphNode(IXCore *pCore):
	m_pCore(pCore)
{
}
C3DOverlayGraphNode::~C3DOverlayGraphNode()
{
	mem_release(m_pDSDefault);
	mem_release(m_pAxesRenderer);
}

void XMETHODCALLTYPE C3DOverlayGraphNode::startup(IXRender *pRender)
{
	m_pRender = pRender;
	m_pDevice = pRender->getDevice();

	m_pMaterialSystem = (IXMaterialSystem*)m_pCore->getPluginManager()->getInterface(IXMATERIALSYSTEM_GUID);
	m_pRenderPassGeometry3D = m_pMaterialSystem->registerRenderPass("xEditor3DWhite", "terrax/white.ps", NULL, NULL, NULL, NULL, true);

	GXDepthStencilDesc dsDesc;
	dsDesc.cmpFuncDepth = GXCMP_GREATER_EQUAL;
	m_pDSDefault = m_pDevice->createDepthStencilState(&dsDesc);
}

void XMETHODCALLTYPE C3DOverlayGraphNode::newData(IXRenderGraphNodeData **ppData)
{
	*ppData = new C3DOverlayGraphNodeData(m_pDevice, m_pRender);
}

void XMETHODCALLTYPE C3DOverlayGraphNode::newOutputTarget(UINT uOutput, UINT uWidth, UINT uHeight, IGXBaseTexture **ppTarget)
{
	*ppTarget = m_pDevice->createTexture2D(uWidth, uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A8B8G8R8);
}

void XMETHODCALLTYPE C3DOverlayGraphNode::updateVisibility(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData *pData)
{
	C3DOverlayGraphNodeData *pTarget = (C3DOverlayGraphNodeData*)pData;
}

void XMETHODCALLTYPE C3DOverlayGraphNode::process(float fDeltaTime, IXRenderTarget *pFinalTarget, IGXBaseTexture **ppInputs, UINT uInputCount, IXRenderGraphNodeData *pData, IGXSurface **ppOutputs, UINT uOutputCount)
{
	XPROFILE_FUNCTION();

	assert(uInputCount == 1);
	assert(uOutputCount == 1);

	//IXRenderGraphNodeData *pLightInput = ppInputs[0];

	//CSceneFinalGraphNodeData *pTarget = (CSceneFinalGraphNodeData*)pOutput;

	IGXContext *pCtx = m_pDevice->getThreadContext();

	pCtx->setColorTarget(ppOutputs[0]);

	pCtx->setDepthStencilState(m_pDSDefault);

	m_pMaterialSystem->bindRenderPass(m_pRenderPassGeometry3D);

	XRender3D();


	g_pEditor->render(true);

	m_pAxesRenderer->render(false);

	pCtx->addTimestamp("3d overlay");
}

void C3DOverlayGraphNode::setAxesRenderer(IXGizmoRenderer *pRenderer)
{
	mem_release(m_pAxesRenderer);
	m_pAxesRenderer = pRenderer;
	add_ref(m_pAxesRenderer);
}

const XRenderGraphNodeInputDesc* XMETHODCALLTYPE C3DOverlayGraphNode::getInputsDesc(UINT *puCount)
{
	static XRenderGraphNodeInputDesc aDesc[] = {
		{"litScene", GXTEXTURE_TYPE_2D, GXFMT_A8B8G8R8, true}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}

const XRenderGraphNodeOutputDesc* XMETHODCALLTYPE C3DOverlayGraphNode::getOutputsDesc(UINT *puCount)
{
	static XRenderGraphNodeOutputDesc aDesc[] = {
		{"litScene", GXTEXTURE_TYPE_2D, GXFMT_A8B8G8R8}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}
