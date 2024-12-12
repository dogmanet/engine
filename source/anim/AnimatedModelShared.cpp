#include "AnimatedModelShared.h"
#include "AnimatedModelProvider.h"
#include <mtrl/IXMaterialSystem.h>

CAnimatedModelShared::CAnimatedModelShared(CAnimatedModelProvider *pProvider):
	m_pProvider(pProvider),
	m_pDevice(pProvider->getDevice()),
	m_pMaterialSystem(pProvider->getMaterialSystem())
{}
CAnimatedModelShared::~CAnimatedModelShared()
{
	m_pProvider->onSharedModelRelease(this);

	mem_delete_a(m_pBones);
	mem_delete_a(m_pInvBindPose);

	for(UINT i = 0; i < m_uSkinCount; ++i)
	{
		for(UINT j = 0; j < m_uMaterialCount; ++j)
		{
			mem_release(m_pppMaterials[i][j]);
		}
	}

	mem_delete_a(m_ppMaterialsBlob);
	m_pppMaterials = NULL;

	if(m_pDevice)
	{
		for(UINT i = 0; i < m_uLodCount; ++i)
		{
			mem_release(m_ppRenderBuffer[i]);
			mem_release(m_ppIndexBuffer[i]);
		}
		mem_delete_a(m_ppRenderBuffer);
		mem_delete_a(m_ppIndexBuffer);
	}

	for(UINT i = 0, l = m_aControllers.size(); i < l; ++i)
	{
		mem_delete_a(m_aControllers[i].pBones);
	}

	for(UINT i = 0, l = m_aSequences.size(); i < l; ++i)
	{
		for(UINT k = 0; k < m_aSequences[i].uNumFrames; ++k)
		{
			mem_delete_a(m_aSequences[i].m_ppSequenceData[k]);
		}
		mem_delete_a(m_aSequences[i].m_ppSequenceData);
	}

	for(UINT i = 0, l = m_apResources.size(); i < l; ++i)
	{
		mem_release(m_apResources[i]);
	}

	if(m_ppTempIndices)
	{
		for(UINT i = 0; i < m_uLodCount; ++i)
		{
			mem_delete_a(m_ppTempIndices[i]);
			mem_delete_a(m_ppTempVertices[i]);
		}
		mem_delete_a(m_ppTempIndices);
		mem_delete_a(m_ppTempVertices);
		mem_delete_a(m_puTempTotalIndices);
		mem_delete_a(m_puTempTotalVertices);
	}
}
void CAnimatedModelShared::AddRef()
{
	++m_uRefCount;
}
void CAnimatedModelShared::Release()
{
	--m_uRefCount;
	if(!m_uRefCount)
	{
		delete this;
	}
}

void CAnimatedModelShared::_collectResources(IXResourceModelAnimated *pResource, Array<IXResourceModelAnimated*> &aResources)
{
	for(UINT i = 0, l = aResources.size(); i < l; ++i)
	{
		if(aResources[i] == pResource)
		{
			return;
		}
	}

	// m_apResources.push_back(pResource);
	aResources.push_back(pResource);

	IXResourceModel *pRes = NULL;
	for(UINT i = 0, l = pResource->getImportsCount(); i < l; ++i)
	{
		pRes = pResource->getImport(i);
		if(pRes->getType() == XMT_ANIMATED)
		{
			_collectResources(pRes->asAnimated(), aResources);
		}
	}

	for(UINT i = 0, l = pResource->getPartsCount(); i < l; ++i)
	{
		pRes = pResource->getPart(i);
		if(pRes->getType() == XMT_ANIMATED)
		{
			_collectResources(pRes->asAnimated(), aResources);
		}
	}
}

