#ifndef __IUIGRID_H
#define __IUIGRID_H

#include "IUIControl.h"

class IUIGrid: public IUIControl
{
public:
	virtual void XMETHODCALLTYPE setColumnCount(UINT uCount) = 0;
	virtual UINT XMETHODCALLTYPE getColumnCount() = 0; 
	virtual void XMETHODCALLTYPE setColumnHeader(UINT uColumn, const char *szHeader) = 0;
	virtual void XMETHODCALLTYPE setColumnWidth(UINT uColumn, UINT uWidth) = 0;

	virtual void XMETHODCALLTYPE setRowCount(UINT uCount) = 0;
	virtual UINT XMETHODCALLTYPE getRowCount() = 0;

	virtual void XMETHODCALLTYPE setCell(UINT uColumn, UINT uRow, const char *szData) = 0;

	virtual void XMETHODCALLTYPE setSelection(int iRow) = 0;
	virtual int XMETHODCALLTYPE getSelection() = 0;
};

#endif
