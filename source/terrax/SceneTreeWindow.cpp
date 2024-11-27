#include <xcommon/IPluginManager.h>
#include "SceneTreeWindow.h"
#include "Editor.h"
#include "terrax.h"
#include "CommandSelect.h"
#include "CommandProperties.h"
#include "resource.h"

// {F60AF558-C50B-40FC-8808-85943BC2B421}
#define CSCENETREEWINDOW_GUID DEFINE_XGUID(0xf60af558, 0xc50b, 0x40fc, 0x88, 0x8, 0x85, 0x94, 0x3b, 0xc2, 0xb4, 0x21)

CSceneTreeWindow::CSceneTreeWindow(CEditor *pEditor, IXCore *pCore):
	m_hMainWnd((HWND)pEditor->getMainWindow()),
	m_pCore(pCore),
	m_pEditor(pEditor)
{
	m_pXUI = (IXUI*)pCore->getPluginManager()->getInterface(IXUI_GUID);

	XWINDOW_DESC wdesc;
	wdesc.iPosX = XCW_USEDEFAULT;
	wdesc.iPosY = XCW_USEDEFAULT;
	wdesc.iSizeX = 300;
	wdesc.iSizeY = 600;
	wdesc.szTitle = "Scene tree";
	wdesc.flags = XWF_BUTTON_CLOSE | XWF_BUTTON_MAXIMIZE | XWF_TRANSPARENT | XWF_INIT_HIDDEN;

	
	m_pWindow = m_pXUI->createWindow(&wdesc, (IUIWindow*)m_hMainWnd); // temporary hack!
	m_pWindow->setCallback(EventProc, this);

	m_pTree = m_pXUI->createTree();
	m_pWindow->insertChild(m_pTree);
	m_pTree->setAdapter(&m_treeAdapter);
	m_treeAdapter.setTree(m_pTree);
	m_treeAdapterFiltered.setTree(m_pTree);

	m_pFilter = m_pXUI->createTextBox();
	m_pWindow->insertChild(m_pFilter);
	//m_pFilter->setLabel("Filter");

	m_pTreeMenu = m_pXUI->createMenu();
	m_pTreeMenu->addItem("Rename\tF2", "rename");
	m_pTreeMenu->addItem("Cut\tCtrl+X", "cut");
	m_pTreeMenu->addItem("Copy\tCtrl+C", "copy");
	m_pTreeMenu->addItem("Delete\tDel", "delete");
	m_pTreeMenu->addSeparator();
	m_pTreeMenu->addItem("To Object\tCtrl+T", "to_object");
	m_pTreeMenu->addItem("To World\tCtrl+Shift+T", "to_world");
	m_pTreeMenu->addSeparator();
	m_pTreeMenu->addItem("Properties\tAlt+Enter", "properties");
	m_pWindow->insertChild(m_pTreeMenu);

	/*
	m_pEmitterAddButton = m_pXUI->createButton();
	m_pWindow->insertChild(m_pEmitterAddButton);
	m_pEmitterAddButton->setLabel("Add emitter");
	*/
	
	IUIAcceleratorTable *pAccelTable = NULL;
	m_pXUI->createAcceleratorTable(&pAccelTable);
	pAccelTable->addItem({XAF_CTRL | XAF_VIRTKEY, KEY_X}, "cut");
	pAccelTable->addItem({XAF_SHIFT | XAF_VIRTKEY, KEY_DELETE}, "cut");
	pAccelTable->addItem({XAF_CTRL | XAF_VIRTKEY, KEY_C}, "copy");
	pAccelTable->addItem({XAF_CTRL | XAF_VIRTKEY, KEY_INSERT}, "copy");
	pAccelTable->addItem({XAF_VIRTKEY, KEY_DELETE}, "delete");
	pAccelTable->addItem({XAF_CTRL | XAF_VIRTKEY, KEY_V}, "paste");
	pAccelTable->addItem({XAF_SHIFT | XAF_VIRTKEY, KEY_INSERT}, "paste");
	pAccelTable->addItem({XAF_CTRL | XAF_VIRTKEY, KEY_T}, "to_object");
	pAccelTable->addItem({XAF_CTRL | XAF_SHIFT | XAF_VIRTKEY, KEY_T}, "to_world");
	pAccelTable->addItem({XAF_ALT | XAF_VIRTKEY, KEY_ENTER}, "properties");
	pAccelTable->addItem({XAF_SHIFT | XAF_VIRTKEY, KEY_Q}, "clear_selection");
	pAccelTable->addItem({XAF_CTRL | XAF_VIRTKEY, KEY_Z}, "undo");
	pAccelTable->addItem({XAF_CTRL | XAF_VIRTKEY, KEY_Y}, "redo");
	pAccelTable->addItem({XAF_CTRL | XAF_SHIFT | XAF_VIRTKEY, KEY_Z}, "redo");
	pAccelTable->addItem({XAF_VIRTKEY, KEY_F2}, "rename");
	pAccelTable->addItem({XAF_CTRL | XAF_VIRTKEY, KEY_E}, "center_on_selection");
	pAccelTable->addItem({XAF_CTRL | XAF_SHIFT | XAF_VIRTKEY, KEY_E}, "go_to_selection");
	m_pWindow->setAcceleratorTable(pAccelTable);
	mem_release(pAccelTable);

	m_pWindow->addCommand("cut", [](void *pCtx){
		((CSceneTreeWindow*)pCtx)->sendParentCommand(ID_EDIT_CUT);
	}, this);
	m_pWindow->addCommand("copy", [](void *pCtx){
		((CSceneTreeWindow*)pCtx)->sendParentCommand(ID_EDIT_COPY);
	}, this);
	m_pWindow->addCommand("delete", [](void *pCtx){
		((CSceneTreeWindow*)pCtx)->sendParentCommand(ID_EDIT_DELETE);
	}, this);
	m_pWindow->addCommand("paste", [](void *pCtx){
		((CSceneTreeWindow*)pCtx)->sendParentCommand(ID_EDIT_PASTE);
	}, this);
	m_pWindow->addCommand("to_object", [](void *pCtx){
		((CSceneTreeWindow*)pCtx)->sendParentCommand(IDC_TIE_TO_OBJECT);
	}, this);
	m_pWindow->addCommand("to_world", [](void *pCtx){
		((CSceneTreeWindow*)pCtx)->sendParentCommand(IDC_BACK_TO_WORLD);
	}, this);
	m_pWindow->addCommand("properties", [](void *pCtx){
		((CSceneTreeWindow*)pCtx)->sendParentCommand(ID_EDIT_PROPERTIES);
	}, this);
	m_pWindow->addCommand("clear_selection", [](void *pCtx){
		((CSceneTreeWindow*)pCtx)->sendParentCommand(ID_EDIT_CLEARSELECTION);
	}, this);
	m_pWindow->addCommand("undo", [](void *pCtx){
		((CSceneTreeWindow*)pCtx)->sendParentCommand(ID_EDIT_UNDO);
	}, this);
	m_pWindow->addCommand("redo", [](void *pCtx){
		((CSceneTreeWindow*)pCtx)->sendParentCommand(ID_EDIT_REDO);
	}, this);
	m_pWindow->addCommand("center_on_selection", [](void *pCtx){
		((CSceneTreeWindow*)pCtx)->sendParentCommand(ID_VIEW_CENTERONSELECTION);
	}, this);
	m_pWindow->addCommand("go_to_selection", [](void *pCtx){
		((CSceneTreeWindow*)pCtx)->sendParentCommand(ID_VIEW_GOTOSELECTION);
	}, this);
	m_pWindow->addCommand("rename", [](void *pCtx){
		((CSceneTreeWindow*)pCtx)->m_pTree->editSelectedNode();
	}, this);

	onResize();

	m_pWindow->maintainPlacement(CSCENETREEWINDOW_GUID);
}