bool CAnimatedModelShared::init(UINT uResourceCount, IXResourceModelAnimated **ppResources)
{
	for(UINT i = 0; i < uResourceCount; ++i)
	{
		m_apResources.push_back(ppResources[i]);
		ppResources[i]->AddRef();
	}
	m_apResources.quickSort([](IXResourceModelAnimated *a, IXResourceModelAnimated *b){
		return(a < b);
	});

	Array<IXResourceModelAnimated*> aResources;
	for(UINT i = 0; i < uResourceCount; ++i)
	{
		//aResources.push_back(ppResources[i]);
		_collectResources(ppResources[i], aResources);
	}

	// merge activities
	for(UINT i = 0, l = aResources.size(); i < l; ++i)
	{
		IXResourceModelAnimated *pResource = aResources[i];
		for(UINT j = 0, jl = pResource->getActivitiesCount(); j < jl; ++j)
		{
			bool isFound = false;
			const char *szActivityName = pResource->getActivityName(j);
			for(UINT k = 0, kl = m_aszActivities.size(); k < kl; ++k)
			{
				if(!strcasecmp(m_aszActivities[k], szActivityName))
				{
					isFound = true;
					break;
				}
			}
			if(!isFound)
			{
				m_aszActivities.push_back(szActivityName);
			}
		}
	}

	// merge bones hierarchy
	{
		UINT uTotalBones = 0;
		for(UINT i = 0, l = aResources.size(); i < l; ++i)
		{
			uTotalBones += aResources[i]->getBoneCount();
		}

		bone_node *pNodes = (bone_node*)alloca(sizeof(bone_node)* uTotalBones);

		// build chains
		UINT uChainCount = 0;
		bone_node *pStartNode = pNodes;
		for(UINT i = 0, l = aResources.size(); i < l; ++i)
		{
			IXResourceModelAnimated *pResource = aResources[i];
			for(UINT j = 0, jl = pResource->getBoneCount(); j < jl; ++j)
			{
				int iParent = pResource->getBoneParent(j);
				pStartNode[j] = {i, j, iParent >= 0 ? &pStartNode[iParent] : NULL, pResource->getBoneName(j), false};
				if(iParent < 0)
				{
					++uChainCount;
				}
			}
			pStartNode += pResource->getBoneCount();
		}

		for(UINT i = 0; i < uTotalBones; ++i)
		{
			if(!pNodes[i].pParent && pNodes[i].szName)
			{
				for(UINT j = 0; j < uTotalBones; ++j)
				{
					// same name found
					if(pNodes[j].pParent && pNodes[j].szName && i != j && !strcasecmp(pNodes[i].szName, pNodes[j].szName))
					{
						pNodes[i].pParent = pNodes[j].pParent;
						pNodes[i].bLooseTransform = true;
					}
				}
			}
		}
		_mergeByParent(NULL, pNodes, uTotalBones);

		// validate
		for(UINT i = 0; i < uTotalBones; ++i)
		{
			if(pNodes[i].szName)
			{
				for(UINT j = i + 1; j < uTotalBones; ++j)
				{
					// same name found
					if(pNodes[j].szName && !strcasecmp(pNodes[i].szName, pNodes[j].szName))
					{
						//@TODO: add file names to report!
						LibReport(REPORT_MSG_LEVEL_WARNING, "Unable to merge models, incompatible bones hierarchy!\n");
						return(false);
					}
				}
			}
		}
		//build final list
		m_uBoneCount = 0;
		for(UINT i = 0; i < uTotalBones; ++i)
		{
			if(pNodes[i].szName)
			{
				++m_uBoneCount;
			}
		}
		m_pBones = new bone_s[m_uBoneCount];
		// memset(m_pBones, 254, sizeof(bone_s)* uFinalBones);
		_buildBoneListByParent(NULL, pNodes, uTotalBones, m_pBones, &aResources[0], -1);

		// calc inverted bind pose
		{
			m_pInvBindPose = new XResourceModelBone[m_uBoneCount];

			for(UINT i = 0; i < m_uBoneCount; ++i)
			{
				m_pInvBindPose[i].orient = m_pBones[i].bone.orient;
				m_pInvBindPose[i].position = m_pBones[i].bone.position;
			}
			for(UINT i = 0; i < m_uBoneCount; ++i)
			{
				if(m_pBones[i].iParent != -1)
				{
					m_pInvBindPose[i].orient = (m_pInvBindPose[i].orient * m_pInvBindPose[m_pBones[i].iParent].orient).Normalize();
					m_pInvBindPose[i].position = (float3)(float4(m_pInvBindPose[m_pBones[i].iParent].orient * m_pInvBindPose[i].position, 1.0f) + m_pInvBindPose[m_pBones[i].iParent].position);
				}
			}
			for(UINT i = 0; i < m_uBoneCount; ++i)
			{
				m_pInvBindPose[i].orient = m_pInvBindPose[i].orient.Conjugate();
				m_pInvBindPose[i].position = (float3)-m_pInvBindPose[i].position;
			}
		}
	}

	// merge skins/materials
	Array<Array<mtl_node>> aaMaterials;
	{
		UINT uTotalMaterials = 0;
		for(UINT i = 0, l = aResources.size(); i < l; ++i)
		{
			UINT uSkinCount = aResources[i]->getSkinCount();
			aaMaterials.reserve(uSkinCount);
			for(UINT j = aaMaterials.size(); j < uSkinCount; ++j)
			{
				aaMaterials[j].resize(uTotalMaterials);
			}
			for(UINT k = 0, kl = aResources[i]->getMaterialCount(); k < kl; ++k)
			{
				Array<mtl_node> &aFirstSkin = aaMaterials[0];
				const char *szMaterial = aResources[i]->getMaterial(k);
				bool isFound = false;
				for(UINT j = 0, jl = aFirstSkin.size(); j < jl; ++j)
				{
					// same name, test skins
					if(aFirstSkin[j].szName && !fstrcmp(aFirstSkin[j].szName, szMaterial))
					{
						isFound = true;
						// start from 1 as 0 slot already tested
						for(UINT m = 1; m < uSkinCount; ++m)
						{
							const char *szSkin = aResources[i]->getMaterial(k, m);
							if(!((aaMaterials[m][j].szName && !fstrcmp(aaMaterials[m][j].szName, szSkin))
								|| ((!aaMaterials[m][j].szName || !aaMaterials[m][j].szName[0]) && !szSkin[0])))
							{
								isFound = false;
								break;
							}
						}
						if(isFound)
						{
							break;
						}
					}
				}
				if(!isFound)
				{
					for(UINT j = 0; j < uSkinCount; ++j)
					{
						mtl_node node;
						node.szName = aResources[i]->getMaterial(k, j);
						node.uResource = i;
						aaMaterials[j][uTotalMaterials] = node;
					}
					++uTotalMaterials;
				}
			}
		}

		m_uMaterialCount = uTotalMaterials;
		m_uSkinCount = aaMaterials.size();
		m_ppMaterialsBlob = new void*[m_uMaterialCount * m_uSkinCount + m_uSkinCount];
		m_pppMaterials = (IXMaterial***)m_ppMaterialsBlob;
		
		for(UINT i = 0; i < m_uSkinCount; ++i)
		{
			m_pppMaterials[i] = (IXMaterial**)(m_ppMaterialsBlob + m_uSkinCount + m_uMaterialCount * i);

			for(UINT j = 0; j < m_uMaterialCount; ++j)
			{
				if(i == 0 || aaMaterials[i][j].szName && aaMaterials[i][j].szName[0])
				{
					m_pMaterialSystem->loadMaterial(aaMaterials[i][j].szName ? aaMaterials[i][j].szName : "", &m_pppMaterials[i][j]);
				}
				else
				{
					m_pppMaterials[i][j] = m_pppMaterials[0][j];
					m_pppMaterials[i][j]->AddRef();
				}
			}
		}
	}

	// define parts
	{
		Array<IXResourceModelAnimated*> aPartedResources;
		Array<IXResourceModelAnimated*> aPart;
		m_aParts[0].szName = "";
		m_aParts[0].flags = XMP_NONE;

		Array<Array<Array<merge_subset_s>>> aPartLods;

		for(UINT i = 0; i < uResourceCount; ++i)
		{
			for(UINT j = 0, jl = ppResources[i]->getPartsCount(); j < jl; ++j)
			{
				IXResourceModel *pRes = ppResources[i]->getPart(j);
				XMODEL_IMPORT importFlags = ppResources[i]->getPartImportFlags(j);
				//@TODO: Test XMODEL_IMPORT flags
				if(pRes->getType() == XMT_ANIMATED)
				{
					aPart.push_back(pRes->asAnimated());
					_collectResources(pRes->asAnimated(), aPart);

					for(UINT k = 0, kl = aPart.size(); k < kl; ++k)
					{
						bool isFound = false;
						for(UINT m = 0, ml = aPartedResources.size(); m < ml; ++m)
						{
							if(aPartedResources[m] == aPart[k])
							{
								isFound = true;
								aPart.erase(k);
								--k; --kl;
								break;
							}
						}
						if(!isFound)
						{
							aPartedResources.push_back(aPart[k]);
						}
					}

					// process part:
					part_s part;
					_initPart(aPart, &part, aPartLods[m_aParts.size()], aaMaterials, importFlags);
					part.szName = ppResources[i]->getPartName(j);
					part.flags = ppResources[i]->getPartFlags(j);

					m_aParts.push_back(part);

					aPart.clearFast();
				}
			}
		}

		for(UINT i = 0, l = aResources.size(); i < l; ++i)
		{
			bool isFound = false;
			for(UINT j = 0, jl = aPartedResources.size(); j < jl; ++j)
			{
				if(aPartedResources[j] == aResources[i])
				{
					isFound = true;
					break;
				}
			}

			if(!isFound)
			{
				aPart.push_back(aResources[i]);
			}
		}

		_initPart(aPart, &m_aParts[0], aPartLods[0], aaMaterials);

		Array<UINT> aLodIndexCount;
		Array<UINT> aLodVertexCount;

		for(UINT uMaterialId = 0, uMaterialCount = aaMaterials[0].size(); uMaterialId < uMaterialCount; ++uMaterialId)
		{
			for(UINT uPart = 0, uPartCount = aPartLods.size(); uPart < uPartCount; ++uPart)
			{
				auto &aLods = aPartLods[uPart];
				for(UINT uLod = 0, uLodCount = aLods.size(); uLod < uLodCount; ++uLod)
				{
					if(uLodCount > m_uLodCount)
					{
						for(UINT i = m_uLodCount; i < uLodCount; ++i)
						{
							aLodIndexCount[i] = 0;
							aLodVertexCount[i] = 0;
						}
						m_uLodCount = uLodCount;
					}
					auto &aSubsets = aLods[uLod];
					subset_t subset;
					for(UINT i = 0, l = aSubsets.size(); i < l; ++i)
					{
						XResourceModelAnimatedSubset *pSubset = &aSubsets[i].data;
						if(pSubset->iMaterialID == uMaterialId)
						{
							// append subset to lod (group by materialId)

							if(!subset.uIndexCount)
							{
								subset.uStartIndex = aLodIndexCount[uLod];
								subset.uStartVertex = aLodVertexCount[uLod];
							}
							subset.uIndexCount += pSubset->iIndexCount;
							subset.uVertexCount += pSubset->iVertexCount;

							aLodIndexCount[uLod] += pSubset->iIndexCount;
							aLodVertexCount[uLod] += pSubset->iVertexCount;
						}
					}

					m_aParts[uPart].aLods[uLod][uMaterialId] = subset;
				}
			}
		}

		if(!m_pProvider->getCore()->isOnMainThread())
		{
			m_puTempTotalIndices = new UINT[m_uLodCount];
			m_puTempTotalVertices = new UINT[m_uLodCount];
		}

		UINT **ppIndices = new UINT*[m_uLodCount];
		XResourceModelAnimatedVertexGPU **ppVertices = new XResourceModelAnimatedVertexGPU*[m_uLodCount];
		for(UINT i = 0; i < m_uLodCount; ++i)
		{
			ppIndices[i] = new UINT[aLodIndexCount[i]];
			ppVertices[i] = new XResourceModelAnimatedVertexGPU[aLodVertexCount[i]];
		}


		float3 vLocalMin;
		float3 vLocalMax;
		bool isMinMaxSet = false;
		// fill buffers!
		for(UINT uMaterialId = 0, uMaterialCount = aaMaterials[0].size(); uMaterialId < uMaterialCount; ++uMaterialId)
		{
			Array<UINT> aLodVertexCount;
			aLodVertexCount.resizeFast(m_uLodCount);
			for(UINT uLod = 0; uLod < m_uLodCount; ++uLod)
			{
				aLodVertexCount[uLod] = 0;
			}

			for(UINT uPart = 0, uPartCount = aPartLods.size(); uPart < uPartCount; ++uPart)
			{
				auto &aLods = aPartLods[uPart];
				for(UINT uLod = 0, uLodCount = aLods.size(); uLod < uLodCount; ++uLod)
				{
					auto &aSubsets = aLods[uLod];
					subset_t &subset = m_aParts[uPart].aLods[uLod][uMaterialId];
					for(UINT i = 0, l = aSubsets.size(); i < l; ++i)
					{
						XResourceModelAnimatedSubset *pSubset = &aSubsets[i].data;
						if(pSubset->iMaterialID == uMaterialId)
						{
							// append subset to lod (group by materialId)

							//memcpy(&ppVertices[uLod][subset.uStartVertex], pSubset->pVertices, sizeof(XResourceModelAnimatedVertex) * pSubset->iVertexCount);
							for(UINT k = 0; k < pSubset->iVertexCount; ++k)
							{
#define TO_SHORT(v) ((short)((v) * 32767.0f))
								auto &dst = (ppVertices[uLod] + subset.uStartVertex)[k];
								auto &src = pSubset->pVertices[k];
								dst.vPos = src.vPos;
								dst.vTex = src.vTex;
								dst.vNorm[0] = TO_SHORT(src.vNorm.x);
								dst.vNorm[1] = TO_SHORT(src.vNorm.y);
								dst.vNorm[2] = TO_SHORT(src.vNorm.z);
								dst.vTangent[0] = TO_SHORT(src.vTangent.x);
								dst.vTangent[1] = TO_SHORT(src.vTangent.y);
								dst.vTangent[2] = TO_SHORT(src.vTangent.z);
								dst.vBinorm[0] = TO_SHORT(src.vBinorm.x);
								dst.vBinorm[1] = TO_SHORT(src.vBinorm.y);
								dst.vBinorm[2] = TO_SHORT(src.vBinorm.z);
								for(UINT k = 0; k < 4; ++k)
								{
									dst.u8BoneIndices[k] = src.u8BoneIndices[k];
								}
								dst.vBoneWeights[0] = (byte)(src.vBoneWeights.x * 255.0f);
								dst.vBoneWeights[1] = (byte)(src.vBoneWeights.y * 255.0f);
								dst.vBoneWeights[2] = (byte)(src.vBoneWeights.z * 255.0f);
								dst.vBoneWeights[3] = (byte)(src.vBoneWeights.w * 255.0f);
#undef TO_SHORT
							}

							//memcpy(&ppIndices[uLod][subset.uStartIndex], pSubset->pIndices, sizeof(UINT) * pSubset->iIndexCount);
							UINT uIndexDelta = aLodVertexCount[uLod];
							for(UINT j = 0; j < pSubset->iIndexCount; ++j)
							{
								ppIndices[uLod][subset.uStartIndex + j] = pSubset->pIndices[j] + uIndexDelta;
							}

							//@TODO: optimize that!!!
							for(UINT j = 0; j < pSubset->iVertexCount; ++j)
							{
								XResourceModelAnimatedVertexGPU &vtx = ppVertices[uLod][subset.uStartVertex + j];
								for(UINT k = 0; k < 4; ++k)
								{
									vtx.u8BoneIndices[k] = getBoneId(aSubsets[i].pResource->getBoneName(vtx.u8BoneIndices[k]));
								}

								if(isMinMaxSet)
								{
									vLocalMin = SMVectorMin(vLocalMin, vtx.vPos);
									vLocalMax = SMVectorMax(vLocalMax, vtx.vPos);
								}
								else
								{
									vLocalMin = vLocalMax = vtx.vPos;
									isMinMaxSet = true;
								}
							}

							aLodVertexCount[uLod] += pSubset->iVertexCount;
						}
					}
				}
			}
		}

		m_vLocalMin = vLocalMin;
		m_vLocalMax = vLocalMax;

		if(m_pDevice)
		{
			bool useDelayedInit = false;

			m_ppRenderBuffer = new IGXRenderBuffer*[m_uLodCount];
			m_ppIndexBuffer = new IGXIndexBuffer*[m_uLodCount];

			for(UINT i = 0; i < m_uLodCount; ++i)
			{
				if(m_pProvider->getCore()->isOnMainThread())
				{
					m_ppIndexBuffer[i] = m_pDevice->createIndexBuffer(sizeof(UINT) * aLodIndexCount[i], GXBUFFER_USAGE_STATIC, GXIT_UINT32, ppIndices[i]);
					IGXVertexBuffer *pVertexBuffer = m_pDevice->createVertexBuffer(sizeof(XResourceModelAnimatedVertexGPU) * aLodVertexCount[i], GXBUFFER_USAGE_STATIC, ppVertices[i]);
					m_ppRenderBuffer[i] = m_pDevice->createRenderBuffer(1, &pVertexBuffer, m_pProvider->getVertexDeclaration());
					mem_release(pVertexBuffer);
				}
				else
				{
					m_ppIndexBuffer[i] = NULL;
					m_ppRenderBuffer[i] = NULL;

					m_ppTempIndices = ppIndices;
					m_ppTempVertices = ppVertices;
					m_puTempTotalIndices[i] = aLodIndexCount[i];
					m_puTempTotalVertices[i] = aLodVertexCount[i];

					useDelayedInit = true;
				}
			}

			if(useDelayedInit)
			{
				m_pProvider->scheduleSharedGPUinit(this);
			}
		}
			
		if(m_pProvider->getCore()->isOnMainThread())
		{
			for(UINT i = 0; i < m_uLodCount; ++i)
			{
				mem_delete_a(ppIndices[i]);
				mem_delete_a(ppVertices[i]);
			}
			mem_delete_a(ppIndices);
			mem_delete_a(ppVertices);
		}
	}


	// merge controllers
	for(UINT i = 0, l = aResources.size(); i < l; ++i)
	{
		const IXResourceModelAnimated *pResource = aResources[i];
		for(UINT j = 0, jl = pResource->getControllersCount(); j < jl; ++j)
		{
			const XResourceModelController *pController = pResource->getController(j);
			m_aControllers.push_back(*pController);
			XResourceModelController &ctl = m_aControllers[m_aControllers.size() - 1];
			ctl.pBones = new UINT[ctl.uBoneCount];

			for(UINT k = 0; k < ctl.uBoneCount; ++k)
			{
				ctl.pBones[k] = getBoneId(pResource->getBoneName(pController->pBones[k]));
			}
		}
	}


	// merge sequences
	{
		Array<UINT> uBoneMap;
		for(UINT i = 0, l = aResources.size(); i < l; ++i)
		{
			const IXResourceModelAnimated *pResource = aResources[i];

			UINT uBoneCount = pResource->getBoneCount();
			for(UINT j = 0, jl = uBoneCount; j < jl; ++j)
			{
				uBoneMap[j] = getBoneId(pResource->getBoneName(j));
			}

			for(UINT j = 0, jl = pResource->getSequenceCount(); j < jl; ++j)
			{
				const XResourceModelSequence *pSequence = pResource->getSequence(j);
				XResourceModelSequence *pNewSequence = NULL;

				for(UINT k = 0, kl = m_aSequences.size(); k < kl; ++k)
				{
					if(!strcasecmp(m_aSequences[k].szName, pSequence->szName))
					{
						//@TODO: Add file names to report
						LibReport(REPORT_MSG_LEVEL_WARNING, "Merging sequence '%s'\n",
							pSequence->szName);
						pNewSequence = &m_aSequences[k];
					}
				}
				if(pNewSequence)
				{
					if(pNewSequence->uNumFrames != pSequence->uNumFrames)
					{
						//@TODO: Add file names to report
						LibReport(REPORT_MSG_LEVEL_WARNING, "Unable to merge sequence '%s': frame count mismatch %u != %u\n",
							pNewSequence->szName, pNewSequence->uNumFrames, pSequence->uNumFrames);
						continue;
					}
				}
				else
				{
					m_aSequences.push_back(*pSequence);
					pNewSequence = &m_aSequences[m_aSequences.size() - 1];

					pNewSequence->m_ppSequenceData = new XResourceModelBone*[pNewSequence->uNumFrames];
					for(UINT k = 0; k < pNewSequence->uNumFrames; ++k)
					{
						pNewSequence->m_ppSequenceData[k] = new XResourceModelBone[m_uBoneCount];
					}
				}

				for(UINT k = 0; k < pNewSequence->uNumFrames; ++k)
				{
					for(UINT m = 0, ml = uBoneCount; m < ml; ++m)
					{
						pNewSequence->m_ppSequenceData[k][uBoneMap[m]] = pSequence->m_ppSequenceData[k][m];
					}
				}
			}

			uBoneMap.clearFast();
		}
	}


	return(true);
}

