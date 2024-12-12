#include "EditorExtension.h"
#include "resource.h"
#include "EditorObject.h"
#include "Editable.h"

CEditorExtension::CEditorExtension(CEditable *pEditable):
	m_pEditable(pEditable),
	m_pOutputsTab(new CEditorOutputsTab(pEditable))
{
}
CEditorExtension::~CEditorExtension()
{
	mem_release(m_pOutputsTab);
}

UINT XMETHODCALLTYPE CEditorExtension::getPropertyTabCount()
{
	return(1);
}
IXEditorPropertyTab* XMETHODCALLTYPE CEditorExtension::getPropertyTab(UINT uId)
{
	return(m_pOutputsTab);
}

UINT XMETHODCALLTYPE CEditorExtension::getToolCount()
{
	return(0);
}
bool XMETHODCALLTYPE CEditorExtension::getTool(UINT uId, IXEditorTool **ppOut)
{
	return(false);
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
	auto &aObjects = m_pEditable->getObjects();

	static UINT s_uLastSelected = 0;

	UINT uSelected = 0;
	for(UINT i = 0, l = aObjects.size(); i < l; ++i)
	{
		if(aObjects[i]->isSelected())
		{
			++uSelected;
		}
	}

	if(uSelected != s_uLastSelected)
	{
		s_uLastSelected = uSelected;
		
		m_pOutputsTab->setEnabled(uSelected != 0);
	}

	if(uSelected != 0)
	{
		m_pOutputsTab->onSelectionChanged();
	}
}
