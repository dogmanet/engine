
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "ShaderManager.h"
#include "ShaderPreprocessor.h"

#define SX_SHADER_CACHE_MAGIC MAKEFOURCC('X', 'C', 'S', 'F') /*!< X Compiled Shader File*/
#define SX_SHADER_CACHE_VERSION 4

//##########################################################################

static void CreateShader(IGXDevice *pDevice, IGXPixelShader    **ppOut, const char *szCode, const char *szFile, GXMacro *pDefs)
{
	*ppOut = pDevice->createPixelShaderFromString(szCode, pDefs);
}
static void CreateShader(IGXDevice *pDevice, IGXVertexShader   **ppOut, const char *szCode, const char *szFile, GXMacro *pDefs)
{
	*ppOut = pDevice->createVertexShaderFromString(szCode, pDefs);
}
static void CreateShader(IGXDevice *pDevice, IGXGeometryShader **ppOut, const char *szCode, const char *szFile, GXMacro *pDefs)
{
	*ppOut = pDevice->createGeometryShaderFromString(szCode, pDefs);
}
static void CreateShader(IGXDevice *pDevice, IGXComputeShader  **ppOut, const char *szCode, const char *szFile, GXMacro *pDefs)
{
	*ppOut = pDevice->createComputeShaderFromString(szCode, pDefs);
}

static void CreateShaderFromData(IGXDevice *pDevice, IGXPixelShader    **ppOut, byte *pCode, UINT uSize)
{
	*ppOut = pDevice->createPixelShaderFromBin(pCode, uSize);
}
static void CreateShaderFromData(IGXDevice *pDevice, IGXVertexShader   **ppOut, byte *pCode, UINT uSize)
{
	*ppOut = pDevice->createVertexShaderFromBin(pCode, uSize);
}
static void CreateShaderFromData(IGXDevice *pDevice, IGXGeometryShader **ppOut, byte *pCode, UINT uSize)
{
	*ppOut = pDevice->createGeometryShaderFromBin(pCode, uSize);
}
static void CreateShaderFromData(IGXDevice *pDevice, IGXComputeShader  **ppOut, byte *pCode, UINT uSize)
{
	*ppOut = pDevice->createComputeShaderFromBin(pCode, uSize);
}