CSceneTreeWindow::~CSceneTreeWindow()
{
	mem_release(m_pFilter);
	mem_release(m_pTreeMenu);
	m_pTree->setAdapter(NULL);
	mem_release(m_pTree);
	mem_release(m_pWindow);
}


void CSceneTreeWindow::EventProc(void *pCtx, IUIControl *pControl, gui::IEvent *ev)
{
	CSceneTreeWindow *pThis = (CSceneTreeWindow*)pCtx;

	pThis->eventProc(pControl, ev);
}
void CSceneTreeWindow::eventProc(IUIControl *pControl, gui::IEvent *ev)
{
	if(ev->type == gui::GUI_EVENT_TYPE_RESIZE)
	{
		onResize();
		return;
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_CONTEXTMENU && pControl == m_pTree)
	{
		m_pTree->dispatchEvent(ev);

		if(m_treeAdapter.hasSelection())
		{
			m_pTreeMenu->show(ev->clientX, ev->clientY);
		}

		ev->preventDefault = true;
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_KEYUP)
	{
		float fValue;
		if(pControl == m_pFilter)
		{
			scheduleFilterIn(0.5f);
		}
	}
}

void CSceneTreeWindow::onResize()
{
	UINT uWidth = m_pWindow->getDesc()->iSizeX;
	UINT uHeight = m_pWindow->getDesc()->iSizeY;
	
	float fExternalMargin = 10.0f;
	float fTextboxHeight = 30.0f; // 48 with label
	float fSpacing = 10.0f;
	//UINT uRightCol = 48;

	m_pFilter->setPosition(fExternalMargin, fExternalMargin);
	m_pFilter->setSize((float)uWidth - fExternalMargin * 2.0f, fTextboxHeight);

	m_pTree->setPosition(fExternalMargin, fExternalMargin + fTextboxHeight + fSpacing);
	m_pTree->setSize((float)uWidth - fExternalMargin * 2.0f, (float)uHeight - fExternalMargin * 2.0f - fTextboxHeight - fSpacing);
	//m_pTree->setColumnWidth(0, (uWidth - (UINT)fExternalMargin) - uRightCol);
	//m_pTree->setColumnWidth(1, uRightCol);


	float fEmitterButtonWidth = 100.0f;
	float fEmitterButtonHeight = 24.0f;
	float fEmitterWindowHeight = 120.0f;

	float fPlaybackButtonWidth = 50.0f;
	float fPlaybackButtonHeight = 24.0f;

	float fControlButtonHeight = 24.0f;
	/*
	float fViewportWidth = uWidth * 0.5f - fExternalMargin;
	float fViewportHeight = uHeight - fExternalMargin * 2.0f - fSpacing - fPlaybackButtonHeight;
	m_pViewport->setPosition(fExternalMargin, fExternalMargin);
	m_pViewport->setSize(fViewportWidth, fViewportHeight);

	float fPanelWidth = uWidth * 0.5f - fSpacing - fExternalMargin;
	float fPanelHeight = uHeight - fEmitterWindowHeight - fExternalMargin * 2.0f - fSpacing * 2.0f - fControlButtonHeight;

	m_pPanel->setPosition(uWidth * 0.5f + fSpacing, fExternalMargin + fEmitterWindowHeight + fSpacing);
	m_pPanel->setSize(fPanelWidth, fPanelHeight);

	m_pEmittersGrid->setPosition(uWidth * 0.5f + fSpacing, fExternalMargin);
	m_pEmittersGrid->setSize(uWidth * 0.5f - fSpacing * 2.0f - fEmitterButtonWidth - fExternalMargin, fEmitterWindowHeight);

	m_pEmitterAddButton->setPosition(uWidth - fExternalMargin - fEmitterButtonWidth, fExternalMargin);
	m_pEmitterAddButton->setSize(fEmitterButtonWidth, fEmitterButtonHeight);

	m_pEmitterRemoveButton->setPosition(uWidth - fExternalMargin - fEmitterButtonWidth, fExternalMargin + fEmitterButtonHeight + fSpacing);
	m_pEmitterRemoveButton->setSize(fEmitterButtonWidth, fEmitterButtonHeight);

	m_pEmitterNameTextbox->setPosition(uWidth - fExternalMargin - fEmitterButtonWidth, fExternalMargin + 2.0f * (fEmitterButtonHeight + fSpacing));
	m_pEmitterNameTextbox->setSize(fEmitterButtonWidth, fEmitterTextboxHeight);

	float fColWidth = (uWidth * 0.5f - 5.0f * fSpacing - fExternalMargin) / 5.0f;
	for(UINT i = 0; i < 5; ++i)
	{
		m_uiEmissionData.pBurstsGrid->setColumnWidth(i, fColWidth);
	}


	fPlaybackButtonWidth = (fViewportWidth - fSpacing * 2.0f) / 3.0f;

	m_pPlayPauseButton->setPosition(fExternalMargin, fExternalMargin + fViewportHeight + fSpacing);
	m_pPlayPauseButton->setSize(fPlaybackButtonWidth, fPlaybackButtonHeight);

	m_pRestartButton->setPosition(fExternalMargin + fPlaybackButtonWidth + fSpacing, fExternalMargin + fViewportHeight + fSpacing);
	m_pRestartButton->setSize(fPlaybackButtonWidth, fPlaybackButtonHeight);

	m_pStopButton->setPosition(fExternalMargin + (fPlaybackButtonWidth + fSpacing) * 2.0f, fExternalMargin + fViewportHeight + fSpacing);
	m_pStopButton->setSize(fPlaybackButtonWidth, fPlaybackButtonHeight);

	float fControlButtonWidth = (fPanelWidth - fSpacing) / 2.0f;

	m_pSaveButton->setPosition(uWidth * 0.5f + fSpacing, fExternalMargin + fEmitterWindowHeight + fSpacing * 2.0f + fPanelHeight);
	m_pSaveButton->setSize(fControlButtonWidth, fControlButtonHeight);

	m_pCancelButton->setPosition(uWidth * 0.5f + fSpacing * 2.0f + fControlButtonWidth, fExternalMargin + fEmitterWindowHeight + fSpacing * 2.0f + fPanelHeight);
	m_pCancelButton->setSize(fControlButtonWidth, fControlButtonHeight);*/
}

