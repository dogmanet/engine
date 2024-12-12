#ifndef XEDITORIMPORTER_H_
#define XEDITORIMPORTER_H_

#include <gdefines.h>
#include "IXEditor.h"

// {41D1F10E-B1E4-454E-8696-A6CD513BDE5C}
#define IXEDITORIMPORTER_GUID DEFINE_XGUID(0x41d1f10e, 0xb1e4, 0x454e, 0x86, 0x96, 0xa6, 0xcd, 0x51, 0x3b, 0xde, 0x5c)
#define IXEDITORIMPORTER_VERSION 1

class IXEditorImporter: public IXUnknown
{
public:
	virtual void XMETHODCALLTYPE startup(IXEditor *pEditor) = 0;
	virtual void XMETHODCALLTYPE shutdown() = 0;
	
	virtual const char* XMETHODCALLTYPE getName() = 0;
	
	virtual void XMETHODCALLTYPE startImport() = 0;
};

#endif
