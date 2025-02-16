#include "Decal.h"
#include "DynamicModel.h"
#include "DecalProvider.h"

CDecal::CDecal(IXSceneQuery *pSceneQuery, IXRender *pRender, CDecalProvider *pProvider):
	m_pSceneQuery(pSceneQuery),
	m_pRender(pRender),
	m_pProvider(pProvider)
{
}

CDecal::~CDecal()
{
	removeOverlays();
	mem_release(m_pMaterial);
}

bool XMETHODCALLTYPE CDecal::isEnabled() const
{
	return(m_isEnabled);
}
void XMETHODCALLTYPE CDecal::enable(bool yesNo)
{
	m_isEnabled = yesNo;
	TODO("Change state");
}

float3 XMETHODCALLTYPE CDecal::getPosition() const
{
	return(m_vPos);
}
void XMETHODCALLTYPE CDecal::setPosition(const float3 &vPos)
{
	m_vPos = vPos;
	setDirty();
}

SMQuaternion XMETHODCALLTYPE CDecal::getOrientation() const
{
	return(m_qRot);
}
void XMETHODCALLTYPE CDecal::setOrientation(const SMQuaternion &qRot)
{
	m_qRot = qRot;
	setDirty();
}

float3 XMETHODCALLTYPE CDecal::getLocalBoundMin() const
{
	float3 vBound;
	for(UINT i = 0; i < DECAL_POINTS; ++i)
	{
		if(i == 0)
		{
			vBound = m_qRot * float3(m_avCorners[0], 0.0f);
		}
		else
		{
			vBound = SMVectorMin(vBound, m_qRot * float3(m_avCorners[0], 0.0f));
		}
		vBound = SMVectorMin(vBound, m_qRot * float3(m_avCorners[0], m_fHeight));
	}

	return(vBound);
}
float3 XMETHODCALLTYPE CDecal::getLocalBoundMax() const
{
	float3 vBound;
	for(UINT i = 0; i < DECAL_POINTS; ++i)
	{
		if(i == 0)
		{
			vBound = m_qRot * float3(m_avCorners[0], 0.0f);
		}
		else
		{
			vBound = SMVectorMax(vBound, m_qRot * float3(m_avCorners[0], 0.0f));
		}
		vBound = SMVectorMax(vBound, m_qRot * float3(m_avCorners[0], m_fHeight));
	}

	return(vBound);
}
SMAABB XMETHODCALLTYPE CDecal::getLocalBound() const
{
	return(SMAABB(getLocalBoundMin(), getLocalBoundMax()));
}

bool XMETHODCALLTYPE CDecal::rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut, float3 *pvNormal, bool isRayInWorldSpace, bool bReturnNearestPoint)
{
	TODO("Implement me!");
	return(false);
}

void XMETHODCALLTYPE CDecal::setLayer(UINT uLayer)
{
	m_uLayer = uLayer;
	setDirty();
}
UINT XMETHODCALLTYPE CDecal::getLayer()
{
	return(m_uLayer);
}

void XMETHODCALLTYPE CDecal::setHeight(float fHeight)
{
	m_fHeight = fHeight;
	setDirty();
}
float XMETHODCALLTYPE CDecal::getHeight()
{
	return(m_fHeight);
}

void XMETHODCALLTYPE CDecal::setTextureRangeU(const float2_t &vRange)
{
	m_vTexRangeU = vRange;
}
float2_t XMETHODCALLTYPE CDecal::getTextureRangeU()
{
	return(m_vTexRangeU);
}

void XMETHODCALLTYPE CDecal::setTextureRangeV(const float2_t &vRange)
{
	m_vTexRangeV = vRange;
}
float2_t XMETHODCALLTYPE CDecal::getTextureRangeV()
{
	return(m_vTexRangeV);
}

