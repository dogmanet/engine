#ifndef __IUISPOILER_H
#define __IUISPOILER_H

#include "IUIControl.h"

class IUISpoiler: public IUIControl
{
public:
	virtual void XMETHODCALLTYPE setCollapsed(bool yesNo) = 0;
};

#endif
