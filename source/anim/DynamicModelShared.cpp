#include "DynamicModelShared.h"
#include "DynamicModelProvider.h"
#include <mtrl/IXMaterialSystem.h>

CDynamicModelShared::CDynamicModelShared(CDynamicModelProvider *pProvider):
	m_pProvider(pProvider),
	m_pDevice(pProvider->getDevice()),
	m_pMaterialSystem(pProvider->getMaterialSystem())
{
}
CDynamicModelShared::~CDynamicModelShared()
{
	m_pProvider->onSharedModelRelease(this);
	
	for(UINT i = 0; i < m_uSkinCount; ++i)
	{
		for(UINT j = 0; j < m_uMaterialCount; ++j)
		{
			mem_release(m_pppMaterials[i][j]);
		}
	}

	mem_delete_a(m_ppMaterialsBlob);
	m_pppMaterials = NULL;

	mem_delete_a(m_bmFeatures);

	if(m_pDevice)
	{
		for(UINT i = 0, l = m_aLods.size(); i < l; ++i)
		{
			mem_release(m_ppRenderBuffer[i]);
			mem_release(m_ppIndexBuffer[i]);
		}
		mem_delete_a(m_ppRenderBuffer);
		mem_delete_a(m_ppIndexBuffer);
	}
	
	mem_release(m_pResource);

	if(m_ppTempIndices)
	{
		for(UINT i = 0, l = m_aLods.size(); i < l; ++i)
		{
			mem_delete_a(m_ppTempIndices[i]);
			mem_delete_a(m_ppTempVertices[i]);
		}
		mem_delete_a(m_ppTempIndices);
		mem_delete_a(m_ppTempVertices);
		mem_delete_a(m_puTempTotalIndices);
		mem_delete_a(m_puTempTotalVertices);
	}

	mem_release(m_pInstanceBuffer);
	mem_release(m_pInstanceColorsBuffer);
}
void CDynamicModelShared::AddRef()
{
	++m_uRefCount;
}
void CDynamicModelShared::Release()
{
	if(!--m_uRefCount)
	{
		delete this;
	}
}