void CAnimatedModelShared::initGPUresources()
{
	if(!m_ppTempIndices)
	{
		return;
	}
	assert(m_pProvider->getCore()->isOnMainThread());

	for(UINT i = 0; i < m_uLodCount; ++i)
	{
		m_ppIndexBuffer[i] = m_pDevice->createIndexBuffer(sizeof(UINT) * m_puTempTotalIndices[i], GXBUFFER_USAGE_STATIC, GXIT_UINT32, m_ppTempIndices[i]);
		IGXVertexBuffer *pVertexBuffer = m_pDevice->createVertexBuffer(sizeof(XResourceModelAnimatedVertexGPU) * m_puTempTotalVertices[i], GXBUFFER_USAGE_STATIC, m_ppTempVertices[i]);
		m_ppRenderBuffer[i] = m_pDevice->createRenderBuffer(1, &pVertexBuffer, m_pProvider->getVertexDeclaration());
		mem_release(pVertexBuffer);

		mem_delete_a(m_ppTempIndices[i]);
		mem_delete_a(m_ppTempVertices[i]);
	}
	mem_delete_a(m_ppTempIndices);
	mem_delete_a(m_ppTempVertices);
	mem_delete_a(m_puTempTotalIndices);
	mem_delete_a(m_puTempTotalVertices);
}

