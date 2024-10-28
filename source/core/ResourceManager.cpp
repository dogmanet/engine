#include "ResourceManager.h"
#include <xcommon/IPluginManager.h>
#include "ResourceModelStatic.h"
#include "ResourceModelAnimated.h"
#include "ResourceTexture.h"

#if 1
#include "File.h"


#include <xcommon/resource/IXModelProvider.h>
#include <xcommon/resource/IXResourceManager.h>
#include <xcommon/resource/IXResourceModel.h>
#include <xcommon/IXAudioCodec.h>
#endif

CResourceManager::CResourceManager(IXCore *pCore):
	m_pCore(pCore)
{

}

void CResourceManager::initPlugins()
{
	IPluginManager *pPluginManager = m_pCore->getPluginManager();

	{
		IXModelLoader *pLoader;
		UINT ic = 0;
		while((pLoader = (IXModelLoader*)pPluginManager->getInterface(IXMODELLOADER_GUID, ic++)))
		{
			if(pLoader->getVersion() == IXMODELLOADER_VERSION)
			{
				LibReport(REPORT_MSG_LEVEL_NOTICE, "Registered model loader:\n %s\n By %s\n %s\n", pLoader->getDescription(), pLoader->getAuthor(), pLoader->getCopyright());

				for(UINT i = 0, l = pLoader->getExtCount(); i < l; ++i)
				{
					AAString sExt;
					sExt.setName(pLoader->getExt(i));
					strlwr(const_cast<char*>(sExt.getName()));
					m_mapModelLoaders[sExt].push_back(pLoader);
					m_aModelExts.push_back({pLoader->getExtText(i), pLoader->getExt(i)});
					LibReport(REPORT_MSG_LEVEL_NOTICE, " Ext: " COLOR_LCYAN "%s" COLOR_RESET ": " COLOR_WHITE "%s" COLOR_RESET "\n", pLoader->getExt(i), pLoader->getExtText(i));
				}
				LibReport(REPORT_MSG_LEVEL_NOTICE, " \n");
			}
		}
	}

	{
		IXTextureLoader *pLoader;
		UINT ic = 0;
		while((pLoader = (IXTextureLoader*)pPluginManager->getInterface(IXTEXTURELOADER_GUID, ic++)))
		{
			if(pLoader->getVersion() == IXTEXTURELOADER_VERSION)
			{
				LibReport(REPORT_MSG_LEVEL_NOTICE, "Registered texture loader:\n %s\n By %s\n %s\n", pLoader->getDescription(), pLoader->getAuthor(), pLoader->getCopyright());

				for(UINT i = 0, l = pLoader->getExtCount(); i < l; ++i)
				{
					AAString sExt;
					sExt.setName(pLoader->getExt(i));
					strlwr(const_cast<char*>(sExt.getName()));
					auto &arr = m_mapTextureLoaders[sExt];
					TextureLoader &loader = arr[arr.size()];
					loader.pLoader = pLoader;
					m_aTextureExts.push_back({pLoader->getExtText(i), pLoader->getExt(i)});
					LibReport(REPORT_MSG_LEVEL_NOTICE, " Ext: " COLOR_LCYAN "%s" COLOR_RESET ": " COLOR_WHITE "%s" COLOR_RESET "\n", pLoader->getExt(i), pLoader->getExtText(i));
				}
				LibReport(REPORT_MSG_LEVEL_NOTICE, " \n");
			}
		}
	}

	{
		IXAudioCodec *pAudioCodec;
		UINT ic = 0;
		while ((pAudioCodec = (IXAudioCodec*)pPluginManager->getInterface(IXAUDIOCODEC_GUID, ic++)))
		{
			if (pAudioCodec->getVersion() == IXAUDIOCODEC_VERSION)
			{
				//LibReport(REPORT_MSG_LEVEL_NOTICE, "Registered sound loader:\n %s\n By %s\n %s\n", pAudioCodec->getDescription(), pAudioCodec->getAuthor(), pAudioCodec->getCopyright());

				for (UINT i = 0, l = pAudioCodec->getExtCount(); i < l; ++i)
				{
					AAString sExt;
					sExt.setName(pAudioCodec->getExt(i));
					strlwr(const_cast<char*>(sExt.getName()));
					m_aSoundExts.push_back({ pAudioCodec->getExt(i), pAudioCodec->getExt(i) });
					LibReport(REPORT_MSG_LEVEL_NOTICE, " Ext: " COLOR_LCYAN "%s" COLOR_RESET ": " COLOR_WHITE "%s" COLOR_RESET "\n", pAudioCodec->getExt(i), pAudioCodec->getExt(i));
				}
				LibReport(REPORT_MSG_LEVEL_NOTICE, " \n");
			}
		}
	}
}

