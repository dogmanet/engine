#include "BrushMesh.h"
#include <xcommon/resource/IXResourceManager.h>
#include <xcommon/resource/IXModelProvider.h>
#include <xcommon/IPluginManager.h>

#undef SMIsZero
#define SMIsZero(ff) (fabsf(ff) < 0.0001f)

CBrushMesh::CBrushMesh(IXCore *pCore, COutline *pOutline, UINT uContour, const char *szMat, float fHeight):
	m_pCore(pCore)
{
	//m_pHandle[0] = NULL;
	//m_pHandle[1] = NULL;
	//m_aMaterials.push_back("LAB1_FLOOR4");
	//m_aMaterials.push_back("C1A3WALL04");
	// m_aMaterials.push_back("dev_floor");
	// m_aMaterials.push_back("dev_wall");


	m_pPhysics = (IXPhysics*)m_pCore->getPluginManager()->getInterface(IXPHYSICS_GUID);

	m_aMaterials.push_back(szMat);

	//m_aMaterials.push_back("block_grass_top");
	//m_aMaterials.push_back("block_grass_side");
	setupFromOutline(pOutline, uContour, fHeight);

}

CBrushMesh::CBrushMesh(IXCore *pCore):
	m_pCore(pCore)
{
	m_pPhysics = (IXPhysics*)m_pCore->getPluginManager()->getInterface(IXPHYSICS_GUID);
}

CBrushMesh::~CBrushMesh()
{
	if(m_pRigidBody && m_isPhysicsLoaded)
	{
		m_pPhysics->getWorld()->removeCollisionObject(m_pRigidBody);
	}
	mem_release(m_pRigidBody);
	mem_release(m_pModel);
}

void CBrushMesh::enable(bool yesNo)
{
	SAFE_CALL(m_pModel, enable, yesNo);
	if(m_pRigidBody)
	{
		if(yesNo)
		{
			if(!m_isPhysicsLoaded)
			{
				m_pPhysics->getWorld()->addCollisionObject(m_pRigidBody, CG_STATIC, CG_STATIC_MASK);
				m_isPhysicsLoaded = true;
			}
		}
		else if(m_isPhysicsLoaded)
		{
			m_pPhysics->getWorld()->removeCollisionObject(m_pRigidBody);
			m_isPhysicsLoaded = false;
		}
	}
}

void CBrushMesh::buildMesh(CMeshBuilder *pBuilder)
{
	Array<UINT> aSubsets(m_aMaterials.size());
	fora(i, m_aMaterials)
	{
		aSubsets[i] = pBuilder->getSubsetIndexForMaterial(m_aMaterials[i].c_str());
	}

	fora(i, m_aFaces)
	{
		Face &face = m_aFaces[i];
		if(face.isInternal)
		{
			continue;
		}
		TextureInfo &texInfo = face.texInfo;
		Subset &subset = pBuilder->getSubset(aSubsets[texInfo.uMatId]);

		bool isFirst = true;
		UINT uFirstIdx;
		UINT uNextIdx;
		UINT uLastVtx = UINT_MAX;

		Edge &e0 = m_aEdges[face.aEdges[0]];
		Edge &e1 = m_aEdges[face.aEdges[1]];
		if(e0.uVertex[0] == e1.uVertex[0] || e0.uVertex[0] == e1.uVertex[1])
		{
			std::swap(e0.uVertex[0], e0.uVertex[1]);
		}

		for(UINT j = 0, jl = face.aEdges.size() - 1; j < jl; ++j)
		{
			Edge edge = m_aEdges[face.aEdges[j]];
			if(edge.uVertex[0] != uLastVtx && uLastVtx != UINT_MAX)
			{
				std::swap(edge.uVertex[0], edge.uVertex[1]);
			}
			uLastVtx = edge.uVertex[1];

			XResourceModelStaticVertex vtx = {};
			vtx.vNorm = face.vNormal;
			vtx.vTangent = texInfo.vS;
			vtx.vBinorm = SMVector3Normalize(SMVector3Cross(vtx.vTangent, vtx.vNorm));
			vtx.vTangent = SMVector3Normalize(SMVector3Cross(vtx.vNorm, vtx.vBinorm));

			if(isFirst)
			{
				isFirst = false;

				vtx.vPos = m_aVertices[edge.uVertex[0]];
				vtx.vTex.x = (SMVector3Dot(vtx.vPos, texInfo.vS) / texInfo.fSScale + texInfo.fSShift);
				vtx.vTex.y = (SMVector3Dot(vtx.vPos, texInfo.vT) / texInfo.fTScale + texInfo.fTShift);

				uFirstIdx = subset.aVertices.size();
				subset.aVertices.push_back(vtx);

				vtx.vPos = m_aVertices[edge.uVertex[1]];
				vtx.vTex.x = (SMVector3Dot(vtx.vPos, texInfo.vS) / texInfo.fSScale + texInfo.fSShift);
				vtx.vTex.y = (SMVector3Dot(vtx.vPos, texInfo.vT) / texInfo.fTScale + texInfo.fTShift);

				uNextIdx = subset.aVertices.size();
				subset.aVertices.push_back(vtx);

				// place first vertex, save idx
				// place second vertex, save idx
			}
			else
			{
				vtx.vPos = m_aVertices[edge.uVertex[1]];
				vtx.vTex.x = (SMVector3Dot(vtx.vPos, texInfo.vS) / texInfo.fSScale + texInfo.fSShift);
				vtx.vTex.y = (SMVector3Dot(vtx.vPos, texInfo.vT) / texInfo.fTScale + texInfo.fTShift);

				subset.aIndices.push_back(uFirstIdx);
				subset.aIndices.push_back(uNextIdx);

				uNextIdx = subset.aVertices.size();
				subset.aVertices.push_back(vtx);
				subset.aIndices.push_back(uNextIdx);

				// place new vertex, place 3 indices, save idx
			}
		}
	}

}

void CBrushMesh::buildModel(bool bBuildPhysbox)
{
	mem_release(m_pModel);

	if(m_isFinalized)
	{
		enable(false);
		return;
	}

	CMeshBuilder meshBuilder;
	buildMesh(&meshBuilder);

	// create model
	IXResourceManager *pRM = m_pCore->getResourceManager();
	IXResourceModelStatic *pResource = pRM->newResourceModelStatic();

	meshBuilder.buildResource(pResource);

	IXModelProvider *pProvider = (IXModelProvider*)m_pCore->getPluginManager()->getInterface(IXMODELPROVIDER_GUID);
	pProvider->createStaticModel(pResource, &m_pModel);
	mem_release(pResource);

	SAFE_CALL(m_pModel, setColor, (float4)m_vColor);

	if(bBuildPhysbox)
	{
		if(m_pRigidBody && m_isPhysicsLoaded)
		{
			m_pPhysics->getWorld()->removeCollisionObject(m_pRigidBody);
		}
		mem_release(m_pRigidBody);

		IXConvexHullShape *pShape;
		m_pPhysics->newConvexHullShape(m_aVertices.size(), m_aVertices, &pShape, sizeof(float3_t), false);

		XRIDIGBODY_DESC desc;
		desc.pCollisionShape = pShape;
		m_pPhysics->newRigidBody(desc, &m_pRigidBody);
		mem_release(pShape);

		m_pPhysics->getWorld()->addCollisionObject(m_pRigidBody, CG_STATIC, CG_STATIC_MASK);
		m_isPhysicsLoaded = true;
	}
}

float3 CBrushMesh::GetNearestAxis(const float3 &vNormal)
{
	float3 vTmp = SMVectorAbs(vNormal);
	if(vTmp.y >= vTmp.x && vTmp.y >= vTmp.z)
	{
		return(float3(0.0f, vNormal.y / vTmp.y, 0.0f));
	}
	else if(vTmp.x >= vTmp.y && vTmp.x >= vTmp.z)
	{
		return(float3(vNormal.x / vTmp.x, 0.0f, 0.0f));
	}
	return(float3(0.0f, 0.0f, vNormal.z / vTmp.z));
}

float3 CBrushMesh::GetTangent(const float3 &vNormal)
{
	if(fabsf(SMVector3Dot(vNormal, float3(0.0f, 1.0f, 0.0f))) > cosf(SM_PIDIV4))
	{
		return(float3(0.0f, 0.0f, 1.0f));
	}
	return(float3(0.0f, -1.0f, 0.0f));
}

float3 CBrushMesh::ProjectToFace(const float3 &vNormal, const float3 &vIn)
{
	return(vIn - SMVector3Dot(vIn, vNormal) * vNormal);
}

