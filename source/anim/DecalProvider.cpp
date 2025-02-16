#include "DecalProvider.h"
//#include <xcommon/IPluginManager.h>
//#include <core/sxcore.h>
#include "ModelOverlay.h"
#include "DynamicModel.h"
#include "DynamicModelProvider.h"
#include <xcommon/IPluginManager.h>
#include "Decal.h"

CDecalProvider::CDecalProvider(IXCore *pCore, CDynamicModelProvider *pProviderDynamic):
	m_pCore(pCore),
	m_pProviderDynamic(pProviderDynamic)
{
	m_pMaterialSystem = (IXMaterialSystem*)pCore->getPluginManager()->getInterface(IXMATERIALSYSTEM_GUID);
	m_pRenderUtils = (IXRenderUtils*)pCore->getPluginManager()->getInterface(IXRENDERUTILS_GUID);
	m_pRender = (IXRender*)pCore->getPluginManager()->getInterface(IXRENDER_GUID);

	IXScene *pScene = (IXScene*)pCore->getPluginManager()->getInterface(IXSCENE_GUID);
	IXSceneObjectType *pType = pScene->getObjectType("xDynamic");
	m_pQuery = pType->newQuery();
	mem_release(pType);
	
	pCore->getConsole()->registerCVar("r_maxdecals", 300, "Max temp decals");
	//pCore->getConsole()->registerCVar("r_max_overlapped_decals", 4, "Max overlapped decals");
	
#ifdef DECAL_DEBUG_DRAW
	m_pRenderUtils->newGizmoRenderer(&m_pDebugRenderer);
#endif

	loadDecals();
}

CDecalProvider::~CDecalProvider()
{
	fora(i, m_aTempDecals)
	{
		mem_release(m_aTempDecals[i]);
	}
	for(UINT i = 0; i < XDT__COUNT; ++i)
	{
		mem_release(m_aDecalTypes[i].pMaterial);
	}
	mem_release(m_pQuery);
	mem_release(m_pBlendState);
	mem_release(m_pDSState);
	mem_release(m_pWorldBuffer);
#ifdef DECAL_DEBUG_DRAW
	mem_release(m_pDebugRenderer);
#endif
}

void CDecalProvider::render(CRenderableVisibility *pVisibility)
{
	XPROFILE_FUNCTION();

	CRenderableVisibility::OverlayData &overlayData = pVisibility->getOverlayData();
	
	UINT uVertexCount = overlayData.aVertices.size();

	if(uVertexCount)
	{
		if(overlayData.uVertexBufferAllocSize < uVertexCount)
		{
			overlayData.uVertexBufferAllocSize = uVertexCount;
			mem_release(overlayData.pRB);
			mem_release(overlayData.pVB);
			mem_release(overlayData.pIB);

			overlayData.pVB = m_pDevice->createVertexBuffer(sizeof(XResourceModelStaticVertexGPU) * uVertexCount, GXBUFFER_USAGE_STREAM);
			overlayData.pRB = m_pDevice->createRenderBuffer(1, &overlayData.pVB, m_pProviderDynamic->getVertexDeclaration());
			m_pRenderUtils->getQuadIndexBuffer(uVertexCount / 4, &overlayData.pIB);
		}

		XResourceModelStaticVertexGPU *pVertices;
		if(overlayData.pVB->lock((void**)&pVertices, GXBL_WRITE))
		{
			memcpy(pVertices, overlayData.aVertices, sizeof(XResourceModelStaticVertexGPU) * uVertexCount);
			overlayData.pVB->unlock();
		}

		m_pProviderDynamic->bindVertexFormat();

		IGXContext *pCtx = m_pDevice->getThreadContext();
		pCtx->setRenderBuffer(overlayData.pRB);
		pCtx->setIndexBuffer(overlayData.pIB);
		pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);
		IGXBlendState *pOldBlendState = pCtx->getBlendState();
		pCtx->setBlendState(m_pBlendState);
		m_pDevice->getThreadContext()->setVSConstant(m_pWorldBuffer, 1 /* SCR_OBJECT */);
		IGXDepthStencilState *pOldDSState = pCtx->getDepthStencilState();
		pCtx->setDepthStencilState(m_pDSState);

		fora(i, overlayData.aSubsets)
		{
			auto &ss = overlayData.aSubsets[i];
			m_pMaterialSystem->bindMaterial(ss.pMaterial);
			pCtx->drawIndexed(ss.uQuadCount * 4, ss.uQuadCount * 2, ss.uStartIndex);
		}

		pCtx->setBlendState(pOldBlendState);
		mem_release(pOldBlendState);

		pCtx->setDepthStencilState(pOldDSState);
		mem_release(pOldDSState);
	}

