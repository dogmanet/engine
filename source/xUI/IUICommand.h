#ifndef __IUICOMMAND_H 
#define __IUICOMMAND_H 

#include <gdefines.h>

class IUIControl;
class IUICommand: public IXUnknown
{
public:
	virtual void execute(IUIControl *pTarget) = 0;
};

#endif