void CSceneTreeWindow::update(float dt)
{
	if(m_isFilterScheduled)
	{
		m_fFilterTimer += dt;
		if(m_fFilterTimer >= m_fFilterAt)
		{
			m_isFilterScheduled = false;
			filter();
		}
	}
}

void CSceneTreeWindow::show()
{
	onObjectsetChanged(true);
	m_pWindow->show();
}

void CSceneTreeWindow::sendParentCommand(UINT uCmd)
{
	SendMessage((HWND)m_pWindow->getXWindow()->getParent(), WM_COMMAND, MAKEWPARAM(uCmd, 0), 0);
}

void CSceneTreeWindow::onObjectsetChanged(bool bForce)
{
	if(m_pWindow->isVisible() || bForce)
	{
		(m_isFilterActive ? m_treeAdapterFiltered : m_treeAdapter).onObjectsetChanged();
	}
}
void CSceneTreeWindow::onObjectNameChanged(IXEditorObject *pObject)
{
	if(m_pWindow->isVisible())
	{
		(m_isFilterActive ? m_treeAdapterFiltered : m_treeAdapter).onObjectNameChanged(pObject);
	}
}
void CSceneTreeWindow::onObjectAdded(IXEditorObject *pObject)
{
	if(m_pWindow->isVisible())
	{
		(m_isFilterActive ? m_treeAdapterFiltered : m_treeAdapter).onObjectAdded(pObject);
	}
}
void CSceneTreeWindow::onObjectRemoved(IXEditorObject *pObject)
{
	if(m_pWindow->isVisible())
	{
		(m_isFilterActive ? m_treeAdapterFiltered : m_treeAdapter).onObjectRemoved(pObject);
	}
}
void CSceneTreeWindow::onObjectSelected(IXEditorObject *pObject)
{
	if(m_pWindow->isVisible())
	{
		(m_isFilterActive ? m_treeAdapterFiltered : m_treeAdapter).onObjectSelected(pObject);
	}
}
void CSceneTreeWindow::onSelectionChanged()
{
	if(m_pWindow->isVisible())
	{
		(m_isFilterActive ? m_treeAdapterFiltered : m_treeAdapter).onSelectionChanged();
	}
}

