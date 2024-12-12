#include "TextureLoader.h"
#include "TGAFile.h"

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
		return("tga");
	}
	return(NULL);
}
const char* XMETHODCALLTYPE CTextureLoader::getExtText(UINT uIndex) const
{
	assert(uIndex < getExtCount());
	switch(uIndex)
	{
	case 0:
		return("Truevision TGA");
	}
	return(NULL);
}
const char* XMETHODCALLTYPE CTextureLoader::getAuthor() const
{
	return("D-AIRY @ DogmaNet");
}
const char* XMETHODCALLTYPE CTextureLoader::getCopyright() const
{
	return("Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2020");
}
const char* XMETHODCALLTYPE CTextureLoader::getDescription() const
{
	return("TGA texture loader");
}

void XMETHODCALLTYPE CTextureLoader::getInfo(XTextureInfo *pTextureInfo)
{
	pTextureInfo->type = getType();
	pTextureInfo->format = m_format;
	
	pTextureInfo->uWidth = m_tgaHeader.width;
	pTextureInfo->uHeight = m_tgaHeader.height;
	pTextureInfo->uDepth = 1;

	pTextureInfo->uFrameCount = 1;
	pTextureInfo->fFrameTime = 0.0f;

	pTextureInfo->uMipCount = 1;
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

	if(pFile->readBin(&m_tgaHeader, sizeof(m_tgaHeader)) != sizeof(m_tgaHeader))
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "Invalid file header '%s'\n", szFileName);
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

	if(m_tgaHeader.datatypecode == 0)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Texture has no image data '%s'\n", szFileName);
		mem_release(m_pCurrentFile);
		return(false);
	}

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

	pResource->init(info.uWidth, info.uHeight, info.format, 1, 1);

	//! @todo add support for other image types
	switch(m_tgaHeader.datatypecode)
	{
	case 2:  // - Uncompressed, RGB images.
		return(loadUncompressedRGB(pResource));

	case 1:  // - Uncompressed, color - mapped images.
	case 3:  // - Uncompressed, black and white images.
	case 9:  // - Runlength encoded color - mapped images.
	case 10: // - Runlength encoded RGB images.
	case 11: // - Compressed, black and white images.
	case 32: // - Compressed color - mapped data, using Huffman, Delta, and runlength encoding.
	case 33: // - Compressed color - mapped data, using Huffman, Delta, and runlength encoding.  4 - pass quadtree - type process.
	default:
		LibReport(REPORT_MSG_LEVEL_ERROR, "Unsupported file data\n");
		return(false);
	}

}
bool XMETHODCALLTYPE CTextureLoader::loadAsCube(IXResourceTextureCube *pResource)
{
	return(false);
}

void XMETHODCALLTYPE CTextureLoader::close()
{
	mem_release(m_pCurrentFile);
}

GXFORMAT CTextureLoader::getFormat()
{
	if(m_tgaHeader.bitsperpixel == 24)
	{
		return(GXFMT_X8R8G8B8);
	}
	else if(m_tgaHeader.bitsperpixel == 32)
	{
		return(GXFMT_A8B8G8R8);
	}
	
	return(GXFMT_UNKNOWN);
}

bool CTextureLoader::loadUncompressedRGB(IXResourceTexture2D *pResource)
{
	XImageMip *pMip = pResource->getMip(0, 0);
	if(pMip)
	{
		assert(pMip->sizeData == pResource->getTextureBytes(m_format, m_tgaHeader.width, m_tgaHeader.height));
		pMip->isWritten = true;
		m_pCurrentFile->setPos(m_pCurrentFile->getPos() + m_tgaHeader.idlength + m_tgaHeader.colormaptype * m_tgaHeader.colormaplength);

		if(m_format == GXFMT_A8B8G8R8)
		{
			if(m_pCurrentFile->readBin(pMip->pData, pMip->sizeData) != pMip->sizeData)
			{
				LibReport(REPORT_MSG_LEVEL_ERROR, "Unexpected end of file\n");
				return(false);
			}

			for(UINT i = 0, l = m_tgaHeader.width * m_tgaHeader.height; i < l; ++i)
			{
				std::swap(pMip->pData[i * 4], pMip->pData[i * 4 + 2]);
			}
		}
		else if(m_format == GXFMT_X8R8G8B8)
		{
			for(UINT i = 0, l = m_tgaHeader.width * m_tgaHeader.height; i < l; ++i)
			{
				if(m_pCurrentFile->readBin(pMip->pData + i * 4, 3) != 3)
				{
					LibReport(REPORT_MSG_LEVEL_ERROR, "Unexpected end of file\n");
					return(false);
				}
				pMip->pData[i * 4 + 3] = 0xFF;
			}
		}
	}

	return(true);
}