void CAnimatedModelShared::_initPart(Array<IXResourceModelAnimated*> &aPart, part_s *pPart, Array<Array<merge_subset_s>> &aPartLods, Array<Array<mtl_node>> &aaMaterials, XMODEL_IMPORT importFlags)
{
	UINT uLodCount = 1;
	for(UINT k = 1, kl = aPart.size(); k < kl; ++k)
	{
		if(importFlags & XMI_MESH)
		{
			UINT uLods = aPart[k]->getLodCount();
			if(uLods > uLodCount)
			{
				uLodCount = uLods;
			}
		}
	}

	aPartLods.resizeFast(uLodCount);

	for(UINT k = 0, kl = aPart.size(); k < kl; ++k)
	{
		// merge vertices/indices
		if(importFlags & XMI_MESH)
		{
			for(UINT m = 0, ml = aPart[k]->getLodCount(); m < ml; ++m)
			{
				// vertices/indices merged
				// indices part map {startIndex, count}
				UINT uPartOffset = aPartLods[m].size();
				aPartLods[m].resizeFast(uPartOffset + aPart[k]->getSubsetCount(m));
				for(UINT n = 0, nl = aPart[k]->getSubsetCount(m); n < nl; ++n)
				{
					const XResourceModelAnimatedSubset *pSubset = aPart[k]->getSubset(m, n);
					int uid = pSubset->iIndexCount;
					aPartLods[m][n + uPartOffset].data = *pSubset;
					aPartLods[m][n + uPartOffset].pResource = aPart[k];

					const char *szMainMaterial = aPart[k]->getMaterial(pSubset->iMaterialID, 0);
					for(UINT i = 0, l = aaMaterials[0].size(); i < l; ++i)
					{
						if(!strcasecmp(szMainMaterial, aaMaterials[0][i].szName))
						{
							bool isSame = true;
							for(UINT j = 1, jl = aaMaterials.size(); j < jl; ++j)
							{
								const char *szSkinMaterial = aPart[k]->getMaterial(pSubset->iMaterialID, j);
								if(!(
									(
										(!aaMaterials[j][i].szName || !aaMaterials[j][i].szName[0]) &&
										(!szSkinMaterial || !szSkinMaterial[0])
									)
									||
									(
										aaMaterials[j][i].szName && szSkinMaterial &&
										!strcasecmp(aaMaterials[j][i].szName, szSkinMaterial)
									)
								))
								{
									isSame = false;
									break;
								}
							}

							if(isSame)
							{
								aPartLods[m][n + uPartOffset].data.iMaterialID = i;
								break;
							}
						}
					}
				}
			}
		}

		// merge hitboxes
		if(importFlags & XMI_HITBOXES)
		{
			for(UINT m = 0, ml = aPart[k]->getHitboxCount(); m < ml; ++m)
			{
				Hitbox &hb = pPart->aHitboxes[pPart->aHitboxes.size()];
				hb.part = aPart[k]->getHitboxBodyPart(m);
				//hb.iBone = aPart[k]->getHitboxBone(m);
				hb.iBone = getBoneId(aPart[k]->getBoneName(aPart[k]->getHitboxBone(m)));
				hb.pPhysbox = aPart[k]->getHitbox(m);

				//				ctl.pBones[k] = getBoneId(pResource->getBoneName(pController->pBones[k]));

			}
		}

		// merge physboxes
		if(importFlags & XMI_PHYSBOXES)
		{
			for(UINT m = 0, ml = aPart[k]->getPhysboxCount(); m < ml; ++m)
			{
				int iBone = aPart[k]->getPhysboxBone(m);
				if(iBone >= 0)
				{
					iBone = getBoneId(aPart[k]->getBoneName(iBone));
				}
				pPart->aPhysboxes.push_back({aPart[k]->getPhysbox(m), iBone});
			}
		}
	}
}

