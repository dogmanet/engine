#ifndef __2DPELINENODE_H
#define __2DPELINENODE_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCore.h>
#include <xcommon/editor/IXEditor.h>
#include <xcommon/gui/IXFontManager.h>
#include <mtrl/IXMaterialSystem.h>

class C2DGraphNodeData;
class C2DGraphNode final: public IXUnknownImplementation<IXRenderGraphNode>
{
public:
	C2DGraphNode(IXCore *pCore);
	~C2DGraphNode();

	XIMPLEMENT_VERSION(IXRENDERGRAPHNODE_VERSION);

	const char* XMETHODCALLTYPE getName() const override
	{
		return("x2D");
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

	void setAxesRenderer(IXGizmoRenderer *pRenderer);

private:
	void drawViewportCaption(X_2D_VIEW x2dView);
	void initViewportCaptions();

private:
	IXCore *m_pCore = NULL;
	IXRender *m_pRender = NULL;
	IGXDevice *m_pDevice = NULL;
	IXMaterialSystem *m_pMaterialSystem = NULL;
	IXGizmoRenderer *m_pAxesRenderer = NULL;

	XRenderPassHandler *m_pRenderPassGeometry2D = NULL;
	XRenderPassHandler *m_pRenderPassGeometry3D = NULL;

	IGXDepthStencilState *m_pDepthStencilStateNoZ = NULL;

	IGXConstantBuffer *m_pCameraConstantBuffer = NULL;

	IXFontManager *m_pFontManager = NULL;
	IXFont *m_pFont = NULL;

	ID m_idTextShader = -1;
	IGXRenderBuffer *m_pViewportCaptionRB[3];
	UINT m_uViewportCaptionQuadCount[3];
	IGXIndexBuffer *m_pViewportCaptionIB = NULL;
	IGXConstantBuffer *m_pTextColorCB = NULL;
	IGXConstantBuffer *m_pTextOffsetCB = NULL;
};

#endif
