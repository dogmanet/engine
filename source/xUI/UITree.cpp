#include "UITree.h"

CUITree::CUITree(ULONG uID):
	BaseClass(uID, "div")
{
}

void CUITree::setColumnCount(UINT uCount)
{
	m_aHeaders.resizeFast(uCount);

	fora(i, m_aRows)
	{
		m_aRows[i].aCells.resizeFast(uCount);
	}

	resize();
}
UINT CUITree::getColumnCount()
{
	return(m_aHeaders.size());
}
void CUITree::setColumnHeader(UINT uColumn, const char *szHeader)
{
	assert(uColumn < m_aHeaders.size());
	if(uColumn < m_aHeaders.size())
	{
		GridColumn &col = m_aHeaders[uColumn];
		col.sHeader = szHeader;
		if(col.pNode)
		{
			col.pNode->setText(StringW(CMB2WC(szHeader)), TRUE);
		}
	}
}
void CUITree::setColumnWidth(UINT uColumn, UINT uWidth)
{
	assert(uColumn < m_aHeaders.size());
	if(uColumn < m_aHeaders.size())
	{
		m_aHeaders[uColumn].uWidth = uWidth;

		resize();
	}
}

void CUITree::setRowCount(UINT uCount)
{
	m_aRows.resizeFast(uCount);

	fora(i, m_aRows)
	{
		m_aRows[i].aCells.resizeFast(m_aHeaders.size());
	}
	resize();
}
UINT CUITree::getRowCount()
{
	return(m_aRows.size());
}

gui::dom::IDOMnode* CUITree::createNode(gui::dom::IDOMdocument *pDomDocument)
{
	m_pDoc = pDomDocument;

	m_pNode = pDomDocument->createNode(L"div");
	m_pNode->classAdd(L"grid-wrapper");

	m_pNode->setUserData(this);
	m_pNode->setAttribute(L"onclick", L"handler");
	m_pNode->setAttribute(L"oncontextmenu", L"handler");
	m_pNode->setAttribute(L"onchange", L"handler");
	m_pNode->setAttribute(L"onlayout", L"handler");

	m_pLabel = pDomDocument->createNode(L"label");

	m_pGridInner = pDomDocument->createNode(L"div");
	m_pGridInner->classAdd(L"grid-inner");

	m_pHeaderRow = pDomDocument->createNode(L"div");
	m_pHeaderRow->classAdd(L"grid-row");
	m_pHeaderRow->classAdd(L"grid-header");
	
	m_pItems = pDomDocument->createNode(L"div");
	m_pItems->classAdd(L"grid-items");
	m_pItems->setAttribute(L"onscroll", L"handler");
	m_pItems->setUserData(this);

	// create rows

	resize();

	m_pNode->appendChild(m_pLabel);
	m_pGridInner->appendChild(m_pHeaderRow);
	m_pGridInner->appendChild(m_pItems);
	m_pNode->appendChild(m_pGridInner);
	return(m_pNode);
}

void XMETHODCALLTYPE CUITree::setLabel(const char *szTitle)
{
//	m_pLabel->setText(StringW(CMB2WC(szTitle)), true);
}

void CUITree::cleanupNodes()
{
	BaseClass::cleanupNodes();
	m_pLabel = NULL;
	m_pHeaderRow = NULL;
	m_pItems = NULL;
	m_pGridInner = NULL;

	m_pTopPadding = NULL;
	m_pBottomPadding = NULL;

	m_pEditNode = NULL;

	fora(i, m_aHeaders)
	{
		m_aHeaders[i].pNode = NULL;
	}

	fora(i, m_aRows)
	{
		GridRow &row = m_aRows[i];
		row.pNode = NULL;
		row.pPrefixNode = NULL;

		fora(j, row.aCells)
		{
			row.aCells[j].pNode = NULL;
		}
	}
}

