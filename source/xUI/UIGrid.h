#ifndef __UIGRID_H
#define __UIGRID_H

#include "IUIWindow.h"
#include "UIControl.h"
#include "IUIGrid.h"

class CUIGrid : public CUIControl<IUIGrid>
{
private:
	Array<IUIControl> mControls;
	IUIWindow *m_pWindow;

	int rows = 0;
	int columns = 0;
};

#endif
