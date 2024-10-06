#ifndef __UITREE_H
#define __UITREE_H

#include "IUIWindow.h"
#include "UIControl.h"
#include "IUITree.h"

class CUITree: public CUIControl<IUITree>
{
public:
	CUITree(ULONG uID);

	void XMETHODCALLTYPE setLabel(const char *szTitle) override;
	gui::dom::IDOMnode* createNode(gui::dom::IDOMdocument *pDomDocument) override;
	void XMETHODCALLTYPE dispatchEvent(gui::IEvent *ev) override;

	void XMETHODCALLTYPE setEnabled(bool bEnable) override;

	void XMETHODCALLTYPE setAdapter(IUITreeAdapter *pAdapter) override;

	void XMETHODCALLTYPE notifyDatasetChanged() override;

	void XMETHODCALLTYPE notifySelectionChanged() override;

	void XMETHODCALLTYPE editSelectedNode() override;
	
	UINT XMETHODCALLTYPE getScroll() override;
	void XMETHODCALLTYPE setScroll(UINT uScroll) override;

private:
	struct GridColumn
	{
		String sHeader;
		UINT uWidth = 100;
		gui::dom::IDOMnode *pNode = NULL;
		UINT uLeft = 0;
	};

	struct GridCell
	{
		//String sData;
		gui::dom::IDOMnode *pNode = NULL;
	};

	struct TreeNode
	{
		UITreeNodeHandle hNode = NULL;
		Array<TreeNode> aChildren;
		//TREE_NODE_STATE state = TNS_UNBOUND;
		bool isBound = false;
		bool isVisible = false;
		bool hasChildren = false;
		bool isExpanded = false;

		bool bToRemove = false;
	};

	struct GridRow
	{
		Array<GridCell> aCells;
		gui::dom::IDOMnode *pPrefixNode = NULL;
		gui::dom::IDOMnode *pNode = NULL;
		TreeNode *pTreeNode = NULL; 
	};

	/*enum TREE_NODE_STATE
	{
		TNS_UNBOUND,
		TNS_BOUND,
		TNS_VISIBLE
	};*/


private:
	gui::dom::IDOMnode *m_pLabel = NULL;
	gui::dom::IDOMnode *m_pHeaderRow = NULL;
	gui::dom::IDOMnode *m_pItems = NULL;
	gui::dom::IDOMnode *m_pGridInner = NULL;

	gui::dom::IDOMnode *m_pTopPadding = NULL;
	gui::dom::IDOMnode *m_pBottomPadding = NULL;

	gui::dom::IDOMnode *m_pEditNode = NULL;
	UITreeNodeHandle m_hEditNode = NULL;

	IUITreeAdapter *m_pAdapter = NULL;

	TreeNode m_topLevelNode;

	UINT m_uWidth = 0;
	UINT m_uHeight = 0;
	UINT m_uNodeCount = 0;
	UINT m_uBindFirst = 0;

	UINT m_uLastScroll = 0;
	UINT m_uScrollThreshold = 0;

	Array<GridColumn> m_aHeaders;
	Array<GridRow> m_aRows;

	gui::dom::IDOMdocument *m_pDoc = NULL;

	UITreeNodeHandle m_hFocusNode = NULL;

private:
	void setColumnCount(UINT uCount);
	UINT getColumnCount();
	void setColumnHeader(UINT uColumn, const char *szHeader);
	void setColumnWidth(UINT uColumn, UINT uWidth);

	void setRowCount(UINT uCount);
	UINT getRowCount();

	void cleanupNodes() override;

	void resize();

	void syncNodes();
	void syncNodes(UITreeNodeHandle hParent, TreeNode *pParentNode);

	UITreeNodeHandle findAnchorNode(TreeNode *pParentNode);

	bool findNodePos(TreeNode *pParentNode, UITreeNodeHandle hFind, UINT *puPos);

	void cleanupTree(TreeNode *pParentNode);

	void updateVisibleNodes(bool bIncremental = false);

	void updateVisibleFlags();

	void updateRowCount(UINT uCount, UINT uTopPadding, UINT uBottomPadding);

	void bindRows(TreeNode *pParentNode, UINT uBindFirst, UINT uBindLast, UINT uVisFirst, UINT uVisLast, UINT *puCounter, Array<bool> &aLines, bool bIncremental, UINT uDepth = 1);

	void toggleNode(UINT uRow, bool isRecursive);
	
	void buildSelectionList(TreeNode *pParentNode, UITreeNodeHandle hFrom, UITreeNodeHandle hTo, Array<UITreeNodeHandle> *paOut, UINT *puState);

	void acceptEdit();
	void abortEdit();
	void destroyEditor();

	void updateDataset();

	UINT countSelection(TreeNode *pParentNode = NULL);
};

#endif