void CSceneTreeWindow::scheduleFilterIn(float fSeconds)
{
	m_fFilterTimer = 0.0f;
	m_fFilterAt = fSeconds;
	m_isFilterScheduled = true;
}

void CSceneTreeWindow::filter()
{
	LogWarning("Filtering!\n");
	const char *szFilter = m_pFilter->getValue();
	TODO("Save scroll position!");
	if(szFilter[0])
	{
		// filter it
		if(!m_isFilterActive)
		{
			m_uUnfilteredScrollPos = m_pTree->getScroll();
		}
		m_isFilterActive = true;
		m_treeAdapterFiltered.setFilter(szFilter);
		m_pTree->setAdapter(&m_treeAdapterFiltered);
	}
	else
	{
		m_isFilterActive = false;
		m_treeAdapter.onObjectsetChanged();
		m_pTree->setAdapter(&m_treeAdapter);
		m_pTree->setScroll(m_uUnfilteredScrollPos);
	}
}

//#############################################################################

static const char* GetObjectName(IXEditorObject *pObj)
{
	const char *szName = pObj->getKV("name");
	if(szName && szName[0])
	{
		return(szName);
	}
	return(pObj->getClassName());
}

static int CompareNodes(const CSceneTreeAdapter::TreeNode &a, const CSceneTreeAdapter::TreeNode &b)
{
	const char *szA = GetObjectName(a.pObject);
	const char *szB = GetObjectName(b.pObject);
	int cmp = strcasecmp(szA, szB);
	if(cmp == 0)
	{
		return((int)(a.pObject - b.pObject));
	}
	return(cmp);
}

void CSceneTreeAdapter::setTree(IUITree *pTree)
{
	m_pTree = pTree;
}