template <class T>
static int LoadShader(CShaderPreprocessor *pPreprocessor, IXCore *pCore, IGXDevice *pDevice, const char *szPath, CShaderImpl<T> *pShader, GXMacro *aMacro)
{
	assert(pCore->isOnMainThread());

	IFileSystem *pFileSystem = pCore->getFileSystem();

	char szFullPath[SXGC_SHADER_MAX_SIZE_FULLPATH];
	char szFullPathCache[SXGC_SHADER_MAX_SIZE_FULLPATH];
	char szDir[SXGC_SHADER_MAX_SIZE_DIR];
	szDir[0] = 0;
	char szName[SXGC_SHADER_MAX_SIZE_NAME];
	StrParsePathName(szPath, szDir, szName);
	//! @todo: place actual value here
	const char *szGAPI = "dx11";
	if(szDir[0] != 0)
	{
		sprintf(szFullPath, "shaders/%s/%s", szDir, szPath);
		sprintf(szFullPathCache, "cache/shaders/%s/%s/%sc", szGAPI, szDir, szPath);
	}
	else
	{
		sprintf(szFullPath, "shaders/%s", szPath);
		sprintf(szFullPathCache, "cache/shaders/%s/%sc", szGAPI, szPath);
	}
	bool isSourceExists = pFileSystem->fileExists(szFullPath);
	bool isCacheExists = pFileSystem->fileExists(szFullPathCache);
	if(!isSourceExists && !isCacheExists)
	{
		return(LOAD_SHADER_FAIL);
	}

	if(isCacheExists)
	{
		IFile *pCacheFile = pFileSystem->openFile(szFullPathCache);
		if(pCacheFile)
		{
			uint32_t uMagic = 0;
			pCacheFile->readBin(&uMagic, sizeof(uMagic));
			if(uMagic != SX_SHADER_CACHE_MAGIC)
			{
				LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid shader cache file (%s)\n", szFullPathCache);
			}
			else
			{
				uint32_t uVersion = 0;
				pCacheFile->readBin(&uVersion, sizeof(uVersion));
				if(uVersion != SX_SHADER_CACHE_VERSION)
				{
					LibReport(REPORT_MSG_LEVEL_WARNING, "Wrong shader cache file version (%s)\n", szFullPathCache);
				}
				else
				{
					uint32_t uMacroCount = 0;
					if(aMacro)
					{
						GXMacro *pMacro = aMacro;
						while(pMacro->szName)
						{
							++pMacro;
							++uMacroCount;
						}
					}

					do
					{
						size_t sizeChunkBegin = pCacheFile->getPos();
						uint32_t uChunkSize = 0;
						if(!pCacheFile->readBin(&uChunkSize, sizeof(uChunkSize)))
						{
							break;
						}
						size_t sizeNextBlockPos = pCacheFile->getPos() + uChunkSize;

						UINT uMacroCountFile = 0;
						pCacheFile->readBin(&uMacroCountFile, sizeof(uMacroCountFile));
						bool isMatched = uMacroCount == uMacroCountFile;
						if(isMatched)
						{
							for(UINT i = 0; i < uMacroCountFile; ++i)
							{
								uint16_t u16Len = 0;
								pCacheFile->readBin(&u16Len, sizeof(u16Len));
								char *szName = (char*)alloca(u16Len);
								pCacheFile->readBin(szName, u16Len);

								if(strcmp(aMacro[i].szName, szName))
								{
									isMatched = false;
									break;
								}

								u16Len = 0;
								pCacheFile->readBin(&u16Len, sizeof(u16Len));
								char *szDefinition = (char*)alloca(u16Len);
								pCacheFile->readBin(szDefinition, u16Len);

								if(strcmp(aMacro[i].szDefinition, szDefinition))
								{
									isMatched = false;
									break;
								}
							}
						}

						if(isMatched)
						{
							uint32_t uIncludeCount = 0;
							pCacheFile->readBin(&uIncludeCount, sizeof(uIncludeCount));
							bool isValid = true;
							for(UINT i = 0; i < uIncludeCount; ++i)
							{
								uint16_t u16Len = 0;
								pCacheFile->readBin(&u16Len, sizeof(u16Len));
								char *szName = (char*)alloca(u16Len);
								pCacheFile->readBin(szName, u16Len);

								time_t tMod = 0;
								pCacheFile->readBin(&tMod, sizeof(tMod));
								if(tMod != pFileSystem->getFileModifyTime(szName))
								{
									isValid = false;
								}
							}

							if(isValid || !isSourceExists)
							{
								UINT uCodeSize = 0;
								pCacheFile->readBin(&uCodeSize, sizeof(uCodeSize));
								byte *pCode;
								bool isFree = false;
								if(uCodeSize <= 32768)
								{
									pCode = (byte*)alloca(uCodeSize);
								}
								else
								{
									isFree = true;
									pCode = (byte*)malloc(uCodeSize);
								}
								pCacheFile->readBin(pCode, uCodeSize);

								T *pGXShader = NULL;
								CreateShaderFromData(pDevice, &pGXShader, pCode, uCodeSize);
								if(pGXShader)
								{
									mem_release(pCacheFile);
									pShader->m_pGXShader = pGXShader;
									return(LOAD_SHADER_CACHE);
								}

								if(isFree)
								{
									free(pCode);
								}
							}
							else
							{
								// clear from file
								// 0 - sizeChunkBegin
								// sizeNextBlockPos - end
								size_t sizeFile = pCacheFile->getSize();
								size_t sizeFileNew = sizeFile - (sizeNextBlockPos - sizeChunkBegin);
								byte *pBuffer;
								bool isFree = false;
								if(sizeFileNew <= 32768)
								{
									pBuffer = (byte*)alloca(sizeFileNew);
								}
								else
								{
									isFree = true;
									pBuffer = (byte*)malloc(sizeFileNew);
								}
								pCacheFile->setPos(0);
								pCacheFile->readBin(pBuffer, sizeChunkBegin);
								pCacheFile->setPos(sizeNextBlockPos);
								pCacheFile->readBin(pBuffer + sizeChunkBegin, sizeFile - sizeNextBlockPos);
								mem_release(pCacheFile);

								pCacheFile = pFileSystem->openFile(szFullPathCache, FILE_MODE_WRITE);
								if(pCacheFile)
								{
									pCacheFile->writeBin(pBuffer, sizeFileNew);
									mem_release(pCacheFile);
								}

								pCacheFile = pFileSystem->openFile(szFullPathCache);
								if(!pCacheFile)
								{
									break;
								}
								sizeNextBlockPos = sizeChunkBegin;

								if(isFree)
								{
									free(pBuffer);
								}
							}
						}

						pCacheFile->setPos(sizeNextBlockPos);
					}
					while(!pCacheFile->isEOF());
				}
			}

			mem_release(pCacheFile);
		}
	}

	if(!isSourceExists)
	{
		return(LOAD_SHADER_FAIL);
	}

	T *pGXShader = NULL;
	do
	{
		IFile *pFile = pFileSystem->openFile(szFullPath);
		if(pFile)
		{
			size_t sizeFile = pFile->getSize();
			char *szShader = (char*)alloca(sizeof(char) * (sizeFile + 1));
			pFile->readBin(szShader, sizeFile);
			szShader[sizeFile] = 0;
			uint32_t uMacroCount = 0;
			if(aMacro)
			{
				GXMacro *pMacro = aMacro;
				while(pMacro->szName)
				{
					char *szMacro = (char*)alloca(sizeof(char) * (strlen(pMacro->szName) + strlen(pMacro->szDefinition) + 2));
					sprintf(szMacro, "%s %s", pMacro->szName, pMacro->szDefinition);
					pPreprocessor->define(szMacro);
					++pMacro;
					++uMacroCount;
				}
			}
				
			String sResult = pPreprocessor->process(szShader, szFullPath);
			if(pPreprocessor->isError())
			{
				LibReport(REPORT_MSG_LEVEL_ERROR, "Preprocess shader error (%s):\n%s", szFullPath, pPreprocessor->getError());
			}
			else
			{
				CreateShader(pDevice, &pGXShader, sResult.c_str(), szFullPath, aMacro);

				if(pGXShader)
				{
					IFile *pCacheFile = pFileSystem->openFile(szFullPathCache, FILE_MODE_APPEND);
					if(!pCacheFile)
					{
						LibReport(REPORT_MSG_LEVEL_ERROR, "Unable to create shader cache file (%s):\n", szFullPathCache);
					}
					else
					{
						if(pCacheFile->getPos() == 0)
						{
							uint32_t uMagic = SX_SHADER_CACHE_MAGIC;
							pCacheFile->writeBin(&uMagic, sizeof(uMagic));
							uint32_t uVersion = SX_SHADER_CACHE_VERSION;
							pCacheFile->writeBin(&uVersion, sizeof(uVersion));
						}
						else
						{
							pCacheFile->setPos(0);
							uint32_t uMagic = 0;
							pCacheFile->readBin(&uMagic, sizeof(uMagic));
							uint32_t uVersion = 0;
							pCacheFile->readBin(&uVersion, sizeof(uVersion));
							pCacheFile->setPos(pCacheFile->getSize());

							if(uMagic != SX_SHADER_CACHE_MAGIC || uVersion != SX_SHADER_CACHE_VERSION)
							{
								mem_release(pCacheFile);
								pCacheFile = pFileSystem->openFile(szFullPathCache, FILE_MODE_WRITE);

								uint32_t uMagic = SX_SHADER_CACHE_MAGIC;
								pCacheFile->writeBin(&uMagic, sizeof(uMagic));
								uint32_t uVersion = SX_SHADER_CACHE_VERSION;
								pCacheFile->writeBin(&uVersion, sizeof(uVersion));
							}
						}

						if(!pCacheFile)
						{
							LibReport(REPORT_MSG_LEVEL_ERROR, "Unable to create shader cache file (%s):\n", szFullPathCache);
						}
						else
						{

							size_t sizeStartPos = pCacheFile->getPos();

							uint32_t uChunkSize = 0;
							pCacheFile->writeBin(&uChunkSize, sizeof(uChunkSize));

							pCacheFile->writeBin(&uMacroCount, sizeof(uMacroCount));

							for(UINT i = 0; i < uMacroCount; ++i)
							{
								uint16_t u16Len = (uint16_t)strlen(aMacro[i].szName) + 1;
								pCacheFile->writeBin(&u16Len, sizeof(u16Len));
								pCacheFile->writeBin(aMacro[i].szName, u16Len);

								u16Len = (uint16_t)strlen(aMacro[i].szDefinition) + 1;
								pCacheFile->writeBin(&u16Len, sizeof(u16Len));
								pCacheFile->writeBin(aMacro[i].szDefinition, u16Len);
							}

							uint32_t uIncludeCount = pPreprocessor->getIncludesCount();
							const char **pszIncludes = (const char**)alloca(sizeof(const char*) * uIncludeCount);
							pPreprocessor->getIncludes(pszIncludes);

							++uIncludeCount;
							pCacheFile->writeBin(&uIncludeCount, sizeof(uIncludeCount));

							uint16_t uLen = (uint16_t)strlen(szFullPath) + 1;
							pCacheFile->writeBin(&uLen, sizeof(uLen));
							pCacheFile->writeBin(szFullPath, uLen);

							time_t tMod = pFileSystem->getFileModifyTime(szFullPath);
							pCacheFile->writeBin(&tMod, sizeof(tMod));

							for(UINT i = 0; i < uIncludeCount - 1; ++i)
							{
								uLen = (uint16_t)strlen(pszIncludes[i]) + 1;
								pCacheFile->writeBin(&uLen, sizeof(uLen));
								pCacheFile->writeBin(pszIncludes[i], uLen);

								tMod = pFileSystem->getFileModifyTime(pszIncludes[i]);
								pCacheFile->writeBin(&tMod, sizeof(tMod));
							}
							UINT uCodeSize = 0;
							pGXShader->getData(NULL, &uCodeSize);
							byte *pCode;
							bool isFree = false;
							if(uCodeSize <= 32768)
							{
								pCode = (byte*)alloca(uCodeSize);
							}
							else
							{
								pCode = (byte*)malloc(uCodeSize);
								isFree = true;
							}
							pGXShader->getData(pCode, &uCodeSize);

							pCacheFile->writeBin(&uCodeSize, sizeof(uCodeSize));
							pCacheFile->writeBin(pCode, uCodeSize);

							uChunkSize = (uint32_t)(pCacheFile->getPos() - sizeStartPos - sizeof(uChunkSize));
							pCacheFile->setPos(sizeStartPos);
							pCacheFile->writeBin(&uChunkSize, sizeof(uChunkSize));

							mem_release(pCacheFile);

							if(isFree)
							{
								free(pCode);
							}
						}
					}
				}
			}
			pPreprocessor->reset();

			if(!pGXShader)
			{
				LibReport(REPORT_MSG_LEVEL_WARNING, " ===== Compiled with: =====\n");
				if(aMacro)
				{
					GXMacro *pMacro = aMacro;
					while(pMacro->szName)
					{
						LibReport(REPORT_MSG_LEVEL_WARNING, " '" COLOR_CYAN "%s" COLOR_YELLOW "' = '" COLOR_LGREEN "%s" COLOR_YELLOW "'\n", pMacro->szName, pMacro->szDefinition);
						++pMacro;
					}
				}
				LibReport(REPORT_MSG_LEVEL_WARNING, " ==========================\n\n");
			}

			mem_release(pFile);
		}

		if(!pGXShader)
		{
			pPreprocessor->clearIncludeCache();
		}
	}
	while(!pGXShader && MessageBoxA(NULL, "Unable to compile shader. Want to retry?", "Shader error", MB_OKCANCEL | MB_ICONSTOP) == IDOK);

	if(!pGXShader)
	{
		return(LOAD_SHADER_FAIL);
	}

	pShader->m_pGXShader = pGXShader;


	return LOAD_SHADER_COMPLETE;
}

