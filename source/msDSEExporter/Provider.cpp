#include <exporter_base/stdafx.h>
#include "Provider.h"

CProvider::CProvider(msModel *pModel):
	m_pModel(pModel)
{
}

bool CProvider::canExportTB()
{
	return(false);
}

bool CProvider::prepare(IProgress *pProgress, bool forStaticExport, bool bPrepareMesh, bool bPrepareTB)
{
	m_isStatic = forStaticExport;

	char szTmp[4096];
	if(bPrepareMesh)
	{
		int iMeshesCount = msModel_GetMeshCount(m_pModel);
		for(int i = 0; i < iMeshesCount; i++)
		{
			//meshes[i]->GetNodeParent();
			msMesh *pMesh = msModel_GetMeshAt(m_pModel, i);
			int iLod = 0;

			getConfigStr(pMesh, "layer", szTmp, sizeof(szTmp));

			int iLayer = m_aLayers.indexOf(szTmp, [](const Layer &pLayer, const char *szLayer){
				return(!fstrcmp(pLayer.sName.c_str(), szLayer));
			});
			if(iLayer < 0)
			{
				iLayer = (int)m_aLayers.size();
				m_aLayers[iLayer].sName = szTmp;
			}

			Object &obj = m_aLayers[iLayer].aObjects[m_aLayers[iLayer].aObjects.size()];

			msVec3 v3;
			msModel_GetPosition(m_pModel, v3);
			obj.vPosition = toPosition(v3);
			msModel_GetRotation(m_pModel, v3);
			obj.qRotation = toRotation(v3);

			msMesh_GetName(pMesh, szTmp, sizeof(szTmp));
			obj.sName = szTmp;

			Subset &ss = obj.aSubsets[0];

			msMaterial *pMtl = msModel_GetMaterialAt(m_pModel, msMesh_GetMaterialIndex(pMesh));
			msMaterial_GetDiffuseTexture(pMtl, szTmp, sizeof(szTmp));
			ss.sTexture = szTmp;

			// ss.aIndices
			int iVertexCount = msMesh_GetVertexCount(pMesh);
			int iTriangleCount = msMesh_GetTriangleCount(pMesh);

			ss.aIndices.reserve(iTriangleCount * 3);

			if(forStaticExport)
			{
				struct StaticVertexCache
				{
					int idVertex;
					int idNormal;
					//float2_t vTexCoord;

					bool operator<(const StaticVertexCache &other) const
					{
						return(memcmp(this, &other, sizeof(*this)) < 0);
					}
					bool operator==(const StaticVertexCache &other) const
					{
						return(memcmp(this, &other, sizeof(*this)) == 0);
					}
				};
				AssotiativeArray<StaticVertexCache, UINT> mVertexCache;
				const AssotiativeArray<StaticVertexCache, UINT>::Node *pNode;
				StaticVertexCache svc = {};
				word pVtxs[3] = {};
				word pNrmls[3] = {};
				msVec2 vTexCoord = {};
				msVec3 vPos = {};
				UINT idx;
				msVertex *pVtx;
				msTriangle *pTri;
				//msTriangleEx *pTriEx;


				ss.aVerticesStatic.reserve(iVertexCount);

				for(int j = 0; j < iTriangleCount; ++j)
				{
					pTri = msMesh_GetTriangleAt(pMesh, j);
					//pTriEx = msMesh_GetTriangleExAt(pMesh, j);

					msTriangle_GetVertexIndices(pTri, pVtxs);
					msTriangle_GetNormalIndices(pTri, pNrmls);
					// msTriangle_GetSmoothingGroup(pTri);

					for(UINT k = 0; k < 3; ++k)
					{
						//msTriangleEx_GetTexCoord(pTriEx, k, vTexCoord);

						svc.idVertex = pVtxs[k];
						svc.idNormal = pNrmls[k];
						//svc.vTexCoord = float2_t(vTexCoord[0], vTexCoord[1]);

						if(mVertexCache.KeyExists(svc, &pNode))
						{
							idx = *pNode->Val;
						}
						else
						{
							mVertexCache[svc] = idx = ss.aVerticesStatic.size();

							pVtx = msMesh_GetVertexAt(pMesh, svc.idVertex);
							msVertex_GetVertex(pVtx, vPos);
							ss.aVerticesStatic[idx].Pos = float3_t(vPos[0], vPos[1], -vPos[2]);

							msMesh_GetVertexNormalAt(pMesh, svc.idNormal, vPos);
							ss.aVerticesStatic[idx].Norm = float3_t(vPos[0], vPos[1], -vPos[2]);

							msVertex_GetTexCoords(pVtx, vTexCoord);
							ss.aVerticesStatic[idx].Tex = float2_t(vTexCoord[0], vTexCoord[1]);
						}

						if(k == 2)
						{
							UINT uLI = ss.aIndices[ss.aIndices.size() - 1];
							ss.aIndices[ss.aIndices.size() - 1] = idx;
							idx = uLI;
						}
						ss.aIndices.push_back(idx);
					}
				}
			}
			else
			{
				struct AnimatedVertexCache
				{
					int idVertex;
					int idNormal;
					//float2_t vTexCoord;

					char		nBoneIndices[4];
					byte		nBoneWeights[4];

					bool operator<(const AnimatedVertexCache &other) const
					{
						return(memcmp(this, &other, sizeof(*this)) < 0);
					}
					bool operator==(const AnimatedVertexCache &other) const
					{
						return(memcmp(this, &other, sizeof(*this)) == 0);
					}
				};
				AssotiativeArray<AnimatedVertexCache, UINT> mVertexCache;
				const AssotiativeArray<AnimatedVertexCache, UINT>::Node *pNode;
				AnimatedVertexCache avc = {};
				word pVtxs[3] = {};
				word pNrmls[3] = {};
				msVec2 vTexCoord = {};
				msVec3 vPos = {};
				UINT idx;
				msVertex *pVtx;
				msVertexEx *pVtxEx;
				msTriangle *pTri;
				byte u8WeightSum;
				//msTriangleEx *pTriEx;

				ss.aVerticesAnimated.reserve(iVertexCount);

				for(int j = 0; j < iTriangleCount; ++j)
				{
					pTri = msMesh_GetTriangleAt(pMesh, j);
					//pTriEx = msMesh_GetTriangleExAt(pMesh, j);

					msTriangle_GetVertexIndices(pTri, pVtxs);
					msTriangle_GetNormalIndices(pTri, pNrmls);
					// msTriangle_GetSmoothingGroup(pTri);

					for(UINT k = 0; k < 3; ++k)
					{
						avc.idVertex = pVtxs[k];
						avc.idNormal = pNrmls[k];

						pVtx = msMesh_GetVertexAt(pMesh, avc.idVertex);
						pVtxEx = msMesh_GetVertexExAt(pMesh, avc.idVertex);


						avc.nBoneIndices[0] = msVertex_GetBoneIndex(pVtx);
						u8WeightSum = 0;
						for(UINT s = 0; s < 3; ++s)
						{
							avc.nBoneIndices[s + 1] = msVertexEx_GetBoneIndices(pVtxEx, s);
							avc.nBoneWeights[s] = msVertexEx_GetBoneWeights(pVtxEx, s);
							u8WeightSum += avc.nBoneWeights[s];
						}
						avc.nBoneWeights[3] = u8WeightSum < 100 ? (100 - u8WeightSum) : 0;
						assert(u8WeightSum <= 100);
						//if(avc.nBoneIndices[0] == -1)
						//{
						//	// if the vertex is not attached to any bones, attach it to the root bone
						//	avc.nBoneIndices[0] = 0;
						//}
						if(avc.nBoneIndices[1] == -1)
						{
							// Force verts that are attached to a single bone to have 100% weight
							avc.nBoneWeights[0] = 100;
							avc.nBoneWeights[1] = 0;
							avc.nBoneWeights[2] = 0;
							avc.nBoneWeights[3] = 0;
						}

						if(mVertexCache.KeyExists(avc, &pNode))
						{
							idx = *pNode->Val;
						}
						else
						{
							mVertexCache[avc] = idx = ss.aVerticesAnimated.size();

							msVertex_GetVertex(pVtx, vPos);
							ss.aVerticesAnimated[idx].Pos = float3_t(vPos[0], vPos[1], -vPos[2]);

							msMesh_GetVertexNormalAt(pMesh, avc.idNormal, vPos);
							ss.aVerticesAnimated[idx].Norm = float3_t(vPos[0], vPos[1], -vPos[2]);

							msVertex_GetTexCoords(pVtx, vTexCoord);
							ss.aVerticesAnimated[idx].Tex = float2_t(vTexCoord[0], vTexCoord[1]);

							for(UINT s = 0; s < 4; ++s)
							{
								ss.aVerticesAnimated[idx].BoneIndices[s] = (byte)avc.nBoneIndices[s];
							}
							ss.aVerticesAnimated[idx].BoneWeights.x = (float)avc.nBoneWeights[0] / 100.0f;
							ss.aVerticesAnimated[idx].BoneWeights.y = (float)avc.nBoneWeights[1] / 100.0f;
							ss.aVerticesAnimated[idx].BoneWeights.z = (float)avc.nBoneWeights[2] / 100.0f;
							ss.aVerticesAnimated[idx].BoneWeights.w = (float)avc.nBoneWeights[3] / 100.0f;

						}

						if(k == 2)
						{
							UINT uLI = ss.aIndices[ss.aIndices.size() - 1];
							ss.aIndices[ss.aIndices.size() - 1] = idx;
							idx = uLI;
						}
						ss.aIndices.push_back(idx);
					}
				}
			}

			pProgress->setProgress((float)(i + 1) / (float)iMeshesCount);
		}
	}

	if(!forStaticExport)
	{
		msBone *pBone;
		AssotiativeArray<String, int> mBoneNames;
		int iBones = msModel_GetBoneCount(m_pModel);
		msVec3 v3 = {};
		msVec3 v3p = {};

		for(int i = 0; i < iBones; ++i)
		{
			pBone = msModel_GetBoneAt(m_pModel, i);
			msBone_GetName(pBone, szTmp, sizeof(szTmp));
			mBoneNames[szTmp] = i;
		}
		if(mBoneNames.Size() != iBones)
		{
			// TODO log warning duplicated bone names!
			return(false);
		}
		for(int i = 0; i < iBones; ++i)
		{
			pBone = msModel_GetBoneAt(m_pModel, i);
			Bone &bone = m_aBones[i];

			msBone_GetName(pBone, szTmp, sizeof(szTmp));
			bone.sName = szTmp;

			msBone_GetParentName(pBone, szTmp, sizeof(szTmp));
			bone.iParent = szTmp[0] ? mBoneNames[szTmp] : -1;
		}
		for(int i = 0; i < iBones; ++i)
		{
			pBone = msModel_GetBoneAt(m_pModel, i);
			Bone &bone = m_aBones[i];
			
			msBone_GetPosition(pBone, v3);
			float3 vPos = toPosition(v3);
			msBone_GetRotation(pBone, v3);
			SMQuaternion qRot = toRotation(v3);

			float3 vOrig = vPos;
			SMQuaternion qOrig = qRot;
			
			int pid = bone.iParent;
			assert(pid < i);

			if(pid >= 0)
			{
				bone.vPos = m_aBones[pid].qRot * vPos + m_aBones[pid].vPos;
				bone.qRot = qRot * m_aBones[pid].qRot;
			}
			else
			{
				bone.vPos = vPos;
				bone.qRot = qRot;
			}
		}

		// why do we really need this!? Incompatible with 3ds max files otherway
		for(int i = 0; i < iBones; ++i)
		{
			Bone &bone = m_aBones[i];
		
			float3 vAxis = bone.qRot * float3(1.0f, 0.0f, 0.0f);
			SMQuaternion qDelta(vAxis, SM_PIDIV2);

			bone.qRot = bone.qRot * qDelta;
		}

		for(int i = iBones - 1; i >= 0; --i)
		{
			Bone &bone = m_aBones[i];
			if(bone.iParent >= 0)
			{
				const Bone &parent = m_aBones[bone.iParent];

				bone.qRot = bone.qRot * parent.qRot.Conjugate();
				bone.vPos = parent.qRot.Conjugate() * (bone.vPos - parent.vPos);
			}
		}
	}

	return(true);
}

