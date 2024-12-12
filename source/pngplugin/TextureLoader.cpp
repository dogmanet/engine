#include "TextureLoader.h"

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
		return("png");
	}
	return(NULL);
}
const char* XMETHODCALLTYPE CTextureLoader::getExtText(UINT uIndex) const
{
	assert(uIndex < getExtCount());
	switch(uIndex)
	{
	case 0:
		return("Portable Network Graphics");
	}
	return(NULL);
}
const char* XMETHODCALLTYPE CTextureLoader::getAuthor() const
{
	return("D-AIRY @ DogmaNet");
}
const char* XMETHODCALLTYPE CTextureLoader::getCopyright() const
{
	return("Copyright © Evgeny Danilovich, 2022");
}
const char* XMETHODCALLTYPE CTextureLoader::getDescription() const
{
	return("PNG texture loader");
}

void XMETHODCALLTYPE CTextureLoader::getInfo(XTextureInfo *pTextureInfo)
{
	pTextureInfo->type = getType();
	pTextureInfo->format = GXFMT_A8B8G8R8;
	
	pTextureInfo->uWidth = png_get_image_width(m_pPngStruct, m_pPngInfo);
	pTextureInfo->uHeight = png_get_image_height(m_pPngStruct, m_pPngInfo);
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

	byte pMagick[8];
	if(pFile->readBin(pMagick, sizeof(pMagick)) != sizeof(pMagick))
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "Unexpected End-of-file '%s'\n", szFileName);
		mem_release(m_pCurrentFile);
		return(false);
	}

	/*if(png_sig_cmp(pMagick, 0, ARRAYSIZE(pMagick)))
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "Not a PNG file '%s'\n", szFileName);
		mem_release(m_pCurrentFile);
		return(false);
	}*/

	pFile->setPos(0);

	m_pPngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, this, [](png_struct *pPngStruct, const char *szMsg){
		LibReport(REPORT_MSG_LEVEL_ERROR, "%s\n", szMsg);
		throw std::exception();
	}, [](png_struct *pPngStruct, const char *szMsg){
		LibReport(REPORT_MSG_LEVEL_WARNING, "%s\n", szMsg);
	});
	if(!m_pPngStruct)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "png_create_read_struct() failed '%s'\n", szFileName);
		mem_release(m_pCurrentFile);
		return(false);
	}

	m_pPngInfo = png_create_info_struct(m_pPngStruct);
	if(!m_pPngInfo)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "png_create_info_struct() failed '%s'\n", szFileName);
		mem_release(m_pCurrentFile);
		png_destroy_read_struct(&m_pPngStruct, NULL, NULL);
		return(false);
	}

	m_pPngEndInfo = png_create_info_struct(m_pPngStruct);
	if(!m_pPngEndInfo)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "png_create_info_struct() failed2 '%s'\n", szFileName);
		mem_release(m_pCurrentFile);
		png_destroy_read_struct(&m_pPngStruct, &m_pPngInfo, NULL);
		return(false);
	}

	png_set_read_fn(m_pPngStruct, m_pCurrentFile, [](png_struct *pPngStruct, png_byte *pData, size_t size){
		IFile *pCurrentFile = (IFile*)png_get_io_ptr(pPngStruct);
		size_t sizeRead = pCurrentFile->readBin(pData, size);
		if(sizeRead != size)
		{
			png_error(pPngStruct, "Unexpected end-of-file");
		}
	});

	try
	{
		png_read_info(m_pPngStruct, m_pPngInfo);
	}
	catch(std::exception&)
	{
		mem_release(m_pCurrentFile);
		png_destroy_read_struct(&m_pPngStruct, &m_pPngInfo, &m_pPngEndInfo);
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

	pResource->init(info.uWidth, info.uHeight, info.format); // , info.uMipCount
	XImageMip *pMip = pResource->getMip(0, 0);

	try
	{
		png_set_packing(m_pPngStruct);
		png_set_scale_16(m_pPngStruct);

		png_byte color_type = png_get_color_type(m_pPngStruct, m_pPngInfo);
		png_byte bit_depth = png_get_bit_depth(m_pPngStruct, m_pPngInfo);

		/* Expand paletted colors into true RGB triplets. */
		if(color_type == PNG_COLOR_TYPE_PALETTE)
		{
			png_set_palette_to_rgb(m_pPngStruct);
		}

		/* Expand grayscale images to the full 8 bits from 1, 2 or 4 bits/pixel. */
		if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		{
			png_set_expand_gray_1_2_4_to_8(m_pPngStruct);
		}

		if(color_type == PNG_COLOR_TYPE_GRAY)
		{
			png_set_gray_to_rgb(m_pPngStruct);
		}

		/* Expand paletted or RGB images with transparency to full alpha channels
		* so the data will be available as RGBA quartets.
		*/
		if(png_get_valid(m_pPngStruct, m_pPngInfo, PNG_INFO_tRNS) != 0)
		{
			png_set_tRNS_to_alpha(m_pPngStruct);
		}

		/* Tell libpng to handle the gamma conversion for you.  The final call
		* is a good guess for PC generated images, but it should be configurable
		* by the user at run time.  Gamma correction support in your application
		* is strongly recommended.
		*/

		int intent;

		//if(png_get_sRGB(m_pPngStruct, m_pPngInfo, &intent) != 0)
		//{
		//	png_set_gamma(m_pPngStruct, 1.0, PNG_DEFAULT_sRGB);
		//}
		//else
		//{
		//	double image_gamma;
		//	if(png_get_gAMA(m_pPngStruct, m_pPngInfo, &image_gamma) != 0)
		//	{
		//		png_set_gamma(m_pPngStruct, 1.0, image_gamma);
		//	}
		//	else
		//	{
		//		png_set_gamma(m_pPngStruct, 1.0, 0.45455);
		//	}
		//}

		/* Add filler (or alpha) byte (before/after each RGB triplet). */
		png_set_filler(m_pPngStruct, 0xffff, PNG_FILLER_AFTER);

		/* Optional call to gamma correct and add the background to the palette
		* and update info structure.  REQUIRED if you are expecting libpng to
		* update the palette for you (i.e. you selected such a transform above).
		*/
		png_read_update_info(m_pPngStruct, m_pPngInfo);

		byte* pData = pMip->pData;
		png_bytep *row_pointers = (png_bytep*)alloca(sizeof(png_bytep) * info.uHeight);
		for(UINT row = 0; row < info.uHeight; row++)
		{
			row_pointers[row] = pData; /* Clear the pointer array */
			pData += png_get_rowbytes(m_pPngStruct, m_pPngInfo);
		}

		png_read_image(m_pPngStruct, row_pointers);
		png_read_end(m_pPngStruct, m_pPngEndInfo);
		pMip->isWritten = true;
	}
	catch(std::exception&)
	{
		return(false);
	}

#if 0
	pResource->setFrameTime(m_fFrameTime);

	XTextureInfo info;
	getInfo(&info);

	bool isBC = isBlockCompressed(info.format);

	pResource->init(info.uWidth, info.uHeight, info.format, info.uMipCount, (UINT)(m_iXFrames * m_iYFrames - m_iSkipFrames));

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
#endif
	return(true);
}
bool XMETHODCALLTYPE CTextureLoader::loadAsCube(IXResourceTextureCube *pResource)
{
	return(false);
}

void XMETHODCALLTYPE CTextureLoader::close()
{
	mem_release(m_pCurrentFile);
	png_destroy_read_struct(&m_pPngStruct, &m_pPngInfo, &m_pPngEndInfo);
}
