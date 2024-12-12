#ifndef __IUITEXTBOX_H
#define __IUITEXTBOX_H

#include "IUIControl.h"

class IUITextBox: public IUIControl
{
public:
	virtual void XMETHODCALLTYPE setValue(const char *szValue) = 0;
	virtual const char* XMETHODCALLTYPE getValue() = 0;
};

#endif