void CBrushMesh::setupFromOutline(COutline *pOutline, UINT uContour, float fHeight)
{
	const CContour &c = pOutline->getContour(uContour);
	UINT uOutlinePts = pOutline->getPointCount();
	UINT uPts = c.getPointCount();

	//const float c_fDefaultWidth = 1.2f;

	m_aVertices.resize(uPts * 2);
	m_aEdges.resize(uPts * 3);
	m_aFaces.resize(uPts + 2);

	float3 vNormal = pOutline->getNormal();
	float3 vOffset = vNormal * fHeight;


	Face &f1 = m_aFaces[uPts + 0];
	Face &f2 = m_aFaces[uPts + 1];

	f1.texInfo = {};
	f2.texInfo = {};
	f1.vNormal = (float3)-vNormal;
	f2.vNormal = vNormal;
	f1.texInfo.vT = GetTangent(GetNearestAxis(f1.vNormal));
	f1.texInfo.vS = SMVector3Cross(f1.texInfo.vT, GetNearestAxis(f1.vNormal));
	f1.texInfo.fSScale = 1.0f;
	f1.texInfo.fTScale = 1.0f;
	f1.isInternal = false;

	f2.texInfo.vT = GetTangent(GetNearestAxis(f2.vNormal));
	f2.texInfo.vS = SMVector3Cross(f2.texInfo.vT, GetNearestAxis(f2.vNormal));
	f2.texInfo.fSScale = 1.0f;
	f2.texInfo.fTScale = 1.0f;
	f2.isInternal = false;

	f1.aEdges.reserve(uPts);
	f2.aEdges.reserve(uPts);

	for(UINT i = 0; i < uPts; ++i)
	{
		m_aVertices[i] = pOutline->getPoint(c.getPoint(i));
		m_aVertices[i + uPts] = (float3)(m_aVertices[i] + vOffset);
	}

	for(UINT i = 0; i < uPts; ++i)
	{
		bool isInternal = (c.getPoint(i) + 1) % uOutlinePts != c.getPoint(i + 1);

		Edge &e = m_aEdges[i];
		e.uVertex[0] = i;
		e.uVertex[1] = (i + 1) % uPts;
		e.isInternal = isInternal;

		Edge &e2 = m_aEdges[i + uPts];
		e2.uVertex[0] = i + uPts;
		e2.uVertex[1] = (i + 1) % uPts + uPts;
		e2.isInternal = isInternal;

		Edge &e3 = m_aEdges[i + uPts * 2];
		e3.uVertex[0] = i;
		e3.uVertex[1] = i + uPts;
		e3.isInternal = false;

		// m_aVertices[i] = pOutline->getPoint(c.getPoint(i));
		// m_aVertices[i + uPts] = (float3)(m_aVertices[i] + vOffset);

		//if(!isInternal)
		{
			Face &f = m_aFaces[i];
			f.isInternal = isInternal;
			f.aEdges.reserve(4);
			f.aEdges.push_back(i + uPts * 2);
			f.aEdges.push_back(i + uPts);
			f.aEdges.push_back((i + 1) % uPts + uPts * 2);
			f.aEdges.push_back(i);

			float3 v1 = (e3.uVertex[0] == e2.uVertex[0] || e3.uVertex[0] == e2.uVertex[1])
				? m_aVertices[e3.uVertex[1]] - m_aVertices[e3.uVertex[0]]
				: m_aVertices[e3.uVertex[0]] - m_aVertices[e3.uVertex[1]];

			float3 v0 = (e2.uVertex[0] == e3.uVertex[0] || e2.uVertex[0] == e3.uVertex[1])
				? m_aVertices[e2.uVertex[1]] - m_aVertices[e2.uVertex[0]]
				: m_aVertices[e2.uVertex[0]] - m_aVertices[e2.uVertex[1]];

			f.texInfo = {};
			f.vNormal = SMVector3Normalize(SMVector3Cross(v0, v1));
			f.texInfo.vT = GetTangent(GetNearestAxis(f.vNormal));
			f.texInfo.vS = SMVector3Cross(f.texInfo.vT, GetNearestAxis(f.vNormal));
			f.texInfo.fSScale = 1.0f;
			f.texInfo.fTScale = 1.0f;
			f.texInfo.uMatId = 0; //1;
		}

		f1.aEdges.push_back(i);
		f2.aEdges.push_back((uPts - i - 1) + uPts);
	}

	for(UINT i = 0, l = m_aFaces.size(); i < l; ++i)
	{
		if(m_aFaces[i].aEdges.size() == 0)
		{
			m_aFaces.erase(i);
			--i;
			--l;
		}
	}

	buildModel();

	m_isBoundDirty = true;
}

const SMAABB& CBrushMesh::getAABB()
{
	if(m_isBoundDirty)
	{
		m_isBoundDirty = false;
		float3 vMin = m_aVertices[0];
		float3 vMax = m_aVertices[0];
		for(UINT i = 1, l = m_aVertices.size(); i < l; ++i)
		{
			vMin = SMVectorMin(vMin, m_aVertices[i]);
			vMax = SMVectorMax(vMax, m_aVertices[i]);
		}
		m_aabb = SMAABB(vMin, vMax);
	}

	return(m_aabb);
}

void CBrushMesh::move(const float3 &vOffset)
{
	if(m_pModel)
	{
		m_pModel->setPosition(m_pModel->getPosition() + vOffset);
	}
	
	for(UINT i = 0, l = m_aVertices.size(); i < l; ++i)
	{
		m_aVertices[i] = (float3)(m_aVertices[i] + vOffset);
	}

	if(m_pRigidBody)
	{
		m_pRigidBody->setPosition(m_pRigidBody->getPosition() + vOffset);
	}

	for(UINT i = 0, l = m_aFaces.size(); i < l; ++i)
	{
		Face &f = m_aFaces[i];
		f.texInfo.fSShift -= SMVector3Dot(f.texInfo.vS, vOffset) / f.texInfo.fSScale;
		f.texInfo.fTShift -= SMVector3Dot(f.texInfo.vT, vOffset) / f.texInfo.fTScale;
	}
	
	m_aabb = m_aabb + vOffset;
}

void CBrushMesh::rotate(const float3 &vOrigin, const SMQuaternion &qOffset)
{
	if(m_pModel)
	{
		m_pModel->setOrientation(m_pModel->getOrientation() * qOffset);
		m_pModel->setPosition(qOffset * (m_pModel->getPosition() - vOrigin) + vOrigin);
	}

	if(m_pRigidBody)
	{
		m_pRigidBody->setRotation(m_pRigidBody->getRotation() * qOffset);
		m_pRigidBody->setPosition(qOffset * (m_pRigidBody->getPosition() - vOrigin) + vOrigin);
	}

	for(UINT i = 0, l = m_aVertices.size(); i < l; ++i)
	{
		m_aVertices[i] = (float3)(qOffset * (m_aVertices[i] - vOrigin) + vOrigin);
	}

	for(UINT i = 0, l = m_aFaces.size(); i < l; ++i)
	{
		Face &f = m_aFaces[i];

		f.texInfo.fSShift += SMVector3Dot(f.texInfo.vS, vOrigin);
		f.texInfo.fTShift += SMVector3Dot(f.texInfo.vT, vOrigin);

		f.vNormal = qOffset * f.vNormal;
		f.texInfo.vS = qOffset * f.texInfo.vS;
		f.texInfo.vT = qOffset * f.texInfo.vT;

		f.texInfo.fSShift -= SMVector3Dot(f.texInfo.vS, vOrigin);
		f.texInfo.fTShift -= SMVector3Dot(f.texInfo.vT, vOrigin);
	}


	m_isBoundDirty = true;


}

void CBrushMesh::resize(const float3 &vOrigin, const float3 &vRelativeSize)
{
	for(UINT i = 0, l = m_aVertices.size(); i < l; ++i)
	{
		m_aVertices[i] = (float3)(vRelativeSize * (m_aVertices[i] - vOrigin) + vOrigin);
	}

	//! TODO: add support for texture scale lock

	/*for(UINT i = 0, l = m_aFaces.size(); i < l; ++i)
	{
		Face &f = m_aFaces[i];
		f.texInfo.vS = (float3)(f.texInfo.vS / vRelativeSize);
		f.texInfo.vT = (float3)(f.texInfo.vT / vRelativeSize);
	}*/

	buildModel();

	m_isBoundDirty = true;
}

bool CBrushMesh::rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut, float3 *pvNormal, bool isRayInWorldSpace, bool bReturnNearestPoint)
{
	float3 vPos;
	fora(i, m_aFaces)
	{
		Face &f = m_aFaces[i];
		SMPLANE plane(f.vNormal, m_aVertices[m_aEdges[f.aEdges[0]].uVertex[0]]);
		
		if(SMVector4Dot(float4(vStart, 1.0f), plane) >= 0.0f && plane.intersectLine(&vPos, vStart, vEnd))
		{
			bool isInternal = true;
			fora(j, m_aFaces)
			{
				if(i != j)
				{
					Face &f2 = m_aFaces[j];
					SMPLANE plane2(f2.vNormal, m_aVertices[m_aEdges[f2.aEdges[0]].uVertex[0]]);
					if(SMVector4Dot(float4(vPos, 1.0f), plane2) > 0.0f)
					{
						isInternal = false;
						break;
					}
				}
			}
			if(isInternal)
			{
				if(pvOut)
				{
					//! Found point is always nearest!
					*pvOut = vPos;
				}
				if(pvNormal)
				{
					*pvNormal = f.vNormal;
				}
				return(true);
			}
		}
		
	}
	return(false);
}