//##########################################################################

CShaderManager::CShaderManager(IXCore *pCore, IGXDevice *pDevice):
	m_pDevice(pDevice),
	m_pCore(pCore),
	m_shaderReloadCommand(this)
{
	m_pPreprocessor = new CShaderPreprocessor(pCore->getFileSystem());
	m_pPreprocessor->addIncPath("shaders/");

	pCore->getConsole()->registerCVar("dev_shader_info", 1, "Shader loading verbosity: 0: no info; 1: progressbar; 2: details");
	pCore->getConsole()->registerCommand("shader_reload", &m_shaderReloadCommand, "Reload changed shaders");
}

CShaderManager::~CShaderManager()
{
	fora(i, m_aVS)
	{
		mem_delete(m_aVS[i]);
	}


	fora(i, m_aPS)
	{
		mem_delete(m_aPS[i]);
	}

	fora(i, m_aGS)
	{
		mem_delete(m_aGS[i]);
	}

	fora(i, m_aCS)
	{
		mem_delete(m_aCS[i]);
	}
			
	mem_delete(m_pPreprocessor);
}


static const char* GetTextResultLoad(int iResult)
{
	switch(iResult)
	{
	case LOAD_SHADER_FAIL:
		return(COLOR_LRED "fail" COLOR_RESET);
	case LOAD_SHADER_COMPLETE:
		return(COLOR_LGREEN "complete" COLOR_RESET);
	case LOAD_SHADER_CACHE:
		return(COLOR_LMAGENTA "cache" COLOR_RESET);
	}

	return("unknown");
}

