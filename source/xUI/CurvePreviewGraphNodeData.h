#ifndef __CURVE_PREVIEW_GRAPH_NODE_DATA_H
#define __CURVE_PREVIEW_GRAPH_NODE_DATA_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCamera.h>
#include <xcommon/IXCore.h>
#include <common/MinMaxCurve.h>

class CRender;
class CCurvePreviewGraphNodeData final: public IXUnknownImplementation<IXRenderGraphNodeData>
{
	friend class CCurvePreviewGraphNode;
public:
	CCurvePreviewGraphNodeData(IGXDevice *pDevice, IXRender *pRender, IXCore *pCore);
	~CCurvePreviewGraphNodeData();
	
	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;

	void updateLine(IXMinMaxCurve *pCurve);

private:
	UINT m_uWidth = 0;
	UINT m_uHeight = 0;

	IGXDevice *m_pDevice = NULL;
	IXGizmoRenderer *m_pLineRenderer = NULL;

	IGXSurface *m_pSurface = NULL;

	IXFont *m_pFont = NULL;

	float2_t m_vAreaMin;
	float2_t m_vAreaMax;

	bool m_isDirty = false;
	IXMinMaxCurve *m_pCurve = NULL;

private:
	void updateLine(int iCurve, IXAnimationCurve *pCurve);
	void drawArea(IXMinMaxCurve *pCurve);
};

#endif
