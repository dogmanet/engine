#include "TextureLoader.h"
#include "DDSFile.h"

CTextureLoader::CTextureLoader(IFileSystem *pFileSystem):
	m_pFileSystem(pFileSystem)
{
}

UINT XMETHODCALLTYPE CTextureLoader::getVersion()
{
	return(IXTEXTURELOADER_VERSION);
}

UINT XMETHODCALLTYPE CTextureLoader::getExtCount() const
{
	return(1);
}
const char* XMETHODCALLTYPE CTextureLoader::getExt(UINT uIndex) const
{
	assert(uIndex < getExtCount());
	switch(uIndex)
	{
	case 0:
		return("dds");
	}
	return(NULL);
}
const char* XMETHODCALLTYPE CTextureLoader::getExtText(UINT uIndex) const
{
	assert(uIndex < getExtCount());
	switch(uIndex)
	{
	case 0:
		return("DirectDraw Surface");
	}
	return(NULL);
}
const char* XMETHODCALLTYPE CTextureLoader::getAuthor() const
{
	return("D-AIRY @ DogmaNet");
}
const char* XMETHODCALLTYPE CTextureLoader::getCopyright() const
{
	return("Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2019");
}
const char* XMETHODCALLTYPE CTextureLoader::getDescription() const
{
	return("DDS texture loader");
}

void XMETHODCALLTYPE CTextureLoader::getInfo(XTextureInfo *pTextureInfo)
{
	pTextureInfo->type = getType();
	pTextureInfo->format = m_format;
	
	pTextureInfo->uWidth = m_ddsHeader.width / m_iXFrames;
	pTextureInfo->uHeight = m_ddsHeader.height / m_iYFrames;
	pTextureInfo->uDepth = m_ddsHeader.depth;

	pTextureInfo->uFrameCount = (UINT)(m_iXFrames * m_iYFrames);
	pTextureInfo->fFrameTime = m_fFrameTime;

	pTextureInfo->uMipCount = 1;
	if(m_ddsHeader.flags & DDS_HEADER_FLAGS_MIPMAP)
	{
		pTextureInfo->uMipCount = m_ddsHeader.mipMapCount;
		if(m_iXFrames != 1 || m_iYFrames != 1)
		{
			UINT uMaxMips = 1;
			UINT uSize = min(pTextureInfo->uWidth, pTextureInfo->uHeight);
			while((uSize >>= 1))
			{
				++uMaxMips;
			}
			if(uMaxMips < pTextureInfo->uMipCount)
			{
				pTextureInfo->uMipCount = uMaxMips;
			}
		}
	}
}

