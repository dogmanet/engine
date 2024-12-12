#ifndef __IUICLICKHANDLER_H
#define __IUICLICKHANDLER_H

#include "IUIControl.h"

class IUIClickHandler : public IUIControl
{
public:
	//Метод установки команды обработчика на клик по контролу
	virtual void XMETHODCALLTYPE setClickCommand(IUICommand* command) = 0;
};

#endif
