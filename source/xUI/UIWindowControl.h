#ifndef __UIGRID_H
#define __UIGRID_H

#include "IUIWindow.h"
#include "UIControl.h"

class CUIWindow;
class CUIWindowControl: public CUIControl<IUIControl>
{
public:
	CUIWindowControl(CUIWindow *pWindow, ULONG uID);
};

#endif
