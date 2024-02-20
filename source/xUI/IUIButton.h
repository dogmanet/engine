#ifndef __IUIBUTTON_H
#define __IUIBUTTON_H

#include "IUIClickHandler.h"

class IUIButton : public IUIControl
{
public:
	virtual void XMETHODCALLTYPE setPicture(const wchar_t *szName, int sizeX, int sizeY) = 0;
};

#endif