template<class T>
static void ReloadShaders(Array<T*> &aShaders)
{
	for(UINT i = 0, l = aShaders.size(); i < l; ++i)
	{
		mem_release(aShaders[i]->m_pGXShader);
	}
}

void CShaderManager::reloadAll()
{
	LibReport(REPORT_MSG_LEVEL_NOTICE, "Reloading shaders\n");

	for(UINT i = 0, l = m_aShaderKit.size(); i < l; ++i)
	{
		if(m_aShaderKit[i])
		{
			mem_release(m_aShaderKit[i]->m_pShaderKit);
		}
	}

	ReloadShaders(m_aVS);
	ReloadShaders(m_aPS);
	ReloadShaders(m_aGS);
	ReloadShaders(m_aCS);

	m_iLastAllLoadVS = 0;
	m_iLastAllLoadPS = 0;
	m_iLastAllLoadGS = 0;
	m_iLastAllLoadCS = 0;

	mem_delete(m_pPreprocessor);
	m_pPreprocessor = new CShaderPreprocessor(m_pCore->getFileSystem());
	m_pPreprocessor->addIncPath("shaders/");

	allLoad(true);
}

ID CShaderManager::existsPathMacro(SHADER_TYPE type, const char *szPath, GXMacro *aMacros)
{
	ID idShader = -1;
	GXMacro *pShaderMacro = NULL;
	if(type == SHADER_TYPE_VERTEX)
	{
		fora(i, m_aVS)
		{
			if(m_aVS[i]->isSame(szPath, aMacros))
			{
				return(i);
			}
		}
	}
	else if(type == SHADER_TYPE_PIXEL)
	{
		fora(i, m_aPS)
		{
			if(m_aPS[i]->isSame(szPath, aMacros))
			{
				return(i);
			}
		}
	}
	else if(type == SHADER_TYPE_GEOMETRY)
	{
		fora(i, m_aGS)
		{
			if(m_aGS[i]->isSame(szPath, aMacros))
			{
				return(i);
			}
		}
	}
	else if(type == SHADER_TYPE_COMPUTE)
	{
		fora(i, m_aCS)
		{
			if(m_aCS[i]->isSame(szPath, aMacros))
			{
				return(i);
			}
		}
	}

	return(-1);
}

