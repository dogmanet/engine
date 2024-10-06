#include "UIMenu.h"

CUIMenu::CUIMenu(ULONG uID):
	BaseClass(uID, "div")
{
}

CUIMenu::~CUIMenu()
{
	fora(i, m_aItems)
	{
		mem_release(m_aItems[i].pSubmenu);
	}
}

gui::dom::IDOMnode* CUIMenu::createNode(gui::dom::IDOMdocument *pDomDocument)
{
	m_pDoc = pDomDocument;

	m_pNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	m_pNode->setAttribute(L"controld_id", StringW(m_id));
	m_pNode->setAttribute(L"onlayout", L"handler");
	m_pNode->setAttribute(L"onblur", L"handler");
	m_pNode->setAttribute(L"onfocus", L"handler");
	m_pNode->setAttribute(L"onmouseover", L"handler");
	m_pNode->setAttribute(L"onclick", L"handler");
	//m_pNode->setAttribute(L"onmouseout", L"handler");
	m_pNode->setUserData(this);
	m_pNode->classAdd(L"menu");
	m_pNode->getStyleSelf()->visibility->setExt(gui::css::ICSSproperty::VISIBILITY_HIDDEN);
	m_pContainerNode = m_pNode;

	update();

	return(m_pNode);
}

void CUIMenu::dispatchEvent(gui::IEvent *ev)
{
	if(ev->type == gui::GUI_EVENT_TYPE_LAYOUT && ev->target == m_pNode)
	{
		m_uWidth = m_pNode->getInnerWidth();
		m_uHeight = m_pNode->getInnerHeight();
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_FOCUS && ev->target == m_pNode)
	{
		m_pPrevFocused = ev->relatedTarget;
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_BLUR && (ev->currentTarget == m_pNode || ev->target == m_pNode))
	{
		//LogInfo("GUI_EVENT_TYPE_BLUR\n");
		bool bShouldHide = true;
		if(ev->relatedTarget)
		{
			gui::dom::IDOMnode *pNode = ev->relatedTarget;
			while(pNode)
			{
				if(pNode->classExists(L"menu"))
				{
					bShouldHide = false;
					break;
				}
				pNode = pNode->parentNode();
			}

			/*if(pNode && pNode != ev->relatedTarget)
			{
				m_pDoc->requestFocus(pNode);
			}*/
		}
		if(bShouldHide)
		{
			hideTree();
		}
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_MOUSEENTER)
	{
		//LogInfo("GUI_EVENT_TYPE_MOUSEENTER\n");
		int idx = m_aItems.indexOf(ev->target, [](const MenuItem &a, gui::dom::IDOMnode *pNode){
			return(a.pNode == pNode);
		});
		if(idx >= 0)
		{
			if(m_pCurrentSubmenu)
			{
				m_pCurrentSubmenu->hide();
				m_pCurrentSubmenu = NULL;
			}

			MenuItem &mi = m_aItems[idx];
			if(mi.pSubmenu)
			{
				RECT rc = mi.pNode->getClientRect();
				mi.pSubmenu->show(rc.right - 4, rc.top + 2, this);
				m_pCurrentSubmenu = mi.pSubmenu;
			}
		}
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_CLICK)
	{
		int idx = m_aItems.indexOf(ev->target, [](const MenuItem &a, gui::dom::IDOMnode *pNode){
			return(a.pNode == pNode || a.pNode == pNode->parentNode());
		});
		if(idx >= 0)
		{
			MenuItem &mi = m_aItems[idx];
			if(mi.sCommand.length())
			{
				m_pUIWindow->execCommand(mi.sCommand.c_str());
			}
			hideTree();
		}
	}
}

void XMETHODCALLTYPE CUIMenu::show(int x, int y)
{
	if(m_pNode && !m_isVisible)
	{
		const XWINDOW_DESC *pDesc = m_pUIWindow->getDesc();

		if(x + m_uWidth > pDesc->iSizeX)
		{
			x -= m_uWidth;
			if(x < 0)
			{
				x = 0;
			}
		}
		if(y + m_uHeight > pDesc->iSizeY)
		{
			y -= m_uHeight;
			if(y < 0)
			{
				y = 0;
			}
		}

		setPosition(x, y);
		m_pNode->getStyleSelf()->visibility->setExt(gui::css::ICSSproperty::VISIBILITY_VISIBLE);
		m_pNode->updateStyles();
		m_isVisible = true;
		m_pDoc->requestFocus(m_pNode);
	}
}

void CUIMenu::show(int x, int y, CUIMenu *pParentMenu)
{
	m_pParentMenu = pParentMenu;
	show(x, y);
}