void CDecal::setMaterial(IXMaterial *pMaterial)
{
	mem_release(m_pMaterial);
	m_pMaterial = pMaterial;
	add_ref(m_pMaterial);
}
void CDecal::setCorner(UINT uCorner, const float2_t &vCorner)
{
	assert(uCorner < DECAL_POINTS);
	if(uCorner < DECAL_POINTS)
	{
		m_avCorners[uCorner] = vCorner;
		setDirty();
	}
}

void CDecal::update(
#ifdef DECAL_DEBUG_DRAW
	IXGizmoRenderer *pDebugRenderer
#endif
)
{
	if(m_isDirty)
	{
		// cleanup old overlays
		removeOverlays();

		// query for objects
		
		SMPLANE aPlanes[] = {
			SMPlaneNormalize(SMPLANE(float3(m_avCorners[0], 0.0f), float3(m_avCorners[1], 0.0f), float3(m_avCorners[1], m_fHeight))),
			SMPlaneNormalize(SMPLANE(float3(m_avCorners[2], 0.0f), float3(m_avCorners[3], 0.0f), float3(m_avCorners[3], m_fHeight))),

			SMPlaneNormalize(SMPLANE(float3(m_avCorners[1], 0.0f), float3(m_avCorners[2], 0.0f), float3(m_avCorners[2], m_fHeight))),
			SMPlaneNormalize(SMPLANE(float3(m_avCorners[3], 0.0f), float3(m_avCorners[0], 0.0f), float3(m_avCorners[0], m_fHeight))),

			SMPLANE(0.0f, 0.0f, 1.0f, m_fHeight),
			SMPLANE(0.0f, 0.0f, -1.0f, m_fHeight),
		};
				
		//SMMATRIX mWorld = SMMatrixTranspose(m_qRot.GetMatrix() * SMMatrixTranslation(m_vPos));
		//SMMATRIX mWorld = m_qRot.GetMatrix();
		for(UINT i = 0; i < ARRAYSIZE(aPlanes); ++i)
		{
			//aPlanes[i] = SMPlaneTransformTI(aPlanes[i], mWorld);
			aPlanes[i] = SMPLANE(m_qRot * float3(aPlanes[i]), aPlanes[i].w);
			aPlanes[i].w -= SMVector3Dot(aPlanes[i], m_vPos);
		}

		IXFrustum *pFrustum;
		m_pRender->newFrustum(&pFrustum);
		
		pFrustum->update(aPlanes, false);

#ifdef DECAL_DEBUG_DRAW
		pDebugRenderer->setColor(float4(1.0f, 1.0f, 0.0f, 1.0f));
		for(UINT i = 0; i < 8; ++i)
		{
			pDebugRenderer->drawPoint(pFrustum->getPoint(i));
		}
		pDebugRenderer->setColor(float4(1.0f, 1.0f, 0.0f, 0.2f));
#endif

		CDynamicModel **ppObjects = NULL;
		m_pSceneQuery->setLayerMask(1 << m_uLayer);
		UINT uCount = m_pSceneQuery->execute(pFrustum, (void***)&ppObjects);

		SMPLANE aPlanesObjectSpace[ARRAYSIZE(aPlanes)];

		// build overlays
		for(UINT i = 0; i < uCount; ++i)
		{
			TODO("Check ppObjects[i]->receivingDecals()");

			for(UINT j = 0; j < ARRAYSIZE(aPlanes); ++j)
			{
				aPlanesObjectSpace[j] = SMPLANE(ppObjects[i]->getOrientation().Conjugate() * float3(aPlanes[j]), aPlanes[j].w + SMVector3Dot(aPlanes[j], ppObjects[i]->getPosition()));
			}
			pFrustum->update(aPlanesObjectSpace, false);

			spawnOverlayForModel(ppObjects[i], pFrustum
#ifdef DECAL_DEBUG_DRAW
				, pDebugRenderer
#endif
			);
		}

		mem_release(pFrustum);

		m_isDirty = false;
	}
}

