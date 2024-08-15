#ifndef _IOPTION_H_
#define _IOPTION_H_

#include "DOM.h"

namespace gui
{
	namespace dom
	{
		class IOPTION: public IDOMnodeTag
		{
		public:
			DECLARE_CLASS(IOPTION, IDOMnodeTag);
			DECLARE_DOM_NODE(IOPTION);

			void dispatchClientEvent(IEvent ev, bool * preventDefault) override;
		};
	};
};

#endif