#ifdef DECAL_DEBUG_DRAW
	m_pDebugRenderer->render(false, false);
#endif
}

void XMETHODCALLTYPE CDecalProvider::shootDecal(XDECAL_TYPE type, const float3 &vWorldPos, const float3 &vNormal, float fScale, const float3 *pvSAxis)
{
	//type = XDT_GLASS;
	const DecalType *pType = getDecalType(type);
	if(!pType || !pType->pMaterial)
	{
		return;
	}

	fScale *= pType->fBaseScale * 0.0008f;

	IXTexture *pTex = pType->pMaterial->getTexture("txBase");
	if(!pTex)
	{
		IKeyIterator *pIter = pType->pMaterial->getTexturesIterator();
		if(pIter)
		{
			pTex = pType->pMaterial->getTexture(pIter->getCurrent());
		}
		mem_release(pIter);
	}

	UINT uTexWidth = pTex->getWidth();
	UINT uTexHeight = pTex->getHeight();

	DecalTexRange texRange = {0, 0, uTexWidth, uTexHeight};
	if(pType->aTextures.size())
	{
		texRange = pType->aTextures[rand() % pType->aTextures.size()];
	}
	
	float2_t vSize((float)(texRange.xmax - texRange.xmin) * fScale, (float)(texRange.ymax - texRange.ymin) * fScale);

	float2_t sBound(-vSize.x * 0.5f, vSize.x * 0.5f);
	float2_t tBound(-vSize.y * 0.5f, vSize.y * 0.5f);

	//compute basis
	SMMATRIX mBasis;
	computeBasis(&mBasis, SMVector3Normalize(vNormal), pvSAxis);

	SMQuaternion qOrient = SMQuaternion(mBasis.r[0], mBasis.r[1], mBasis.r[2]).Normalize();

	//vWorldPos;
	//qOrient;
	//pType->pMaterial;
	float2_t avCorners[] = {
		float2_t(sBound.x, tBound.x),
		float2_t(sBound.x, tBound.y),
		float2_t(sBound.y, tBound.y),
		float2_t(sBound.y, tBound.x),
	};
	float fHeight = min(vSize.x, vSize.y) * 0.5f + 0.1f;

#ifdef DECAL_DEBUG_DRAW
	m_pDebugRenderer->setPointSize(0.01f);
	m_pDebugRenderer->setLineWidth(0.01f);
	m_pDebugRenderer->setColor(float4(1.0f, 0.0f, 0.0f, 1.0f));
	m_pDebugRenderer->jumpTo(vWorldPos);
	m_pDebugRenderer->lineTo(vWorldPos + mBasis.r[0]);
	m_pDebugRenderer->setColor(float4(0.0f, 1.0f, 0.0f, 1.0f));
	m_pDebugRenderer->jumpTo(vWorldPos);
	m_pDebugRenderer->lineTo(vWorldPos + mBasis.r[1]);
	m_pDebugRenderer->setColor(float4(0.0f, 0.0f, 1.0f, 1.0f));
	m_pDebugRenderer->jumpTo(vWorldPos);
	m_pDebugRenderer->lineTo(vWorldPos + mBasis.r[2]);

	m_pDebugRenderer->setColor(float4(1.0f, 1.0f, 1.0f, 1.0f));
	/*for(UINT i = 0; i < ARRAYSIZE(avCorners); ++i)
	{
		m_pDebugRenderer->drawPoint(qOrient * float3(avCorners[i], -fHeight) + vWorldPos);
		m_pDebugRenderer->drawPoint(qOrient * float3(avCorners[i], fHeight) + vWorldPos);
	}*/
#endif

	// create CDecal
	CDecal *pDecal = allocDecal();
	pDecal->setMaterial(pType->pMaterial);
	for(UINT i = 0; i < DECAL_POINTS; ++i)
	{
		pDecal->setCorner(i, avCorners[i]);
	}
	pDecal->setPosition(vWorldPos);
	pDecal->setOrientation(qOrient);
	pDecal->setHeight(fHeight);
	pDecal->setTextureRangeU(float2((float)texRange.xmin, (float)texRange.xmax) / (float)uTexWidth);
	pDecal->setTextureRangeV(float2((float)texRange.ymin, (float)texRange.ymax) / (float)uTexHeight);

	addTempDecal(pDecal);
}

