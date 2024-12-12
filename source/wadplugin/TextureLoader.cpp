#include "TextureLoader.h"

CTextureLoader::CTextureLoader(IFileSystem *pFileSystem):
	m_pFileSystem(pFileSystem)
{
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
		return("wad");
	}
	return(NULL);
}
const char* XMETHODCALLTYPE CTextureLoader::getExtText(UINT uIndex) const
{
	assert(uIndex < getExtCount());
	switch(uIndex)
	{
	case 0:
		return("Half-Life WAD");
	}
	return(NULL);
}
const char* XMETHODCALLTYPE CTextureLoader::getAuthor() const
{
	return("D-AIRY @ DogmaNet");
}
const char* XMETHODCALLTYPE CTextureLoader::getCopyright() const
{
	return("Copyright © Evgeny Danilovich, 2021");
}
const char* XMETHODCALLTYPE CTextureLoader::getDescription() const
{
	return("WAD texture loader");
}

void XMETHODCALLTYPE CTextureLoader::getInfo(XTextureInfo *pTextureInfo)
{
	pTextureInfo->type = getType();
	pTextureInfo->format = m_format;
	
	pTextureInfo->uWidth = m_texture43.uWidth;
	pTextureInfo->uHeight = m_texture43.uHeight;
	pTextureInfo->uDepth = 1;

	pTextureInfo->uFrameCount = m_uFrameCount;
	pTextureInfo->fFrameTime = 0.1f;

	pTextureInfo->uMipCount = 4;
}

bool XMETHODCALLTYPE CTextureLoader::open(const char *szFileName, const char *szArg)
{
	assert(!m_pCurrentFile && "File already opened!");
	if(m_pCurrentFile)
	{
		return(false);
	}
	
	if(!szArg)
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "Texture name is not specified '%s'\n", szFileName);
		return(false);
	}

	m_pCurrentFile = m_pFileSystem->openFile(szFileName);
	if(!m_pCurrentFile)
	{
		return(false);
	}

	if(m_pCurrentFile->readBin(&m_hdr, sizeof(m_hdr)) != sizeof(m_hdr))
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "Invalid file header '%s'\n", szFileName);
		mem_release(m_pCurrentFile);
		return(false);
	}

	m_format = szArg[0] == '{' ? GXFMT_A8B8G8R8 : GXFMT_X8R8G8B8;


	m_pCurrentFile->setPos(m_hdr.uLumpsOffset);

	WADLump lump;

	bool isFound = false;
	for(UINT i = 0; i < m_hdr.uTextures; ++i)
	{
		if(m_pCurrentFile->readBin(&lump, sizeof(lump)) != sizeof(lump))
		{
			LibReport(REPORT_MSG_LEVEL_WARNING, "Unexpected EOF '%s'\n", szFileName);
			mem_release(m_pCurrentFile);
			return(false);
		}

		lump.szName[sizeof(lump.szName) - 1] = 0;

		if(!strcasecmp(lump.szName, szArg))
		{
			isFound = true;
			break;
		}
	}

	if(!isFound)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Texture '%s' is not found in file '%s'\n", szArg, szFileName);
		mem_release(m_pCurrentFile);
		return(false);
	}

	if(lump.u8Type != 0x43 || lump.u8CompressionType != 0x00)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Texture '%s' in file '%s' has unexpected parameters\n", szArg, szFileName);
		mem_release(m_pCurrentFile);
		return(false);
	}

	m_uFrameCount = 1;
	if(szArg[0] == '+' && szArg[1] == '0')
	{
		// multiframe texture
		char *szTexName = strdupa(szArg);

		while(true)
		{
			szTexName[1] = m_uFrameCount + '0';
			if(isTextureExist(szTexName))
			{
				++m_uFrameCount;
			}
			else
			{
				break;
			}
		}
	}

	m_pCurrentFile->setPos(lump.uTextureOffset);
	if(m_pCurrentFile->readBin(&m_texture43, sizeof(m_texture43)) != sizeof(m_texture43))
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "Unexpected EOF '%s'\n", szFileName);
		mem_release(m_pCurrentFile);
		return(false);
	}
	m_texture43.szName[sizeof(m_texture43.szName) - 1] = 0;

	m_pCurrentFile->setPos(lump.uTextureOffset);

	return(true);
}
GXTEXTURE_TYPE XMETHODCALLTYPE CTextureLoader::getType() const
{
	return(GXTEXTURE_TYPE_2D);
}

