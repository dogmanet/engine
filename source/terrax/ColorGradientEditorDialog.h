#ifndef __COLOR_GRADIENT_EDITOR_DIALOG_H
#define __COLOR_GRADIENT_EDITOR_DIALOG_H


#include <xUI/IXUI.h>
#include <xcommon/IXCore.h>

#include <xcommon/editor/IXColorGradientEditor.h>
#include <common/ColorGradient.h>

class CColorGradientEditorDialog final: public IXUnknownImplementation<IXColorGradientEditor>
{
private:
	static void EventProc(void *pCtx, IUIControl *pControl, gui::IEvent *ev);
	void eventProc(IUIControl *pControl, gui::IEvent *ev);

public:
	CColorGradientEditorDialog(HWND hMainWnd, IXCore *pCore);
	~CColorGradientEditorDialog();

	void XMETHODCALLTYPE edit(IXColorGradient *pGradient, IXColorGradientEditorCallback *pCallback) override;
	void XMETHODCALLTYPE abort() override;

private:
	HWND m_hMainWnd = NULL;
	IUIWindow *m_pWindow = NULL;
	IXCore *m_pCore = NULL;
	IXUI *m_pXUI = NULL;

	IXCamera *m_pCamera = NULL;
	IUIViewport *m_pViewport = NULL;
	IUIButton *m_pOkButton = NULL;
	IUIButton *m_pCancelButton = NULL;
	IUIMultiTrackbar *m_pTopTrackbar = NULL;
	IUIMultiTrackbar *m_pBottomTrackbar = NULL;
	IUIColor *m_pColorInput = NULL;
	IUITextBox *m_pAlphaInput = NULL;

	//CColorGradient m_gradient;
	CColorGradient m_gradientBackup;
	IXColorGradient *m_pGradient = NULL; // &m_gradient;

	IXRenderTarget *m_pRenderTarget = NULL;
	IXRenderGraph *m_pRenderGraph = NULL;

	IXColorGradientEditorCallback *m_pCallback = NULL;

private:
	void onResize();

	void updatePeview();

	void syncColors();
	void syncAlphas();

	void callAccept();
	void callAbort();
};

#endif
