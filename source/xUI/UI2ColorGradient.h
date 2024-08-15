#ifndef __UI2COLORGRADIENT_H
#define __UI2COLORGRADIENT_H

#include "UIControl.h"
#include "IUI2ColorGradient.h"
#include <common/2ColorGradients.h>
#include <xcommon/IXCore.h>
#include <xcommon/editor/IXColorGradientEditor.h>
#include <xcommon/editor/IXColorPicker.h>

class CUI2ColorGradient;
class CGradientEditorCallback final: public IXColorGradientEditorCallback
{
public:
	CGradientEditorCallback(CUI2ColorGradient *pControl);

	void XMETHODCALLTYPE onAccept() override;
	void XMETHODCALLTYPE onCancel() override;
private:
	CUI2ColorGradient *m_pControl = NULL;
};

//##########################################################################

class CColorPickerCallback final: public IXColorPickerCallback
{
public:
	CColorPickerCallback(CUI2ColorGradient *pControl);

	void XMETHODCALLTYPE onAccept(const float4_t &vColor) override;
	void XMETHODCALLTYPE onPreview(const float4_t &vColor) override;
	void XMETHODCALLTYPE onCancel(const float4_t &vStartColor) override;
private:
	CUI2ColorGradient *m_pControl = NULL;
};

//##########################################################################

class CUI2ColorGradient final: public CUIControl<IUI2ColorGradient>
{
	friend class CGradientEditorCallback;
	friend class CColorPickerCallback;
public:
	CUI2ColorGradient(IXCore *pCore, IXRender *pRender, ULONG uID);
	~CUI2ColorGradient();

	gui::dom::IDOMnode* createNode(gui::dom::IDOMdocument *pDomDocument) override;

	void XMETHODCALLTYPE setLabel(const char *szTitle) override;

	//void XMETHODCALLTYPE setValue(const char *szValue) override;
	//const char* XMETHODCALLTYPE getValue() override;

	void XMETHODCALLTYPE dispatchEvent(gui::IEvent *ev) override;

	void XMETHODCALLTYPE setEnabled(bool bEnable) override;

	void XMETHODCALLTYPE setGradient(IX2ColorGradients *pGradient) override;

	void XMETHODCALLTYPE setVisible(bool isVisible) override;

protected:
	void addItem(const char *szName, const char *szValue);

private:
	IXCore *m_pCore = NULL;

	gui::dom::IDOMnode *m_pLabel = NULL;


	gui::dom::IDOMnode *m_pColor0Preview = NULL;
	gui::dom::IDOMnode *m_pColor0Textbox = NULL;
	gui::dom::IDOMnode *m_pColor1Preview = NULL;
	gui::dom::IDOMnode *m_pColor1Textbox = NULL;

	gui::dom::IDOMnode *m_pGradientPreview = NULL;

	void cleanupNodes() override;

	IXRender *m_pRender = NULL;
	IXCamera *m_pCamera = NULL;
	IXRenderTarget *m_pRenderTarget = NULL;
	IXRenderGraph *m_pRenderGraph = NULL;
	UINT m_uRenderWidth = 0;
	UINT m_uRenderHeight = 0;
	char m_szTextureName[64];

	IXColorGradientEditor *m_pGradientEditor = NULL;
	IXColorPicker *m_pColorPicker = NULL;
	C2ColorGradients m_gradient;
	IX2ColorGradients *m_pGradient = &m_gradient;
	CGradientEditorCallback m_editCallback;
	CColorPickerCallback m_editColorCallback;
	int m_iColorEditingPart = 0;
	bool m_isEditing = false;
	bool m_isEditingColor = false;

private:
	void startEdit(int iPart);
	void abortEdit();
	void onEditAccept();
	void onEditCancel();

	void updatePeview();

	void setMode(X2COLOR_GRADIENT_MODE mode);

	void setColor(int idx, const float4_t &vColor, bool bTriggerChange = false, bool bUpdateText = true);

	void startEditColor(int iPart);
	void abortEditColor();
	void onColorEditAccept(const float4_t &vColor);
	void onColorEditPreview(const float4_t &vColor);
	void onColorEditCancel(const float4_t &vStartColor);
};

#endif
