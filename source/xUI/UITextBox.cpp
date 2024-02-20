#include "UITextBox.h"

CUITextBox::CUITextBox(ULONG uID) :
	BaseClass(uID, "input")
{

}

gui::dom::IDOMnode* CUITextBox::createNode(gui::dom::IDOMdocument *pDomDocument)
{
	//m_pNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	//m_pNode->setAttribute(L"controld_id", StringW(m_id));
	//return(m_pNode);

	m_pNode = pDomDocument->createNode(L"div");
	m_pNode->classAdd(L"textfield-wrapper");
	m_pLabel = pDomDocument->createNode(L"label");
	m_pInputNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	m_pInputNode->setUserData(this);
	m_pInputNode->setAttribute(L"onchange", L"handler");
	m_pInputNode->setAttribute(L"onkeyup", L"handler");
	m_pInputNode->setAttribute(L"onkeypress", L"handler");

	StringW wsId = StringW(L"input_") + StringW(m_id);

	m_pInputNode->setAttribute(L"id", StringW(m_id));
	m_pLabel->setAttribute(L"for", StringW(m_id));
	
	m_pNode->appendChild(m_pLabel, true);
	m_pNode->appendChild(m_pInputNode, true);
	return(m_pNode);
}

void CUITextBox::setLabel(const char *szTitle)
{
	m_pLabel->setText(StringW(CMB2WC(szTitle)), true);
}

void CUITextBox::cleanupNodes()
{
	BaseClass::cleanupNodes();
	m_pLabel = NULL;
}

void XMETHODCALLTYPE CUITextBox::setValue(const char *szValue) 
{
	m_pInputNode->setText(StringW(CMB2WC(szValue)));
}
const char* XMETHODCALLTYPE CUITextBox::getValue()
{
	m_sValue = m_pInputNode->getText();
	return(m_sValue.c_str());
}