void CBrushMesh::renderSelection(bool is3D, IXGizmoRenderer *pGizmoRenderer, CLIP_PLANE_STATE clipPlaneState, const SMPLANE &clipPlane)
{
	const float4 c_vLineSplit = float4(1.0f, 1.0f, 1.0f, 1.0f);
	const float4 c_vLineNormal = clipPlaneState == CPS_TWOSIDE ? c_vLineSplit : float4(1.0f, 1.0f, 0.0f, 1.0f);
	pGizmoRenderer->setColor(c_vLineNormal);
	pGizmoRenderer->setLineWidth(is3D ? 0.02f : clipPlaneState == CPS_NONE ? 1.0f : 2.0f);
	float3 p0, p1, p2;
	for(UINT i = 0, l = m_aEdges.size(); i < l; ++i)
	{
		Edge &e = m_aEdges[i];
		if(!e.isInternal)
		{
			if(clipPlaneState != CPS_ONESIDE)
			{
				pGizmoRenderer->jumpTo(m_aVertices[e.uVertex[0]]);
				pGizmoRenderer->lineTo(m_aVertices[e.uVertex[1]]);
			}
			else
			{
				p0 = m_aVertices[e.uVertex[0]];
				p1 = m_aVertices[e.uVertex[1]];

				if(!SMIsZero(SMVector3Dot(p0 - p1, clipPlane)) && clipPlane.intersectLine(&p2, p0, p1))
				{
					float fDist = SMVector4Dot(float4(p0, 1.0f), clipPlane);
					bool f = fDist > 0.0f;
					if(SMIsZero(fDist))
					{
						fDist = SMVector4Dot(float4(p1, 1.0f), clipPlane);
						f = fDist < 0.0f;
					}
					pGizmoRenderer->setColor(f ? c_vLineSplit : c_vLineNormal);
					pGizmoRenderer->jumpTo(p0);
					pGizmoRenderer->lineTo(p2);
					pGizmoRenderer->setColor(!f ? c_vLineSplit : c_vLineNormal);
					pGizmoRenderer->jumpTo(p2);
					pGizmoRenderer->lineTo(p1);
				}
				else
				{
					float fDist = SMVector4Dot(float4(p0, 1.0f), clipPlane);
					bool f = fDist >= 0.0f;
					if(SMIsZero(fDist))
					{
						fDist = SMVector4Dot(float4(p1, 1.0f), clipPlane);
						f = fDist >= 0.0f;
					}

					pGizmoRenderer->setColor(f ? c_vLineSplit : c_vLineNormal);
					pGizmoRenderer->jumpTo(p0);
					pGizmoRenderer->lineTo(p1);
				}
			}
		}
	}

	if(clipPlaneState != CPS_NONE)
	{
		// draw new edges
		for(UINT i = 0, l = m_aFaces.size(); i < l; ++i)
		{
			Face &face = m_aFaces[i];
			if(face.isInternal)
			{
				continue;
			}

			if(SMIsZero(1.0f - fabsf(SMVector3Dot(face.vNormal, clipPlane))))
			{
				continue;
			}

			float3 p0, p1;
			bool isFound0 = false;
			bool isFound1 = false;
			for(UINT j = 0, jl = face.aEdges.size(); j < jl; ++j)
			{
				Edge edge = m_aEdges[face.aEdges[j]];
				if(clipPlane.intersectLine(isFound0 ? &p1 : &p0, m_aVertices[edge.uVertex[0]], m_aVertices[edge.uVertex[1]]))
				{
					if(isFound0 && !SMIsZero(SMVector3Length2(p0 - p1)))
					{
						isFound1 = true;
						break;
					}
					else
					{
						isFound0 = true;
					}
				}
			}

			if(isFound1)
			{
				pGizmoRenderer->setColor(c_vLineSplit);
				pGizmoRenderer->jumpTo(p0);
				pGizmoRenderer->lineTo(p1);
			}
		}
	}

	if(is3D)
	{
		pGizmoRenderer->setColor(float4(1.0f, 0.0f, 0.0f, 0.15f));
		// generate polygons
		for(UINT i = 0, l = m_aFaces.size(); i < l; ++i)
		{
			Face &face = m_aFaces[i];
			if(face.isInternal)
			{
				continue;
			}
			float3 vS;

			vS = face.vNormal * 0.005f;

			triangulateFace(i, [&](const float3 &vA, const float3 &vB, const float3 &vC){
				pGizmoRenderer->drawPoly((float3)(vA + vS), (float3)(vB + vS), (float3)(vC + vS));
				return(true);
			});
		}
	}
}

void CBrushMesh::renderFace(IXGizmoRenderer *pGizmoRenderer, UINT uFace)
{
	assert(uFace < m_aFaces.size());
	//pGizmoRenderer->setColor(float4(1.0f, 0.0f, 0.0f, 0.15f));

	Face &face = m_aFaces[uFace];
	float3 vS;

	vS = face.vNormal * 0.005f;

	triangulateFace(uFace, [&](const float3 &vA, const float3 &vB, const float3 &vC){
		pGizmoRenderer->drawPoly((float3)(vA + vS), (float3)(vB + vS), (float3)(vC + vS));
		return(true);
	});
}

void CBrushMesh::serialize(Array<char> *paData)
{
	char tmp[256];
	paData->append("{\"v\":[");
	for(UINT i = 0, l = m_aVertices.size(); i < l; ++i)
	{
		const float3_t &v = m_aVertices[i];
		sprintf(tmp, "%s[%g,%g,%g]", i == 0 ? "" : ",", v.x, v.y, v.z);
		paData->append(tmp);
	}
	paData->append("],\"e\":[");
	for(UINT i = 0, l = m_aEdges.size(); i < l; ++i)
	{
		const Edge &e = m_aEdges[i];
		sprintf(tmp, "%s[%u,%u,%u]", i == 0 ? "" : ",", e.uVertex[0], e.uVertex[1], e.isInternal ? 1 : 0);
		paData->append(tmp);
	}
	paData->append("],\"f\":[");
	for(UINT i = 0, l = m_aFaces.size(); i < l; ++i)
	{
		const Face &f = m_aFaces[i];
		const TextureInfo &t = f.texInfo;
		if(i != 0)
		{
			paData->push_back(',');
		}
		paData->append("{\"t\":{");
		sprintf(tmp, "\"s\":[%g,%g,%g,%g,%g],\"t\":[%g,%g,%g,%g,%g],\"m\":%u", 
			t.vS.x, t.vS.y, t.vS.z, t.fSShift, t.fSScale,
			t.vT.x, t.vT.y, t.vT.z, t.fTShift, t.fTScale,
			t.uMatId
			);
		paData->append(tmp);
		paData->append("},\"e\":[");
		for(UINT j = 0, jl = f.aEdges.size(); j < jl; ++j)
		{
			sprintf(tmp, "%s%u", j == 0 ? "" : ",", f.aEdges[j]);
			paData->append(tmp);
		}
		paData->append("],\"n\":[");
		sprintf(tmp, "%g,%g,%g", f.vNormal.x, f.vNormal.y, f.vNormal.z);
		paData->append(tmp);
		paData->append("],\"i\":");
		sprintf(tmp, "%d", f.isInternal ? 1 : 0);
		paData->append(tmp);
		paData->append("}");

		// sprintf(tmp, "%s[%u,%u,%u]", i == 0 ? "" : ",", e.uVertex[0], e.uVertex[1], e.isInternal ? 1 : 0);
		// paData->append(tmp);
	}
	paData->append("],\"m\":[");
	for(UINT i = 0, l = m_aMaterials.size(); i < l; ++i)
	{
		if(i != 0)
		{
			paData->push_back(',');
		}
		paData->push_back('"');
		paData->append(m_aMaterials[i].c_str());
		paData->push_back('"');
	}
	paData->append("]");
	paData->append("}");
	// m_aVertices
}

bool CBrushMesh::deserialize(IXJSONObject *pData)
{
	assert(pData);
	if(!pData)
	{
		return(false);
	}

	IXJSONArray *pArr, *pVec;
	IXJSONObject *pFace, *pTI;
	int64_t a, b, c;

	for(UINT i = 0, l = pData->size(); i < l; ++i)
	{
		const char *pKey = pData->getKey(i);
		switch(pKey[0])
		{
		case 'v':
			pArr = pData->at(i)->asArray();
			if(pArr)
			{
				m_aVertices.resize(pArr->size());
				for(UINT j = 0, jl = pArr->size(); j < jl; ++j)
				{
					pVec = pArr->at(j)->asArray();
					if(!pVec || pVec->size() != 3)
					{
						return(false);
					}

					float3_t &vec = m_aVertices[j];

					if(!(pVec->at(0)->getFloat(&vec.x) &&
						pVec->at(1)->getFloat(&vec.y) &&
						pVec->at(2)->getFloat(&vec.z)))
					{
						return(false);
					}
				}
			}
			break;
		case 'e':
			pArr = pData->at(i)->asArray();
			if(pArr)
			{
				m_aEdges.resize(pArr->size());
				for(UINT j = 0, jl = pArr->size(); j < jl; ++j)
				{
					pVec = pArr->at(j)->asArray();
					if(!pVec || pVec->size() != 3)
					{
						return(false);
					}

					Edge &e = m_aEdges[j];
					if(!(pVec->at(0)->getInt64(&a) &&
						pVec->at(1)->getInt64(&b) &&
						pVec->at(2)->getInt64(&c)))
					{
						return(false);
					}

					e.uVertex[0] = (UINT)a;
					e.uVertex[1] = (UINT)b;
					e.isInternal = c != 0;
				}
			}
			break;
		case 'f':
			pArr = pData->at(i)->asArray();
			if(pArr)
			{
				m_aFaces.resize(pArr->size());
				for(UINT j = 0, jl = pArr->size(); j < jl; ++j)
				{
					pFace = pArr->at(j)->asObject();
					if(!pFace)
					{
						return(false);
					}
					Face &f = m_aFaces[j];
					f.isInternal = false;

					for(UINT k = 0, kl = pFace->size(); k < kl; ++k)
					{
						pKey = pFace->getKey(k);
						switch(pKey[0])
						{
						case 't':
							// s t m
							pTI = pFace->at(k)->asObject();
							if(!pTI)
							{
								return(false);
							}

							for(UINT s = 0, sl = pTI->size(); s < sl; ++s)
							{
								pKey = pTI->getKey(s);
								switch(pKey[0])
								{
								case 's':
								case 't':
									pVec = pTI->at(s)->asArray();
									if(!pVec || (pVec->size() != 4 && pVec->size() != 5))
									{
										return(false);
									}

									{
										float3_t &vec = pKey[0] == 's' ? f.texInfo.vS : f.texInfo.vT;
										float &fShift = pKey[0] == 's' ? f.texInfo.fSShift : f.texInfo.fTShift;
										float &fScale = pKey[0] == 's' ? f.texInfo.fSScale : f.texInfo.fTScale;

										if(!(pVec->at(0)->getFloat(&vec.x) &&
											pVec->at(1)->getFloat(&vec.y) &&
											pVec->at(2)->getFloat(&vec.z) &&
											pVec->at(3)->getFloat(&fShift)))
										{
											return(false);
										}

										if(pVec->size() == 5)
										{
											pVec->at(4)->getFloat(&fScale);
										}
										else
										{
											fScale = 1.0f;
										}
									}

									break;
								case 'm':
									if(!pTI->at(s)->getInt64(&a))
									{
										return(false);
									}
									f.texInfo.uMatId = (UINT)a;
									break;
								}
							}

							break;

						case 'e':
							pVec = pFace->at(k)->asArray();
							if(!pVec || pVec->size() < 3)
							{
								return(false);
							}
							f.aEdges.resize(pVec->size());
							for(UINT s = 0, sl = pVec->size(); s < sl; ++s)
							{
								if(!pVec->at(s)->getInt64(&a))
								{
									return(false);
								}
								f.aEdges[s] = (UINT)a;
							}

							break;

						case 'n':
							pVec = pFace->at(k)->asArray();
							if(!pVec || pVec->size() != 3)
							{
								return(false);
							}
							else
							{
								float3_t &vec = f.vNormal;

								if(!(pVec->at(0)->getFloat(&vec.x) &&
									pVec->at(1)->getFloat(&vec.y) &&
									pVec->at(2)->getFloat(&vec.z)))
								{
									return(false);
								}
							}
							break;

						case 'i':
							f.isInternal = pFace->at(k)->asBool();
							break;
						}
					}
				}
			}
			break;
		case 'm':
			pArr = pData->at(i)->asArray();
			if(pArr)
			{
				m_aMaterials.resize(pArr->size());
				for(UINT j = 0, jl = pArr->size(); j < jl; ++j)
				{
					m_aMaterials[j] = pArr->at(j)->getString();
				}
			}
			break;
		}
	}

	buildModel();

	m_isBoundDirty = true;

	return(true);
}