bool XMETHODCALLTYPE CResourceManager::getModel(const char *szName, IXResourceModel **ppOut, bool bForceReload)
{
	const Map<String, IXResourceModel*>::Node *pNode1;
	if(!bForceReload && m_mpModels.KeyExists(szName, &pNode1))
	{
		(*pNode1->Val)->AddRef();
		*ppOut = *pNode1->Val;
		return(true);
	}

	const char *szExt = getExtension(szName);

	if(!szExt || !*szExt)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid model name '%s'. Missing extension\n", szName);
		return(false);
	}

	char *szLowcaseExt = strdupa(szExt);
	strlwr(szLowcaseExt);

	CResourceModel *pResource = NULL;
	const Map<AAString, Array<IXModelLoader*>>::Node *pNode;
	if(m_mapModelLoaders.KeyExists(AAString(szLowcaseExt), &pNode))
	{
		auto &aLoaders = *pNode->Val;
		if(!m_pCore->getFileSystem()->fileExists(szName))
		{
			LibReport(REPORT_MSG_LEVEL_ERROR, "File not found '%s'\n", szName);
			return(false);
		}
		for(UINT i = 0, l = aLoaders.size(); i < l; ++i)
		{
			IXModelLoader *pLoader = aLoaders[i];
			if(pLoader->open(szName))
			{
				switch(pLoader->getType())
				{
				case XMT_STATIC:
					{
						CResourceModelStatic *pModel = new CResourceModelStatic(this);
						if(pLoader->loadAsStatic(pModel))
						{
							pResource = pModel;
						}
						else
						{
							mem_delete(pModel);
						}
					}
					break;
				case XMT_ANIMATED:
					{
						CResourceModelAnimated *pModel = new CResourceModelAnimated(this);
						if(pLoader->loadAsAnimated(pModel))
						{
							pResource = pModel;
						}
						else
						{
							mem_delete(pModel);
						}
					}
					break;
				}
				pLoader->close();

				if(pResource)
				{
					break;
				}
			}
		}

		if(pResource)
		{
			*ppOut = pResource;
			if(bForceReload && m_mpModels.KeyExists(szName))
			{
				mem_release(m_mpModels[szName]);
			}
			m_mpModels[szName] = pResource;
			pResource->setFileName(m_mpModels.TmpNode->Key.c_str());

			for(UINT i = 0, l = pResource->getGibsCount(); i < l; ++i)
			{
				IXResourceModel *pGibResource;
				if(getModel(pResource->getGibName(i), &pGibResource, bForceReload))
				{
					pResource->setGib(i, pGibResource);
					mem_release(pGibResource);
				}
				else
				{
					LibReport(REPORT_MSG_LEVEL_ERROR, "Unable to load gib #%u for model '%s'\n", i, szName);
					mem_release(pResource);
					return(false);
				}
			}

			if(pResource->getType() == XMT_ANIMATED)
			{
				CResourceModelAnimated *pModel = (CResourceModelAnimated*)pResource;
				for(UINT i = 0, l = pModel->getImportsCount(); i < l; ++i)
				{
					IXResourceModel *pImportResource;
					if(getModel(pModel->getImportName(i), &pImportResource, bForceReload))
					{
						pModel->setImport(i, pImportResource);
						mem_release(pImportResource);
					}
					else
					{
						LibReport(REPORT_MSG_LEVEL_ERROR, "Unable to load import #%u for model '%s'\n", i, szName);
						mem_release(pResource);
						return(false);
					}
				}

				for(UINT i = 0, l = pModel->getPartsCount(); i < l; ++i)
				{
					IXResourceModel *pPartResource;
					if(getModel(pModel->getPartFileName(i), &pPartResource, bForceReload))
					{
						pModel->setPart(i, pPartResource);
						mem_release(pPartResource);
					}
					else
					{
						LibReport(REPORT_MSG_LEVEL_ERROR, "Unable to load part #%u (%s: %s) for model '%s'\n", i, pModel->getPartName(i), pModel->getPartFileName(i), szName);
						mem_release(pResource);
						return(false);
					}
				}
			}

			return(true);
		}

		LibReport(REPORT_MSG_LEVEL_ERROR, "No one plugin could load model '%s'\n", szName);
		return(false);
	}

	LibReport(REPORT_MSG_LEVEL_ERROR, "No plugins for model extension found '%s'\n", szName);
	return(false);
}
bool XMETHODCALLTYPE CResourceManager::getModelStatic(const char *szName, IXResourceModelStatic **ppOut, bool bForceReload)
{
	IXResourceModel *pModel = NULL;
	if(getModel(szName, &pModel, bForceReload))
	{
		if(pModel->getType() == XMT_STATIC)
		{
			*ppOut = pModel->asStatic();
			return(true);
		}
		mem_release(pModel);
	}
	*ppOut = NULL;
	return(false);
}
bool XMETHODCALLTYPE CResourceManager::getModelAnimated(const char *szName, IXResourceModelAnimated **ppOut, bool bForceReload)
{
	IXResourceModel *pModel = NULL;
	if(getModel(szName, &pModel, bForceReload))
	{
		if(pModel->getType() == XMT_ANIMATED)
		{
			*ppOut = pModel->asAnimated();
			return(true);
		}
		mem_release(pModel);
	}
	*ppOut = NULL;
	return(false);
}

