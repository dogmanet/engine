#ifndef __RESOURCETEXTURE_H
#define __RESOURCETEXTURE_H

#include <xcommon/resource/IXResourceTexture.h>

class CResourceManager;
template<class T>
class CResourceTextureImpl: public IXUnknownImplementation<T>
{
public:
	typedef CResourceTextureImpl<T> BaseClass;

	CResourceTextureImpl(GXTEXTURE_TYPE type, CResourceManager *pMgr):
		m_type(type),
		m_pManager(pMgr)
	{
	}
	~CResourceTextureImpl()
	{
		m_pManager->onResourceTextureRelease(this);
		mem_delete_a(pDataBlob);
	}

	GXTEXTURE_TYPE XMETHODCALLTYPE getType() const override
	{
		return(m_type);
	}
	GXFORMAT XMETHODCALLTYPE getFormat() const override
	{
		if(m_isSRGB)
		{
			switch(m_format)
			{
			case GXFMT_A8B8G8R8:
				return(GXFMT_A8B8G8R8_SRGB);
			case GXFMT_X8R8G8B8:
				return(GXFMT_X8R8G8B8_SRGB);
			case GXFMT_DXT1:
				return(GXFMT_DXT1_SRGB);
			case GXFMT_DXT3:
				return(GXFMT_DXT3_SRGB);
			case GXFMT_DXT5:
				return(GXFMT_DXT5_SRGB);
			}
		}
		return(m_format);
	}
	
	UINT XMETHODCALLTYPE getFrameCount() const override
	{
		return(m_uFrameCount);
	}
	float XMETHODCALLTYPE getFrameTime() const override
	{
		return(m_fFrameTime);
	}
	void XMETHODCALLTYPE setFrameTime(float fTime) override
	{
		m_fFrameTime = fTime;
	}

	UINT XMETHODCALLTYPE getMipmapCount() const override
	{
		return(m_uMipmapCount);
	}

	const IXResourceTexture2D* XMETHODCALLTYPE as2D() const override
	{
		return(NULL);
	}
	const IXResourceTextureCube* XMETHODCALLTYPE asCube() const override
	{
		return(NULL);
	}
	IXResourceTexture2D* XMETHODCALLTYPE as2D() override
	{
		return(NULL);
	}
	IXResourceTextureCube* XMETHODCALLTYPE asCube() override
	{
		return(NULL);
	}
	
	void XMETHODCALLTYPE makeReadOnly() override
	{
		//! @todo Implement me!
	}

	UINT getBitsPerPixel(GXFORMAT format) const
	{
		switch(format)
		{
		case GXFMT_A32B32G32R32F:
			return(128);

		case GXFMT_G32R32F:
		case GXFMT_A16B16G16R16F:
		case GXFMT_A16B16G16R16:
			return(64);

		case GXFMT_R32F:
		case GXFMT_G16R16F:
		case GXFMT_D24X8:
		case GXFMT_D24S8:
		case GXFMT_D32:
		case GXFMT_X8R8G8B8:
		case GXFMT_A8B8G8R8:
		case GXFMT_X8R8G8B8_SRGB:
		case GXFMT_A8B8G8R8_SRGB:
			return(32);

		case GXFMT_D16:
		case GXFMT_R16F:
		case GXFMT_R5G6B5:
			return(16);

		case GXFMT_DXT1:
		case GXFMT_DXT1_SRGB:
		case GXFMT_ATI1N:
			return(4);

		case GXFMT_DXT3:
		case GXFMT_DXT5:
		case GXFMT_DXT3_SRGB:
		case GXFMT_DXT5_SRGB:
		case GXFMT_ATI2N:
			return(8);
		}
		assert(!"Unknown format!");
		return(0);
	}

	UINT XMETHODCALLTYPE getTextureBytes(GXFORMAT format, UINT uWidth, UINT uHeight) const override
	{
		bool bc = true;
		int bcnumBytesPerBlock = 16;

		switch(format)
		{
		case GXFMT_DXT1:
		case GXFMT_DXT1_SRGB:
		case GXFMT_ATI1N:
			bcnumBytesPerBlock = 8;
		case GXFMT_DXT3:
		case GXFMT_DXT5:
		case GXFMT_DXT3_SRGB:
		case GXFMT_DXT5_SRGB:
		case GXFMT_ATI2N:
			bc = true;
			break;

		default:
			bc = false;
		}

		if(bc)
		{
			int numBlocksWide = max(1, (uWidth + 3) / 4);
			int numBlocksHigh = max(1, (uHeight + 3) / 4);

			return(numBlocksWide * numBlocksHigh * bcnumBytesPerBlock);
		}
		else
		{
			UINT bpp = getBitsPerPixel(format);
			return((uWidth * bpp + 7) / 8 * uHeight); // round up to nearest byte
		}
	}

