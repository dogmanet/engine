#ifndef __UIGRID_H
#define __UIGRID_H

#include "IUIWindow.h"
#include "UIControl.h"
#include "IUIGrid.h"

class CUIGrid: public CUIControl<IUIGrid>
{
public:
	CUIGrid(ULONG uID);

	void XMETHODCALLTYPE setColumnCount(UINT uCount) override;
	UINT XMETHODCALLTYPE getColumnCount() override;
	void XMETHODCALLTYPE setColumnHeader(UINT uColumn, const char *szHeader) override;
	void XMETHODCALLTYPE setColumnWidth(UINT uColumn, UINT uWidth) override;

	void XMETHODCALLTYPE setRowCount(UINT uCount) override;
	UINT XMETHODCALLTYPE getRowCount() override;

	void XMETHODCALLTYPE setCell(UINT uColumn, UINT uRow, const char *szData) override;


	void XMETHODCALLTYPE setLabel(const char *szTitle) override;

	void XMETHODCALLTYPE setSelection(int iRow) override;
	int XMETHODCALLTYPE getSelection() override;

	gui::dom::IDOMnode* createNode(gui::dom::IDOMdocument *pDomDocument) override;
	void XMETHODCALLTYPE dispatchEvent(gui::IEvent *ev) override;

	void XMETHODCALLTYPE setEnabled(bool bEnable) override;

private:
	struct GridColumn
	{
		String sHeader;
		UINT uWidth = 100;
		gui::dom::IDOMnode *pNode = NULL;
		UINT uLeft;
	};

	struct GridCell
	{
		String sData;
		gui::dom::IDOMnode *pNode = NULL;
	};

	struct GridRow
	{
		Array<GridCell> aCells;
		gui::dom::IDOMnode *pNode = NULL;
	};

private:
	gui::dom::IDOMnode *m_pLabel = NULL;
	gui::dom::IDOMnode *m_pHeaderRow = NULL;
	gui::dom::IDOMnode *m_pItems = NULL;
	gui::dom::IDOMnode *m_pGridInner = NULL;

	Array<GridColumn> m_aHeaders;
	Array<GridRow> m_aRows;

	int m_iSelectedRow = -1;
	
	gui::dom::IDOMdocument *m_pDoc = NULL;

private:
	void cleanupNodes() override;

	void resize();
};

#endif
