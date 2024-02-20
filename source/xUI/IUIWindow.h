#ifndef __IUIWINDOW_H 
#define __IUIWINDOW_H

#include <xWindow/IXWindowSystem.h>
#include "IUIControl.h"
#include <gui/guimain.h>

typedef void(*XUIWINDOW_PROC)(void *pCtx, IUIControl *pControl, gui::IEvent *ev);

class IUIWindow: public IXUnknown
{
public:
	//! Скрывает окно (остается в таскбаре)
	virtual void XMETHODCALLTYPE hide() = 0;

	//! Закрывает окно (исчезает из таскбара)
	virtual void XMETHODCALLTYPE close() = 0;

	//! Открывает окно
	virtual void XMETHODCALLTYPE show() = 0;

	//! Видимо ли окно (открыто и не свернуто)
	virtual bool XMETHODCALLTYPE isVisible() = 0;

	//! 
	virtual void XMETHODCALLTYPE setTitle(const char *szTitle) = 0;

	//! 
	virtual void XMETHODCALLTYPE update(const XWINDOW_DESC *pWindowDesc) = 0;

	//! 
	virtual const XWINDOW_DESC* XMETHODCALLTYPE getDesc() = 0;


	virtual UINT XMETHODCALLTYPE getChildrenCount() const = 0;
	virtual IUIControl* XMETHODCALLTYPE getChild(UINT uIdx) const = 0;

	virtual void XMETHODCALLTYPE insertChild(IUIControl *pChild, UINT uPos = UINT_MAX) = 0;
	virtual void XMETHODCALLTYPE removeChild(IUIControl *pChild) = 0;

	virtual void XMETHODCALLTYPE setCallback(XUIWINDOW_PROC pfnCallback, void *pCtx) = 0;
};

#endif