ID CShaderManager::preLoad(SHADER_TYPE type, const char *szPath, GXMacro *aMacros)
{
	ID id = existsPathMacro(type, szPath, aMacros);

	if(!ID_VALID(id))
	{
		ScopedSpinLock lock(m_spLock);

		CShader *pShader = 0;
		if(type == SHADER_TYPE_VERTEX)
		{
			CShaderVS *pVS = new CShaderVS();
			m_aVS.push_back(pVS);
			id = m_aVS.size() - 1;
			pShader = pVS;
		}
		else if(type == SHADER_TYPE_PIXEL)
		{
			CShaderPS *pPS = new CShaderPS();
			m_aPS.push_back(pPS);
			id = m_aPS.size() - 1;
			pShader = pPS;
		}
		else if(type == SHADER_TYPE_GEOMETRY)
		{
			CShaderGS *pGS = new CShaderGS();
			m_aGS.push_back(pGS);
			id = m_aGS.size() - 1;
			pShader = pGS;
		}
		else if(type == SHADER_TYPE_COMPUTE)
		{
			CShaderCS *pCS = new CShaderCS();
			m_aCS.push_back(pCS);
			id = m_aCS.size() - 1;
			pShader = pCS;
		}
		else
		{
			assert(!"Unknown shader type!");
			return(-1);
		}
		strcpy(pShader->m_szPath, szPath);
		
		if(aMacros)
		{
			int iCountMacros = 0;
			for(int i = 0; i < SXGC_SHADER_COUNT_MACRO; i++)
			{
				if(!aMacros[i].szName)
				{
					iCountMacros = i;
					break;
				}
				assert(i < SXGC_SHADER_COUNT_MACRO - 1);
			}

			for(int i = 0; i < iCountMacros; i++)
			{
				pShader->m_aMacros[i].szDefinition = strdup(aMacros[i].szDefinition);
				pShader->m_aMacros[i].szName = strdup(aMacros[i].szName);
			}

			pShader->m_aMacros[iCountMacros].szName = 0;
			pShader->m_aMacros[iCountMacros].szDefinition = 0;
		}
	}

	return(id);
}

