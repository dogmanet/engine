#ifndef __IUICONTROL_H
#define __IUICONTROL_H

#include "IUICommand.h"
#include <gui/guimain.h>

// {843042B6-7D26-4ACE-BA65-F0BA94F4B84C}
#define IXUICONTROL_DOM_NODE_OUTER_GUID DEFINE_XGUID(0x843042b6, 0x7d26, 0x4ace, 0xba, 0x65, 0xf0, 0xba, 0x94, 0xf4, 0xb8, 0x4c)

// {A57B046B-37CD-4A59-96C6-A7BCFE458DB8}
#define IXUICONTROL_DOM_NODE_CONTAINER_GUID DEFINE_XGUID(0xa57b046b, 0x37cd, 0x4a59, 0x96, 0xc6, 0xa7, 0xbc, 0xfe, 0x45, 0x8d, 0xb8)

// {7C34C420-58F8-4958-A1D7-C9134B582428}
#define IXUICONTROL_UIWINDOW_GUID DEFINE_XGUID(0x7c34c420, 0x58f8, 0x4958, 0xa1, 0xd7, 0xc9, 0x13, 0x4b, 0x58, 0x24, 0x28)

class IUIControl: public IXUnknown
{
public:
	//! Метод установки надписи в контроле
	virtual void XMETHODCALLTYPE setLabel(const char *szTitle) = 0;

	//! Метод установки позиции в окне
	virtual void XMETHODCALLTYPE setPosition(float fPosX, float fPosY) = 0;

	//! Метод установки видимости для контрола
	virtual void XMETHODCALLTYPE setVisible(bool isVisible) = 0;

	//! Метод установки размера по высоте и ширине
	virtual void XMETHODCALLTYPE setSize(float fSizeX, float fSizeY) = 0;

	//! Метод позволяет "включить/выключить" контрол
	virtual void XMETHODCALLTYPE setEnabled(bool bEnable) = 0;

	//! Возвращает состояние контрола, включен ли он или нет
	virtual bool XMETHODCALLTYPE isEnabled() = 0;

	//! Метод устанавливает обработчик на клик по контролу
	//virtual void XMETHODCALLTYPE setClickCommand(IUICommand *pCommand) = 0;

	//! Возвращает ID элемента
	//virtual ULONG XMETHODCALLTYPE getElementID() const = 0;

	//! Возвращает родительский элемент
	virtual IUIControl* XMETHODCALLTYPE getParent() const = 0;
	virtual void XMETHODCALLTYPE setParent(IUIControl *pParent) = 0;

	virtual UINT XMETHODCALLTYPE getChildrenCount() const = 0;
	virtual IUIControl* XMETHODCALLTYPE getChild(UINT uIdx) const = 0;

	virtual void XMETHODCALLTYPE insertChild(IUIControl *pChild, UINT uPos = UINT_MAX) = 0;
	virtual void XMETHODCALLTYPE removeChild(IUIControl *pChild) = 0;

	virtual void XMETHODCALLTYPE dispatchEvent(gui::IEvent *ev) = 0;
};

#endif
