#ifndef __IUIPICTURE_H
#define __IUIPICTURE_H

#include "IUIClickHandler.h"

class IUIPicture : public IUIControl
{
public:
	virtual void XMETHODCALLTYPE setPicture(const wchar_t *szName, int sizeX, int sizeY) = 0;
};

#endif
