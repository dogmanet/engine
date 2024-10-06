#ifndef __UIACCELERATORTABLE_H
#define __UIACCELERATORTABLE_H

#include "IUIAcceleratorTable.h"
#include "IUIWindow.h"

class CUIAcceleratorTable: public IXUnknownImplementation<IUIAcceleratorTable>
{
public:
	UINT XMETHODCALLTYPE addItem(const XAccelItem &accel, const char *szCommand) override;

	UINT XMETHODCALLTYPE getItemCount() override;

	const XAccelItem* XMETHODCALLTYPE getItemInfo(UINT uIdx) override;
	const char* XMETHODCALLTYPE getItemCommand(UINT uIdx) override;

	void XMETHODCALLTYPE setItemInfo(UINT uIdx, const XAccelItem &accel) override;
	void XMETHODCALLTYPE setItemCommand(UINT uIdx, const char *szCommand) override;

private:
	struct Item
	{
		XAccelItem accel;
		String sCommand;
	};
	Array<Item> m_aItems;

private:
	Item* getItem(UINT uIdx);
};

#endif
