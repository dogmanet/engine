#ifndef __SCENEPELINENODE_H
#define __SCENEPELINENODE_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCore.h>
#include <mtrl/IXMaterialSystem.h>

class CGBufferGraphNodeData;
class CGBufferGraphNode final: public IXUnknownImplementation<IXRenderGraphNode>
{
public:
	CGBufferGraphNode(IXCore *pCore);
	~CGBufferGraphNode();

	XIMPLEMENT_VERSION(IXRENDERGRAPHNODE_VERSION);

	const char* XMETHODCALLTYPE getName() const override
	{
		return("xGBuffer");
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
		
	const XRenderGraphNodeInputDesc* XMETHODCALLTYPE getInputsDesc(UINT *puCount) override
	{
		*puCount = 0;
		return(NULL);
	}
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
	void renderGBuffer(IXCamera *pCamera, IGXSurface **ppOutputs);

	void showTexture(IGXTexture2D *pTexture, IXRenderTarget *pFinalTarget);

private:
	IXCore *m_pCore = NULL;
	IXRender *m_pRender = NULL;
	IGXDevice *m_pDevice = NULL;
	IXMaterialSystem *m_pMaterialSystem = NULL;

	XRenderPassHandler *m_pRenderPassGBuffer = NULL;

	IGXDepthStencilState *m_pDepthStencilStateDefault = NULL;
	IGXDepthStencilState *m_pDepthStencilStateNoZWrite = NULL;
	IGXDepthStencilState *m_pDepthStencilStateNoZ = NULL;

	ID m_idShaderScreenOut = -1;
};

#endif
