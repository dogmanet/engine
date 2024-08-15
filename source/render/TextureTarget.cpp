#include "TextureTarget.h"
#include "Render.h"
#include "RenderGraph.h"

CTextureTarget::CTextureTarget(CRender *pRender, IGXDevice *pDevice, IXMaterialSystem *pMaterialSystem):
	BaseClass(pRender, pDevice),
	m_pMaterialSystem(pMaterialSystem)
{
	XCreateGUID(&m_guid);
}
CTextureTarget::~CTextureTarget()
{
	mem_release(m_pTexture);
	mem_release(m_pTex);
}

bool XMETHODCALLTYPE CTextureTarget::getTarget(IGXSurface **ppTarget)
{
	if(m_pTexture)
	{
		*ppTarget = m_pTexture->asRenderTarget();
		return(true);
	}

	return(false);
}

void XMETHODCALLTYPE CTextureTarget::resize(UINT uWidth, UINT uHeight)
{
	// resize targets and swap chain
	mem_release(m_pTexture);
	m_pTexture = m_pDevice->createTexture2D(uWidth, uHeight, 0, GX_TEXFLAG_RENDERTARGET, GXFMT_A8B8G8R8);

	char tmp[128];
	getTextureName(tmp, sizeof(tmp));
	m_pMaterialSystem->addTexture(tmp, m_pTexture, &m_pTex);


	BaseClass::resize(uWidth, uHeight);
}

void XMETHODCALLTYPE CTextureTarget::FinalRelease()
{
	m_pRender->onTextureTargetReleased(this);
}

void CTextureTarget::getTextureName(char *szTextureName, size_t sizeTextureNameBuffer)
{
	int iWritten = sprintf_s(szTextureName, sizeTextureNameBuffer, "@final/");

	XGUIDToSting(m_guid, szTextureName + iWritten, sizeTextureNameBuffer - iWritten);

	szTextureName[sizeTextureNameBuffer - 1] = 0;
}