void XMETHODCALLTYPE CUIMenu::hide()
{
	if(m_isVisible)
	{
		m_isVisible = false;

		if(m_pCurrentSubmenu)
		{
			m_pCurrentSubmenu->hide();
			m_pCurrentSubmenu = NULL;
		}

		if(m_pNode)
		{
			m_pNode->getStyleSelf()->visibility->setExt(gui::css::ICSSproperty::VISIBILITY_HIDDEN);
			m_pNode->updateStyles();

			if(m_pPrevFocused)
			{
				m_pDoc->requestFocus(m_pPrevFocused);
				m_pPrevFocused = NULL;
			}
		}

		if(m_pParentMenu)
		{
			m_pParentMenu->onChildMenuHidden(this);
			m_pParentMenu = NULL;
		}
	}
}

void CUIMenu::hideTree()
{
	if(m_pParentMenu)
	{
		m_pParentMenu->hideTree();
	}

	m_pPrevFocused = NULL;
	hide();
}

UINT XMETHODCALLTYPE CUIMenu::addSeparator()
{
	UINT uIdx = m_aItems.size();
	m_aItems[uIdx].isSeparator = true;
	return(uIdx);
}
void XMETHODCALLTYPE CUIMenu::insertSeparator(UINT uIdx)
{
	assert(uIdx < m_aItems.size());
	if(uIdx < m_aItems.size())
	{
		UINT uSep = addSeparator();
		std::swap(m_aItems[uIdx], m_aItems[uSep]);
		update();
	}
}

UINT XMETHODCALLTYPE CUIMenu::addItem(const char *szText, const char *szCommand, const char *szIcon, IUIMenu *pSubmenu)
{
	UINT uIdx = m_aItems.size();
	m_aItems[uIdx].sText = szText;
	if(szCommand)
	{
		m_aItems[uIdx].sCommand = szCommand;
	}
	if(szIcon)
	{
		m_aItems[uIdx].sIcon = szIcon;
	}
	add_ref(pSubmenu);
	m_aItems[uIdx].pSubmenu = (CUIMenu*)pSubmenu;

	update();

	return(uIdx);
}
void XMETHODCALLTYPE CUIMenu::insertItem(UINT uIdx, const char *szText, const char *szCommand, const char *szIcon, IUIMenu *pSubmenu)
{
	assert(uIdx < m_aItems.size());
	if(uIdx < m_aItems.size())
	{
		UINT uSep = addItem(szText, szCommand, szIcon, pSubmenu);
		std::swap(m_aItems[uIdx], m_aItems[uSep]);
		update();
	}
}

UINT XMETHODCALLTYPE CUIMenu::getItemCount()
{
	return(m_aItems.size());
}

const char* XMETHODCALLTYPE CUIMenu::getItemText(UINT uIdx)
{
	MenuItem *pItem = getItem(uIdx);
	if(pItem)
	{
		return(pItem->sText.c_str());
	}
	return(NULL);
}
const char* XMETHODCALLTYPE CUIMenu::getItemIcon(UINT uIdx)
{
	MenuItem *pItem = getItem(uIdx);
	if(pItem)
	{
		return(pItem->sIcon.c_str());
	}
	return(NULL);
}
const char* XMETHODCALLTYPE CUIMenu::getItemCommand(UINT uIdx)
{
	MenuItem *pItem = getItem(uIdx);
	if(pItem)
	{
		return(pItem->sCommand.c_str());
	}
	return(NULL);
}
bool XMETHODCALLTYPE CUIMenu::isItemEnabled(UINT uIdx)
{
	MenuItem *pItem = getItem(uIdx);
	if(pItem)
	{
		return(pItem->isEnabled);
	}
	return(false);
}
IUIMenu* XMETHODCALLTYPE CUIMenu::getItemSubmenu(UINT uIdx)
{
	MenuItem *pItem = getItem(uIdx);
	if(pItem)
	{
		return(pItem->pSubmenu);
	}
	return(NULL);
}

void XMETHODCALLTYPE CUIMenu::setItemText(UINT uIdx, const char *szText)
{
	MenuItem *pItem = getItem(uIdx);
	if(pItem)
	{
		pItem->sText = szText;
		update(pItem);
	}
}
void XMETHODCALLTYPE CUIMenu::setItemIcon(UINT uIdx, const char *szIcon)
{
	MenuItem *pItem = getItem(uIdx);
	if(pItem)
	{
		pItem->sIcon = szIcon;
		update(pItem);
	}
}
void XMETHODCALLTYPE CUIMenu::setItemCommand(UINT uIdx, const char *szCommand)
{
	MenuItem *pItem = getItem(uIdx);
	if(pItem)
	{
		pItem->sCommand = szCommand;
	}
}
void XMETHODCALLTYPE CUIMenu::setItemEnabled(UINT uIdx, bool isEnabled)
{
	MenuItem *pItem = getItem(uIdx);
	if(pItem)
	{
		pItem->isEnabled = isEnabled;
		update(pItem);
	}
}
void XMETHODCALLTYPE CUIMenu::setItemSubmenu(UINT uIdx, IUIMenu *pSubmenu)
{
	MenuItem *pItem = getItem(uIdx);
	if(pItem)
	{
		mem_release(pItem->pSubmenu);
		pItem->pSubmenu = (CUIMenu*)pSubmenu;
		add_ref(pItem->pSubmenu);
		update(pItem);
	}
}