bool CDynamicModelShared::init(IXResourceModelStatic *pResource)
{
	m_pResource = pResource;
	pResource->AddRef();

	m_topology = pResource->getPrimitiveTopology();

	{
		m_uMaterialCount = pResource->getMaterialCount();
		m_uSkinCount = pResource->getSkinCount();

		if(!m_uMaterialCount || !m_uSkinCount || !pResource->getLodCount() || !pResource->getSubsetCount(0))
		{
			return(false);
		}

		m_bmFeatures = new XMODEL_FEATURE[m_uSkinCount];

		m_ppMaterialsBlob = new void*[m_uMaterialCount * m_uSkinCount + m_uSkinCount];
		m_pppMaterials = (IXMaterial***)m_ppMaterialsBlob;
		
		const char *szMaterial;
		for(UINT i = 0; i < m_uSkinCount; ++i)
		{
			m_pppMaterials[i] = (IXMaterial**)(m_ppMaterialsBlob + m_uSkinCount + m_uMaterialCount * i);
			XMODEL_FEATURE bmFeatures = MF_NONE;

			for(UINT j = 0; j < m_uMaterialCount; ++j)
			{
				szMaterial = m_pResource->getMaterial(j, i);
				if(szMaterial && szMaterial[0])
				{
					m_pMaterialSystem->loadMaterial(szMaterial, &m_pppMaterials[i][j]);
				}
				else if(i == 0)
				{
					m_pppMaterials[i][j] = NULL;
				}
				else
				{
					m_pppMaterials[i][j] = m_pppMaterials[0][j];
					m_pppMaterials[i][j]->AddRef();
				}

				if(m_pppMaterials[i][j])
				{
					if(m_pppMaterials[i][j]->isTransparent())
					{
						bmFeatures |= MF_TRANSPARENT;
					}
					else
					{
						bmFeatures |= MF_OPAQUE;
					}
					if(m_pppMaterials[i][j]->isEmissive())
					{
						bmFeatures |= MF_SELFILLUM;
					}
				}
			}
			
			m_bmFeatures[i] = bmFeatures;
		}
	}

	{
		bool useDelayInit = false;

		UINT uLodCount = m_pResource->getLodCount();
		m_aLods.resizeFast(uLodCount);
		if(m_pDevice)
		{
			m_ppRenderBuffer = new IGXRenderBuffer*[uLodCount];
			m_ppIndexBuffer = new IGXIndexBuffer*[uLodCount];

			if(!m_pProvider->getCore()->isOnMainThread())
			{
				m_ppTempIndices = new UINT*[uLodCount];
				m_ppTempIndices16 = new USHORT*[uLodCount];
				m_ppTempVertices = new XResourceModelStaticVertexGPU*[uLodCount];
				m_puTempTotalIndices = new UINT[uLodCount];
				m_puTempTotalVertices = new UINT[uLodCount];
			}

			bool isMinMaxSet = false;
			float3 vLocalMin, vLocalMax;
			for(UINT i = 0; i < uLodCount; ++i)
			{
				UINT uTotalIndices = 0;
				UINT uTotalVertices = 0;

				GXINDEXTYPE indexType = GXIT_UINT32;

				for(UINT j = 0, jl = m_pResource->getSubsetCount(i); j < jl; ++j)
				{
					auto pSubset = m_pResource->getSubset(i, j);
					uTotalIndices += pSubset->iIndexCount;
					uTotalVertices += pSubset->iVertexCount;
				}

				if(uTotalVertices <= USHRT_MAX)
				{
					indexType = GXIT_UINT16;
				}

				XResourceModelStaticVertexGPU *pVertices = new XResourceModelStaticVertexGPU[uTotalVertices];
				UINT *pIndices = NULL;
				USHORT *pIndices16 = NULL;
				if(indexType == GXIT_UINT16)
				{
					pIndices16 = new USHORT[uTotalIndices];
				}
				else
				{
					pIndices = new UINT[uTotalIndices];
				}

				subset_t subset;
				for(UINT uMaterial = 0; uMaterial < m_uMaterialCount; ++uMaterial)
				{
					for(UINT j = 0, jl = m_pResource->getSubsetCount(i); j < jl; ++j)
					{
						auto pSubset = m_pResource->getSubset(i, j);
						if(pSubset->iMaterialID == uMaterial)
						{
							subset.uStartIndex += subset.uIndexCount;
							subset.uStartVertex += subset.uVertexCount;

							subset.uIndexCount = pSubset->iIndexCount;
							subset.uVertexCount = pSubset->iVertexCount;

							//memcpy(pVertices + subset.uStartVertex, pSubset->pVertices, sizeof(XResourceModelStaticVertex) * pSubset->iVertexCount);
							for(UINT k = 0; k < pSubset->iVertexCount; ++k)
							{
#define TO_SHORT(v) ((short)((v) * 32767.0f))
								auto &dst = (pVertices + subset.uStartVertex)[k];
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
#undef TO_SHORT
							}
							if(indexType == GXIT_UINT16)
							{
								for(UINT k = 0; k < pSubset->iIndexCount; ++k)
								{
									(pIndices16 + subset.uStartIndex)[k] = pSubset->pIndices[k];
								}
							}
							else
							{
								memcpy(pIndices + subset.uStartIndex, pSubset->pIndices, sizeof(UINT) * pSubset->iIndexCount);
							}

							m_aLods[i][uMaterial] = subset;

							if(i == 0)
							{
								for(UINT k = 0; k < pSubset->iVertexCount; ++k)
								{
									if(isMinMaxSet)
									{
										vLocalMin = SMVectorMin(vLocalMin, pSubset->pVertices[k].vPos);
										vLocalMax = SMVectorMax(vLocalMax, pSubset->pVertices[k].vPos);
									}
									else
									{
										vLocalMin = vLocalMax = pSubset->pVertices[k].vPos;
										isMinMaxSet = true;
									}
								}
							}

							break;
						}
					}
				}

				if(m_pProvider->getCore()->isOnMainThread())
				{
					m_ppIndexBuffer[i] = m_pDevice->createIndexBuffer((indexType == GXIT_UINT16 ? sizeof(USHORT) : sizeof(UINT)) * uTotalIndices, GXBUFFER_USAGE_STATIC, indexType, indexType == GXIT_UINT16 ? (void*)pIndices16 : (void*)pIndices);
					IGXVertexBuffer *pVertexBuffer = m_pDevice->createVertexBuffer(sizeof(XResourceModelStaticVertexGPU) * uTotalVertices, GXBUFFER_USAGE_STATIC, pVertices);
					m_ppRenderBuffer[i] = m_pDevice->createRenderBuffer(1, &pVertexBuffer, m_pProvider->getVertexDeclaration());
					mem_release(pVertexBuffer);

					mem_delete_a(pIndices);
					mem_delete_a(pVertices);

					if(!m_pInstanceBuffer)
					{
						m_pInstanceBuffer = m_pDevice->createConstantBuffer(sizeof(m_instanceData), true);
						m_pInstanceColorsBuffer = m_pDevice->createConstantBuffer(sizeof(m_instanceColors), true);
					}
				}
				else
				{
					m_ppIndexBuffer[i] = NULL;
					m_ppRenderBuffer[i] = NULL;

					m_ppTempIndices[i] = pIndices;
					m_ppTempIndices16[i] = pIndices16;
					m_ppTempVertices[i] = pVertices;
					m_puTempTotalIndices[i] = uTotalIndices;
					m_puTempTotalVertices[i] = uTotalVertices;

					useDelayInit = true;
				}

			}

			m_vLocalMin = vLocalMin;
			m_vLocalMax = vLocalMax;

			//printf("Min: %.2f, %.2f, %.2f; Max: %.2f, %.2f, %.2f\n", m_vLocalMin.x, m_vLocalMin.y, m_vLocalMin.z, m_vLocalMax.x, m_vLocalMax.y, m_vLocalMax.z);
		}


		if(useDelayInit)
		{
			m_pProvider->scheduleSharedGPUinit(this);
		}
	}

	for(UINT m = 0, ml = pResource->getPhysboxCount(); m < ml; ++m)
	{
		m_apPhysboxes.push_back(pResource->getPhysbox(m));
	}

	buildPSPs();
	

	return(true);
}

