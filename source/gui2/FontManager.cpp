#include "FontManager.h"
#include <xcommon/IPluginManager.h>

CFontManager::CFontManager(IXCore *pCore, IGXDevice *pDev, IFileSystem *pFileSystem):
	m_pDev(pDev),
	m_pFileSystem(pFileSystem)
{
	if(FT_Init_FreeType(&m_pFT))
	{
		LibReport(REPORT_MSG_LEVEL_FATAL, "Failed to initialise FreeType library\n");
	}

	scanFonts();

	GXVertexElement oLayout[] =
	{
		{0, 0, GXDECLTYPE_FLOAT2, GXDECLUSAGE_POSITION},
		{0, 8, GXDECLTYPE_FLOAT2, GXDECLUSAGE_TEXCOORD},
		GX_DECL_END()
	};
	m_pVertexDeclaration = m_pDev->createVertexDeclaration(oLayout);

	m_pRenderUtils = (IXRenderUtils*)pCore->getPluginManager()->getInterface(IXRENDERUTILS_GUID);
}

CFontManager::~CFontManager()
{
	if(m_pFT)
	{
		m_mFonts.clear();
		FT_Done_FreeType(m_pFT);
		m_pFT = NULL;
	}

	mem_release(m_pVertexDeclaration);
}

bool XMETHODCALLTYPE CFontManager::getFont(IXFont **ppOut, const char *szFile, UINT uSize, XFONT_STYLE style, int iBlurRadius)
{
	char tmp[256];
	sprintf(tmp, "%s_%u+%d-%d", basename(szFile), uSize, (int)style, iBlurRadius);

	const AssotiativeArray<String, CFont>::Node *pNode;
	if(m_mFonts.KeyExists(tmp, &pNode))
	{
		pNode->Val->AddRef();
		*ppOut = pNode->Val;
		return(true);
	}
	m_mFonts[tmp];
	m_mFonts.TmpNode->Val->load(this, m_pFileSystem, m_pFT, m_pDev, m_mFonts.TmpNode->Key.c_str(), szFile, uSize, style, iBlurRadius);
	
	*ppOut = m_mFonts.TmpNode->Val;
	return(true);
}

void CFontManager::onFontReleased(CFont *pFont)
{
	const char *szIdent = pFont->getIdent();

	assert(m_mFonts.KeyExists(szIdent));
	
	m_mFonts.erase(szIdent);
}

#if 0
FT_Library CFontManager::requestFT()
{
	return(m_pFT);
}
#endif

void XMETHODCALLTYPE CFontManager::getFontIndexBuffer(UINT uChars, IGXIndexBuffer **ppIndexBuffer)
{
	m_pRenderUtils->getQuadIndexBuffer(uChars, ppIndexBuffer);
}

void XMETHODCALLTYPE CFontManager::getFontVertexDeclaration(IGXVertexDeclaration **ppVertexDeclaration)
{
	m_pVertexDeclaration->AddRef();
	*ppVertexDeclaration = m_pVertexDeclaration;
}

void CFontManager::scanFonts()
{
	const char *aszExts[] = {
		"ttf", "otf", "woff"
	};
	IFileIterator *pIter = m_pFileSystem->getFileListRecursive("fonts/", aszExts, ARRAYSIZE(aszExts));
	if(pIter)
	{
		const char *szFile;
		while((szFile = pIter->next()))
		{
			// process file
		}
		mem_release(pIter);
	}
}