template<typename T>
static void LoadArray(IXCore *pCore, IGXDevice *pDevice, Array<T*> &aList, const char *szType, CShaderPreprocessor *pPreprocessor)
{
	const char *szResult;

	static const int *dev_shader_info = pCore->getConsole()->getPCVarInt("dev_shader_info");
	static const int *con_width = pCore->getConsole()->getPCVarInt("con_width");
	int iShaderInfo = *dev_shader_info;
	int iConWidth = *con_width - 6 - (int)strlen(szType);

	UINT uCountForLoad = 0;
	if(iShaderInfo == 1)
	{
		for(UINT i = 0, l = aList.size(); i < l; ++i)
		{
			if(!aList[i]->m_pGXShader)
			{
				++uCountForLoad;
			}
		}
	}
	
	UINT uCountLoaded = 0;

	char *tmp = (char*)alloca(sizeof(char) * (iConWidth + 1));

	for(UINT i = 0, l = aList.size(); i < l; ++i)
	{
		T *pShader = aList[i];
		if(pShader->m_pGXShader)
		{
			continue;
		}
		szResult = GetTextResultLoad(LoadShader(pPreprocessor, pCore, pDevice, pShader->m_szPath, pShader, pShader->m_aMacros));
		++uCountLoaded;

		if(iShaderInfo == 2)
		{
			LibReport(REPORT_MSG_LEVEL_NOTICE, "  %s id [" COLOR_LCYAN "%d" COLOR_RESET "], file [" COLOR_LCYAN "%s" COLOR_RESET "], result [%s] \n", szType, i, pShader->m_szPath, szResult);
		}
		else if(iShaderInfo == 1)
		{
			int iCount = (int)(roundf((((float)uCountLoaded / (float)uCountForLoad) * (float)iConWidth)) + 0.5f);
			int j;
			for(j = 0; j < iCount; ++j)
			{
				tmp[j] = '=';
			}
			if(j && j != iConWidth)
			{
				tmp[j - 1] = '>';
			}
			for(; j < iConWidth; ++j)
			{
				tmp[j] = ' ';
			}
			tmp[iConWidth] = 0;
			LogInfo("\r%s: [%s]", szType, tmp);
		}
	}
	if(iShaderInfo == 1 && uCountLoaded)
	{
		LogInfo("\n");
	}
}