bool XMETHODCALLTYPE CTextureLoader::open(const char *szFileName, const char *szArg)
{
	assert(!m_pCurrentFile && "File already opened!");
	if(m_pCurrentFile)
	{
		return(false);
	}
	
	m_pCurrentFile = m_pFileSystem->openFile(szFileName);
	if(!m_pCurrentFile)
	{
		return(false);
	}

	IFile *pFile = m_pCurrentFile;

	uint32_t uMagick = 0;
	memset(&m_ddsHeader, 0, sizeof(m_ddsHeader));
	memset(&m_dxt10Header, 0, sizeof(m_dxt10Header));
	
	pFile->readBin(&uMagick, sizeof(uMagick));
	if(uMagick != DDS_MAGIC)
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "Invalid magick number '%s'\n", szFileName);
		mem_release(m_pCurrentFile);
		return(false);
	}

	if(pFile->readBin(&m_ddsHeader, sizeof(m_ddsHeader)) != sizeof(m_ddsHeader))
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "Invalid file header '%s'\n", szFileName);
		mem_release(m_pCurrentFile);
		return(false);
	}
	m_hasDXT10Header = (m_ddsHeader.ddspf.flags & DDS_FOURCC) && m_ddsHeader.ddspf.fourCC == MAKEFOURCC('D', 'X', '1', '0');
	
	if(m_hasDXT10Header && pFile->readBin(&m_dxt10Header, sizeof(m_dxt10Header)) != sizeof(m_dxt10Header))
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "Invalid file header 10 '%s'\n", szFileName);
		mem_release(m_pCurrentFile);
		return(false);
	}

	m_format = getFormat();
	if(m_format == GXFMT_UNKNOWN)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Unsuported texture format '%s'\n", szFileName);
		mem_release(m_pCurrentFile);
		return(false);
	}

	if(isBlockCompressed(m_format))
	{
		m_ddsHeader.width = max(1, ((m_ddsHeader.width + 3) / 4) * 4);
		m_ddsHeader.height = max(1, ((m_ddsHeader.height + 3) / 4) * 4);
	}

	m_iXFrames = 1;
	m_iYFrames = 1;
	m_fFrameTime = 0.0f;
	m_iSkipFrames = 0;

	size_t sizeName = strlen(szFileName);
	char *szMetaFile = (char*)alloca(sizeof(char) * (sizeName + 2));
	memcpy(szMetaFile, szFileName, sizeof(char) * (sizeName - 3));
	szMetaFile[sizeName - 3] = 'm';
	szMetaFile[sizeName - 2] = 'e';
	szMetaFile[sizeName - 1] = 't';
	szMetaFile[sizeName + 0] = 'a';
	szMetaFile[sizeName + 1] = 0;
	
	if((pFile = m_pFileSystem->openFile(szMetaFile)))
	{
		size_t sizeFile = pFile->getSize();

		if(sizeFile <= 4096)
		{
			char *szFileContent = (char*)alloca(sizeof(char) * (sizeFile + 1));
			char *tmp = szFileContent;

			bool isComment = false;
			for(size_t i = 0; i < sizeFile; ++i)
			{
				int ch = pFile->readChar();
				if(ch == '#')
				{
					isComment = true;
				}
				else if(ch == '\n')
				{
					isComment = false;
				}
				if(isComment)
				{
					continue;
				}

				*tmp++ = (char)ch;
			}
			*tmp = 0;

			// # xFrames:(int) yFrames(int) frameTime(float) skipFrames(int)
			int x, y, s;
			float t;
			int iRes = sscanf(szFileContent, "%d %d %f %d", &x, &y, &t, &s);
			if(iRes >= 2)
			{
				m_iXFrames = x;
				m_iYFrames = y;

				if(iRes >= 3)
				{
					m_fFrameTime = t;
				}

				if(iRes >= 4)
				{
					m_iSkipFrames = s;
				}

				if(m_iXFrames <= 0)
				{
					LibReport(REPORT_MSG_LEVEL_WARNING, "Invalid texture frame count (%d) defaulting to 1: '%s'\n", m_iXFrames, szMetaFile);
					m_iXFrames = 1;
				}
				if(m_iYFrames <= 0)
				{
					LibReport(REPORT_MSG_LEVEL_WARNING, "Invalid texture frame count (%d) defaulting to 1: '%s'\n", m_iYFrames, szMetaFile);
					m_iYFrames = 1;
				}
				if(m_iSkipFrames < 0 || m_iSkipFrames >= m_iXFrames * m_iYFrames)
				{
					LibReport(REPORT_MSG_LEVEL_WARNING, "Invalid texture skip frames (%d) defaulting to 0: '%s'\n", m_iSkipFrames, szMetaFile);
					m_iSkipFrames = 0;
				}

				if((m_ddsHeader.width % m_iXFrames) != 0)
				{
					LibReport(REPORT_MSG_LEVEL_WARNING, "Invalid texture X frames (%d) bad divider, defaulting to 1: '%s'\n", m_iXFrames, szMetaFile);
					m_iXFrames = 1;
				}
				if((m_ddsHeader.height % m_iYFrames) != 0)
				{
					LibReport(REPORT_MSG_LEVEL_WARNING, "Invalid texture Y frames (%d) bad divider, defaulting to 1: '%s'\n", m_iYFrames, szMetaFile);
					m_iYFrames = 1;
				}

				if(isBlockCompressed(m_format))
				{
					if(((m_ddsHeader.width / m_iXFrames) % 4) != 0)
					{
						LibReport(REPORT_MSG_LEVEL_WARNING, "Invalid texture X frames (%d) frame size must be multiply of 4, defaulting to 1: '%s'\n", m_iXFrames, szMetaFile);
						m_iXFrames = 1;
					}
					if(((m_ddsHeader.height / m_iYFrames) % 4) != 0)
					{
						LibReport(REPORT_MSG_LEVEL_WARNING, "Invalid texture Y frames (%d) frame size must be multiply of 4, defaulting to 1: '%s'\n", m_iYFrames, szMetaFile);
						m_iYFrames = 1;
					}
				}
			}
			else
			{
				LibReport(REPORT_MSG_LEVEL_WARNING, "Invalid texture meta file: '%s'\n", m_iYFrames, szMetaFile);
			}
		}
		else
		{
			LibReport(REPORT_MSG_LEVEL_WARNING, "Too big texture meta file '%s'\n", szMetaFile);
		}

		mem_release(pFile);
	}

	return(true);
}
GXTEXTURE_TYPE XMETHODCALLTYPE CTextureLoader::getType() const
{
	if(m_ddsHeader.caps2 & DDS_FLAGS_VOLUME)
	{
		return(GXTEXTURE_TYPE_3D);
	}
	if(m_ddsHeader.caps2 & DDS_CUBEMAP)
	{
		return(GXTEXTURE_TYPE_CUBE);
	}
	if(m_ddsHeader.caps & DDS_SURFACE_FLAGS_TEXTURE)
	{
		return(GXTEXTURE_TYPE_2D);
	}
	/*if(m_ddsHeader.caps & DDS_SURFACE_FLAGS_CUBEMAP) // DDS_CUBEMAP
	{
		return(GXTEXTURE_TYPE_CUBE);
	}*/
	return(GXTEXTURE_TYPE_UNKNOWN);
}

