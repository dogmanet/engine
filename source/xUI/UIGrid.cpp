#include "UIGrid.h"

CUIGrid::CUIGrid(ULONG uID):
	BaseClass(uID, "div")
{
}


void XMETHODCALLTYPE CUIGrid::setColumnCount(UINT uCount)
{
	m_aHeaders.resizeFast(uCount);

	fora(i, m_aRows)
	{
		m_aRows[i].aCells.resizeFast(uCount);
	}

	resize();
}
UINT XMETHODCALLTYPE CUIGrid::getColumnCount()
{
	return(m_aHeaders.size());
}
void XMETHODCALLTYPE CUIGrid::setColumnHeader(UINT uColumn, const char *szHeader)
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
void XMETHODCALLTYPE CUIGrid::setColumnWidth(UINT uColumn, UINT uWidth)
{
	assert(uColumn < m_aHeaders.size());
	if(uColumn < m_aHeaders.size())
	{
		m_aHeaders[uColumn].uWidth = uWidth;

		resize();
	}
}

void XMETHODCALLTYPE CUIGrid::setRowCount(UINT uCount)
{
	if((UINT)m_iSelectedRow >= uCount)
	{
		m_iSelectedRow = -1;
	}

	m_aRows.resizeFast(uCount);

	fora(i, m_aRows)
	{
		m_aRows[i].aCells.resizeFast(m_aHeaders.size());
	}
	resize();
}
UINT XMETHODCALLTYPE CUIGrid::getRowCount()
{
	return(m_aRows.size());
}

void XMETHODCALLTYPE CUIGrid::setCell(UINT uColumn, UINT uRow, const char *szData)
{
	assert(uColumn < m_aHeaders.size());
	assert(uRow < m_aRows.size());
	if(uColumn < m_aHeaders.size() && uRow < m_aRows.size())
	{
		GridCell &cell = m_aRows[uRow].aCells[uColumn];
		cell.sData = szData;
		if(cell.pNode)
		{
			cell.pNode->setText(StringW(CMB2WC(szData)), TRUE);
		}
	}
}

gui::dom::IDOMnode* CUIGrid::createNode(gui::dom::IDOMdocument *pDomDocument)
{
	m_pDoc = pDomDocument;

	m_pNode = pDomDocument->createNode(L"div");
	m_pNode->classAdd(L"grid-wrapper");

	m_pNode->setUserData(this);
	m_pNode->setAttribute(L"onclick", L"handler");
	m_pNode->setAttribute(L"onchange", L"handler");

	m_pLabel = pDomDocument->createNode(L"label");

	m_pGridInner = pDomDocument->createNode(L"div");
	m_pGridInner->classAdd(L"grid-inner");

	m_pHeaderRow = pDomDocument->createNode(L"div");
	m_pHeaderRow->classAdd(L"grid-row");
	m_pHeaderRow->classAdd(L"grid-header");
	
	m_pItems = pDomDocument->createNode(L"div");
	m_pItems->classAdd(L"grid-items");

	// create rows

	resize();

	m_pNode->appendChild(m_pLabel);
	m_pGridInner->appendChild(m_pHeaderRow);
	m_pGridInner->appendChild(m_pItems);
	m_pNode->appendChild(m_pGridInner);
	return(m_pNode);
}

void XMETHODCALLTYPE CUIGrid::setLabel(const char *szTitle)
{
//	m_pLabel->setText(StringW(CMB2WC(szTitle)), true);
}

void CUIGrid::cleanupNodes()
{
	BaseClass::cleanupNodes();
	m_pLabel = NULL;
	m_pHeaderRow = NULL;
	m_pItems = NULL;
	m_pGridInner = NULL;

	fora(i, m_aHeaders)
	{
		m_aHeaders[i].pNode = NULL;
	}

	fora(i, m_aRows)
	{
		GridRow &row = m_aRows[i];
		row.pNode = NULL;

		fora(j, row.aCells)
		{
			row.aCells[j].pNode = NULL;
		}
	}
}

