#ifndef __UIVIEWPORT_H
#define __UIVIEWPORT_H

#include "UIControl.h"
#include "IUIViewport.h"

class CUIViewport: public CUIControl<IUIViewport>
{
public:
	CUIViewport(IXRender *pRender, ULONG uID);
	~CUIViewport();

	gui::dom::IDOMnode* createNode(gui::dom::IDOMdocument *pDomDocument) override;

	void XMETHODCALLTYPE getRenderTarget(IXRenderTarget **ppOut) override;

	//void XMETHODCALLTYPE setPicture(const wchar_t *szName, int sizeX, int sizeY) override;

	void XMETHODCALLTYPE setSize(float fSizeX, float fSizeY) override;

private:
	IXRenderTarget *m_pRenderTarget = NULL;
	char m_szTextureName[64];

	gui::dom::IDOMdocument *m_pDoc = NULL;
};

#endif