bool XMETHODCALLTYPE CTextureLoader::loadAs2D(IXResourceTexture2D *pResource)
{
	if(getType() != GXTEXTURE_TYPE_2D)
	{
		return(false);
	}

	pResource->setFrameTime(m_fFrameTime);

	XTextureInfo info;
	getInfo(&info);

	bool isBC = isBlockCompressed(info.format);

	pResource->init(info.uWidth, info.uHeight, info.format, info.uMipCount == 1 ? IXRESOURCE_TEXTURE_AUTO_MIPS : info.uMipCount, (UINT)(m_iXFrames * m_iYFrames - m_iSkipFrames));

	byte *pData = new byte[pResource->getTextureBytes(info.format, m_ddsHeader.width, m_ddsHeader.height)];

	UINT uMipWidth = m_ddsHeader.width;
	UINT uMipHeight = m_ddsHeader.height;
	for(UINT i = 0; i < info.uMipCount; ++i)
	{
		size_t sizeMip = pResource->getTextureBytes(info.format, uMipWidth, uMipHeight);

		if(m_bConvertFromRGB24)
		{
			UINT uPixels = uMipWidth * uMipHeight;
			byte *pTmp = pData;
			for(UINT j = 0; j < uPixels; ++j)
			{
				for(UINT k = 0; k < 3; ++k)
				{
					if(m_pCurrentFile->readBin(pTmp++, 1) != 1)
					{
						LibReport(REPORT_MSG_LEVEL_ERROR, "Unexpected end of file\n");
						goto end;
					}
				}
				*pTmp++ = 0xFF;
			}
		}
		else
		{
			if(m_pCurrentFile->readBin(pData, sizeMip) != sizeMip)
			{
				LibReport(REPORT_MSG_LEVEL_ERROR, "Unexpected end of file\n");
				goto end;
			}
		}

		if(m_bConvertSwapRB)
		{
			UINT uPixels = uMipWidth * uMipHeight;
			byte tmp;
			for(UINT j = 0; j < uPixels; ++j)
			{
				tmp = pData[j * 4 + 0];
				pData[j * 4 + 0] = pData[j * 4 + 2];
				pData[j * 4 + 2] = tmp;
			}
		}

		// m_iXFrames
		// m_iYFrames

		if(m_iXFrames == 1 && m_iYFrames == 1)
		{
			XImageMip *pMip = pResource->getMip(i, 0);
			if(pMip)
			{
				assert(pMip->sizeData == sizeMip);

				memcpy(pMip->pData, pData, sizeMip);
				pMip->isWritten = true;
			}
		}
		else
		{
			int iXFrames = m_iXFrames;
			int iYFrames = m_iYFrames - m_iSkipFrames / m_iXFrames;
			int iSkipFrames = m_iSkipFrames % m_iXFrames;
			int iFrame = 0;
			for(int y = 0; y < iYFrames; ++y)
			{
				if(y == iYFrames - 1)
				{
					iXFrames -= iSkipFrames;
				}

				for(int x = 0; x < iXFrames; ++x)
				{
					XImageMip *pMip = pResource->getMip(i, iFrame);
					if(pMip)
					{
						size_t sizeRow = pResource->getTextureBytes(info.format, uMipWidth, 1);
						int iRowCount = uMipHeight / m_iYFrames;
						if(isBC)
						{
							iRowCount /= 4;
						}
						if(iRowCount == 0)
						{
							iRowCount = 1;
						}
						//size_t sizeRowFrame = sizeRow / m_iXFrames;
						size_t sizeRowFrame = pResource->getTextureBytes(info.format, max(1, uMipWidth / m_iXFrames), 1);

						assert(pMip->sizeData == sizeRowFrame * iRowCount);

						byte *pDest = pMip->pData;
						for(int row = 0; row < iRowCount; ++row)
						{
							memcpy(pDest, pData + sizeRow * (row + iRowCount * y) + sizeRowFrame * x, sizeRowFrame);
							pDest += sizeRowFrame;
						}

						pMip->isWritten = true;
					}

					++iFrame;
				}
			}
		}

		uMipWidth >>= 1;
		uMipHeight >>= 1;
		uMipWidth = max(uMipWidth, 1);
		uMipHeight = max(uMipHeight, 1);
	}

	mem_delete_a(pData);
	return(true);
end:
	mem_delete_a(pData);
	return(false);
}
bool XMETHODCALLTYPE CTextureLoader::loadAsCube(IXResourceTextureCube *pResource)
{
	if(getType() != GXTEXTURE_TYPE_CUBE)
	{
		return(false);
	}

	pResource->setFrameTime(m_fFrameTime);

	XTextureInfo info;
	getInfo(&info);

	bool isBC = isBlockCompressed(info.format);

	pResource->init(info.uWidth, info.format, info.uMipCount == 1 ? IXRESOURCE_TEXTURE_AUTO_MIPS : info.uMipCount);

	byte *pData = new byte[pResource->getTextureBytes(info.format, m_ddsHeader.width, m_ddsHeader.height)];

	UINT uSideFlags[] = {
		DDS_CUBEMAP_POSITIVEX, DDS_CUBEMAP_NEGATIVEX,
		DDS_CUBEMAP_POSITIVEY, DDS_CUBEMAP_NEGATIVEY,
		DDS_CUBEMAP_POSITIVEZ, DDS_CUBEMAP_NEGATIVEZ
	};

	for(int side = 0; side < 6; ++side)
	{
		if(m_ddsHeader.caps2 & uSideFlags[side])
		{
			UINT uMipSize = m_ddsHeader.width;
			for(UINT i = 0; i < info.uMipCount; ++i)
			{
				size_t sizeMip = pResource->getTextureBytes(info.format, uMipSize, uMipSize);

				if(m_bConvertFromRGB24)
				{
					UINT uPixels = uMipSize * uMipSize;
					byte *pTmp = pData;
					for(UINT j = 0; j < uPixels; ++j)
					{
						for(UINT k = 0; k < 3; ++k)
						{
							if(m_pCurrentFile->readBin(pTmp++, 1) != 1)
							{
								LibReport(REPORT_MSG_LEVEL_ERROR, "Unexpected end of file\n");
								goto end;
							}
						}
						*pTmp++ = 0xFF;
					}
				}
				else
				{
					if(m_pCurrentFile->readBin(pData, sizeMip) != sizeMip)
					{
						LibReport(REPORT_MSG_LEVEL_ERROR, "Unexpected end of file\n");
						goto end;
					}
				}

				if(m_bConvertSwapRB)
				{
					UINT uPixels = uMipSize * uMipSize;
					byte tmp;
					for(UINT j = 0; j < uPixels; ++j)
					{
						tmp = pData[j * 4 + 0];
						pData[j * 4 + 0] = pData[j * 4 + 2];
						pData[j * 4 + 2] = tmp;
					}
				}

				// m_iXFrames
				// m_iYFrames

				XImageMip *pMip = pResource->getMip((GXCUBEMAP_FACES)side, i, 0);
				if(pMip)
				{
					assert(pMip->sizeData == sizeMip);

					memcpy(pMip->pData, pData, sizeMip);
					pMip->isWritten = true;
				}

				uMipSize >>= 1;
				uMipSize = max(uMipSize, 1);
			}
		}
	}
	mem_delete_a(pData);
	return(true);
end:
	mem_delete_a(pData);
	return(false);
}