bool XMETHODCALLTYPE CUIMenu::isSeparator(UINT uIdx)
{
	MenuItem *pItem = getItem(uIdx);
	if(pItem)
	{
		return(pItem->isSeparator);
	}
	return(false);
}

CUIMenu::MenuItem* CUIMenu::getItem(UINT uIdx)
{
	assert(uIdx < m_aItems.size());
	if(uIdx < m_aItems.size())
	{
		return(&m_aItems[uIdx]);
	}
	return(NULL);
}

void CUIMenu::cleanupNodes()
{
	BaseClass::cleanupNodes();

	fora(i, m_aItems)
	{
		m_aItems[i].pNode = NULL;
		m_aItems[i].pTextNode = NULL;
		m_aItems[i].pIconNode = NULL;
	}
}

void CUIMenu::onChildMenuHidden(CUIMenu *pChildMenu)
{
	if(m_pCurrentSubmenu == pChildMenu)
	{
		m_pCurrentSubmenu = NULL;
	}
}

void CUIMenu::update(MenuItem *pItem)
{
	if(pItem)
	{
		if(!pItem->pNode)
		{
			return;
		}
		
		if(pItem->isEnabled)
		{
			TODO("Use constant for pseudoclass");
			pItem->pNode->removePseudoclass(4 /* PSEUDOCLASS_DISABLED */);
		}
		else
		{
			TODO("Use constant for pseudoclass");
			pItem->pNode->addPseudoclass(4 /* PSEUDOCLASS_DISABLED */);
		}

		if(pItem->pSubmenu)
		{
			pItem->pNode->classAdd(L"-has-submenu");
		}
		else
		{
			pItem->pNode->classRemove(L"-has-submenu");
		}
		pItem->pNode->updateStyles();

		gui::css::ICSSstyle *pStyle = pItem->pIconNode->getStyleSelf();
		if(pItem->sIcon.length())
		{
			pStyle->background_image->set(CMB2WC(pItem->sIcon.c_str()));
		}
		else
		{
			pStyle->background_image->unset();
		}
		pItem->pIconNode->updateStyles();

		const char *szText = pItem->sText.c_str();
		const char *szTabPos = strchr(szText, '\t');
		if(szTabPos)
		{
			char *szNewText = (char*)alloca(sizeof(char) * (pItem->sText.length() + 34));
			int iTextLen = szTabPos - szText;
			memcpy(szNewText, szText, iTextLen);
			//sprintf(szNewText + iTextLen, " (%s)", szTabPos + 1);
			//pItem->pTextNode->setText(StringW(CMB2WC(szNewText)), TRUE);

			sprintf(szNewText + iTextLen, " <span class=\"-shortcut\">(%s)</span>", szTabPos + 1);
			pItem->pTextNode->setHTML(StringW(CMB2WC(szNewText)), TRUE);
		}
		else
		{
			pItem->pTextNode->setText(StringW(CMB2WC(szText)), TRUE);
		}
	}
	else if(m_pDoc)
	{
		// build structure
		fora(i, m_aItems)
		{
			if(m_aItems[i].pNode)
			{
				m_pContainerNode->removeChild(m_aItems[i].pNode);
			}
		}

		bool wasSep = false;
		fora(i, m_aItems)
		{
			MenuItem *pItem = &m_aItems[i];

			if(pItem->isSeparator && (wasSep || i == 0 || i == il - 1))
			{
				continue;
			}
			wasSep = pItem->isSeparator;

			pItem->pNode = m_pDoc->createNode(L"div");
			pItem->pNode->classAdd(L"menu-item");

			if(pItem->isSeparator)
			{
				pItem->pNode->classAdd(L"-sep");
			}
			else
			{
				pItem->pIconNode = m_pDoc->createNode(L"div");
				pItem->pIconNode->classAdd(L"-icon");
				pItem->pNode->appendChild(pItem->pIconNode);

				pItem->pTextNode = m_pDoc->createNode(L"div");
				pItem->pTextNode->classAdd(L"-text");
				pItem->pNode->appendChild(pItem->pTextNode);

				update(pItem);
			}

			m_pContainerNode->appendChild(pItem->pNode);
		}
	}
}
