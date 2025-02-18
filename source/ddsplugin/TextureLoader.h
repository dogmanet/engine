#ifndef __TEXTURELOADER_H
#define __TEXTURELOADER_H

#include <xcommon/IXTextureLoader.h>
#include <xcommon/IFileSystem.h>
#include "DDSFile.h"

class CTextureLoader: public IXUnknownImplementation<IXTextureLoader>
{
public:
	CTextureLoader(IFileSystem *pFileSystem);

	UINT XMETHODCALLTYPE getVersion() override;

	UINT XMETHODCALLTYPE getExtCount() const override;
	const char* XMETHODCALLTYPE getExt(UINT uIndex) const override;
	const char* XMETHODCALLTYPE getExtText(UINT uIndex) const override;
	const char* XMETHODCALLTYPE getAuthor() const override;
	const char* XMETHODCALLTYPE getCopyright() const override;
	const char* XMETHODCALLTYPE getDescription() const override;

	bool XMETHODCALLTYPE open(const char *szFileName, const char *szArg) override;
	GXTEXTURE_TYPE XMETHODCALLTYPE getType() const override;
	bool XMETHODCALLTYPE loadAs2D(IXResourceTexture2D *pResource) override;
	bool XMETHODCALLTYPE loadAsCube(IXResourceTextureCube *pResource) override;
	void XMETHODCALLTYPE getInfo(XTextureInfo *pTextureInfo) override;
	void XMETHODCALLTYPE close() override;

	GXFORMAT getFormat();

	bool isBlockCompressed(GXFORMAT format);
	
protected:
	IFileSystem *m_pFileSystem;

	IFile *m_pCurrentFile = NULL;
	DDS_HEADER m_ddsHeader;
	bool m_hasDXT10Header = false;
	DDS_HEADER_DXT10 m_dxt10Header;
	GXFORMAT m_format;

	int m_iXFrames = 1;
	int m_iYFrames = 1;
	float m_fFrameTime = 0.0;
	int m_iSkipFrames = 0;

	bool m_bConvertFromRGB24 = false;
	bool m_bConvertSwapRB = false;
};

#endif
