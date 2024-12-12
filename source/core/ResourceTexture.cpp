#include "ResourceTexture.h"
#include "ResourceManager.h"

CResourceTexture2D::CResourceTexture2D(CResourceManager *pMgr):
	BaseClass(GXTEXTURE_TYPE_2D, pMgr)
{
}

UINT XMETHODCALLTYPE CResourceTexture2D::getWidth() const
{
	return(m_uWidth);
}
UINT XMETHODCALLTYPE CResourceTexture2D::getHeight() const
{
	return(m_uHeight);
}

void XMETHODCALLTYPE CResourceTexture2D::init(UINT uWidth, UINT uHeight, GXFORMAT format, UINT uMipmapCount, UINT uFrameCount)
{
	assert(uWidth);
	assert(uHeight);
	assert(uMipmapCount > 0);
	assert(uFrameCount > 0);

	switch(format)
	{
	case GXFMT_DXT1_SRGB:
		format = GXFMT_DXT1;
		m_isSRGB = true;
		break;
	case GXFMT_DXT3_SRGB:
		format = GXFMT_DXT3;
		m_isSRGB = true;
		break;
	case GXFMT_DXT5_SRGB:
		format = GXFMT_DXT5;
		m_isSRGB = true;
		break;
	case GXFMT_X8R8G8B8_SRGB:
		format = GXFMT_X8R8G8B8;
		m_isSRGB = true;
		break;
	case GXFMT_A8B8G8R8_SRGB:
		format = GXFMT_A8B8G8R8;
		m_isSRGB = true;
		break;
	}

	m_format = format;
	m_uFrameCount = uFrameCount;
	m_uWidth = uWidth;
	m_uHeight = uHeight;

	mem_delete_a(pDataBlob);

	if(uMipmapCount == IXRESOURCE_TEXTURE_AUTO_MIPS)
	{
		if(isBlockCompressed(format))
		{
			uMipmapCount = 1;
		}
		else
		{
			UINT uSize = max(uWidth, uHeight);
			uMipmapCount = 1;
			while((uSize >>= 1) >= 1)
			{
				++uMipmapCount;
			}
		}
	}
	
	m_uMipmapCount = uMipmapCount;

	size_t sizeData = uFrameCount * (sizeof(void*) + uMipmapCount * sizeof(XImageMip));
	UINT uTmpWidth = uWidth;
	UINT uTmpHeight = uHeight;
	for(UINT i = 0; i < uMipmapCount; ++i)
	{
		sizeData += getTextureBytes(format, uTmpWidth, uTmpHeight) * uFrameCount;
		uTmpWidth >>= 1;
		uTmpHeight >>= 1;
		uTmpWidth = max(uTmpWidth, 1);
		uTmpHeight = max(uTmpHeight, 1);
	}

	pDataBlob = new byte[sizeData];

	m_ppData = (XImageMip**)pDataBlob;
	byte *pTmp = pDataBlob + sizeof(void*) * uFrameCount;
	for(UINT i = 0; i < uFrameCount; ++i)
	{
		m_ppData[i] = (XImageMip*)pTmp;
		pTmp += sizeof(XImageMip) * uMipmapCount;
	}

	for(UINT i = 0; i < uFrameCount; ++i)
	{
		uTmpWidth = uWidth;
		uTmpHeight = uHeight;

		for(UINT j = 0; j < uMipmapCount; ++j)
		{
			m_ppData[i][j].pData = pTmp;
			m_ppData[i][j].sizeData = getTextureBytes(format, uTmpWidth, uTmpHeight);
			m_ppData[i][j].isWritten = false;
			pTmp += m_ppData[i][j].sizeData;
			uTmpWidth >>= 1;
			uTmpHeight >>= 1;
			uTmpWidth = max(uTmpWidth, 1);
			uTmpHeight = max(uTmpHeight, 1);
		}
	}
}

XImageMip* XMETHODCALLTYPE CResourceTexture2D::getMip(UINT uMipmap, UINT uFrame)
{
	assert(m_ppData);
	assert(m_uMipmapCount > uMipmap);
	assert(m_uFrameCount > uFrame);
	if(!m_ppData || m_uMipmapCount <= uMipmap || m_uFrameCount <= uFrame)
	{
		return(NULL);
	}
	return(&m_ppData[uFrame][uMipmap]);
}
const XImageMip* XMETHODCALLTYPE CResourceTexture2D::getMip(UINT uMipmap, UINT uFrame) const
{
	assert(m_ppData);
	assert(m_uMipmapCount > uMipmap);
	assert(m_uFrameCount > uFrame);
	if(!m_ppData || m_uMipmapCount <= uMipmap || m_uFrameCount <= uFrame)
	{
		return(NULL);
	}
	return(&m_ppData[uFrame][uMipmap]);
}