void CDynamicModelShared::onMaterialTransparencyChanged(const IXMaterial *pMaterial)
{
	bool isChanged = false;
	for(UINT i = 0; i < m_uSkinCount; ++i)
	{
		bool isTransparent = false;
		bool isOpaque = false;

		for(UINT j = 0; j < m_uMaterialCount; ++j)
		{
			if(m_pppMaterials[i][j])
			{
				if(m_pppMaterials[i][j]->isTransparent())
				{
					isTransparent = true;
				}
				else
				{
					isOpaque = true;
				}

				if(m_pppMaterials[i][j] == pMaterial)
				{
					isChanged = true;
				}
			}
		}

		if(isTransparent)
		{
			m_bmFeatures[i] |= MF_TRANSPARENT;
		}
		else
		{
			m_bmFeatures[i] &= ~MF_TRANSPARENT;
		}

		if(isOpaque)
		{
			m_bmFeatures[i] |= MF_OPAQUE;
		}
		else
		{
			m_bmFeatures[i] &= ~MF_OPAQUE;
		}
	}

	if(isChanged)
	{
		buildPSPs();

		m_pProvider->onSharedModelFeaturesChanged(this);
	}
}

void CDynamicModelShared::onMaterialEmissivityChanged(const IXMaterial *pMaterial)
{
	bool isChanged = false;

	for(UINT i = 0; i < m_uSkinCount; ++i)
	{
		bool isEmissive = false;

		for(UINT j = 0; j < m_uMaterialCount; ++j)
		{
			if(m_pppMaterials[i][j] && m_pppMaterials[i][j]->isEmissive())
			{
				isEmissive = true;
			}

			if(m_pppMaterials[i][j] == pMaterial)
			{
				isChanged = true;
			}
		}

		if(isEmissive)
		{
			m_bmFeatures[i] |= MF_SELFILLUM;
		}
		else
		{
			m_bmFeatures[i] &= ~MF_SELFILLUM;
		}
	}

	if(isChanged)
	{
		m_pProvider->onSharedModelFeaturesChanged(this);
	}
}

