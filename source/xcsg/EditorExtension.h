#ifndef __EDITOR_EXTENSION_H
#define __EDITOR_EXTENSION_H

#include <xcommon/editor/IXEditable.h>
#include <mtrl/IXMaterialSystem.h>
#include <xcommon/IXCore.h>
#include "EditorBrushTool.h"
#include "FaceEditTool.h"
#include "ClipTool.h"
#include "VertexTool.h"

class CEditorExtension final: public IXUnknownImplementation<IXEditorExtension>
{
public:
	CEditorExtension(CEditable *pEditable, IXEditor *pEditor);
	~CEditorExtension();

	UINT XMETHODCALLTYPE getPropertyTabCount() override;
	IXEditorPropertyTab* XMETHODCALLTYPE getPropertyTab(UINT uId) override;

	UINT XMETHODCALLTYPE getToolCount() override;
	bool XMETHODCALLTYPE getTool(UINT uId, IXEditorTool **ppOut) override;

	void XMETHODCALLTYPE render(bool is3D) override;

	UINT XMETHODCALLTYPE getResourceBrowserCount() override;
	bool XMETHODCALLTYPE getResourceBrowser(UINT uId, IXEditorResourceBrowser **ppOut) override;
	
	void onSelectionChanged(CEditorObject *pObject);

	CVertexTool* getVertexTool()
	{
		return(m_pVertexTool);
	}
	
private:
	CEditorBrushTool *m_pBrushTool = NULL;
	CFaceEditTool *m_pFaceEditTool = NULL;
	CClipTool *m_pClipTool = NULL;
	CVertexTool *m_pVertexTool = NULL;

	CEditable *m_pEditable = NULL;
};

#endif