const IXResourceTexture2D* XMETHODCALLTYPE CResourceTexture2D::as2D() const
{
	return(this);
}
IXResourceTexture2D* XMETHODCALLTYPE CResourceTexture2D::as2D()
{
	return(this);
}

void XMETHODCALLTYPE CResourceTexture2D::clone(IXResourceTexture **ppOut) const
{
	IXResourceTexture2D *pRes = m_pManager->newResourceTexture2D();
	*ppOut = pRes;
	cloneBaseData((CResourceTextureImpl<CResourceTexture2D>*)pRes);
	pRes->init(m_uWidth, m_uHeight, m_format, m_uMipmapCount, m_uFrameCount);

	for(UINT i = 0, l = m_uFrameCount; i < l; ++i)
	{
		for(UINT j = 0, jl = m_uMipmapCount; j < jl; ++j)
		{
			const XImageMip *pSrcMip = getMip(j, i);
			XImageMip *pDstMip = pRes->getMip(j, i);
			pDstMip->isWritten = pSrcMip->isWritten;
			if(pSrcMip->isWritten)
			{
				memcpy(pDstMip->pData, pSrcMip->pData, pSrcMip->sizeData);
			}
		}
	}
}

void CResourceTexture2D::genMipmaps()
{
	UINT bpp = getTextureBytes(m_format, 1, 1);

	for(UINT i = 0, l = m_uFrameCount; i < l; ++i)
	{
		XImageMip *pSrcMip = NULL;
		UINT uSrcWidth = m_uWidth;
		UINT uSrcHeight = m_uHeight;
		
		for(UINT j = 0, jl = m_uMipmapCount; j < jl; ++j)
		{
			XImageMip *pMip = getMip(j, i);
			if(pSrcMip && !pMip->isWritten)
			{
				UINT uDstWidth = max(uSrcWidth >> 1, 1);
				UINT uDstHeight = max(uSrcHeight >> 1, 1);

				for(UINT y = 0; y < uDstHeight; ++y)
				{
					for(UINT x = 0; x < uDstWidth; ++x)
					{
						UINT uDst = (y * uDstWidth + x) * bpp;
						UINT uSrc0 = (y * 2 * uSrcWidth + x * 2) * bpp;
						UINT uSrc1 = (y * 2 * uSrcWidth + x * 2 + (uSrcWidth == 1 ? 0 : 1))* bpp;
						UINT uSrc2 = ((y * 2 + (uSrcHeight == 1 ? 0 : 1)) * uSrcWidth + x * 2) * bpp;
						UINT uSrc3 = ((y * 2 + (uSrcHeight == 1 ? 0 : 1)) * uSrcWidth + x * 2 + (uSrcWidth == 1 ? 0 : 1)) * bpp;

						for(UINT p = 0; p < bpp; ++p)
						{
							pMip->pData[uDst + p] = (
								pSrcMip->pData[uSrc0 + p] +
								pSrcMip->pData[uSrc1 + p] +
								pSrcMip->pData[uSrc2 + p] +
								pSrcMip->pData[uSrc3 + p] + 2
								) / 4;
						}
					}
				}

				pMip->isWritten = true;
			}

			if(pSrcMip)
			{
				uSrcWidth >>= 1;
				uSrcHeight >>= 1;
				uSrcWidth = max(uSrcWidth, 1);
				uSrcHeight = max(uSrcHeight, 1);
			}
			pSrcMip = pMip;
		}
	}
}

//##########################################################################

CResourceTextureCube::CResourceTextureCube(CResourceManager *pMgr):
	BaseClass(GXTEXTURE_TYPE_CUBE, pMgr)
{
}

UINT XMETHODCALLTYPE CResourceTextureCube::getSize() const
{
	return(m_uSize);
}