void CShaderManager::allLoad(bool bReload)
{
	ScopedSpinLock lock(m_spLock);

	if(m_aVS.size() == m_iLastAllLoadVS && m_aPS.size() == m_iLastAllLoadPS && m_aGS.size() == m_iLastAllLoadGS && m_aCS.size() == m_iLastAllLoadCS)
		return;

	DWORD dwTime = GetTickCount();
	if(!bReload)
	{
		LibReport(REPORT_MSG_LEVEL_NOTICE, "Loading shaders\n");
	}

	static const int *dev_shader_info = m_pCore->getConsole()->getPCVarInt("dev_shader_info");
	int iShaderInfo = *dev_shader_info;

	LoadArray(m_pCore, m_pDevice, m_aVS, "VS", m_pPreprocessor);
	if(iShaderInfo == 2)
	{
		LibReport(REPORT_MSG_LEVEL_NOTICE, "  -------\n");
	}

	LoadArray(m_pCore, m_pDevice, m_aPS, "PS", m_pPreprocessor);
	if(iShaderInfo == 2)
	{
		LibReport(REPORT_MSG_LEVEL_NOTICE, "  -------\n");
	}

	LoadArray(m_pCore, m_pDevice, m_aGS, "GS", m_pPreprocessor);
	if(iShaderInfo == 2)
	{
		LibReport(REPORT_MSG_LEVEL_NOTICE, "  -------\n");
	}

	LoadArray(m_pCore, m_pDevice, m_aCS, "CS", m_pPreprocessor);

	m_iLastAllLoadVS = m_aVS.size();
	m_iLastAllLoadPS = m_aPS.size();
	m_iLastAllLoadGS = m_aGS.size();
	m_iLastAllLoadCS = m_aCS.size();

	for(UINT i = 0, l = m_aShaderKit.size(); i < l; ++i)
	{
		if(m_aShaderKit[i] && !(m_aShaderKit[i]->m_pShaderKit))
		{
			ID idVS = m_aShaderKit[i]->m_idVertexShader;
			ID idPS = m_aShaderKit[i]->m_idPixelShader;
			ID idGS = m_aShaderKit[i]->m_idGeometryShader;
			ID idCS = m_aShaderKit[i]->m_idComputeShader;

			m_aShaderKit[i]->m_pShaderKit = m_pDevice->createShader(
				ID_VALID(idVS) ? m_aVS[idVS]->m_pGXShader : NULL,
				ID_VALID(idPS) ? m_aPS[idPS]->m_pGXShader : NULL,
				ID_VALID(idGS) ? m_aGS[idGS]->m_pGXShader : NULL,
				ID_VALID(idCS) ? m_aCS[idCS]->m_pGXShader : NULL
				);
		}
	}
	LibReport(REPORT_MSG_LEVEL_NOTICE, "all shaders %sloaded, time " COLOR_LCYAN "%dms" COLOR_RESET "\n", bReload ? "re" : "", GetTickCount() - dwTime);
}