int CAnimatedModelShared::_buildBoneListByParent(bone_node *pParent, bone_node *pNodes, UINT uTotalBones, bone_s *pList, IXResourceModelAnimated **ppResources, int iParent)
{
	int iOffset = 0;
	for(UINT i = 0; i < uTotalBones; ++i)
	{
		if(pNodes[i].pParent == pParent && pNodes[i].szName)
		{
			pList[iOffset].bone.position = ppResources[pNodes[i].uResource]->getBoneTranslation(pNodes[i].uBoneIdx);
			pList[iOffset].bone.orient = ppResources[pNodes[i].uResource]->getBoneRotation(pNodes[i].uBoneIdx);
			pList[iOffset].szName = pNodes[i].szName;
			pList[iOffset].iParent = iParent;
			++iOffset;
			iOffset += _buildBoneListByParent(&pNodes[i], pNodes, uTotalBones, pList + iOffset, ppResources, iParent + iOffset);
		}
	}
	return(iOffset);
}

void CAnimatedModelShared::_mergeByParent(bone_node *pParent, bone_node *pNodes, UINT uTotalBones)
{
	for(UINT i = 0; i < uTotalBones; ++i)
	{
		if(pNodes[i].pParent == pParent && pNodes[i].szName)
		{
			for(UINT j = 0; j < uTotalBones; ++j)
			{
				// same name found
				if(pNodes[j].pParent == pParent && pNodes[j].szName && i != j && !strcasecmp(pNodes[i].szName, pNodes[j].szName))
				{
					UINT uFrom = j, uTo = i;
					if(pNodes[i].bLooseTransform)
					{
						uFrom = i, uTo = j;
					}
					for(UINT k = j + 1; k < uTotalBones; ++k)
					{
						if(pNodes[k].pParent == &pNodes[uFrom])
						{
							pNodes[k].pParent = &pNodes[uTo];
						}
					}
					pNodes[uFrom].szName = NULL;
				}
			}
		}
	}

	//@TODO: can here be endless recursive with broken models?
	for(UINT i = 0; i < uTotalBones; ++i)
	{
		if(pNodes[i].pParent == pParent && pNodes[i].szName)
		{
			_mergeByParent(&pNodes[i], pNodes, uTotalBones);
		}
	}

}