void XMETHODCALLTYPE CTextureLoader::close()
{
	mem_release(m_pCurrentFile);
}

bool CTextureLoader::isBlockCompressed(GXFORMAT format)
{
	switch(format)
	{
	case GXFMT_DXT1:
	case GXFMT_DXT3:
	case GXFMT_DXT5:
	case GXFMT_DXT1_SRGB:
	case GXFMT_DXT3_SRGB:
	case GXFMT_DXT5_SRGB:
	case GXFMT_ATI1N:
	case GXFMT_ATI2N:
		return(true);

	default:
		return(false);
	}
}

GXFORMAT CTextureLoader::getFormat()
{
	m_bConvertFromRGB24 = false;
	m_bConvertSwapRB = false;

	if(m_ddsHeader.ddspf.flags == DDS_RGB)
	{
		if(m_ddsHeader.ddspf.RGBBitCount == 16)
		{
			if(m_ddsHeader.ddspf.RBitMask == 0xf800 && m_ddsHeader.ddspf.GBitMask == 0x7e0 && m_ddsHeader.ddspf.BBitMask == 0x1f)
			{
				return(GXFMT_R5G6B5);
			}
		}
		else if(m_ddsHeader.ddspf.RGBBitCount == 24)
		{
			if(m_ddsHeader.ddspf.RBitMask == 0xff0000 && m_ddsHeader.ddspf.GBitMask == 0xff00 && m_ddsHeader.ddspf.BBitMask == 0xff)
			{
				//m_bConvertSwapRB = true;
				m_bConvertFromRGB24 = true;
				return(GXFMT_X8R8G8B8);
			}
		}
		else if(m_ddsHeader.ddspf.RGBBitCount == 32)
		{
			if(m_ddsHeader.ddspf.RBitMask == 0xff && m_ddsHeader.ddspf.GBitMask == 0xff00 && m_ddsHeader.ddspf.BBitMask == 0xff0000)
			{
				m_bConvertSwapRB = true;
				return(GXFMT_X8R8G8B8); // GXFMT_X8B8G8R8
			}
			if(m_ddsHeader.ddspf.RBitMask == 0xff0000 && m_ddsHeader.ddspf.GBitMask == 0xff00 && m_ddsHeader.ddspf.BBitMask == 0xff)
			{
				//m_bConvertSwapRB = true;
				return(GXFMT_X8R8G8B8);
			}
		}
	}
	else if(m_ddsHeader.ddspf.flags == DDS_RGBA)
	{
		if(m_ddsHeader.ddspf.RGBBitCount == 32)
		{
			if(m_ddsHeader.ddspf.RBitMask == 0xff0000 && m_ddsHeader.ddspf.GBitMask == 0xff00 && m_ddsHeader.ddspf.BBitMask == 0xff && m_ddsHeader.ddspf.ABitMask == 0xff000000)
			{
				m_bConvertSwapRB = true;
				return(GXFMT_A8B8G8R8);
			}
			if(m_ddsHeader.ddspf.RBitMask == 0xff && m_ddsHeader.ddspf.GBitMask == 0xff00 && m_ddsHeader.ddspf.BBitMask == 0xff0000 && m_ddsHeader.ddspf.ABitMask == 0xff000000)
			{
				return(GXFMT_A8B8G8R8); // GXFMT_A8B8G8R8
			}
		}
	}
	else if(DDS_FOURCC)
	{
		switch(m_ddsHeader.ddspf.fourCC)
		{
		case MAKEFOURCC('D', 'X', 'T', '1'):
			return(GXFMT_DXT1);
		case MAKEFOURCC('D', 'X', 'T', '3'):
			return(GXFMT_DXT3);
		case MAKEFOURCC('D', 'X', 'T', '5'):
			return(GXFMT_DXT5);
		case MAKEFOURCC('B', 'C', '4', 'U'):
			return(GXFMT_ATI1N);
		case MAKEFOURCC('A', 'T', 'I', '2'):
			return(GXFMT_ATI2N);

		case 36: // D3DFMT_A16B16G16R16
			return(GXFMT_A16B16G16R16);
		case 111: // D3DFMT_R16F
			return(GXFMT_R16F);
		case 112: // D3DFMT_G16R16F
			return(GXFMT_G16R16F);
		case 113: // D3DFMT_A16B16G16R16F
			return(GXFMT_A16B16G16R16F);
		case 114: // D3DFMT_R32F
			return(GXFMT_R32F);
		case 115: // D3DFMT_G32R32F
			return(GXFMT_G32R32F);
		case 116: // D3DFMT_A32B32G32R32F
			return(GXFMT_A32B32G32R32F);

		case MAKEFOURCC('D', 'X', '1', '0'):
			switch(m_dxt10Header.dxgiFormat)
			{
			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM:
				return(GXFMT_A8B8G8R8);
			case DXGI_FORMAT_B8G8R8X8_UNORM:
				return(GXFMT_X8R8G8B8);
			case DXGI_FORMAT_B5G6R5_UNORM:
				return(GXFMT_R5G6B5);
			case DXGI_FORMAT_R16G16B16A16_UNORM:
				return(GXFMT_A16B16G16R16);
			case DXGI_FORMAT_BC1_UNORM:
				return(GXFMT_DXT1);
			case DXGI_FORMAT_BC2_UNORM:
				return(GXFMT_DXT3);
			case DXGI_FORMAT_BC3_UNORM:
				return(GXFMT_DXT5);
			case DXGI_FORMAT_BC4_UNORM:
				return(GXFMT_ATI1N);
			case DXGI_FORMAT_BC5_UNORM:
				return(GXFMT_ATI2N);
			case DXGI_FORMAT_D32_FLOAT:
				return(GXFMT_D32);
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
				return(GXFMT_D24S8);
			case DXGI_FORMAT_D16_UNORM:
				return(GXFMT_D16);
			case DXGI_FORMAT_R16_FLOAT:
				return(GXFMT_R16F);
			case DXGI_FORMAT_R16G16_FLOAT:
				return(GXFMT_G16R16F);
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
				return(GXFMT_A16B16G16R16F);
			case DXGI_FORMAT_R32_FLOAT:
				return(GXFMT_R32F);
			case DXGI_FORMAT_R32G32_FLOAT:
				return(GXFMT_G32R32F);
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
				return(GXFMT_A32B32G32R32F);
			}
		}
	}
	
	return(GXFMT_UNKNOWN);
}