void CDecalProvider::computeVisibility(const float3 &vHintDir, CRenderableVisibility *pVisibility)
{
	Array<CDynamicModel*> &aRenderList = pVisibility->getRenderList();

	struct TmpOverlay
	{
		CDynamicModel *pModel;
		CModelOverlay *pOverlay;
	};

	Array<TmpOverlay> aOverlays;

	bool useHintDir = !SMIsZero(SMVector3Length2(vHintDir));

	UINT uVertices = 0;
	fora(i, aRenderList)
	{
		CDynamicModel *pMdl = aRenderList[i];
		CModelOverlay *pOverlay = pMdl->getOverlay();
		while(pOverlay)
		{
			// skip backface overlays
			bool bRender = !useHintDir || pOverlay->getMaterial()->isTwoSided();
			if(!bRender)
			{
				float3 vNormal = pOverlay->getNormal();
				if(!pMdl->isStatic())
				{
					vNormal = pMdl->getOrientation() * vNormal;
				}
				bRender = SMVector3Dot(vNormal, vHintDir) < 0.0f;
			}
			if(bRender)
			{
				aOverlays.push_back({pMdl, pOverlay});

				uVertices += pOverlay->getVertices().size();
			}

			pOverlay = pOverlay->getNextOverlay();
		}
	}

	CRenderableVisibility::OverlayData &overlayData = pVisibility->getOverlayData();

	Array<XResourceModelStaticVertexGPU> &aVertices = overlayData.aVertices;
	aVertices.clearFast();
	aVertices.reserve(uVertices);
	Array<CRenderableVisibility::OverlaySubset>& aSubsets = overlayData.aSubsets;
	aSubsets.clearFast();
	
	aOverlays.quickSort([](const TmpOverlay &a, const TmpOverlay &b){
		return(a.pOverlay->getMaterial() < b.pOverlay->getMaterial());
	});

	XResourceModelStaticVertex tempVertex;
	XResourceModelStaticVertexGPU tempGpuVertex;
	CRenderableVisibility::OverlaySubset curSubset = {};
	fora(i, aOverlays)
	{
		TmpOverlay &overlay = aOverlays[i];

		if(curSubset.pMaterial != overlay.pOverlay->getMaterial())
		{
			if(curSubset.uStartVertex != aVertices.size())
			{
				curSubset.uQuadCount = (aVertices.size() - curSubset.uStartVertex) / 4;
				aSubsets.push_back(curSubset);
			}
			curSubset.pMaterial = overlay.pOverlay->getMaterial();

			curSubset.uStartVertex = aVertices.size();
			curSubset.uStartIndex = aVertices.size() / 4 * 6;
		}

		const Array<XResourceModelStaticVertex>& aOverlayVertices = overlay.pOverlay->getVertices();
		bool bNeedTransform = !overlay.pModel->isStatic();

		float3 vPos = overlay.pModel->getPosition();
		SMQuaternion qRot = overlay.pModel->getOrientation();

		fora(j, aOverlayVertices)
		{
			tempVertex = aOverlayVertices[j];
			if(bNeedTransform)
			{
				// transform vertices
				tempVertex.vPos = qRot * tempVertex.vPos + vPos;
				tempVertex.vNorm = qRot * tempVertex.vNorm;
				tempVertex.vTangent = qRot * tempVertex.vTangent;
				tempVertex.vBinorm = qRot * tempVertex.vBinorm;
			}
			
#define TO_SHORT(v) ((short)((v) * 32767.0f))
			auto &dst = tempGpuVertex;
			auto &src = tempVertex;
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

			aVertices.push_back(tempGpuVertex);
		}	
	}

	if(curSubset.uStartVertex != aVertices.size())
	{
		curSubset.uQuadCount = (aVertices.size() - curSubset.uStartVertex) / 4;
		aSubsets.push_back(curSubset);
	}
}

