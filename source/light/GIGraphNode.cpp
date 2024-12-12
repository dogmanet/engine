#include "GIGraphNode.h"
#include "GIGraphNodeData.h"
#include <xcommon/IXCamera.h>
#include <xcommon/IXRenderable.h>
#include <game/sxgame.h>
#include <xcommon/IPluginManager.h>

CGIGraphNode::CGIGraphNode(IXCore *pCore, CLightSystem *pLightSystem):
	m_pCore(pCore),
	m_pLightSystem(pLightSystem)
{
	pCore->getConsole()->registerCVar("dev_lpv_cubes", false, "Отображать сетку LPV");
	pCore->getConsole()->registerCVar("dev_lpv_points", false, "Отображать VPL при инъекции в LPV");
}
CGIGraphNode::~CGIGraphNode()
{
}


void XMETHODCALLTYPE CGIGraphNode::startup(IXRender *pRender)
{
	m_pRender = pRender;
	m_pDevice = pRender->getDevice();
}

void XMETHODCALLTYPE CGIGraphNode::newData(IXRenderGraphNodeData **ppData)
{
	*ppData = new CGIGraphNodeData(m_pDevice, m_pRender);
}

void XMETHODCALLTYPE CGIGraphNode::newOutputTarget(UINT uOutput, UINT uWidth, UINT uHeight, IGXBaseTexture **ppTarget)
{
	*ppTarget = m_pDevice->createTexture2D(uWidth, uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A16B16G16R16F);
}

void XMETHODCALLTYPE CGIGraphNode::updateVisibility(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData *pData)
{
	CGIGraphNodeData *pTarget = (CGIGraphNodeData*)pData;

	IXCamera *pCamera;
	pFinalTarget->getCamera(&pCamera);
	m_pLightSystem->setFrameObserverCamera(pCamera);
	m_pLightSystem->updateVisibility(pCamera);
	mem_release(pCamera);
}

void XMETHODCALLTYPE CGIGraphNode::process(float fDeltaTime, IXRenderTarget *pFinalTarget, IGXBaseTexture **ppInputs, UINT uInputCount, IXRenderGraphNodeData *pData, IGXSurface **ppOutputs, UINT uOutputCount)
{
	XPROFILE_FUNCTION();

	assert(uInputCount == 4);
	assert(uOutputCount == 1);

	CGIGraphNodeData *pTarget = (CGIGraphNodeData*)pData;

	IGXContext *pCtx = m_pDevice->getThreadContext();

	pCtx->setColorTarget(ppOutputs[0]);
	
	if(m_pLightSystem)
	{
		IXCamera *pCamera;
		pFinalTarget->getCamera(&pCamera);
		m_pLightSystem->setFrameObserverCamera(pCamera);
		mem_release(pCamera);

		pTarget->setGBuffer(ppInputs[0], ppInputs[1], ppInputs[2], ppInputs[3]);

		m_pLightSystem->renderGI(pTarget, pFinalTarget);

#ifdef __TRANSIENT
		if(*r_final_image == DS_RT_AMBIENTDIFF)
		{
			//pCtx->setColorTarget(pBackBuf);
			showTexture(m_pLightTotal); // -> pBackBuf
			goto end;
		}
#endif
		pCtx->addTimestamp("gi");
	}
}

const XRenderGraphNodeInputDesc* XMETHODCALLTYPE CGIGraphNode::getInputsDesc(UINT *puCount)
{
	static XRenderGraphNodeInputDesc aDesc[] = {
		{"color", GXTEXTURE_TYPE_2D},
		{"normals", GXTEXTURE_TYPE_2D},
		{"params", GXTEXTURE_TYPE_2D},
		{"depth", GXTEXTURE_TYPE_2D}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}

const XRenderGraphNodeOutputDesc* XMETHODCALLTYPE CGIGraphNode::getOutputsDesc(UINT *puCount)
{
	static XRenderGraphNodeOutputDesc aDesc[] = {
		{"litScene", GXTEXTURE_TYPE_2D}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}