UINT CSceneTreeAdapter::getColumnCount()
{
	return(1);
}
const char* CSceneTreeAdapter::getColumnHeader(UINT uCol)
{
	if(uCol == 0)
	{
		return("Object");
	}
	else if(uCol == 1)
	{
		return("Vis");
	}
	assert(!"Invalid col index");
	return("");
}

UINT CSceneTreeAdapter::getColumnHeaderWidth(UINT uCol, UINT uControlWidth)
{
	return(uControlWidth);

	if(uCol == 0)
	{
		return(uControlWidth - 32);
	}
	else if(uCol == 1)
	{
		return(32);
	}
	assert(!"Invalid col index");
	return(0);
}

UINT CSceneTreeAdapter::getChildrenCount(UITreeNodeHandle hNode)
{
	if(hNode == UI_TREE_NODE_ROOT)
	{
		return(m_rootNode.aChildren.size());
	}

	TreeNode *pNode = findTreeNode((IXEditorObject*)hNode);
	if(pNode->aChildren.size())
	{
		return(pNode->aChildren.size());
	}

	void *isProxy = NULL;
	((IXEditorObject*)hNode)->getInternalData(&X_IS_PROXY_GUID, &isProxy);

	if(isProxy)
	{
		return(((CProxyObject*)hNode)->getObjectCount());
	}
	return(0);
}
UITreeNodeHandle CSceneTreeAdapter::getChild(UITreeNodeHandle hNode, UINT uIdx)
{
	/*switch((UINT)hNode)
	{
	case 0:
		switch(uIdx)
		{
		case 0:
			return((UITreeNodeHandle)1);
		case 1:
			return((UITreeNodeHandle)2);
		case 2:
			return((UITreeNodeHandle)3);
		case 3:
			return((UITreeNodeHandle)4);
		}
	case 2:
		switch(uIdx)
		{
		case 0:
			return((UITreeNodeHandle)5);
		case 1:
			return((UITreeNodeHandle)6);
		case 2:
			return((UITreeNodeHandle)7);
		}
	case 7:
		switch(uIdx)
		{
		case 0:
			return((UITreeNodeHandle)9);
		}
	case 4:
		return((UITreeNodeHandle)(8 + uIdx));
		switch(uIdx)
		{
		case 0:
			return((UITreeNodeHandle)8);
		}
	}

	assert(!"Invalid parameter");
	return((UITreeNodeHandle)0);*/

	if(hNode == UI_TREE_NODE_ROOT)
	{
		assert(m_rootNode.aChildren.size() > uIdx);
		return(m_rootNode.aChildren[uIdx].pObject);
	}

	TreeNode *pNode = findTreeNode((IXEditorObject*)hNode);
	assert(pNode && pNode->aChildren.size() > uIdx);
	if(pNode)
	{
		return(pNode->aChildren[uIdx].pObject);
	}

	return(NULL);
}

static UINT CountMatches(const char *szHaystack, const char *szNeedle)
{
	size_t len = strlen(szNeedle);
	UINT uCount = 0;
	const char *tmp = szHaystack;
	while((tmp = strcasestr(tmp, szNeedle)))
	{
		tmp += len;
		++uCount;
	}
	return(uCount);
}

static void HighlightMatches(char *szOut, const char *szHaystack, const char *szNeedle, const char *szPrefix, const char *szPostfix)
{
	size_t sizeNeedle = strlen(szNeedle);
	size_t sizePrefix = strlen(szPrefix);
	size_t sizePostfix = strlen(szPostfix);

	const char *tmp = szHaystack;
	const char *szPos;
	while((szPos = strcasestr(tmp, szNeedle)))
	{
		memcpy(szOut, tmp, szPos - tmp);
		szOut += szPos - tmp;

		memcpy(szOut, szPrefix, sizePrefix);
		szOut += sizePrefix;

		memcpy(szOut, szPos, sizeNeedle);
		szOut += sizeNeedle;

		memcpy(szOut, szPostfix, sizePostfix);
		szOut += sizePostfix;

		tmp = szPos + sizeNeedle;
	}

	memcpy(szOut, tmp, strlen(tmp) + 1);
}

