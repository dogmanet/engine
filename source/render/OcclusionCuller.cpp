#include "OcclusionCuller.h"
//#include <physics/sxphysics.h>

#include <core/sxcore.h>
#include <xcommon/IXCamera.h>

/*struct CAllHitsResultCallback: public btCollisionWorld::RayResultCallback
{
	btAlignedObjectArray<const btCollisionObject*>		m_collisionObjects;

	float3	m_rayToWorld;

	btAlignedObjectArray<btScalar> m_hitFractions;

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override
	{
		m_collisionObject = rayResult.m_collisionObject;

		int idx = m_collisionObjects.findLinearSearch2(m_collisionObject);
		if(idx >= 0)
		{
			m_hitFractions[idx] = rayResult.m_hitFraction;
		}
		else
		{
			m_collisionObjects.push_back(rayResult.m_collisionObject);
			m_hitFractions.push_back(rayResult.m_hitFraction);
		}
		return m_closestHitFraction;
	}
};*/

//##########################################################################

COcclusionCuller::COcclusionCuller()
{
	m_pLightSystem = (IXLightSystem*)Core_GetIXCore()->getPluginManager()->getInterface(IXLIGHTSYSTEM_GUID);

	IXPhysics *pPhysics = (IXPhysics*)Core_GetIXCore()->getPluginManager()->getInterface(IXPHYSICS_GUID);
	m_pPhysWorld = pPhysics->getWorld();

	Core_0RegisterConcmdCls("oprint", this, (SXCONCMDCLS)&COcclusionCuller::_print);
}
COcclusionCuller::~COcclusionCuller()
{
	//! @todo uncomment me!
	// mem_release(m_pCamera);
}

void XMETHODCALLTYPE COcclusionCuller::update(IXCamera *pCamera, float fFOV, float fAspectRatio, float fFarPlane)
{
	XPROFILE_FUNCTION();
	assert(pCamera);

	//! @todo uncomment me!
	// mem_release(m_pCamera);

	m_pCamera = pCamera;

	//! @todo uncomment me!
	// m_pCamera->AddRef();

	m_fFOV = fFOV;
	m_fAspectRatio = fAspectRatio;
	m_fFarPlane = fFarPlane;

	if(m_fFarPlane > 42.0f)
	{
		m_fFarPlane = 42.0f;
	}

	_update();
}

bool XMETHODCALLTYPE COcclusionCuller::isPointVisible(const float3 &vPoint) const
{
	return(classifyPoint(vPoint) >= 0);
}

//bool XMETHODCALLTYPE COcclusionCuller::isPolyVisible(const float3 &vPoint1, const float3 &vPoint2, const float3 &vPoint3) const
//{
//	return(true);
//}
//
//bool XMETHODCALLTYPE COcclusionCuller::isSphereVisible(const float3 &vOrigin, float fRadius) const
//{
//	return(true);
//}