void CUITree::resize()
{
	if(!m_pHeaderRow)
	{
		return;
	}

	XPROFILE_FUNCTION();
	// 1 remove extra rows/cols

	const gui::dom::IDOMnodeCollection *pRowsChildren = m_pItems->getChilds();
	UINT uPaddingsCount = (m_pTopPadding ? 1 : 0) + (m_pBottomPadding ? 1 : 0);
	UINT uCurrentRows = pRowsChildren->size() - uPaddingsCount;
	
	while((pRowsChildren->size() - uPaddingsCount) > m_aRows.size())
	{
		m_pItems->removeChild(pRowsChildren[0][pRowsChildren->size() - (m_pBottomPadding ? 2 : 1)]);
	}

	const gui::dom::IDOMnodeCollection *pHeaderChildren = m_pHeaderRow->getChilds();

	while(pHeaderChildren->size() > m_aHeaders.size())
	{
		m_pHeaderRow->removeChild(pHeaderChildren[0][pHeaderChildren->size() - 1]);

		// remove data cells too
		fora(i, m_aRows)
		{
			const gui::dom::IDOMnodeCollection *pCells = m_aRows[i].pNode->getChilds();
			m_aRows[i].pNode->removeChild(pCells[0][pCells->size() - 1]);
		}
	}


	// 2 create missing cols
	for(UINT i = pHeaderChildren->size(), l = m_aHeaders.size(); i < l; ++i)
	{
		gui::dom::IDOMnode *pNode = m_pDoc->createNode(L"div");
		pNode->classAdd(L"grid-cell");
		pNode->setText(StringW(CMB2WC(m_aHeaders[i].sHeader.c_str())));
		m_pHeaderRow->appendChild(pNode);
		m_aHeaders[i].pNode = pNode;

		// create data cells too
		fora(j, m_aRows)
		{
			if(m_aRows[j].pNode)
			{
				pNode = m_pDoc->createNode(L"div");
				pNode->classAdd(L"grid-cell");
				if(i == 0)
				{
					pNode->classAdd(L"tree-row");
				}
				//pNode->setText(StringW(CMB2WC(m_aRows[j].aCells[i].sData.c_str())));
				m_aRows[j].pNode->appendChild(pNode, false);
				m_aRows[j].aCells[i].pNode = pNode;
			}
		}
	}

	// 3 create missing rows

	for(UINT i = uCurrentRows, l = m_aRows.size(); i < l; ++i)
	{
		GridRow &row = m_aRows[i];

		gui::dom::IDOMnode *pNode = m_pDoc->createNode(L"div");
		pNode->classAdd(L"grid-row");
		m_pItems->appendChild(pNode, false, m_pBottomPadding);
		row.pNode = pNode;

		pNode = m_pDoc->createNode(L"div");
		pNode->classAdd(L"grid-cell");
		pNode->classAdd(L"tree-row");
		row.pNode->appendChild(pNode, false);
		row.pPrefixNode = pNode;

		fora(j, row.aCells)
		{
			pNode = m_pDoc->createNode(L"div");
			pNode->classAdd(L"grid-cell");
			//pNode->setText(StringW(CMB2WC(row.aCells[j].sData.c_str())));
			row.pNode->appendChild(pNode, false);
			row.aCells[j].pNode = pNode;
		}
	}

	m_pItems->updateLayout();

	gui::css::ICSSstyle *pStyle;

	UINT uLeft = 0;
	fora(i, m_aHeaders)
	{
		GridColumn &col = m_aHeaders[i];
		col.uLeft = uLeft;
		uLeft += col.uWidth;

		gui::css::ICSSstyle *pStyle = col.pNode->getStyleSelf();
		pStyle->left->set((float)col.uLeft);
		pStyle->left->setDim(gui::css::ICSSproperty::DIM_PX);

		pStyle->width->set((float)col.uWidth);
		pStyle->width->setDim(gui::css::ICSSproperty::DIM_PX);

		fora(j, m_aRows)
		{
			UINT uColWidth = col.uWidth;
			UINT uColLeft = col.uLeft;
			if(i == 0)
			{
				pStyle = m_aRows[j].pPrefixNode->getStyleSelf();

				pStyle->left->set((float)uColLeft);
				pStyle->left->setDim(gui::css::ICSSproperty::DIM_PX);

				UINT uPrefixWidth = (UINT)pStyle->width->getFloat();
				if(uPrefixWidth > uColWidth)
				{
					uPrefixWidth = uColWidth;
				}
				uColLeft = uPrefixWidth;
				uColWidth -= uPrefixWidth;
				pStyle->width->set((float)uPrefixWidth);
				pStyle->width->setDim(gui::css::ICSSproperty::DIM_PX);
			}

			pStyle = m_aRows[j].aCells[i].pNode->getStyleSelf();

			pStyle->left->set((float)uColLeft);
			pStyle->left->setDim(gui::css::ICSSproperty::DIM_PX);

			pStyle->width->set((float)uColWidth);
			pStyle->width->setDim(gui::css::ICSSproperty::DIM_PX);
		}

		col.pNode->updateStyles();
	}

	pStyle = m_pGridInner->getStyleSelf();
	pStyle->width->set((float)uLeft);
	pStyle->width->setDim(gui::css::ICSSproperty::DIM_PX);

	m_pGridInner->updateStyles();
	// resize rows
}

