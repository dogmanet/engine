#ifndef __IXTEXTURE_H
#define __IXTEXTURE_H

#include <gdefines.h>
#include <graphix/graphix.h>

class IXTexture: public IXUnknown
{
public:
	virtual void XMETHODCALLTYPE getAPITexture(IGXBaseTexture **ppTexture, UINT uFrame = 0) = 0;
	virtual bool XMETHODCALLTYPE isReady() const = 0;

	virtual GXTEXTURE_TYPE XMETHODCALLTYPE getType() const = 0;

	virtual UINT XMETHODCALLTYPE getWidth() const = 0;
	virtual UINT XMETHODCALLTYPE getHeight() const = 0;
	virtual UINT XMETHODCALLTYPE getDepth() const = 0;

	virtual UINT XMETHODCALLTYPE getNumFrames() const = 0;
	virtual float XMETHODCALLTYPE getFrameTime() const = 0;

	virtual const char* XMETHODCALLTYPE getName() const = 0;
};

#endif
