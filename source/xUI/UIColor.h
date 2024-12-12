#ifndef __UICOLOR_H
#define __UICOLOR_H

#include "UIControl.h"
#include "IUIColor.h"

class CUIColor: public CUIControl<IUIColor>
{
public:
	CUIColor(ULONG uID);

	gui::dom::IDOMnode* createNode(gui::dom::IDOMdocument *pDomDocument) override;

	void XMETHODCALLTYPE setLabel(const char *szTitle) override;

	void XMETHODCALLTYPE setValue(const float4_t &vColor) override;
	float4_t XMETHODCALLTYPE getValue() override;

	void XMETHODCALLTYPE dispatchEvent(gui::IEvent *ev) override;
	
	void XMETHODCALLTYPE setEnabled(bool bEnable) override;

private:
	gui::dom::IDOMnode *m_pLabel = NULL;
	gui::dom::IDOMnode *m_pPreview = NULL;

	void cleanupNodes() override;

	float4_t m_vValue;
};

#endif
