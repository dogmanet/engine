#include <xcommon/IXPlugin.h>
#include <game/sxgame.h>


#if defined(_DEBUG)
#pragma comment(lib, "sxgame_d.lib")
#else
#pragma comment(lib, "sxgame.lib")
#endif

//! магическое число для идентификации файла статики
#define SX_GEOM_MAGIC_NUM	2314970533678512243

//! версия формата файла статики
#define SX_GEOM_FILE_FORMAT_VERSION 1

struct vertex_static
{
	float3_t Pos;  /*!< Позиция */
	float2_t Tex;  /*!< Текстурные координаты */
	float3_t Norm; /*!< Нормаль */
};


class CLevelLoadListener: public IEventListener<XEventLevel>
{
public:
	CLevelLoadListener(IXCore *pCore, ID idPlugin):
		m_pCore(pCore),
		m_idPlugin(idPlugin)
	{
	}
	void onEvent(const XEventLevel *pData) override
	{
		char szPathLevel[1024];

		switch(pData->type)
		{
		case XEventLevel::TYPE_LOAD:
			sprintf(szPathLevel, "levels/%s/%s.geom", pData->szLevelName, pData->szLevelName);
			LibReport(REPORT_MSG_LEVEL_NOTICE, "loading level\n");
			//if(FileExistsFile(szPathLevel))
			{
				IEventChannel<XEventLevelProgress> *pProgressChannel = m_pCore->getEventChannel<XEventLevelProgress>(EVENT_LEVEL_PROGRESS_GUID);
				XEventLevelProgress levelProgress;
				levelProgress.idPlugin = m_idPlugin;
				levelProgress.fProgress = 0.0f;
				levelProgress.szLoadingText = "Импорт геометрии";
				levelProgress.type = XEventLevelProgress::TYPE_PROGRESS_BEGIN;
				pProgressChannel->broadcastEvent(&levelProgress);

				loadLevel(szPathLevel);

				levelProgress.fProgress = 1.0f;
				levelProgress.type = XEventLevelProgress::TYPE_PROGRESS_END;
				pProgressChannel->broadcastEvent(&levelProgress);
			}
			break;
		case XEventLevel::TYPE_UNLOAD:
			break;
		case XEventLevel::TYPE_SAVE:
		{
			char szPathLevel2[1024];
			sprintf(szPathLevel, "levels/%s/%s.geom", pData->szLevelName, pData->szLevelName);
			sprintf(szPathLevel2, "levels/%s/%s.geom~", pData->szLevelName, pData->szLevelName);
			MoveFileA(szPathLevel, szPathLevel2);
		}
		break;
		}
	}

