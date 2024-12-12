#ifndef __CLIP_TOOL_H
#define __CLIP_TOOL_H

#include <xcommon/editor/IXEditable.h>
#include <mtrl/IXMaterialSystem.h>
#include <xcommon/IXCore.h>
//#include "Outline.h"
//#include "BrushMesh.h"
//#include "FaceEdit.h"

class CEditable;
class CClipTool;
//class CEditorObject;

class CGizmoCallback: public IXEditorGizmoHandleCallback
{
public:
	CGizmoCallback(CClipTool *pTool):
		m_pTool(pTool)
	{
	}
	void XMETHODCALLTYPE moveTo(const float3 &vNewPos, IXEditorGizmoHandle *pGizmo) override;
	void XMETHODCALLTYPE onStart(IXEditorGizmoHandle *pGizmo) override
	{
	}
	void XMETHODCALLTYPE onEnd(IXEditorGizmoHandle *pGizmo) override
	{
	}

private:
	CClipTool *m_pTool;
};

//##########################################################################

class CClipTool final: public IXUnknownImplementation<IXEditorTool>
{
public:
	CClipTool(CEditable *pEditable, IXEditor *pEditor);
	~CClipTool();

	void* XMETHODCALLTYPE getIcon() override;
	const char* XMETHODCALLTYPE getTitle() override;
	XAccelItem XMETHODCALLTYPE getAccel() override;

	void XMETHODCALLTYPE activate() override;
	void XMETHODCALLTYPE deactivate() override;

	bool XMETHODCALLTYPE onMouseDown(bool isPrimary) override;
	bool XMETHODCALLTYPE onMouseMove() override;
	void XMETHODCALLTYPE onMouseUp(bool isPrimary) override;

	bool XMETHODCALLTYPE onKeyDown(UINT uKey) override;
	void XMETHODCALLTYPE onKeyUp(UINT uKey) override;

	void render(bool is3D);

	bool XMETHODCALLTYPE wantDrawSelection(bool is3D) override
	{
		return(false);
	}

	void XMETHODCALLTYPE onNextMode() override;

	bool XMETHODCALLTYPE wantUseClassSelector() override
	{
		return(false);
	}
	bool XMETHODCALLTYPE isRandomScaleOrYawSupported() override
	{
		return(false);
	}
	UINT XMETHODCALLTYPE getClassCount() override
	{
		return(0);
	}
	const char* XMETHODCALLTYPE getClassAt(UINT idx) override
	{
		return(NULL);
	}
	bool XMETHODCALLTYPE useClass(const char *szClassName) override
	{
		return(false);
	}

	bool XMETHODCALLTYPE allowUseCamera() override
	{
		return(false);
	}

	void onPosChanged();

	SX_ALIGNED_OP_MEM();

private:
	CEditable *m_pEditable;
	IXEditor *m_pEditor;

	//CFaceEdit *m_pFaceEdit;

	HBITMAP m_hBitmap;

	IXGizmoRenderer *m_pRenderer = NULL;

	bool m_isActive = false;

	IXEditorGizmoHandle *m_pGizmos[3];

	bool m_isInitiated = false;

	bool m_isMouseDown = false;

	bool m_isValid = false;

	bool m_isDirty = false;

	SMPLANE m_clipPlane;

	CGizmoCallback m_gizmoCallback;

	int m_iMode = -1;
};

#endif