bool XMETHODCALLTYPE CResourceManager::getModelInfo(const char *szName, XModelInfo *pInfo)
{
	assert(szName);
	assert(pInfo);

	const char *szExt = getExtension(szName);

	if(!szExt || !*szExt)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid model name '%s'. Missing extension\n", szName);
		return(false);
	}

	char *szLowcaseExt = strdupa(szExt);
	strlwr(szLowcaseExt);

	const Map<AAString, Array<IXModelLoader*>>::Node *pNode;
	if(m_mapModelLoaders.KeyExists(AAString(szLowcaseExt), &pNode))
	{
		auto &aLoaders = *pNode->Val;
		if(!m_pCore->getFileSystem()->fileExists(szName))
		{
			LibReport(REPORT_MSG_LEVEL_ERROR, "File not found '%s'\n", szName);
			return(false);
		}
		bool isSuccess = false;

		for(UINT i = 0, l = aLoaders.size(); i < l; ++i)
		{
			IXModelLoader *pLoader = aLoaders[i];
			if(pLoader->open(szName))
			{
				pLoader->getInfo(pInfo);

				isSuccess = true;

				pLoader->close();
				break;
			}
		}

		if(isSuccess)
		{
			return(true);
		}

		LibReport(REPORT_MSG_LEVEL_ERROR, "No one plugin could load model '%s'\n", szName);
		return(false);
	}

	LibReport(REPORT_MSG_LEVEL_ERROR, "No plugins for model extension found '%s'\n", szName);
	return(false);
}

void CResourceManager::onResourceModelRelease(CResourceModel *pResource)
{
	const Map<String, IXResourceModel*>::Node *pNode;
	if(pResource->getFileName() && m_mpModels.KeyExists(pResource->getFileName(), &pNode) && *pNode->Val == pResource)
	{
		m_mpModels.erase(pResource->getFileName());
	}
}

UINT XMETHODCALLTYPE CResourceManager::getModelSupportedFormats()
{
	return(m_aModelExts.size());
}
const XFormatName* XMETHODCALLTYPE CResourceManager::getModelSupportedFormat(UINT uIndex)
{
	assert(uIndex < m_aModelExts.size());

	return(&m_aModelExts[uIndex]);
}

