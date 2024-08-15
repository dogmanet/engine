#ifndef __XUI_H 
#define __XUI_H

#include "IXUI.h"
#include <graphix/graphix.h>
#include <gui/guimain.h>
#include <xcommon/IXCore.h>

class CUIWindow;
class CXUI: public IXUnknownImplementation<IXUI>
{
public:
	CXUI(IXCore *pCore, IXRender *pRender, IXWindowSystem *pWindowSystem, gui::IGUI *pGUI);

	IUIWindow* XMETHODCALLTYPE createWindow(const XWINDOW_DESC *pWindowDesc, IUIWindow *pParent = NULL) override;

	IUIButton* XMETHODCALLTYPE createButton() override;

	IUITextBox* XMETHODCALLTYPE createTextBox() override;

	IUIComboBox* XMETHODCALLTYPE createComboBox() override;

	IUICheckbox* XMETHODCALLTYPE createCheckBox() override;

	//IUIPicture* XMETHODCALLTYPE createPicture() override;

	IUIPanel* XMETHODCALLTYPE createPanel() override;

	IUISpoiler* XMETHODCALLTYPE createSpoiler() override;

	IUIViewport* XMETHODCALLTYPE createViewport() override;

	IUIGrid* XMETHODCALLTYPE createGrid() override;

	IUIMinMaxCurve* XMETHODCALLTYPE createMinMaxCurve() override;

	IUIColor* XMETHODCALLTYPE createColor() override;

	IUIMultiTrackbar* XMETHODCALLTYPE createMultiTrackbar() override;

	IUI2ColorGradient* XMETHODCALLTYPE create2ColorGradient() override;

	IUIMaterialBox* XMETHODCALLTYPE createMaterialBox() override;

	void onDestroyWindow(CUIWindow *pWindow);

	IXWindowSystem* getWindowSystem();
	gui::IGUI* getGUI();
	IGXDevice* getGXDevice();

	void XMETHODCALLTYPE render() override;
	void XMETHODCALLTYPE present() override;

private:
	IXCore *m_pCore = NULL;

	Array<CUIWindow*> m_pWindows;

	IXRender *m_pRender = NULL;
	IXWindowSystem *m_pWindowSystem = NULL;
	gui::IGUI *m_pGUI = NULL;

	ULONG m_elemendID = 0;
};

#endif
