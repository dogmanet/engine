#include "ModelLoader.h"
#include "ModelFile.h"

CModelLoader::CModelLoader(IFileSystem *pFileSystem):
m_pFileSystem(pFileSystem)
{
}

UINT XMETHODCALLTYPE CModelLoader::getExtCount() const
{
	return(1);
}
const char* XMETHODCALLTYPE CModelLoader::getExt(UINT uIndex) const
{
	assert(uIndex < getExtCount());
	switch(uIndex)
	{
	case 0:
		return("dse");
	}
	return(NULL);
}
const char* XMETHODCALLTYPE CModelLoader::getExtText(UINT uIndex) const
{
	assert(uIndex < getExtCount());
	switch(uIndex)
	{
	case 0:
		return("SkyXEngine model");
	}
	return(NULL);
}
const char* XMETHODCALLTYPE CModelLoader::getAuthor() const
{
	return("D-AIRY @ DogmaNet");
}
const char* XMETHODCALLTYPE CModelLoader::getCopyright() const
{
	return("Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2019");
}
const char* XMETHODCALLTYPE CModelLoader::getDescription() const
{
	return("DSE model loader");
}

void XMETHODCALLTYPE CModelLoader::getInfo(XModelInfo *pModelInfo)
{
	pModelInfo->type = getType();
	pModelInfo->uSkinCount = m_hdr.iSkinCount;
	pModelInfo->uSubsetsCount = m_hdr.iMaterialCount;
	pModelInfo->uBoneCount = m_hdr2.iBoneTableCount;
	pModelInfo->uAnimationCount = m_hdr.iAnimationCount;
	pModelInfo->uHitboxCount = m_hdr2.iHitboxCount;
	pModelInfo->uLodCount = m_hdr.iLODcount;

	pModelInfo->uVertexCount = 0;
	pModelInfo->uIndexCount = 0;

	if(m_hdr.iLODcount && m_hdr.iLODoffset)
	{
		m_pCurrentFile->setPos((size_t)m_hdr.iLODoffset);

		ModelLoD lod;
		if(m_hdr.iLODcount)
		{
			m_pCurrentFile->readBin(&lod, MODEL_LOD_STRUCT_SIZE);
			float3 vMin, vMax;
			float3 vPos;

			for(UINT j = 0; j < lod.iSubMeshCount; j++)
			{
				uint32_t uMaterialID, uVertexCount, uIndexCount;
				m_pCurrentFile->readBin(&uMaterialID, sizeof(uint32_t));
				m_pCurrentFile->readBin(&uVertexCount, sizeof(uint32_t));
				m_pCurrentFile->readBin(&uIndexCount, sizeof(uint32_t));

				pModelInfo->uVertexCount += uVertexCount;
				pModelInfo->uIndexCount += uIndexCount;

				for(UINT i = 0; i < uVertexCount; ++i)
				{
					if(m_hdr.iFlags & MODEL_FLAG_STATIC)
					{
						if(m_hdr.iFlags & MODEL_FLAG_HAS_TANGENT_BINORM)
						{
							vertex_static_ex v;
							m_pCurrentFile->readBin(&v, sizeof(v));
							vPos = v.Pos;
						}
						else
						{
							vertex_static v;
							m_pCurrentFile->readBin(&v, sizeof(v));
							vPos = v.Pos;
						}
					}
					else
					{
						if(m_hdr.iFlags & MODEL_FLAG_HAS_TANGENT_BINORM)
						{
							vertex_animated_ex v;
							m_pCurrentFile->readBin(&v, sizeof(v));
							vPos = v.Pos;
						}
						else
						{
							vertex_animated v;
							m_pCurrentFile->readBin(&v, sizeof(v));
							vPos = v.Pos;
						}
					}
					if(j == 0 && i == 0)
					{
						vMin = vPos;
						vMax = vPos;
					}
					else
					{
						vMin = SMVectorMin(vMin, vPos);
						vMax = SMVectorMax(vMax, vPos);
					}
				}
			}

			pModelInfo->vDimensions = float3_t(vMax - vMin);
			pModelInfo->vCenter = float3_t((vMax + vMin) * 0.5f);
		}
	}
}

