#include "XUI.h"
#include "UIWindow.h"

#include "UIButton.h"
#include "UITextBox.h"
#include "UIComboBox.h"
#include "UICheckbox.h"
#include "UIPicture.h"
#include "UIPanel.h"
#include "UISpoiler.h"
#include "UIViewport.h"
#include "UIGrid.h"
#include "UIMinMaxCurve.h"
#include "UIColor.h"
#include "UIMultiTrackbar.h"
#include "UI2ColorGradient.h"
#include "UIMaterialBox.h"
#include "UITree.h"
#include "UIMenu.h"

#include "UIAcceleratorTable.h"

//#include <core/sxcore.h>

CXUI::CXUI(IXCore *pCore, IXRender *pRender, IXWindowSystem *pWindowSystem, gui::IGUI *pGUI):
	m_pCore(pCore),
	m_pRender(pRender),
	m_pWindowSystem(pWindowSystem),
	m_pGUI(pGUI)
{
}

IUIWindow* XMETHODCALLTYPE CXUI::createWindow(const XWINDOW_DESC *pWindowDesc, IUIWindow *pParent)
{
	CUIWindow *pWindow = new CUIWindow(this, pWindowDesc, pParent);
	m_pWindows.push_back(pWindow);
	return(pWindow);
}

IUIButton* XMETHODCALLTYPE CXUI::createButton()
{
	return(new CUIButton(++m_elemendID));
}

IUITextBox* XMETHODCALLTYPE CXUI::createTextBox()
{
	return(new CUITextBox(++m_elemendID));
}

IUIComboBox* XMETHODCALLTYPE CXUI::createComboBox()
{
	return(new CUIComboBox(++m_elemendID));
}

IUICheckbox* XMETHODCALLTYPE CXUI::createCheckBox()
{
	return(new CUICheckBox(++m_elemendID));
}

/*IUIPicture* XMETHODCALLTYPE CXUI::createPicture()
{
	return(new CUIPicture(++m_elemendID));
}*/

IUIPanel* XMETHODCALLTYPE CXUI::createPanel()
{
	return(new CUIPanel(++m_elemendID));
}

IUISpoiler* XMETHODCALLTYPE CXUI::createSpoiler()
{
	return(new CUISpoiler(++m_elemendID));
}

IUIViewport* XMETHODCALLTYPE CXUI::createViewport()
{
	return(new CUIViewport(m_pRender, ++m_elemendID));
}

IUIGrid* XMETHODCALLTYPE CXUI::createGrid()
{
	return(new CUIGrid(++m_elemendID));
}

IUIMinMaxCurve* XMETHODCALLTYPE CXUI::createMinMaxCurve()
{
	return(new CUIMinMaxCurve(m_pCore, m_pRender, ++m_elemendID));
}

IUIColor* XMETHODCALLTYPE CXUI::createColor()
{
	return(new CUIColor(++m_elemendID));
}

IUIMultiTrackbar* XMETHODCALLTYPE CXUI::createMultiTrackbar()
{
	return(new CUIMultiTrackbar(++m_elemendID));
}

IUI2ColorGradient* XMETHODCALLTYPE CXUI::create2ColorGradient()
{
	return(new CUI2ColorGradient(m_pCore, m_pRender, ++m_elemendID));
}

IUIMaterialBox* XMETHODCALLTYPE CXUI::createMaterialBox()
{
	return(new CUIMaterialBox(m_pCore, m_pRender, ++m_elemendID));
}

IUITree* XMETHODCALLTYPE CXUI::createTree()
{
	return(new CUITree(++m_elemendID));
}

IUIMenu* XMETHODCALLTYPE CXUI::createMenu()
{
	return(new CUIMenu(++m_elemendID));
}

void XMETHODCALLTYPE CXUI::createAcceleratorTable(IUIAcceleratorTable **ppOut)
{
	*ppOut = new CUIAcceleratorTable();
}

void CXUI::onDestroyWindow(CUIWindow *pWindow)
{
	for(UINT i = 0, l = m_pWindows.size(); i < l; ++i)
	{
		if(m_pWindows[i] == pWindow)
		{
			m_pWindows.erase(i);
			break;
		}
	}
}

IXWindowSystem* CXUI::getWindowSystem()
{
	return(m_pWindowSystem);
}
gui::IGUI* CXUI::getGUI()
{
	return(m_pGUI);
}
IGXDevice* CXUI::getGXDevice()
{
	return(m_pRender->getDevice());
}

void XMETHODCALLTYPE CXUI::update()
{
	for(UINT i = 0, l = m_pWindows.size(); i < l; ++i)
	{
		if(m_pWindows[i]->isVisible())
		{
			m_pWindows[i]->update();
		}
	}
}

void XMETHODCALLTYPE CXUI::render()
{
	IGXContext *pCtx = m_pRender->getDevice()->getThreadContext();

	IGXSurface *pOldSurface = pCtx->getColorTarget();
	IGXDepthStencilSurface *pOldDS = pCtx->getDepthStencilSurface();

	pCtx->setDepthStencilSurface(NULL);

	for(UINT i = 0, l = m_pWindows.size(); i < l; ++i)
	{
		if(m_pWindows[i]->isVisible())
		{
			m_pWindows[i]->render(pCtx);
		}
	}

	pCtx->setColorTarget(pOldSurface);
	pCtx->setDepthStencilSurface(pOldDS);
	mem_release(pOldSurface);
	mem_release(pOldDS);
}
void XMETHODCALLTYPE CXUI::present()
{
	for(UINT i = 0, l = m_pWindows.size(); i < l; ++i)
	{
		if(m_pWindows[i]->isVisible())
		{
			m_pWindows[i]->present();
		}
	}
}

void CXUI::storeWindowPlacement(const XGUID &guid, const XWindowPlacement &placement)
{
	TODO("Use files!");
	//IFileSystem *pFS = m_pCore->getFileSystem();
	char szGuid[39];
	XGUIDToSting(guid, szGuid, sizeof(szGuid));

	char szKey[512];
	sprintf(szKey, "SOFTWARE\\DogmaNet\\TerraX\\Windows\\%s", szGuid);

	HKEY hKey;
	if(ERROR_SUCCESS == RegCreateKeyExA(HKEY_CURRENT_USER, szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_QUERY_VALUE, NULL, &hKey, NULL))
	{
		RegSetValueExA(hKey, "WinPos", 0, REG_BINARY, (BYTE*)&placement, sizeof(XWindowPlacement));
	}
	RegCloseKey(hKey);
}
bool CXUI::loadWindowPlacement(const XGUID &guid, XWindowPlacement *pPlacement)
{
	char szGuid[39];
	XGUIDToSting(guid, szGuid, sizeof(szGuid));

	char szKey[512];
	sprintf(szKey, "SOFTWARE\\DogmaNet\\TerraX\\Windows\\%s", szGuid);

	DWORD dwKeyLen = sizeof(XWindowPlacement);
	if(ERROR_SUCCESS == RegGetValueA(HKEY_CURRENT_USER, szKey, "WinPos", RRF_RT_REG_BINARY, NULL, (BYTE*)pPlacement, &dwKeyLen) && pPlacement->length == sizeof(XWindowPlacement))
	{
		return(true);
	}
	return(false);
}

//##########################################################################
#if 0
EXTERN_C __declspec(dllexport) IXUI* InitInstance(IXRender *pRender, IXWindowSystem *pWindowSystem, gui::IGUI *pGUI)
{
	Core_SetOutPtr();

	return(new CXUI(pRender, pWindowSystem, pGUI));
}

DECLARE_PROFILER_INTERNAL();
#endif
