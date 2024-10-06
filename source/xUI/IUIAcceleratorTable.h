#ifndef __IUIACCELERATORTABLE_H
#define __IUIACCELERATORTABLE_H

#include <xcommon/editor/IXEditorExtension.h>

class IUIAcceleratorTable: public IXUnknown
{
public:
	virtual UINT XMETHODCALLTYPE addItem(const XAccelItem &accel, const char *szCommand) = 0;

	virtual UINT XMETHODCALLTYPE getItemCount() = 0;

	virtual const XAccelItem* XMETHODCALLTYPE getItemInfo(UINT uIdx) = 0;
	virtual const char* XMETHODCALLTYPE getItemCommand(UINT uIdx) = 0;
	
	virtual void XMETHODCALLTYPE setItemInfo(UINT uIdx, const XAccelItem &accel) = 0;
	virtual void XMETHODCALLTYPE setItemCommand(UINT uIdx, const char *szCommand) = 0;
};

#endif
