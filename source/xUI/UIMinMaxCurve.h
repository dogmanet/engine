#ifndef __UIMINMAXCURVE_H
#define __UIMINMAXCURVE_H

#include "UIControl.h"
#include "IUIMinMaxCurve.h"
#include <common/MinMaxCurve.h>
#include <xcommon/IXCore.h>
#include <xcommon/editor/IXCurveEditor.h>

class CUIMinMaxCurve;
class CCurveEditorCallback final: public IXCurveEditorCallback
{
public:
	CCurveEditorCallback(CUIMinMaxCurve *pControl);

	void XMETHODCALLTYPE onAccept() override;
	void XMETHODCALLTYPE onCancel() override;
private:
	CUIMinMaxCurve *m_pControl = NULL;
};

//##########################################################################

class CUIMinMaxCurve final: public CUIControl<IUIMinMaxCurve>
{
	friend class CCurveEditorCallback;
public:
	CUIMinMaxCurve(IXCore *pCore, IXRender *pRender, ULONG uID);
	~CUIMinMaxCurve();

	gui::dom::IDOMnode* createNode(gui::dom::IDOMdocument *pDomDocument) override;

	void XMETHODCALLTYPE setLabel(const char *szTitle) override;

	//void XMETHODCALLTYPE setValue(const char *szValue) override;
	//const char* XMETHODCALLTYPE getValue() override;

	void XMETHODCALLTYPE dispatchEvent(gui::IEvent *ev) override;

	void XMETHODCALLTYPE setEnabled(bool bEnable) override;

	void XMETHODCALLTYPE setCurve(IXMinMaxCurve *pCurve) override;

	void XMETHODCALLTYPE setVisible(bool isVisible) override;

protected:
	void addItem(const char *szName, const char *szValue);

private:
	IXCore *m_pCore = NULL;

	gui::dom::IDOMnode *m_pLabel = NULL;


	gui::dom::IDOMnode *m_pConstantTextbox = NULL;

	gui::dom::IDOMnode *m_pTwoConstantsTextbox0 = NULL;
	gui::dom::IDOMnode *m_pTwoConstantsTextbox1 = NULL;

	gui::dom::IDOMnode *m_pCurvePreview = NULL;

	void cleanupNodes() override;

	IXRender *m_pRender = NULL;
	IXCamera *m_pCamera = NULL;
	IXRenderTarget *m_pRenderTarget = NULL;
	IXRenderGraph *m_pRenderGraph = NULL;
	UINT m_uRenderWidth = 0;
	UINT m_uRenderHeight = 0;
	char m_szTextureName[64];

	IXCurveEditor *m_pCurveEditor = NULL;
	bool m_isEditing = false;
	CMinMaxCurve m_curve;
	IXMinMaxCurve *m_pCurve = &m_curve;
	CCurveEditorCallback m_editCallback;

private:
	void startEdit();
	void abortEdit();
	void onEditAccept();
	void onEditCancel();

	void updatePeview();

	void setMode(XMINMAX_CURVE_MODE mode);
};

#endif
