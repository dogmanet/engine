#include "GUIbase.h"
#include "ISELECT.h"
#include "IRenderFrame.h"
#include "IScrollBar.h"
#include "DOMdocument.h"

namespace gui
{
	namespace dom
	{
		DEFINE_DOM_NODE(ISELECT);
		LINK_DOM_NODE_TO_TAG(ISELECT, L"select");

		void ISELECT::dispatchClientEvent(IEvent ev, bool *preventDefault)
		{
			if(ev.target == this)
			{
				if(ev.type == GUI_EVENT_TYPE_CLICK)
				{
					addPseudoclass(css::ICSSrule::PSEUDOCLASS_CHECKED);
				}

				if(ev.type == GUI_EVENT_TYPE_BLUR)
				{
					static UINT nOPTION = CDOMnode::getNodeIdByName(L"option");
					if(((CDOMnode*)ev.relatedTarget)->getNodeId() != nOPTION)
					{
						removePseudoclass(css::ICSSrule::PSEUDOCLASS_CHECKED);
					}
				}
			}

			BaseClass::dispatchClientEvent(ev, preventDefault);
		}

		void ISELECT::dispatchEvent(IEvent &ev)
		{
			BaseClass::dispatchEvent(ev);

			if(ev.preventDefault)
			{
				return;
			}

			render::IRenderSelectOptionsBlock *pRF = (render::IRenderSelectOptionsBlock*)((render::IRenderSelectBlock*)m_pRenderFrame)->m_pOptionsFrame;

			if(pRF && pRF->m_pScrollBarVert)
			{
				pRF->m_pScrollBarVert->dispatchEvent(ev);
			}
			switch(ev.type)
			{
			case GUI_EVENT_TYPE_MOUSEWHEELUP:
				if(pRF->m_iScrollTopMax != 0)
				{
					if(pRF->m_iScrollTop != 0)
					{
						ev.stopPropagation();
						m_pDocument->markDirty();
					}
					//m_pRenderFrame->m_iScrollTop -= 32;

					if(pRF->m_iScrollSpeedY > 0)
					{
						pRF->m_iScrollSpeedY = -SCROLL_SPEED;
					}
					else
					{
						pRF->m_iScrollSpeedY -= SCROLL_SPEED;
						if(pRF->m_iScrollSpeedY < -SCROLL_SPEED_MAX)
						{
							pRF->m_iScrollSpeedY = -SCROLL_SPEED_MAX;
						}
					}
				}
				break;
			case GUI_EVENT_TYPE_MOUSEWHEELDOWN:
				if(pRF->m_iScrollTopMax != 0)
				{
					if(pRF->m_iScrollTopMax != pRF->m_iScrollTop)
					{
						ev.stopPropagation();
						m_pDocument->markDirty();
					}
					//m_pRenderFrame->m_iScrollTop += 32;
					if(pRF->m_iScrollSpeedY < 0)
					{
						pRF->m_iScrollSpeedY = SCROLL_SPEED;
					}
					else
					{
						pRF->m_iScrollSpeedY += SCROLL_SPEED;
						if(pRF->m_iScrollSpeedY > SCROLL_SPEED_MAX)
						{
							pRF->m_iScrollSpeedY = SCROLL_SPEED_MAX;
						}
					}

				}
				break;
			}
		}

		void ISELECT::setAttribute(const StringW &name, const StringW &value)
		{
			BaseClass::setAttribute(name, value);

			if(name == L"value")
			{
				static UINT nOPTION = CDOMnode::getNodeIdByName(L"option");

				for(int i = 0, l = m_vChilds.size(); i < l; ++i)
				{
					if(((CDOMnode*)m_vChilds[i])->getNodeId() == nOPTION && m_vChilds[i]->getAttribute(L"value") == value)
					{
						setText(m_vChilds[i]->getText(), TRUE);
						break;
					}
				}
			}
		}
	};
};
