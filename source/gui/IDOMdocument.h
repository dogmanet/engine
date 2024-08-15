#ifndef _IDOMdocument_H_
#define _IDOMdocument_H_

#include "IDOM.h"
//#include "IRenderFrame.h"

namespace gui
{
	class IDesktop;
	namespace dom
	{
		class IDOMdocument
		{
		public:
			virtual ~IDOMdocument()
			{
			}

			virtual IDOMnode* createNode(const wchar_t *tag) = 0;

			virtual IDOMnode* getElementById(const StringW &id) = 0;

			virtual const IDOMnodeCollection* getElementsByClass(const StringW &id) = 0;

			virtual const IDOMnodeCollection* getElementsByPseudoClass(UINT cid) = 0;

			virtual const IDOMnodeCollection* getElementsByTag(const StringW &id) = 0;

			virtual IDOMnode* getElementByXY(int x, int y, bool sendEnterLeave = false) = 0;

			virtual IDOMnode* getRootNode() = 0;

			//	IDesktop * GetDesktop();
			virtual void setDesktop(IDesktop *pdp) = 0;

			virtual void requestFocus(IDOMnode *pn) = 0;
			virtual IDOMnode* getFocus() = 0;

			virtual const IDOMnodeCollection& createFromText(const StringW &html) = 0;

			virtual void forceDirty(bool set) = 0;

			virtual void setCapture(IDOMnode *pNode) = 0;
			virtual void releaseCapture() = 0;
		};
	};
};

#endif
