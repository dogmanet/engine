#ifndef __CURVE_EDITOR_GRAPH_NODE_DATA_H
#define __CURVE_EDITOR_GRAPH_NODE_DATA_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCamera.h>
#include <xcommon/IXCore.h>
#include <common/MinMaxCurve.h>

class CRender;
class CCurveEditorGraphNodeData final: public IXUnknownImplementation<IXRenderGraphNodeData>
{
	friend class CCurveEditorGraphNode;
public:
	CCurveEditorGraphNodeData(IGXDevice *pDevice, IXRender *pRender, IXCore *pCore);
	~CCurveEditorGraphNodeData();
	
	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;

	void updateLine(IXMinMaxCurve *pCurve);
	void setHighlight(int iCurve, int iKeyFrame); // -1, 0, 1
	void setSelect(int iCurve, int iKeyFrame); // -1, 0, 1

	bool screenToGraph(int iX, int iY, float *pfX, float *pfY);
	void getScale(float *pfX, float *pfY);

	bool getSelectedPointTangents(float2_t *pvInPos, float2_t *pvOutPos);

	void calcNewTangent(int iX, int iY, bool isIn);

private:
	UINT m_uWidth = 0;
	UINT m_uHeight = 0;

	IGXDevice *m_pDevice = NULL;
	IXGizmoRenderer *m_pRenderer = NULL;
	IXGizmoRenderer *m_pLineRenderer = NULL;

	IGXSurface *m_pSurface = NULL;

	IXFont *m_pFont = NULL;

	float2_t m_vAreaMin;
	float2_t m_vAreaMax;

	int m_iHighlightCurve = -1;
	int m_iHighlightKey = -1;
	int m_iSelectedCurve = -1;
	int m_iSelectedKey = -1;
	bool m_isDirty = false;
	IXMinMaxCurve *m_pCurve = NULL;

private:
	void updateLine(int iCurve, IXAnimationCurve *pCurve);
	void drawArea(IXMinMaxCurve *pCurve);
};

#endif
