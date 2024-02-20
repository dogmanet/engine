#ifndef __UIPICTURE_H
#define __UIPICTURE_H

#include "UIControl.h"
#include "IUIPicture.h"

class CUIPicture : public CUIControl<IUIPicture>
{
public:
	CUIPicture(ULONG uID);

	gui::dom::IDOMnode* createNode(gui::dom::IDOMdocument *pDomDocument) override;

	void XMETHODCALLTYPE setPicture(const wchar_t *szName, int sizeX, int sizeY) override;
};

#endif
