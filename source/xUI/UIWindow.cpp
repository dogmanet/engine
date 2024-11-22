#include "UIWindow.h"
#include "XUI.h"
#include "UIWindowControl.h"

INT_PTR XMETHODCALLTYPE CWindowCallback::onMessage(UINT msg, WPARAM wParam, LPARAM lParam, IXWindow *pWindow)
{
	if(!m_pUIWindow)
	{
		return(pWindow->runDefaultCallback(msg, wParam, lParam));
	}

	if(!m_pDesktopStack->putMessage(msg, wParam, lParam))
	{
		return(TRUE);
	}

	if(m_pUIWindow->translateAccelerator(msg, wParam, lParam))
	{
		return(TRUE);
	}

	if(msg == WM_DESTROY)
	{
		m_pUIWindow->storePlacement();
	}

	switch(msg)
	{
	case WM_SIZE:
		m_uNewWidth = LOWORD(lParam);
		m_uNewHeight = HIWORD(lParam);
		m_isScreenSizeChanged = true;
		if(!m_isResizing)
		{
			m_pUIWindow->onResize(m_uNewWidth, m_uNewHeight);
		}
		break;

	case WM_ENTERSIZEMOVE:
		m_isResizing = true;
		m_isScreenSizeChanged = false;
		break;

	case WM_EXITSIZEMOVE:
		m_isResizing = false;
		if(m_isScreenSizeChanged)
		{
			m_isScreenSizeChanged = false;
			m_pUIWindow->onResize(m_uNewWidth, m_uNewHeight);
		}
		break;

	case WM_DPICHANGED:
		m_pUIWindow->m_pDesktopStack->setScale(m_pUIWindow->m_pXWindow->getScale());
		return(pWindow->runDefaultCallback(msg, wParam, lParam));

	case WM_CLOSE:
		if(!m_pUIWindow->onClose())
		{
			break;
		}
		// fallthrough

	default:
		return(pWindow->runDefaultCallback(msg, wParam, lParam));
	}

	return(0);
}

void CWindowCallback::dropWindow()
{
	m_pUIWindow = NULL;
	m_pDesktopStack = NULL;
}

//##########################################################################

CUIWindow::CUIWindow(CXUI *pXUI, const XWINDOW_DESC *pWindowDesc, IUIWindow *pParent):
	m_pXUI(pXUI)
{
	add_ref(pXUI);
	IXWindow *pXParent = NULL;
	if(pParent)
	{
		if(IsWindow((HWND)pParent))
		{
			FIXME("HACK: Remove that as soon as all windows is turned to xWindow");
			pXParent = (IXWindow*)pParent;
		}
		else
		{
			pXParent = ((CUIWindow*)pParent)->getXWindow();
		}
	}
	m_pDesktopStack = pXUI->getGUI()->newDesktopStack("editor_gui/", (UINT)pWindowDesc->iSizeX, (UINT)pWindowDesc->iSizeY);
	wchar_t wszName[64];
	swprintf(wszName, L"xui/0x%p", this);
	m_pDefaultDesktop = m_pDesktopStack->createDesktopW(wszName, L"main.html");
	m_pDesktopStack->setActiveDesktop(m_pDefaultDesktop);

	m_pDesktopStack->registerCallbackDefault([](const WCHAR *cb_name, gui::IEvent *ev)
	{
		CUIWindow *win = (CUIWindow*)ev->pCallbackData;
		win->callEventHandler(cb_name, ev);
	}, true, this);

	m_pXWindowCallback = new CWindowCallback(this, m_pDesktopStack);
	m_pXWindow = pXUI->getWindowSystem()->createWindow(pWindowDesc, m_pXWindowCallback, pXParent);

	createSwapChain((UINT)pWindowDesc->iSizeX, (UINT)pWindowDesc->iSizeY);

	m_pDesktopStack->setScale(m_pXWindow->getScale());

	m_pControl = new CUIWindowControl(this, 0);
}
CUIWindow::~CUIWindow()
{
	fora(i, m_aUpdateControls)
	{
		mem_release(m_aUpdateControls[i]);
	}
	releaseSwapChain();
	m_pXWindowCallback->dropWindow();
	mem_release(m_pXWindow);
	mem_release(m_pXWindowCallback);
	mem_release(m_pDefaultDesktop);
	mem_release(m_pDesktopStack);
	mem_release(m_pAcceleratorTable);

	mem_release(m_pControl);

	m_pXUI->onDestroyWindow(this);

	mem_release(m_pXUI);
}