bool XMETHODCALLTYPE CModelLoader::open(const char *szFileName)
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

	memset(&m_hdr, 0, sizeof(m_hdr));
	m_pCurrentFile->readBin(&m_hdr, sizeof(m_hdr));

	if(m_hdr.Magick != SX_MODEL_MAGICK)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Invalid DSE magick\n");
		mem_release(m_pCurrentFile);
		return(false);
	}

	if(m_hdr.iVersion != SX_MODEL_VERSION)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Unsupported DSE version %d\n", m_hdr.iVersion);
		mem_release(m_pCurrentFile);
		return(false);
	}

	if(m_hdr.iSecondHeaderOffset)
	{
		m_pCurrentFile->setPos((size_t)m_hdr.iSecondHeaderOffset);
		m_pCurrentFile->readBin(&m_hdr2, sizeof(m_hdr2));
	}
	else
	{
		m_hdr2 = {};
	}

	if(m_hdr2.iThirdHeaderOffset)
	{
		m_pCurrentFile->setPos((size_t)m_hdr2.iThirdHeaderOffset);
		m_pCurrentFile->readBin(&m_hdr3, sizeof(m_hdr3));
	}
	else
	{
		m_hdr3 = {};
	}

	return(true);
}
XMODELTYPE XMETHODCALLTYPE CModelLoader::getType() const
{
	return((m_hdr.iFlags & MODEL_FLAG_STATIC) ? XMT_STATIC : XMT_ANIMATED);
}
bool XMETHODCALLTYPE CModelLoader::loadAsStatic(IXResourceModelStatic *pResource)
{
	if(!loadGeneric(pResource))
	{
		return(false);
	}

	switch(m_hdr2.topology)
	{
	case MDLPT_TRIANGLESTRIP:
		pResource->setPrimitiveTopology(XPT_TRIANGLESTRIP);
		break;
	case MDLPT_TRIANGLELIST:
	default:
		pResource->setPrimitiveTopology(XPT_TRIANGLELIST);
	}

	if(m_hdr.iLODcount && m_hdr.iLODoffset)
	{
		m_pCurrentFile->setPos((size_t)m_hdr.iLODoffset);

		for(uint32_t i = 0; i < m_hdr.iLODcount; i++)
		{
			ModelLoD lod;
			m_pCurrentFile->readBin(&lod, MODEL_LOD_STRUCT_SIZE);

			size_t uStartPos = m_pCurrentFile->getPos();

			UINT *puVC = (UINT*)alloca(sizeof(UINT) * lod.iSubMeshCount);
			UINT *puIC = (UINT*)alloca(sizeof(UINT) * lod.iSubMeshCount);
			for(uint32_t j = 0; j < lod.iSubMeshCount; j++)
			{
				uint32_t uMaterialID, uVertexCount, uIndexCount;
				m_pCurrentFile->readBin(&uMaterialID, sizeof(uint32_t));
				m_pCurrentFile->readBin(&uVertexCount, sizeof(uint32_t));
				m_pCurrentFile->readBin(&uIndexCount, sizeof(uint32_t));

				puVC[j] = uVertexCount;
				puIC[j] = uIndexCount;

				size_t uSkipSize = 0;

				if(m_hdr.iFlags & MODEL_FLAG_STATIC)
				{
					if(m_hdr.iFlags & MODEL_FLAG_HAS_TANGENT_BINORM)
					{
						uSkipSize += sizeof(vertex_static_ex) * uVertexCount;
					}
					else
					{
						uSkipSize += sizeof(vertex_static) * uVertexCount;
					}
				}
				else
				{
					if(m_hdr.iFlags & MODEL_FLAG_HAS_TANGENT_BINORM)
					{
						uSkipSize += sizeof(vertex_animated_ex) * uVertexCount;
					}
					else
					{
						uSkipSize += sizeof(vertex_animated) * uVertexCount;
					}
				}
				uSkipSize += sizeof(UINT) * uIndexCount;

				m_pCurrentFile->setPos(m_pCurrentFile->getPos() + uSkipSize);
			}

			UINT uLod = pResource->addLod(lod.iSubMeshCount, puVC, puIC);
			m_pCurrentFile->setPos(uStartPos);

			for(uint32_t j = 0; j < lod.iSubMeshCount; j++)
			{
				XResourceModelStaticSubset *pSubSet = pResource->getSubset(uLod, j);

				uint32_t uMaterialID, uVertexCount, uIndexCount;
				m_pCurrentFile->readBin(&uMaterialID, sizeof(uint32_t));
				m_pCurrentFile->readBin(&uVertexCount, sizeof(uint32_t));
				m_pCurrentFile->readBin(&uIndexCount, sizeof(uint32_t));

				pSubSet->iMaterialID = uMaterialID;

				if(m_hdr.iFlags & MODEL_FLAG_STATIC)
				{
					if(m_hdr.iFlags & MODEL_FLAG_HAS_TANGENT_BINORM)
					{
						static_assert(sizeof(vertex_static_ex) == sizeof(XResourceModelStaticVertex), "vertex_static_ex must be equal to XResourceModelStaticVertex");
						m_pCurrentFile->readBin(pSubSet->pVertices, sizeof(vertex_static_ex) * pSubSet->iVertexCount);
					}
					else
					{
						vertex_static vs;
						for(UINT k = 0; k < pSubSet->iVertexCount; ++k)
						{
							m_pCurrentFile->readBin(&vs, sizeof(vertex_static));
							pSubSet->pVertices[k].vPos = vs.Pos;
							pSubSet->pVertices[k].vNorm = vs.Norm;
							pSubSet->pVertices[k].vTex = vs.Tex;
							pSubSet->pVertices[k].vBinorm = float3_t(0, 0, 0);
							pSubSet->pVertices[k].vTangent = float3_t(0, 0, 0);
						}
					}
				}
				else
				{
					if(m_hdr.iFlags & MODEL_FLAG_HAS_TANGENT_BINORM)
					{
						vertex_animated_ex vs;
						for(UINT k = 0; k < pSubSet->iVertexCount; ++k)
						{
							m_pCurrentFile->readBin(&vs, sizeof(vertex_animated_ex));
							pSubSet->pVertices[k].vPos = vs.Pos;
							pSubSet->pVertices[k].vNorm = vs.Norm;
							pSubSet->pVertices[k].vTex = vs.Tex;
							pSubSet->pVertices[k].vBinorm = vs.Binorm;
							pSubSet->pVertices[k].vTangent = vs.Tangent;
						}
					}
					else
					{
						vertex_animated vs;
						for(UINT k = 0; k < pSubSet->iVertexCount; ++k)
						{
							m_pCurrentFile->readBin(&vs, sizeof(vertex_animated));
							pSubSet->pVertices[k].vPos = vs.Pos;
							pSubSet->pVertices[k].vNorm = vs.Norm;
							pSubSet->pVertices[k].vTex = vs.Tex;
							pSubSet->pVertices[k].vBinorm = float3_t(0, 0, 0);
							pSubSet->pVertices[k].vTangent = float3_t(0, 0, 0);
						}
					}
				}
				m_pCurrentFile->readBin(pSubSet->pIndices, sizeof(UINT) * pSubSet->iIndexCount);

				if(!(m_hdr.iFlags & MODEL_FLAG_NORMALIZED_NORMALS))
				{
					for(UINT k = 0; k < pSubSet->iVertexCount; ++k)
					{
						pSubSet->pVertices[k].vNorm = SMVector3Normalize(pSubSet->pVertices[k].vNorm);
					}
				}
			}
		}
	}

	return(true);
}
bool XMETHODCALLTYPE CModelLoader::loadAsAnimated(IXResourceModelAnimated *pResource)
{
	if(getType() != XMT_ANIMATED)
	{
		return(false);
	}

	if(!loadGeneric(pResource))
	{
		return(false);
	}

	if(m_hdr2.iDepsCount && m_hdr2.iDependensiesOffset)
	{
		m_pCurrentFile->setPos((size_t)m_hdr2.iDependensiesOffset);

		if(m_hdr.iFlags & MODEL_FLAG_NEW_STYLE_DEPS)
		{
			ModelPart *pParts = (ModelPart*)alloca(sizeof(ModelPart) * m_hdr2.iDepsCount);
			m_pCurrentFile->readBin(pParts, sizeof(ModelPart) * m_hdr2.iDepsCount);

			for(uint32_t i = 0; i < m_hdr2.iDepsCount; ++i)
			{
				//@TODO: Separate addPartName and addImportName
				pResource->addPartName(pParts[i].file, pParts[i].name, (XMODEL_IMPORT)pParts[i].uImportFlags, (XMODEL_PART_FLAGS)pParts[i].uFlags);
				// pResource->addImportName(pParts[i].file, (XMODEL_IMPORT)pParts[i].uImportFlags);
			}
		}
		else
		{
			ModelDependensy md;
			for(uint32_t i = 0; i < m_hdr2.iDepsCount; ++i)
			{
				m_pCurrentFile->readBin(&md, sizeof(ModelDependensy));
				pResource->addImportName(md.szName, XMI_ALL);
			}
		}
	}

	if(m_hdr.iLODcount && m_hdr.iLODoffset)
	{
		m_pCurrentFile->setPos((size_t)m_hdr.iLODoffset);

		for(uint32_t i = 0; i < m_hdr.iLODcount; i++)
		{
			ModelLoD lod;
			m_pCurrentFile->readBin(&lod, MODEL_LOD_STRUCT_SIZE);

			size_t uStartPos = m_pCurrentFile->getPos();

			UINT *puVC = (UINT*)alloca(sizeof(UINT) * lod.iSubMeshCount);
			UINT *puIC = (UINT*)alloca(sizeof(UINT) * lod.iSubMeshCount);
			for(uint32_t j = 0; j < lod.iSubMeshCount; j++)
			{
				uint32_t uMaterialID, uVertexCount, uIndexCount;
				m_pCurrentFile->readBin(&uMaterialID, sizeof(uint32_t));
				m_pCurrentFile->readBin(&uVertexCount, sizeof(uint32_t));
				m_pCurrentFile->readBin(&uIndexCount, sizeof(uint32_t));

				puVC[j] = uVertexCount;
				puIC[j] = uIndexCount;

				size_t uSkipSize = 0;

				if(m_hdr.iFlags & MODEL_FLAG_STATIC)
				{
					if(m_hdr.iFlags & MODEL_FLAG_HAS_TANGENT_BINORM)
					{
						uSkipSize += sizeof(vertex_static_ex) * uVertexCount;
					}
					else
					{
						uSkipSize += sizeof(vertex_static) * uVertexCount;
					}
				}
				else
				{
					if(m_hdr.iFlags & MODEL_FLAG_HAS_TANGENT_BINORM)
					{
						uSkipSize += sizeof(vertex_animated_ex) * uVertexCount;
					}
					else
					{
						uSkipSize += sizeof(vertex_animated) * uVertexCount;
					}
				}
				uSkipSize += sizeof(UINT) * uIndexCount;

				m_pCurrentFile->setPos(m_pCurrentFile->getPos() + uSkipSize);
			}

			UINT uLod = pResource->addLod(lod.iSubMeshCount, puVC, puIC);
			m_pCurrentFile->setPos(uStartPos);

			for(uint32_t j = 0; j < lod.iSubMeshCount; j++)
			{
				XResourceModelAnimatedSubset *pSubSet = pResource->getSubset(uLod, j);

				uint32_t uMaterialID, uVertexCount, uIndexCount;
				m_pCurrentFile->readBin(&uMaterialID, sizeof(uint32_t));
				m_pCurrentFile->readBin(&uVertexCount, sizeof(uint32_t));
				m_pCurrentFile->readBin(&uIndexCount, sizeof(uint32_t));

				pSubSet->iMaterialID = uMaterialID;

				if(!(m_hdr.iFlags & MODEL_FLAG_STATIC))
				{
					if(m_hdr.iFlags & MODEL_FLAG_HAS_TANGENT_BINORM)
					{
						m_pCurrentFile->readBin(pSubSet->pVertices, sizeof(vertex_animated_ex) * pSubSet->iVertexCount);
					}
					else
					{
						vertex_animated vs;
						for(UINT k = 0; k < pSubSet->iVertexCount; ++k)
						{
							m_pCurrentFile->readBin(&vs, sizeof(vertex_animated));
							pSubSet->pVertices[k].vPos = vs.Pos;
							pSubSet->pVertices[k].vNorm = vs.Norm;
							pSubSet->pVertices[k].vTex = vs.Tex;
							pSubSet->pVertices[k].vBoneWeights = vs.BoneWeights;
							pSubSet->pVertices[k].u8BoneIndices[0] = vs.BoneIndices[0];
							pSubSet->pVertices[k].u8BoneIndices[1] = vs.BoneIndices[1];
							pSubSet->pVertices[k].u8BoneIndices[2] = vs.BoneIndices[2];
							pSubSet->pVertices[k].u8BoneIndices[3] = vs.BoneIndices[3];
							pSubSet->pVertices[k].vBinorm = float3_t(0, 0, 0);
							pSubSet->pVertices[k].vTangent = float3_t(0, 0, 0);
						}
					}
				}
				m_pCurrentFile->readBin(pSubSet->pIndices, sizeof(UINT) * pSubSet->iIndexCount);

				if(!(m_hdr.iFlags & MODEL_FLAG_NORMALIZED_NORMALS))
				{
					for(UINT k = 0; k < pSubSet->iVertexCount; ++k)
					{
						pSubSet->pVertices[k].vNorm = SMVector3Normalize(pSubSet->pVertices[k].vNorm);
					}
				}
				//for(UINT k = 0; k < pSubSet->iVertexCount; ++k)
				//{
				//	pSubSet->pVertices[k].vPos = /*(float3)(*/pSubSet->pVertices[k].vPos/* * 0.0254f)*/;
				//}
			}
		}
	}

	if(m_hdr2.iActivitiesTableCount && m_hdr2.iActivitiesTableOffset)
	{
		m_pCurrentFile->setPos((size_t)m_hdr2.iActivitiesTableOffset);

		ModelActivity ma;
		for(UINT i = 0; i < m_hdr2.iActivitiesTableCount; ++i)
		{
			m_pCurrentFile->readBin(&ma, sizeof(ModelActivity));
			pResource->addActivity(ma.szName);
		}
	}

	if(m_hdr2.iBoneTableOffset)
	{
		m_pCurrentFile->setPos((size_t)m_hdr2.iBoneTableOffset);
		
		pResource->setBoneCount(m_hdr2.iBoneTableCount);

		ModelBoneName mbn;
		for(UINT i = 0; i < m_hdr2.iBoneTableCount; ++i)
		{
			m_pCurrentFile->readBin(&mbn, sizeof(ModelBoneName));
			assert(i == mbn.bone.id);
			pResource->setBoneInfo(i, mbn.szName, mbn.bone.pid, mbn.bone.position/* * 0.0254f*/, mbn.bone.orient);
		}
	}

	if(m_hdr2.iControllersCount && m_hdr2.iControllersOffset)
	{
		m_pCurrentFile->setPos((size_t)m_hdr2.iControllersOffset);

		pResource->setControllersCount(m_hdr2.iControllersCount);

		ModelBoneController mbc;
		for(UINT i = 0; i < m_hdr2.iControllersCount; ++i)
		{
			m_pCurrentFile->readBin(&mbc, sizeof(ModelBoneController));
			pResource->setControllerBoneCount(i, mbc.iBoneCount);

			XResourceModelController *pCtl = pResource->getController(i);
			for(UINT j = 0; j < mbc.iBoneCount; ++j)
			{
				pCtl->pBones[j] = mbc.bones[j];
			}
			
			pCtl->qMinRot = SMQuaternion(SMToRadian(mbc.fMinRot.x), 'x') * SMQuaternion(SMToRadian(mbc.fMinRot.y), 'y') * SMQuaternion(SMToRadian(mbc.fMinRot.z), 'z');
			pCtl->qMaxRot = SMQuaternion(SMToRadian(mbc.fMaxRot.x), 'x') * SMQuaternion(SMToRadian(mbc.fMaxRot.y), 'y') * SMQuaternion(SMToRadian(mbc.fMaxRot.z), 'z');
			pCtl->vMinTrans = mbc.fMinTrans;
			pCtl->vMaxTrans = mbc.fMaxTrans;

			assert(sizeof(pCtl->szName) == sizeof(mbc.szName));
			memcpy(pCtl->szName, mbc.szName, MODEL_MAX_NAME);

		}
	}

	if(m_hdr.iAnimationsOffset)
	{
		m_pCurrentFile->setPos((size_t)m_hdr.iAnimationsOffset);

		pResource->setSequenceCount(m_hdr.iAnimationCount);

		ModelSequence ms;
		for(UINT i = 0; i < m_hdr.iAnimationCount; ++i)
		{
			if(m_hdr.iFlags & MODEL_FLAG_NEW_PACKED_ANIMS)
			{
				m_pCurrentFile->readBin(&ms, MODEL_SEQUENCE_STRUCT_SIZE);
			}
			else
			{
				m_pCurrentFile->readBin(&ms, MODEL_SEQUENCE_STRUCT_SIZE_OLD);
			}

			pResource->setSequenceFrameCount(i, ms.iNumFrames);

			XResourceModelSequence *pSequence = pResource->getSequence(i);
			
			assert(sizeof(pSequence->szName) == sizeof(ms.name));
			memcpy(pSequence->szName, ms.name, MODEL_MAX_NAME);

			pSequence->iActivity = (int)ms.activity - 1;
			pSequence->uActivityChance = ms.act_chance;
			pSequence->isLooped = !!ms.bLooped;
			pSequence->iFramerate = ms.framerate;

			ModelBone mb;
			for(UINT j = 0; j < ms.iNumFrames; ++j)
			{
				for(UINT k = 0; k < m_hdr2.iBoneTableCount; ++k)
				{
					m_pCurrentFile->readBin(&mb, sizeof(ModelBone));

					pSequence->m_ppSequenceData[j][k].orient = mb.orient;
					pSequence->m_ppSequenceData[j][k].position = /*(float3)(*/mb.position/* * 0.0254f)*/;
				}
			}
		}
	}

#if 0
	if(m_hdr2.iHitboxCount && m_hdr2.iHitboxesOffset && false)
	{
		m_pCurrentFile->setPos((size_t)m_hdr2.iHitboxesOffset);

		pResource->setHitboxCount(m_hdr2.iHitboxCount);

		ModelHitbox mh;
		for(UINT i = 0; i < m_hdr2.iHitboxCount; ++i)
		{
			m_pCurrentFile->readBin(&mh, sizeof(ModelHitbox));
			XResourceModelHitbox *pHitbox = pResource->getHitbox(i);

			//assert(!"Check bone_id");
			pHitbox->bone_id = mh.bone_id;
			pHitbox->lwh = mh.lwh;
			pHitbox->part = (XHITBOXBODYPART)mh.part;
			pHitbox->pos = mh.pos;
			pHitbox->rot = SMQuaternion(mh.rot.x, 'x') * SMQuaternion(mh.rot.y, 'y') * SMQuaternion(mh.rot.z, 'z');

			switch(mh.type)
			{
			case HT_BOX: pHitbox->type = XHT_BOX; break;
			case HT_CYLINDER: pHitbox->type = XHT_CYLINDER; break;
			case HT_CAPSULE: pHitbox->type = XHT_CAPSULE; break;
			case HT_ELIPSOID: pHitbox->type = XHT_SPHERE; break;
			//default:
				//LibReport(REPORT_MSG_LEVEL_WARNING, "Unsupported hitbox type");
			}

			assert(sizeof(pHitbox->szName) == sizeof(mh.name));
			memcpy(pHitbox->szName, mh.name, MODEL_MAX_NAME);
		}
	}
#endif

	if(m_hdr3.uChunksCount && m_hdr3.uChunksOffset)
	{
		m_pCurrentFile->setPos((size_t)m_hdr3.uChunksOffset);
		ModelChunkHeader chunk;
		for(UINT uChunk = 0; uChunk < m_hdr3.uChunksCount; ++uChunk)
		{
			if(m_pCurrentFile->readBin(&chunk, sizeof(chunk)) == sizeof(chunk))
			{
				if(chunk.guidType == DSE_CHUNK_HITBOXES_EX_GUID)
				{
					ModelHitboxExChunk chunkData;
					if(m_pCurrentFile->readBin(&chunkData, sizeof(chunkData)) == sizeof(chunkData) && chunkData.uHitboxExCount)
					{
						pResource->setHitboxCount(chunkData.uHitboxExCount);

						ModelHitboxEx *pHitboxes = (ModelHitboxEx*)alloca(sizeof(ModelHitboxEx) * chunkData.uHitboxExCount);
						memset(pHitboxes, 0, sizeof(ModelHitboxEx) *chunkData.uHitboxExCount);
						for(UINT i = 0; i < chunkData.uHitboxExCount; ++i)
						{
							m_pCurrentFile->readBin(&pHitboxes[i], MODEL_HITBOXEX_STRUCT_SIZE);
						}

						for(UINT i = 0; i < chunkData.uHitboxExCount; ++i)
						{
							IModelPhysbox *pPhysbox = NULL;
							switch(pHitboxes[i].hitbox.type)
							{
							case HT_BOX:
								{
									IModelPhysboxBox *pBox = pResource->newPhysboxBox();
									pBox->setSize(pHitboxes[i].hitbox.lwh);
									pPhysbox = pBox;
								}
								break;
							case HT_ELIPSOID:
								{
									IModelPhysboxSphere *pSphere = pResource->newPhysboxSphere();
									pSphere->setRadius(pHitboxes[i].hitbox.lwh.x);
									pPhysbox = pSphere;
								}
								break;
							case HT_CONVEX:
								{
									IModelPhysboxConvex *pConvex = pResource->newPhysboxConvex();

									m_pCurrentFile->setPos((size_t)pHitboxes[i].hitbox.iDataOffset);
									ModelPhyspartDataConvex *pData = (ModelPhyspartDataConvex *)alloca(sizeof(ModelPhyspartDataConvex));
									m_pCurrentFile->readBin(pData, MODEL_PHYSPART_DATA_CONVEX_STRUCT_SIZE);

									pConvex->initData(pData->iVertCount);
									m_pCurrentFile->readBin(pConvex->getData(), sizeof(float3_t) * pData->iVertCount);

									pPhysbox = pConvex;
								}
								break;
								//default:
									//LibReport(REPORT_MSG_LEVEL_WARNING, "Unknown physbox type");
							}

							if(pPhysbox)
							{
								pPhysbox->setPosition(pHitboxes[i].hitbox.pos);
								pPhysbox->setOrientation(SMQuaternion(pHitboxes[i].hitbox.rot));
								pResource->setHitbox(i, pPhysbox, pHitboxes[i].hitbox.bone_id, pHitboxes[i].hitbox.name, (XHITBOXBODYPART)pHitboxes[i].part);
								mem_release(pPhysbox);
							}
						}
					}
				}
			}
		}
	}

	return(true);
}

