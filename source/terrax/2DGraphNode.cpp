#include "2DGraphNode.h"
#include "2DGraphNodeData.h"
#include <xcommon/IPluginManager.h>
#include <xcommon/IXCamera.h>
#include <xcommon/IXRenderable.h>
#include "terrax.h"

C2DGraphNode::C2DGraphNode(IXCore *pCore):
	m_pCore(pCore)
{
	memset(m_pViewportCaptionRB, 0, sizeof(m_pViewportCaptionRB));
}
C2DGraphNode::~C2DGraphNode()
{
	mem_release(m_pDepthStencilStateNoZ);
	mem_release(m_pCameraConstantBuffer);

	for(UINT i = 0; i < 3; ++i)
	{
		mem_release(m_pViewportCaptionRB[i]);
	}

	mem_release(m_pViewportCaptionIB);

	mem_release(m_pTextColorCB);
	mem_release(m_pTextOffsetCB);

	mem_release(m_pFont);

	mem_release(m_pAxesRenderer);
}

void XMETHODCALLTYPE C2DGraphNode::startup(IXRender *pRender)
{
	IPluginManager *pPluginManager = m_pCore->getPluginManager();
	m_pMaterialSystem = (IXMaterialSystem*)pPluginManager->getInterface(IXMATERIALSYSTEM_GUID);

	m_pRenderPassGeometry2D = m_pMaterialSystem->registerRenderPass("xEditor2D", "terrax/geom2d.ps", NULL, NULL, NULL, NULL, true);
	m_pRenderPassGeometry3D = m_pMaterialSystem->getRenderPass("xEditor3DWhite");

	m_pRender = pRender;
	m_pDevice = pRender->getDevice();

	GXDepthStencilDesc dsDesc;
	dsDesc.cmpFuncDepth = GXCMP_GREATER_EQUAL;
	dsDesc.useDepthTest = FALSE;
	dsDesc.useDepthWrite = FALSE;
	m_pDepthStencilStateNoZ = m_pDevice->createDepthStencilState(&dsDesc);

	m_pCameraConstantBuffer = m_pDevice->createConstantBuffer(sizeof(SMMATRIX));

	m_idTextShader = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "gui_text.vs"), m_pRender->loadShader(SHADER_TYPE_PIXEL, "gui_main.ps"));

	m_pTextColorCB = m_pDevice->createConstantBuffer(sizeof(float4_t));
	m_pTextColorCB->update(&float4_t(198.0f / 255.0f, 240.0f / 255.0f, 253.0f / 255.0f, 1.0f));
	m_pTextOffsetCB = m_pDevice->createConstantBuffer(sizeof(float4_t));

	initViewportCaptions();
}

void XMETHODCALLTYPE C2DGraphNode::newData(IXRenderGraphNodeData **ppData)
{
	*ppData = new C2DGraphNodeData(m_pDevice, m_pRender);
}

void XMETHODCALLTYPE C2DGraphNode::newOutputTarget(UINT uOutput, UINT uWidth, UINT uHeight, IGXBaseTexture **ppTarget)
{
	*ppTarget = m_pDevice->createTexture2D(uWidth, uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A8B8G8R8);
}

void XMETHODCALLTYPE C2DGraphNode::updateVisibility(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData *pData)
{
	C2DGraphNodeData *pTarget = (C2DGraphNodeData*)pData;

	IXCamera *pCamera;
	pFinalTarget->getCamera(&pCamera);

	if(pCamera)
	{
		pCamera->updateVisibility();
	}
	mem_release(pCamera);
}

void XMETHODCALLTYPE C2DGraphNode::process(float fDeltaTime, IXRenderTarget *pFinalTarget, IGXBaseTexture **ppInputs, UINT uInputCount, IXRenderGraphNodeData *pData, IGXSurface **ppOutputs, UINT uOutputCount)
{
	assert(uInputCount == 0);
	assert(uOutputCount == 1);

	static const bool *terrax_detach_3d = m_pCore->getConsole()->getPCVarBool("terrax_detach_3d");
	if(*terrax_detach_3d)
	{
		return;
	}

	C2DGraphNodeData *pTarget = (C2DGraphNodeData*)pData;

	IGXContext *pCtx = m_pDevice->getThreadContext();
	pCtx->setColorTarget(ppOutputs[0]);

	pCtx->setDepthStencilSurface(pTarget->m_pDepthStencilSurface);

	pCtx->clear(GX_CLEAR_COLOR | GX_CLEAR_DEPTH | GX_CLEAR_STENCIL);

	m_pMaterialSystem->setFillMode(GXFILL_WIREFRAME);
	//			pDXDevice->setRasterizerState(g_xRenderStates.pRSWireframe);
	pCtx->setDepthStencilState(m_pDepthStencilStateNoZ);
	pCtx->setBlendState(NULL);
	
#ifndef __TRANSIENT
	m_pCameraConstantBuffer->update(&SMMatrixIdentity());
	pCtx->setVSConstant(m_pCameraConstantBuffer, SCR_OBJECT);
#endif

	IXCamera *pCamera;
	pFinalTarget->getCamera(&pCamera);

	m_pMaterialSystem->bindRenderPass(m_pRenderPassGeometry2D);

	int iFound = -1;
	for(int i = 0; i < 4; ++i)
	{
		if(g_xConfig.m_pViewportCamera[i] == pCamera)
		{
			iFound = i;
			break;
		}
	}

	assert(iFound >= 0);

	X_2D_VIEW x2dView = g_xConfig.m_x2DView[iFound];

	XRender2D(pCamera, x2dView, pCamera->getScale(), true, false);

	IXRenderableVisibility *pVis;
	pCamera->getVisibility(&pVis);
	m_pRender->renderStage(XRS_EDITOR_2D, pVis);
	mem_release(pVis);

#ifndef __TRANSIENT
	pCtx->setVSConstant(m_pCameraConstantBuffer, SCR_OBJECT);
#endif
	XRender2D(pCamera, x2dView, pCamera->getScale(), false, false);

	m_pMaterialSystem->bindRenderPass(m_pRenderPassGeometry3D);
	XRender2D(pCamera, x2dView, pCamera->getScale(), false, true);

	g_pEditor->render(false);
	m_pAxesRenderer->render(true);

	drawViewportCaption(x2dView);

	mem_release(pCamera);
}

