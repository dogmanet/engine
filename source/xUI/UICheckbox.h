#ifndef __UICKECKBOX_H
#define __UICKECKBOX_H

#include "UIControl.h"
#include "IUICheckbox.h"

class CUICheckBox: public CUIControl<IUICheckbox>
{
public:
	CUICheckBox(ULONG uID);

	gui::dom::IDOMnode* createNode(gui::dom::IDOMdocument *pDomDocument) override;

	void XMETHODCALLTYPE setLabel(const char *szTitle) override;

	bool XMETHODCALLTYPE isChecked() override;

	void XMETHODCALLTYPE setChecked(bool set) override;

private:
	gui::dom::IDOMnode *m_pText = NULL;

	void cleanupNodes() override;
};

#endif
