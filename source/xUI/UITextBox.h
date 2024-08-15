#ifndef __UITEXTBOX_H
#define __UITEXTBOX_H

#include "UIControl.h"
#include "IUITextBox.h"

class CUITextBox: public CUIControl<IUITextBox>
{
public:
	CUITextBox(ULONG uID);

	gui::dom::IDOMnode* createNode(gui::dom::IDOMdocument *pDomDocument) override;

	void XMETHODCALLTYPE setLabel(const char *szTitle) override;

	void XMETHODCALLTYPE setValue(const char *szValue) override;
	const char* XMETHODCALLTYPE getValue() override;

private:
	gui::dom::IDOMnode *m_pLabel = NULL;

	void cleanupNodes() override;

	String m_sValue;
};

#endif