void C2DGraphNode::drawViewportCaption(X_2D_VIEW x2dView)
{
	if(!m_pViewportCaptionRB[x2dView])
	{
		return;
	}

	IGXContext *pCtx = m_pDevice->getThreadContext();

	pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);
	pCtx->setRasterizerState(NULL);
	IGXDepthStencilSurface *pOldDS = pCtx->getDepthStencilSurface();
	pCtx->setDepthStencilSurface(NULL);
	pCtx->setBlendState(g_xRenderStates.pBlendAlpha);

	IGXSurface *pTarget = pCtx->getColorTarget();

	float fWidth = (float)pTarget->getWidth();
	float fHeight = (float)pTarget->getHeight();
	mem_release(pTarget);

	SMMATRIX mProj = SMMatrixTranslation(-0.5f, -0.5f, 0.0f) * SMMatrixOrthographicLH(fWidth, fHeight, 1.0f, 2000.0f);
	SMMATRIX mView = SMMatrixLookToLH(float3(fWidth * 0.5f, -fHeight * 0.5f, -1.0f), float3(0.0f, 0.0f, 1.0f), float3(0.0f, 1.0f, 0.0f));

	m_pCameraConstantBuffer->update(&SMMatrixTranspose(mView * mProj));

	pCtx->setVSConstant(m_pCameraConstantBuffer, SCR_CAMERA);

	IGXTexture2D *pTex;
	m_pFont->getTexture(0, &pTex);
	pCtx->setPSTexture(pTex);
	mem_release(pTex);
	pCtx->setRenderBuffer(m_pViewportCaptionRB[x2dView]);
	pCtx->setIndexBuffer(m_pViewportCaptionIB);
	pCtx->setPSConstant(m_pTextColorCB);
	m_pTextOffsetCB->update(&float4_t(0.0f, 0.0f, 0.0f, 0.0f));
	pCtx->setVSConstant(m_pTextOffsetCB, 6);
	m_pRender->bindShader(pCtx, m_idTextShader);
	pCtx->drawIndexed(m_uViewportCaptionQuadCount[x2dView] * 4, m_uViewportCaptionQuadCount[x2dView] * 2);
	m_pRender->unbindShader(pCtx);

	pCtx->setDepthStencilSurface(pOldDS);
	mem_release(pOldDS);
}

void C2DGraphNode::initViewportCaptions()
{
	m_pFontManager = (IXFontManager*)m_pCore->getPluginManager()->getInterface(IXFONTMANAGER_GUID);
	if(!m_pFontManager)
	{
		return;
	}
	IGXVertexDeclaration *pVD = NULL;
	m_pFontManager->getFont(&m_pFont, "gui/fonts/tahoma.ttf", 10);
	m_pFontManager->getFontVertexDeclaration(&pVD);

	XFontBuildParams xfbp;
	XFontStringMetrics xfsm = {};

	// X_2D_VIEW

	const char *aszCaptions[] = {
		"Top (x/z)",
		"Front (x/y)",
		"Side (z/y)"
	};

	UINT uMaxLen = 0;

	for(UINT i = 0, l = ARRAYSIZE(aszCaptions); i < l; ++i)
	{
		xfbp.pVertices = NULL;
		m_pFont->buildString(aszCaptions[i], xfbp, &xfsm);
		xfbp.pVertices = (XFontVertex*)alloca(sizeof(XFontVertex) * xfsm.uVertexCount);
		m_pFont->buildString(aszCaptions[i], xfbp, &xfsm);

		XFontGPUVertex *pBuffer = (XFontGPUVertex*)alloca(sizeof(XFontGPUVertex) * xfsm.uVertexCount);

		for(UINT j = 0; j < xfsm.uVertexCount; ++j)
		{
			pBuffer[j] = xfbp.pVertices[j];
			pBuffer[j].vPos.y *= -1.0f;
		}

		m_uViewportCaptionQuadCount[i] = xfsm.uVertexCount / 4;

		IGXVertexBuffer *pVB = m_pDevice->createVertexBuffer(sizeof(XFontGPUVertex) * xfsm.uVertexCount, GXBUFFER_USAGE_STATIC, pBuffer);

		m_pViewportCaptionRB[i] = m_pDevice->createRenderBuffer(1, &pVB, pVD);

		mem_release(pVB);

		UINT uLen = (UINT)strlen(aszCaptions[i]);
		if(uLen > uMaxLen)
		{
			uMaxLen = uLen;
		}
	}

	m_pFontManager->getFontIndexBuffer(uMaxLen, &m_pViewportCaptionIB);
}

void C2DGraphNode::setAxesRenderer(IXGizmoRenderer *pRenderer)
{
	mem_release(m_pAxesRenderer);
	m_pAxesRenderer = pRenderer;
	add_ref(m_pAxesRenderer);
}

const XRenderGraphNodeOutputDesc* XMETHODCALLTYPE C2DGraphNode::getOutputsDesc(UINT *puCount)
{
	static XRenderGraphNodeOutputDesc aDesc[] = {
		{"scene", GXTEXTURE_TYPE_2D}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}