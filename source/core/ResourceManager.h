#ifndef __RESOURCEMANAGER_H
#define __RESOURCEMANAGER_H

#include <xcommon/resource/IXResourceManager.h>
#include <xcommon/IXModelLoader.h>
#include <common/string.h>
#include <common/aastring.h>
#include <xcommon/IXCore.h>

class CResourceModel;

class CResourceManager: public IXUnknownImplementation<IXResourceManager>
{
public:
	CResourceManager(IXCore *pCore);

	void initPlugins();

	bool XMETHODCALLTYPE getModel(const char *szName, IXResourceModel **ppOut, bool bForceReload = false) override;
	bool XMETHODCALLTYPE getModelStatic(const char *szName, IXResourceModelStatic **ppOut, bool bForceReload = false) override;
	bool XMETHODCALLTYPE getModelAnimated(const char *szName, IXResourceModelAnimated **ppOut, bool bForceReload = false) override;

	bool XMETHODCALLTYPE getModelInfo(const char *szName, XModelInfo *pInfo) override;

	UINT XMETHODCALLTYPE getModelSupportedFormats() override;
	const XFormatName* XMETHODCALLTYPE getModelSupportedFormat(UINT uIndex) override;

	IXResourceModelStatic* XMETHODCALLTYPE newResourceModelStatic() override;
	IXResourceModelAnimated* XMETHODCALLTYPE newResourceModelAnimated() override;
	void XMETHODCALLTYPE addModel(const char *szName, IXResourceModel *pModel) override;

	void onResourceModelRelease(CResourceModel *pResource);



	bool XMETHODCALLTYPE getTexture(const char *szName, IXResourceTexture **ppOut, bool bForceReload = false) override;
	bool XMETHODCALLTYPE getTexture2D(const char *szName, IXResourceTexture2D **ppOut, bool bForceReload = false) override;
	bool XMETHODCALLTYPE getTextureCube(const char *szName, IXResourceTextureCube **ppOut, bool bForceReload = false) override;

	UINT XMETHODCALLTYPE getTextureSupportedFormats() override;
	const XFormatName* XMETHODCALLTYPE getTextureSupportedFormat(UINT uIndex) override;

	bool XMETHODCALLTYPE getTextureInfo(const char *szName, XTextureInfo *pInfo) override;

	IXResourceTexture2D* XMETHODCALLTYPE newResourceTexture2D() override;
	IXResourceTextureCube* XMETHODCALLTYPE newResourceTextureCube() override;
	void XMETHODCALLTYPE addTexture(const char *szName, IXResourceTexture *pTexture) override;

	template<class T>
	void onResourceTextureRelease(T *pTexture)
	{
		const char *szName = pTexture->getFileName();
		if(szName)
		{
			m_mpTextures.erase(szName);
		}
	}

	UINT XMETHODCALLTYPE getSoundSupportedFormats() override;
	const XFormatName* XMETHODCALLTYPE getSoundSupportedFormat(UINT uIndex) override;

protected:
	IXCore *m_pCore;

	Map<AAString, Array<IXModelLoader*>> m_mapModelLoaders;
	Map<String, IXResourceModel*> m_mpModels;
	
	Array<XFormatName> m_aModelExts;

	struct TextureLoader
	{
		IXTextureLoader *pLoader = NULL;
		SpinLock slock;
	};

	Map<AAString, Array<TextureLoader>> m_mapTextureLoaders;
	Map<String, IXResourceTexture*> m_mpTextures;

	Array<XFormatName> m_aTextureExts;

	const char* getExtension(const char *szName);

	Array<XFormatName> m_aSoundExts;
};

#endif
