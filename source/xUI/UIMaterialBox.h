#ifndef __UIMATERIALBPX_H
#define __UIMATERIALBPX_H

#include "UIControl.h"
#include "IUIMaterialBox.h"
#include <xcommon/IXCore.h>
#include <xcommon/editor/IXEditorMaterialBrowser.h>

class CUIMaterialBox;
class CMaterialBrowserCallback final: public IXEditorMaterialBrowserCallback
{
public:
	CMaterialBrowserCallback(CUIMaterialBox *pControl);

	void XMETHODCALLTYPE onSelected(const char *szName) override;
	void XMETHODCALLTYPE onCancel() override;
private:
	CUIMaterialBox *m_pControl = NULL;
};

//##########################################################################

class CUIMaterialBox final: public CUIControl<IUIMaterialBox>
{
	friend class CMaterialBrowserCallback;
public:
	CUIMaterialBox(IXCore *pCore, IXRender *pRender, ULONG uID);
	~CUIMaterialBox();

	gui::dom::IDOMnode* createNode(gui::dom::IDOMdocument *pDomDocument) override;

	void XMETHODCALLTYPE setLabel(const char *szTitle) override;

	void XMETHODCALLTYPE setValue(const char *szValue) override;
	const char* XMETHODCALLTYPE getValue() override;

	void XMETHODCALLTYPE dispatchEvent(gui::IEvent *ev) override;

	void XMETHODCALLTYPE setEnabled(bool bEnable) override;

private:
	IXCore *m_pCore = NULL;

	gui::dom::IDOMnode *m_pLabel = NULL;

	gui::dom::IDOMnode *m_pTextbox = NULL;

	gui::dom::IDOMnode *m_pMaterialPreview = NULL;


	IXEditorMaterialBrowser *m_pMaterialBrowser = NULL;
	bool m_isEditing = false;
	CMaterialBrowserCallback m_browseCallback;

	IXMaterialSystem *m_pMaterialSystem = NULL;

private:
	void startEdit();
	void abortEdit();
	void onEditAccept(const char *szName);
	void onEditCancel();

	void updatePeview();

	void cleanupNodes() override;
};

#endif
