#include "UIViewport.h"

CUIViewport::CUIViewport(IXRender *pRender, ULONG uID):
	BaseClass(uID, "div")
{
	m_szTextureName[0] = '!';
	pRender->newTextureTarget(&m_pRenderTarget, m_szTextureName + 1, sizeof(m_szTextureName) - 1);
}

CUIViewport::~CUIViewport()
{
	SAFE_CALL(m_pDoc, forceDirty, false);
	mem_release(m_pRenderTarget);
}

gui::dom::IDOMnode* CUIViewport::createNode(gui::dom::IDOMdocument *pDomDocument)
{
	m_pNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	m_pNode->setAttribute(L"controld_id", StringW(m_id));
	m_pNode->setAttribute(L"onmousewheeldown", L"handler");
	m_pNode->setAttribute(L"onmousewheelup", L"handler");
	m_pNode->setAttribute(L"onmousemove", L"handler");
	m_pNode->setAttribute(L"onmousedown", L"handler");
	m_pNode->setAttribute(L"onmouseup", L"handler");
	m_pNode->classAdd(L"viewport");
	m_pNode->setUserData(this);
	//m_pNode->setAttribute(L"src", StringW(CMB2WC(m_szTextureName)));
	m_pNode->getStyleSelf()->background_image->set(CMB2WC(m_szTextureName));

	if(!m_pDoc)
	{
		m_pDoc = pDomDocument;
		m_pDoc->forceDirty(true);
	}

	return(m_pNode);
}

void XMETHODCALLTYPE CUIViewport::getRenderTarget(IXRenderTarget **ppOut)
{
	add_ref(m_pRenderTarget);
	*ppOut = m_pRenderTarget;
}

void XMETHODCALLTYPE CUIViewport::setSize(float fSizeX, float fSizeY)
{
	BaseClass::setSize(fSizeX, fSizeY);

	m_pRenderTarget->resize((UINT)fSizeX, (UINT)fSizeY);
}
