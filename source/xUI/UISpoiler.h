#ifndef __UISPOILER_H
#define __UISPOILER_H

#include "UIControl.h"
#include "IUISpoiler.h"

class CUISpoiler: public CUIControl<IUISpoiler>
{
public:
	CUISpoiler(ULONG uID);

	gui::dom::IDOMnode* createNode(gui::dom::IDOMdocument *pDomDocument) override;

	void setLabel(const char *szTitle) override;

	void dispatchEvent(gui::IEvent *ev) override;

protected:
	void cleanupNodes() override;

private:
	gui::dom::IDOMnode *m_pHeader = NULL;
	gui::dom::IDOMnode *m_pText = NULL;

	bool m_isCollapsed = false;
};

#endif