bool XMETHODCALLTYPE COcclusionCuller::isAABBvisible(const SMAABB &aabb, bool *pisFullyVisible) const
{
	bool isCulledByFrustum;
	//SMAABB aabb(aabbInput.vMin - float3(0.01f), aabbInput.vMax + float3(0.01f));

 	const float3 &vCamPos = m_pCamera->getPosition();

	const float3 avPoints[] = {
		aabb.vMin,
		float3(aabb.vMin.x, aabb.vMin.y, aabb.vMax.z),
		float3(aabb.vMin.x, aabb.vMax.y, aabb.vMin.z),
		float3(aabb.vMin.x, aabb.vMax.y, aabb.vMax.z),
		float3(aabb.vMax.x, aabb.vMin.y, aabb.vMin.z),
		float3(aabb.vMax.x, aabb.vMin.y, aabb.vMax.z),
		float3(aabb.vMax.x, aabb.vMax.y, aabb.vMin.z),
		aabb.vMax
	};
	const SMPLANE aPlanes[] = {
		SMPLANE(-1.0f, 0.0f, 0.0f, aabb.vMin.x),
		SMPLANE(1.0f, 0.0f, 0.0f, -aabb.vMax.x),
		SMPLANE(0.0f, -1.0f, 0.0f, aabb.vMin.y),
		SMPLANE(0.0f, 1.0f, 0.0f, -aabb.vMax.y),
		SMPLANE(0.0f, 0.0f, -1.0f, aabb.vMin.z),
		SMPLANE(0.0f, 0.0f, 1.0f, -aabb.vMax.z)
	};
	bool aisPlaneOriented[ARRAYSIZE(aPlanes)];
	bool isOutside = false;
	for(UINT i = 0, l = ARRAYSIZE(aPlanes); i < l; ++i)
	{
		aisPlaneOriented[i] = SMVector4Dot(aPlanes[i], float4(vCamPos, 1.0f)) >= 0.0f;
		if(aisPlaneOriented[i])
		{
			isOutside = true;
		}
	}
	if(!isOutside)
	{
		if(pisFullyVisible)
		{
			*pisFullyVisible = false;
		}
		return(true);
	}

	const bool aisPointValid[] = {
		aisPlaneOriented[0] || aisPlaneOriented[2] || aisPlaneOriented[4],
		aisPlaneOriented[0] || aisPlaneOriented[2] || aisPlaneOriented[5],
		aisPlaneOriented[0] || aisPlaneOriented[3] || aisPlaneOriented[4],
		aisPlaneOriented[0] || aisPlaneOriented[3] || aisPlaneOriented[5],
		aisPlaneOriented[1] || aisPlaneOriented[2] || aisPlaneOriented[4],
		aisPlaneOriented[1] || aisPlaneOriented[2] || aisPlaneOriented[5],
		aisPlaneOriented[1] || aisPlaneOriented[3] || aisPlaneOriented[4],
		aisPlaneOriented[1] || aisPlaneOriented[3] || aisPlaneOriented[5]
	};
	const UINT aauEdges[][2] = {
		{0, 1},
		{0, 2},
		{0, 4},
		{1, 3},
		{1, 5},
		{2, 3},
		{2, 6},
		{3, 7},
		{4, 5},
		{4, 5},
		{5, 7},
		{6, 7},
	};

	bool hasHiddenPoints = false;
	bool hasVisiblePoints = false;
	bool hasNonFrustumCulled = false;
	float3 vPoint;

	int cls;
	for(UINT i = 0, l = ARRAYSIZE(avPoints); i < l; ++i)
	{
		if(!aisPointValid[i])
		{
			continue;
		}
		cls = classifyPoint(avPoints[i], &isCulledByFrustum);
		if(cls > 0)
		{
			if(!pisFullyVisible)
			{
				return(true);
			}
			if(hasHiddenPoints)
			{
				*pisFullyVisible = false;
				return(true);
			}
			hasVisiblePoints = true;
		}
		else if(cls < 0)
		{
			if(hasVisiblePoints)
			{
				*pisFullyVisible = false;
				return(true);
			}
			if(!isCulledByFrustum)
			{
				hasNonFrustumCulled = true;
			}
			hasHiddenPoints = true;
		}
	}

	if(!hasVisiblePoints/* && !hasNonFrustumCulled*/)
	{
		// will get there if all points is outside the frustum
		IXFrustum *pFrustum = m_pCamera->getFrustum();

		bool hasIntersection = false;

		for(UINT a = 0; a < ARRAYSIZE(aauEdges) && !hasIntersection; ++a)
		{
			if(!aisPointValid[aauEdges[a][0]] || !aisPointValid[aauEdges[a][1]])
			{
				continue;
			}
			const float3 &vA = avPoints[aauEdges[a][0]];
			const float3 &vB = avPoints[aauEdges[a][1]];

			for(UINT i = 0, l = pFrustum->getPlaneCount(); i < l; ++i)
			{
				const SMPLANE &plane = pFrustum->getPlaneAt(i);
				if(plane.intersectLine(&vPoint, vA, vB))
				{
					vPoint.w = 1.0f;
					bool isInside = true;
					for(UINT j = 0; j < l; ++j)
					{
						if(j != i && SMVector4Dot(vPoint, pFrustum->getPlaneAt(j)) < 0.0f)
						{
							isInside = false;
							break;
						}
					}

					if(isInside)
					{
						hasIntersection = true;
						cls = classifyPoint(vPoint, NULL, true);
						if(pisFullyVisible)
						{
							*pisFullyVisible = false;
						}
						if(cls >= 0)
						{
							mem_release(pFrustum);
							return(true);
						}
						break;
					}
				}
			}
		}

		//if(!hasIntersection)
		{
			// raycast corners
			
			float3 vA = m_pCamera->getPosition();
			for(UINT i = 0; i < 4; ++i)
			{
				float3 vB = cbs[i & 2 ? 0 : OCCLUSION_BUFFER_WIDTH][i & 1 ? 0 : OCCLUSION_BUFFER_HEIGHT].m_rayToWorld;
				
				for(UINT j = 0, jl = ARRAYSIZE(aPlanes); j < jl; ++j)
				{
					if(aisPlaneOriented[j] && aPlanes[j].intersectLine(&vPoint, vA, vB))
					{
						vPoint.w = 1.0f;
						bool isInside = true;
						for(UINT k = 0; k < jl; ++k)
						{
							if(k != j && SMVector4Dot(vPoint, aPlanes[k]) > 0.0f)
							{
								isInside = false;
								break;
							}
						}
						if(isInside)
						{
							hasIntersection = true;
							cls = classifyPoint(vPoint, NULL, true);
							if(pisFullyVisible)
							{
								*pisFullyVisible = false;
							}
							if(cls >= 0)
							{
								mem_release(pFrustum);
								return(true);
							}
							break;
						}
					}
				}
			}
		}

		vPoint = pFrustum->getPoint(0);
		mem_release(pFrustum);

		/*if(!hasIntersection)
		{
			if(vPoint.x >= aabb.vMin.x && vPoint.x <= aabb.vMax.x
				&& vPoint.y >= aabb.vMin.y && vPoint.y <= aabb.vMax.y
				&& vPoint.z >= aabb.vMin.z && vPoint.z <= aabb.vMax.z)
			{
				// if one frustum point is inside the aabb, the all the frustum is inside, so the aabb is visible
				hasVisiblePoints = true;
			}

			if(pisFullyVisible)
			{
				*pisFullyVisible = false;
			}

			return(hasVisiblePoints);
		}*/
	}
	for(UINT a = 0; a < ARRAYSIZE(aauEdges); ++a)
	{
		if(!aisPointValid[aauEdges[a][0]] || !aisPointValid[aauEdges[a][1]])
		{
			continue;
		}

		const float3 &vA = avPoints[aauEdges[a][0]];
		const float3 &vB = avPoints[aauEdges[a][1]];

		for(UINT x = 0; x < OCCLUSION_BUFFER_WIDTH; ++x)
		{
			for(UINT y = 0; y < OCCLUSION_BUFFER_HEIGHT; ++y)
			{
				auto &ph = m_ppPolyhedra[x][y];
				if(ph.isValid)
				{
					for(UINT i = 0; i < 5; ++i)
					{
						if(ph.planes[i].intersectLine(&vPoint, vA, vB))
						{
							vPoint.w = 1.0f;
							bool isInside = true;
							for(UINT j = 0; j < 5; ++j)
							{
								if(i != j && SMVector4Dot(vPoint, ph.planes[j]) > 0.0f)
								{
									isInside = false;
									break;
								}
							}

							if(isInside)
							{
								cls = classifyPoint(vPoint);
								if(cls > 0)
								{
									if(!pisFullyVisible)
									{
										return(true);
									}
									if(hasHiddenPoints)
									{
										*pisFullyVisible = false;
										return(true);
									}
									hasVisiblePoints = true;
								}
								else if(cls < 0)
								{
									if(hasVisiblePoints)
									{
										*pisFullyVisible = false;
										return(true);
									}
									hasHiddenPoints = true;
								}
							}
						}
					}
				}
			}
		}
	}
	
	

	if(pisFullyVisible)
	{
		*pisFullyVisible = !hasHiddenPoints;
	}

	return(hasVisiblePoints);
}