void CUIGrid::resize()
{
	if(!m_pHeaderRow)
	{
		return;
	}

	// 1 remove extra rows/cols

	const gui::dom::IDOMnodeCollection *pRowsChildren = m_pItems->getChilds();

	while(pRowsChildren->size() > m_aRows.size())
	{
		m_pItems->removeChild(pRowsChildren[0][pRowsChildren->size() - 1]);
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
				pNode->setText(StringW(CMB2WC(m_aRows[j].aCells[i].sData.c_str())));
				m_aRows[j].pNode->appendChild(pNode);
				m_aRows[j].aCells[i].pNode = pNode;
			}
		}
	}

	// 3 create missing rows

	for(UINT i = pRowsChildren->size(), l = m_aRows.size(); i < l; ++i)
	{
		GridRow &row = m_aRows[i];

		gui::dom::IDOMnode *pNode = m_pDoc->createNode(L"div");
		pNode->classAdd(L"grid-row");
		m_pItems->appendChild(pNode);
		row.pNode = pNode;

		if(m_iSelectedRow == i)
		{
			TODO("Use constant for pseudoclass");
			pNode->addPseudoclass(2 /* PSEUDOCLASS_CHECKED */);
		}

		fora(j, row.aCells)
		{
			pNode = m_pDoc->createNode(L"div");
			pNode->classAdd(L"grid-cell");
			pNode->setText(StringW(CMB2WC(row.aCells[j].sData.c_str())));
			row.pNode->appendChild(pNode);
			row.aCells[j].pNode = pNode;
		}
	}

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
			pStyle = m_aRows[j].aCells[i].pNode->getStyleSelf();

			pStyle->left->set((float)col.uLeft);
			pStyle->left->setDim(gui::css::ICSSproperty::DIM_PX);

			pStyle->width->set((float)col.uWidth);
			pStyle->width->setDim(gui::css::ICSSproperty::DIM_PX);
		}
	}

	pStyle = m_pGridInner->getStyleSelf();
	pStyle->width->set((float)uLeft);
	pStyle->width->setDim(gui::css::ICSSproperty::DIM_PX);

	// resize rows
}

void CUIGrid::dispatchEvent(gui::IEvent *ev)
{
	if(ev->type == gui::GUI_EVENT_TYPE_CLICK)
	{
		int iRow = m_aRows.indexOf(ev->target, [](const GridRow &row, gui::dom::IDOMnode *pNode){
			return(row.pNode == pNode || row.pNode == pNode->parentNode());
		});

		if(iRow >= 0)
		{
			setSelection(iRow);
		}
	}
}

void XMETHODCALLTYPE CUIGrid::setSelection(int iRow)
{
	if((UINT)iRow >= m_aRows.size())
	{
		iRow = -1;
	}

	if(iRow == m_iSelectedRow)
	{
		return;
	}

	if(m_iSelectedRow >= 0 && m_aRows[m_iSelectedRow].pNode)
	{
		TODO("Use constant for pseudoclass");
		m_aRows[m_iSelectedRow].pNode->removePseudoclass(2 /* PSEUDOCLASS_CHECKED */);
	}

	m_iSelectedRow = iRow;
	if(m_iSelectedRow >= 0 && m_aRows[m_iSelectedRow].pNode)
	{
		TODO("Use constant for pseudoclass");
		m_aRows[m_iSelectedRow].pNode->addPseudoclass(2 /* PSEUDOCLASS_CHECKED */);
	}

	gui::IEvent ev;
	ev.type = gui::GUI_EVENT_TYPE_CHANGE;
	ev.target = m_pNode;
	ev.relatedTarget = m_iSelectedRow >= 0 ? m_aRows[m_iSelectedRow].pNode : NULL;
	m_pNode->dispatchEvent(ev);
}
int XMETHODCALLTYPE CUIGrid::getSelection()
{
	return(m_iSelectedRow);
}

void XMETHODCALLTYPE CUIGrid::setEnabled(bool bEnable)
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
