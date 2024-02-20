#include "UIButton.h"

CUIButton::CUIButton(ULONG uID): 
	BaseClass(uID, "button")
{

}

gui::dom::IDOMnode* CUIButton::createNode(gui::dom::IDOMdocument *pDomDocument)
{
	m_pNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	m_pNode->setAttribute(L"controld_id", StringW(m_id));
	m_pNode->setAttribute(L"onclick", L"handler");
	m_pNode->setUserData(this);
	m_pInputNode = m_pNode;

	return(m_pNode);
}

void XMETHODCALLTYPE CUIButton::setPicture(const wchar_t *szName, int sizeX, int sizeY)
{
	wchar_t htmlNode[255];
	swprintf(htmlNode, L"<img src=\"%s\" height=\"%d\" width=\"%d\" />", szName, sizeX, sizeY);
	m_pNode->appendHTML(htmlNode);
}