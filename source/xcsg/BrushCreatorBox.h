#ifndef __BRUSH_CREATOR_BOX_H
#define __BRUSH_CREATOR_BOX_H

#include <xcommon/editor/IXEditable.h>
#include <mtrl/IXMaterialSystem.h>
#include <xcommon/IXCore.h>
#include "Outline.h"
#include "BrushMesh.h"
#include "IBrushCreator.h"

class CEditable;
//class CEditorObject;

class CBrushCreatorBox;
class CBrushCreatorHandleCallback: public IXEditorGizmoHandleCallback
{
public:
	CBrushCreatorHandleCallback(CBrushCreatorBox *pBrushCreator):
		m_pBrushCreator(pBrushCreator)
	{
	}

	void XMETHODCALLTYPE moveTo(const float3 &vNewPos, IXEditorGizmoHandle *pGizmo) override;
	void XMETHODCALLTYPE onStart(IXEditorGizmoHandle *pGizmo) override;
	void XMETHODCALLTYPE onEnd(IXEditorGizmoHandle *pGizmo) override;

private:
	CBrushCreatorBox *m_pBrushCreator;
	//bool m_isMoved = false;
	//bool m_isFirstMov = false;
};

//##########################################################################

class CBrushCreatorBox final: public IXUnknownImplementation<IBrushCreator>
{
public:
	CBrushCreatorBox(CEditable *pEditable, IXEditor *pEditor, IXGizmoRenderer *pRenderer);
	~CBrushCreatorBox();

	bool isInitiated() override;
	void init(const float3 &vNormal) override;

	void activate() override;
	void deactivate() override;

	bool onMouseDown(bool isPrimary) override;
	bool onMouseMove() override;
	void onMouseUp(bool isPrimary) override;

	bool onKeyDown(UINT uKey) override;
	void onKeyUp(UINT uKey) override;

	void render(bool is3D);

	UINT getClassCount() override;
	const char* getClassAt(UINT idx) override;
	bool useClass(const char *szClassName) override;

	bool isValid();

	void onHandleMove(const float3 &vNewPos, IXEditorGizmoHandle *pHandle);

	SX_ALIGNED_OP_MEM();

private:
	void placeHandlers();

private:
	CEditable *m_pEditable;
	IXEditor *m_pEditor;

	IXGizmoRenderer *m_pRenderer = NULL;

	float3 m_vUp;
	float3 m_vPos0;
	float3 m_vPos1;
	float m_fHeight = 0.2f;

	bool m_isMouseDown = false;

	bool m_isInitiated = false;
	bool m_isBoxBuilt = false;

	bool m_isBoxMode = true;

	IXEditorGizmoHandle *m_apHandlers[6];
	CBrushCreatorHandleCallback m_handlerCallback;
};

#endif
