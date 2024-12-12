#ifndef __IXCOLOR_GRADIENT_EDITOR_H
#define __IXCOLOR_GRADIENT_EDITOR_H

#include <gdefines.h>
#include <xcommon/util/IXColorGradient.h>

class IXColorGradientEditorCallback
{
public:
	virtual void XMETHODCALLTYPE onAccept() = 0;
	virtual void XMETHODCALLTYPE onCancel() = 0;
};

//##########################################################################

// {8EE07DE7-6109-4F98-B395-518953BF86E0}
#define IXCOLORGRADIENTEDITOR_GUID DEFINE_XGUID(0x8ee07de7, 0x6109, 0x4f98, 0xb3, 0x95, 0x51, 0x89, 0x53, 0xbf, 0x86, 0xe0)

class IXColorGradientEditor: public IXUnknown
{
public:
	virtual void XMETHODCALLTYPE edit(IXColorGradient *pGradient, IXColorGradientEditorCallback *pCallback) = 0;
	virtual void XMETHODCALLTYPE abort() = 0;
};


#endif
