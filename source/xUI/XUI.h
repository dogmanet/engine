#ifndef __XUI_H 
#define __XUI_H

#include "IXUI.h"
#include <graphix/graphix.h>
#include <gui/guimain.h>

class CUIWindow;
class CXUI: public IXUnknownImplementation<IXUI>
{
public:
	CXUI(IGXDevice *pDev, IXWindowSystem *pWindowSystem, gui::IGUI *pGUI);

	IUIWindow* XMETHODCALLTYPE createWindow(const XWINDOW_DESC *pWindowDesc, IUIWindow *pParent = NULL) override;

	IUIButton* XMETHODCALLTYPE createButton() override;

	IUITextBox* XMETHODCALLTYPE createTextBox() override;

	IUIComboBox* XMETHODCALLTYPE createComboBox() override;

	IUICheckbox* XMETHODCALLTYPE createCheckBox() override;

	//IUIPicture* XMETHODCALLTYPE createPicture() override;

	IUIPanel* XMETHODCALLTYPE createPanel() override;

	IUISpoiler* XMETHODCALLTYPE createSpoiler() override;

	void onDestroyWindow(CUIWindow *pWindow);

	IXWindowSystem* getWindowSystem();
	gui::IGUI* getGUI();
	IGXDevice* getGXDevice();

	void XMETHODCALLTYPE render() override;
	void XMETHODCALLTYPE present() override;

private:
	Array<CUIWindow*> m_pWindows;

	IGXDevice *m_pDev = NULL;
	IXWindowSystem *m_pWindowSystem = NULL;
	gui::IGUI *m_pGUI = NULL;

	ULONG m_elemendID = 0;
};

#endif
