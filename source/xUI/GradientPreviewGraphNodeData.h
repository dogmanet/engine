#ifndef __GRADIENT_PREVIEW_GRAPH_NODE_DATA_H
#define __GRADIENT_PREVIEW_GRAPH_NODE_DATA_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCamera.h>
#include <xcommon/IXCore.h>
#include <xcommon/util/IX2ColorGradients.h>

class CRender;
class CGradientPreviewGraphNodeData final: public IXUnknownImplementation<IXRenderGraphNodeData>
{
	friend class CGradientPreviewGraphNode;
public:
	CGradientPreviewGraphNodeData(IGXDevice *pDevice, IXRender *pRender, IXCore *pCore);
	~CGradientPreviewGraphNodeData();
	
	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;

	void update(IX2ColorGradients *pGradient);

private:
	UINT m_uWidth = 0;
	UINT m_uHeight = 0;

	IGXDevice *m_pDevice = NULL;
	IXGizmoRenderer *m_pRenderer = NULL;

	IGXSurface *m_pSurface = NULL;

	bool m_isDirty = false;

private:
	void drawArea(IXColorGradient *pGradient, UINT uStartY, UINT uHeight);
};

#endif