bool CProvider::preapareAnimationTrack(IProgress *pProgress, UINT uStartFrame, UINT uFrameCount)
{
	m_aaAnimationTrack.resize(uFrameCount);
	for(UINT j = 0; j < uFrameCount; ++j)
	{
		m_aaAnimationTrack[j].resize(m_aBones.size());

		for(UINT i = 0, l = m_aBones.size(); i < l; ++i)
		{
			int pid = m_aBones[i].iParent;
			assert(pid < (int)i);

			TrackBone &bone = m_aaAnimationTrack[j][i];

			bone.vPos = getBonePosForFrame(i, j + uStartFrame);
			bone.qRot = getBoneRotForFrame(i, j + uStartFrame);

			if(pid >= 0)
			{
				TrackBone &parent = m_aaAnimationTrack[j][pid];

				bone.vPos = parent.qRot * bone.vPos + parent.vPos;
				bone.qRot = bone.qRot * parent.qRot;
			}
		}

		for(UINT i = 0, l = m_aBones.size(); i < l; ++i)
		{
			TrackBone &bone = m_aaAnimationTrack[j][i];

			float3 vAxis = bone.qRot * float3(1.0f, 0.0f, 0.0f);
			SMQuaternion qDelta(vAxis, SM_PIDIV2);

			bone.qRot = bone.qRot * qDelta;
		}

		for(int i = m_aBones.size() - 1; i >= 0; --i)
		{
			TrackBone &bone = m_aaAnimationTrack[j][i];
			if(m_aBones[i].iParent >= 0)
			{
				const TrackBone &parent = m_aaAnimationTrack[j][m_aBones[i].iParent];

				bone.qRot = bone.qRot * parent.qRot.Conjugate();
				bone.vPos = parent.qRot.Conjugate() * (bone.vPos - parent.vPos);
			}
		}

		pProgress->setProgress((float)(j + 1) / (float)uFrameCount);
	}
	return(true);
}