IXResourceModelStatic* XMETHODCALLTYPE CResourceManager::newResourceModelStatic()
{
	return(new CResourceModelStatic(this));
}
IXResourceModelAnimated* XMETHODCALLTYPE CResourceManager::newResourceModelAnimated()
{
	return(new CResourceModelAnimated(this));
}
void XMETHODCALLTYPE CResourceManager::addModel(const char *szName, IXResourceModel *pModel)
{
	CResourceModel *pResource = dynamic_cast<CResourceModel*>(pModel);
	assert(pResource);
	
	m_mpModels[szName] = pResource;
	pResource->setFileName(m_mpModels.TmpNode->Key.c_str());
}

//##########################################################################

UINT XMETHODCALLTYPE CResourceManager::getSoundSupportedFormats()
{
	return(m_aSoundExts.size());
}

const XFormatName* XMETHODCALLTYPE CResourceManager::getSoundSupportedFormat(UINT uIndex)
{
	assert(uIndex < m_aSoundExts.size());

	return(&m_aSoundExts[uIndex]);
}

//##########################################################################



bool XMETHODCALLTYPE CResourceManager::getTexture(const char *szName, IXResourceTexture **ppOut, bool bForceReload)
{
	const Map<String, IXResourceTexture*>::Node *pNode1;
	if(!bForceReload && m_mpTextures.KeyExists(szName, &pNode1))
	{
		IXResourceTexture *pOut = (*pNode1->Val);
		*ppOut = pOut;
		pOut->AddRef();
		return(true);
	}

	char *szFileName = strdupa(szName);
	char *szArg = NULL;
	char *szPound = strstr(szFileName, "#");
	if(szPound)
	{
		szArg = szPound + 1;
		*szPound = 0;
	}

	const char *szExt = getExtension(szFileName);

	if(!szExt || !*szExt)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid texture name '%s'. Missing extension\n", szName);
		return(false);
	}

	char *szLowcaseExt = strdupa(szExt);
	strlwr(szLowcaseExt);

	const Map<AAString, Array<TextureLoader>>::Node *pNode;
	if(m_mapTextureLoaders.KeyExists(AAString(szLowcaseExt), &pNode))
	{
		auto &aLoaders = *pNode->Val;
		IXResourceTexture *pResource = NULL;
		for(UINT i = 0, l = aLoaders.size(); i < l; ++i)
		{
			TextureLoader &loader = aLoaders[i];
			IXTextureLoader *pLoader = loader.pLoader;

			ScopedSpinLock lock(loader.slock);

			if(pLoader->open(szFileName, szArg))
			{
				switch(pLoader->getType())
				{
				case GXTEXTURE_TYPE_2D:
					{
						CResourceTexture2D *pTexture = new CResourceTexture2D(this);
						if(pLoader->loadAs2D(pTexture))
						{
							pTexture->genMipmaps();
							pResource = pTexture;
						}
						else
						{
							mem_delete(pTexture);
						}
					}
					break;
				case GXTEXTURE_TYPE_CUBE:
					{
						CResourceTextureCube *pTexture = new CResourceTextureCube(this);
						if(pLoader->loadAsCube(pTexture))
						{
							pResource = pTexture;
						}
						else
						{
							mem_delete(pTexture);
						}
					}
					break;
				}

				pLoader->close();

				if(pResource)
				{
					break;
				}
			}
		}

		if(pResource)
		{
			addTexture(szName, pResource);
			*ppOut = pResource;
			return(true);
		}

		LibReport(REPORT_MSG_LEVEL_ERROR, "No one plugin could load texture '%s'\n", szName);
		return(false);
	}

	LibReport(REPORT_MSG_LEVEL_ERROR, "No plugins for texture extension found '%s'\n", szName);
	return(false);
}
bool XMETHODCALLTYPE CResourceManager::getTexture2D(const char *szName, IXResourceTexture2D **ppOut, bool bForceReload)
{
	IXResourceTexture *pTexture = NULL;
	if(getTexture(szName, &pTexture, bForceReload))
	{
		if(pTexture->getType() == GXTEXTURE_TYPE_2D)
		{
			*ppOut = pTexture->as2D();
			return(true);
		}
		mem_release(pTexture);
	}
	*ppOut = NULL;
	return(false);
}
bool XMETHODCALLTYPE CResourceManager::getTextureCube(const char *szName, IXResourceTextureCube **ppOut, bool bForceReload)
{
	IXResourceTexture *pTexture = NULL;
	if(getTexture(szName, &pTexture, bForceReload))
	{
		if(pTexture->getType() == GXTEXTURE_TYPE_CUBE)
		{
			*ppOut = pTexture->asCube();
			return(true);
		}
		mem_release(pTexture);
	}
	*ppOut = NULL;
	return(false);
}

