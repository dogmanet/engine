#ifndef __IUIVIEWPORT_H
#define __IUIVIEWPORT_H

#include <xcommon/render/IXRender.h>
#include "IUIControl.h"

class IUIViewport: public IUIControl
{
public:
	virtual void XMETHODCALLTYPE getRenderTarget(IXRenderTarget **ppOut) = 0;
};

#endif
