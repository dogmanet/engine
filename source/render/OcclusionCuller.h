#ifndef __OCCLUSIONCULLER_H
#define __OCCLUSIONCULLER_H

#include <xcommon/render/IXOcclusionCuller.h>
#include <light/IXLightSystem.h>
#include <xcommon/physics/IXPhysics.h>

#define OCCLUSION_BUFFER_WIDTH 16
#define OCCLUSION_BUFFER_HEIGHT 16

class CAllHitsRayResultCallback: public IXRayCallback
{
public:
	float XMETHODCALLTYPE addSingleResult(const XRayResult &result) override
	{
		m_aResults.push_back(result);

		return(1.0f);
	}

	bool hasHit()
	{
		return(m_aResults.size() != 0);
	}

	SX_ALIGNED_OP_MEM();

	Array<XRayResult> m_aResults;
	float3 m_rayToWorld;
};

//##########################################################################

class COcclusionCuller final: public IXUnknownImplementation<IXOcclusionCuller>
{
public:
	COcclusionCuller();
	~COcclusionCuller();

	SX_ALIGNED_OP_MEM();

	void XMETHODCALLTYPE update(IXCamera *pCamera, float fFOV, float fAspectRatio, float fFarPlane) override;

	bool XMETHODCALLTYPE isPointVisible(const float3 &vPoint) const override;

	//bool XMETHODCALLTYPE isPolyVisible(const float3 &vPoint1, const float3 &vPoint2, const float3 &vPoint3) const override;

	//bool XMETHODCALLTYPE isSphereVisible(const float3 &vOrigin, float fRadius) const override;

	bool XMETHODCALLTYPE isAABBvisible(const SMAABB &aabb, bool *pisFullyVisible = NULL) const override;

private:
	void _update();

	void _print();

	int classifyPoint(const float3 &vPoint, bool *pisCulledByFrustum = NULL, bool bIgnoreFrustum = false) const;

private:
	IXCamera *m_pCamera = NULL;
	float m_fFOV = 1.0f;
	float m_fAspectRatio = 1.0f;
	float m_fFarPlane = 1.0f;

	float m_ppfOcclusionBuffer[OCCLUSION_BUFFER_WIDTH][OCCLUSION_BUFFER_HEIGHT];
	CAllHitsRayResultCallback cbs[OCCLUSION_BUFFER_WIDTH + 1][OCCLUSION_BUFFER_HEIGHT + 1];

	enum
	{
		PLANE_TOP,
		PLANE_BOTTOM,
		PLANE_RIGHT,
		PLANE_LEFT,
		PLANE_FRONT
	};

	struct Polyhedron
	{
		SMPLANE planes[5];
		bool isValid = false;

		SX_ALIGNED_OP_MEM();

		UINT getPlaneCount()
		{
			return(5);
		}

		const SMPLANE& getPlaneAt(UINT idx)
		{
			return(planes[idx]);
		}
	} m_ppPolyhedra[OCCLUSION_BUFFER_WIDTH][OCCLUSION_BUFFER_HEIGHT];

	IXLightSystem *m_pLightSystem = NULL;
	IXPhysicsWorld *m_pPhysWorld = NULL;
	Array<IXLightPoint*> m_apTemp;
};

#endif
