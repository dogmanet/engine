#ifndef __IXUI_H 
#define __IXUI_H

#include "IUIWindow.h"
#include "IUIButton.h"
#include "IUITextBox.h"
#include "IUIComboBox.h"
#include "IUICheckbox.h"
#include "IUIPicture.h"
#include "IUIPanel.h"
#include "IUISpoiler.h"
#include "IUIViewport.h"
#include "IUIGrid.h"
#include "IUIMinMaxCurve.h"
#include "IUIColor.h"
#include "IUIMultiTrackbar.h"
#include "IUI2ColorGradient.h"
#include "IUIMaterialBox.h"
#include <gui/guimain.h>

// {D3FE7B60-BDE7-4A3F-95D8-2D6A102A42E2}
#define IXUI_GUID DEFINE_XGUID(0xd3fe7b60, 0xbde7, 0x4a3f, 0x95, 0xd8, 0x2d, 0x6a, 0x10, 0x2a, 0x42, 0xe2)

class IXUI: public IXUnknown
{
public:
	virtual IUIWindow* XMETHODCALLTYPE createWindow(const XWINDOW_DESC *pWindowDesc, IUIWindow *pParent = NULL) = 0;

	virtual IUIButton* XMETHODCALLTYPE createButton() = 0;

	virtual IUITextBox* XMETHODCALLTYPE createTextBox() = 0;

	virtual IUIComboBox* XMETHODCALLTYPE createComboBox() = 0;

	virtual IUICheckbox* XMETHODCALLTYPE createCheckBox() = 0;

	//virtual IUIPicture* XMETHODCALLTYPE createPicture() = 0;

	virtual IUIPanel* XMETHODCALLTYPE createPanel() = 0;

	virtual IUISpoiler* XMETHODCALLTYPE createSpoiler() = 0;

	virtual IUIViewport* XMETHODCALLTYPE createViewport() = 0;

	virtual IUIGrid* XMETHODCALLTYPE createGrid() = 0;

	virtual IUIColor* XMETHODCALLTYPE createColor() = 0;

	virtual IUIMultiTrackbar* XMETHODCALLTYPE createMultiTrackbar() = 0;

	virtual IUIMinMaxCurve* XMETHODCALLTYPE createMinMaxCurve() = 0;

	virtual IUI2ColorGradient* XMETHODCALLTYPE create2ColorGradient() = 0;

	virtual IUIMaterialBox* XMETHODCALLTYPE createMaterialBox() = 0;

	virtual void XMETHODCALLTYPE render() = 0;
	virtual void XMETHODCALLTYPE present() = 0;
};

// typedef IXUI*(*PFNXUIINIT)(IXRender*, IXWindowSystem*, gui::IGUI*);

#endif