bool XMETHODCALLTYPE CTextureLoader::loadAs2D(IXResourceTexture2D *pResource)
{
	if(getType() != GXTEXTURE_TYPE_2D)
	{
		return(false);
	}
	
	XTextureInfo info;
	getInfo(&info);

	pResource->init(info.uWidth, info.uHeight, info.format, info.uMipCount, info.uFrameCount);

	pResource->setFrameTime(info.fFrameTime);

	char *szName = strdupa(m_texture43.szName);

	for(UINT i = 0; i < info.uFrameCount; ++i)
	{
		if(i != 0)
		{
			szName[1] = i + '0';
			WADLump lump = {};
			isTextureExist(szName, &lump);

			m_pCurrentFile->setPos(lump.uTextureOffset);
			if(m_pCurrentFile->readBin(&m_texture43, sizeof(m_texture43)) != sizeof(m_texture43))
			{
				LibReport(REPORT_MSG_LEVEL_WARNING, "Unexpected EOF\n");
				return(false);
			}
			m_texture43.szName[sizeof(m_texture43.szName) - 1] = 0;

			m_pCurrentFile->setPos(lump.uTextureOffset);
		}

		size_t zTextureOffset = m_pCurrentFile->getPos();
		m_pCurrentFile->setPos(zTextureOffset + m_texture43.uMip3Offset + (info.uWidth / 8) * (info.uHeight / 8) + 2);

		byte aPalette[256 * 3];
		m_pCurrentFile->readBin(aPalette, sizeof(aPalette));

		m_pCurrentFile->setPos(zTextureOffset + m_texture43.uImageOffset);
		loadIndexedRGB(0, i, aPalette, pResource);
		m_pCurrentFile->setPos(zTextureOffset + m_texture43.uMip1Offset);
		loadIndexedRGB(1, i, aPalette, pResource);
		m_pCurrentFile->setPos(zTextureOffset + m_texture43.uMip2Offset);
		loadIndexedRGB(2, i, aPalette, pResource);
		m_pCurrentFile->setPos(zTextureOffset + m_texture43.uMip3Offset);
		loadIndexedRGB(3, i, aPalette, pResource);
	}

	return(true);
}
bool XMETHODCALLTYPE CTextureLoader::loadAsCube(IXResourceTextureCube *pResource)
{
	return(false);
}

void XMETHODCALLTYPE CTextureLoader::close()
{
	mem_release(m_pCurrentFile);
}

bool CTextureLoader::loadIndexedRGB(UINT uMip, UINT uFrame, byte *aPalette, IXResourceTexture2D *pResource)
{
	XImageMip *pMip = pResource->getMip(uMip, uFrame);
	if(pMip)
	{
		assert(pMip->sizeData == pResource->getTextureBytes(m_format, m_texture43.uWidth >> uMip, m_texture43.uHeight >> uMip));
		pMip->isWritten = true;

		byte *pData = pMip->pData;
		byte ci;
		for(UINT j = 0; j < m_texture43.uHeight >> uMip; ++j)
		{
			for(UINT i = 0; i < m_texture43.uWidth >> uMip; ++i)
			{
				m_pCurrentFile->readBin(&ci, sizeof(ci));
				// r = aPalette[ci];
				// g = aPalette[ci + 1];
				// b = aPalette[ci + 2];

				if(m_format == GXFMT_X8R8G8B8)
				{
					*pData = aPalette[(UINT)ci * 3 + 2]; ++pData;
					*pData = aPalette[(UINT)ci * 3 + 1]; ++pData;
					*pData = aPalette[(UINT)ci * 3 + 0]; ++pData;
					*pData = 0xFF; ++pData;
				}
				else
				{
					*pData = aPalette[(UINT)ci * 3 + 0]; ++pData;
					*pData = aPalette[(UINT)ci * 3 + 1]; ++pData;
					*pData = aPalette[(UINT)ci * 3 + 2]; ++pData;
					*pData = ci == 255 ? 0x00 : 0xFF; ++pData;
				}
			}
		}
	}

	return(true);
}

bool CTextureLoader::isTextureExist(const char *szName, WADLump *pOutLump)
{
	m_pCurrentFile->setPos(m_hdr.uLumpsOffset);

	WADLump lump;

	bool isFound = false;
	for(UINT i = 0; i < m_hdr.uTextures; ++i)
	{
		if(m_pCurrentFile->readBin(&lump, sizeof(lump)) != sizeof(lump))
		{
			//LibReport(REPORT_MSG_LEVEL_WARNING, "Unexpected EOF '%s'\n", szFileName);
			return(false);
		}

		lump.szName[sizeof(lump.szName) - 1] = 0;

		if(!strcasecmp(lump.szName, szName))
		{
			if(pOutLump)
			{
				*pOutLump = lump;
			}
			return(true);
		}
	}

	return(false);
}