bool CBrushMesh::findFace(const float3 &vRayStart, const float3 &vRayDir, UINT *puFace, float3 *pvFacePoint)
{
	if(!m_aVertices.size())
	{
		return(false);
	}

	float3 vA, vB, vC, vOut;

	float3 vRayEnd = vRayStart + (SMVector3Length(vRayStart - m_aVertices[0]) + SMVector3Length(m_aabb.vMax - m_aabb.vMin) + 1.0f) * SMVector3Normalize(vRayDir);
	for(UINT i = 0, l = m_aFaces.size(); i < l; ++i)
	{
		Face &face = m_aFaces[i];

		if(face.isInternal)
		{
			continue;
		}

		if(SMVector3Dot(face.vNormal, vRayDir) > 0.0f)
		{
			continue;
		}


		if(!triangulateFace(i, [&](const float3 &vA, const float3 &vB, const float3 &vC){
			if(SMTriangleIntersectLine(vA, vB, vC, vRayStart, vRayEnd, &vOut))
			{
				if(puFace)
				{
					*puFace = i;
				}
				if(pvFacePoint)
				{
					*pvFacePoint = vOut;
				}
				return(false);
			}
			return(true);
		}))
		{
			return(true);
		}
	}
	return(false);
}

UINT CBrushMesh::getFaceCount() const
{
	return(m_aFaces.size());
}

void CBrushMesh::getFaceInfo(UINT uFace, BrushFace *pOut)
{
	assert(uFace < m_aFaces.size());

	Face &f = m_aFaces[uFace];

	pOut->vN = f.vNormal;
	pOut->vS = f.texInfo.vS;
	pOut->vT = f.texInfo.vT;
	pOut->fSShift = f.texInfo.fSShift;
	pOut->fTShift = f.texInfo.fTShift;
	pOut->fSScale = f.texInfo.fSScale;
	pOut->fTScale = f.texInfo.fTScale;
	pOut->szMaterial = m_aMaterials[f.texInfo.uMatId].c_str();
}

void CBrushMesh::setFaceInfo(UINT uFace, const BrushFace &brushFace, bool bRebuild)
{
	assert(uFace < m_aFaces.size());

	Face &f = m_aFaces[uFace];

	f.texInfo.vS = brushFace.vS;
	f.texInfo.vT = brushFace.vT;
	f.texInfo.fSShift = brushFace.fSShift;
	f.texInfo.fTShift = brushFace.fTShift;
	f.texInfo.fSScale = brushFace.fSScale;
	f.texInfo.fTScale = brushFace.fTScale;

	if(fstrcmp(m_aMaterials[f.texInfo.uMatId].c_str(), brushFace.szMaterial))
	{
		int idx = m_aMaterials.indexOf(brushFace.szMaterial, [](const String &a, const char *b){
			return(!strcmp(a.c_str(), b));
		});
		if(idx < 0)
		{
			idx = (int)m_aMaterials.size();
			m_aMaterials.push_back(brushFace.szMaterial);
		}
		f.texInfo.uMatId = (UINT)idx;
	}

	if(bRebuild)
	{
		buildModel(false);
	}
}

void CBrushMesh::BuildExtents(Extents extents, const float3 &vC)
{
	if(vC.x < extents[GXCUBEMAP_FACE_NEGATIVE_X].x)
	{
		extents[GXCUBEMAP_FACE_NEGATIVE_X] = vC;
	}

	if(vC.x > extents[GXCUBEMAP_FACE_POSITIVE_X].x)
	{
		extents[GXCUBEMAP_FACE_POSITIVE_X] = vC;
	}

	if(vC.y < extents[GXCUBEMAP_FACE_NEGATIVE_Y].y)
	{
		extents[GXCUBEMAP_FACE_NEGATIVE_Y] = vC;
	}

	if(vC.y > extents[GXCUBEMAP_FACE_POSITIVE_Y].y)
	{
		extents[GXCUBEMAP_FACE_POSITIVE_Y] = vC;
	}

	if(vC.z < extents[GXCUBEMAP_FACE_NEGATIVE_Z].z)
	{
		extents[GXCUBEMAP_FACE_NEGATIVE_Z] = vC;
	}

	if(vC.z > extents[GXCUBEMAP_FACE_POSITIVE_Z].z)
	{
		extents[GXCUBEMAP_FACE_POSITIVE_Z] = vC;
	}
}

void CBrushMesh::getFaceExtents(UINT uFace, Extents extents)
{
	assert(uFace < m_aFaces.size());

	SMAABB aabb;
	UINT i = 0;
	triangulateFace(uFace, [&](const float3 &vA, const float3 &vB, const float3 &vC){
		if(i == 0)
		{
			for(UINT j = 0; j < 6; ++j)
			{
				extents[j] = vA;
			}
			
			BuildExtents(extents, vB);
		}

		BuildExtents(extents, vC);

		++i;
		return(true);
	});
}

int CBrushMesh::classify(const SMPLANE &plane)
{
	int res = -2, cls;
	float fDist;

	for(UINT i = 0, l = m_aVertices.size(); i < l; ++i)
	{
		fDist = SMVector4Dot(float4(m_aVertices[i], 1.0f), plane);
		if(SMIsZero(fDist))
		{
			continue;
		}
		cls = fDist > 0.0f ? 1 : -1;
		if(res == -2)
		{
			res = cls;
		}
		else if(res != cls)
		{
			return(0);
		}
	}

	return(res);
}

bool CBrushMesh::findInternalFace(Array<float3_t> &aDest)
{
	for(UINT i = 0, l = m_aEdges.size(); i < l; ++i)
	{
		Edge &e = m_aEdges[i];

		if(e.isInternal)
		{
			UINT uCurEdge = i;
			UINT uPrevEdge2 = i;
			UINT uStartEdge = i;
			UINT uPrevEdge = i;

			aDest.push_back(m_aVertices[e.uVertex[0]]);
			aDest.push_back(m_aVertices[e.uVertex[1]]);

			UINT iCounter = 0;

			e.isInternal = false;

			while((uCurEdge = findNeighbourEdge(uCurEdge, uPrevEdge2)) != uStartEdge)
			{
				uPrevEdge2 = uPrevEdge;
				Edge &e0 = m_aEdges[uPrevEdge];
				Edge &e1 = m_aEdges[uCurEdge];

				e1.isInternal = false;

				if(e0.uVertex[0] == e1.uVertex[0] || e0.uVertex[1] == e1.uVertex[0])
				{
					aDest.push_back(m_aVertices[e1.uVertex[1]]);
				}
				else
				{
					aDest.push_back(m_aVertices[e1.uVertex[0]]);
				}

				if(++iCounter > m_aEdges.size())
				{
					break;
				}

				uPrevEdge = uCurEdge;
			}
			
			if(iCounter > m_aEdges.size())
			{
				LibReport(REPORT_MSG_LEVEL_WARNING, "CBrushMesh::findInternalFace(): cycle detected!\n");
				return(false);
			}

			assert(SMIsZero(SMVector3Length2(aDest[0] - aDest[aDest.size() - 1])) || SMIsZero(SMVector3Length2(aDest[1] - aDest[aDest.size() - 1])));

			aDest.erase(aDest.size() - 1);

			return(true);
		}
	}

	return(false);
}

UINT CBrushMesh::findNeighbourEdge(UINT uEdge, UINT uSkipEdge)
{
	Edge &e0 = m_aEdges[uEdge];
	float3 vNormal;
	bool useNormal = false;
	if(uEdge != uSkipEdge)
	{
		useNormal = true;
		Edge &eS = m_aEdges[uSkipEdge];
		vNormal = SMVector3Cross(m_aVertices[e0.uVertex[0]] - m_aVertices[e0.uVertex[1]], m_aVertices[eS.uVertex[0]] - m_aVertices[eS.uVertex[1]]);
	}

	for(UINT i = 0, l = m_aEdges.size(); i < l; ++i)
	{
		if(i != uEdge && i != uSkipEdge)
		{
			Edge &e1 = m_aEdges[i];

			if(e0.uVertex[0] == e1.uVertex[0]
				|| e0.uVertex[0] == e1.uVertex[1]
				|| e0.uVertex[1] == e1.uVertex[0]
				|| e0.uVertex[1] == e1.uVertex[1])
			{
				if(useNormal && !SMIsZero(SMVector3Dot(vNormal, m_aVertices[e1.uVertex[0]] - m_aVertices[e1.uVertex[1]])))
				{
					continue;
				}

				bool isFaceFound = false;
				for(UINT j = 0, jl = m_aFaces.size(); j < jl; ++j)
				{
					Face &f = m_aFaces[j];
					if(f.isInternal)
					{
						continue;
					}
					if(f.aEdges.indexOf(uEdge) >= 0 && f.aEdges.indexOf(i) >= 0)
					{
						isFaceFound = true;
						break;
					}
				}
				if(!isFaceFound)
				{
					return(i);
				}
			}
		}
	}
	assert(!"Something wrong!");
	return(UINT_MAX);
}

