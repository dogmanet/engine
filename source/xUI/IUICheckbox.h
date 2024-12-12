#ifndef __IUICHECKBOX_H
#define __IUICHECKBOX_H

#include "IUIControl.h"

class IUICheckbox: public IUIControl
{
public:
	virtual void XMETHODCALLTYPE setChecked(bool set) = 0;
	virtual bool XMETHODCALLTYPE isChecked() = 0;
};

#endif