	// https://dev.ds-servers.com/sip/engine/-/blob/8306303dda397cb047048559a29ea8051822fa04/source/geom/static_geom.cpp
	bool loadOld(FILE *pFile)
	{
		int32_t countgroup = -1;
		fread(&countgroup, sizeof(int32_t), 1, pFile);

		for(int i = 0; i < countgroup; ++i)
		{
			int32_t tmpstrlen;
			fread(&tmpstrlen, sizeof(int32_t), 1, pFile);
			fseek(pFile, sizeof(char) * tmpstrlen, SEEK_CUR);

			//записываем количество буферов в подгруппе
			int32_t countbuffingroup = -1;
			fread(&countbuffingroup, sizeof(int32_t), 1, pFile);

			for(int k = 0; k < countbuffingroup; ++k)
			{
				//записываем количество моделей, которые используют данную подгруппу
				int32_t countusingmodels;
				fread(&countusingmodels, sizeof(int32_t), 1, pFile);
				for(int j = 0; j < countusingmodels; ++j)
				{
					fseek(pFile, sizeof(int32_t), SEEK_CUR);
				}

				int32_t iCountVertex, iCountIndex;

				fread(&iCountVertex, sizeof(int32_t), 1, pFile);
				fread(&iCountIndex, sizeof(int32_t), 1, pFile);

				fseek(pFile, sizeof(vertex_static)* iCountVertex, SEEK_CUR);
			}
		}

		int32_t countmodels;
		fread(&countmodels, sizeof(int32_t), 1, pFile);

		for(int i = 0; i < countmodels; ++i)
		{
			int32_t countsubset;
			fread(&countsubset, sizeof(int32_t), 1, pFile);

			CBaseEntity *pEntity = SGame_CreateEntity("prop_static");

			int32_t iStrLen = 0;
			char szStr[1024];

			fread(&iStrLen, sizeof(int32_t), 1, pFile);
			fread(szStr, sizeof(char), iStrLen, pFile);
			szStr[iStrLen] = 0;
			pEntity->setKV("name", szStr);

			fread(&iStrLen, sizeof(int32_t), 1, pFile);
			strcpy(szStr, "meshes/");
			fread(szStr + 7, sizeof(char), iStrLen, pFile);
			szStr[iStrLen + 7] = 0;
			pEntity->setKV("model", szStr);

			int32_t iCountPoly;
			fread(&iCountPoly, sizeof(int32_t), 1, pFile);

			//считываем трансформации
			//{
			float3 vPosition, vRotation, vScale;
			fread(&(vPosition.x), sizeof(float), 1, pFile);
			fread(&(vPosition.y), sizeof(float), 1, pFile);
			fread(&(vPosition.z), sizeof(float), 1, pFile);
			pEntity->setPos(vPosition);

			fread(&(vRotation.x), sizeof(float), 1, pFile);
			fread(&(vRotation.y), sizeof(float), 1, pFile);
			fread(&(vRotation.z), sizeof(float), 1, pFile);
			pEntity->setOrient(SMQuaternion(-vRotation.x, 'x') * SMQuaternion(-vRotation.y, 'y') * SMQuaternion(-vRotation.z, 'z'));

			fread(&(vScale.x), sizeof(float), 1, pFile);
			fread(&(vScale.y), sizeof(float), 1, pFile);
			fread(&(vScale.z), sizeof(float), 1, pFile);
			sprintf_s(szStr, "%f", vScale.y);
			pEntity->setKV("scale", szStr);
			//}

			pEntity->setKV("use_trimesh", "1");

			fread(&iStrLen, sizeof(int32_t), 1, pFile);
			if(iStrLen > 0)
			{
				fseek(pFile, sizeof(char) * iStrLen, SEEK_SET);
			}


			pEntity->setFlags(pEntity->getFlags() | EF_LEVEL | EF_EXPORT);
#if 0
			for(int k = 0; k < countsubset; ++k)
			{
				CModel::CSubSet gdb;
				fread(&gdb, sizeof(CModel::CSubSet), 1, file);
				m_aAllModels[i]->m_aSubSets.push_back(gdb);

				char tmptex[1024];
				sprintf(tmptex, "%s.dds", m_aAllGroups[gdb.m_idGroup]->m_sName.c_str());
				m_aAllModels[i]->m_aIDsTexures[k] = SGCore_MtlLoad(tmptex, MTL_TYPE_GEOM);
			}

			Array<CSegment**> queue;
			int tmpcount = 0;
			queue.push_back(&(m_aAllModels[i]->m_pArrSplits));

			while(queue.size())
			{
				loadSplit(queue[0], file, &queue);

				queue.erase(0);
				++tmpcount;
			}
#endif

			break;
		}


		return(false);
	}