void CUITree::dispatchEvent(gui::IEvent *ev)
{
	if(ev->type == gui::GUI_EVENT_TYPE_LAYOUT && ev->target == m_pNode)
	{
		UINT uWidth = m_pNode->getInnerWidth();
		UINT uHeight = m_pNode->getInnerHeight();
		if(uWidth != m_uWidth || uHeight != m_uHeight)
		{
			m_uWidth = uWidth;
			m_uHeight = uHeight;

			if(m_pAdapter)
			{
				UINT uCols = m_pAdapter->getColumnCount();
				for(UINT i = 0; i < uCols; ++i)
				{
					setColumnWidth(i, m_pAdapter->getColumnHeaderWidth(i, m_uWidth));
				}

				updateVisibleNodes();
			}
		}
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_CLICK || ev->type == gui::GUI_EVENT_TYPE_CONTEXTMENU)
	{
		int iRow = m_aRows.indexOf(ev->target, [](const GridRow &row, gui::dom::IDOMnode *pNode){
			return(row.pNode == pNode || row.pNode == pNode->parentNode() || row.pNode == pNode->parentNode()->parentNode() && row.pPrefixNode != pNode->parentNode());
		});

		if(iRow >= 0)
		{
			GridRow &row = m_aRows[iRow];

			if(ev->type == gui::GUI_EVENT_TYPE_CONTEXTMENU)
			{
				if(!ev->bCtrlKey && !m_pAdapter->isNodeSelected(row.pTreeNode->hNode))
				{
					if(m_pAdapter->onNodeSelected(row.pTreeNode->hNode, true, false))
					{
						notifySelectionChanged();
					}
				}
			}
			else
			{
				if(ev->bShiftKey)
				{
					// select range from m_hFocusNode to row.pTreeNode->hNode
					if(!m_hFocusNode && m_topLevelNode.aChildren.size())
					{
						m_hFocusNode = m_topLevelNode.aChildren[0].hNode;
					}
					Array<UITreeNodeHandle> aToSelect;
					if(m_hFocusNode == row.pTreeNode->hNode)
					{
						aToSelect.push_back(m_hFocusNode);
					}
					else
					{
						UINT uState = 0;
						buildSelectionList(&m_topLevelNode, m_hFocusNode, row.pTreeNode->hNode, &aToSelect, &uState);
					}

					if(m_pAdapter->onMultiSelected(aToSelect, aToSelect.size(), ev->bCtrlKey))
					{
						notifySelectionChanged();
					}
				}
				else
				{
					m_hFocusNode = row.pTreeNode->hNode;
					bool isSelected = m_pAdapter->isNodeSelected(row.pTreeNode->hNode);
					if(isSelected && !ev->bCtrlKey)
					{
						// if has other selected item -> select current only
						// else -> start edit
						if(countSelection() > 1)
						{
							if(m_pAdapter->onNodeSelected(row.pTreeNode->hNode, true, false))
							{
								notifySelectionChanged();
							}
						}
						else
						{
							editSelectedNode();
						}
					}
					else if(m_pAdapter->onNodeSelected(row.pTreeNode->hNode, !isSelected, ev->bCtrlKey))
					{
						notifySelectionChanged();
					}
				}
			}
		}
		else if(ev->type == gui::GUI_EVENT_TYPE_CLICK)
		{
			iRow = m_aRows.indexOf(ev->target, [](const GridRow &row, gui::dom::IDOMnode *pNode){
				return(row.pPrefixNode == pNode->parentNode());
			});

			if(iRow >= 0)
			{
				toggleNode(iRow, ev->bAltKey);
				ev->preventDefault = true;
			}
		}
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_SCROLL && ev->currentTarget == m_pItems)
	{
		UINT uScroll = m_pItems->getScrollTop();
		UINT uScrollDelta;
		if(uScroll > m_uLastScroll)
		{
			uScrollDelta = uScroll - m_uLastScroll;
		}
		else
		{
			uScrollDelta = m_uLastScroll - uScroll;
		}

		if(uScrollDelta > m_uScrollThreshold)
		{
			updateVisibleNodes(true);
			ev->stopPropagation();
		}
		else
		{
			updateVisibleFlags();
		}
	}
	else if(ev->target == m_pEditNode && (ev->type == gui::GUI_EVENT_TYPE_BLUR || (ev->type == gui::GUI_EVENT_TYPE_KEYDOWN && (ev->key == KEY_ESCAPE || ev->key == KEY_ENTER))))
	{
		ev->stopPropagation();
		ev->preventDefault = true;
		if(ev->type == gui::GUI_EVENT_TYPE_KEYDOWN && ev->key == KEY_ESCAPE)
		{
			abortEdit();
		}
		else
		{
			acceptEdit();
		}
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_UPDATE)
	{
		updateDataset();
	}
}

void XMETHODCALLTYPE CUITree::setEnabled(bool bEnable)
{
	if(bEnable)
	{
		m_pNode->removePseudoclass(0x00004);
	}
	else
	{
		m_pNode->addPseudoclass(0x00004);
	}
}

void XMETHODCALLTYPE CUITree::setAdapter(IUITreeAdapter *pAdapter)
{
	m_pAdapter = pAdapter;
	notifyDatasetChanged();
}

void XMETHODCALLTYPE CUITree::notifyDatasetChanged()
{
	XPROFILE_FUNCTION();

	abortEdit();

	m_pUIWindow->registerForUpdate(this);
}

