#ifndef __EDITOR_EXTENSION_H
#define __EDITOR_EXTENSION_H

#include <xcommon/editor/IXEditable.h>
#include <mtrl/IXMaterialSystem.h>
#include <xcommon/IXCore.h>
#include "GameData.h"
#include "EditorOutputsTab.h"

class CEditorExtension final: public IXUnknownImplementation<IXEditorExtension>
{
public:
	CEditorExtension(CEditable *pEditable);
	~CEditorExtension();

	UINT XMETHODCALLTYPE getPropertyTabCount() override;
	IXEditorPropertyTab* XMETHODCALLTYPE getPropertyTab(UINT uId) override;

	UINT XMETHODCALLTYPE getToolCount() override;
	bool XMETHODCALLTYPE getTool(UINT uId, IXEditorTool **ppOut) override;

	void onSelectionChanged(CEditorObject *pObject);

	void XMETHODCALLTYPE render(bool is3D) override
	{
	}

	UINT XMETHODCALLTYPE getResourceBrowserCount() override;
	bool XMETHODCALLTYPE getResourceBrowser(UINT uId, IXEditorResourceBrowser **ppOut) override;

private:
	CEditorOutputsTab *m_pOutputsTab = NULL;

	CEditable *m_pEditable = NULL;
};

#endif