void CDecalProvider::setDevice(IGXDevice *pDevice)
{
	m_pDevice = pDevice;

	GXBlendDesc blendDesc;
	memset(&blendDesc, 0, sizeof(blendDesc));
	blendDesc.renderTarget[0].u8RenderTargetWriteMask = GXCOLOR_WRITE_ENABLE_RED | GXCOLOR_WRITE_ENABLE_GREEN | GXCOLOR_WRITE_ENABLE_BLUE;
	blendDesc.renderTarget[0].blendSrcColor = GXBLEND_DEST_COLOR;
	blendDesc.renderTarget[0].blendDestColor = GXBLEND_SRC_COLOR;
	blendDesc.renderTarget[0].blendSrcAlpha = GXBLEND_SRC_ALPHA;
	blendDesc.renderTarget[0].blendDestAlpha = GXBLEND_INV_SRC_ALPHA;
	blendDesc.renderTarget[0].blendOpColor = GXBLEND_OP_ADD;
	blendDesc.renderTarget[0].blendOpAlpha = GXBLEND_OP_ADD;

	/*blendDesc.renderTarget[0].blendSrcColor = GXBLEND_DEST_COLOR;
	blendDesc.renderTarget[0].blendDestColor = GXBLEND_SRC_COLOR;
	blendDesc.renderTarget[0].blendSrcAlpha = GXBLEND_DEST_ALPHA;
	blendDesc.renderTarget[0].blendDestAlpha = GXBLEND_SRC_ALPHA;
	blendDesc.renderTarget[0].blendOpColor = GXBLEND_OP_ADD;
	blendDesc.renderTarget[0].blendOpAlpha = GXBLEND_OP_ADD;*/

	blendDesc.renderTarget[0].useBlend = TRUE;

	blendDesc.renderTarget[1].blendSrcColor = GXBLEND_ZERO;
	blendDesc.renderTarget[1].blendDestColor = GXBLEND_ONE;

	blendDesc.renderTarget[2].blendSrcColor = GXBLEND_ONE;
	blendDesc.renderTarget[2].blendDestColor = GXBLEND_ZERO;

	blendDesc.renderTarget[3].blendSrcColor = GXBLEND_ONE;
	blendDesc.renderTarget[3].blendDestColor = GXBLEND_ZERO;

	blendDesc.renderTarget[1].useBlend = FALSE;
	blendDesc.renderTarget[2].useBlend = FALSE;
	blendDesc.renderTarget[3].useBlend = FALSE;

	blendDesc.useIndependentBlend = TRUE;

	m_pBlendState = pDevice->createBlendState(&blendDesc);


	GXDepthStencilDesc dsDesc = {};
	dsDesc.cmpFuncDepth = GXCMP_GREATER_EQUAL;
	dsDesc.useDepthWrite = FALSE;
	m_pDSState = pDevice->createDepthStencilState(&dsDesc);


	m_pWorldBuffer = pDevice->createConstantBuffer(sizeof(SMMATRIX));
	m_pWorldBuffer->update(&SMMatrixIdentity());
}

void CDecalProvider::onDecalReleased(CDecal *pDecal)
{
	ScopedSpinLock lock(m_slMemDecals);
	m_memDecals.Delete(pDecal);
}

void CDecalProvider::onDecalEmptied(CDecal *pDecal)
{
	int idx = -1;
	{
		ScopedSpinLock lock(m_slTempDecals);

		idx = m_aTempDecals.indexOf(pDecal);
		if(idx >= 0)
		{
			m_aTempDecals.erase(idx);
		}
	}

	if(idx >= 0)
	{
		mem_release(pDecal);
	}
}

void CDecalProvider::updateDecal(CDecal *pDecal)
{
	add_ref(pDecal);
	m_qDecalsForUpdate.push(pDecal);
}

void CDecalProvider::update()
{
	CDecal *pDecal;
	while(m_qDecalsForUpdate.pop(&pDecal))
	{
		pDecal->update(
#ifdef DECAL_DEBUG_DRAW
			m_pDebugRenderer
#endif
		);
		mem_release(pDecal);
	}
}

CDecal* CDecalProvider::allocDecal()
{
	ScopedSpinLock lock(m_slMemDecals);
	return(m_memDecals.Alloc(m_pQuery, m_pRender, this));
}

CModelOverlay* CDecalProvider::allocOverlay(CDecal *pDecal, IXMaterial *pMaterial, Array<XResourceModelStaticVertex> &aVertices, const float3_t &vNormal)
{
	ScopedSpinLock lock(m_slMemOverlays);
	return(m_memOverlays.Alloc(pDecal, pMaterial, aVertices, vNormal));
}
void CDecalProvider::freeOverlay(CModelOverlay *pOverlay)
{
	ScopedSpinLock lock(m_slMemOverlays);
	m_memOverlays.Delete(pOverlay);
}

