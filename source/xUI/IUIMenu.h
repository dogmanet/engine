#ifndef __IUIMENU_H
#define __IUIMENU_H

#include "IUIControl.h"

class IUIMenu: public IUIControl
{
public:
	virtual void XMETHODCALLTYPE show(int x, int y) = 0;
	virtual void XMETHODCALLTYPE hide() = 0;

	virtual UINT XMETHODCALLTYPE addSeparator() = 0;
	virtual void XMETHODCALLTYPE insertSeparator(UINT uIdx) = 0;

	virtual UINT XMETHODCALLTYPE addItem(const char *szText, const char *szCommand = NULL, const char *szIcon = NULL, IUIMenu *pSubmenu = NULL) = 0;
	virtual void XMETHODCALLTYPE insertItem(UINT uIdx, const char *szText, const char *szCommand = NULL, const char *szIcon = NULL, IUIMenu *pSubmenu = NULL) = 0;

	virtual UINT XMETHODCALLTYPE getItemCount() = 0;

	virtual const char* XMETHODCALLTYPE getItemText(UINT uIdx) = 0;
	virtual const char* XMETHODCALLTYPE getItemIcon(UINT uIdx) = 0;
	virtual const char* XMETHODCALLTYPE getItemCommand(UINT uIdx) = 0;
	virtual bool XMETHODCALLTYPE isItemEnabled(UINT uIdx) = 0;
	virtual IUIMenu* XMETHODCALLTYPE getItemSubmenu(UINT uIdx) = 0;
	
	virtual void XMETHODCALLTYPE setItemText(UINT uIdx, const char *szText) = 0;
	virtual void XMETHODCALLTYPE setItemIcon(UINT uIdx, const char *szIcon) = 0;
	virtual void XMETHODCALLTYPE setItemCommand(UINT uIdx, const char *szCommand) = 0;
	virtual void XMETHODCALLTYPE setItemEnabled(UINT uIdx, bool isEnabled) = 0;
	virtual void XMETHODCALLTYPE setItemSubmenu(UINT uIdx, IUIMenu *pSubmenu) = 0;

	virtual bool XMETHODCALLTYPE isSeparator(UINT uIdx) = 0;
};

#endif