const char* CSceneTreeAdapter::getNodeView(UITreeNodeHandle hNode, UINT uCol, bool *pisRichFormat)
{
	if(uCol == 1)
	{
		return("");
	}

	static char szBuf[512];

	IXEditorObject *pObj = (IXEditorObject*)hNode;

	const char *szName = pObj->getKV("name");
	const char *szClassName = pObj->getClassName();

	if(m_hasFilter)
	{
		const char *szPrefix = "<span style=\"color: #ff0;\">";
		const char *szPostfix = "</span>";

		if(szName)
		{
			UINT uMatches = CountMatches(szName, m_sFilter.c_str());
			if(uMatches)
			{
				char *buf = (char*)alloca(sizeof(char) * ((strlen(szPrefix) + strlen(szPostfix)) * uMatches + strlen(szName) + 1));
				HighlightMatches(buf, szName, m_sFilter.c_str(), szPrefix, szPostfix);
				szName = buf;
			}
		}

		UINT uMatches = CountMatches(szClassName, m_sFilter.c_str());
		if(uMatches)
		{
			char *buf = (char*)alloca(sizeof(char) * ((strlen(szPrefix) + strlen(szPostfix)) * uMatches + strlen(szClassName) + 1));
			HighlightMatches(buf, szClassName, m_sFilter.c_str(), szPrefix, szPostfix);
			szClassName = buf;
		}
	}

	IXTexture *pIcon = pObj->getIcon();
	int nPrint = _snprintf(szBuf, sizeof(szBuf), 
		"<div style=\"width: 16px; height: 16px; display: inline-block%s%s;background-size-x: 100%%;background-size-y: 100%%;\"/> "
		"<div style=\"display: inline-block\">%s%s<span style=\"color: #ccc;\">%s</span></div>", 
		pIcon ? "; background-image: !" : "", pIcon ? pIcon->getName() : "", 
		szName ? szName : "", szName && szName[0] ? " - " : "", szClassName);
	if(nPrint < 0)
	{
		if(szName && szName[0])
		{
			return(szName);
		}
		return(szClassName);
	}
	*pisRichFormat = true;
	return(szBuf);
}
const char* CSceneTreeAdapter::getNodeText(UITreeNodeHandle hNode, UINT uCol)
{
	if(uCol == 1)
	{
		return("");
	}

	const char *szName = ((IXEditorObject*)hNode)->getKV("name");

	return(szName ? szName : "");
}

bool CSceneTreeAdapter::isNodeExpanded(UITreeNodeHandle hNode)
{	
	TreeNode *pNode = findTreeNode((IXEditorObject*)hNode);
	assert(pNode);
	if(pNode)
	{
		return(pNode->isExpanded);
	}

	return(false);
}
bool CSceneTreeAdapter::isNodeSelected(UITreeNodeHandle hNode)
{
	return(((IXEditorObject*)hNode)->isSelected());
}

bool CSceneTreeAdapter::onNodeExpanded(UITreeNodeHandle hNode, bool isExpanded, bool isRecursive)
{
	TreeNode *pNode = findTreeNode((IXEditorObject*)hNode);
	assert(pNode);
	if(pNode)
	{
		pNode->isExpanded = isExpanded;
		if(isExpanded)
		{
			loadChildren(pNode);
		}
		return(true);
	}
	return(false);
}
bool CSceneTreeAdapter::onNodeSelected(UITreeNodeHandle hNode, bool isSelected, bool isAdditive)
{
	CCommandSelect *pCmd = new CCommandSelect();

	IXEditorObject *pSelectedObject = (IXEditorObject*)hNode;

	if(!g_xConfig.m_bIgnoreGroups)
	{
		CProxyObject *pCur;
		while((pCur = XGetObjectParent(pSelectedObject)))
		{
			pSelectedObject = pCur;
		}
	}

	if(!isAdditive)
	{
		XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
			//if(!(g_xConfig.m_bIgnoreGroups && isProxy))
			{
				bool sel = false;
				if(!g_xConfig.m_bIgnoreGroups && pParent)
				{
					CProxyObject *pCur;
					while((pCur = XGetObjectParent(pParent)))
					{
						pParent = pCur;
					}
					pObj = pParent;
				}
				if(pObj->isSelected())
				{
					pCmd->addDeselected(pObj);
				}
			}
		});
	}


	if(isSelected)
	{
		pCmd->addSelected(pSelectedObject);
	}
	else
	{
		pCmd->addDeselected(pSelectedObject);
	}

	XExecCommand(pCmd);
	return(true);
}