void XMETHODCALLTYPE CModelLoader::close()
{
	mem_release(m_pCurrentFile);
}

bool CModelLoader::loadGeneric(IXResourceModel *pResource)
{
	if(m_hdr2.iPhysicsDataOffset)
	{
		m_pCurrentFile->setPos((size_t)m_hdr2.iPhysicsDataOffset);

		ModelPhysData physData;
		m_pCurrentFile->readBin(&physData, MODEL_PHYSDATA_STRUCT_SIZE);
		pResource->setMass(physData.fWeight);

		m_pCurrentFile->setPos((size_t)physData.iPartsOffset);

		ModelPhyspart *pPhysparts = (ModelPhyspart*)_malloca(sizeof(ModelPhyspart) * physData.iPhyspartCount);
		memset(pPhysparts, 0, sizeof(ModelPhyspart) * physData.iPhyspartCount);
		for(UINT i = 0; i < physData.iPhyspartCount; ++i)
		{
			m_pCurrentFile->readBin(&pPhysparts[i], MODEL_PHYSPART_STRUCT_SIZE);
		}

		for(UINT i = 0; i < physData.iPhyspartCount; ++i)
		{
			//@TODO: Attach to bones
			IModelPhysbox *pPhysbox = NULL;
			switch(pPhysparts[i].type)
			{
			case HT_BOX:
				{
					IModelPhysboxBox *pBox = pResource->newPhysboxBox();
					pBox->setSize(pPhysparts[i].lwh);
					pPhysbox = pBox;
				}
				break;
			case HT_ELIPSOID:
				{
					IModelPhysboxSphere *pSphere = pResource->newPhysboxSphere();
					pSphere->setRadius(pPhysparts[i].lwh.x);
					pPhysbox = pSphere;
				}
				break;
			case HT_CONVEX:
				{
					IModelPhysboxConvex *pConvex = pResource->newPhysboxConvex();

					m_pCurrentFile->setPos((size_t)pPhysparts[i].iDataOffset);
					ModelPhyspartDataConvex *pData = (ModelPhyspartDataConvex*)alloca(sizeof(ModelPhyspartDataConvex));
					m_pCurrentFile->readBin(pData, MODEL_PHYSPART_DATA_CONVEX_STRUCT_SIZE);

					pConvex->initData(pData->iVertCount);
					m_pCurrentFile->readBin(pConvex->getData(), sizeof(float3_t) * pData->iVertCount);

					pPhysbox = pConvex;
				}
				break;
			//default:
				//LibReport(REPORT_MSG_LEVEL_WARNING, "Unknown physbox type");
			}

			if(pPhysbox)
			{
				pPhysbox->setPosition(pPhysparts[i].pos);
				pPhysbox->setOrientation(SMQuaternion(pPhysparts[i].rot));
				pResource->addPhysbox(pPhysbox);
				mem_release(pPhysbox);
			}
		}

		_freea(pPhysparts);
	}
	
	if(m_hdr.iMaterialsOffset)
	{
		m_pCurrentFile->setPos((size_t)m_hdr.iMaterialsOffset);

		pResource->setMaterialCount(m_hdr.iMaterialCount, m_hdr.iSkinCount);
		char szName[MODEL_MAX_NAME];

		for(UINT j = 0; j < m_hdr.iSkinCount; j++)
		{
			for(uint32_t i = 0; i < m_hdr.iMaterialCount; i++)
			{
				m_pCurrentFile->readBin(szName, MODEL_MAX_NAME);
				pResource->setMaterial(i, j, szName);
			}
		}
	}

	//addGibName
	return(true);
}