void CUIWindow::releaseSwapChain()
{
	mem_release(m_pGuiDepthStencilSurface);
	mem_release(m_pGuiSwapChain);
}
void CUIWindow::createSwapChain(UINT uWidth, UINT uHeight)
{
	if(!m_pXWindow)
	{
		return;
	}
	m_pGuiSwapChain = m_pXUI->getGXDevice()->createSwapChain(uWidth, uHeight, m_pXWindow->getOSHandle());
	m_pGuiDepthStencilSurface = m_pXUI->getGXDevice()->createDepthStencilSurface(uWidth, uHeight, GXFMT_D24S8, GXMULTISAMPLE_NONE);
}

void CUIWindow::onResize(UINT uWidth, UINT uHeight)
{
	releaseSwapChain();
	createSwapChain(uWidth, uHeight);

	if(m_pfnCallback)
	{
		gui::IEvent ev;
		ev.type = gui::GUI_EVENT_TYPE_RESIZE;
		m_pfnCallback(m_pCallbackContext, NULL, &ev);
	}
}

void XMETHODCALLTYPE CUIWindow::hide()
{
	m_pXWindow->hide();
}

void XMETHODCALLTYPE CUIWindow::close()
{
	m_pXWindow->close();
}

void XMETHODCALLTYPE CUIWindow::show()
{
	m_pXWindow->show();
}

bool XMETHODCALLTYPE CUIWindow::isVisible()
{
	return(m_pXWindow->isVisible());
}

void XMETHODCALLTYPE CUIWindow::setTitle(const char *szTitle)
{
	m_pXWindow->setTitle(szTitle);
}

void XMETHODCALLTYPE CUIWindow::update(const XWINDOW_DESC *pWindowDesc)
{
	float fScale = m_pXWindow->getScale();

	if(fScale == 1.0f)
	{
		m_pXWindow->update(pWindowDesc);
	}
	else
	{
		XWINDOW_DESC desc = *pWindowDesc;
		desc.iSizeX = (int)((float)desc.iSizeX * fScale);
		desc.iSizeY = (int)((float)desc.iSizeY * fScale);
		m_pXWindow->update(&desc);
	}
}

const XWINDOW_DESC* XMETHODCALLTYPE CUIWindow::getDesc()
{
	float fScale = m_pXWindow->getScale();

	if(fScale == 1.0f)
	{
		return(m_pXWindow->getDesc());
	}

	m_descCached = *m_pXWindow->getDesc();
	m_descCached.iSizeX = (int)((float)m_descCached.iSizeX / fScale);
	m_descCached.iSizeY = (int)((float)m_descCached.iSizeY / fScale);
	return(&m_descCached);
}

gui::IDesktop* XMETHODCALLTYPE CUIWindow::getDesktop() const
{
	return(m_pDefaultDesktop);
}

/*IUIControl* XMETHODCALLTYPE CUIWindow::getControlByID(ULONG uid) const
{
	int index = m_ChildControls.indexOf(uid, [&](const IUIControl* control, ULONG uid) -> bool{ return control->getElementID() == uid; });
	return index == -1 ? NULL : m_ChildControls[index];
}*/

