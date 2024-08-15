#ifndef __UISPOILER_H
#define __UISPOILER_H

#include "UIControl.h"
#include "IUISpoiler.h"

class CUISpoiler: public CUIControl<IUISpoiler>
{
public:
	CUISpoiler(ULONG uID);

	gui::dom::IDOMnode* createNode(gui::dom::IDOMdocument *pDomDocument) override;

	void XMETHODCALLTYPE setLabel(const char *szTitle) override;

	void XMETHODCALLTYPE dispatchEvent(gui::IEvent *ev) override;

	void XMETHODCALLTYPE setCollapsed(bool yesNo) override;

protected:
	void cleanupNodes() override;

private:
	gui::dom::IDOMnode *m_pHeader = NULL;
	gui::dom::IDOMnode *m_pText = NULL;

	bool m_isCollapsed = false;
};

#endif