void CDynamicModelShared::initGPUresources()
{
	if(!m_ppTempIndices)
	{
		return;
	}
	assert(m_pProvider->getCore()->isOnMainThread());

	for(UINT i = 0, l = m_aLods.size(); i < l; ++i)
	{
		m_ppIndexBuffer[i] = m_pDevice->createIndexBuffer((m_ppTempIndices[i] ? sizeof(UINT) : sizeof(USHORT)) * m_puTempTotalIndices[i], GXBUFFER_USAGE_STATIC, m_ppTempIndices[i] ? GXIT_UINT32 : GXIT_UINT16, m_ppTempIndices[i] ? (void*)m_ppTempIndices[i] : (void*)m_ppTempIndices16[i]);
		IGXVertexBuffer *pVertexBuffer = m_pDevice->createVertexBuffer(sizeof(XResourceModelStaticVertexGPU) * m_puTempTotalVertices[i], GXBUFFER_USAGE_STATIC, m_ppTempVertices[i]);
		m_ppRenderBuffer[i] = m_pDevice->createRenderBuffer(1, &pVertexBuffer, m_pProvider->getVertexDeclaration());
		mem_release(pVertexBuffer);

		mem_delete_a(m_ppTempIndices[i]);
		mem_delete_a(m_ppTempIndices16[i]);
		mem_delete_a(m_ppTempVertices[i]);
	}
	mem_delete_a(m_ppTempIndices);
	mem_delete_a(m_ppTempIndices16);
	mem_delete_a(m_ppTempVertices);
	mem_delete_a(m_puTempTotalIndices);
	mem_delete_a(m_puTempTotalVertices);

	m_pInstanceBuffer = m_pDevice->createConstantBuffer(sizeof(m_instanceData), true);
	m_pInstanceColorsBuffer = m_pDevice->createConstantBuffer(sizeof(m_instanceColors), true);
}

IXResourceModelStatic *CDynamicModelShared::getResource()
{
	return(m_pResource);
}


UINT CDynamicModelShared::getPhysboxCount() const
{
	return(m_apPhysboxes.size());
}
bool CDynamicModelShared::isReady() const
{
	return(m_ppTempIndices != NULL);
}
const IModelPhysbox *CDynamicModelShared::getPhysBox(UINT id) const
{
	assert(id < m_apPhysboxes.size());

	return(m_apPhysboxes[id]);
}

float3 CDynamicModelShared::getLocalBoundMin() const
{
	return(m_vLocalMin);
}
float3 CDynamicModelShared::getLocalBoundMax() const
{
	return(m_vLocalMax);
}
SMAABB CDynamicModelShared::getLocalBound() const
{
	return(SMAABB(m_vLocalMin, m_vLocalMax));
}

void CDynamicModelShared::beginInstancing(UINT uSkin, UINT uLod, XMODEL_FEATURE bmWhat)
{
	assert(!m_isInstancingEnabled);
	m_isInstancingEnabled = true;
	m_iInstanceCount = 0;
	m_uInstancingSkin = uSkin;
	m_uInstancingLod = uLod;
	m_bmInstancingFeatures = bmWhat;
}
void CDynamicModelShared::endInstancing()
{
	assert(m_isInstancingEnabled);

	if(m_iInstanceCount && m_pInstanceBuffer)
	{
		m_pInstanceBuffer->update(m_instanceData, sizeof(m_instanceData[0]) * m_iInstanceCount);
		m_pInstanceColorsBuffer->update(m_instanceColors, sizeof(m_instanceColors[0]) * m_iInstanceCount);

		m_pProvider->bindVertexFormat(true);
		render(m_uInstancingSkin, m_uInstancingLod, m_bmInstancingFeatures);
	}

	m_isInstancingEnabled = false;
}