void CUIWindow::callEventHandler(const WCHAR *cb_name, gui::IEvent *ev)
{
	if(!wcscmp(cb_name, L"mbox_click"))
	{
		XMESSAGE_BOX_RESULT result = XMBR_OK;

		if(ev->type == gui::GUI_EVENT_TYPE_CLICK)
		{
			const wchar_t *wszName = ev->target->getAttribute(L"name").c_str();

			if(!wcscmp(wszName, L"IDOK"))
			{
				result = XMBR_OK;
			}
			else if(!wcscmp(wszName, L"IDYES"))
			{
				result = XMBR_YES;
			}
			else if(!wcscmp(wszName, L"IDNO"))
			{
				result = XMBR_NO;
			}
			else if(!wcscmp(wszName, L"IDCANCEL"))
			{
				result = XMBR_CANCEL;
			}
			else if(!wcscmp(wszName, L"IDTRYAGAIN"))
			{
				result = XMBR_TRY_AGAIN;
			}
			else if(!wcscmp(wszName, L"IDCONTINUE"))
			{
				result = XMBR_CONTINUE;
			}
			else if(!wcscmp(wszName, L"IDRETRY"))
			{
				result = XMBR_RETRY;
			}
		}

		if((ev->key == KEY_ESCAPE && ((m_messageBoxFlags & 0xf) == XMBF_OK)) || ev->type == gui::GUI_EVENT_TYPE_CLICK)
		{
			m_pDesktopStack->popDesktop();

			// copy handler so we can spawn another messagebox from it
			XMESSAGEBOXFUNC pfnHandler = m_pfnMessageBoxHandler;
			m_pfnMessageBoxHandler = NULL;
			pfnHandler(m_pMessageBoxHandlerContext, result);
		}

		if(ev->key == KEY_ESCAPE)
		{
			ev->preventDefault = true;
		}
		return;
	}

	IUIControl *pControl = (IUIControl*)ev->target->getUserData();
	if(!pControl)
	{
		pControl = (IUIControl*)ev->currentTarget->getUserData();
	}

	if(m_pfnCallback)
	{
		m_pfnCallback(m_pCallbackContext, pControl, ev);
	}

	if(!ev->preventDefault)
	{
		SAFE_CALL(pControl, dispatchEvent, ev);
	}
}

void CUIWindow::update()
{
	gui::IEvent ev;
	ev.type = gui::GUI_EVENT_TYPE_UPDATE;
	fora(i, m_aUpdateControls)
	{
		m_aUpdateControls[i]->dispatchEvent(&ev);
	}
	m_aUpdateControls.clearFast();
}

void CUIWindow::render(IGXContext *pContext)
{
	IGXSurface *pTarget = m_pGuiSwapChain->getColorTarget();
	pContext->setColorTarget(pTarget);
	mem_release(pTarget);

	pContext->setDepthStencilSurface(m_pGuiDepthStencilSurface);

	pContext->clear(GX_CLEAR_COLOR);

	m_pDesktopStack->render();
}

void CUIWindow::present()
{
	m_pGuiSwapChain->swapBuffers();
}

UINT XMETHODCALLTYPE CUIWindow::getChildrenCount() const
{
	return(m_pControl->getChildrenCount());
}
IUIControl* XMETHODCALLTYPE CUIWindow::getChild(UINT uIdx) const
{
	return(m_pControl->getChild(uIdx));
}

void XMETHODCALLTYPE CUIWindow::insertChild(IUIControl *pChild, UINT uPos)
{
	m_pControl->insertChild(pChild, uPos);
}
void XMETHODCALLTYPE CUIWindow::removeChild(IUIControl *pChild)
{
	m_pControl->removeChild(pChild);
}

void XMETHODCALLTYPE CUIWindow::setCallback(XUIWINDOW_PROC pfnCallback, void *pCtx)
{
	m_pfnCallback = pfnCallback;
	m_pCallbackContext = pCtx;
}

bool CUIWindow::onClose()
{
	if(m_pfnCallback)
	{
		gui::IEvent ev;
		ev.type = gui::GUI_EVENT_TYPE_CLOSE;

		m_pfnCallback(m_pCallbackContext, NULL, &ev);

		return(!ev.preventDefault);
	}
	return(true);
}

