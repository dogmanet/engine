#ifndef __UICONTROL_H
#define __UICONTROL_H

#include <gui/guimain.h>
#include "IUICommand.h"
#include "IUIControl.h"
#include "UIWindow.h"

template <typename T>
class CUIControl: public IXUnknownImplementation<T>
{
public:
	typedef CUIControl<T> BaseClass;

	CUIControl(ULONG uID, const String &sName):
		m_id(uID),
		m_sName(sName)
	{}

	~CUIControl()
	{
		//mem_delete(m_pClick);
		//mem_delete(m_pKeyUp);

		for(UINT i = 0, l = m_aChildren.size(); i < l; ++i)
		{
			mem_release(m_aChildren[i]);
		}
	}

	void XMETHODCALLTYPE setLabel(const char *szTitle) override
	{
		m_pNode->setText(StringW(CMB2WC(szTitle)), true);
	}

	void XMETHODCALLTYPE setPosition(float fSizeX, float fSizeY) override
	{
		m_pNode->getStyleSelf()->position->setExt(gui::css::ICSSproperty::POSITION_ABSOLUTE);
		m_pNode->getStyleSelf()->left->setDim(gui::css::ICSSproperty::DIM_PX);
		m_pNode->getStyleSelf()->top->setDim(gui::css::ICSSproperty::DIM_PX);
		m_pNode->getStyleSelf()->left->set(fSizeX);
		m_pNode->getStyleSelf()->top->set(fSizeY);
		m_pNode->updateStyles();
	}

	void XMETHODCALLTYPE setSize(float fSizeX, float fSizeY) override
	{
		m_pNode->getStyleSelf()->height->setDim(gui::css::ICSSproperty::DIM_PX);
		m_pNode->getStyleSelf()->width->setDim(gui::css::ICSSproperty::DIM_PX);
		m_pNode->getStyleSelf()->width->set(fSizeX);
		m_pNode->getStyleSelf()->height->set(fSizeY);
		m_pNode->updateStyles();
	}

	/*void XMETHODCALLTYPE setClickCommand(IUICommand *pCommand) override
	{
		m_pClick = pCommand;
	}*/

	/*ULONG XMETHODCALLTYPE getElementID() const override
	{
		return m_id;
	}*/

	IUIControl* XMETHODCALLTYPE getParent() const override
	{
		return(m_pParent);
	}

	UINT XMETHODCALLTYPE getChildrenCount() const override 
	{
		return(m_aChildren.size());
	}
	IUIControl* XMETHODCALLTYPE getChild(UINT uIdx) const override
	{
		assert(uIdx < m_aChildren.size());

		if(uIdx < m_aChildren.size())
		{
			return(m_aChildren[uIdx]);
		}

		return(NULL);
	}

	void XMETHODCALLTYPE insertChild(IUIControl *pChild, UINT uPos = UINT_MAX) override 
	{
		add_ref(pChild);

		if(pChild->getParent())
		{
			pChild->getParent()->removeChild(pChild);
		}

		if(uPos >= m_aChildren.size())
		{
			m_aChildren.push_back(pChild);
		}
		else
		{
			m_aChildren.insert(pChild, uPos);
		}

		pChild->setParent(this);
	}
	void XMETHODCALLTYPE removeChild(IUIControl *pChild) override 
	{
		for(UINT i = 0, l = m_aChildren.size(); i < l; ++i)
		{
			if(m_aChildren[i] == pChild)
			{
				m_aChildren[i]->setParent(NULL);
				mem_release(m_aChildren[i]);
				m_aChildren.erase(i);
				break;
			}
		}
	}

