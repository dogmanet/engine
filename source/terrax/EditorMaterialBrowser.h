#ifndef __EDITORMATERIALBROWSER_H
#define __EDITORMATERIALBROWSER_H

#include <xcommon/editor/IXEditorMaterialBrowser.h>
#include "MaterialBrowser.h"

class CMaterialBrowserCallback: public IMaterialBrowserCallback
{
public:
	void init(IXEditorMaterialBrowserCallback *pCallback)
	{
		m_pCallback = pCallback;
	}
	void onSelected(const char *szName) override
	{
		SAFE_CALL(m_pCallback, onSelected, szName);
	}
	void onCancel() override
	{
		SAFE_CALL(m_pCallback, onCancel);
	}


private:
	IXEditorMaterialBrowserCallback *m_pCallback = NULL;
};

//#############################################################################

class CEditorMaterialBrowser: public IXUnknownImplementation<IXEditorMaterialBrowser>
{
public:
	//CEditorMaterialBrowser();
	//~CEditorMaterialBrowser();

	const char* XMETHODCALLTYPE getCurrentMaterial() override;
	void XMETHODCALLTYPE setCurrentMaterial(const char *szMaterial) override;

	void XMETHODCALLTYPE getCurrentMaterialInfo(IXMaterial **ppMat, IXTexture **ppTex) override;

	UINT XMETHODCALLTYPE getRecentMaterialCount() override;
	const char* XMETHODCALLTYPE getRecentMaterial(UINT idx) override;

	void XMETHODCALLTYPE browse(IXEditorMaterialBrowserCallback *pCallback) override;
	void XMETHODCALLTYPE abort() override;

private:
	CMaterialBrowserCallback m_callback;
};

#endif
