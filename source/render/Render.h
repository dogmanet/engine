#ifndef __RENDER_H
#define __RENDER_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCore.h>
#include <common/AAString.h>
#include "FinalTarget.h"
#include "TextureTarget.h"
#include <mtrl/IXMaterialSystem.h>
#include <xcommon/IXRenderable.h>
#include "ShaderManager.h"

class CGXLogger: public IGXLogger
{
public:
	void logInfo(const char *szString) override
	{
		LibReport(REPORT_MSG_LEVEL_NOTICE, "%s", szString);
	}
	void logWarning(const char *szString) override
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "%s", szString);
	}
	void logError(const char *szString) override
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "%s", szString);
	}
};

//#############################################################################

class CRender final: public IXUnknownImplementation<IXRender>
{
public:
	CRender(IXCore *pCore);
	~CRender();

	SX_ALIGNED_OP_MEM();

	bool XMETHODCALLTYPE newFinalTarget(SXWINDOW hWnd, const char *szName, IXRenderTarget **ppOut) override;

	bool XMETHODCALLTYPE newTextureTarget(IXRenderTarget **ppOut, char *szTextureName = NULL, size_t sizeTextureNameBuffer = 0) override;

	bool XMETHODCALLTYPE getFinalTarget(const char *szName, IXRenderTarget **ppOut) override;

	bool XMETHODCALLTYPE getGraphNode(const char *szName, IXRenderGraphNode **ppOut) override;

	bool XMETHODCALLTYPE getGraph(const char *szName, IXRenderGraph **ppOut) override;

	void onFinalTargetReleased(CFinalTarget *pTarget);
	void onTextureTargetReleased(CTextureTarget *pTarget);
	void onGraphDestroyed(CRenderGraph *pGraph);

	void XMETHODCALLTYPE updateVisibility() override;

	void XMETHODCALLTYPE renderFrame(float fDeltaTime) override;
	void XMETHODCALLTYPE endFrame() override;

	void XMETHODCALLTYPE newVisData(IXRenderableVisibility **ppVisibility) override;

	IGXDevice* XMETHODCALLTYPE getDevice() override;

	void XMETHODCALLTYPE renderStage(X_RENDER_STAGE stage, IXRenderableVisibility *pVisibility = NULL) override;

	void XMETHODCALLTYPE setSceneFilter(IGXContext *pCtx, UINT uSlot = 0) override;

	//void XMETHODCALLTYPE renderPrepare() override;
	////void XMETHODCALLTYPE renderGBuffer() override;
	//void XMETHODCALLTYPE renderShadows() override;

	void XMETHODCALLTYPE drawScreenQuad(IGXContext *pCtx, IXRenderTarget *pFinalTarget = NULL) override;

	void XMETHODCALLTYPE bindShader(IGXContext *pCtx, ID idShaderKit) override;
	void XMETHODCALLTYPE unbindShader(IGXContext *pCtx) override;

	ID XMETHODCALLTYPE loadShader(SHADER_TYPE type_shader, const char *szPath, GXMacro *pMacro = NULL) override;
	ID XMETHODCALLTYPE createShaderKit(ID idVertexShader, ID idPixelShader, ID idGeometryShader = -1, ID idComputeShader = -1) override;

	IGXVertexDeclaration* getScreenQuadVertexDeclaration();

	void XMETHODCALLTYPE newCamera(IXCamera **ppCamera) override;
	void XMETHODCALLTYPE newFrustum(IXFrustum **ppFrustum) override;

	UINT XMETHODCALLTYPE getTransparentObjectsCount(IXRenderableVisibility *pVisibility) override;
	void XMETHODCALLTYPE getTransparentObjects(IXRenderableVisibility *pVisibility, XRenderTransparentObject *pObjects) override;

	void XMETHODCALLTYPE renderTransparentObject(IXRenderableVisibility *pVisibility, XRenderTransparentObject *pObject, UINT uSplitPlanes) override;

	IXRenderUtils* XMETHODCALLTYPE getUtils() override;

	const GXModeDesc* XMETHODCALLTYPE getModes(UINT *puCount, IXRenderTarget *pTarget = NULL) override;

	void XMETHODCALLTYPE initGraphics(void *pReserved = NULL) override;

	void update(float fDelta);

private:
	struct GraphNode
	{
		IXRenderGraphNode *pNode;
		Array<IXRenderGraphNode*> aPreprocessors;
		Array<IXRenderGraphNode*> aPostprocessors;
		Array<GraphNode*> aDependencies;
		bool isReady = true;

		Array<GraphNode*> aProcessOrder;
	};

	struct NodeTarget
	{
		IXRenderGraphNodeData *pNodeTarget;
		GraphNode *pPlNode;
	};

	struct FinalTarget
	{
		CFinalTarget *pFinalTarget;
	};

	struct RenderSystem
	{
		IXRenderable *pRenderable;
		UINT uPriority;
		UINT uRenderableId;
	};

	struct RenderStage
	{
		Array<RenderSystem> aSystems; // sorted list of render systems
	};

private:
	void setupNodes();
	void setupPasses();
	void setupGraphs();

	bool loadGraph(const char *szName, IXRenderGraph **ppOut);

	UINT getIndexForStage(X_RENDER_STAGE stage);

	void updateMatrices(IXCamera *pCamera, UINT uTargetWidth, UINT uTargetHeight);

private:
	IGXDevice *m_pDevice = NULL;
	IXCore *m_pCore = NULL;
	IXMaterialSystem *m_pMaterialSystem = NULL;

	IGXSamplerState *m_pSamplerScene = NULL;

	XRenderPassHandler *m_pRenderPassGBuffer = NULL;
	XRenderPassHandler *m_pRenderPassTransparency = NULL;
	XRenderPassHandler *m_pRenderPassIllumination = NULL;
	XRenderPassHandler *m_pRenderPassPostprocess = NULL;
	XRenderPassHandler *m_pRenderPassShadow = NULL;

	Array<RenderStage> m_apRenderStages;
	Array<IXRenderable*> m_apRenderables;

	Map<AAString, FinalTarget> m_mapFinalTargets;
	Array<FinalTarget*> m_aFinalTargets;
	Array<CTextureTarget*> m_aTextureTargets;

	Map<AAString, GraphNode> m_mapRenderNodes;

	Map<AAString, CRenderGraph*> m_mapRenderGraphs;

	IGXVertexDeclaration *m_pScreenVertexDeclaration = NULL;

	IXRenderUtils *m_pRenderUtils = NULL;

	struct
	{
		float fFrameTime;
		float _padding[3];
	} m_frameShaderData;
	IGXConstantBuffer *m_pFrameShaderData = NULL;

	struct
	{
		SMMATRIX mVP;
		float3 vPosCam;
		SMMATRIX mInvVP;
		SMMATRIX mInvV;
		float4 vNearFarInvWinSize;
		float3 vParamProj;
	} m_cameraShaderData;
	IGXConstantBuffer *m_pCameraShaderData = NULL;

	CBaseTarget *m_pCurrentRenderTarget = NULL;

	CShaderManager *m_pShaderManager = NULL;

	CGXLogger m_gxLogger;
};

#endif
