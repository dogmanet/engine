#ifndef __IUIMINMAXCURVE_H
#define __IUIMINMAXCURVE_H

#include "IUIControl.h"

class IXMinMaxCurve;
class IUIMinMaxCurve: public IUIControl
{
public:
	virtual void XMETHODCALLTYPE setCurve(IXMinMaxCurve *pCurve) = 0;
};

#endif
