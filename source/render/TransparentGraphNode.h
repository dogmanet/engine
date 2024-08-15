#ifndef __TRANSPARENTGRAPHNODE_H
#define __TRANSPARENTGRAPHNODE_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCore.h>
#include <mtrl/IXMaterialSystem.h>

#define MAX_TRANSPARENCY_CLIP_PANES 4

class CTransparentGraphNodeData;
class CTransparentGraphNode final: public IXUnknownImplementation<IXRenderGraphNode>
{
public:
	CTransparentGraphNode(IXCore *pCore);
	~CTransparentGraphNode();

	XIMPLEMENT_VERSION(IXRENDERGRAPHNODE_VERSION);

	const char* XMETHODCALLTYPE getName() const override
	{
		return("xTransparent");
	}

	bool XMETHODCALLTYPE isExtension() const override
	{
		return(false);
	}
	XRenderPipelineExtensionDesc* XMETHODCALLTYPE getExtensionDesc() override
	{
		return(NULL);
	}

	void XMETHODCALLTYPE startup(IXRender *pRender) override;

	void XMETHODCALLTYPE newData(IXRenderGraphNodeData **ppData) override;

	void XMETHODCALLTYPE newOutputTarget(UINT uOutput, UINT uWidth, UINT uHeight, IGXBaseTexture **ppTarget) override;

	void XMETHODCALLTYPE updateVisibility(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData *pData) override;

	void XMETHODCALLTYPE process(float fDeltaTime, IXRenderTarget *pFinalTarget, IGXBaseTexture **ppInputs, UINT uInputCount, IXRenderGraphNodeData *pData, IGXSurface **ppOutputs, UINT uOutputCount) override;

	const XRenderGraphNodeInputDesc* XMETHODCALLTYPE getInputsDesc(UINT *puCount) override;
	const XRenderGraphNodeOutputDesc* XMETHODCALLTYPE getOutputsDesc(UINT *puCount) override;

	const XRenderGraphNodeTextureDesc* XMETHODCALLTYPE getTexturesDesc(UINT *puCount)
	{
		*puCount = 0;
		return(NULL);
	}
	const XRenderGraphNodePropDesc* XMETHODCALLTYPE getPropsDesc(UINT *puCount)
	{
		*puCount = 0;
		return(NULL);
	}
	const XRenderGraphNodeFlagDesc* XMETHODCALLTYPE getFlagsDesc(UINT *puCount) override
	{
		*puCount = 0;
		return(NULL);
	}

	void XMETHODCALLTYPE setTexture(IXRenderGraphNodeData *pNodeData, UINT uIdx, IXTexture *pTexture) override
	{
	}
	void XMETHODCALLTYPE setProp(IXRenderGraphNodeData *pNodeData, UINT uIdx, const float4_t &vProp) override
	{
	}
	void XMETHODCALLTYPE setFlag(IXRenderGraphNodeData *pNodeData, UINT uIdx, bool bVal) override
	{
	}

private:
	struct XTransparentNodeSP
	{
		int iPSP = -1;
		XTransparentNodeSP *pNext = NULL;
		float fCamDist = -1.0f;
	};

	struct XTransparentNode
	{
		XRenderTransparentObject obj;
		// split plane list
		UINT uSplitPlanes = 0;
		XTransparentNodeSP *pSplitPlanes = NULL;
	};

	struct XTransparentPSP
	{
		SMPLANE psp;
		UINT uNode;
		bool useMe = false;
		int iBasePSP = -1; // номер похожей плоскости

		bool isRenderFront = false;
	};

	struct XTransparentBSPObject
	{
		XTransparentBSPObject *pNext = NULL;
		float fCamDist;
		UINT uNode;
	};
	struct XTransparentBSPNode
	{
		int iPSP = -1;
		XTransparentBSPObject *pObjects = NULL;
		XTransparentBSPNode *pBack = NULL;
		XTransparentBSPNode *pFront = NULL;
	};

private:
	void buildTransparencyBSP(XTransparentPSP *pPSPs, UINT uPSPcount, UINT uStartPSP, XTransparentBSPNode *pNode, XTransparentBSPObject *pObjects, XTransparentNode *pObjectNodes, const float3 &vCamPos);
	void renderTransparencyBSP(CTransparentGraphNodeData *pTarget, IXRenderTarget *pFinalTarget, IXRenderableVisibility *pVis, XTransparentBSPNode *pNode, XTransparentPSP *pPSPs, XTransparentNode *pObjectNodes, const float3 &vCamPos, UINT *puPlanesStack, UINT uPlaneCount, const SMMATRIX &mTransInvVP);

	void showTexture(IGXBaseTexture *pTexture, IXRenderTarget *pFinalTarget);

private:
	IXCore *m_pCore = NULL;
	IXRender *m_pRender = NULL;
	IGXDevice *m_pDevice = NULL;

	IXMaterialSystem *m_pMaterialSystem = NULL;
	XRenderPassHandler *m_pRenderPassTransparency = NULL;

	MemAlloc<XTransparentBSPNode> m_poolTransparencyBSPnodes;
	MemAlloc<XTransparentBSPObject> m_poolTransparencyBSPobjects;
	MemAlloc<XTransparentNodeSP> m_poolTransparencyBSPsplitPlanes;
	Array<XTransparentNode> m_aTmpNodes;
	Array<XTransparentPSP> m_aTmpPSPs;

	IGXConstantBuffer *m_pTransparencyShaderClipPlanes = NULL;
	IGXSamplerState *m_pRefractionScene = NULL;
	IGXSamplerState *m_pSamplerLinearClamp = NULL;
	IGXDepthStencilState *m_pDepthStencilStateNoZWrite = NULL;
	IGXDepthStencilState *m_pDepthStencilStateNoZ = NULL;
	IGXBlendState *m_pBlendStateAlpha = NULL;
	ID m_idShaderScreenOut = -1;
};

#endif