bool CAnimatedModelShared::isSame(UINT uResourceCount, IXResourceModelAnimated **ppResources)
{
	if(uResourceCount != m_apResources.size())
	{
		return(false);
	}

	for(UINT i = 0; i < uResourceCount; ++i)
	{
		bool isFound = false;
		for(UINT j = 0, jl = m_apResources.size(); j < jl; ++j)
		{
			if((isFound = (m_apResources[j] == ppResources[i])))
			{
				break;
			}
		}
		if(!isFound)
		{
			return(false);
		}
	}

	return(true);
}

const Array<IXResourceModelAnimated*> &CAnimatedModelShared::getResources()
{
	return(m_apResources);
}


UINT CAnimatedModelShared::getPhysboxCount(UINT uPartIndex) const
{
	assert(uPartIndex < m_aParts.size());

	return(m_aParts[uPartIndex].aPhysboxes.size());
}
const IModelPhysbox *CAnimatedModelShared::getPhysBox(UINT id, UINT uPartIndex) const
{
	assert(uPartIndex < m_aParts.size() && id < m_aParts[uPartIndex].aPhysboxes.size());

	return(m_aParts[uPartIndex].aPhysboxes[id].pPhysbox);
}
const int CAnimatedModelShared::getPhysBoxBone(UINT id, UINT uPartIndex) const
{
	assert(uPartIndex < m_aParts.size() && id < m_aParts[uPartIndex].aPhysboxes.size());

	return(m_aParts[uPartIndex].aPhysboxes[id].iBone);
}
const IXResourceModel *CAnimatedModelShared::getResource(UINT uIndex)
{
	return(m_apResources[uIndex]);
}


