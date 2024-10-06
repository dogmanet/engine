#ifndef __SCENETREEWINDOW_H
#define __SCENETREEWINDOW_H


#include <xUI/IXUI.h>
#include <xcommon/IXCore.h>
#include <xcommon/editor/IXEditorObject.h>

class CProxyObject;
class CSceneTreeAdapter final: public IUITreeAdapter
{
public:
	void setTree(IUITree *pTree);

	UINT getColumnCount() override;
	const char* getColumnHeader(UINT uCol) override;
	UINT getColumnHeaderWidth(UINT uCol, UINT uControlWidth) override;

	UINT getChildrenCount(UITreeNodeHandle hNode) override;
	UITreeNodeHandle getChild(UITreeNodeHandle hNode, UINT uIdx) override;

	const char* getNodeView(UITreeNodeHandle hNode, UINT uCol, bool *pisRichFormat) override;
	const char* getNodeText(UITreeNodeHandle hNode, UINT uCol) override;

	bool isNodeExpanded(UITreeNodeHandle hNode) override;
	bool isNodeSelected(UITreeNodeHandle hNode) override;

	bool onNodeExpanded(UITreeNodeHandle hNode, bool isExpanded, bool isRecursive) override;
	bool onNodeSelected(UITreeNodeHandle hNode, bool isSelected, bool isAdditive) override;
	bool onMultiSelected(UITreeNodeHandle *aNodes, UINT uNodeCount, bool isAdditive) override;

	void onNodeEdited(UITreeNodeHandle hNode, const char *szNewText) override;

	void onObjectsetChanged();
	void onObjectNameChanged(IXEditorObject *pObject = NULL);
	void onObjectAdded(IXEditorObject *pObject);
	void onObjectRemoved(IXEditorObject *pObject);
	void onSelectionChanged();

	bool hasSelection();

	void setFilter(const char *szFilter);

	struct TreeNode
	{
		IXEditorObject *pObject = NULL;
		Array<TreeNode> aChildren;
		bool isExpanded = false;
	};

private:
	IUITree *m_pTree = NULL;

	//bool m_is2Expanded = true;
	//bool m_is4Expanded = true;
	//bool m_is7Expanded = true;

	//Array<bool> m_aSelected;

	TreeNode m_rootNode;

	String m_sFilter;
	bool m_hasFilter = false;


private:
	TreeNode* findTreeNode(IXEditorObject *pObject, TreeNode *pFrom = NULL, TreeNode **ppParent = NULL);
	bool removeTreeNode(IXEditorObject *pObject, TreeNode *pFrom = NULL);
	void loadChildren(TreeNode *pNode);
	void sortChildren(TreeNode *pNode);

	void loadFiltered(TreeNode *pRoot, CProxyObject *pParent, bool *pHasItems);

	bool isFilterPassed(IXEditorObject *pObject);
};

//#############################################################################

class CEditor;
class CSceneTreeWindow final: public IXUnknownImplementation<IXUnknown>
{
private:
	static void EventProc(void *pCtx, IUIControl *pControl, gui::IEvent *ev);
	void eventProc(IUIControl *pControl, gui::IEvent *ev);

public:
	CSceneTreeWindow(CEditor *pEditor, IXCore *pCore);
	~CSceneTreeWindow();
	
	void onObjectsetChanged(bool bForce = false);
	void onObjectNameChanged(IXEditorObject *pObject = NULL);
	void onObjectAdded(IXEditorObject *pObject);
	void onObjectRemoved(IXEditorObject *pObject);
	void onSelectionChanged();

	void update(float dt);

	void show();

private:
	HWND m_hMainWnd = NULL;
	IUIWindow *m_pWindow = NULL;
	IXCore *m_pCore = NULL;
	IXUI *m_pXUI = NULL;
	CEditor *m_pEditor = NULL;
	
	IUITree *m_pTree = NULL;
	IUITextBox *m_pFilter = NULL;
	IUIMenu *m_pTreeMenu = NULL;

	CSceneTreeAdapter m_treeAdapter;
	CSceneTreeAdapter m_treeAdapterFiltered;

	UINT m_uUnfilteredScrollPos = 0;
	float m_fFilterTimer = 0.0f;
	float m_fFilterAt = 0.0f;
	bool m_isFilterScheduled = false;
	bool m_isFilterActive = false;

private:
	void onResize();

	void sendParentCommand(UINT uCmd);

	void scheduleFilterIn(float fSeconds);
	void filter();
};

#endif
