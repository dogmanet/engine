#ifndef __UIPANEL_H
#define __UIPANEL_H

#include "UIControl.h"
#include "IUIPanel.h"

class CUIPanel: public CUIControl<IUIPanel>
{
public:
	CUIPanel(ULONG uID);

	gui::dom::IDOMnode* createNode(gui::dom::IDOMdocument *pDomDocument) override;
};

#endif