UINT CProvider::getLayerCount()
{
	return(m_aLayers.size());
}
const char* CProvider::getLayerName(UINT uLayer)
{
	assert(uLayer < m_aLayers.size());
	return(m_aLayers[uLayer].sName.c_str());
}
UINT CProvider::getLayerObjectCount(UINT uLayer)
{
	assert(uLayer < m_aLayers.size());

	return(m_aLayers[uLayer].aObjects.size());
}

float3 CProvider::getObjectPosition(UINT uLayer, UINT uObject)
{
	assert(uLayer < m_aLayers.size());
	assert(uObject < m_aLayers[uLayer].aObjects.size());

	return(m_aLayers[uLayer].aObjects[uObject].vPosition);
}
SMQuaternion CProvider::getObjectRotation(UINT uLayer, UINT uObject)
{
	assert(uLayer < m_aLayers.size());
	assert(uObject < m_aLayers[uLayer].aObjects.size());

	return(m_aLayers[uLayer].aObjects[uObject].qRotation);
}

UINT CProvider::getObjectSubsetCount(UINT uLayer, UINT uObject)
{
	assert(uLayer < m_aLayers.size());
	assert(uObject < m_aLayers[uLayer].aObjects.size());

	return(m_aLayers[uLayer].aObjects[uObject].aSubsets.size());
}
const char* CProvider::getObjectName(UINT uLayer, UINT uObject)
{
	assert(uLayer < m_aLayers.size());
	assert(uObject < m_aLayers[uLayer].aObjects.size());

	return(m_aLayers[uLayer].aObjects[uObject].sName.c_str());
}
const char* CProvider::getObjectSubsetTexture(UINT uLayer, UINT uObject, UINT uSubset)
{
	assert(uLayer < m_aLayers.size());
	assert(uObject < m_aLayers[uLayer].aObjects.size());
	assert(uSubset < m_aLayers[uLayer].aObjects[uObject].aSubsets.size());

	return(m_aLayers[uLayer].aObjects[uObject].aSubsets[uSubset].sTexture.c_str());
}

