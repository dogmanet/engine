#include "UIComboBox.h"

CUIComboBox::CUIComboBox(ULONG uID) :
	BaseClass(uID, "select")
{

}

gui::dom::IDOMnode* CUIComboBox::createNode(gui::dom::IDOMdocument *pDomDocument)
{
	//m_pNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	//m_pNode->setAttribute(L"controld_id", StringW(m_id));
	//return(m_pNode);

	m_pNode = pDomDocument->createNode(L"div");
	m_pNode->classAdd(L"combobox-wrapper");
	m_pLabel = pDomDocument->createNode(L"label");
	m_pInputNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	m_pInputNode->setAttribute(L"onchange", L"handler");
	m_pInputNode->setUserData(this);

	StringW wsId = StringW(L"select_") + StringW(m_id);

	m_pInputNode->setAttribute(L"id", StringW(m_id));
	m_pLabel->setAttribute(L"for", StringW(m_id));
	
	m_pNode->appendChild(m_pLabel, true);
	m_pNode->appendChild(m_pInputNode, true);
	return(m_pNode);
}

void CUIComboBox::setLabel(const char *szTitle)
{
	m_pLabel->setText(StringW(CMB2WC(szTitle)), true);
}

void CUIComboBox::cleanupNodes()
{
	BaseClass::cleanupNodes();
	m_pLabel = NULL;
}

void XMETHODCALLTYPE CUIComboBox::addItem(const char *szName, const char *szValue)
{
	m_pInputNode->appendHTML(StringW(L"<option value=\"")+CMB2WC(szValue)+L"\">"+CMB2WC(szName)+L"</option>");
}
void XMETHODCALLTYPE CUIComboBox::removeItems()
{
	while(m_pInputNode->getChilds()->size() > 1)
	{
		m_pInputNode->removeChild(m_pInputNode->getChilds()[0][1]);
	}
}

void XMETHODCALLTYPE CUIComboBox::setValue(const char *szValue)
{
	m_pInputNode->setAttribute(L"value", StringW(CMB2WC(szValue)));
}
const char* XMETHODCALLTYPE CUIComboBox::getValue()
{
	m_sValue = m_pInputNode->getAttribute(L"value");
	return(m_sValue.c_str());
}
