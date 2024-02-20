#include "UIPanel.h"

CUIPanel::CUIPanel(ULONG uID) :
	BaseClass(uID, "div")
{
}

gui::dom::IDOMnode* CUIPanel::createNode(gui::dom::IDOMdocument *pDomDocument)
{
	m_pNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	m_pNode->setAttribute(L"controld_id", StringW(m_id));
	m_pNode->classAdd(L"panel");
	m_pContainerNode = m_pNode;
	return(m_pNode);
}
