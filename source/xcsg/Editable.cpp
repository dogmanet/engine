#include "Editable.h"
#include "EditorObject.h"
#include "EditorModel.h"
#include <xcommon/IPluginManager.h>
#include <xcommon/IXModelWriter.h>
#include "Combiner.h"

CEditable::CEditable(IXCore *pCore):
	m_pCore(pCore)
{
	IXEditor *pEditor = (IXEditor*)pCore->getPluginManager()->getInterface(IXEDITOR_GUID);
	m_pEditorExtension = new CEditorExtension(this, pEditor);
}

CEditable::~CEditable()
{
	shutdown();
	mem_delete(m_pEditorExtension);
}

void XMETHODCALLTYPE CEditable::startup(IGXDevice *pDevice)
{
	m_pDevice = pDevice;

	//GXBlendDesc blendDesc;
	//blendDesc.renderTarget[0].useBlend = true;
	//blendDesc.renderTarget[0].blendSrcColor = blendDesc.renderTarget[0].blendSrcAlpha = GXBLEND_BLEND_FACTOR;
	//blendDesc.renderTarget[0].blendDestColor = GXBLEND_ONE;
	//blendDesc.renderTarget[0].blendDestAlpha = GXBLEND_INV_BLEND_FACTOR;
	//m_pBlendColorFactor = m_pDevice->createBlendState(&blendDesc);

	//GXRasterizerDesc rsDesc;
	//rsDesc.cullMode = GXCULL_NONE;
	//rsDesc.fillMode = GXFILL_WIREFRAME;
	//m_pRSWireframe = m_pDevice->createRasterizerState(&rsDesc);

	//IPluginManager *pPluginManager = m_pCore->getPluginManager();
	//IXMaterialSystem *pMaterialSystem = (IXMaterialSystem*)pPluginManager->getInterface(IXMATERIALSYSTEM_GUID);
	//m_pMaterialSystem = pMaterialSystem;

	//pMaterialSystem->getTexture("dev_white", &m_pWhiteTexture);
}
void XMETHODCALLTYPE CEditable::shutdown()
{
	//mem_release(m_pWhiteTexture);
	//mem_release(m_pBlendColorFactor);
	//mem_release(m_pRSWireframe);
}

UINT XMETHODCALLTYPE CEditable::getObjectCount()
{
	return(m_aObjects.size());
}
IXEditorObject* XMETHODCALLTYPE CEditable::getObject(UINT id)
{
	return(m_aObjects[id]);
}
IXEditorObject* XMETHODCALLTYPE CEditable::newObject(const char *szClassName)
{
	if(!fstrcmp(szClassName, "Brush"))
	{
		return(new CEditorObject(this));
	}
	return(NULL);
	//CEditorObject *pObj = new CEditorObject(this, szClassName);
	//m_aObjects.push_back(pObj);
	//return(pObj);
}

void CEditable::removeObject(CEditorObject *pObject)
{
	int idx = m_aObjects.indexOf(pObject);
	if(idx >= 0)
	{
		m_aObjects.erase(idx);
	}
}

//void CEditable::resync()
//{
//	for(UINT i = 0, l = m_aObjects.size(); i < l; ++i)
//	{
//		m_aObjects[i]->resync();
//	}
//}

void CEditable::onSelectionChanged(CEditorObject *pObject)
{
	m_pEditorExtension->onSelectionChanged(pObject);
}