void XMETHODCALLTYPE CUIWindow::messageBox(const char *szMessage, const char *szTitle, XMESSAGE_BOX_FLAG flags, XMESSAGEBOXFUNC pfnHandler, void *pCtx)
{
	assert(!m_pfnMessageBoxHandler);
	assert(pfnHandler);
	if(m_pfnMessageBoxHandler)
	{
		LogError("Unable to spawn second messagebox!\n");
		return;
	}
	if(!pfnHandler)
	{
		LogError("Messagebox handler cannot be NULL!\n");
		return;
	}

	m_pfnMessageBoxHandler = pfnHandler;
	m_pMessageBoxHandlerContext = pCtx;
	m_messageBoxFlags = flags;

	XMESSAGE_BOX_FLAG icon = (XMESSAGE_BOX_FLAG)(flags & 0xf0);
	XMESSAGE_BOX_FLAG buttons = (XMESSAGE_BOX_FLAG)(flags & 0x0f);
	gui::IDesktop *dp;
	if((icon == XMBF_ICONERROR || icon == XMBF_ICONWARNING) && buttons == XMBF_OK)
	{
		dp = m_pDesktopStack->createDesktopW(L"MessageBoxUI", L"sys/messagebox_warn.html");
		
		gui::dom::IDOMdocument *doc = dp->getDocument();
		doc->getElementById(L"mb_title")->setText(StringW(szTitle ? CMB2WC(szTitle) : ((flags & XMBF_ICONERROR) ? L"ERROR" : L"WARNING")), TRUE);
		doc->getElementById(L"mb_text")->setText(StringW(CMB2WC(szMessage)), TRUE);

		m_pDesktopStack->pushDesktop(dp);
	}
	else
	{
		dp = m_pDesktopStack->createDesktopW(L"MessageBoxUI", L"sys/messagebox_reg.html");

		gui::dom::IDOMdocument *doc = dp->getDocument();

		const wchar_t *wszDefaultTitle = NULL;
		switch(icon)
		{
		case XMBF_ICONERROR:
			wszDefaultTitle = L"ERROR";
			break;
		case XMBF_ICONWARNING:
			wszDefaultTitle = L"WARNING";
			break;
		case XMBF_ICONINFORMATION:
			wszDefaultTitle = L"INFO";
			break;
		case XMBF_ICONQUESTION:
			wszDefaultTitle = L"QUESTION";
			break;
		}

		const wchar_t *wszId = NULL;
		switch(flags & 0xF)
		{
		case XMBF_OK:
			wszId = L"XMBF_OK";
			break;
		case XMBF_YESNO:
			wszId = L"XMBF_YESNO";
			break;
		case XMBF_YESNOCANCEL:
			wszId = L"XMBF_YESNOCANCEL";
			break;
		case XMBF_CANCELTRYCONTINUE:
			wszId = L"XMBF_CANCELTRYCONTINUE";
			break;
		case XMBF_OKCANCEL:
			wszId = L"XMBF_OKCANCEL";
			break;
		case XMBF_RETRYCANCEL:
			wszId = L"XMBF_RETRYCANCEL";
			break;
		}

		if(wszId)
		{
			gui::dom::IDOMnode *pNode = doc->getElementById(wszId);
			if(pNode)
			{
				pNode->getStyleSelf()->display->unset();
				pNode->updateStyles(true);
			}
		}

		gui::dom::IDOMnode *pBody = doc->getElementsByTag(L"body")[0][0];
		pBody->classAdd((icon == XMBF_ICONERROR || icon == XMBF_ICONWARNING) ? L"warn" : L"info");
		pBody->updateStyles();

		doc->getElementById(L"mb_title")->setText(StringW(szTitle ? CMB2WC(szTitle) : wszDefaultTitle), TRUE);
		doc->getElementById(L"mb_text")->setText(StringW(CMB2WC(szMessage)), TRUE);
		
		m_pDesktopStack->pushDesktop(dp);
	}

	mem_release(dp);
}

UINT XMETHODCALLTYPE CUIWindow::addCommand(const char *szCommand, XUICOMMAND pfnCommand, void *pCtx)
{
	if(getCommand(szCommand))
	{
		LogError("Command '%s' already defined!\n", szCommand);
		return(-1);
	}

	UINT idx = m_aCommands.size();
	Command &cmd = m_aCommands[idx];
	cmd.sName = szCommand;
	cmd.pfnCommand = pfnCommand;
	cmd.pCtx = pCtx;
	return(idx);
}

CUIWindow::Command* CUIWindow::getCommand(const char *szName)
{
	int idx = m_aCommands.indexOf(szName, [](const Command &cmd, const char *szName){
		return(!strcasecmp(szName, cmd.sName.c_str()));
	});
	if(idx >= 0)
	{
		return(&m_aCommands[idx]);
	}
	return(NULL);
}

