#ifndef __IUICOLOR_H
#define __IUICOLOR_H

#include "IUIControl.h"

class IUIColor: public IUIControl
{
public:
	virtual void XMETHODCALLTYPE setValue(const float4_t &vColor) = 0;
	virtual float4_t XMETHODCALLTYPE getValue() = 0;
};

#endif
