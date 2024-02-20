#include "GUIbase.h"
#include "ICHECKBOX.h"

namespace gui
{
	namespace dom
	{
		DEFINE_DOM_NODE(ICHECKBOX);
		LINK_DOM_NODE_TO_TAG(ICHECKBOX, L"checkbox");

		ICHECKBOX::ICHECKBOX()
		{
			m_bToggleable = true;
			m_bFocusable = true;
		}

		void ICHECKBOX::setAttribute(const StringW &name, const StringW &value)
		{
			if(name == L"checked")
			{
				if(value.toBool())
				{
					addPseudoclass(css::ICSSrule::PSEUDOCLASS_ACTIVE);
					m_bToggleState = true;
				}
				else
				{
					removePseudoclass(css::ICSSrule::PSEUDOCLASS_ACTIVE);
					m_bToggleState = false;
				}
			}
			BaseClass::setAttribute(name, value);
		}
	};
};
