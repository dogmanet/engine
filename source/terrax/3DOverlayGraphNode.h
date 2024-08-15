#ifndef __3DOVERLAYGRAPHNODE_H
#define __3DOVERLAYGRAPHNODE_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCore.h>
#include <mtrl/IXMaterialSystem.h>

class C3DOverlayGraphNodeData;
class C3DOverlayGraphNode final: public IXUnknownImplementation<IXRenderGraphNode>
{
public:
	C3DOverlayGraphNode(IXCore *pCore);
	~C3DOverlayGraphNode();

	XIMPLEMENT_VERSION(IXRENDERGRAPHNODE_VERSION);

	const char* XMETHODCALLTYPE getName() const override
	{
		return("x3DOverlay");
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

	void setAxesRenderer(IXGizmoRenderer *pRenderer);

private:
	IXCore *m_pCore = NULL;
	IXRender *m_pRender = NULL;
	IGXDevice *m_pDevice = NULL;
	IXGizmoRenderer *m_pAxesRenderer = NULL;

	IGXDepthStencilState *m_pDSDefault = NULL;
	XRenderPassHandler *m_pRenderPassGeometry3D = NULL;
	IXMaterialSystem *m_pMaterialSystem = NULL;
};

#endif