UINT CProvider::getObjectSubsetVertexCount(UINT uLayer, UINT uObject, UINT uSubset)
{
	assert(uLayer < m_aLayers.size());
	assert(uObject < m_aLayers[uLayer].aObjects.size());
	assert(uSubset < m_aLayers[uLayer].aObjects[uObject].aSubsets.size());

	if(m_isStatic)
	{
		return(m_aLayers[uLayer].aObjects[uObject].aSubsets[uSubset].aVerticesStatic.size());
	}
	return(m_aLayers[uLayer].aObjects[uObject].aSubsets[uSubset].aVerticesAnimated.size());
}
UINT CProvider::getObjectSubsetIndexCount(UINT uLayer, UINT uObject, UINT uSubset)
{
	assert(uLayer < m_aLayers.size());
	assert(uObject < m_aLayers[uLayer].aObjects.size());
	assert(uSubset < m_aLayers[uLayer].aObjects[uObject].aSubsets.size());

	return(m_aLayers[uLayer].aObjects[uObject].aSubsets[uSubset].aIndices.size());
}

UINT* CProvider::getObjectSubsetIndices(UINT uLayer, UINT uObject, UINT uSubset)
{
	assert(uLayer < m_aLayers.size());
	assert(uObject < m_aLayers[uLayer].aObjects.size());
	assert(uSubset < m_aLayers[uLayer].aObjects[uObject].aSubsets.size());

	return(m_aLayers[uLayer].aObjects[uObject].aSubsets[uSubset].aIndices);
}

