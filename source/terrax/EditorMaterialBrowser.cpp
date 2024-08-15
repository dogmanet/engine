#include "EditorMaterialBrowser.h"
#include "terrax.h"

extern HWND g_hComboCurrentMatWnd;

const char* XMETHODCALLTYPE CEditorMaterialBrowser::getCurrentMaterial()
{
	return(g_matBrowserCallback.getSelection());
}

void XMETHODCALLTYPE CEditorMaterialBrowser::setCurrentMaterial(const char *szMaterial)
{
	g_matBrowserCallback.onSelected(szMaterial);
}

void XMETHODCALLTYPE CEditorMaterialBrowser::getCurrentMaterialInfo(IXMaterial **ppMat, IXTexture **ppTex)
{
	g_matBrowserCallback.getInfo(ppMat, ppTex);
}

UINT XMETHODCALLTYPE CEditorMaterialBrowser::getRecentMaterialCount()
{
	return(g_matBrowserCallback.getRecentMaterialCount());
}
const char* XMETHODCALLTYPE CEditorMaterialBrowser::getRecentMaterial(UINT idx)
{
	return(g_matBrowserCallback.getRecentMaterial(idx));
}

void XMETHODCALLTYPE CEditorMaterialBrowser::browse(IXEditorMaterialBrowserCallback *pCallback)
{
	m_callback.init(pCallback);
	g_pMaterialBrowser->browse(&m_callback);
}

void XMETHODCALLTYPE CEditorMaterialBrowser::abort()
{
	g_pMaterialBrowser->abort();
}
