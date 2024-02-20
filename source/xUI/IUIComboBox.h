#ifndef __IUICOMBOBOX_H
#define __IUICOMBOBOX_H

#include "IUIControl.h"

class IUIComboBox: public IUIControl
{
public:
	//Метод возвращает содержимое контрола
	//char* XMETHODCALLTYPE getText();
	virtual void XMETHODCALLTYPE addItem(const char *szName, const char *szValue) = 0;
	virtual void XMETHODCALLTYPE removeItems() = 0;

	virtual void XMETHODCALLTYPE setValue(const char *szValue) = 0;
	virtual const char* XMETHODCALLTYPE getValue() = 0;
};

#endif