void CDecal::onOverlayRemoved(CModelOverlay *pOverlay)
{
	int idx = m_aOverlays.indexOf(pOverlay, [](const Overlay &a, CModelOverlay *pB){
		return(a.pOverlay == pB);
	});
	assert(idx >= 0);
	if(idx >= 0)
	{
		m_aOverlays.erase(idx);
		m_pProvider->freeOverlay(pOverlay);
		if(!m_aOverlays.size())
		{
			m_pProvider->onDecalEmptied(this);
		}
	}
}

template<typename T>
static T ArrGet(T *pArr, UINT uSize, UINT uIdx, int iStartOffset = 0)
{
	return(pArr[((int)uIdx + iStartOffset) % uSize]);
}

template<typename T>
static void ArrDel(T *pArr, UINT &uSize, UINT uIdx, int &iStartOffset)
{
	UINT uStartIndex = ((int)uIdx + iStartOffset) % uSize;
	--uSize;
	if(uStartIndex != uSize)
	{
		for(UINT i = uStartIndex; i < uSize; ++i)
		{
			pArr[i] = pArr[i + 1];
		}
	}
	if(uStartIndex < iStartOffset)
	{
		--iStartOffset;
	}
}

template<typename T>
static void ArrIns(const T &val, T *pArr, UINT &uSize, UINT uIdx, int &iStartOffset)
{
	UINT uStartIndex = ((int)uIdx + iStartOffset) % uSize;
	if(uStartIndex == 0)
	{
		uStartIndex = uSize;
	}
	else
	{
		for(UINT i = uSize; i > uStartIndex; --i)
		{
			pArr[i] = pArr[i - 1];
		}
	}
	assert(uStartIndex < 10);
	pArr[uStartIndex] = val;
	++uSize;

	if(uStartIndex < iStartOffset)
	{
		++iStartOffset;
	}
}

template<typename T>
static void ArrSet(const T &val, T *pArr, UINT &uSize, UINT uIdx, int iStartOffset = 0)
{
	pArr[((int)uIdx + iStartOffset) % uSize] = val;
}

static bool IsInside(const SMPLANE &plane, const float3_t &vPos)
{
	return(SMVector4Dot(plane, float4(vPos, 1.0f)) > 0.0f);
}

void XMETHODCALLTYPE CDecal::FinalRelease()
{
	m_pProvider->onDecalReleased(this);
}

