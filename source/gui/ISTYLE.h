#ifndef _ILINK_H_
#define _ILINK_H_

#include "DOM.h"

namespace gui
{
	namespace dom
	{
		class ISTYLE: public IDOMnodeTag
		{
		public:
			DECLARE_CLASS(ISTYLE, IDOMnodeTag);
			DECLARE_DOM_NODE(ISTYLE);
		};
	};
};

#endif