vertex_static* CProvider::getObjectSubsetStaticVertices(UINT uLayer, UINT uObject, UINT uSubset)
{
	assert(uLayer < m_aLayers.size());
	assert(uObject < m_aLayers[uLayer].aObjects.size());
	assert(uSubset < m_aLayers[uLayer].aObjects[uObject].aSubsets.size());

	return(m_aLayers[uLayer].aObjects[uObject].aSubsets[uSubset].aVerticesStatic);
}
vertex_static_ex* CProvider::getObjectSubsetStaticExVertices(UINT uLayer, UINT uObject, UINT uSubset)
{
	assert(!"Not implemented!");
	return(NULL);
}
vertex_animated* CProvider::getObjectSubsetAnimatedVertices(UINT uLayer, UINT uObject, UINT uSubset)
{
	assert(uLayer < m_aLayers.size());
	assert(uObject < m_aLayers[uLayer].aObjects.size());
	assert(uSubset < m_aLayers[uLayer].aObjects[uObject].aSubsets.size());

	return(m_aLayers[uLayer].aObjects[uObject].aSubsets[uSubset].aVerticesAnimated);
}
vertex_animated_ex* CProvider::getObjectSubsetAnimatedExVertices(UINT uLayer, UINT uObject, UINT uSubset)
{
	assert(!"Not implemented!");
	return(NULL);
}