UINT XMETHODCALLTYPE CResourceManager::getTextureSupportedFormats()
{
	return(m_aTextureExts.size());
}
const XFormatName* XMETHODCALLTYPE CResourceManager::getTextureSupportedFormat(UINT uIndex)
{
	assert(uIndex < m_aTextureExts.size());

	return(&m_aTextureExts[uIndex]);
}

bool XMETHODCALLTYPE CResourceManager::getTextureInfo(const char *szName, XTextureInfo *pInfo)
{
	char *szFileName = strdupa(szName);
	char *szArg = NULL;
	char *szPound = strstr(szFileName, "#");
	if(szPound)
	{
		szArg = szPound + 1;
		*szPound = 0;
	}

	const char *szExt = getExtension(szFileName);

	if(!szExt || !*szExt)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid texture name '%s'. Missing extension\n", szName);
		return(false);
	}

	char *szLowcaseExt = strdupa(szExt);
	strlwr(szLowcaseExt);

	const Map<AAString, Array<TextureLoader>>::Node *pNode;
	if(m_mapTextureLoaders.KeyExists(AAString(szLowcaseExt), &pNode))
	{
		auto &aLoaders = *pNode->Val;
		bool isSuccess = false;
		for(UINT i = 0, l = aLoaders.size(); i < l; ++i)
		{
			TextureLoader &loader = aLoaders[i];
			IXTextureLoader *pLoader = loader.pLoader;

			ScopedSpinLock lock(loader.slock);

			if(pLoader->open(szFileName, szArg))
			{
				pLoader->getInfo(pInfo);

				isSuccess = true;

				pLoader->close();
				break;
			}
		}

		if(isSuccess)
		{
			return(true);
		}

		LibReport(REPORT_MSG_LEVEL_ERROR, "No one plugin could load texture '%s'\n", szName);
		return(false);
	}

	LibReport(REPORT_MSG_LEVEL_ERROR, "No plugins for texture extension found '%s'\n", szName);
	return(false);
}

IXResourceTexture2D* XMETHODCALLTYPE CResourceManager::newResourceTexture2D()
{
	return(new CResourceTexture2D(this));
}
IXResourceTextureCube* XMETHODCALLTYPE CResourceManager::newResourceTextureCube()
{
	return(new CResourceTextureCube(this));
}

void XMETHODCALLTYPE CResourceManager::addTexture(const char *szName, IXResourceTexture *pTexture)
{
	switch(pTexture->getType())
	{
	case GXTEXTURE_TYPE_2D:
		m_mpTextures[szName] = pTexture;
		((CResourceTexture2D*)pTexture->as2D())->setFileName(m_mpTextures.TmpNode->Key.c_str());
		break;
	case GXTEXTURE_TYPE_CUBE:
		m_mpTextures[szName] = pTexture;
		((CResourceTextureCube*)pTexture->asCube())->setFileName(m_mpTextures.TmpNode->Key.c_str());
		break;
	default:
		assert(!"Unknown texture type!");
	}
}

const char* CResourceManager::getExtension(const char *szName)
{
	size_t iLen = strlen(szName);
	const char *szExt = NULL;
	for(size_t i = iLen - 1; i > 0; --i)
	{
		if(szName[i] == '.')
		{
			szExt = &szName[i + 1];
			break;
		}
	}

	return(szExt);
}
