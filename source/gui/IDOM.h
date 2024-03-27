#ifndef _IDOM_H_
#define _IDOM_H_

#ifdef GUI_DLL
#include "GUIbase.h"
#include "ICSS.h"
#else
#include "ICSSstyle.h"
#endif
#include "IEvent.h"

namespace gui
{
	namespace dom
	{
		class IDOMnode
		{
		public:
			virtual ~IDOMnode()
			{
			};
			virtual void appendChild(IDOMnode * pEl, bool regen = true, IDOMnode *pInsertBefore=NULL) = 0;
			virtual void appendHTML(const StringW &wsHTML, bool regen = true, IDOMnode *pInsertBefore = NULL) = 0;
			virtual void setHTML(const StringW &wsHTML, bool regen = true) = 0;
			virtual void removeChild(IDOMnode * pEl, bool regen = true) = 0;
			virtual void takeChild(IDOMnode * pEl, bool regen = true) = 0;
			virtual IDOMnode * parentNode() = 0;

			virtual void setText(const StringW & text, BOOL build=FALSE) = 0;
			virtual const StringW & getText() = 0;

			virtual bool isTextNode() = 0;

			virtual void setAttribute(const StringW & name, const StringW & value) = 0;
			virtual const StringW & getAttribute(const StringW & name) = 0;
			
			virtual bool isChildOf(IDOMnode * pNode) = 0;
			virtual bool isAfter(IDOMnode * pNode) = 0;

			virtual IDOMnode * prevSibling() = 0;

			virtual IDOMnode * nextSibling() = 0;

			virtual css::ICSSstyle * getStyle() = 0;

			virtual css::ICSSstyle * getStyleSelf() = 0;

			virtual const Array<IDOMnode*> * getChilds() = 0;

			virtual void addPseudoclass(UINT id) = 0;
			virtual bool pseudoclassExists(UINT id) = 0;
			virtual void removePseudoclass(UINT id) = 0;

			virtual void dispatchEvent(IEvent & ev) = 0;
			virtual void dispatchClientEvent(IEvent ev, bool * preventDefault) = 0;

			virtual void updateStyles(bool forceUpdate = false) = 0;

			virtual void classAdd(const StringW & cls) = 0;

			virtual void classRemove(const StringW & cls) = 0;

			virtual void classToggle(const StringW & cls, int set = -1) = 0;

			virtual BOOL classExists(const StringW & cls) = 0;

			virtual RECT getClientRect() = 0;
		};


		typedef Array<IDOMnode*> IDOMnodeCollection;
	};
};

#endif
