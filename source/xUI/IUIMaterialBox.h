#ifndef __IUIMATERIALBOX_H
#define __IUIMATERIALBOX_H

#include "IUIControl.h"

class IUIMaterialBox: public IUIControl
{
public:
	virtual void XMETHODCALLTYPE setValue(const char *szValue) = 0;
	virtual const char* XMETHODCALLTYPE getValue() = 0;
};

#endif