void XMETHODCALLTYPE CResourceTextureCube::init(UINT uSize, GXFORMAT format, UINT uMipmapCount, UINT uFrameCount)
{
	assert(uSize);
	assert(uMipmapCount > 0);

	m_format = format;
	m_uFrameCount = uFrameCount;
	m_uMipmapCount = uMipmapCount == IXRESOURCE_TEXTURE_AUTO_MIPS ? 1 : uMipmapCount;
	m_uSize = uSize;

	mem_delete_a(pDataBlob);

	if(uMipmapCount == IXRESOURCE_TEXTURE_AUTO_MIPS)
	{
		if(isBlockCompressed(format))
		{
			uMipmapCount = 1;
		}
		else
		{
			UINT uTmpSize = uSize;
			uMipmapCount = 1;
			while((uTmpSize >>= 1) >= 1)
			{
				++uMipmapCount;
			}
		}
	}

	size_t sizeData = uFrameCount * (sizeof(void*) + uMipmapCount * (sizeof(void*) + 6 * sizeof(XImageMip)));
	UINT uTmpSize = uSize;
	for(UINT i = 0; i < uMipmapCount; ++i)
	{
		sizeData += getTextureBytes(format, uTmpSize, uTmpSize) * 6 * uFrameCount;
		uTmpSize >>= 1;
		uTmpSize = max(uTmpSize, 1);
	}

	pDataBlob = new byte[sizeData];

	m_pppData = (XImageMip***)pDataBlob;
	byte *pTmp = pDataBlob + sizeof(void*) * uFrameCount;
	for(UINT i = 0; i < uFrameCount; ++i)
	{
		m_pppData[i] = (XImageMip**)pTmp;
		pTmp += sizeof(void*) * uMipmapCount;
		for(UINT mip = 0; mip < uMipmapCount; ++mip)
		{
			m_pppData[i][mip] = (XImageMip*)pTmp;
			pTmp += sizeof(XImageMip) * 6;
		}
	}

	for(UINT i = 0; i < uFrameCount; ++i)
	{
		uTmpSize = uSize;
		for(UINT j = 0; j < uMipmapCount; ++j)
		{
			for(int side = 0; side < 6; ++side)
			{
				m_pppData[i][j][side].pData = pTmp;
				m_pppData[i][j][side].sizeData = getTextureBytes(format, uTmpSize, uTmpSize);
				m_pppData[i][j][side].isWritten = false;
				pTmp += m_pppData[i][j][side].sizeData;
			}
			uTmpSize >>= 1;
			uTmpSize = max(uTmpSize, 1);
		}
	}
}

const IXResourceTextureCube* XMETHODCALLTYPE CResourceTextureCube::asCube() const
{
	return(this);
}

IXResourceTextureCube* XMETHODCALLTYPE CResourceTextureCube::asCube()
{
	return(this);
}

XImageMip* XMETHODCALLTYPE CResourceTextureCube::getMip(GXCUBEMAP_FACES face, UINT uMipmap, UINT uFrame)
{
	assert(m_pppData);
	assert(m_uMipmapCount > uMipmap);
	assert(m_uFrameCount > uFrame);
	if(!m_pppData || m_uMipmapCount <= uMipmap || m_uFrameCount <= uFrame)
	{
		return(NULL);
	}
	return(&m_pppData[uFrame][uMipmap][face]);
}
const XImageMip* XMETHODCALLTYPE CResourceTextureCube::getMip(GXCUBEMAP_FACES face, UINT uMipmap, UINT uFrame) const
{
	assert(m_pppData);
	assert(m_uMipmapCount > uMipmap);
	assert(m_uFrameCount > uFrame);
	if(!m_pppData || m_uMipmapCount <= uMipmap || m_uFrameCount <= uFrame)
	{
		return(NULL);
	}
	return(&m_pppData[uFrame][uMipmap][face]);
}

void XMETHODCALLTYPE CResourceTextureCube::clone(IXResourceTexture **ppOut) const
{
	IXResourceTextureCube *pRes = m_pManager->newResourceTextureCube();
	*ppOut = pRes;
	cloneBaseData((CResourceTextureImpl<IXResourceTextureCube>*)pRes);
	pRes->init(m_uSize, m_format, m_uMipmapCount, m_uFrameCount);

	for(UINT i = 0, l = m_uFrameCount; i < l; ++i)
	{
		for(UINT j = 0, jl = m_uMipmapCount; j < jl; ++j)
		{
			for(UINT k = 0; k < 6; ++k)
			{
				const XImageMip *pSrcMip = getMip((GXCUBEMAP_FACES)k, j, i);
				XImageMip *pDstMip = pRes->getMip((GXCUBEMAP_FACES)k, j, i);
				pDstMip->isWritten = pSrcMip->isWritten;
				if(pSrcMip->isWritten)
				{
					memcpy(pDstMip->pData, pSrcMip->pData, pSrcMip->sizeData);
				}
			}
		}
	}
}
