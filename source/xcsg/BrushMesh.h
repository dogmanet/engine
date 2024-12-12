#ifndef __BRUSHMESH_H
#define __BRUSHMESH_H


#include <gdefines.h>
#include <xcommon/resource/IXModel.h>
#include <xcommon/IXCore.h>
#include <xcommon/IXJSON.h>
#include "Outline.h"
#include <xcommon/physics/IXPhysics.h>

struct BrushFace
{
	float3_t vN;
	float3_t vS;
	float fSScale;
	float fSShift;
	float3_t vT;
	float fTScale;
	float fTShift;
	const char *szMaterial;
};

typedef float3_t Extents[6];

enum CLIP_PLANE_STATE
{
	CPS_NONE,
	CPS_ONESIDE,
	CPS_TWOSIDE
};

struct Subset
{
	Array<XResourceModelStaticVertex> aVertices;
	Array<UINT> aIndices;
};

class CMeshBuilder
{
public:
	Subset& getSubset(UINT id);
	UINT getSubsetIndexForMaterial(const char *szMat);

	UINT getSubsetCount();

	const char* getMaterial(UINT id);

	void buildResource(IXResourceModelStatic *pResource, UINT idx = UINT_MAX);

private:
	Array<Subset> m_aSubsets;
	Array<String> m_aMaterials;
};

//##########################################################################

class CBrushMesh
{
public:
	CBrushMesh(IXCore *pCore, COutline *pOutline, UINT uContour, const char *szMat, float fHeight);
	CBrushMesh(IXCore *pCore);
	~CBrushMesh();

	SX_ALIGNED_OP_MEM();

	void enable(bool yesNo);
	const SMAABB& getAABB();

	void move(const float3 &vOffset);
	void rotate(const float3 &vOrigin, const SMQuaternion &qOffset);
	void resize(const float3 &vOrigin, const float3 &vRelativeSize);

	bool rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut = NULL, float3 *pvNormal = NULL, bool isRayInWorldSpace = true, bool bReturnNearestPoint = false);

	void renderSelection(bool is3D, IXGizmoRenderer *pGizmoRenderer, CLIP_PLANE_STATE clipPlaneState, const SMPLANE &clipPlane);
	void renderFace(IXGizmoRenderer *pGizmoRenderer, UINT uFace);

	void serialize(Array<char> *paData);

	bool deserialize(IXJSONObject *pData);

	bool findFace(const float3 &vRayStart, const float3 &vRayDir, UINT *puFace, float3 *pvFacePoint = NULL);

	UINT getFaceCount() const;

	void getFaceInfo(UINT uFace, BrushFace *pOut);
	void setFaceInfo(UINT uFace, const BrushFace &brushFace, bool bRebuild = true);

	static float3 GetNearestAxis(const float3 &vNormal);
	static float3 GetTangent(const float3 &vNormal);
	static float3 ProjectToFace(const float3 &vNormal, const float3 &vIn);
	static void BuildExtents(Extents extents, const float3 &vC);

	void getFaceExtents(UINT uFace, Extents extents);

	int classify(const SMPLANE &plane);

	bool clip(const SMPLANE &plane);

	bool findInternalFace(Array<float3_t> &aDest);
	bool fillInternalFace(const Array<float3_t> &aSrc);

	void buildMesh(CMeshBuilder *pBuilder);
	void buildPhysbox(IXResourceModel *pResource);

	void setFinalized(bool set);

	UINT getVertexCount()
	{
		return(m_aVertices.size());
	}
	const float3_t& getVertexAt(UINT uVertex)
	{
		assert(m_aVertices.size() > uVertex);
		return(m_aVertices[uVertex]);
	}

	bool couldMoveVertices(const UINT *puAffectedVertices, UINT uVertexCount, UINT uVertexOffset, const float3 &vDeltaPos);
	UINT moveVertices(const UINT *puAffectedVertices, UINT uVertexCount, const float3 &vDeltaPos, UINT *puRemovedVertices);

	void setColor(const float3_t &vColor);

private:
	void buildModel(bool bBuildPhysbox = true);
	void setupFromOutline(COutline *pOutline, UINT uContour, float fHeight);


	/*! Генерирует треугольники фейса
		func принимает три точки(const float3 &vA, const float3 &vB, const float3 &vC), возвращает истину для продолжения, ложь для остановки
		возвращает истину, если процесс не был прерван досрочно
	*/
	template<typename T>
	bool triangulateFace(UINT uFace, const T &func)
	{
		Face &face = m_aFaces[uFace];
		
		bool isFirst = true;
		UINT uLastVtx = UINT_MAX;

		float3 vA, vB, vC;

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

				vA = m_aVertices[edge.uVertex[0]];
				vB = m_aVertices[edge.uVertex[1]];
			}
			else
			{
				vC = m_aVertices[edge.uVertex[1]];

				if(!func(vA, vB, vC))
				{
					return(false);
				}

				vB = vC;
			}
		}
		return(true);
	}

	UINT findNeighbourEdge(UINT uEdge, UINT uSkipEdge);

	int classifyFace(UINT uFace, const SMPLANE &plane);
	int classifyEdge(UINT uEdge, const SMPLANE &plane);

	UINT findOrAddEdge(const float3 &vA, const float3 &vB);
	UINT findOrAddEdge(UINT uVert0, UINT uVert1, bool isInternal = false);
	UINT findOrAddVertex(const float3 &v);

	void cleanupUnreferencedEdges();
	UINT cleanupUnreferencedVertices(UINT *puRemovedVertices = NULL);

	void dropFace(UINT uFace);

	float3 calcFaceNormal(UINT uFace);
	bool isFacesSibling(UINT uFace0, UINT uFace1, UINT *puCommonEdge = NULL);

private:
	//IXEditorGizmoHandle *m_pHandle[2];
	IXStaticModel *m_pModel = NULL;
	IXCore *m_pCore = NULL;
	IXPhysics *m_pPhysics = NULL;
	IXRigidBody *m_pRigidBody = NULL;
	bool m_isFinalized = false;

	struct Edge
	{
		UINT uVertex[2];
		bool isInternal;
	};

	struct TextureInfo
	{
		float3_t vS;
		float fSScale;
		float fSShift;    // Texture shift in s direction
		float3_t vT;
		float fTScale;
		float fTShift;    // Texture shift in t direction
		UINT uMatId;
		//uint32_t iMiptex; // Index into textures array
		//uint32_t nFlags;  // Texture flags, seem to always be 0
	};

	struct Face
	{
		TextureInfo texInfo;
		Array<UINT> aEdges;
		float3_t vNormal;
		bool isInternal;
	};

	Array<float3_t> m_aVertices;
	Array<Edge> m_aEdges;
	Array<Face> m_aFaces;

	Array<String> m_aMaterials;

	SMAABB m_aabb;
	bool m_isBoundDirty = true;
	bool m_isPhysicsLoaded = false;

	float3_t m_vColor;
};

#endif
