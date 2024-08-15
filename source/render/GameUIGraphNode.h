#ifndef __GAMEUIGRAPHNODE_H
#define __GAMEUIGRAPHNODE_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCore.h>

class CGameUIGraphNodeData;
class CGameUIGraphNode final: public IXUnknownImplementation<IXRenderGraphNode>
{
public:
	CGameUIGraphNode(IXCore *pCore);
	~CGameUIGraphNode();

	XIMPLEMENT_VERSION(IXRENDERGRAPHNODE_VERSION);

	const char* XMETHODCALLTYPE getName() const override
	{
		return("xGameUI");
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
	IXCore *m_pCore = NULL;
	IXRender *m_pRender = NULL;
	IGXDevice *m_pDevice = NULL;
};

#endif