	bool isBlockCompressed(GXFORMAT format)
	{
		switch(format)
		{
		case GXFMT_DXT1:
		case GXFMT_ATI1N:
		case GXFMT_DXT3:
		case GXFMT_DXT5:
		case GXFMT_DXT1_SRGB:
		case GXFMT_DXT3_SRGB:
		case GXFMT_DXT5_SRGB:
		case GXFMT_ATI2N:
			return(true);
		}

		return(false);
	}

	void setFileName(const char *szFilename)
	{
		m_szFileName = szFilename;
	}
	const char *getFileName() const
	{
		return(m_szFileName);
	}

	bool XMETHODCALLTYPE isSRGB() const override
	{
		return(m_isSRGB);
	}

	void XMETHODCALLTYPE setIsSRGB(bool yesNo) override
	{
		m_isSRGB = yesNo;
	}

protected:
	void cloneBaseData(CResourceTextureImpl *pTo) const
	{
		// pTo->setFileName(m_szFileName);
		pTo->setFrameTime(m_fFrameTime);
		pTo->setIsSRGB(m_isSRGB);
	}

protected:
	GXTEXTURE_TYPE m_type = GXTEXTURE_TYPE_UNKNOWN;
	GXFORMAT m_format = GXFMT_UNKNOWN;
	bool m_isSRGB = false;
	UINT m_uFrameCount = 0;
	UINT m_uMipmapCount = 0;
	float m_fFrameTime = 0.0f;

	byte *pDataBlob = NULL;

	const char *m_szFileName = NULL;
	CResourceManager *m_pManager;
};

// Implemented in core
class CResourceTexture2D: public CResourceTextureImpl<IXResourceTexture2D>
{
public:
	CResourceTexture2D(CResourceManager *pMgr);

	UINT XMETHODCALLTYPE getWidth() const override;
	UINT XMETHODCALLTYPE getHeight() const override;

	void XMETHODCALLTYPE init(UINT uWidth, UINT uHeight, GXFORMAT format, UINT uMipmapCount = IXRESOURCE_TEXTURE_AUTO_MIPS, UINT uFrameCount = 1) override;

	XImageMip* XMETHODCALLTYPE getMip(UINT uMipmap, UINT uFrame = 0) override;
	const XImageMip* XMETHODCALLTYPE getMip(UINT uMipmap, UINT uFrame = 0) const override;

	const IXResourceTexture2D* XMETHODCALLTYPE as2D() const override;
	IXResourceTexture2D* XMETHODCALLTYPE as2D() override;

	void XMETHODCALLTYPE clone(IXResourceTexture **pOut) const override;

	void genMipmaps();
protected: 
	UINT m_uWidth = 0;
	UINT m_uHeight = 0;

	XImageMip **m_ppData = NULL; // m_ppData[uFrame][uMip]
};

// Implemented in core
class CResourceTextureCube: public CResourceTextureImpl<IXResourceTextureCube>
{
public:
	CResourceTextureCube(CResourceManager *pMgr);

	UINT XMETHODCALLTYPE getSize() const override;

	void XMETHODCALLTYPE init(UINT uSize, GXFORMAT format, UINT uMipmapCount = IXRESOURCE_TEXTURE_AUTO_MIPS, UINT uFrameCount = 1) override;

	XImageMip* XMETHODCALLTYPE getMip(GXCUBEMAP_FACES face, UINT uMipmap, UINT uFrame = 0) override;
	const XImageMip* XMETHODCALLTYPE getMip(GXCUBEMAP_FACES face, UINT uMipmap, UINT uFrame = 0) const override;

	const IXResourceTextureCube* XMETHODCALLTYPE asCube() const override;
	IXResourceTextureCube* XMETHODCALLTYPE asCube() override;

	void XMETHODCALLTYPE clone(IXResourceTexture **pOut) const override;
protected:
	UINT m_uSize = 0;

	XImageMip ***m_pppData = NULL; // m_pppData[uFrame][uMip][side]
};


#endif
