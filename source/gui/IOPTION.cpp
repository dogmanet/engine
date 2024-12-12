#include "GUIbase.h"
#include "IOPTION.h"


namespace gui
{
	namespace dom
	{
		DEFINE_DOM_NODE(IOPTION);
		LINK_DOM_NODE_TO_TAG(IOPTION, L"option");

		void IOPTION::dispatchClientEvent(IEvent ev, bool *preventDefault)
		{
			if(ev.type == GUI_EVENT_TYPE_CLICK)
			{
				if(m_pParent->pseudoclassExists(css::ICSSrule::PSEUDOCLASS_CHECKED))
				{
					m_pParent->setText(getText(), TRUE);
					m_pParent->setAttribute(L"value", getAttribute(L"value"));

					m_pParent->removePseudoclass(css::ICSSrule::PSEUDOCLASS_CHECKED);

					IEvent ev2 = ev;
					ev2.type = GUI_EVENT_TYPE_CHANGE;
					ev2.target = m_pParent;
					ev2.relatedTarget = this;
					m_pParent->dispatchEvent(ev2);

					ev.stopPropagation();
				}
			}

			BaseClass::dispatchClientEvent(ev, preventDefault);
		}
	};
};