void XMETHODCALLTYPE CUITree::notifySelectionChanged()
{
	fora(i, m_aRows)
	{
		GridRow &row = m_aRows[i];
		if(m_pAdapter->isNodeSelected(row.pTreeNode->hNode))
		{
			row.pNode->addPseudoclass(2 /* PSEUDOCLASS_CHECKED */);
		}
		else
		{
			row.pNode->removePseudoclass(2 /* PSEUDOCLASS_CHECKED */);
		}
	}
}

void XMETHODCALLTYPE CUITree::editSelectedNode()
{
	if(m_pEditNode)
	{
		m_pEditNode->parentNode()->removeChild(m_pEditNode);
		m_pEditNode = NULL;
	}
	fora(i, m_aRows)
	{
		GridRow &row = m_aRows[i];
		if(row.pTreeNode->isVisible && m_pAdapter->isNodeSelected(row.pTreeNode->hNode))
		{
			m_hEditNode = row.pTreeNode->hNode;

			RECT rc = row.aCells[0].pNode->getClientRect();
			RECT rc2 = row.pPrefixNode->getClientRect();
			rc.left = rc2.right;

			rc2 = m_pItems->getClientRect();
			rc2.top -= m_pItems->getScrollTop();
			rc2.left -= m_pItems->getScrollLeft();
			rc.top -= rc2.top;
			rc.bottom -= rc2.top;
			rc.left -= rc2.left;
			rc.right -= rc2.left;

			m_pEditNode = m_pDoc->createNode(L"input");
			m_pEditNode->setAttribute(L"type", L"text");
			m_pEditNode->setAttribute(L"onblur", L"handler");
			m_pEditNode->setAttribute(L"onkeydown", L"handler");
			m_pEditNode->setAttribute(L"class", L"-opague");
			m_pEditNode->setUserData(this);
			gui::css::ICSSstyle *pStyle = m_pEditNode->getStyleSelf();
			pStyle->position->setExt(gui::css::ICSSproperty::POSITION_ABSOLUTE);
			pStyle->top->set((float)rc.top);
			pStyle->top->setDim(gui::css::ICSSproperty::DIM_PX);
			pStyle->left->set((float)rc.left);
			pStyle->left->setDim(gui::css::ICSSproperty::DIM_PX);
			pStyle->width->set((float)(rc.right - rc.left));
			pStyle->width->setDim(gui::css::ICSSproperty::DIM_PX);
			pStyle->height->set((float)(rc.bottom - rc.top));
			pStyle->height->setDim(gui::css::ICSSproperty::DIM_PX);

			m_pEditNode->setText(StringW(CMB2WC(m_pAdapter->getNodeText(row.pTreeNode->hNode, 0))), TRUE);

			m_pItems->appendChild(m_pEditNode);
			m_pDoc->requestFocus(m_pEditNode);

#if 0
			gui::IEvent ev;
			ev.type = gui::GUI_EVENT_TYPE_KEYPRESS;
			ev.target = m_pEditNode;
			ev.relatedTarget = NULL;
			ev.bCtrlKey = true;
			ev.key = 1;
			m_pEditNode->dispatchEvent(ev);
#endif
			break;
		}
	}
}

UINT XMETHODCALLTYPE CUITree::getScroll()
{
	return((UINT)m_pItems->getScrollTop());
}
void XMETHODCALLTYPE CUITree::setScroll(UINT uScroll)
{
	m_pItems->setScrollTop((float)uScroll);
}

void XMETHODCALLTYPE CUITree::scrollIntoView(UITreeNodeHandle hNode)
{
	int idx = m_aRows.indexOf(hNode, [](const GridRow &row, UITreeNodeHandle hNode){
		return(row.pTreeNode->hNode == hNode);
	});
	if(idx >= 0 && m_aRows[idx].pTreeNode->isVisible)
	{
		return;
	}

	bool bFirstStep = true;
	while(true)
	{
		UINT uPos = 0;
		if(findNodePos(&m_topLevelNode, hNode, &uPos))
		{
			UINT uRowHeight = 23; TODO("Query actual value");
			setScroll(uPos * uRowHeight);
			return;
		}

		if(!bFirstStep)
		{
			return;
		}
		m_pAdapter->ensureExpanded(hNode);
		syncNodes();
		bFirstStep = false;
	}
}

