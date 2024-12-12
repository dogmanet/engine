#include "EditorExtension.h"
#include "EditorObject.h"
#include "Editable.h"

CEditorExtension::CEditorExtension(CEditable *pEditable, IXEditor *pEditor):
	m_pEditable(pEditable)
{
	if(pEditor)
	{
		m_pBrushTool = new CEditorBrushTool(pEditable, pEditor);
		m_pFaceEditTool = new CFaceEditTool(pEditable, pEditor);
		m_pClipTool = new CClipTool(pEditable, pEditor);
		m_pVertexTool = new CVertexTool(pEditable, pEditor);
	}
}
CEditorExtension::~CEditorExtension()
{
	mem_release(m_pVertexTool);
	mem_release(m_pClipTool);
	mem_release(m_pFaceEditTool);
	mem_release(m_pBrushTool);
}

UINT XMETHODCALLTYPE CEditorExtension::getPropertyTabCount()
{
	return(0);
}
IXEditorPropertyTab* XMETHODCALLTYPE CEditorExtension::getPropertyTab(UINT uId)
{
	return(false);
}

UINT XMETHODCALLTYPE CEditorExtension::getToolCount()
{
	return(4);
}
bool XMETHODCALLTYPE CEditorExtension::getTool(UINT uId, IXEditorTool **ppOut)
{
	switch(uId)
	{
	case 0:
		add_ref(m_pBrushTool);
		*ppOut = m_pBrushTool;
		return(true);

	case 1:
		add_ref(m_pFaceEditTool);
		*ppOut = m_pFaceEditTool;
		return(true);

	case 2:
		add_ref(m_pClipTool);
		*ppOut = m_pClipTool;
		return(true);

	case 3:
		add_ref(m_pVertexTool);
		*ppOut = m_pVertexTool;
		return(true);
	}
	return(false);
}

void XMETHODCALLTYPE CEditorExtension::render(bool is3D)
{
	m_pBrushTool->render(is3D);
	m_pFaceEditTool->render(is3D);
	m_pClipTool->render(is3D);
	m_pVertexTool->render(is3D);
}

UINT XMETHODCALLTYPE CEditorExtension::getResourceBrowserCount()
{
	return(0);
}
bool XMETHODCALLTYPE CEditorExtension::getResourceBrowser(UINT uId, IXEditorResourceBrowser **ppOut)
{
	return(false);
}

void CEditorExtension::onSelectionChanged(CEditorObject *pObject)
{
	m_pVertexTool->onSelectionChanged(pObject);
//	auto &aObjects = m_pEditable->getObjects();
//
//	static UINT s_uLastSelected = 0;
//
//	UINT uSelected = 0;
//	for(UINT i = 0, l = aObjects.size(); i < l; ++i)
//	{
//		if(aObjects[i]->isSelected())
//		{
//			++uSelected;
//		}
//	}
//
//	if(uSelected != s_uLastSelected)
//	{
//		s_uLastSelected = uSelected;
//		
//		m_pOutputsTab->setEnabled(uSelected != 0);
//	}
//
//	if(uSelected != 0)
//	{
//		m_pOutputsTab->onSelectionChanged();
//	}
}
