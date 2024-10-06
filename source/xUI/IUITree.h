#ifndef __IUITREE_H
#define __IUITREE_H

#include "IUIControl.h"

typedef void* UITreeNodeHandle;
#define UI_TREE_NODE_ROOT ((UITreeNodeHandle)0)

class IUITreeAdapter
{
public:
	virtual UINT getColumnCount() = 0;
	virtual const char* getColumnHeader(UINT uCol) = 0;
	virtual UINT getColumnHeaderWidth(UINT uCol, UINT uControlWidth) = 0;

	virtual UINT getChildrenCount(UITreeNodeHandle hNode) = 0;
	virtual UITreeNodeHandle getChild(UITreeNodeHandle hNode, UINT uIdx) = 0;

	virtual const char* getNodeView(UITreeNodeHandle hNode, UINT uCol, bool *pisRichFormat) = 0;
	virtual const char* getNodeText(UITreeNodeHandle hNode, UINT uCol) = 0;

	virtual bool isNodeExpanded(UITreeNodeHandle hNode) = 0;
	virtual bool isNodeSelected(UITreeNodeHandle hNode) = 0;

	virtual bool onNodeExpanded(UITreeNodeHandle hNode, bool isExpanded, bool isRecursive) = 0;
	virtual bool onNodeSelected(UITreeNodeHandle hNode, bool isSelected, bool isAdditive) = 0;
	virtual bool onMultiSelected(UITreeNodeHandle *aNodes, UINT uNodeCount, bool isAdditive) = 0;

	virtual void onNodeEdited(UITreeNodeHandle hNode, const char *szNewText) = 0;
};

//#############################################################################

class IUITree: public IUIControl
{
public:
	virtual void XMETHODCALLTYPE setAdapter(IUITreeAdapter *pAdapter) = 0;

	virtual void XMETHODCALLTYPE notifyDatasetChanged() = 0;

	virtual void XMETHODCALLTYPE notifySelectionChanged() = 0;

	virtual void XMETHODCALLTYPE editSelectedNode() = 0;

	virtual UINT XMETHODCALLTYPE getScroll() = 0;
	virtual void XMETHODCALLTYPE setScroll(UINT uScroll) = 0;
};

#endif