bool CBrushMesh::fillInternalFace(const Array<float3_t> &aSrc)
{
	assert(aSrc.size() >= 3);

	Array<UINT> aVertices;
	for(UINT i = 0, l = aSrc.size(); i < l; ++i)
	{
		int idx = m_aVertices.indexOf(aSrc[i], [](const float3_t &a, const float3 &b){
			return(SMIsZero(SMVector3Length2(a - b)));
		});
		if(idx < 0)
		{
			return(false);
		}

		aVertices.push_back((UINT)idx);
	}

	Face f;
	bool isInternalFound = false;
	for(UINT i = 0, l = aVertices.size(); i < l; ++i)
	{
		UINT v0 = aVertices[i];

		for(UINT k = i + 1; k < l; ++k)
		{
			UINT v1 = aVertices[k];

			for(UINT j = 0, jl = m_aEdges.size(); j < jl; ++j)
			{
				Edge &e = m_aEdges[j];
				if(e.uVertex[0] == v0 && e.uVertex[1] == v1 || e.uVertex[0] == v1 && e.uVertex[1] == v0)
				{
					if(e.isInternal)
					{
						isInternalFound = true;
					}
					e.isInternal = false;
					f.aEdges.push_back(j);
					break;
				}
			}
		}
	}

	for(UINT i = 0, l = f.aEdges.size(); i < l; ++i)
	{
		Edge &e0 = m_aEdges[f.aEdges[i]];
		for(UINT j = i + 1; j < l; ++j)
		{
			Edge &e1 = m_aEdges[f.aEdges[j]];
			if(e0.uVertex[0] == e1.uVertex[0]
				|| e0.uVertex[0] == e1.uVertex[1]
				|| e0.uVertex[1] == e1.uVertex[0]
				|| e0.uVertex[1] == e1.uVertex[1])
			{
				if(j != i + 1)
				{
					UINT tmp = f.aEdges[j];
					f.aEdges[j] = f.aEdges[i + 1];
					f.aEdges[i + 1] = tmp;
				}
				break;
			}
		}
	}

	fora(i, m_aFaces)
	{
		Face &f0 = m_aFaces[i];
		if(f0.isInternal && f0.aEdges.size() == f.aEdges.size())
		{
			bool isFound = true;
			fora(j, f0.aEdges)
			{
				if(f.aEdges.indexOf(f0.aEdges[j]) < 0)
				{
					isFound = false;
					break;
				}
			}
			if(isFound)
			{
				f0.isInternal = false;
				break;
			}
		}
	}

	buildModel(false);

	return(true);

	{
		Edge &e0 = m_aEdges[f.aEdges[0]];
		Edge &e1 = m_aEdges[f.aEdges[1]];

		UINT uCenter = e0.uVertex[0] == e1.uVertex[0] || e0.uVertex[0] == e1.uVertex[1] ? e0.uVertex[0] : e0.uVertex[1];
		float3 vPos = m_aVertices[uCenter];
		float3 v0 = m_aVertices[e0.uVertex[0] == uCenter ? e0.uVertex[1] : e0.uVertex[0]];
		float3 v1 = m_aVertices[e1.uVertex[0] == uCenter ? e1.uVertex[1] : e1.uVertex[0]];

		float3 vInternal;
		fora(i, m_aVertices)
		{
			bool isFound = false;
			fora(k, f.aEdges)
			{
				Edge &e = m_aEdges[f.aEdges[k]];
				if(i == e.uVertex[0] || i == e.uVertex[1])
				{
					isFound = true;
					break;
				}
			}

			if(!isFound)
			{
				vInternal = m_aVertices[i] - vPos;
			}
		}

		f.vNormal = SMVector3Normalize(SMVector3Cross(v1 - vPos, v0 - vPos));

		if(SMVector3Dot(f.vNormal, vInternal) > 0.0f)
		{
			f.vNormal = (float3)-f.vNormal;

			Array<UINT> aNewEdges(f.aEdges.size());
			for(int i = (int)f.aEdges.size() - 1; i >= 0; --i)
			{
				aNewEdges.push_back(f.aEdges[i]);
			}
			f.aEdges.swap(aNewEdges);
		}
	}

	/*Array<UINT> aNewEdges(f.aEdges.size());
	for(int i = (int)f.aEdges.size() - 1; i >= 0; --i)
	{
		aNewEdges.push_back(f.aEdges[i]);
	}
	f.aEdges.swap(aNewEdges);

	float3 v0 = aSrc[0] - aSrc[1];
	float3 v1 = aSrc[0] - aSrc[2];
	*/
	f.texInfo = {};
	//f.vNormal = SMVector3Normalize(SMVector3Cross(v0, v1));
	f.texInfo.vT = GetTangent(GetNearestAxis(f.vNormal));
	f.texInfo.vS = SMVector3Cross(f.texInfo.vT, GetNearestAxis(f.vNormal));
	f.texInfo.fSScale = 1.0f;
	f.texInfo.fTScale = 1.0f;
	f.texInfo.uMatId = 0;

	buildModel(false);

	return(true);
}

int CBrushMesh::classifyFace(UINT uFace, const SMPLANE &plane)
{
	int res = -2, cls;
	float fDist;

	Face &f = m_aFaces[uFace];

	for(UINT i = 0, l = f.aEdges.size(); i < l; ++i)
	{
		Edge &e = m_aEdges[f.aEdges[i]];
		for(UINT j = 0; j < 2; ++j)
		{
			fDist = SMVector4Dot(float4(m_aVertices[e.uVertex[j]], 1.0f), plane);
			if(SMIsZero(fDist))
			{
				continue;
			}
			cls = fDist > 0.0f ? 1 : -1;
			if(res == -2)
			{
				res = cls;
			}
			else if(res != cls)
			{
				return(0);
			}
		}
	}

	return(res);
}

void CBrushMesh::dropFace(UINT uFace)
{
	UINT uOldMat = m_aFaces[uFace].texInfo.uMatId;
	m_aFaces.erase(uFace);

	for(UINT i = 0, l = m_aFaces.size(); i < l; ++i)
	{
		if(!m_aFaces[i].isInternal && m_aFaces[i].texInfo.uMatId == uOldMat)
		{
			return;
		}
	}

	m_aMaterials.erase(uOldMat);
	if(uOldMat != m_aMaterials.size())
	{
		for(UINT i = 0, l = m_aFaces.size(); i < l; ++i)
		{
			Face &f = m_aFaces[i];
			if(f.texInfo.uMatId > uOldMat)
			{
				--f.texInfo.uMatId;
			}
		}
	}
}

int CBrushMesh::classifyEdge(UINT uEdge, const SMPLANE &plane)
{
	Edge &e = m_aEdges[uEdge];

	float f0 = SMVector4Dot(float4(m_aVertices[e.uVertex[0]], 1.0f), plane);
	float f1 = SMVector4Dot(float4(m_aVertices[e.uVertex[1]], 1.0f), plane);

	bool z0 = SMIsZero(f0);
	bool z1 = SMIsZero(f1);

	if(z0 || z1)
	{
		// 1 or -1
		return((z0 && f1 > 0.0f || z1 && f0 > 0.0f) ? 1 : -1);
	}

	// 0 or 2 or -2
	
	if(f0 > 0.0f && f1 > 0.0f)
	{
		return(2);
	}
	if(f0 < 0.0f && f1 < 0.0f)
	{
		return(-2);
	}

	return(0);
}

UINT CBrushMesh::findOrAddVertex(const float3 &v)
{
	int idx = m_aVertices.indexOf(v, [](const float3_t &a, const float3 &b){
		return(SMIsZero(SMVector3Length2(a - b)));
	});
	if(idx < 0)
	{
		idx = m_aVertices.size();
		m_aVertices.push_back(v);
	}

	return(idx);
}

UINT CBrushMesh::findOrAddEdge(const float3 &vA, const float3 &vB)
{
	UINT u0 = findOrAddVertex(vA);
	UINT u1 = findOrAddVertex(vB);

	return(findOrAddEdge(u0, u1));
}

UINT CBrushMesh::findOrAddEdge(UINT u0, UINT u1, bool isInternal)
{
	for(UINT i = 0, l = m_aEdges.size(); i < l; ++i)
	{
		Edge &e = m_aEdges[i];
		if(e.uVertex[0] == u0 && e.uVertex[1] == u1 || e.uVertex[0] == u1 && e.uVertex[1] == u0)
		{
			return(i);
		}
	}

	m_aEdges.push_back({{u0, u1}, isInternal});
	return(m_aEdges.size() - 1);
}

