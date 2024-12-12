#ifndef _CDesktop_H_
#define _CDesktop_H_

#include "GUIbase.h"
#include "IDesktop.h"
#include "DOMdocument.h"

namespace gui
{
	class CDesktop: public IXUnknownImplementation<IDesktop>
	{
	public:
		CDesktop(CDesktopStack *pDecktopStack, const StringW &sName);
		~CDesktop();

		void loadFromFile(const WCHAR *str) override;

		void setWidth(UINT w) override;
		void setHeight(UINT h) override;
		void updateSize() override;

		void render(float fTimeDelta, bool bPresent = true) override;

		IXTexture* getTexture() override;

		void dispatchEvent(IEvent &ev) override;

		dom::IDOMdocument* getDocument() override;

		void requestFocus(dom::IDOMnode *pNode) override;

		dom::IDOMnode* getFocus() override;

		const dom::IDOMnodeCollection& createFromText(const StringW &html) override;
		
		void createRenderTarget();
		void releaseRenderTarget();
		void setDirty();

		float getParallaxFactor() override;

		void reload() override;

	protected:
		CDesktopStack *m_pDesktopStack;

		StringW m_wsFile;

		UINT m_iWidth;
		UINT m_iHeight;

		IXTexture *m_txFinal = NULL;

		bool m_bShowSimulatedCursor;
		bool m_bShowSystemCursor;

		dom::CDOMdocument * m_pDoc;

		dom::IDOMnode * m_pFocusedNode = NULL;
		dom::IDOMnode * m_pHoveredNode = NULL;

		//dom::IDOMnodeCollection m_cTmpNodes;

		IGXSurface * m_pRenderSurface = NULL;
		IGXDepthStencilSurface * m_pDepthStencilSurface = NULL;

		StringW m_sName;

		float m_fParallaxFactor = 0.0f;

		IGXVertexBuffer *m_pVertices = NULL;
		IGXRenderBuffer *m_pRenderBuffer = NULL;
		IGXConstantBuffer *m_pColorWhite = NULL;
	};
};

#endif
