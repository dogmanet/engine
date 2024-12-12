#include "MaterialBrowserGraphNode.h"
#include "MaterialBrowserGraphNodeData.h"
//#include <xcommon/IXCamera.h>
//#include <xcommon/IXRenderable.h>
//#include "Editor.h"
//#include <xcommon/IPluginManager.h>
#include "terrax.h"


CMaterialBrowserGraphNode::CMaterialBrowserGraphNode(IXCore *pCore):
	m_pCore(pCore)
{
}
CMaterialBrowserGraphNode::~CMaterialBrowserGraphNode()
{
}

void XMETHODCALLTYPE CMaterialBrowserGraphNode::startup(IXRender *pRender)
{
	m_pRender = pRender;
	m_pDevice = pRender->getDevice();
}

void XMETHODCALLTYPE CMaterialBrowserGraphNode::newData(IXRenderGraphNodeData **ppData)
{
	*ppData = new CMaterialBrowserGraphNodeData(m_pDevice, m_pRender);
}

void XMETHODCALLTYPE CMaterialBrowserGraphNode::newOutputTarget(UINT uOutput, UINT uWidth, UINT uHeight, IGXBaseTexture **ppTarget)
{
	*ppTarget = m_pDevice->createTexture2D(uWidth, uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A8B8G8R8);
}

void XMETHODCALLTYPE CMaterialBrowserGraphNode::updateVisibility(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData *pData)
{
	CMaterialBrowserGraphNodeData *pTarget = (CMaterialBrowserGraphNodeData*)pData;
}

void XMETHODCALLTYPE CMaterialBrowserGraphNode::process(float fDeltaTime, IXRenderTarget *pFinalTarget, IGXBaseTexture **ppInputs, UINT uInputCount, IXRenderGraphNodeData *pData, IGXSurface **ppOutputs, UINT uOutputCount)
{
	XPROFILE_FUNCTION();

	assert(uInputCount == 0);
	assert(uOutputCount == 1);

	//CSceneFinalGraphNodeData *pTarget = (CSceneFinalGraphNodeData*)pOutput;

	IGXContext *pCtx = m_pDevice->getThreadContext();

	pCtx->setColorTarget(ppOutputs[0]);

	g_pMaterialBrowser->render();
}

const XRenderGraphNodeOutputDesc* XMETHODCALLTYPE CMaterialBrowserGraphNode::getOutputsDesc(UINT *puCount)
{
	static XRenderGraphNodeOutputDesc aDesc[] = {
		{"scene", GXTEXTURE_TYPE_2D}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}
