#ifndef _ISELECT_H_
#define _ISELECT_H_

#include "DOM.h"

namespace gui
{
	namespace dom
	{
		class ISELECT: public IDOMnodeTag
		{
		public:
			DECLARE_CLASS(ISELECT, IDOMnodeTag);
			DECLARE_DOM_NODE(ISELECT);

			void dispatchClientEvent(IEvent ev, bool *preventDefault) override;
			void setAttribute(const StringW & name, const StringW & value) override;
			void dispatchEvent(IEvent &ev) override;
		};
	};
};

#endif