void CEditable::save(const char *szLevelName)
{
	char szFile[1024];
	sprintf(szFile, "levels/%s/editor/xcsg.json", szLevelName);

	IFileSystem *pFS = m_pCore->getFileSystem();

	IFile *pFile = pFS->openFile(szFile, FILE_MODE_WRITE);
	if(!pFile)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Unable to save data '%s'\n", szFile);
		return;
	}

	pFile->writeText("{\"brushes\":[\n");

	bool isFirst = true;

	fora(i, m_aObjects)
	{
		CEditorObject *pObj = m_aObjects[i];
		if(pObj->isRemoved())
		{
			continue;
		}
		pFile->writeText("\t%s{\n", isFirst ? "" : ",");
		isFirst = false;
		//float3_t vPos = pObj->getPos();
		//pFile->writeText("\t\t\"_origin\": [%g,%g,%g]\n", vPos.x, vPos.y, vPos.z);
		for(UINT j = 0, jl = pObj->getProperyCount(); j < jl; ++j)
		{
			const char *pKey = pObj->getPropertyMeta(j)->szKey;
			pFile->writeText("\t\t%s\"%s\": %s\n", j == 0 ? "" : ",", pKey, pObj->getKV(pKey, true));
		}
		pFile->writeText("\t}\n");
	}

	pFile->writeText("\n]\n,\"models\":[\n");

	char tmp[64];
	isFirst = true;
	fora(i, m_aModels)
	{
		CEditorModel *pMdl = m_aModels[i];
		if(!pMdl->getObjectCount())
		{
			continue;
		}
		bool isValid = false;
		for(UINT j = 0, jl = pMdl->getObjectCount(); j < jl && !isValid; ++j)
		{
			IXEditorObject *pObj;
			if(pMdl->getObject(j, &pObj))
			{
				if(!((CEditorObject*)pObj)->isRemoved())
				{
					isValid = true;
				}
				mem_release(pObj);
			}
		}
		if(!isValid)
		{
			continue;
		}
		pFile->writeText("\t%s{\n", isFirst ? "" : ",");
		isFirst = false;

		XGUIDToSting(*pMdl->getGUID(), tmp, sizeof(tmp));
		pFile->writeText("\t\t\"guid\": \"%s\"\n", tmp);
		pFile->writeText("\t\t,\"o\": [\n", tmp);

		for(UINT j = 0, jl = pMdl->getObjectCount(); j < jl; ++j)
		{
			IXEditorObject *pObj;
			if(pMdl->getObject(j, &pObj))
			{
				XGUIDToSting(*pObj->getGUID(), tmp, sizeof(tmp));
				pFile->writeText("\t\t\t%s\"%s\"\n", j == 0 ? "" : ",", tmp);
				mem_release(pObj);
			}
		}

		pFile->writeText("\t\t]\n", tmp);

		pFile->writeText("\t}\n");
	}

	pFile->writeText("\n]}\n");

	mem_release(pFile);

	sprintf(szFile, "levels/%s/xcsg", szLevelName);
	pFS->deleteDirectory(szFile);

	IXModelWriter *pWriter = (IXModelWriter*)m_pCore->getPluginManager()->getInterface(IXMODELWRITER_GUID);
	if(pWriter)
	{
		CCombiner combiner(this);
		combiner.build();
		IXResourceModel *pResource;
		for(UINT i = 0, l = combiner.getResourceCount(); i < l; ++i)
		{
			combiner.getResourceAt(i, &pResource);
			sprintf(szFile, "levels/%s/xcsg/%u.dse", szLevelName, i);
			IFile *pFile = pFS->openFile(szFile, FILE_MODE_WRITE);
			if(pFile)
			{
				pWriter->writeModel(pResource, pFile);
				mem_release(pFile);
			}
			else
			{
				LibReport(REPORT_MSG_LEVEL_ERROR, "Unable to save data '%s'\n", szFile);
			}
			
			mem_release(pResource);
		}
	}
	else
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Unable to save world models! No writer available\n");
	}
}

void CEditable::load(const char *szLevelName, ID idPlugin)
{
	char szFile[1024];
	sprintf(szFile, "levels/%s/editor/xcsg.json", szLevelName);

	IFile *pFile = m_pCore->getFileSystem()->openFile(szFile, FILE_MODE_READ);
	if(!pFile)
	{
		return;
	}

	LibReport(REPORT_MSG_LEVEL_NOTICE, "loading level\n");

	IEventChannel<XEventLevelProgress> *pProgressChannel = m_pCore->getEventChannel<XEventLevelProgress>(EVENT_LEVEL_PROGRESS_GUID);
	XEventLevelProgress levelProgress;
	levelProgress.idPlugin = idPlugin;
	levelProgress.fProgress = 0.0f;
	levelProgress.szLoadingText = "Загрузка мира";
	levelProgress.type = XEventLevelProgress::TYPE_PROGRESS_BEGIN;
	pProgressChannel->broadcastEvent(&levelProgress);

	//loadLevel(szPathLevel);

	size_t sizeFile = pFile->getSize();
	char *szJSON = new char[sizeFile + 1];
	pFile->readBin(szJSON, sizeFile);
	szJSON[sizeFile] = 0;

	bool isLoaded = false;

	IXJSON *pJSON = (IXJSON*)m_pCore->getPluginManager()->getInterface(IXJSON_GUID);
	IXJSONItem *pRoot;
	if(pJSON->parse(szJSON, &pRoot))
	{
		IXJSONObject *pRootObj = pRoot->asObject();
		if(pRootObj)
		{
			for(UINT i = 0, l = pRootObj->size(); i < l; ++i)
			{
				const char *szKey = pRootObj->getKey(i);
				if(!strcmp(szKey, "brushes"))
				{
					IXJSONArray *pArr = pRootObj->at(i)->asArray();
					if(pArr)
					{
						for(UINT j = 0, jl = pArr->size(); j < jl; ++j)
						{
							IXJSONObject *pBrushObj = pArr->at(j)->asObject();
							if(pBrushObj)
							{
								CEditorObject *pObj = new CEditorObject(this);
								for(UINT k = 0, kl = pBrushObj->size(); k < kl; ++k)
								{
									pObj->setKV(pBrushObj->getKey(k), pBrushObj->at(k), true);
								}
							}
						}
						isLoaded = true;
					}
				}
				else if(!strcmp(szKey, "models"))
				{
					IXJSONArray *pArr = pRootObj->at(i)->asArray();
					if(pArr)
					{
						for(UINT j = 0, jl = pArr->size(); j < jl; ++j)
						{
							IXJSONObject *pModelObj = pArr->at(j)->asObject();
							if(pModelObj)
							{
								IXJSONItem *pGuidItem = pModelObj->getItem("guid");
								const char *szGUID = NULL;
								XGUID guid;
								if(!pGuidItem || !(szGUID = pGuidItem->getString()) || !XGUIDFromString(&guid, szGUID))
								{
									LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid model '%u' in '%s'. Invalid GUID\n", j, szFile);
									continue;
								}
								IXJSONItem *pOItem = pModelObj->getItem("o");
								IXJSONArray *pOArr = NULL;
								if(!pOItem || !(pOArr = pOItem->asArray()))
								{
									LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid model '%u' in '%s'. Missing 'o' key\n", j, szFile);
									continue;
								}

								if(!pOArr->size())
								{
									LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid model '%u' in '%s'. No child objects\n", j, szFile);
									continue;
								}

								CEditorModel *pMdl = new CEditorModel(this, m_pCore, guid);
								m_aModels.push_back(pMdl);

								for(UINT k = 0, kl = pOArr->size(); k < kl; ++k)
								{
									szGUID = pOArr->at(k)->getString();
									if(!szGUID || !XGUIDFromString(&guid, szGUID))
									{
										LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid object '%u' guid in model '%u' in '%s'. '%s'\n", k, j, szFile, szGUID ? szGUID : "");
										continue;
									}

									CEditorObject *pObj = getObjectByGUID(guid);

									if(!pObj)
									{
										LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid object '%s' in model '%u' in '%s'. The object is not found\n", szGUID, j, szFile);
										continue;
									}

									pMdl->addObject(pObj);
								}

								if(!pMdl->getObjectCount())
								{
									mem_release(pMdl);
									LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid model '%u' in '%s'. No child object found\n", j, szFile);
									continue;
								}
							}
						}
					}
				}
			}

		}

		mem_release(pRoot);
	}

	if(!isLoaded)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Unable to load '%s'\n", szFile);
	}

	mem_delete_a(szJSON);

	levelProgress.fProgress = 1.0f;
	levelProgress.type = XEventLevelProgress::TYPE_PROGRESS_END;
	pProgressChannel->broadcastEvent(&levelProgress);
	

	mem_release(pFile);
}