void CDynamicModelShared::renderInstanced(const float3 &vPos, const SMQuaternion &qRot, float fScale, const float4_t &vColor)
{
	assert(m_isInstancingEnabled);

	m_instanceData[m_iInstanceCount].vPosScale = float4(vPos, fScale);
	m_instanceData[m_iInstanceCount].qRot = qRot;
	m_instanceColors[m_iInstanceCount].vColor = vColor;
	
	if(++m_iInstanceCount == MAX_INSTANCES)
	{
		endInstancing();
		beginInstancing(m_uInstancingSkin, m_uInstancingLod, m_bmInstancingFeatures);
	}
}

bool CDynamicModelShared::isInstancing()
{
	return(m_isInstancingEnabled);
}

void CDynamicModelShared::render(UINT uSkin, UINT uLod, XMODEL_FEATURE bmFeatures)
{

	//m_instanceData[MAX_INSTANCES];
	//IGXConstantBuffer *m_pInstanceBuffer = NULL;
	//int m_iInstanceCount = 0;
	//m_isInstancingEnabled


	if(!m_pDevice)
	{
		return;
	}

	if(uSkin >= m_uSkinCount)
	{
		uSkin = 0;
	}

	if((getFeatures(uSkin, uLod) & bmFeatures) == 0)
	{
		return;
	}
	
	if(uLod >= m_aLods.size())
	{
		return;
	}

	if(!m_ppIndexBuffer[uLod] || !m_ppRenderBuffer[uLod])
	{
		return;
	}

	IGXContext *pCtx = m_pDevice->getThreadContext();

	if(m_isInstancingEnabled)
	{
		pCtx->setVSConstant(m_pInstanceBuffer, 1 /* SCR_OBJECT */);
		pCtx->setPSConstant(m_pInstanceColorsBuffer, 8);
	}

	pCtx->setIndexBuffer(m_ppIndexBuffer[uLod]);
	pCtx->setRenderBuffer(m_ppRenderBuffer[uLod]);

	switch(m_topology)
	{
	case XPT_TRIANGLELIST:
		pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);
		break;
	case XPT_TRIANGLESTRIP:
		pCtx->setPrimitiveTopology(GXPT_TRIANGLESTRIP);
		break;
	}


	UINT uPrimCount = 0;
	subset_t *pSubset;
	for(UINT i = 0; i < m_uMaterialCount; ++i)
	{
		pSubset = &m_aLods[uLod][i];
		auto *pMat = m_pppMaterials[uSkin][i];
		if(pSubset->uIndexCount != 0 
			&& pMat
			&& (
				((bmFeatures & MF_OPAQUE) && !pMat->isTransparent())
				|| ((bmFeatures & MF_TRANSPARENT) && pMat->isTransparent())
				|| ((bmFeatures & MF_SELFILLUM) && pMat->isEmissive())
			)
		)
		{
			if(m_pMaterialSystem->bindMaterial(m_pppMaterials[uSkin][i]))
			{
				switch(m_topology)
				{
				case XPT_TRIANGLELIST:
					uPrimCount = pSubset->uIndexCount / 3;
					break;
				case XPT_TRIANGLESTRIP:
					uPrimCount = pSubset->uIndexCount - 2;
					break;
				}

				if(m_isInstancingEnabled)
				{
					pCtx->drawIndexedInstanced(m_iInstanceCount, pSubset->uVertexCount, uPrimCount, pSubset->uStartIndex, pSubset->uStartVertex);
				}
				else
				{
					pCtx->drawIndexed(pSubset->uVertexCount, uPrimCount, pSubset->uStartIndex, pSubset->uStartVertex);
				}
			}
		}
	}
}