bool CSceneTreeAdapter::onMultiSelected(UITreeNodeHandle *aNodes, UINT uNodeCount, bool isAdditive)
{
	CCommandSelect *pCmd = new CCommandSelect();
	if(!isAdditive)
	{
		XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
			//if(!(g_xConfig.m_bIgnoreGroups && isProxy))
			{
				bool sel = false;
				if(!g_xConfig.m_bIgnoreGroups && pParent)
				{
					CProxyObject *pCur;
					while((pCur = XGetObjectParent(pParent)))
					{
						pParent = pCur;
					}
					pObj = pParent;
				}
				if(pObj->isSelected())
				{
					pCmd->addDeselected(pObj);
				}
			}
		});
	}

	for(UINT i = 0; i < uNodeCount; ++i)
	{
		IXEditorObject *pSelectedObject = (IXEditorObject*)aNodes[i];

		if(!g_xConfig.m_bIgnoreGroups)
		{
			CProxyObject *pCur;
			while((pCur = XGetObjectParent(pSelectedObject)))
			{
				pSelectedObject = pCur;
			}
		}

		pCmd->addSelected(pSelectedObject);
	}

	XExecCommand(pCmd);


	return(true);
}

void CSceneTreeAdapter::onNodeEdited(UITreeNodeHandle hNode, const char *szNewText)
{
	CCommandProperties *pCmd = new CCommandProperties();
	XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
		if(pObj->isSelected() && (g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent))
		{
			pCmd->addObject(pObj);
		}
	});
	//pCmd->addObject((IXEditorObject*)hNode);
	pCmd->setKV("name", szNewText);
	XExecCommand(pCmd);
}

void CSceneTreeAdapter::ensureExpanded(UITreeNodeHandle hNode)
{
	IXEditorObject *pObj = (IXEditorObject*)hNode;
	
	if((pObj = XGetObjectParent(pObj)))
	{
		ensureExpanded((UITreeNodeHandle)pObj);

		onNodeExpanded((UITreeNodeHandle)pObj, true, false);
	}
}

void CSceneTreeAdapter::onObjectsetChanged()
{
	m_rootNode.aChildren.clearFast();
	m_rootNode.aChildren.reserve(g_pLevelObjects.size());

	if(m_hasFilter)
	{
		// load filtered recursive
		bool hasItems = false;
		loadFiltered(&m_rootNode, NULL, &hasItems);
	}
	else
	{
		TreeNode tmp;

		fora(i, g_pLevelObjects)
		{
			IXEditorObject *pObj = g_pLevelObjects[i];
			//isProxy = NULL;
			//pObj->getInternalData(&X_IS_PROXY_GUID, &isProxy);

			tmp.pObject = pObj;
			m_rootNode.aChildren.push_back(tmp);
			/*
			//Func(pObj, isProxy ? true : false, pWhere);


			if(isProxy)
			{
				((CProxyObject*)pObj)->getObjectCount();
			}*/
		}

		sortChildren(&m_rootNode);
	}

	m_pTree->notifyDatasetChanged();
}
void CSceneTreeAdapter::onObjectNameChanged(IXEditorObject *pObject)
{
	if(pObject)
	{
		TreeNode *pParentNode;
		TreeNode *pNode = findTreeNode(pObject, NULL, &pParentNode);
		if(pNode)
		{
			int idx = pNode - pParentNode->aChildren;
			TreeNode copy = *pNode;
			pParentNode->aChildren.erase(idx);
			pParentNode->aChildren.insert(copy, [](const TreeNode &a, const TreeNode &b){
				return(CompareNodes(a, b) > 0);
			});
		}
	}
	else
	{
		m_pTree->notifyDatasetChanged();
	}
}
void CSceneTreeAdapter::onObjectAdded(IXEditorObject *pObject)
{
	onObjectsetChanged();
}
void CSceneTreeAdapter::onObjectRemoved(IXEditorObject *pObject)
{
	bool bFound = removeTreeNode(pObject);
	//assert(bFound);
	if(bFound)
	{
		m_pTree->notifyDatasetChanged();
	}
}
void CSceneTreeAdapter::onObjectSelected(IXEditorObject *pObject)
{
	m_pScrollTo = pObject;
}
void CSceneTreeAdapter::onSelectionChanged()
{
	m_pTree->notifySelectionChanged();

	if(m_pScrollTo)
	{
		m_pTree->scrollIntoView((UITreeNodeHandle)m_pScrollTo);
		m_pScrollTo = NULL;
	}
}

bool CSceneTreeAdapter::hasSelection()
{
	bool hasSelection = false;
	XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
		if(!hasSelection && pObj->isSelected())
		{
			hasSelection = true;
		}
	});

	return(hasSelection);
}

void CSceneTreeAdapter::setFilter(const char *szFilter)
{
	m_sFilter = szFilter;
	m_hasFilter = !!szFilter[0];
	onObjectsetChanged();
}