void COcclusionCuller::_update()
{
	for(UINT x = 0; x < OCCLUSION_BUFFER_WIDTH; ++x)
	{
		for(UINT y = 0; y < OCCLUSION_BUFFER_HEIGHT; ++y)
		{
			m_ppfOcclusionBuffer[x][y] = FLT_MAX;
		}
	}

	float3 vStart = m_pCamera->getPosition();
	float3 vDir = SMVector3Normalize(m_pCamera->getLook());
	float3 vRight = m_pCamera->getRight();
	float3 vUp = m_pCamera->getUp();

	float3 vCenter = vStart + vDir * m_fFarPlane;
	float fHalfHeight = tanf(m_fFOV * 0.5f) * m_fFarPlane;
	float fHalfWidth = fHalfHeight * m_fAspectRatio;

//	float3 vUpRight = vCenter + vUp * fHalfHeight + vRight * fHalfWidth;
//	float3 vUpLeft = vCenter + vUp * fHalfHeight - vRight * fHalfWidth;
//	float3 vDnRight = vCenter - vUp * fHalfHeight + vRight * fHalfWidth;
//	float3 vDnLeft = vCenter - vUp * fHalfHeight - vRight * fHalfWidth;
//
//	float3 vEnd = vDnRight;
//	
//	btCollisionWorld::AllHitsRayResultCallback cb(F3_BTVEC(vStart), F3_BTVEC(vEnd));
//	SPhysics_GetDynWorld()->rayTest(F3_BTVEC(vStart), F3_BTVEC(vEnd), cb);

	
	for(UINT x = 0; x <= OCCLUSION_BUFFER_WIDTH; ++x)
	{
		float3 vTmp = vCenter + vRight * lerpf(-fHalfWidth, fHalfWidth, (float)x / OCCLUSION_BUFFER_WIDTH);
		for(UINT y = 0; y <= OCCLUSION_BUFFER_HEIGHT; ++y)
		{
			CAllHitsRayResultCallback &cb = cbs[x][y];
			cb.m_aResults.clearFast();
			cb.m_rayToWorld = vTmp + vUp * lerpf(fHalfHeight, -fHalfHeight, (float)y / OCCLUSION_BUFFER_HEIGHT);

			m_pPhysWorld->rayTest(vStart, cb.m_rayToWorld, &cb);

			SMPLANE planeTop = SMPLANE(vStart, cb.m_rayToWorld, cb.m_rayToWorld + vRight);
			SMPLANE planeLeft = SMPLANE(vStart, cb.m_rayToWorld, cb.m_rayToWorld + vUp);

			if(x != OCCLUSION_BUFFER_WIDTH && y != OCCLUSION_BUFFER_HEIGHT)
			{
				m_ppPolyhedra[x][y].isValid = false;
				m_ppPolyhedra[x][y].planes[PLANE_LEFT] = planeLeft;
				m_ppPolyhedra[x][y].planes[PLANE_TOP] = planeTop;
			}

			if(x != 0 && y < OCCLUSION_BUFFER_HEIGHT)
			{
				m_ppPolyhedra[x - 1][y].planes[PLANE_RIGHT] = -planeLeft;
			}

			if(y != 0 && x < OCCLUSION_BUFFER_WIDTH)
			{
				m_ppPolyhedra[x][y - 1].planes[PLANE_BOTTOM] = -planeTop;
			}

#if 0
			if(m_pLightSystem && false)
			{
				UINT uLightKey = x * OCCLUSION_BUFFER_HEIGHT + y;
				if(m_apTemp.size() <= uLightKey)
				{
					m_apTemp[uLightKey] = m_pLightSystem->newPoint();
					m_apTemp[uLightKey]->setColor(float3(1.0f, 0.0f, 0.0f));
				}

				if(cb.hasHit())
				{
					float fHitFraction = FLT_MAX;
					int idx = -1;
					for(UINT i = 0, l = cb.m_hitFractions.size(); i < l; ++i)
					{
						if(cb.m_hitFractions[i] < fHitFraction)
						{
							fHitFraction = cb.m_hitFractions[i];
							idx = (int)i;
						}
					}
					if(idx >= 0)
					{
						auto lp = BTVEC_F3(cb.m_hitPointWorld[idx]) + SMVector3Normalize(BTVEC_F3(cb.m_hitNormalWorld[idx])) * 0.1f;
						m_apTemp[uLightKey]->setPosition(lp);
						m_apTemp[uLightKey]->setEnabled(true);
					}
				}
				else
				{
					m_apTemp[uLightKey]->setEnabled(false);
				}
			}
#endif
		}
	}

	bool isFound = false;
	for(UINT x = 0; x < OCCLUSION_BUFFER_WIDTH; ++x)
	{
		for(UINT y = 0; y < OCCLUSION_BUFFER_HEIGHT; ++y)
		{
			CAllHitsRayResultCallback &cb0 = cbs[x][y];
			CAllHitsRayResultCallback &cb1 = cbs[x + 1][y];
			CAllHitsRayResultCallback &cb2 = cbs[x][y + 1];
			CAllHitsRayResultCallback &cb3 = cbs[x + 1][y + 1];
			if(!cb0.hasHit() || !cb1.hasHit() || !cb2.hasHit() || !cb3.hasHit())
			{
				continue;
			}

			fora(idx0, cb0.m_aResults)
			{
				auto &obj = cb0.m_aResults[idx0];

				int idx1 = cb1.m_aResults.indexOf(obj, [](const XRayResult &a, const XRayResult &b){
					return(a.pCollisionObject == b.pCollisionObject);
				});
				if(idx1 < 0)
				{
					continue;
				}

				int idx2 = cb2.m_aResults.indexOf(obj, [](const XRayResult &a, const XRayResult &b){
					return(a.pCollisionObject == b.pCollisionObject);
				});
				if(idx2 < 0)
				{
					continue;
				}

				int idx3 = cb3.m_aResults.indexOf(obj, [](const XRayResult &a, const XRayResult &b){
					return(a.pCollisionObject == b.pCollisionObject);
				});
				if(idx3 < 0)
				{
					continue;
				}

				// something found!
				
				float z[] = {
					fabs(SMVector3Dot(vDir, (cb0.m_rayToWorld - vStart) * cb0.m_aResults[idx0].fHitFraction)),
					fabs(SMVector3Dot(vDir, (cb1.m_rayToWorld - vStart) * cb1.m_aResults[idx1].fHitFraction)),
					fabs(SMVector3Dot(vDir, (cb2.m_rayToWorld - vStart) * cb2.m_aResults[idx2].fHitFraction)),
					fabs(SMVector3Dot(vDir, (cb3.m_rayToWorld - vStart) * cb3.m_aResults[idx3].fHitFraction))
				};

				int maxIdx = 0;
				float maxZ = z[maxIdx];
				for(int i = 1; i < ARRAYSIZE(z); ++i)
				{
					if(maxZ < z[i])
					{
						maxZ = z[i];
						maxIdx = i;
					}
				}

				if(m_ppfOcclusionBuffer[x][y] > maxZ)
				{
					float3 zPos[] = {
						cb0.m_aResults[idx0].vHitPoint,
						cb1.m_aResults[idx1].vHitPoint,
						cb2.m_aResults[idx2].vHitPoint,
						cb3.m_aResults[idx3].vHitPoint
					};
					m_ppfOcclusionBuffer[x][y] = maxZ;
					m_ppPolyhedra[x][y].isValid = true;

					zPos[maxIdx] = zPos[maxIdx] + vDir * 0.01f; // some bias

					m_ppPolyhedra[x][y].planes[PLANE_FRONT] = SMPLANE(zPos[maxIdx], zPos[maxIdx] + vUp, zPos[maxIdx] + vRight);
				}
			}
		}
	}
}

