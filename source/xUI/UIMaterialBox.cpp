#include "UIMaterialBox.h"
#include <xcommon/IPluginManager.h>
#include <xcommon/editor/IXEditor.h>

CUIMaterialBox::CUIMaterialBox(IXCore *pCore, IXRender *pRender, ULONG uID):
	BaseClass(uID, "div"),
	m_pCore(pCore),
	m_browseCallback(this)
{
}

CUIMaterialBox::~CUIMaterialBox()
{
	abortEdit();
}

gui::dom::IDOMnode* CUIMaterialBox::createNode(gui::dom::IDOMdocument *pDomDocument)
{
	//m_pNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	//m_pNode->setAttribute(L"controld_id", StringW(m_id));
	//return(m_pNode);

	m_pNode = pDomDocument->createNode(L"div");
	m_pNode->classAdd(L"combobox-wrapper materialbox-wrapper");
	m_pNode->setAttribute(L"onchange", L"handler");
	m_pNode->setUserData(this);
	m_pLabel = pDomDocument->createNode(L"label");
	m_pInputNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	m_pInputNode->setAttribute(L"onchange", L"handler");
	m_pInputNode->setAttribute(L"onclick", L"handler");
	m_pInputNode->setUserData(this);
	m_pInputNode->classAdd(L"-combo-like");

	StringW wsId = StringW(L"select_") + StringW(m_id);

	m_pInputNode->setAttribute(L"id", StringW(m_id));
	m_pLabel->setAttribute(L"for", StringW(m_id));
	
	m_pNode->appendChild(m_pLabel, true);
	m_pNode->appendChild(m_pInputNode, true);

	m_pTextbox = pDomDocument->createNode(L"input");
	m_pTextbox->classAdd(L"inner");
	m_pTextbox->setAttribute(L"onchange", L"handler");
	m_pTextbox->setAttribute(L"onkeyup", L"handler");
	m_pTextbox->setUserData(this);
	m_pInputNode->appendChild(m_pTextbox);

	m_pMaterialPreview = pDomDocument->createNode(L"div");
	m_pMaterialPreview->classAdd(L"inner");
	m_pMaterialPreview->classAdd(L"preview");
	m_pMaterialPreview->setAttribute(L"onclick", L"handler");
	m_pMaterialPreview->setAttribute(L"onlayout", L"handler");
	m_pMaterialPreview->getStyleSelf()->background_image->set(L"!dev_null");
	m_pMaterialPreview->setUserData(this);
	m_pInputNode->appendChild(m_pMaterialPreview);
	
	return(m_pNode);
}

void XMETHODCALLTYPE CUIMaterialBox::setLabel(const char *szTitle)
{
	m_pLabel->setText(StringW(CMB2WC(szTitle)), true);
}

void CUIMaterialBox::cleanupNodes()
{
	BaseClass::cleanupNodes();
	m_pLabel = NULL;

	m_pTextbox = NULL;
	m_pMaterialPreview = NULL;
}

void XMETHODCALLTYPE CUIMaterialBox::setValue(const char *szValue)
{
	m_pTextbox->setText(StringW(CMB2WC(szValue)), TRUE);
	updatePeview();
}
const char* XMETHODCALLTYPE CUIMaterialBox::getValue()
{
	m_sValue = m_pTextbox->getText();
	return(m_sValue.c_str());
}

void CUIMaterialBox::dispatchEvent(gui::IEvent *ev)
{
	if(ev->type == gui::GUI_EVENT_TYPE_CLICK && ev->target == m_pInputNode)
	{
		startEdit();
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_KEYUP)
	{
		updatePeview();
	}
}

void CUIMaterialBox::startEdit()
{
	if(!m_pMaterialBrowser)
	{
		IXEditor *pEditor = (IXEditor*)m_pCore->getPluginManager()->getInterface(IXEDITOR_GUID);
		if(pEditor)
		{
			m_pMaterialBrowser = pEditor->getMaterialBrowser();
		}
	}

	if(!m_pMaterialBrowser)
	{
		LogError("Interface IXEditorMaterialBrowser is not available!\n");
		return;
	}

	m_pMaterialBrowser->browse(&m_browseCallback);
	m_isEditing = true;
}

void CUIMaterialBox::abortEdit()
{
	if(m_isEditing)
	{
		m_pMaterialBrowser->abort();
		m_isEditing = false;
	}
}

void CUIMaterialBox::onEditAccept(const char *szName)
{
	m_isEditing = false;
	setValue(szName);
	
	gui::IEvent ev;
	ev.type = gui::GUI_EVENT_TYPE_CHANGE;
	ev.target = m_pNode;
	ev.relatedTarget = m_pTextbox;
	m_pNode->dispatchEvent(ev);
}
void CUIMaterialBox::onEditCancel()
{
	m_isEditing = false;
	updatePeview();
}

void CUIMaterialBox::updatePeview()
{
	if(!m_pMaterialSystem)
	{
		m_pMaterialSystem = (IXMaterialSystem*)m_pCore->getPluginManager()->getInterface(IXMATERIALSYSTEM_GUID);
		if(!m_pMaterialSystem)
		{
			return;
		}
	}

	IXMaterial *pMaterial = NULL;
	m_pMaterialSystem->loadMaterial(getValue(), &pMaterial);
	
	IKeyIterator *pIter = NULL;
	const char *szTexture = pMaterial->getTextureName("txBase");
	if(!szTexture)
	{
		pIter = pMaterial->getTexturesIterator();
		if(pIter)
		{
			szTexture = pIter->getCurrent();
		}
	}
	m_pMaterialPreview->getStyleSelf()->background_image->set(StringW(L"!") + CMB2WC(szTexture));
	mem_release(pIter);
	mem_release(pMaterial);

	m_pMaterialPreview->updateStyles();
}

void XMETHODCALLTYPE CUIMaterialBox::setEnabled(bool bEnable)
{
	BaseClass::setEnabled(bEnable);

	if(bEnable)
	{
		m_pTextbox->removePseudoclass(0x00004);
		m_pMaterialPreview->removePseudoclass(0x00004);
	}
	else
	{
		m_pTextbox->addPseudoclass(0x00004);
		m_pMaterialPreview->addPseudoclass(0x00004);
	}
}

//##########################################################################

CMaterialBrowserCallback::CMaterialBrowserCallback(CUIMaterialBox *pControl):
	m_pControl(pControl)
{
}

void XMETHODCALLTYPE CMaterialBrowserCallback::onSelected(const char *szName)
{
	m_pControl->onEditAccept(szName);
}
void XMETHODCALLTYPE CMaterialBrowserCallback::onCancel()
{
	m_pControl->onEditCancel();
}