void CUITree::syncNodes()
{
	TreeNode copy = m_topLevelNode;
	syncNodes(UI_TREE_NODE_ROOT, &m_topLevelNode);

	// sync visual

	// find anchor (visible & no to delete)
	UITreeNodeHandle hAnchor = findAnchorNode(&m_topLevelNode);
	// find anchor node pos
	UINT uOldAnchorPos = 0;
	if(hAnchor && !findNodePos(&m_topLevelNode, hAnchor, &uOldAnchorPos))
	{
		uOldAnchorPos = 0;
	}

	// cleanup deleted nodes
	m_uNodeCount = 0;
	cleanupTree(&m_topLevelNode);

	if(0)
	{
		m_topLevelNode = copy;
	}

	if(m_hEditNode)
	{
		UITreeNodeHandle hEditNode = m_hEditNode;
		m_hEditNode = NULL;
		scrollIntoView(hEditNode);
	}
	else if(uOldAnchorPos)
	{
		UINT uAnchorPos = 0;
		if(hAnchor && !findNodePos(&m_topLevelNode, hAnchor, &uAnchorPos))
		{
			uAnchorPos = 0;
		}

		UINT uRowHeight = 23; TODO("Query actual value");

		int iDeltaScroll = ((int)uAnchorPos - (int)uOldAnchorPos) * uRowHeight;
		m_pItems->setScrollTop(m_pItems->getScrollTop() - iDeltaScroll);
	}

	UINT uPos;
	if(!findNodePos(&m_topLevelNode, m_hFocusNode, &uPos))
	{
		m_hFocusNode = NULL;
	}

	updateVisibleNodes();
}

void CUITree::updateVisibleNodes(bool bIncremental)
{
	XPROFILE_FUNCTION();

	abortEdit();

	TODO("Use incremental with sxgui2");
	bIncremental = false;

	UINT uVisibleHeight = m_pItems->getInnerHeight();
	if(!uVisibleHeight)
	{
		uVisibleHeight = m_uHeight;
	}
	UINT uHalfHeight = uVisibleHeight / 2;
	UINT uRowHeight = 23; TODO("Query actual value");
	UINT uScroll = m_pItems->getScrollTop();
	UINT uTopOffset = (uScroll > uHalfHeight) ? (uScroll - uHalfHeight) : 0;
	uTopOffset -= uTopOffset % uRowHeight;

	m_uLastScroll = uScroll;
	m_uScrollThreshold = uVisibleHeight / 2;

	UINT uBindCount = uVisibleHeight * 3 / uRowHeight;
	if(uBindCount > m_uNodeCount)
	{
		uBindCount = m_uNodeCount;
	}

	UINT uMaxTopOffset = (m_uNodeCount - uBindCount) * uRowHeight;
	if(uTopOffset > uMaxTopOffset)
	{
		uTopOffset = uMaxTopOffset;
	}

	UINT uBottomOffset = (m_uNodeCount - uBindCount) * uRowHeight;
	if(uBottomOffset > uTopOffset)
	{
		uBottomOffset -= uTopOffset;
	}
	else
	{
		uBottomOffset = 0;
	}

	updateRowCount(uBindCount, uTopOffset, uBottomOffset);

	UINT uBindFirst = uTopOffset / uRowHeight;
	UINT uBindLast = uBindFirst + uBindCount;
	UINT uVisFirst = uScroll / uRowHeight;
	UINT uVisLast = (uScroll + uVisibleHeight) / uRowHeight;

	if(bIncremental)
	{
		if(m_uBindFirst > uBindFirst)
		{
			// move from bottom to top
			UINT uCount = m_uBindFirst - uBindFirst;
			UINT uRowCount = m_aRows.size();
			while(uCount > 0)
			{
				GridRow row = m_aRows[uRowCount - 1];
				m_aRows.erase(uRowCount - 1);
				m_aRows.insert(row, 0);
				m_pItems->takeChild(row.pNode, true);
				m_pItems->appendChild(row.pNode, false, m_pItems->getChilds()[0][m_pTopPadding ? 1 : 0]);
				--uCount;
			}
		}
		else if(m_uBindFirst < uBindFirst)
		{
			// move from top to bottom
			UINT uCount = uBindFirst - m_uBindFirst;
			UINT uRowCount = m_aRows.size();
			while(uCount > 0)
			{
				GridRow row = m_aRows[0];
				m_aRows.erase(0);
				m_aRows.push_back(row);
				m_pItems->takeChild(row.pNode, true);
				m_pItems->appendChild(row.pNode, false, m_pBottomPadding);
				--uCount;
			}
		}
	}
	m_uBindFirst = uBindFirst;

	UINT uCounter = 0;
	Array<bool> aLines;
	{
		XPROFILE_SECTION("cleanRows");
		fora(i, m_aRows)
		{
			GridRow &row = m_aRows[i];
			
			while(row.pPrefixNode->getChilds()->size())
			{
				row.pPrefixNode->removeChild(row.pPrefixNode->getChilds()[0][0]);
			}

			Array<GridCell> &aCells = row.aCells;
			fora(j, aCells)
			{
				if(aCells[j].pNode->getChilds()->size() > 1 || (aCells[j].pNode->getChilds()->size() == 1 && !aCells[j].pNode->getChilds()[0][0]->isTextNode()))
				{
					while(aCells[j].pNode->getChilds()->size())
					{
						aCells[j].pNode->removeChild(aCells[j].pNode->getChilds()[0][0], false);
					}
				}
			}
		}
	}
	{
		XPROFILE_SECTION("bindRows");
		bindRows(&m_topLevelNode, uBindFirst, uBindLast, uVisFirst, uVisLast, &uCounter, aLines, bIncremental);
	}

	resize();

	m_pItems->updateLayout();
}

