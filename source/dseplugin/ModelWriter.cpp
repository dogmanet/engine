#include "ModelWriter.h"
#include "ModelFile.h"

UINT XMETHODCALLTYPE CModelWriter::getExtCount() const
{
	return(1);
}
const char* XMETHODCALLTYPE CModelWriter::getExt(UINT uIndex) const
{
	assert(uIndex < getExtCount());
	switch(uIndex)
	{
	case 0:
		return("dse");
	}
	return(NULL);
}
const char* XMETHODCALLTYPE CModelWriter::getExtText(UINT uIndex) const
{
	assert(uIndex < getExtCount());
	switch(uIndex)
	{
	case 0:
		return("SkyXEngine model");
	}
	return(NULL);
}
const char* XMETHODCALLTYPE CModelWriter::getAuthor() const
{
	return("D-AIRY @ DogmaNet");
}
const char* XMETHODCALLTYPE CModelWriter::getCopyright() const
{
	return("Copyright © DogmaNet, 2021");
}
const char* XMETHODCALLTYPE CModelWriter::getDescription() const
{
	return("DSE model writer");
}

bool XMETHODCALLTYPE CModelWriter::writeModel(IXResourceModel *pResource, IFile *pFile)
{
	memset(&m_hdr, 0, sizeof(m_hdr));
	memset(&m_hdr2, 0, sizeof(m_hdr2));

	m_hdr.Magick = SX_MODEL_MAGICK;
	m_hdr.iVersion = SX_MODEL_VERSION;
	m_hdr.iFlags = MODEL_FLAG_COMPILED | MODEL_FLAG_HAS_TANGENT_BINORM | MODEL_FLAG_NORMALIZED_NORMALS;

	if(pResource->getType() == XMT_STATIC)
	{
		m_hdr.iFlags |= MODEL_FLAG_STATIC;
		m_bWantHdr2 = pResource->asStatic()->getPrimitiveTopology() != XPT_TRIANGLELIST || pResource->getPhysboxCount() != 0;
	}
	else
	{
		m_hdr.iFlags |= MODEL_FLAG_ANIMATED;
		m_bWantHdr2 = true;
	}

	pFile->setPos(0);
	pFile->writeBin(&m_hdr, sizeof(m_hdr));

	if(m_bWantHdr2)
	{
		m_hdr.iSecondHeaderOffset = pFile->getPos();
		pFile->writeBin(&m_hdr2, sizeof(m_hdr2));
	}

	{
		m_hdr.iMaterialsOffset = pFile->getPos();
		m_hdr.iMaterialCount = pResource->getMaterialCount();
		m_hdr.iSkinCount = pResource->getSkinCount();

		char szName[MODEL_MAX_NAME];

		for(UINT j = 0; j < m_hdr.iSkinCount; j++)
		{
			for(uint32_t i = 0; i < m_hdr.iMaterialCount; i++)
			{
				strncpy(szName, pResource->getMaterial(i, j), MODEL_MAX_NAME - 1);
				szName[MODEL_MAX_NAME - 1] = 0;
				pFile->writeBin(szName, MODEL_MAX_NAME);
			}
		}
	}

	{
		m_hdr2.iPhysicsDataOffset = pFile->getPos();
		ModelPhysData physData = {};
		physData.fWeight = pResource->getMass();

		pFile->writeBin(&physData, MODEL_PHYSDATA_STRUCT_SIZE);
		physData.iPhyspartCount = pResource->getPhysboxCount();
		if(physData.iPhyspartCount)
		{
			ModelPhyspart *pPhysparts = (ModelPhyspart*)_malloca(sizeof(ModelPhyspart) * physData.iPhyspartCount);
			memset(pPhysparts, 0, sizeof(ModelPhyspart) * physData.iPhyspartCount);


			// write parts
			for(UINT i = 0; i < physData.iPhyspartCount; ++i)
			{
				ModelPhyspart &part = pPhysparts[i];
				IModelPhysbox *pPhysbox = pResource->getPhysbox(i);;

				part.pos = pPhysbox->getPosition();
				part.rot = pPhysbox->getOrientation().toFloat4();

				part.bone_id = pResource->getPhysboxBone(i);
				if(part.bone_id >= 0 && pResource->getType() == XMT_ANIMATED)
				{
					strncpy(part.bone, pResource->asAnimated()->getBoneName(part.bone_id), MODEL_MAX_NAME - 1);
				}

				switch(pPhysbox->getType())
				{
				case XPBT_BOX:
					part.type = HT_BOX;
					part.lwh = pPhysbox->asBox()->getSize();
					break;
				case XPBT_SPHERE:
					part.type = HT_ELIPSOID;
					part.lwh = float3(pPhysbox->asSphere()->getRadius());
					break;
				case XPBT_CYLINDER:
					// TODO check that
					part.type = HT_CYLINDER;
					part.lwh.x = pPhysbox->asCylinder()->getRadius();
					part.lwh.z = part.lwh.x;
					part.lwh.y = pPhysbox->asCylinder()->getHeight();
					break;
				case XPBT_CAPSULE:
					// TODO check that
					part.type = HT_CAPSULE;
					part.lwh.x = pPhysbox->asCapsule()->getRadius();
					part.lwh.z = part.lwh.x;
					part.lwh.y = pPhysbox->asCapsule()->getHeight();
					break;
				case XPBT_CONVEX:
					part.type = HT_CONVEX;
					part.iDataOffset = pFile->getPos();

					IModelPhysboxConvex *pConvex = pPhysbox->asConvex();

					ModelPhyspartDataConvex data = {};
					data.iVertCount = pConvex->getVertexCount();
					pFile->writeBin(&data, MODEL_PHYSPART_DATA_CONVEX_STRUCT_SIZE);
					pFile->writeBin(pConvex->getData(), sizeof(float3_t) * data.iVertCount);


					break;
				}
			}

			physData.iPartsOffset = pFile->getPos();
			for(UINT i = 0; i < physData.iPhyspartCount; ++i)
			{
				pFile->writeBin(&pPhysparts[i], MODEL_PHYSPART_STRUCT_SIZE);
			}

			_freea(pPhysparts);

			size_t pos = pFile->getPos();
			pFile->setPos(m_hdr2.iPhysicsDataOffset);
			pFile->writeBin(&physData, MODEL_PHYSDATA_STRUCT_SIZE);
			pFile->setPos(pos);
		}
	}

	if(pResource->getType() == XMT_STATIC)
	{
		IXResourceModelStatic *pStatic = pResource->asStatic();

		switch(pStatic->getPrimitiveTopology())
		{
		case XPT_TRIANGLESTRIP:
			m_hdr2.topology = MDLPT_TRIANGLESTRIP;
			break;
		case XPT_TRIANGLELIST:
		default:
			m_hdr2.topology = MDLPT_TRIANGLELIST;
		}

		m_hdr.iLODcount = pResource->getLodCount();
		m_hdr.iLODoffset = pFile->getPos();


		for(uint32_t i = 0; i < m_hdr.iLODcount; i++)
		{
			ModelLoD lod = {};
			lod.iSubMeshCount = pStatic->getSubsetCount(i);
			pFile->writeBin(&lod, MODEL_LOD_STRUCT_SIZE);

			for(uint32_t j = 0; j < lod.iSubMeshCount; j++)
			{
				XResourceModelStaticSubset *pSubset = pStatic->getSubset(i, j);

				uint32_t uMaterialID = pSubset->iMaterialID;
				uint32_t uVertexCount = pSubset->iVertexCount;
				uint32_t  uIndexCount = pSubset->iIndexCount;
				pFile->writeBin(&uMaterialID, sizeof(uint32_t));
				pFile->writeBin(&uVertexCount, sizeof(uint32_t));
				pFile->writeBin(&uIndexCount, sizeof(uint32_t));

				static_assert(sizeof(vertex_static_ex) == sizeof(XResourceModelStaticVertex), "vertex_static_ex must be equal to XResourceModelStaticVertex");
				pFile->writeBin(pSubset->pVertices, sizeof(vertex_static_ex) * pSubset->iVertexCount);

				pFile->writeBin(pSubset->pIndices, sizeof(UINT) * pSubset->iIndexCount);
			}
		}
	}
	else
	{
		assert(!"Not implemented!");
		return(false);
	}

	pFile->setPos(0);
	pFile->writeBin(&m_hdr, sizeof(m_hdr));
	if(m_bWantHdr2)
	{
		pFile->writeBin(&m_hdr2, sizeof(m_hdr2));
	}

	return(true);
}

#if 0
bool XMETHODCALLTYPE CModelLoader::loadAsAnimated(IXResourceModelAnimated *pResource)
{
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

	if(m_hdr2.iHitboxCount && m_hdr2.iHitboxesOffset)
	{
		m_pCurrentFile->setPos((size_t)m_hdr2.iHitboxesOffset);

		pResource->setHitboxCount(m_hdr2.iHitboxCount);

		ModelHitbox mh;
		for(UINT i = 0; i < m_hdr2.iHitboxCount; ++i)
		{
			m_pCurrentFile->readBin(&mh, sizeof(ModelHitbox));
			XResourceModelHitbox *pHitbox = pResource->getHitbox(i);

			assert(!"Check bone_id");
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

	return(true);
}
#endif