void COcclusionCuller::_print()
{
	for(UINT y = 0; y < OCCLUSION_BUFFER_HEIGHT; ++y)
	{
		for(UINT x = 0; x < OCCLUSION_BUFFER_WIDTH; ++x)
		{
			if(m_ppfOcclusionBuffer[x][y] > 99.0f)
			{
				printf("...");
			}
			else
			{
				int val = (int)(m_ppfOcclusionBuffer[x][y]);
				if(val > 99)
				{
					val = 99;
				}
				printf("%3d", val);
			}
		}
		printf("\n");
	}
	printf("\n");
}

int COcclusionCuller::classifyPoint(const float3 &vPoint, bool *pisCulledByFrustum, bool bIgnoreFrustum) const
{

	bool isOnSurface = false;
	
	if(!bIgnoreFrustum)
	{
		IXFrustum *pFrustum = m_pCamera->getFrustum();
		for(UINT i = 0, l = pFrustum->getPlaneCount(); i < l; ++i)
		{
			float f = SMVector4Dot(float4(vPoint, 1.0f), pFrustum->getPlaneAt(i));
			if(SMIsZero(f))
			{
				isOnSurface = true;
				continue;
			}
			if(f < -0.0001f)
			{
				if(pisCulledByFrustum)
				{
					*pisCulledByFrustum = true;
				}
				mem_release(pFrustum);
				return(-1);
			}
		}
		mem_release(pFrustum);
	}
	if(pisCulledByFrustum)
	{
		*pisCulledByFrustum = false;
	}


	for(UINT x = 0; x < OCCLUSION_BUFFER_WIDTH; ++x)
	{
		for(UINT y = 0; y < OCCLUSION_BUFFER_HEIGHT; ++y)
		{
			auto &ph = m_ppPolyhedra[x][y];
			if(ph.isValid)
			{
				bool isInside = true;
				bool isOnSurfaceLocal = false;
				for(UINT i = 0; i < 5; ++i)
				{
					float f = SMVector4Dot(float4(vPoint, 1.0f), ph.planes[i]);
					if(SMIsZero(f))
					{
						isOnSurfaceLocal = true;
						continue;
					}
					if(f > 0.0f)
					{
						isInside = false;
						break;
					}
				}
				if(isInside)
				{
					if(isOnSurfaceLocal)
					{
						isOnSurface = true;
					}
					else
					{
						return(-1);
					}
				}
			}
		}
	}
	return(isOnSurface ? 0 : 1);
}