void CDecalProvider::loadDecals()
{
	IXConfig *pConfig = m_pCore->newConfig();
	if(pConfig->open("config/decals/decals.cfg"))
	{
		UINT uSections = pConfig->getSectionCount();
		for(UINT i = 0; i < uSections; ++i)
		{
			const char *szSection = pConfig->getSectionName(i);

			int id;
			if(pConfig->keyExists(szSection, "id") && sscanf(pConfig->getKey(szSection, "id"), "%d", &id))
			{
				if(id < 0 || id >= XDT__COUNT)
				{
					LogError("Incorrect decal type id '%s'\n", szSection);
					continue;
				}
			}
			else
			{
				LogError("Unable to read decal id '%s'\n", szSection);
				continue;
			}

			const char *szTex = pConfig->getKey(szSection, "tex");
			if(!szTex)
			{
				LogError("Unable to read decal tex '%s'\n", szSection);
				continue;
			}

			const char *szScale = pConfig->getKey(szSection, "base_scale");
			if(szScale)
			{
				sscanf(szScale, "%f", &m_aDecalTypes[id].fBaseScale);
			}

			
			int j = 0;

			DecalTexRange rng;
			char key[64];
			while(sprintf(key, "tex%d", j) && pConfig->keyExists(szSection, key))
			{
				if(sscanf(pConfig->getKey(szSection, key), "[%d,%d,%d,%d]", &rng.xmin, &rng.ymin, &rng.xmax, &rng.ymax) != 4)
				{
					LogError("Unable to read decal tex coords \"%s\" \"%s\"\n", pConfig->getKey(szSection, key), szSection);
				}
				else
				{
					m_aDecalTypes[id].aTextures.push_back(rng);
				}
				j++;
			}

			m_pMaterialSystem->loadMaterial(szTex, &m_aDecalTypes[id].pMaterial);
		}
	}

	mem_release(pConfig);
}

const CDecalProvider::DecalType* CDecalProvider::getDecalType(XDECAL_TYPE type)
{
	if(type < 0 || type >= XDT__COUNT)
	{
		LogError("Incorrect decal type %d\n", type);
		return(NULL);
	}
	return(&m_aDecalTypes[type]);
}

void CDecalProvider::computeBasis(SMMATRIX *pmOut, const float3_t &vSurfaceNormal, const float3 *pvSAxis)
{
	// s, t, textureSpaceNormal (S cross T = textureSpaceNormal(N))
	//         
	//      +---->S
	//     /| 
	//	  /	|  
	//   N  |T    
	//
	// S = textureSpaceBasis[0]
	// T = textureSpaceBasis[1]
	// N = textureSpaceBasis[2]

	// Get the surface normal.
	pmOut->r[2] = vSurfaceNormal;

	if(pvSAxis)
	{
		// T = N cross S
		pmOut->r[1] = SMVector3Cross(pmOut->r[2], *pvSAxis);

		// Name sure they aren't parallel or antiparallel
		// In that case, fall back to the normal algorithm.
		if(SMVector3Dot(pmOut->r[1], pmOut->r[1]) > 1e-6)
		{
			// S = T cross N
			pmOut->r[0] = SMVector3Cross(pmOut->r[1], pmOut->r[2]);

			pmOut->r[0] = SMVector3Normalize(pmOut->r[0]);
			pmOut->r[1] = SMVector3Normalize(pmOut->r[1]);
			return;
		}

		// Fall through to the standard algorithm for parallel or antiparallel
	}

	// floor/ceiling?
	if(fabs(vSurfaceNormal.y) > SIN_45_DEGREES)
	{
		pmOut->r[0] = float4(1.0f, 0.0f, 0.0f, 0.0f);

		// T = N cross S
		pmOut->r[1] = SMVector3Cross(pmOut->r[2], pmOut->r[0]);

		// S = T cross N
		pmOut->r[0] = SMVector3Cross(pmOut->r[1], pmOut->r[2]);
	}
	// wall
	else
	{
		pmOut->r[1] = float4(0.0f, -1.0f, 0.0f, 0.0f);

		// S = T cross N
		pmOut->r[0] = SMVector3Cross(pmOut->r[1], pmOut->r[2]);
		// T = N cross S
		pmOut->r[1] = SMVector3Cross(pmOut->r[2], pmOut->r[0]);
	}

	pmOut->r[0] = SMVector3Normalize(pmOut->r[0]);
	pmOut->r[1] = SMVector3Normalize(pmOut->r[1]);
}

void CDecalProvider::addTempDecal(CDecal *pDecal)
{
	ScopedSpinLock lock(m_slTempDecals);

	static const int *r_maxdecals = m_pCore->getConsole()->getPCVarInt("r_maxdecals");
	if(m_aTempDecals.size() > *r_maxdecals)
	{
		for(UINT i = (UINT)*r_maxdecals, l = m_aTempDecals.size(); i < l; ++i)
		{
			mem_release(m_aTempDecals[i]);
		}
	}
	m_aTempDecals.reserve(*r_maxdecals);

	TODO("Use ring array for that");
	if(m_aTempDecals.size() == *r_maxdecals)
	{
		mem_release(m_aTempDecals[0]);
		m_aTempDecals.erase(0);
	}
	m_aTempDecals.push_back(pDecal);
}