UINT CAnimatedModelShared::getPartsCount() const
{
	return(m_aParts.size());
}
const char *CAnimatedModelShared::getPartName(UINT uIndex) const
{
	assert(uIndex < m_aParts.size());

	return(m_aParts[uIndex].szName);
}
UINT CAnimatedModelShared::getPartIndex(const char *szName)
{
	for(UINT i = 0, l = m_aParts.size(); i < l; ++i)
	{
		if(!strcasecmp(szName, m_aParts[i].szName))
		{
			return(i);
		}
	}
	return(~0);
}
XMODEL_PART_FLAGS CAnimatedModelShared::getPartFlags(UINT uIndex) const
{
	assert(uIndex < m_aParts.size());

	return(m_aParts[uIndex].flags);
}


UINT CAnimatedModelShared::getHitboxCount(UINT uPartIndex) const
{
	assert(uPartIndex < m_aParts.size());

	return(m_aParts[uPartIndex].aHitboxes.size());
}
const IModelPhysbox* CAnimatedModelShared::getHitbox(UINT id, UINT uPartIndex) const
{
	assert(uPartIndex < m_aParts.size() && id < m_aParts[uPartIndex].aHitboxes.size());

	return(m_aParts[uPartIndex].aHitboxes[id].pPhysbox);
}
int CAnimatedModelShared::getHitboxBone(UINT id, UINT uPartIndex) const
{
	assert(uPartIndex < m_aParts.size() && id < m_aParts[uPartIndex].aHitboxes.size());

	return(m_aParts[uPartIndex].aHitboxes[id].iBone);
}
XHITBOXBODYPART CAnimatedModelShared::getHitboxBodyPart(UINT id, UINT uPartIndex) const
{
	assert(uPartIndex < m_aParts.size() && id < m_aParts[uPartIndex].aHitboxes.size());

	return(m_aParts[uPartIndex].aHitboxes[id].part);
}


