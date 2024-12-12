#ifndef __FACE_EDIT_TOOL_H
#define __FACE_EDIT_TOOL_H

#include <xcommon/editor/IXEditable.h>
#include <mtrl/IXMaterialSystem.h>
#include <xcommon/IXCore.h>
//#include "Outline.h"
//#include "BrushMesh.h"
#include "FaceEdit.h"

class CEditable;
//class CEditorObject;

class CFaceEditTool final: public IXUnknownImplementation<IXEditorTool>
{
public:
	CFaceEditTool(CEditable *pEditable, IXEditor *pEditor);
	~CFaceEditTool();

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
		return(is3D);
	}

	void XMETHODCALLTYPE onNextMode() override
	{
	}

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
		return(true);
	}

private:
	CEditable *m_pEditable;
	IXEditor *m_pEditor;

	CFaceEdit *m_pFaceEdit;

	HBITMAP m_hBitmap;

	IXGizmoRenderer *m_pRenderer = NULL;

	bool m_isActive = false;

	//bool m_isMouseDown = false;
};

#endif