void CDecal::spawnOverlayForModel(CDynamicModel *pModel, IXFrustum *pFrustum
#ifdef DECAL_DEBUG_DRAW
	, IXGizmoRenderer *pDebugRenderer
#endif
)
{
	const IXResourceModelStatic *pResource = pModel->getResource()->asStatic();
	if(!pResource)
	{
		return;
	}

	if(pResource->getPrimitiveTopology() != XPT_TRIANGLELIST)
	{
		LogError("CDecal::spawnOverlayForModel(): Unsupported primitive topology");
	}

	SMQuaternion qDecalToModel = m_qRot * pModel->getOrientation().Conjugate();
	float3 vS = qDecalToModel * float3(1.0f, 0.0f, 0.0f);
	float3 vT = qDecalToModel * float3(0.0f, 1.0f, 0.0f);
	float3 vN = qDecalToModel * float3(0.0f, 0.0f, 1.0f);

	float2_t sBound;
	float2_t tBound;
	// center point
	float3 vModelSpaceDecalCenter = pModel->getOrientation().Conjugate() * (m_vPos - pModel->getPosition());
	
	for(UINT j = 0; j < DECAL_POINTS; ++j)
	{
		float s = m_avCorners[j].x;
		float t = m_avCorners[j].y;
		if(j == 0)
		{
			sBound.x = s;
			sBound.y = s;
			tBound.x = t;
			tBound.y = t;
		}
		else
		{
			if(sBound.x > s)
			{
				sBound.x = s;
			}
			if(sBound.y < s)
			{
				sBound.y = s;
			}
			if(tBound.x > t)
			{
				tBound.x = t;
			}
			if(tBound.y < t)
			{
				tBound.y = t;
			}
		}
	}
	float2_t vInvBoundRange = float2(1.0f, 1.0f) / float2(sBound.y - sBound.x, tBound.y - tBound.x);
	XResourceModelStaticVertex vtx;
	vtx.vTangent = vS;
	vtx.vBinorm = vT;
	vtx.vNorm = vN;

	float3_t aTempVertices[10]; // 9 - max vertex count of clipped triangle + 1 temp vertex
	UINT uVertexCount;

	Array<XResourceModelStaticVertex> aOverlayVertices;

	UINT uSubsets = pResource->getSubsetCount(0);
	for(UINT uSubset = 0; uSubset < uSubsets; ++uSubset)
	{
		TODO("Separate transparent subsets");
		const XResourceModelStaticSubset *pSubset = pResource->getSubset(0, uSubset);
		for(UINT i = 0; i < pSubset->iIndexCount; i += 3)
		{
			float3_t &a = pSubset->pVertices[pSubset->pIndices[i]].vPos;
			float3_t &b = pSubset->pVertices[pSubset->pIndices[i + 1]].vPos;
			float3_t &c = pSubset->pVertices[pSubset->pIndices[i + 2]].vPos;

			if(pFrustum->polyInFrustum(a, b, c))
			{
				aTempVertices[0] = a;
				aTempVertices[1] = b;
				aTempVertices[2] = c;
				uVertexCount = 3;

				//pDebugRenderer->drawPoly(a, b, c);

				// clip by frustum planes
				for(UINT j = 0, jl = pFrustum->getPlaneCount(); j < jl && uVertexCount >= 3/* && j < 4*/; ++j)
				{
					const SMPLANE &plane = pFrustum->getPlaneAt(j);
					int iInsideVertex = -1;
					// - find inside vertex
					for(UINT k = 0; k < uVertexCount; ++k)
					{
						if(IsInside(plane, aTempVertices[k]))
						{
							iInsideVertex = (int)k;
							break;
						}
					}

					if(iInsideVertex < 0)
					{
						// all vertices outside clipping plane
						uVertexCount = 0;
						break;
					}
					// - rotate array to make inside vertex first
					// - clip

					for(UINT k = 0; k < uVertexCount; ++k)
					{
						float3_t vCur = ArrGet(aTempVertices, uVertexCount, k, iInsideVertex);
						float3_t vNext = ArrGet(aTempVertices, uVertexCount, k + 1, iInsideVertex);
						bool bCurInside = IsInside(plane, vCur);
						bool bNextInside = IsInside(plane, vNext);
						if(bCurInside)
						{
							if(bNextInside)
							{
								continue;
							}
							else
							{
								// clip
								float3 vPt;
								bool b = plane.intersectLine(&vPt, vCur, vNext);
								assert(b);
								// insert after cur
								ArrIns((float3_t)vPt, aTempVertices, uVertexCount, k + 1, iInsideVertex);
								++k;
							}
						}
						else
						{
							if(bNextInside)
							{
								// clip
								float3 vPt;
								bool b = plane.intersectLine(&vPt, vCur, vNext);
								assert(b);
								// replace cur
								ArrSet((float3_t)vPt, aTempVertices, uVertexCount, k, iInsideVertex);
							}
							else
							{
								// drop cur
								ArrDel(aTempVertices, uVertexCount, k, iInsideVertex);
								--k;
							}
						}
					}
				}

				// put into overlay
				if(uVertexCount >= 3)
				{
					//if(pModel->isStatic())
					//{
					//	// transform to world pos
					//	float3 vPos = pModel->getPosition();
					//	SMQuaternion qRot = pModel->getOrientation();
					//	for(UINT j = 0; j < uVertexCount; ++j)
					//	{
					//		aTempVertices[j] = qRot * aTempVertices[j] + vPos;
					//	}
					//}

					UINT uStartVtx = aOverlayVertices.size();
					for(UINT b = 0, e = uVertexCount - 1; b + 1 <= e - 1; ++b, --e)
					{
						vtx.vPos = aTempVertices[e];
						aOverlayVertices.push_back(vtx);

						vtx.vPos = aTempVertices[b];
						aOverlayVertices.push_back(vtx);

						vtx.vPos = aTempVertices[e - 1];
						aOverlayVertices.push_back(vtx);

						vtx.vPos = aTempVertices[b + 1];
						aOverlayVertices.push_back(vtx);
					}

					float3 vTriN = SMVector3Normalize(SMVector3Cross(b - a, c - a));
					float3 vTriS = vS;
					float3 vTriT = vT;
					float fCos = SMVector3Dot(vN, vTriN);
					if(fCos < 0.2f)
					{
						float3 vRotAxis = SMVector3Cross(vN, vTriN);

						float fTempS = SMVector3Dot(vRotAxis, vS);
						float fTempT = SMVector3Dot(vRotAxis, vT);
						
						float fSign = max(fTempS, fTempT);
						float fAngle = safe_acosf(fCos);
						if(fSign < 0.0f)
						{
							fAngle = SM_2PI - fAngle;
						}

						SMQuaternion q(vRotAxis, fAngle);
						vTriS = q * vS;
						vTriT = q * vT;
					}

					for(UINT j = uStartVtx, jl = aOverlayVertices.size(); j < jl; ++j)
					{
						XResourceModelStaticVertex &vt = aOverlayVertices[j];

						float3 vPos = vt.vPos - vModelSpaceDecalCenter;
						float s = SMVector3Dot(vPos, vTriS);
						float t = SMVector3Dot(vPos, vTriT);
						vt.vPos = vt.vPos + vTriN * 0.0001f;

						vt.vTex.x = lerpf(m_vTexRangeU.x, m_vTexRangeU.y, clampf((s - sBound.x) * vInvBoundRange.x, 0.0f, 1.0f));
						vt.vTex.y = lerpf(m_vTexRangeV.x, m_vTexRangeV.y, clampf((t - tBound.x) * vInvBoundRange.y, 0.0f, 1.0f));
					}
				}
			}
		}
	}

	// aOverlayVertices
	CModelOverlay *pOverlay = m_pProvider->allocOverlay(this, m_pMaterial, aOverlayVertices, vN);
	m_aOverlays.push_back({pModel, pOverlay});

	CModelOverlay *pLastOverlay = pModel->getOverlay();
	if(!pLastOverlay)
	{
		pModel->setOverlay(pOverlay);
	}
	else
	{
		while(pLastOverlay->getNextOverlay())
		{
			pLastOverlay = pLastOverlay->getNextOverlay();
		}
		pLastOverlay->setNextOverlay(pOverlay);
	}
}

void CDecal::removeOverlays()
{
	fora(i, m_aOverlays)
	{
		Overlay &ovl = m_aOverlays[i];

		CModelOverlay *pPrev = NULL;
		CModelOverlay *pCur = ovl.pModel->getOverlay();
		while(pCur)
		{
			if(pCur == ovl.pOverlay)
			{
				if(pPrev)
				{
					pPrev->setNextOverlay(pCur->getNextOverlay());
				}
				else
				{
					ovl.pModel->setOverlay(pCur->getNextOverlay());
				}

				break;
			}

			pPrev = pCur;
			pCur = pCur->getNextOverlay();
		}

		m_pProvider->freeOverlay(ovl.pOverlay);
	}

	m_aOverlays.clearFast();
}

void CDecal::setDirty()
{
	if(!m_isDirty)
	{
		m_isDirty = true;
		m_pProvider->updateDecal(this);
	}
}