int CAnimatedModelShared::getBoneId(const char *szName)
{
	for(UINT i = 0; i < m_uBoneCount; ++i)
	{
		if(!strcasecmp(m_pBones[i].szName, szName))
		{
			return(i);
		}
	}
	return(-1);
}
UINT CAnimatedModelShared::getBoneCount() const
{
	return(m_uBoneCount);
}
const char *CAnimatedModelShared::getBoneName(UINT id) const
{
	assert(id < m_uBoneCount);

	return(m_pBones[id].szName);
}
int CAnimatedModelShared::getBoneParent(UINT id) const
{
	assert(id < m_uBoneCount);

	return(m_pBones[id].iParent);
}
const XResourceModelBone *CAnimatedModelShared::getBone(UINT id) const
{
	assert(id < m_uBoneCount);

	return(&m_pBones[id].bone);
}
const XResourceModelBone *CAnimatedModelShared::getInvertedBindPose() const
{
	return(m_pInvBindPose);
}

UINT CAnimatedModelShared::getControllersCount() const
{
	return(m_aControllers.size());
}
const char *CAnimatedModelShared::getControllerName(UINT id) const
{
	assert(id < m_aControllers.size());

	return(m_aControllers[id].szName);
}
UINT CAnimatedModelShared::getControllerId(const char *szName) const
{
	for(UINT i = 0; i < m_aControllers.size(); ++i)
	{
		if(!strcasecmp(m_aControllers[i].szName, szName))
		{
			return(i);
		}
	}
	return(~0);
}
const XResourceModelController *CAnimatedModelShared::getController(UINT uIndex) const
{
	assert(uIndex < m_aControllers.size());

	return(&m_aControllers[uIndex]);
}

int CAnimatedModelShared::getActivityId(const char *szName) const
{
	for(UINT i = 0, l = m_aszActivities.size(); i < l; ++i)
	{
		if(!strcasecmp(m_aszActivities[i], szName))
		{
			return(i);
		}
	}
	return(-1);
}

const char *CAnimatedModelShared::getActivityName(UINT id) const
{
	assert(id < m_aszActivities.size());

	return(m_aszActivities[id]);
}

UINT CAnimatedModelShared::getSequenceCount() const
{
	return(m_aSequences.size());
}
const XResourceModelSequence *CAnimatedModelShared::getSequence(UINT uIndex) const
{
	assert(uIndex < m_aSequences.size());

	return(&m_aSequences[uIndex]);
}

int CAnimatedModelShared::getSequenceId(const char *szName) const
{
	for(UINT i = 0, l = m_aSequences.size(); i < l; ++i)
	{
		if(!strcasecmp(m_aSequences[i].szName, szName))
		{
			return(i);
		}
	}
	return(-1);
}

float3 CAnimatedModelShared::getLocalBoundMin() const
{
	return(m_vLocalMin);
}

float3 CAnimatedModelShared::getLocalBoundMax() const
{
	return(m_vLocalMax);
}
SMAABB CAnimatedModelShared::getLocalBound() const
{
	return(SMAABB(m_vLocalMin, m_vLocalMax));
}

void CAnimatedModelShared::render(UINT uSkin, UINT uLod)
{
	if(!m_pDevice)
	{
		return;
	}

	if(uSkin >= m_uSkinCount)
	{
		uSkin = 0;
	}

	if(uLod >= m_uLodCount)
	{
		return;
	}

	IGXContext *pCtx = m_pDevice->getThreadContext();

	pCtx->setIndexBuffer(m_ppIndexBuffer[uLod]);
	pCtx->setRenderBuffer(m_ppRenderBuffer[uLod]);
	pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);

	for(UINT i = 0; i < m_uMaterialCount; ++i)
	{
		subset_t subset;
		//UINT uDrawIndices = 0;
		//UINT uDrawVertices = 0;
		for(UINT uPart = 0, uPartCount = m_aParts.size(); uPart < uPartCount; ++uPart)
		{
			if(true /* subset enabled */)
			{
				subset_t &lodSubset = m_aParts[uPart].aLods[uLod][i];

				if(subset.uIndexCount == 0)
				{
					subset = lodSubset;
				}
				else
				{
					subset.uIndexCount += lodSubset.uIndexCount;
					subset.uVertexCount += lodSubset.uVertexCount;
				}

				/*if(subset.uIndexCount == lodSubset.uStartIndex)
				{
					subset.uIndexCount += lodSubset.uIndexCount;
					subset.uVertexCount += lodSubset.uVertexCount;
				}
				else
				{
					// draw part (subset.uIndexCount - uDrawIndices), (subset.uVertexCount - uDrawVertices)
					uDrawIndices = subset.uIndexCount;
					uDrawVertices = subset.uVertexCount;
				}*/
			}
		}

		if(subset.uIndexCount != 0)
		{
			if(m_pMaterialSystem->bindMaterial(m_pppMaterials[uSkin][i]))
			{
				pCtx->drawIndexed(subset.uVertexCount, subset.uIndexCount / 3, subset.uStartIndex, subset.uStartVertex);
			}
		}
	}
}