void CBrushMesh::cleanupUnreferencedEdges()
{
	Array<bool> aUsed(false, m_aEdges.size());

	fora(i, m_aFaces)
	{
		Face &f = m_aFaces[i];
		fora(j, f.aEdges)
		{
			aUsed[f.aEdges[j]] = true;
		}
	}

	for(int i = (int)aUsed.size() - 1; i >= 0; --i)
	{
		if(!aUsed[i])
		{
			fora(j, m_aFaces)
			{
				Face &f = m_aFaces[j];
				fora(k, f.aEdges)
				{
					if(f.aEdges[k] > (UINT)i)
					{
						--f.aEdges[k];
					}
				}
			}
			m_aEdges.erase(i);
		}
	}
}
UINT CBrushMesh::cleanupUnreferencedVertices(UINT *puRemovedVertices)
{
	Array<bool> aUsed(false, m_aVertices.size());

	fora(i, m_aEdges)
	{
		Edge &e = m_aEdges[i];
		aUsed[e.uVertex[0]] = true;
		aUsed[e.uVertex[1]] = true;
	}

	UINT uRemoved = 0;
	if(puRemovedVertices)
	{
		fora(i, aUsed)
		{
			if(!aUsed[i])
			{
				puRemovedVertices[uRemoved++] = i;
			}
		}
	}

	if(uRemoved || !puRemovedVertices)
	{
		for(int i = (int)aUsed.size() - 1; i >= 0; --i)
		{
			if(!aUsed[i])
			{
				fora(j, m_aEdges)
				{
					Edge &e = m_aEdges[j];
					if(e.uVertex[0] > (UINT)i)
					{
						--e.uVertex[0];
					}
					if(e.uVertex[1] > (UINT)i)
					{
						--e.uVertex[1];
					}
				}

				m_aVertices.erase(i);
			}
		}
	}
	return(uRemoved);
}

bool CBrushMesh::clip(const SMPLANE &plane)
{
	int cls;
	float3 v0, v1, vPos;

	for(UINT i = 0, l = m_aFaces.size(); i < l; ++i)
	{
		cls = classifyFace(i, plane);

		if(cls == 0)
		{
			Face &f = m_aFaces[i];

			int iInsPos = -1;

			float3 avNewEdge[2];
			UINT uNewVtxCount = 0;

			bool isStartInside = false;
			cls = classifyEdge(f.aEdges[0], plane);
			if(cls < 0)
			{
				isStartInside = true;
			}
			else if(cls == 0)
			{
				cls = classifyEdge(f.aEdges[1], plane);
				if(cls > 0)
				{
					isStartInside = true;
				}
				else if(cls == 0)
				{
					cls = classifyEdge(f.aEdges[2], plane);
					if(cls < 0)
					{
						isStartInside = true;
					}
				}
			}

			for(UINT j = 0, jl = f.aEdges.size(); j < jl; ++j)
			{
				Edge &e = m_aEdges[f.aEdges[j]];

				cls = classifyEdge(f.aEdges[j], plane);
				if(cls <= 0)
				{
					if(cls == 0)
					{
						// split edge, replace with new
						v0 = m_aVertices[e.uVertex[0]];
						v1 = m_aVertices[e.uVertex[1]];
						plane.intersectLine(&vPos, v0, v1);

						bool isEdgeInternal = e.isInternal;

						if(SMVector4Dot(float4(v0, 1.0f), plane) > 0.0f)
						{
							f.aEdges[j] = findOrAddEdge(v0, vPos);
						}
						else
						{
							f.aEdges[j] = findOrAddEdge(v1, vPos);
						}

						if(isEdgeInternal)
						{
							m_aEdges[f.aEdges[j]].isInternal = true;
						}

						avNewEdge[uNewVtxCount++] = vPos;
						if(iInsPos < 0)
						{
							iInsPos = (int)j + (isStartInside ? 0 : 1);
						}
					}
					else
					{
						if(cls == -1)
						{
							vPos;
							v0 = m_aVertices[e.uVertex[0]];
							v1 = m_aVertices[e.uVertex[1]];

							if(SMIsZero(SMVector4Dot(float4(v0, 1.0f), plane)))
							{
								vPos = v0;
							}
							else
							{
								vPos = v1;
							}

							avNewEdge[uNewVtxCount++] = vPos;
							if(iInsPos < 0)
							{
								iInsPos = (int)j;
							}
						}
						f.aEdges.erase(j);
						--j;
						--jl;
					}
				}

				/*if(uNewVtxCount == 2)
				{
					break;
				}*/
			}

			assert(uNewVtxCount == 2);
			assert(iInsPos >= 0);

			// insert new edge avNewEdge at iInsPos
			UINT uNewEdge = findOrAddEdge(avNewEdge[0], avNewEdge[1]);
			if(f.isInternal)
			{
				m_aEdges[uNewEdge].isInternal = true;
			}
			f.aEdges.insert(uNewEdge, iInsPos);
		}
		else if(cls < 0)
		{
			dropFace(i);
			--i;
			--l;
		}
	}

	cleanupUnreferencedEdges();
	cleanupUnreferencedVertices();

	// insert new face
	Face &f = m_aFaces[m_aFaces.size()];
	f.isInternal = false;

	for(UINT i = 0, l = m_aEdges.size(); i < l; ++i)
	{
		Edge &e = m_aEdges[i];
		v0 = m_aVertices[e.uVertex[0]];
		v1 = m_aVertices[e.uVertex[1]];

		if(SMIsZero(SMVector4Dot(float4(v0, 1.0f), plane)) && SMIsZero(SMVector4Dot(float4(v1, 1.0f), plane)))
		{
			f.aEdges.push_back(i);
		}
	}

	assert(f.aEdges.size() >= 3);

	if(f.aEdges.size() < 3)
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "CBrushMesh::clip(): f.aEdges.size() < 3\n");
		return(false);
	}

	for(UINT i = 0, l = f.aEdges.size(); i < l; ++i)
	{
		Edge &e0 = m_aEdges[f.aEdges[i]];
		for(UINT j = i + 1; j < l; ++j)
		{
			Edge &e1 = m_aEdges[f.aEdges[j]];
			if(e0.uVertex[0] == e1.uVertex[0]
				|| e0.uVertex[0] == e1.uVertex[1]
				|| e0.uVertex[1] == e1.uVertex[0]
				|| e0.uVertex[1] == e1.uVertex[1])
			{
				if(j != i + 1)
				{
					UINT tmp = f.aEdges[j];
					f.aEdges[j] = f.aEdges[i + 1];
					f.aEdges[i + 1] = tmp;
				}
				break;
			}
		}
	}
	

	//f.vNormal = (float3)-plane;
	Edge &e0 = m_aEdges[f.aEdges[0]];
	Edge &e1 = m_aEdges[f.aEdges[1]];

	UINT uCenter = e0.uVertex[0] == e1.uVertex[0] || e0.uVertex[0] == e1.uVertex[1] ? e0.uVertex[0] : e0.uVertex[1];
	vPos = m_aVertices[uCenter];
	v0 = m_aVertices[e0.uVertex[0] == uCenter ? e0.uVertex[1] : e0.uVertex[0]];
	v1 = m_aVertices[e1.uVertex[0] == uCenter ? e1.uVertex[1] : e1.uVertex[0]];

	f.vNormal = SMVector3Normalize(SMVector3Cross(v1 - vPos, v0 - vPos));

	if(SMVector3Dot(f.vNormal, plane) > 0.0f)
	{
		f.vNormal = (float3)-f.vNormal;

		Array<UINT> aNewEdges(f.aEdges.size());
		for(int i = (int)f.aEdges.size() - 1; i >= 0; --i)
		{
			aNewEdges.push_back(f.aEdges[i]);
		}
		f.aEdges.swap(aNewEdges);
	}

	f.texInfo = {};
	f.texInfo.vT = GetTangent(GetNearestAxis(f.vNormal));
	f.texInfo.vS = SMVector3Cross(f.texInfo.vT, GetNearestAxis(f.vNormal));
	f.texInfo.fSScale = 1.0f;
	f.texInfo.fTScale = 1.0f;
	f.texInfo.uMatId = 0;
	

	m_isBoundDirty = true;
	buildModel();

	return(true);
}

void CBrushMesh::setFinalized(bool set)
{
	m_isFinalized = set;
	buildModel();
}

void CBrushMesh::buildPhysbox(IXResourceModel *pResource)
{
	IModelPhysboxConvex *pConvex = pResource->newPhysboxConvex();

	pConvex->initData(m_aVertices.size(), m_aVertices);

	pResource->addPhysbox(pConvex);
}