void CUITree::updateVisibleFlags()
{
	XPROFILE_FUNCTION();

	RECT rcItems = m_pItems->getClientRect();
	RECT rc;
	fora(i, m_aRows)
	{
		GridRow &row = m_aRows[i];
		rc = row.pNode->getClientRect();
		row.pTreeNode->isVisible = !(rc.top >= rcItems.bottom || rc.bottom <= rcItems.top);
	}
}

void CUITree::updateRowCount(UINT uCount, UINT uTopPadding, UINT uBottomPadding)
{
	XPROFILE_FUNCTION();

	UINT uPaddingCount = 0;
	if(uTopPadding && !m_pTopPadding)
	{
		m_pTopPadding = m_pDoc->createNode(L"div");
		m_pItems->appendChild(m_pTopPadding, false, m_pItems->getChilds()->size() ? m_pItems->getChilds()[0][0] : NULL);
	}
	else if(!uTopPadding && m_pTopPadding)
	{
		m_pItems->removeChild(m_pTopPadding);
		m_pTopPadding = NULL;
	}

	if(uBottomPadding && !m_pBottomPadding)
	{
		m_pBottomPadding = m_pDoc->createNode(L"div");
		m_pItems->appendChild(m_pBottomPadding);
	}
	else if(!uBottomPadding && m_pBottomPadding)
	{
		m_pItems->removeChild(m_pBottomPadding);
		m_pBottomPadding = NULL;
	}

	if(m_pTopPadding)
	{
		m_pTopPadding->getStyleSelf()->height->set((float)uTopPadding);
		m_pTopPadding->getStyleSelf()->height->setDim(gui::css::ICSSproperty::DIM_PX);
		m_pTopPadding->updateStyles();
		++uPaddingCount;
	}
	if(m_pBottomPadding)
	{
		m_pBottomPadding->getStyleSelf()->height->set((float)uBottomPadding);
		m_pBottomPadding->getStyleSelf()->height->setDim(gui::css::ICSSproperty::DIM_PX);
		m_pBottomPadding->updateStyles();
		++uPaddingCount;
	}

	setRowCount(uCount);
}

void CUITree::cleanupTree(TreeNode *pParentNode)
{
	fora(i, pParentNode->aChildren)
	{
		TreeNode *pNode = &pParentNode->aChildren[i];
		if(pNode->bToRemove)
		{
			pParentNode->aChildren.erase(i);
			--i; --il;
		}
		else
		{
			++m_uNodeCount;
			pNode->isBound = false;
			pNode->isVisible = false;
			cleanupTree(pNode);
		}
	}
}

bool CUITree::findNodePos(TreeNode *pParentNode, UITreeNodeHandle hFind, UINT *puPos)
{
	fora(i, pParentNode->aChildren)
	{
		TreeNode *pNode = &pParentNode->aChildren[i];
		if(pNode->hNode == hFind)
		{
			return(true);
		}
		++*puPos;

		if(findNodePos(pNode, hFind, puPos))
		{
			return(true);
		}
	}

	return(false);
}

UITreeNodeHandle CUITree::findAnchorNode(TreeNode *pParentNode)
{
	fora(i, pParentNode->aChildren)
	{
		TreeNode *pNode = &pParentNode->aChildren[i];
		if(!pNode->bToRemove)
		{
			if(pNode->isVisible)
			{
				return(pNode);
			}

			UITreeNodeHandle hNode = findAnchorNode(pNode);
			if(hNode)
			{
				return(hNode);
			}
		}
	}

	return(NULL);
}