bool CProvider::hasBones()
{
	return(msModel_GetBoneCount(m_pModel) > 0);
}

UINT CProvider::getBonesCount()
{
	return(m_aBones.size());
}
const char* CProvider::getBoneName(UINT uBone)
{
	assert(uBone < m_aBones.size());

	return(m_aBones[uBone].sName.c_str());
}
int CProvider::getBoneParent(UINT uBone)
{
	assert(uBone < m_aBones.size());
	
	return(m_aBones[uBone].iParent);
}
float3 CProvider::getBoneLocalPos(UINT uBone)
{
	assert(uBone < m_aBones.size());

	return(m_aBones[uBone].vPos);
}
SMQuaternion CProvider::getBoneLocalRot(UINT uBone)
{
	assert(uBone < m_aBones.size());

	return(m_aBones[uBone].qRot);
}

UINT CProvider::getBonePositionKeysCount(UINT uBone)
{
	assert(uBone < m_aBones.size());

	msBone *pBone = msModel_GetBoneAt(m_pModel, uBone);
	return(msBone_GetPositionKeyCount(pBone));
}
UINT CProvider::getBoneRotationKeysCount(UINT uBone)
{
	assert(uBone < m_aBones.size());

	msBone *pBone = msModel_GetBoneAt(m_pModel, uBone);
	return(msBone_GetRotationKeyCount(pBone));
}
float3 CProvider::getBonePositionKey(UINT uBone, UINT uKey, float *pfTime)
{
	assert(uBone < m_aBones.size());

	msBone *pBone = msModel_GetBoneAt(m_pModel, uBone);

	msPositionKey *pKey = msBone_GetPositionKeyAt(pBone, uKey);
	if(pfTime)
	{
		*pfTime = pKey->fTime - 1.0f;
	}
	return(toPosition(pKey->Position));
}
SMQuaternion CProvider::getBoneRotationKey(UINT uBone, UINT uKey, float *pfTime)
{
	assert(uBone < m_aBones.size());

	msBone *pBone = msModel_GetBoneAt(m_pModel, uBone);

	msRotationKey *pKey = msBone_GetRotationKeyAt(pBone, uKey);
	if(pfTime)
	{
		*pfTime = pKey->fTime - 1.0f;
	}
	return(toRotation(pKey->Rotation));
}

float3 CProvider::toPosition(msVec3 vec)
{
	return(float3(vec[0], vec[1], -vec[2]));
}
SMQuaternion CProvider::toRotation(msVec3 vec)
{
	return(SMQuaternion(float3(1.0f, 0.0f, 0.0f), vec[0]) * SMQuaternion(float3(0.0f, 1.0f, 0.0f), vec[1]) * SMQuaternion(float3(0.0f, 0.0f, 1.0f), -vec[2]));
}


float3 CProvider::getBonePosForFrame(UINT uBone, int iFrame)
{
	msBone *pBone = msModel_GetBoneAt(m_pModel, uBone);

	msVec3 v3 = {};
	msBone_GetPosition(pBone, v3);
	float3 vInitialPos = toPosition(v3);

	msBone_GetRotation(pBone, v3);
	SMQuaternion qInitialRotation = toRotation(v3);

	UINT uKeys = getBonePositionKeysCount(uBone);
	if(/*uFrame && */uKeys)
	{
		int frame1 = -1;
		int frame2 = uKeys - 1;
		float fTime = 0.0f, fTime2 = 0.0f;
		for(UINT i = 0; i < uKeys; i++)
		{
			getBonePositionKey(uBone, i, &fTime);
			int keyframe = (int)fTime;

			if(keyframe <= iFrame)
				frame1 = i;

			if(keyframe >= iFrame)
			{
				if(frame1 == -1)
				{
					frame1 = i;
				}
				frame2 = i;
				break;
			}
		}

		float3 vPos1 = getBonePositionKey(uBone, frame1, &fTime);
		float3 vPos2 = getBonePositionKey(uBone, frame2, &fTime2);

		int time1 = (iFrame < (int)fTime) ? iFrame : (int)fTime;
		int time2 = (iFrame > (int)fTime2) ? iFrame : (int)fTime2;
		if(time1 == time2)
		{
			return(vInitialPos + qInitialRotation * vPos1);
		}
		return(vInitialPos + qInitialRotation * vlerp(vPos1, vPos2, (float)(iFrame - time1) / (float)(time2 - time1)));
	}

	return(vInitialPos);
}