bool CBrushMesh::couldMoveVertices(const UINT *puAffectedVertices, UINT uVertexCount, UINT uVertexOffset, const float3 &vDeltaPos)
{
	// FIXME: enum status
	// OPTIMIZE: dynamic allocations
	Array<float3_t> aOldVertices = m_aVertices;
	Array<byte> aFaceStatus(m_aFaces.size()); // 0 = steady, 1 = intermediate, 2 = moved
	for(UINT i = 0; i < uVertexCount; ++i)
	{
		m_aVertices[puAffectedVertices[i] - uVertexOffset] = (float3)(m_aVertices[puAffectedVertices[i] - uVertexOffset] + vDeltaPos);
	}

	// compute face's status
	UINT uFaceAffectedVertices;
	fora(uFace, m_aFaces)
	{
		Face &face = m_aFaces[uFace];
		uFaceAffectedVertices = 0;
		fora(uEdge, face.aEdges)
		{
			Edge &e = m_aEdges[face.aEdges[uEdge]];
			for(UINT i = 0; i < uVertexCount; ++i)
			{
				if(e.uVertex[0] == (puAffectedVertices[i] - uVertexOffset))
				{
					++uFaceAffectedVertices;
				}
				if(e.uVertex[1] == (puAffectedVertices[i] - uVertexOffset))
				{
					++uFaceAffectedVertices;
				}
			}
		}

		if(uFaceAffectedVertices == 0)
		{
			aFaceStatus[uFace] = 0;
		}
		else if(uFaceAffectedVertices == face.aEdges.size() * 2)
		{
			aFaceStatus[uFace] = 2;
		}
		else
		{
			aFaceStatus[uFace] = 1;
		}
	}

	bool isValid = true;
	// find broken faces
	Array<UINT> aFaceVertices(m_aVertices.size());
	Array<byte> aPotentiallyBrokenVertices(0, m_aVertices.size());
	Array<float3_t> aVertices(m_aVertices.size());
	fora(uFace, m_aFaces)
	{
		if(aFaceStatus[uFace] == 1)
		{
			Face &face = m_aFaces[uFace];

			{
				bool isFirst = true;
				UINT uLastVtx = UINT_MAX;

				Edge &e0 = m_aEdges[face.aEdges[0]];
				Edge &e1 = m_aEdges[face.aEdges[1]];
				if(e0.uVertex[0] == e1.uVertex[0] || e0.uVertex[0] == e1.uVertex[1])
				{
					std::swap(e0.uVertex[0], e0.uVertex[1]);
				}

				for(UINT j = 0, jl = face.aEdges.size() - 1; j < jl; ++j)
				{
					Edge edge = m_aEdges[face.aEdges[j]];
					if(edge.uVertex[0] != uLastVtx && uLastVtx != UINT_MAX)
					{
						std::swap(edge.uVertex[0], edge.uVertex[1]);
					}
					uLastVtx = edge.uVertex[1];

					if(isFirst)
					{
						isFirst = false;

						aFaceVertices.push_back(edge.uVertex[0]);
						aFaceVertices.push_back(edge.uVertex[1]);
					}
					else
					{
						aFaceVertices.push_back(edge.uVertex[1]);
					}
				}

				// project face vertices onto face's plane
				fora(uVtx, aFaceVertices)
				{
					aVertices.push_back(ProjectToFace(face.vNormal, m_aVertices[aFaceVertices[uVtx]]));
				}

				fora(uVtx, aFaceVertices)
				{
					//5
					//012, 123, 234, 340, 401
					UINT a, b, c;
					a = uVtx;
					b = (uVtx + 1) % uVtxl;
					c = (uVtx + 2) % uVtxl;

					float fAngle = SMRightAngleBetweenVectors(aVertices[c] - aVertices[b], aVertices[a] - aVertices[b], face.vNormal);
					//printf("%f\n", fAngle);
					if(fabsf(fAngle - SM_PI) < 0.001f || fAngle > SM_PI)
					{
						isValid = false;
					}
				}

				aFaceVertices.clearFast();
				aVertices.clearFast();
			}
			
			{
				float3 vNormal0 = calcFaceNormal(uFace);

				for(UINT uFace1 = 0; uFace1 < uFacel; ++uFace1)
				{
					if(uFace1 > uFace || aFaceStatus[uFace1] != 1)
					{
						Face &face1 = m_aFaces[uFace1];
						float3 vNormal1 = calcFaceNormal(uFace1);

						//face0.isInternal && face1.isInternal
						UINT uCommonEdge;
						if(isFacesSibling(uFace, uFace1, &uCommonEdge))
						{
							// classifyFace
							SMPLANE planeFace0(vNormal0, m_aVertices[m_aEdges[uCommonEdge].uVertex[0]]);
							int cls = classifyFace(uFace1, planeFace0);

							if(cls == -2 || cls > 0)
							{
								++aPotentiallyBrokenVertices[m_aEdges[uCommonEdge].uVertex[0]];
								++aPotentiallyBrokenVertices[m_aEdges[uCommonEdge].uVertex[1]];
								break;
							}
						}
					}
				}
			}

			/*plane = SMPLANE(calcFaceNormal(uFace), m_aVertices[m_aEdges[m_aFaces[uFace].aEdges[0]].uVertex[0]]);

			float f0 = SMVector4Dot(float4(m_aVertices[puAffectedVertices[i]], 1.0f), plane);
			if(f0 >= 0.0f)
			{
				isValid = false;
				break;
			}*/
		}
	}

	fora(uVtx, aPotentiallyBrokenVertices)
	{
		if(aPotentiallyBrokenVertices[uVtx] > 1)
		{
			isValid = false;
			break;
		}
	}

	//printf("%d\n", isValid ? 1 : 0);
	
	m_aVertices.swap(aOldVertices);

	return(isValid);
}