	void XMETHODCALLTYPE setParent(IUIControl *pParent) override
	{
		IUIControl *pPrevControl = NULL;
		if(pParent)
		{
			// check if we are in parent's children list
			bool isFound = false;
			for(UINT i = 0, l = pParent->getChildrenCount(); i < l; ++i)
			{
				IUIControl *pCtl = pParent->getChild(i);
				if(pCtl == this)
				{
					isFound = true;
					break;
				}

				pPrevControl = pCtl;
			}

			assert(isFound && "Cannot use IUIControl::setParent() unless control in new parent's children list!");

			if(!isFound)
			{
				pParent = NULL;
				LogError("Cannot use IUIControl::setParent() unless control in new parent's children list!");
			}
		}

		m_pParent = pParent;

		if(pParent)
		{
			gui::dom::IDOMnode *pParentNode;
			m_pParent->getInternalData(&IXUICONTROL_DOM_NODE_CONTAINER_GUID, (void**)&pParentNode);
			CUIWindow *pUIWindow;
			m_pParent->getInternalData(&IXUICONTROL_UIWINDOW_GUID, (void**)&pUIWindow);

			bool bUpdateChildren = !m_pUIWindow && pUIWindow;

			m_pUIWindow = pUIWindow;

			if(pParentNode && pUIWindow)
			{
				m_pNode = createNode(pUIWindow->getDesktop()->getDocument());
				if(m_pNode)
				{
					if(pPrevControl)
					{
						gui::dom::IDOMnode *pPrevNode;
						pPrevControl->getInternalData(&IXUICONTROL_DOM_NODE_OUTER_GUID, (void**)&pPrevNode);

						pParentNode->appendChild(m_pNode, true, pPrevNode ? pPrevNode->nextSibling() : (pParentNode->getChilds()->size() ? pParentNode->getChilds()[0][0] : NULL));
					}
					else
					{
						pParentNode->appendChild(m_pNode, true, pParentNode->getChilds()->size() ? pParentNode->getChilds()[0][0] : NULL);
					}
				}
			}
			else
			{
				cleanupNodes();
			}

			if(bUpdateChildren)
			{
				for(UINT i = 0, l = m_aChildren.size(); i < l; ++i)
				{
					m_aChildren[i]->setParent(this);
				}
			}
		}
		else
		{
			if(m_pNode && m_pNode->parentNode())
			{
				m_pNode->parentNode()->removeChild(m_pNode);
				cleanupNodes();

				for(UINT i = 0, l = m_aChildren.size(); i < l; ++i)
				{
					m_aChildren[i]->setParent(this);
				}
			}

			m_pUIWindow = NULL;
		}
	}

	void XMETHODCALLTYPE getInternalData(const XGUID *pGUID, void **ppOut) override 
	{
		if(*pGUID == IXUICONTROL_DOM_NODE_OUTER_GUID)
		{
			*ppOut = m_pNode;
		}
		else if(*pGUID == IXUICONTROL_DOM_NODE_CONTAINER_GUID)
		{
			*ppOut = m_pContainerNode;
		}
		else if(*pGUID == IXUICONTROL_UIWINDOW_GUID)
		{
			*ppOut = m_pUIWindow;
		}
		else
		{
			BaseClass::getInternalData(pGUID, ppOut);
		}
	}

	void dispatchEvent(gui::IEvent *ev) override
	{}

	virtual gui::dom::IDOMnode* createNode(gui::dom::IDOMdocument *pDomDocument)
	{
		return(NULL);
	}

	void XMETHODCALLTYPE setEnabled(bool bEnable) override
	{
		if(bEnable)
		{
			m_pInputNode->removePseudoclass(0x00004);
		}
		else
		{
			m_pInputNode->addPseudoclass(0x00004);
		}
	}

	bool XMETHODCALLTYPE isEnabled() override 
	{
		return(m_pInputNode->pseudoclassExists(0x04));
	}

	void XMETHODCALLTYPE setVisible(bool isVisible) override
	{
		if(isVisible)
		{
			m_pNode->getStyleSelf()->display->set(gui::css::ICSSproperty::DISPLAY_NONE);
		}
		else
		{
			m_pNode->getStyleSelf()->display->unset();
		}
		m_pNode->updateStyles();
	}

protected:
	//IUICommand *m_pClick = NULL;
	//IUICommand *m_pKeyUp = NULL;

	gui::dom::IDOMnode *m_pNode = NULL;
	gui::dom::IDOMnode *m_pInputNode = NULL;
	gui::dom::IDOMnode *m_pContainerNode = NULL;
	const ULONG m_id;
	const String m_sName;

	friend class CUIWindow;

	IUIControl *m_pParent = NULL;
	Array<IUIControl*> m_aChildren;

	CUIWindow *m_pUIWindow = NULL;

	virtual void cleanupNodes()
	{
		m_pNode = NULL;
		m_pContainerNode = NULL;
		m_pInputNode = NULL;
	}
};

#endif