void CUITree::syncNodes(UITreeNodeHandle hParent, TreeNode *pParentNode)
{
	UINT uInternalIdx = 0;
	UINT uChildrenCount = m_pAdapter ? m_pAdapter->getChildrenCount(hParent) : 0;
	if(hParent && uChildrenCount && !m_pAdapter->isNodeExpanded(hParent))
	{
		uChildrenCount = 0;
	}
	for(UINT i = 0; i < uChildrenCount; ++i)
	{
		UITreeNodeHandle hNode = m_pAdapter->getChild(hParent, i);
		TreeNode *pFoundNode = NULL;
		if(uInternalIdx >= pParentNode->aChildren.size())
		{
			bool isFound = false;

			for(UINT j = 0; j < uInternalIdx; ++j)
			{
				if(hNode == pParentNode->aChildren[j].hNode)
				{
					isFound = true;
					// перемещаем в позицию uInternalIdx, снимаем метку удаления
					pParentNode->aChildren[j].bToRemove = false;
					pParentNode->aChildren.reserve(uInternalIdx + 1);
					pParentNode->aChildren[uInternalIdx] = pParentNode->aChildren[j];
					pParentNode->aChildren.erase(j);
					--uInternalIdx;

					pFoundNode = &pParentNode->aChildren[uInternalIdx];
					++uInternalIdx;
					break;
				}
			}

			if(!isFound)
			{
				// add new node
				pFoundNode = &pParentNode->aChildren[uInternalIdx];
				pFoundNode->hNode = hNode;
				++uInternalIdx;
			}
		}
		else if(hNode == pParentNode->aChildren[uInternalIdx].hNode)
		{
			// matched
			pFoundNode = &pParentNode->aChildren[uInternalIdx];
			++uInternalIdx;
		}
		else
		{
			// ...
			// 1. пытаемся найти hNode дальше по списку. если нашли - помечаем удаленными внутренние ноды от uInternalIdx до найденной
			// - не нашли -- тогда пытаемся найти hNode (помеченную как удаленную) в первой части списка. если нашли -- перемещаем в позицию uInternalIdx, снимаем метку удаления
			// - не нашли -- тогда добавляем в позицию uInternalIdx
			// 
			bool isFound = false;
			for(UINT j = uInternalIdx, jl = pParentNode->aChildren.size(); j < jl; ++j)
			{
				if(hNode == pParentNode->aChildren[j].hNode)
				{
					isFound = true;
					pFoundNode = &pParentNode->aChildren[j];

					// помечаем удаленными внутренние ноды от uInternalIdx до найденной
					for(UINT k = uInternalIdx; k < j; ++k)
					{
						pParentNode->aChildren[k].bToRemove = true;
					}
					uInternalIdx = j + 1;
					break;
				}
			}
			if(!isFound)
			{
				for(UINT j = 0; j < uInternalIdx; ++j)
				{
					if(hNode == pParentNode->aChildren[j].hNode)
					{
						isFound = true;
						// перемещаем в позицию uInternalIdx, снимаем метку удаления
						pParentNode->aChildren[j].bToRemove = false;
						std::swap(pParentNode->aChildren[j], pParentNode->aChildren[uInternalIdx]);
						pParentNode->aChildren[j].bToRemove = true;

						pFoundNode = &pParentNode->aChildren[uInternalIdx];
						++uInternalIdx;
						break;
					}
				}
			}
			if(!isFound)
			{
				// добавляем в позицию uInternalIdx
				TreeNode newNode;
				newNode.hNode = hNode;
				pParentNode->aChildren.insert(newNode, uInternalIdx);
				pFoundNode = &pParentNode->aChildren[uInternalIdx];
				++uInternalIdx;
			}
		}

		pFoundNode->hasChildren = m_pAdapter->getChildrenCount(hNode) > 0;
		pFoundNode->isExpanded = pFoundNode->hasChildren && m_pAdapter->isNodeExpanded(hNode);

		// рекурсивно пройти для поднод
		syncNodes(hNode, pFoundNode);
	}

	for(UINT i = uInternalIdx, l = pParentNode->aChildren.size(); i < l; ++i)
	{
		pParentNode->aChildren[i].bToRemove = true;
	}
}