	void loadLevel(const char *szPath)
	{
		FILE *pFile = fopen(szPath, "rb");

		if(!pFile)
		{
		//	LibReport(REPORT_MSG_LEVEL_ERROR, "failed to open (load) file static geometry [%s]\n", szPath);
			return;
		}

		//первичные проверки
		//{
		//считывание и проверка магического слова
		uint64_t ui64Magic;
		fread(&ui64Magic, sizeof(uint64_t), 1, pFile);

		if(ui64Magic != SX_GEOM_MAGIC_NUM)
		{
			fseek(pFile, 0, SEEK_SET);
			if(!loadOld(pFile))
			{
				LibReport(REPORT_MSG_LEVEL_ERROR, "file static geometry [%s] is not static geometry\n", szPath);
			}
			fclose(pFile);
			return;
		}

		//считывание и проверка поддерживаемой версии
		uint32_t uiFmtVersion;
		fread(&uiFmtVersion, sizeof(uint32_t), 1, pFile);

		if(uiFmtVersion != SX_GEOM_FILE_FORMAT_VERSION)
		{
			LibReport(REPORT_MSG_LEVEL_ERROR, "file static geometry [%s] have unsuported version %d\n", szPath, uiFmtVersion);
			fclose(pFile);
			return;
		}

		//считывание и проверка размера файла
		uint32_t uiSizeFile;
		fread(&uiSizeFile, sizeof(uint32_t), 1, pFile);
		long lCurrPos = ftell(pFile);
		fseek(pFile, 0, SEEK_END);

		if(uiSizeFile != ftell(pFile))
		{
			LibReport(REPORT_MSG_LEVEL_ERROR, "file static geometry [%s] damaged, whole file %d\n", szPath);
			fclose(pFile);
			return;
		}

		fseek(pFile, lCurrPos, SEEK_SET);
		//}

		int32_t iCountModel = 0;
		fread(&iCountModel, sizeof(int32_t), 1, pFile);

		for(int i = 0; i < iCountModel; ++i)
		{
			uint32_t uiSizeBlock = 0;
			uint32_t uNextBlock = ftell(pFile);
			fread(&uiSizeBlock, sizeof(uint32_t), 1, pFile);
			uNextBlock += uiSizeBlock;

			if(uiSizeBlock >= uiSizeFile)
			{
				LibReport(REPORT_MSG_LEVEL_ERROR, "file static geometry [%s] damaged, block size (model) %d\n", szPath);
				fclose(pFile);
				return;
			}


			CBaseEntity *pEntity = SGame_CreateEntity("prop_static");

			int32_t iStrLen = 0;
			char szStr[1024];

			fread(&iStrLen, sizeof(int32_t), 1, pFile);
			fread(szStr, sizeof(char), iStrLen, pFile);
			szStr[iStrLen] = 0;
			pEntity->setKV("name", szStr);

			//считываем трансформации
			//{
			float3 vPosition, vRotation, vScale;
			fread(&(vPosition.x), sizeof(float), 1, pFile);
			fread(&(vPosition.y), sizeof(float), 1, pFile);
			fread(&(vPosition.z), sizeof(float), 1, pFile);
			pEntity->setPos(vPosition);

			fread(&(vRotation.x), sizeof(float), 1, pFile);
			fread(&(vRotation.y), sizeof(float), 1, pFile);
			fread(&(vRotation.z), sizeof(float), 1, pFile);
			pEntity->setOrient(SMQuaternion(-vRotation.x, 'x') * SMQuaternion(-vRotation.y, 'y') * SMQuaternion(-vRotation.z, 'z'));

			fread(&(vScale.x), sizeof(float), 1, pFile);
			fread(&(vScale.y), sizeof(float), 1, pFile);
			fread(&(vScale.z), sizeof(float), 1, pFile);
			sprintf_s(szStr, "%f", vScale.y);
			pEntity->setKV("scale", szStr);
			//}

			pEntity->setKV("use_trimesh", "1");

			fread(&iStrLen, sizeof(int32_t), 1, pFile);
			strcpy(szStr, "meshes/");
			fread(szStr + 7, sizeof(char), iStrLen, pFile);
			szStr[iStrLen + 7] = 0;
			pEntity->setKV("model", szStr);


			pEntity->setFlags(pEntity->getFlags() | EF_LEVEL | EF_EXPORT);
			
			fseek(pFile, uNextBlock, SEEK_SET);

#if 0
			//создаем статическую модель и считываем для нее данные
			//{
			UINT uiSubsetCount = 0;
			UINT uiAllVertexCount = 0;
			UINT uiAllIndexCount = 0;
			fread(&uiSubsetCount, sizeof(uint32_t), 1, pFile);
			

			//если модель не пустая (есть не пп подгруппы)
			if(uiSubsetCount)
			{
				fread(&uiAllVertexCount, sizeof(uint32_t), 1, pFile);
				fread(&uiAllIndexCount, sizeof(uint32_t), 1, pFile);

				for(int k = 0; k < uiSubsetCount; ++k)
				{
					fread(&iStrLen, sizeof(int32_t), 1, pFile);
					fread(szStr, sizeof(char), iStrLen, pFile);
					szStr[iStrLen] = 0;
				}

				fseek(pFile, sizeof(uint32_t) * uiSubsetCount * 4, SEEK_CUR);
				fseek(pFile, sizeof(vertex_static_ex) * uiAllVertexCount, SEEK_CUR);
				fseek(pFile, sizeof(uint32_t) * uiAllIndexCount, SEEK_CUR);
			}
			//}

			//считываем лод, если есть
			//{
			uint8_t ui8ExistsLod = 0;
			fread(&ui8ExistsLod, sizeof(uint8_t), 1, pFile);

			if(ui8ExistsLod)
			{

				fread(&iStrLen, sizeof(int32_t), 1, pFile);
				fread(szStr, sizeof(char), iStrLen, pFile);
				szStr[iStrLen] = 0;

				fread(&uiSubsetCount, sizeof(uint32_t), 1, pFile);
				fread(&uiAllVertexCount, sizeof(uint32_t), 1, pFile);
				fread(&uiAllIndexCount, sizeof(uint32_t), 1, pFile);

				for(int k = 0; k < uiSubsetCount; ++k)
				{
					fread(&iStrLen, sizeof(int32_t), 1, pFile);
					fread(szStr, sizeof(char), iStrLen, pFile);
					szStr[iStrLen] = 0;
				}

				fseek(pFile, sizeof(uint32_t) * uiSubsetCount * 4, SEEK_CUR);
				fseek(pFile, sizeof(vertex_static_ex) * uiAllVertexCount, SEEK_CUR);
				fseek(pFile, sizeof(uint32_t) * uiAllIndexCount, SEEK_CUR);
			}
			//}

			//считываем физическую модель, если есть
			//{
			uint8_t ui8ExistsPhysics = 0;
			fread(&ui8ExistsPhysics, sizeof(uint8_t), 1, pFile);

			if(ui8ExistsPhysics)
			{
				fread(&iStrLen, sizeof(int32_t), 1, pFile);
				fread(szStr, sizeof(char), iStrLen, pFile);

				//загрузка размера массива вершин и вершин
				uint32_t uiSizeArr = 0;
				fread(&uiSizeArr, sizeof(int32_t), 1, pFile);
				fseek(pFile, sizeof(float3_t) * uiSizeArr, SEEK_CUR);

				//загрузка размера массива индексов и индексов
				fread(&uiSizeArr, sizeof(int32_t), 1, pFile);
				fseek(pFile, sizeof(uint32_t) * uiSizeArr, SEEK_CUR);

				//загрузка размеров массива с номерами материалов (для каждого индекса номер из локального массива) и номеров
				fread(&uiSizeArr, sizeof(int32_t), 1, pFile);
				fseek(pFile, sizeof(uint32_t) * uiSizeArr, SEEK_CUR);

				//загрузка массива с именами текстур (для материалов) и создание массива материалов для каждой подгруппы
				Array<ID> aIDs;
				fread(&uiSizeArr, sizeof(int32_t), 1, pFile);
				for(int k = 0; k < uiSizeArr; ++k)
				{
					fread(&iStrLen, sizeof(int32_t), 1, pFile);
					fread(szStr, sizeof(char), iStrLen, pFile);
					szStr[iStrLen] = 0;
				}
			}
			//}

			//считывание идентификаторов пп моделей для текущей модели
			//{
			uint32_t uiCountTransparency = 0;
			fread(&uiCountTransparency, sizeof(uint32_t), 1, pFile);
			if(uiCountTransparency > 0)
			{
				fseek(pFile, sizeof(ID) * uiCountTransparency, SEEK_CUR);
			}
			//}

			//считывание сегментов модели, если есть
			//{
			int8_t i8ExistsSegments = 0;
			fread(&i8ExistsSegments, sizeof(int8_t), 1, pFile);

			if(i8ExistsSegments)
			{
				Array<CSegment**> aQueue;
				int iCountSplits = 0;
				aQueue.push_back(&(pModel->m_pArrSplits));

				while(aQueue.size() > iCountSplits)
				{
					float3 vMin, vMax;
					CSegment **ppSplit = aQueue[iCountSplits];
					(*ppSplit) = new CSegment();

					fread(&vMin.x, sizeof(float), 1, pFile);
					fread(&vMin.y, sizeof(float), 1, pFile);
					fread(&vMin.z, sizeof(float), 1, pFile);

					fread(&vMax.x, sizeof(float), 1, pFile);
					fread(&vMax.y, sizeof(float), 1, pFile);
					fread(&vMax.z, sizeof(float), 1, pFile);

					(*ppSplit)->m_pBoundVolumeSys = SGCore_CrBound();
					(*ppSplit)->m_pBoundVolumeSys->setMinMax(&vMin, &vMax);

					fread(&vMin.x, sizeof(float), 1, pFile);
					fread(&vMin.y, sizeof(float), 1, pFile);
					fread(&vMin.z, sizeof(float), 1, pFile);

					fread(&vMax.x, sizeof(float), 1, pFile);
					fread(&vMax.y, sizeof(float), 1, pFile);
					fread(&vMax.z, sizeof(float), 1, pFile);

					(*ppSplit)->m_pBoundVolumeP = SGCore_CrBound();
					(*ppSplit)->m_pBoundVolumeP->setMinMax(&vMin, &vMax);

					fread(&(*ppSplit)->m_idDepth, sizeof(ID), 1, pFile);
					fread(&(*ppSplit)->m_uiCountAllPoly, sizeof(uint32_t), 1, pFile);
					fread(&(*ppSplit)->m_isFinite, sizeof(int8_t), 1, pFile);

					if(!(*ppSplit)->m_isFinite)
					{
						int8_t iExists = false;

						for(int i = 0; i < GEOM_COUNT_TYPE_SEGMENTATION_OCTO; ++i)
						{
							fread(&iExists, sizeof(int8_t), 1, pFile);
							if(iExists)
								aQueue.push_back(&((*ppSplit)->m_aSplits[i]));

							iExists = false;
						}
					}
					else
					{
						fread(&(*ppSplit)->m_uiCountSubSet, sizeof(uint32_t), 1, pFile);

						(*ppSplit)->m_pNumberGroup = new uint32_t[(*ppSplit)->m_uiCountSubSet];
						(*ppSplit)->m_pCountPoly = new uint32_t[(*ppSplit)->m_uiCountSubSet];

						fread((*ppSplit)->m_pNumberGroup, sizeof(uint32_t)*(*ppSplit)->m_uiCountSubSet, 1, pFile);
						fread((*ppSplit)->m_pCountPoly, sizeof(uint32_t)*(*ppSplit)->m_uiCountSubSet, 1, pFile);

						(*ppSplit)->m_ppArrPoly = new uint32_t*[(*ppSplit)->m_uiCountSubSet];

						for(int k = 0; k < (*ppSplit)->m_uiCountSubSet; ++k)
						{
							(*ppSplit)->m_ppArrPoly[k] = new uint32_t[(*ppSplit)->m_pCountPoly[k] * 3];
							fread((*ppSplit)->m_ppArrPoly[k], sizeof(uint32_t)*(*ppSplit)->m_pCountPoly[k] * 3, 1, pFile);
						}
					}

					++iCountSplits;
				}

				if(pModel->m_pArrSplits)
				{
					setSplitID(pModel->m_pArrSplits, &(pModel->m_idCountSplits), &(pModel->m_idCountSplitsRender));

					createExternalData4SegmentModel(pModel);
				}
			}
			//}
#endif
		}

#if 0
		uint32_t uiCountTransparency = 0;
		fread(&uiCountTransparency, sizeof(uint32_t), 1, pFile);

		for(int i = 0; i < uiCountTransparency; ++i)
		{
			uint32_t uiSizeBlock = 0;
			fread(&uiSizeBlock, sizeof(uint32_t), 1, pFile);

			if(uiSizeBlock >= uiSizeFile)
			{
				LibReport(REPORT_MSG_LEVEL_ERROR, "file static geometry [%s] damaged, block size (transparency) %d\n", szPath);
				fclose(pFile);
				return;
			}

			CTransparencyModel *pTransparency = new CTransparencyModel();
			fread(&(pTransparency->m_idModel), sizeof(ID), 1, pFile);
			int32_t iStrLen = 0;
			char szStr[1024];
			fread(&iStrLen, sizeof(int32_t), 1, pFile);
			fread(szStr, sizeof(char), iStrLen, pFile);
			szStr[iStrLen] = 0;
			pTransparency->m_sTex = szStr;
			pTransparency->m_sTex += ".dds";

			XSHADER_DEFAULT_DESC shaderDesc;
			shaderDesc.szFileVS = "mtrlgeom_base.vs";
			shaderDesc.szFilePS = "mtrlgeom_base.ps";
			g_pRenderable->getMaterialSystem()->loadMaterial(pTransparency->m_sTex.c_str(), &pTransparency->m_idTex, &shaderDesc);


			fread(&(pTransparency->m_iCountVertex), sizeof(int32_t), 1, pFile);
			fread(&(pTransparency->m_iCountIndex), sizeof(int32_t), 1, pFile);

			pTransparency->m_pVertices = new vertex_static_ex[pTransparency->m_iCountVertex];
			pTransparency->m_pIndices = new UINT[pTransparency->m_iCountIndex];
			//g_pDXDevice->CreateVertexBuffer(sizeof(vertex_static_ex)* pTransparency->m_iCountVertex, NULL, NULL, D3DPOOL_MANAGED, &(pTransparency->m_pVertexBuffer), 0);
			//g_pDXDevice->CreateIndexBuffer(sizeof(UINT)* pTransparency->m_iCountIndex, NULL, D3DFMT_INDEX32, D3DPOOL_MANAGED, &(pTransparency->m_pIndexBuffer), 0);

			UINT *pIndexTransparency = pTransparency->m_pIndices;
			//pTransparency->m_pIndexBuffer->Lock(0, 0, (void **)&pIndexTransparency, 0);
			vertex_static_ex *pVertexTransparency = pTransparency->m_pVertices;
			//pTransparency->m_pVertexBuffer->Lock(0, 0, (void **)&pVertexTransparency, 0);

			fread(pVertexTransparency, sizeof(vertex_static_ex), pTransparency->m_iCountVertex, pFile);
			fread(pIndexTransparency, sizeof(uint32_t), pTransparency->m_iCountIndex, pFile);

			pTransparency->syncBuffers();

			//pTransparency->m_pVertexBuffer->Unlock();
			//pTransparency->m_pIndexBuffer->Unlock();

			CModel *pModel = m_aModels[pTransparency->m_idModel];
			pTransparency->m_pBoundVolume->setPosition(pModel->m_pBoundVolume->getPosition());
			pTransparency->m_pBoundVolume->setRotation(pModel->m_pBoundVolume->getRotation());
			pTransparency->m_pBoundVolume->setScale(pModel->m_pBoundVolume->getScale());
			pTransparency->m_pBoundVolume->calcBound(pTransparency->m_pVertices, pTransparency->m_iCountVertex, sizeof(vertex_static_ex));

			modelGenGroupInfo4Transparency(pTransparency);

			m_aTransparency.push_back(pTransparency);
		}
#endif
		fclose(pFile);
	}

protected:
	IXCore *m_pCore;
	ID m_idPlugin;
};

