#ifndef __IEXTENDED_TAB_H
#define __IEXTENDED_TAB_H

#include "stdafx.h"

class IExtendedTab
{
public:
	virtual ~IExtendedTab()
	{
	}

	virtual void init(HWND hWndParent) = 0;
	virtual const char* getTitle() = 0;

	virtual void show(int X, int Y, int cx, int cy) = 0;
	virtual void hide() = 0;

	virtual void commit() = 0;
	virtual void rollback() = 0;
};

#endif