void CUITree::bindRows(TreeNode *pParentNode, UINT uBindFirst, UINT uBindLast, UINT uVisFirst, UINT uVisLast, UINT *puCounter, Array<bool> &aLines, bool bIncremental, UINT uDepth)
{
	bool isRichView = false;
	StringW wsCell;

	fora(i, pParentNode->aChildren)
	{
		TreeNode *pNode = &pParentNode->aChildren[i];
		bool isLastOnLevel = i == il - 1;

		if(*puCounter >= uBindFirst && *puCounter < uBindLast)
		{
			pNode->isBound = true;
			pNode->isVisible = (*puCounter >= uVisFirst && *puCounter < uVisLast);

			GridRow &row = m_aRows[*puCounter - uBindFirst];
			if(!bIncremental || pNode != row.pTreeNode)
			{
				row.pTreeNode = pNode;
				Array<GridCell> &aCells = row.aCells;

				//row.pPrefixNode->classToggle(L"-expandable", pNode->hasChildren);
				//row.pPrefixNode->classToggle(L"-expanded", pNode->isExpanded);

#if 0
				while(row.pPrefixNode->getChilds()->size())
				{
					row.pPrefixNode->removeChild(row.pPrefixNode->getChilds()[0][0]);
				}
#endif

				gui::dom::IDOMnode *pLineNode;
				for(UINT i = 0; i < uDepth - 1; ++i)
				{
					pLineNode = m_pDoc->createNode(L"div");
					if(aLines[i])
					{
						pLineNode->setAttribute(L"class", L"tree-line -passthrough");
					}
					else
					{
						pLineNode->setAttribute(L"class", L"tree-line");
					}
					row.pPrefixNode->appendChild(pLineNode, false);
				}

				pLineNode = m_pDoc->createNode(L"div");
				if(isLastOnLevel)
				{
					pLineNode->setAttribute(L"class", L"tree-line -last");
				}
				else
				{
					pLineNode->setAttribute(L"class", L"tree-line -normal");
				}
				if(pNode->hasChildren)
				{
					if(pNode->isExpanded)
					{
						pLineNode->classAdd(L"-expanded");
					}
					else
					{
						pLineNode->classAdd(L"-collapsed");
					}
					pLineNode->setAttribute(L"onclick", L"handler");
				}
				row.pPrefixNode->appendChild(pLineNode, false);


				gui::css::ICSSstyle *pStyle = row.pPrefixNode->getStyleSelf();

				UINT uPrefixWidth = 16;

				pStyle->width->set((float)(uDepth * uPrefixWidth));
				pStyle->width->setDim(gui::css::ICSSproperty::DIM_PX);

				if(m_pAdapter->isNodeSelected(pNode->hNode))
				{
					row.pNode->addPseudoclass(2 /* PSEUDOCLASS_CHECKED */);
				}
				else
				{
					row.pNode->removePseudoclass(2 /* PSEUDOCLASS_CHECKED */);
				}

				fora(i, aCells)
				{
					isRichView = false;
					wsCell = CMB2WC(m_pAdapter->getNodeView(pNode->hNode, i, &isRichView));

					if(isRichView)
					{
						aCells[i].pNode->setHTML(wsCell, false);
					}
					else
					{
#if 0
						if(aCells[i].pNode->getChilds()->size() > 1 || (aCells[i].pNode->getChilds()->size() == 1 && !aCells[i].pNode->getChilds()[0][0]->isTextNode()))
						{
							while(aCells[i].pNode->getChilds()->size())
							{
								aCells[i].pNode->removeChild(aCells[i].pNode->getChilds()[0][0], false);
							}
						}
#endif
						aCells[i].pNode->setText(wsCell);
					}
				}
			}
		}

		aLines[uDepth - 1] = !isLastOnLevel;

		++*puCounter;
		bindRows(pNode, uBindFirst, uBindLast, uVisFirst, uVisLast, puCounter, aLines, bIncremental, uDepth + 1);
	}
}

void CUITree::toggleNode(UINT uRow, bool isRecursive)
{
	if(uRow < m_aRows.size() && m_aRows[uRow].pTreeNode->hasChildren)
	{
		if(m_pAdapter->onNodeExpanded(m_aRows[uRow].pTreeNode->hNode, !m_aRows[uRow].pTreeNode->isExpanded, isRecursive))
		{
			syncNodes();
		}
	}
}

void CUITree::buildSelectionList(TreeNode *pParentNode, UITreeNodeHandle hFrom, UITreeNodeHandle hTo, Array<UITreeNodeHandle> *paOut, UINT *puState)
{
	fora(i, pParentNode->aChildren)
	{
		TreeNode *pNode = &pParentNode->aChildren[i];
		if(pNode->hNode == hFrom || pNode->hNode == hTo)
		{
			paOut->push_back(pNode->hNode);
			++*puState;
		}
		else if(*puState == 1)
		{
			paOut->push_back(pNode->hNode);
		}
		
		buildSelectionList(pNode, hFrom, hTo, paOut, puState);

		if(*puState > 1)
		{
			return;
		}
	}
}

void CUITree::acceptEdit()
{
	if(m_pEditNode)
	{
		CWC2MB tmp(m_pEditNode->getText().c_str());
		destroyEditor();
		m_pAdapter->onNodeEdited(m_hEditNode, tmp);
	}
}
void CUITree::abortEdit()
{
	destroyEditor();
}
void CUITree::destroyEditor()
{
	if(m_pEditNode)
	{
		gui::dom::IDOMnode *pNode = m_pEditNode;
		m_pEditNode = NULL;
		pNode->parentNode()->removeChild(pNode);
	}
}

void CUITree::updateDataset()
{
	if(m_pAdapter)
	{
		UINT uCols = m_pAdapter->getColumnCount();
		setColumnCount(uCols);
		for(UINT i = 0; i < uCols; ++i)
		{
			setColumnHeader(i, m_pAdapter->getColumnHeader(i));
			setColumnWidth(i, m_pAdapter->getColumnHeaderWidth(i, m_uWidth));
		}
	}

	syncNodes();
}

UINT CUITree::countSelection(TreeNode *pParentNode)
{
	if(!pParentNode)
	{
		pParentNode = &m_topLevelNode;
	}

	UINT uCount = 0;

	fora(i, pParentNode->aChildren)
	{
		TreeNode *pNode = &pParentNode->aChildren[i];

		if(m_pAdapter->isNodeSelected(pNode->hNode))
		{
			++uCount;
		}

		if(pNode->isExpanded)
		{
			uCount += countSelection(pNode);
		}
	}

	return(uCount);
}
