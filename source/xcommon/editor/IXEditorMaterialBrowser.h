#ifndef __XEDITORMATERIALBROWSER_H
#define __XEDITORMATERIALBROWSER_H

#include <gdefines.h>

class IXEditorMaterialBrowserCallback
{
public:
	virtual void XMETHODCALLTYPE onSelected(const char *szName) = 0;
	virtual void XMETHODCALLTYPE onCancel() = 0;
};

//##########################################################################

class IXMaterial;
class IXTexture;
class IXEditorMaterialBrowser: public IXUnknown
{
public:
	virtual const char* XMETHODCALLTYPE getCurrentMaterial() = 0;
	virtual void XMETHODCALLTYPE setCurrentMaterial(const char *szMaterial) = 0;

	virtual void XMETHODCALLTYPE getCurrentMaterialInfo(IXMaterial **ppMat, IXTexture **ppTex) = 0;

	virtual UINT XMETHODCALLTYPE getRecentMaterialCount() = 0;
	virtual const char* XMETHODCALLTYPE getRecentMaterial(UINT idx) = 0;

	virtual void XMETHODCALLTYPE browse(IXEditorMaterialBrowserCallback *pCallback) = 0;
	virtual void XMETHODCALLTYPE abort() = 0;
};


#endif
