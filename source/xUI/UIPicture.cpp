#include "UIPicture.h"

CUIPicture::CUIPicture(ULONG uID) :
	BaseClass(uID, "img")
{

}

gui::dom::IDOMnode* CUIPicture::createNode(gui::dom::IDOMdocument *pDomDocument)
{
	m_pNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	m_pNode->setAttribute(L"controld_id", StringW(m_id));
	//m_pNode->setAttribute(L"onclick", m_wsName + m_id);
	return(m_pNode);
}

void XMETHODCALLTYPE CUIPicture::setPicture(const wchar_t *szName, int sizeX, int sizeY)
{
	m_pNode->getStyleSelf()->background_image->set(szName);
	//wchar_t htmlNode[255];
	//swprintf(htmlNode, L"<img src=\"%s\" height=\"%d\" width=\"%d\" />", szName, sizeX, sizeY);
	//m_pNode->appendHTML(htmlNode);
	m_pNode->updateStyles();
}