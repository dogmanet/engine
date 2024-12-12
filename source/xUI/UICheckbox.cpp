#include "UICheckbox.h"

CUICheckBox::CUICheckBox(ULONG uID):
	BaseClass(uID, "checkbox")
{

}

gui::dom::IDOMnode* CUICheckBox::createNode(gui::dom::IDOMdocument *pDomDocument)
{
	m_pNode = pDomDocument->createNode(L"div");
	m_pNode->classAdd(L"checkbox-wrapper");
	m_pText = pDomDocument->createNode(L"label");
	m_pInputNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	m_pInputNode->setUserData(this);

	StringW wsId = StringW(L"checkbox_") + StringW(m_id);

	m_pInputNode->setAttribute(L"id", StringW(m_id));
	m_pText->setAttribute(L"for", StringW(m_id));
	m_pInputNode->setAttribute(L"onclick", L"handler");
	m_pInputNode->setAttribute(L"onchange", L"handler");

	m_pNode->appendChild(m_pInputNode, true);
	m_pNode->appendChild(m_pText, true);
	return(m_pNode);
}

void XMETHODCALLTYPE CUICheckBox::setLabel(const char *szTitle)
{
	m_pText->setText(StringW(CMB2WC(szTitle)), true);
}

bool XMETHODCALLTYPE CUICheckBox::isChecked()
{
	//! @fixme: добавить enum
	return(m_pInputNode->pseudoclassExists(1));
}

void CUICheckBox::cleanupNodes()
{
	BaseClass::cleanupNodes();
	m_pText = NULL;
}

void XMETHODCALLTYPE CUICheckBox::setChecked(bool set)
{
	m_pInputNode->setAttribute(L"checked", set ? L"1" : L"0");
}