bool CSceneTreeAdapter::isFilterPassed(IXEditorObject *pObject)
{
	const char *szName = pObject->getKV("name");

	return((szName && strcasestr(szName, m_sFilter.c_str())) || strcasestr(pObject->getClassName(), m_sFilter.c_str()));
}

void CSceneTreeAdapter::loadFiltered(TreeNode *pRoot, CProxyObject *pParent, bool *pHasItems)
{
	void *isProxy;
	TreeNode tmp;
	tmp.isExpanded;

	if(pParent)
	{
		for(UINT i = 0, l = pParent->getObjectCount(); i < l; ++i)
		{
			IXEditorObject *pObj = pParent->getObject(i);
			isProxy = NULL;
			pObj->getInternalData(&X_IS_PROXY_GUID, &isProxy);

			tmp.pObject = pObj;
			pRoot->aChildren.push_back(tmp);
			bool hasItems = isFilterPassed(pObj);

			if(isProxy)
			{
				loadFiltered(&pRoot->aChildren[pRoot->aChildren.size() - 1], (CProxyObject*)pObj, &hasItems);
			}
			if(hasItems)
			{
				*pHasItems = true;
			}
			else
			{
				pRoot->aChildren.erase(pRoot->aChildren.size() - 1);
			}
		}
	}
	else
	{
		fora(i, g_pLevelObjects)
		{
			IXEditorObject *pObj = g_pLevelObjects[i];
			isProxy = NULL;
			pObj->getInternalData(&X_IS_PROXY_GUID, &isProxy);

			tmp.pObject = pObj;
			pRoot->aChildren.push_back(tmp);
			bool hasItems = isFilterPassed(pObj);

			if(isProxy)
			{
				loadFiltered(&pRoot->aChildren[pRoot->aChildren.size() - 1], (CProxyObject*)pObj, &hasItems);
			}
			if(hasItems)
			{
				*pHasItems = true;
			}
			else
			{
				pRoot->aChildren.erase(pRoot->aChildren.size() - 1);
			}
		}
	}
	if(pRoot->aChildren.size())
	{
		pRoot->isExpanded = true;
	}

	sortChildren(pRoot);
}

CSceneTreeAdapter::TreeNode* CSceneTreeAdapter::findTreeNode(IXEditorObject *pObject, TreeNode *pFrom, TreeNode **ppParent)
{
	if(!pFrom)
	{
		pFrom = &m_rootNode;
	}

	TreeNode *pNested;
	fora(i, pFrom->aChildren)
	{
		if(pFrom->aChildren[i].pObject == pObject)
		{
			if(ppParent)
			{
				*ppParent = pFrom;
			}
			return(&pFrom->aChildren[i]);
		}

		if((pNested = findTreeNode(pObject, &pFrom->aChildren[i], ppParent)))
		{
			return(pNested);
		}
	}

	return(NULL);
}

bool CSceneTreeAdapter::removeTreeNode(IXEditorObject *pObject, TreeNode *pFrom)
{
	if(!pFrom)
	{
		pFrom = &m_rootNode;
	}

	TreeNode *pNested;
	fora(i, pFrom->aChildren)
	{
		if(pFrom->aChildren[i].pObject == pObject)
		{
			pFrom->aChildren.erase(i);
			return(true);
		}

		if(removeTreeNode(pObject, &pFrom->aChildren[i]))
		{
			return(true);
		}
	}

	return(false);
}

void CSceneTreeAdapter::loadChildren(TreeNode *pNode)
{
	if(pNode->aChildren.size())
	{
		return;
	}

	void *isProxy = NULL;
	pNode->pObject->getInternalData(&X_IS_PROXY_GUID, &isProxy);
	assert(isProxy);

	if(isProxy)
	{
		CProxyObject *pProxy = (CProxyObject*)pNode->pObject;

		TreeNode tmp;
		for(UINT i = 0, l = pProxy->getObjectCount(); i < l; ++i)
		{
			IXEditorObject *pObj = pProxy->getObject(i);
			//isProxy = NULL;
			//pObj->getInternalData(&X_IS_PROXY_GUID, &isProxy);

			tmp.pObject = pObj;
			pNode->aChildren.push_back(tmp);
		}

		sortChildren(pNode);
	}
}

void CSceneTreeAdapter::sortChildren(TreeNode *pNode)
{
	pNode->aChildren.quickSort([](const TreeNode &a, const TreeNode &b){
		return(CompareNodes(a, b) < 0);
	});
}