ID CShaderManager::createKit(ID idVertexShader, ID idPixelShader, ID idGeometryShader, ID idComputeShader)
{
	if(
		!(ID_VALID(idVertexShader) && (UINT)idVertexShader < m_aVS.size()) && 
		!(ID_VALID(idPixelShader) && (UINT)idPixelShader < m_aPS.size()) &&
		!(ID_VALID(idGeometryShader) && (UINT)idGeometryShader < m_aGS.size()) &&
		!(ID_VALID(idComputeShader) && (UINT)idComputeShader < m_aCS.size())
		)
		return -1;

	ID idShaderKit = -1;

	for (int i = 0, il = m_aShaderKit.size(); i < il; ++i)
	{
		if(m_aShaderKit[i] && 
			m_aShaderKit[i]->m_idVertexShader == idVertexShader && 
			m_aShaderKit[i]->m_idPixelShader == idPixelShader && 
			m_aShaderKit[i]->m_idGeometryShader == idGeometryShader &&
			m_aShaderKit[i]->m_idComputeShader == idComputeShader
			)
		{
			idShaderKit = i;
			break;
		}
	}

	if (idShaderKit == -1)
	{
		CShaderKit *pShaderKit = new CShaderKit();
		pShaderKit->m_idVertexShader = idVertexShader;
		pShaderKit->m_idPixelShader = idPixelShader;
		pShaderKit->m_idGeometryShader = idGeometryShader;
		pShaderKit->m_idComputeShader = idComputeShader;

		if(
			(!ID_VALID(idVertexShader) || m_aVS[idVertexShader]->m_pGXShader) &&
			(!ID_VALID(idPixelShader) || m_aPS[idPixelShader]->m_pGXShader) &&
			(!ID_VALID(idGeometryShader) || m_aGS[idGeometryShader]->m_pGXShader) &&
			(!ID_VALID(idComputeShader) || m_aCS[idComputeShader]->m_pGXShader)
			)
			pShaderKit->m_pShaderKit = m_pDevice->createShader(
				ID_VALID(idVertexShader) ? m_aVS[idVertexShader]->m_pGXShader : NULL,
				ID_VALID(idPixelShader) ? m_aPS[idPixelShader]->m_pGXShader : NULL,
				ID_VALID(idGeometryShader) ? m_aGS[idGeometryShader]->m_pGXShader : NULL,
				ID_VALID(idComputeShader) ? m_aCS[idComputeShader]->m_pGXShader : NULL
				);

		m_aShaderKit.push_back(pShaderKit);
		idShaderKit = m_aShaderKit.size() - 1;
	}

	return idShaderKit;
}

void CShaderManager::bind(ID idShaderKit, IGXContext *pCtx)
{
	if (!(ID_VALID(idShaderKit) && (UINT)idShaderKit < m_aShaderKit.size() && m_aShaderKit[idShaderKit]))
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "shader kit not bind, id [%d]\n", idShaderKit);
		return;
	}

	(pCtx ? pCtx : m_pDevice->getThreadContext())->setShader(m_aShaderKit[idShaderKit]->m_pShaderKit);
}

void CShaderManager::unbind(IGXContext *pCtx)
{
	(pCtx ? pCtx : m_pDevice->getThreadContext())->setShader(0);
}

//##########################################################################

CShaderReloadCommand::CShaderReloadCommand(CShaderManager *pShaderManager):
	m_pManager(pShaderManager)
{
}

void XMETHODCALLTYPE CShaderReloadCommand::execute(int argc, const char **argv)
{
	m_pManager->reloadAll();
}
