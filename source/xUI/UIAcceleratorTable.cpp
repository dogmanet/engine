#include "UIAcceleratorTable.h"

UINT XMETHODCALLTYPE CUIAcceleratorTable::addItem(const XAccelItem &accel, const char *szCommand)
{
	UINT uIdx = m_aItems.size();
	m_aItems[uIdx].accel = accel;
	m_aItems[uIdx].sCommand = szCommand;
	return(uIdx);
}

UINT XMETHODCALLTYPE CUIAcceleratorTable::getItemCount()
{
	return(m_aItems.size());
}

const XAccelItem* XMETHODCALLTYPE CUIAcceleratorTable::getItemInfo(UINT uIdx)
{
	Item *pItem = getItem(uIdx);
	if(pItem)
	{
		return(&pItem->accel);
	}
	return(NULL);
}
const char* XMETHODCALLTYPE CUIAcceleratorTable::getItemCommand(UINT uIdx)
{
	Item *pItem = getItem(uIdx);
	if(pItem)
	{
		return(pItem->sCommand.c_str());
	}
	return(NULL);
}

void XMETHODCALLTYPE CUIAcceleratorTable::setItemInfo(UINT uIdx, const XAccelItem &accel)
{
	Item *pItem = getItem(uIdx);
	if(pItem)
	{
		pItem->accel = accel;
	}
}
void XMETHODCALLTYPE CUIAcceleratorTable::setItemCommand(UINT uIdx, const char *szCommand)
{
	Item *pItem = getItem(uIdx);
	if(pItem)
	{
		pItem->sCommand = szCommand;
	}
}

CUIAcceleratorTable::Item* CUIAcceleratorTable::getItem(UINT uIdx)
{
	assert(uIdx < m_aItems.size());
	if(uIdx < m_aItems.size())
	{
		return(&m_aItems[uIdx]);
	}
	return(NULL);
}