class CStaticGeomPlugin: public IXUnknownImplementation<IXPlugin>
{
public:
	~CStaticGeomPlugin()
	{
		mem_delete(m_pEventListener);
	}
	
	void XMETHODCALLTYPE startup(IXCore *pCore) override
	{
		m_pCore = pCore;
		m_pEventListener = new CLevelLoadListener(pCore, getID());

		m_pCore->getEventChannel<XEventLevel>(EVENT_LEVEL_GUID)->addListener(m_pEventListener);
	}

	void XMETHODCALLTYPE shutdown() override
	{
		m_pCore->getEventChannel<XEventLevel>(EVENT_LEVEL_GUID)->removeListener(m_pEventListener);
		mem_delete(m_pEventListener);
	}

	UINT XMETHODCALLTYPE getInterfaceCount() override
	{
		return(0);
	}
	const XGUID* XMETHODCALLTYPE getInterfaceGUID(UINT id) override
	{
		static XGUID s_guid;
		switch(id)
		{
		default:
			return(NULL);
		}
		return(&s_guid);
	}
	void XMETHODCALLTYPE getInterface(UINT id, void **ppOut) override
	{
		*ppOut = NULL;
	}

protected:
	IXCore *m_pCore = NULL;
	CLevelLoadListener *m_pEventListener = NULL;
};

DECLARE_XPLUGIN(CStaticGeomPlugin);
