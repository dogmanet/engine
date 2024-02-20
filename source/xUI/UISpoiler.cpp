#include "UISpoiler.h"

CUISpoiler::CUISpoiler(ULONG uID) :
	BaseClass(uID, "div")
{
}

gui::dom::IDOMnode* CUISpoiler::createNode(gui::dom::IDOMdocument *pDomDocument)
{
	m_pNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	m_pNode->setAttribute(L"controld_id", StringW(m_id));
	
	m_pNode->classAdd(L"spoiler");

	m_pHeader = pDomDocument->createNode(L"div");
	m_pHeader->classAdd(L"spoiler-head");
	m_pHeader->setAttribute(L"onclick", L"handler");
	m_pHeader->setUserData(this);
	m_pNode->appendChild(m_pHeader);

	m_pContainerNode = pDomDocument->createNode(L"div");
	m_pContainerNode->classAdd(L"spoiler-content");
	m_pNode->appendChild(m_pContainerNode);
	return(m_pNode);
}

void CUISpoiler::setLabel(const char *szTitle)
{
	m_pHeader->setText(StringW(CMB2WC(szTitle)), true);
}

void CUISpoiler::dispatchEvent(gui::IEvent *ev)
{
	if(ev->type == gui::GUI_EVENT_TYPE_CLICK)
	{
		m_isCollapsed = !m_isCollapsed;

		if(m_isCollapsed)
		{
			m_pContainerNode->getStyleSelf()->display->set(gui::css::ICSSproperty::DISPLAY_NONE);
		}
		else
		{
			m_pContainerNode->getStyleSelf()->display->unset();
		}
		m_pContainerNode->updateStyles();
	}
}

void CUISpoiler::cleanupNodes()
{
	BaseClass::cleanupNodes();
	m_pText = NULL;
	m_pHeader = NULL;
}