SMQuaternion CProvider::getBoneRotForFrame(UINT uBone, int iFrame)
{
	msBone *pBone = msModel_GetBoneAt(m_pModel, uBone);

	msVec3 v3 = {};
	msBone_GetRotation(pBone, v3);
	SMQuaternion qInitialRotation = toRotation(v3);
	
	UINT uKeys = getBoneRotationKeysCount(uBone);
	if(/*uFrame && */uKeys)
	{
		int frame1 = -1;
		int frame2 = uKeys - 1;
		float fTime = 0.0f, fTime2 = 0.0f;
		for(UINT i = 0; i < uKeys; i++)
		{
			getBoneRotationKey(uBone, i, &fTime);
			int keyframe = (int)fTime;

			if(keyframe <= iFrame)
				frame1 = i;

			if(keyframe >= iFrame)
			{
				if(frame1 == -1)
				{
					frame1 = i;
				}
				frame2 = i;
				break;
			}
		}

		SMQuaternion qRot1 = getBoneRotationKey(uBone, frame1, &fTime);
		SMQuaternion qRot2 = getBoneRotationKey(uBone, frame2, &fTime2);
		
		int time1 = (iFrame < (int)fTime) ? iFrame : (int)fTime;
		int time2 = (iFrame >(int)fTime2) ? iFrame : (int)fTime2;
		if(time1 == time2)
		{
			return(qRot1 * qInitialRotation);
		}
		return(SMquaternionSlerp(qRot1, qRot2, (float)(iFrame - time1) / (float)(time2 - time1)) * qInitialRotation);
	}
	
	return(qInitialRotation);
}

float3 CProvider::getBonePositionAtFrame(UINT uBone, UINT uFrame)
{
	assert(uFrame < m_aaAnimationTrack.size() && uBone < m_aaAnimationTrack[uFrame].size());

	return(m_aaAnimationTrack[uFrame][uBone].vPos);
}
SMQuaternion CProvider::getBoneRotationAtFrame(UINT uBone, UINT uFrame)
{
	assert(uFrame < m_aaAnimationTrack.size() && uBone < m_aaAnimationTrack[uFrame].size());

	return(m_aaAnimationTrack[uFrame][uBone].qRot);
}

bool CProvider::getConfigBool(const char *szKey, bool bDefault)
{
	return(getConfigBool(m_pModel, szKey, bDefault));
}
void CProvider::setConfigBool(const char *szKey, bool bvalue)
{
	setConfigBool(m_pModel, szKey, bvalue);
}
float CProvider::getConfigFloat(const char *szKey, float fDefault)
{
	return(getConfigFloat(m_pModel, szKey, fDefault));
}
void CProvider::setConfigFloat(const char *szKey, float fvalue)
{
	setConfigFloat(m_pModel, szKey, fvalue);
}
int CProvider::getConfigInt(const char *szKey, int iDefault)
{
	return(getConfigInt(m_pModel, szKey, iDefault));
}
void CProvider::setConfigInt(const char *szKey, int ivalue)
{
	setConfigInt(m_pModel, szKey, ivalue);
}
void CProvider::getConfigStr(const char *szKey, char *szOut, int iMaxOut)
{
	getConfigStr(m_pModel, szKey, szOut, iMaxOut);
}
void CProvider::setConfigStr(const char *szKey, const char *szValue)
{
	setConfigStr(m_pModel, szKey, szValue);
}