UINT CBrushMesh::moveVertices(const UINT *puAffectedVertices, UINT uVertexCount, const float3 &vDeltaPos, UINT *puRemovedVertices)
{
	for(UINT i = 0; i < uVertexCount; ++i)
	{
		m_aVertices[puAffectedVertices[i]] = (float3)(m_aVertices[puAffectedVertices[i]] + vDeltaPos);
	}
	
	// weld merged vertices
	{
		bool wasWelded = false;
		for(UINT i = 0; i < uVertexCount; ++i)
		{
			fora(uVtx, m_aVertices)
			{
				if(puAffectedVertices[i] != uVtx && fabsf(SMVector3Length2(m_aVertices[puAffectedVertices[i]] - m_aVertices[uVtx]) < 0.000001f))
				{
					// weld
					fora(uEdge, m_aEdges)
					{
						Edge &e = m_aEdges[uEdge];
						for(UINT j = 0; j < 2; ++j)
						{
							if(e.uVertex[j] == uVtx)
							{
								e.uVertex[j] = puAffectedVertices[i];
								wasWelded = true;
							}
						}
					}
					break;
				}
			}
		}

		if(wasWelded)
		{
			fora(uFace, m_aFaces)
			{
				Face &f = m_aFaces[uFace];
				fora(uEdge, f.aEdges)
				{
					Edge &e = m_aEdges[f.aEdges[uEdge]];
					if(e.uVertex[0] == e.uVertex[1])
					{
						f.aEdges.erase(uEdge);
						--uEdge;
						--uEdgel;
					}
				}

				if(f.aEdges.size() < 3)
				{
					m_aFaces.erase(uFace);
					--uFace;
					--uFacel;
				}
			}

			fora(uEdge, m_aEdges)
			{
				Edge &e = m_aEdges[uEdge];
				for(UINT uEdge2 = uEdge + 1; uEdge2 < uEdgel; ++uEdge2)
				{
					Edge &e2 = m_aEdges[uEdge2];
					if((e.uVertex[0] == e2.uVertex[0] && e.uVertex[1] == e2.uVertex[1])
						|| (e.uVertex[0] == e2.uVertex[1] && e.uVertex[1] == e2.uVertex[0])
						)
					{
						fora(uFace, m_aFaces)
						{
							Face &f = m_aFaces[uFace];
							fora(uFEdge, f.aEdges)
							{
								if(f.aEdges[uFEdge] == uEdge2)
								{
									f.aEdges[uFEdge] = uEdge;
								}
							}
						}
					}
				}
			}
		}
	}

	//UINT *puVerts = (UINT*)alloca(sizeof(UINT) * m_aVertices.size());
	Array<UINT> puVerts(m_aVertices.size());

	// merge coplanar faces
	{
		float3 vNormal0, vNormal1;
		fora(uFace, m_aFaces)
		{
			Face &face0 = m_aFaces[uFace];
			vNormal0 = calcFaceNormal(uFace);

			for(UINT uFace1 = uFace + 1; uFace1 < uFacel; ++uFace1)
			{
				Face &face1 = m_aFaces[uFace1];
				vNormal1 = calcFaceNormal(uFace1);

				//face0.isInternal && face1.isInternal
				UINT uCommonEdge;
				if(isFacesSibling(uFace, uFace1, &uCommonEdge))
				{
					// classifyFace
					SMPLANE planeFace0(vNormal0, m_aVertices[m_aEdges[uCommonEdge].uVertex[0]]);
					int cls = classifyFace(uFace1, planeFace0);

					if(cls == -2 || cls > 0)
					{
						face0.aEdges.reserve(face0.aEdges.size() + face1.aEdges.size());

						UINT uDstPos = face0.aEdges.indexOf(uCommonEdge);
						UINT uSrcPos = face1.aEdges.indexOf(uCommonEdge);

						Edge &testEdge0 = m_aEdges[face0.aEdges[(uDstPos + 1) % face0.aEdges.size()]];
						Edge &testEdge1 = m_aEdges[face1.aEdges[(uSrcPos + 1) % face1.aEdges.size()]];

						bool isForwardDirection = testEdge0.uVertex[0] != testEdge1.uVertex[0] && testEdge0.uVertex[0] != testEdge1.uVertex[1]
							&& testEdge0.uVertex[1] != testEdge1.uVertex[0] && testEdge0.uVertex[1] != testEdge1.uVertex[1];

						face0.aEdges.erase(uDstPos);

						for(UINT i = uSrcPos + 1, l = uSrcPos + face1.aEdges.size(); i < l; ++i)
						{
							face0.aEdges.insert(face1.aEdges[i % face1.aEdges.size()], uDstPos);

							if(isForwardDirection)
							{
								++uDstPos;
							}
						}

						m_aFaces.erase(uFace1);

						--uFace;
						--uFacel;
						break;
					}

				}
				/*if(fabsf(1.0f - SMVector3Dot(vNormal0, vNormal1)) < 0.001f && isFacesSibling(uFace, uFace1, &uCommonEdge))
				{
					face0.aEdges.reserve(face0.aEdges.size() + face1.aEdges.size());

					UINT uDstPos = face0.aEdges.indexOf(uCommonEdge);
					UINT uSrcPos = face1.aEdges.indexOf(uCommonEdge);

					Edge &testEdge0 = m_aEdges[face0.aEdges[(uDstPos + 1) % face0.aEdges.size()]];
					Edge &testEdge1 = m_aEdges[face1.aEdges[(uSrcPos + 1) % face1.aEdges.size()]];

					bool isForwardDirection = testEdge0.uVertex[0] != testEdge1.uVertex[0] && testEdge0.uVertex[0] != testEdge1.uVertex[1]
						&& testEdge0.uVertex[1] != testEdge1.uVertex[0] && testEdge0.uVertex[1] != testEdge1.uVertex[1];

					face0.aEdges.erase(uDstPos);

					for(UINT i = uSrcPos + 1, l = uSrcPos + face1.aEdges.size(); i < l; ++i)
					{
						face0.aEdges.insert(face1.aEdges[i % face1.aEdges.size()], uDstPos);

						if(isForwardDirection)
						{
							++uDstPos;
						}
					}

					m_aFaces.erase(uFace1);

					--uFace;
					--uFacel;
					break;
				}*/
			}
		}
	}

	cleanupUnreferencedEdges();

	// fix broken faces
	// found affected faces
	// fix it
	fora(uFace, m_aFaces)
	{
		Face &face = m_aFaces[uFace];

		if(face.aEdges.size() == 3 || fabsf(SMVector3Dot(face.vNormal, vDeltaPos)) < 0.001f)
		{
			// moved in the face plane, skipping
			continue;
		}

		bool hasAffected = false, isFullAffected = true;
		fora(j, face.aEdges)
		{
			Edge edge = m_aEdges[face.aEdges[j]];

			for(UINT v = 0; v < 2; ++v)
			{
				bool isFound = false;
				for(UINT i = 0; i < uVertexCount; ++i)
				{
					if(puAffectedVertices[i] == edge.uVertex[v])
					{
						isFound = true;
						break;
					}
				}

				if(isFound)
				{
					hasAffected = true;
				}
				else
				{
					isFullAffected = false;
				}
			}
		}

		if(hasAffected && !isFullAffected)
		{
			// potentially broken face
			UINT uCurVec = 0;
			puVerts.clearFast();

			{
				Edge &e0 = m_aEdges[face.aEdges[0]];
				Edge &e1 = m_aEdges[face.aEdges[1]];
				if(e0.uVertex[0] == e1.uVertex[0] || e0.uVertex[0] == e1.uVertex[1])
				{
					std::swap(e0.uVertex[0], e0.uVertex[1]);
				}
			}

			UINT uLastVtx = UINT_MAX;
			bool isFirst = true;
			for(UINT j = 0, jl = face.aEdges.size() - 1; j < jl; ++j)
			{
				Edge edge = m_aEdges[face.aEdges[j]];
				if(edge.uVertex[0] != uLastVtx && uLastVtx != UINT_MAX)
				{
					std::swap(edge.uVertex[0], edge.uVertex[1]);
				}
				uLastVtx = edge.uVertex[1];

				if(isFirst)
				{
					isFirst = false;

					puVerts[uCurVec++] = edge.uVertex[0];
				}

				puVerts[uCurVec++] = edge.uVertex[1];
			}

			UINT uMaxVec = uCurVec;
			UINT uFaceVerts;
			uCurVec = 0;
			float fDist;
			bool isFirstPass = true;
			while(uMaxVec >= 3)
			{
				uCurVec %= uMaxVec;
				SMPLANE plane(
					m_aVertices[puVerts[uCurVec]],
					m_aVertices[puVerts[(uCurVec + 1) % uMaxVec]],
					m_aVertices[puVerts[(uCurVec + 2) % uMaxVec]]
					);

				bool isConvex = true;
				uFaceVerts = 3;
				for(UINT i = uCurVec + 3, l = uCurVec + uMaxVec; i < l; ++i)
				{
					fDist = SMVector4Dot(float4(m_aVertices[puVerts[i % uMaxVec]], 1.0f), plane);

					if(SMIsZero(fDist))
					{
						++uFaceVerts;
					}
					else if(fDist > 0.0f)
					{
						isConvex = false;
						break;
					}
				}

				if(!isConvex)
				{
					++uCurVec;
					continue;
				}
				
				if(isFirstPass)
				{
					isFirstPass = false;
					uCurVec += uFaceVerts - 1;
					continue;
				}

				Face &newFace = m_aFaces[m_aFaces.size()];
				Face &srcFace = m_aFaces[uFace];

				newFace.texInfo = srcFace.texInfo;
				newFace.vNormal = SMVector3Normalize(SMVector3Cross(
					m_aVertices[puVerts[uCurVec]] - m_aVertices[puVerts[(uCurVec + 1) % uMaxVec]],
					m_aVertices[puVerts[uCurVec]] - m_aVertices[puVerts[(uCurVec + 2) % uMaxVec]]
					));
				newFace.isInternal = srcFace.isInternal;
				newFace.aEdges.reserve(uFaceVerts);
				newFace.aEdges.push_back(findOrAddEdge(puVerts[(uCurVec + uFaceVerts - 1) % uMaxVec], puVerts[uCurVec], newFace.isInternal));
				for(UINT i = uCurVec, l = uCurVec + uFaceVerts - 1; i < l; ++i)
				{
					newFace.aEdges.push_back(findOrAddEdge(puVerts[i % uMaxVec], puVerts[(i + 1) % uMaxVec]));
				}

				//puVerts.erase(uCurVec + 1);

				UINT uS = (uCurVec + 1) % uMaxVec;
				UINT uE = uS + uFaceVerts - 3;

				if(uE >= uMaxVec)
				{
					uE %= uMaxVec;
					for(UINT i = uMaxVec; uS < i; --i)
					{
						puVerts.erase(i - 1);
					}
					for(int i = (int)uE; i >= 0; --i)
					{
						puVerts.erase(i);
					}
					uCurVec = 0;
				}
				else
				{
					for(int i = (int)uE; (int)uS <= i; --i)
					{
						puVerts.erase(i);
					}
					++uCurVec;
				}
				
				uMaxVec -= uFaceVerts - 2;
			}

			m_aFaces.erase(uFace);
			--uFacel;
			--uFace;
		}
	}

	fora(uFace, m_aFaces)
	{
		Face &face = m_aFaces[uFace];
		face.vNormal = calcFaceNormal(uFace);
	}

	UINT uRemovedCount = cleanupUnreferencedVertices(puRemovedVertices);

	m_isBoundDirty = true;
	buildModel();

	return(uRemovedCount);
}

float3 CBrushMesh::calcFaceNormal(UINT uFace)
{
	float3 vNormal;
	triangulateFace(uFace, [&vNormal](const float3 &vA, const float3 &vB, const float3 &vC){
		vNormal = SMVector3Normalize(SMVector3Cross(vA - vB, vA - vC));
		return(false);
	});

	return(vNormal);
}
bool CBrushMesh::isFacesSibling(UINT uFace0, UINT uFace1, UINT *puCommonEdge)
{
	Face &face0 = m_aFaces[uFace0];
	Face &face1 = m_aFaces[uFace1];
	fora(uEdge, face0.aEdges)
	{
		int edg = face1.aEdges.indexOf(face0.aEdges[uEdge]);
		if(edg >= 0)
		{
			if(puCommonEdge)
			{
				*puCommonEdge = face1.aEdges[edg];
			}
			return(true);
		}
	}

	return(false);
}

void CBrushMesh::setColor(const float3_t &vColor)
{
	m_vColor = vColor;
	SAFE_CALL(m_pModel, setColor, (float4)vColor);
}

//##########################################################################

Subset& CMeshBuilder::getSubset(UINT id)
{
	assert(id < m_aSubsets.size());
	return(m_aSubsets[id]);
}
UINT CMeshBuilder::getSubsetIndexForMaterial(const char *szMat)
{
	int idx = m_aMaterials.indexOf(szMat, [](const String &a, const char *b){
		return(!strcmp(a.c_str(), b));
	});
	if(idx < 0)
	{
		idx = m_aMaterials.size();
		m_aMaterials.push_back(szMat);
	}

	m_aSubsets.resizeFast(m_aMaterials.size());

	return(idx);
}

UINT CMeshBuilder::getSubsetCount()
{
	return(m_aMaterials.size());
}

const char* CMeshBuilder::getMaterial(UINT id)
{
	assert(id < m_aMaterials.size());
	return(m_aMaterials[id].c_str());
}

void CMeshBuilder::buildResource(IXResourceModelStatic *pResource, UINT idx)
{
	Array<UINT> aSubsetIdxs;
	if(idx != UINT_MAX)
	{
		pResource->setMaterialCount(1, 1);
		pResource->setMaterial(0, 0, getMaterial(idx));
		aSubsetIdxs.push_back(idx);
	}
	else
	{
		pResource->setMaterialCount(getSubsetCount(), 1);
		for(UINT i = 0, l = getSubsetCount(); i < l; ++i)
		{
			if(getSubset(i).aIndices.size())
			{
				pResource->setMaterial(aSubsetIdxs.size(), 0, getMaterial(i));
				aSubsetIdxs.push_back(i);
			}
		}
	}

	UINT *puVtxCount = (UINT*)alloca(sizeof(UINT) * aSubsetIdxs.size());
	UINT *puIdxCount = (UINT*)alloca(sizeof(UINT) * aSubsetIdxs.size());

	fora(i, aSubsetIdxs)
	{
		Subset &subset = getSubset(aSubsetIdxs[i]);

		puVtxCount[i] = subset.aVertices.size();
		puIdxCount[i] = subset.aIndices.size();
	}

	pResource->addLod(aSubsetIdxs.size(), puVtxCount, puIdxCount);
	fora(i, aSubsetIdxs)
	{
		Subset &subset = getSubset(aSubsetIdxs[i]);

		XResourceModelStaticSubset *pSubset = pResource->getSubset(0, i);

		pSubset->iMaterialID = i;
		pSubset->iIndexCount = subset.aIndices.size();
		pSubset->iVertexCount = subset.aVertices.size();

		memcpy(pSubset->pIndices, subset.aIndices, sizeof(UINT) * pSubset->iIndexCount);
		memcpy(pSubset->pVertices, subset.aVertices, sizeof(XResourceModelStaticVertex) * pSubset->iVertexCount);

	}
}