XMODEL_FEATURE CDynamicModelShared::getFeatures(UINT uSkin, UINT uLod)
{
	assert(uSkin < m_uSkinCount);
	if(uSkin >= m_uSkinCount)
	{
		return(MF_NONE);
	}
	//! @todo add uLod support

	return(m_bmFeatures[uSkin]);
}

IXMaterial* CDynamicModelShared::getTransparentMaterial(UINT uSkin, UINT uLod)
{
	assert(uSkin < m_uSkinCount);
	if(uSkin >= m_uSkinCount)
	{
		return(NULL);
	}

	assert(uLod < m_aLods.size());
	if(uLod >= m_aLods.size())
	{
		return(NULL);
	}

	for(UINT j = 0, jl = m_pResource->getSubsetCount(uLod); j < jl; ++j)
	{
		auto pSubset = m_pResource->getSubset(uLod, j);
		IXMaterial *pMaterial = m_pppMaterials[uSkin][pSubset->iMaterialID];

		if(pMaterial && pMaterial->isTransparent())
		{
			pMaterial->AddRef();
			return(pMaterial);
		}
	}
	return(NULL);
}

float3 CDynamicModelShared::getTransparentBoundMin(UINT uSkin, UINT uLod) const
{
	//! @todo Implement me!
	return(m_vLocalMin);
}
float3 CDynamicModelShared::getTransparentBoundMax(UINT uSkin, UINT uLod) const
{
	//! @todo Implement me!
	return(m_vLocalMax);
}

void CDynamicModelShared::buildPSPs()
{
	assert(m_topology == XPT_TRIANGLELIST);
	if(m_topology != XPT_TRIANGLELIST)
	{
		return;
	}

	const float c_fEpsilon = 0.01f;

	m_aPSPs.reserve(m_aLods.size());
	for(UINT i = 0, uLodCount = m_aLods.size(); i < uLodCount; ++i)
	{
		m_aPSPs[i].reserve(m_uSkinCount);
		for(UINT j = 0, jl = m_pResource->getSubsetCount(i); j < jl; ++j)
		{
			auto pSubset = m_pResource->getSubset(i, j);

			for(UINT k = 0; k < m_uSkinCount; ++k)
			{
				if(j == 0)
				{
					m_aPSPs[i][k].clearFast();
				}
				IXMaterial *pMaterial = m_pppMaterials[k][pSubset->iMaterialID];
				if(pMaterial && pMaterial->isTransparent())
				{
					// Если сабсет достаточно плоский, он может породить PSP
					UINT uTotalFaces = pSubset->iIndexCount / 3;
					float3 vFirstNormal, vNormal;
					bool isGood = true;
					float3 a, b, c;
					for(UINT uFace = 0; uFace < uTotalFaces; ++uFace)
					{
						a = pSubset->pVertices[pSubset->pIndices[uFace * 3]].vPos;
						b = pSubset->pVertices[pSubset->pIndices[uFace * 3 + 1]].vPos;
						c = pSubset->pVertices[pSubset->pIndices[uFace * 3 + 2]].vPos;
						vNormal = SMVector3Normalize(SMVector3Cross(b - a, c - a));

						if(uFace == 0)
						{
							vFirstNormal = vNormal;
						}
						else
						{
							if(1.0f - fabsf(SMVector3Dot(vNormal, vFirstNormal)) > c_fEpsilon)
							{
								isGood = false;
								break;
							}
						}
					}

					//! @fixme Здесь не учитывается расстояние между полигонами

					if(isGood)
					{
						m_aPSPs[i][k].push_back(SMPLANE(vNormal, a));
					}
				}
			}
		}
	}
}

const Array<float4_t>& CDynamicModelShared::getPSPs(UINT uSkin, UINT uLod) const
{
	assert(uSkin < m_uSkinCount);
	assert(uLod < m_aLods.size());

	return(m_aPSPs[uLod][uSkin]);
}
