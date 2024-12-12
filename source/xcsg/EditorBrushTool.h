#ifndef __EDITOR_BRUSH_TOOL_H
#define __EDITOR_BRUSH_TOOL_H

#include <xcommon/editor/IXEditable.h>
#include <mtrl/IXMaterialSystem.h>
#include <xcommon/IXCore.h>
#include "Outline.h"
#include "BrushMesh.h"
#include "IBrushCreator.h"

class CEditable;
//class CEditorObject;

class CEditorBrushTool final: public IXUnknownImplementation<IXEditorTool>
{
public:
	CEditorBrushTool(CEditable *pEditable, IXEditor *pEditor);
	~CEditorBrushTool();

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

	void XMETHODCALLTYPE onNextMode() override
	{
	}

	bool XMETHODCALLTYPE wantUseClassSelector() override
	{
		return(true);
	}
	bool XMETHODCALLTYPE isRandomScaleOrYawSupported() override
	{
		return(false);
	}
	UINT XMETHODCALLTYPE getClassCount() override;
	const char* XMETHODCALLTYPE getClassAt(UINT idx) override;
	bool XMETHODCALLTYPE useClass(const char *szClassName) override;

	bool XMETHODCALLTYPE allowUseCamera() override
	{
		return(false);
	}

	void registerCreator(IBrushCreator *pCreator);

private:
	CEditable *m_pEditable;
	IXEditor *m_pEditor;

	HBITMAP m_hBitmap;

	IXGizmoRenderer *m_pRenderer = NULL;

	bool m_isActive = false;

	IBrushCreator *m_pCreator = NULL;

	struct Creator
	{
		IBrushCreator *pCreator;
		const char *szClassName;
	};
	Array<Creator> m_aCreators;
};

#endif
