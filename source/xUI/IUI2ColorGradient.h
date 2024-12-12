#ifndef __IUI2COLORGRADIENT_H
#define __IUI2COLORGRADIENT_H

#include "IUIControl.h"

class IX2ColorGradients;
class IUI2ColorGradient: public IUIControl
{
public:
	virtual void XMETHODCALLTYPE setGradient(IX2ColorGradients *pGradient) = 0;
};

#endif
