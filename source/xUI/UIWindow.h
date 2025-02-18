#ifndef __UIWINDOW_H 
#define __UIWINDOW_H

#include "IUIWindow.h"
#include "UIControl.h"

class CUIWindow;
class CWindowCallback final: public IXUnknownImplementation<IXWindowCallback>
{
public:
	CWindowCallback(CUIWindow *pUIWindow, gui::IDesktopStack *pDesktopStack):
		m_pUIWindow(pUIWindow),
		m_pDesktopStack(pDesktopStack)
	{
	}

	INT_PTR XMETHODCALLTYPE onMessage(UINT msg, WPARAM wParam, LPARAM lParam, IXWindow *pWindow) override;

	void dropWindow();

private:
	CUIWindow *m_pUIWindow = NULL;
	gui::IDesktopStack *m_pDesktopStack = NULL;
	bool m_isScreenSizeChanged = false;
	bool m_isResizing = false;
	UINT m_uNewWidth = 0;
	UINT m_uNewHeight = 0;
};

//##########################################################################

class CXUI;
class CUIWindowControl;
class CUIWindow: public IXUnknownImplementation<IUIWindow>
{
public:
	friend class CWindowCallback;

	CUIWindow(CXUI *pXUI, const XWINDOW_DESC *pWindowDesc, IUIWindow *pParent = NULL);
	~CUIWindow();

	void XMETHODCALLTYPE hide() override;

	void XMETHODCALLTYPE close() override;

	void XMETHODCALLTYPE show() override;

	bool XMETHODCALLTYPE isVisible() override;

	void XMETHODCALLTYPE setTitle(const char *szTitle) override;

	void XMETHODCALLTYPE update(const XWINDOW_DESC *pWindowDesc) override;

	const XWINDOW_DESC* XMETHODCALLTYPE getDesc() override;

	gui::IDesktop* XMETHODCALLTYPE getDesktop() const;

	//IUIControl* getControlByID(ULONG uid) const;

	void callEventHandler(const WCHAR *cb_name, gui::IEvent *ev);

	void update();
	void render(IGXContext *pContext);
	void present();
	

	UINT XMETHODCALLTYPE getChildrenCount() const override;
	IUIControl* XMETHODCALLTYPE getChild(UINT uIdx) const override;

	void XMETHODCALLTYPE insertChild(IUIControl *pChild, UINT uPos = UINT_MAX) override;
	void XMETHODCALLTYPE removeChild(IUIControl *pChild) override;

	void XMETHODCALLTYPE setCallback(XUIWINDOW_PROC pfnCallback, void *pCtx) override;

	void XMETHODCALLTYPE messageBox(const char *szMessage, const char *szTitle, XMESSAGE_BOX_FLAG flags, XMESSAGEBOXFUNC pfnHandler, void *pCtx = NULL) override;

	UINT XMETHODCALLTYPE addCommand(const char *szCommand, XUICOMMAND pfnCommand, void *pCtx = NULL) override;
	void XMETHODCALLTYPE execCommand(const char *szCommand) override;

	void XMETHODCALLTYPE setAcceleratorTable(IUIAcceleratorTable *pTable) override;
	IUIAcceleratorTable* XMETHODCALLTYPE getAcceleratorTable() override;

	IXWindow* XMETHODCALLTYPE getXWindow() override;

	void registerForUpdate(IUIControl *pControl);

	void XMETHODCALLTYPE maintainPlacement(const XGUID &guid, bool bVisibilityToo = true);

private:
	CXUI *m_pXUI = NULL;
	IXWindow *m_pXWindow = NULL;
	Array<IUIControl*> m_ChildControls;
	gui::IDesktopStack *m_pDesktopStack = NULL;
	gui::IDesktop *m_pDefaultDesktop = NULL;
	IGXSwapChain *m_pGuiSwapChain = NULL;
	IGXDepthStencilSurface *m_pGuiDepthStencilSurface = NULL;
	CWindowCallback *m_pXWindowCallback = NULL;

	IUIAcceleratorTable *m_pAcceleratorTable = NULL;

	CUIWindowControl *m_pControl;

	XUIWINDOW_PROC m_pfnCallback = NULL;
	void *m_pCallbackContext = NULL;

	XMESSAGE_BOX_FLAG m_messageBoxFlags = XMBF_OK;
	XMESSAGEBOXFUNC m_pfnMessageBoxHandler = NULL;
	void *m_pMessageBoxHandlerContext = NULL;

	struct Command
	{
		String sName;
		XUICOMMAND pfnCommand = NULL;
		void *pCtx = NULL;
	};
	Array<Command> m_aCommands;

	Array<IUIControl*> m_aUpdateControls;

	XGUID m_guidPlacement;
	bool m_bMaintainPlacement = false;
	bool m_bMaintainVisibility = false;

	//! Used only for scaled windows
	XWINDOW_DESC m_descCached;

private:
	void releaseSwapChain();
	void createSwapChain(UINT uWidth, UINT uHeight);

	void onResize(UINT uWidth, UINT uHeight);

	bool onClose();

	Command* getCommand(const char *szName);

	bool translateAccelerator(UINT msg, WPARAM wParam, LPARAM lParam);

	void storePlacement();
	void restorePlacement();
};

#endif
