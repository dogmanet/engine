#ifndef __UIDEMOCONTAINER_H
#define __UIDEMOCONTAINER_H

#include <xUI/UIWindow.h>
#include <xUI/IXUI.h>
//#include "UICommand.h"
//#include "UIClassCommand.h"

class UIDemoContainer : public IXUnknownImplementation<IXUnknown>
{
public:
	UIDemoContainer(IXUI* pXUI);

private:
	IXUI* pXUI = NULL;
	IUIWindow *pDemo = NULL;

	IUITextBox *m_pTextBox = NULL;
	IUIButton *m_pButton = NULL;
};

#endif
