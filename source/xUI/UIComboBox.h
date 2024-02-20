#ifndef __UICOMBOBOXBOX_H
#define __UICOMBOBOXBOX_H

#include "UIControl.h"
#include "IUIComboBox.h"

class CUIComboBox: public CUIControl<IUIComboBox>
{
public:
	CUIComboBox(ULONG uID);

	gui::dom::IDOMnode* createNode(gui::dom::IDOMdocument *pDomDocument) override;

	void setLabel(const char *szTitle) override;

	void XMETHODCALLTYPE addItem(const char *szName, const char *szValue) override;
	void XMETHODCALLTYPE removeItems() override;

	void XMETHODCALLTYPE setValue(const char *szValue) override;
	const char* XMETHODCALLTYPE getValue() override;

private:
	gui::dom::IDOMnode *m_pLabel = NULL;

	void cleanupNodes() override;

	String m_sValue;
};

#endif
