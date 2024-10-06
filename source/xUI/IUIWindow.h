#ifndef __IUIWINDOW_H 
#define __IUIWINDOW_H

#include <xWindow/IXWindowSystem.h>
#include "IUIControl.h"
#include <gui/guimain.h>
#include "IUIAcceleratorTable.h"

typedef void(*XUIWINDOW_PROC)(void *pCtx, IUIControl *pControl, gui::IEvent *ev);

enum XMESSAGE_BOX_FLAG
{
	XMBF_OK = 0x0000, //!< The message box contains one push button : OK.This is the default.
	XMBF_YESNO = 0x0001, //!< The message box contains two push buttons : Yes and No.
	XMBF_YESNOCANCEL = 0x0002, //!< The message box contains three push buttons : Yes, No, and Cancel.
	XMBF_CANCELTRYCONTINUE = 0x0003, //!< The message box contains three push buttons : Cancel, Try Again, Continue.
	XMBF_OKCANCEL = 0x0004, //!< The message box contains two push buttons : OK and Cancel.
	XMBF_RETRYCANCEL = 0x0005, //!< The message box contains two push buttons : Retry and Cancel.

	XMBF_ICONINFORMATION = 0x0000, // default
	XMBF_ICONERROR = 0x0010,
	XMBF_ICONWARNING = 0x0020,
	XMBF_ICONQUESTION = 0x0030
};

XDEFINE_ENUM_FLAG_OPERATORS(XMESSAGE_BOX_FLAG);

enum XMESSAGE_BOX_RESULT
{
	XMBR_OK,
	XMBR_YES,
	XMBR_NO,
	XMBR_CANCEL,
	XMBR_TRY_AGAIN,
	XMBR_CONTINUE,
	XMBR_RETRY,
};

typedef void(*XMESSAGEBOXFUNC)(void *pCtx, XMESSAGE_BOX_RESULT result);

typedef void(*XUICOMMAND)(void *pCtx);

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

	virtual void XMETHODCALLTYPE messageBox(const char *szMessage, const char *szTitle, XMESSAGE_BOX_FLAG flags, XMESSAGEBOXFUNC pfnHandler, void *pCtx = NULL) = 0;

	virtual UINT XMETHODCALLTYPE addCommand(const char *szCommand, XUICOMMAND pfnCommand, void *pCtx = NULL) = 0;
	virtual void XMETHODCALLTYPE execCommand(const char *szCommand) = 0;

	virtual void XMETHODCALLTYPE setAcceleratorTable(IUIAcceleratorTable *pTable) = 0;
	virtual IUIAcceleratorTable* XMETHODCALLTYPE getAcceleratorTable() = 0;

	virtual IXWindow* XMETHODCALLTYPE getXWindow() = 0;

	virtual void XMETHODCALLTYPE maintainPlacement(const XGUID &guid, bool bVisibilityToo = true) = 0;
};

#endif