void XMETHODCALLTYPE CUIWindow::execCommand(const char *szCommand)
{
	Command *pCmd = getCommand(szCommand);
	if(pCmd)
	{
		pCmd->pfnCommand(pCmd->pCtx);
	}
	else
	{
		LogError("Trying to execute unknown command '%s'\n", szCommand);
	}
}

void XMETHODCALLTYPE CUIWindow::setAcceleratorTable(IUIAcceleratorTable *pTable)
{
	mem_release(m_pAcceleratorTable);
	m_pAcceleratorTable = pTable;
	add_ref(m_pAcceleratorTable);
}
IUIAcceleratorTable* XMETHODCALLTYPE CUIWindow::getAcceleratorTable()
{
	return(m_pAcceleratorTable);
}

IXWindow* XMETHODCALLTYPE CUIWindow::getXWindow()
{
	return(m_pXWindow);
}

void CUIWindow::registerForUpdate(IUIControl *pControl)
{
	if(m_aUpdateControls.indexOf(pControl) < 0)
	{
		add_ref(pControl);
		m_aUpdateControls.push_back(pControl);
	}
}

void XMETHODCALLTYPE CUIWindow::maintainPlacement(const XGUID &guid, bool bVisibilityToo)
{
	m_guidPlacement = guid;
	m_bMaintainPlacement = true;
	m_bMaintainVisibility = bVisibilityToo;

	restorePlacement();
}

bool CUIWindow::translateAccelerator(UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(
		msg != WM_KEYDOWN &&
		msg != WM_SYSKEYDOWN &&
		msg != WM_CHAR &&
		msg != WM_SYSCHAR
	)
	{
		return(false);
	}

	if(!m_pAcceleratorTable)
	{
		return(false);
	}
	
	XACCEL_FLAG mask = XAF_NONE;
	if(m_pDesktopStack->getKeyState(KEY_CTRL))
	{
		mask |= XAF_CTRL;
	}
	if(m_pDesktopStack->getKeyState(KEY_ALT))
	{
		mask |= XAF_ALT;
	}
	if(m_pDesktopStack->getKeyState(KEY_SHIFT))
	{
		mask |= XAF_SHIFT;
	}

	const XAccelItem *pAccel;
	for(UINT i = 0, l = m_pAcceleratorTable->getItemCount(); i < l; ++i)
	{
		if((pAccel = m_pAcceleratorTable->getItemInfo(i)))
		{
			if(LOWORD(wParam) != pAccel->uKey)
			{
				continue;
			}

			bool isFound = false;
			if(msg == WM_CHAR || msg == WM_SYSCHAR)
			{
				if(!(pAccel->flags & XAF_VIRTKEY) && ((mask & XAF_ALT) == (pAccel->flags & XAF_ALT)))
				{
					isFound = true;
				}
			}
			else
			{
				if(pAccel->flags & XAF_VIRTKEY)
				{
					if(mask == (pAccel->flags & (XAF_SHIFT | XAF_CTRL | XAF_ALT)))
					{
						isFound = true;
					}
				}
				else if((pAccel->flags & XAF_ALT) && (lParam & 0x20000000)) /* ALT pressed */
				{
					isFound = true;
				}
			}

			if(isFound)
			{
				execCommand(m_pAcceleratorTable->getItemCommand(i));
				return(true);
			}
		}
	}

	return(false);
}

void CUIWindow::storePlacement()
{
	if(m_bMaintainPlacement)
	{
		// 1. query placement from m_pXWindow;
		XWindowPlacement placement;
		if(m_pXWindow->getPlacement(&placement))
		{
			// 2. store placement somewhere
			m_pXUI->storeWindowPlacement(m_guidPlacement, placement);
		}
	}
}
void CUIWindow::restorePlacement()
{
	if(m_bMaintainPlacement)
	{
		// 1. query placement from storage;
		XWindowPlacement placement;
		if(m_pXUI->loadWindowPlacement(m_guidPlacement, &placement))
		{
			// 2. set placement to m_pXWindow
			m_pXWindow->setPlacement(placement, !m_bMaintainVisibility);
		}
	}
}
