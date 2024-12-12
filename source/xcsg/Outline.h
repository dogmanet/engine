#ifndef __OUTLINE_H
#define __OUTLINE_H

#include <xcommon/editor/IXEditorGizmo.h>
#include <xcommon/editor/IXEditor.h>
#include <xcommon/render/IXRenderUtils.h>
//#include <xcommon/editor/IXEditable.h>
//#include <mtrl/IXMaterialSystem.h>
//#include <xcommon/IXCore.h>
#include <gdefines.h>

class COutline;
class COutlinePointCallback: public IXEditorGizmoHandleCallback
{
public:
	COutlinePointCallback(COutline *pOutline):
		m_pOutline(pOutline)
	{
	}

	void XMETHODCALLTYPE moveTo(const float3 &vNewPos, IXEditorGizmoHandle *pGizmo) override;
	void XMETHODCALLTYPE onStart(IXEditorGizmoHandle *pGizmo) override;
	void XMETHODCALLTYPE onEnd(IXEditorGizmoHandle *pGizmo) override;

private:
	COutline *m_pOutline;
	bool m_isMoved = false;
	bool m_isFirstMov = false;
};

//##########################################################################

class CContour
{
public:
	UINT getPointCount() const
	{
		return(m_aPoints.size());
	}
	UINT getPoint(UINT uIdx) const
	{
		return(m_aPoints[uIdx % m_aPoints.size()]);
	}
	//bool isSegmentInternal(UINT uIdx) const;

	void setPoints(Array<UINT> &pts)
	{
		m_aPoints.swap(pts);
	}

private:
	Array<UINT> m_aPoints;
};

//##########################################################################

class COutline
{
public:
	COutline(IXEditor *pEditor, IXGizmoRenderer *pRenderer);
	~COutline();

	void addPoint();

	void setMouse(const float3_t &vPoint);

	void setPlane(const float3_t &vNormal);

	void render(bool is3D);

	void activate();
	void deactivate();

	void onHandleClick(IXEditorGizmoHandle *pHandle);
	void onHandleMove(const float3 &vNewPos, IXEditorGizmoHandle *pHandle);

	void closePath();

	void deleteSelected();

	bool isClosed()
	{
		return(m_isClosed);
	}

	bool isValid();

	UINT getContourCount() const;
	const CContour& getContour(UINT idx) const;

	UINT getPointCount() const;
	const float3_t& getPoint(UINT idx) const;

	const float3_t& getNormal() const;

	void buildContours();

private:	
	void buildContoursRecursive(const Array<UINT> &aIn, UINT uCurDepth = 0);

	void fixVertexOrder();

	//bool canUsePoint(const Array<UINT> &aIn, UINT uPoint, UINT uPrevPoint0, UINT uPrevPoint1);
	bool checkPoint(const Array<UINT> &aIn, UINT uPoint, UINT uNextPoint);

private:
	
	struct PathPoint
	{
		float3_t vPos;
		bool isSelected;
		IXEditorGizmoHandle *pHandle;
	};

	IXEditor *m_pEditor = NULL;
	IXGizmoRenderer *m_pRenderer = NULL;

	Array<PathPoint> m_aPoints;

	float3_t m_vNormal;

	float3_t m_vNewPoint;

	bool m_isClosed = false;
	bool m_isDirty = false;
	bool m_isContoursFailed = false;

	COutlinePointCallback m_callBack;

	Array<CContour> m_aContours;
};

#endif