CEditorObject* CEditable::getObjectByGUID(const XGUID &guid)
{
	fora(i, m_aObjects)
	{
		if(*m_aObjects[i]->getGUID() == guid)
		{
			return(m_aObjects[i]);
		}
	}

	return(NULL);
}

void CEditable::unload()
{
	for(int i = m_aModels.size() - 1; i >= 0; --i)
	{
		mem_release(m_aModels[i]);
	}
	m_aModels.clearFast();

	for(int i = m_aObjects.size() - 1; i >= 0; --i)
	{
		mem_release(m_aObjects[i]);
	}
	m_aObjects.clearFast();
}

bool XMETHODCALLTYPE CEditable::buildModelFromSelection(IXEditorModel **ppOut)
{
	bool hasSelection = false;
	fora(i, m_aObjects)
	{
		CEditorObject *pObj = m_aObjects[i];
		if(pObj->isSelected())
		{
			hasSelection = true;
			break;
		}
	}
	if(!hasSelection)
	{
		return(false);
	}

	CEditorModel *pMdl = new CEditorModel(this, m_pCore);
	m_aModels.push_back(pMdl);
	*ppOut = pMdl;
	add_ref(pMdl);

	return(true);
}
bool XMETHODCALLTYPE CEditable::newModel(IXEditorModel **ppOut)
{
	XGUID guid;
	XCreateGUID(&guid);
	CEditorModel *pMdl = new CEditorModel(this, m_pCore, guid);
	m_aModels.push_back(pMdl);
	*ppOut = pMdl;
	add_ref(pMdl);

	return(true);
}
UINT XMETHODCALLTYPE CEditable::getModelCount()
{
	return(m_aModels.size());
}
bool XMETHODCALLTYPE CEditable::getModel(UINT id, IXEditorModel **ppOut)
{
	assert(id < m_aModels.size());

	if(id < m_aModels.size())
	{
		*ppOut = m_aModels[id];
		add_ref(*ppOut);
		return(true);
	}

	return(false);
}

void CEditable::onModelDestroy(CEditorModel *pModel)
{
	int idx = m_aModels.indexOf(pModel);
	if(idx >= 0)
	{
		m_aModels.erase(idx);
		mem_release(pModel);
	}
}

void CEditable::onModelRestored(CEditorModel *pModel)
{
	int idx = m_aModels.indexOf(pModel);
	assert(idx < 0);
	if(idx < 0)
	{
		add_ref(pModel);
		m_aModels.push_back(pModel);
	}
}

CVertexTool* CEditable::getVertexTool()
{
	return(m_pEditorExtension->getVertexTool());
}
