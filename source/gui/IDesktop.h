#ifndef _IDesktop_H_
#define _IDesktop_H_

//#include "guimain.h"
#include "IDOMdocument.h"
//#include "ITexture.h"
//#include "IEvent.h"

namespace gui
{
	namespace dom
	{
		class IDOMnode;
	};
	class CTexture;
	class IEvent;

	class IDesktop: public IXUnknown
	{
	public:
		virtual void loadFromFile(const WCHAR* str) = 0;

		virtual void setWidth(UINT w) = 0;
		virtual void setHeight(UINT h) = 0;
		virtual void updateSize() = 0;

		virtual void render(float fTimeDelta, bool bPresent=true) = 0;

		virtual IXTexture* getTexture() = 0;

		virtual void dispatchEvent(IEvent &ev) = 0;

		virtual dom::IDOMdocument* getDocument() = 0;

		virtual void requestFocus(dom::IDOMnode *pNode) = 0;

		virtual dom::IDOMnode* getFocus() = 0;

		virtual const dom::IDOMnodeCollection& createFromText(const StringW &html) = 0;

		virtual float getParallaxFactor() = 0;

		virtual void reload() = 0;
	};
};

#endif
