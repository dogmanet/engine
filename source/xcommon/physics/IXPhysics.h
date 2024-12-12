#ifndef __IXPHYSICS_H
#define __IXPHYSICS_H

#include "IXCollisionShape.h"
#include "IXCollisionObject.h"
#include "IXCharacterController.h"
#include "IXMutationObserver.h"

/*
typedef enum PHY_ScalarType
{
	PHY_FLOAT,
	PHY_DOUBLE,
	PHY_INTEGER,
	PHY_SHORT,
	PHY_FIXEDPOINT88,
	PHY_UCHAR
} PHY_ScalarType;
*/

struct XRayResult
{
	float3_t vHitPoint;
	float3_t vHitNormal;
	IXCollisionObject *pCollisionObject;
	void *_reserved;
	float fHitFraction;
};

class IXRayCallback
{
public:
	virtual float XMETHODCALLTYPE addSingleResult(const XRayResult &result) = 0;
};

//#############################################################################

struct XConvexResult
{
	float3_t vHitPoint;
	float3_t vHitNormal;
	IXCollisionObject *pCollisionObject;
	void *_reserved;
	float fHitFraction;
};

class IXConvexCallback
{
public:
	virtual float XMETHODCALLTYPE addSingleResult(const XConvexResult &result) = 0;
};

//#############################################################################

class IXPhysicsWorld: public IXUnknown
{
public:
	virtual void XMETHODCALLTYPE addCollisionObject(IXCollisionObject *pCollisionObject, COLLISION_GROUP collisionGroup = CG_DEFAULT, COLLISION_GROUP collisionMask = CG_ALL) = 0;
	virtual void XMETHODCALLTYPE removeCollisionObject(IXCollisionObject *pCollisionObject) = 0;

	virtual void XMETHODCALLTYPE rayTest(const float3 &vFrom, const float3 &vTo, IXRayCallback *pCallback, COLLISION_GROUP collisionGroup = CG_DEFAULT, COLLISION_GROUP collisionMask = CG_ALL) = 0;
	
	virtual void XMETHODCALLTYPE disableSimulation() = 0;
	virtual void XMETHODCALLTYPE enableSimulation() = 0;

	virtual void XMETHODCALLTYPE convexSweepTest(IXConvexShape *pShape, const transform_t &xfFrom, const transform_t &xfTo, IXConvexCallback *pCallback, COLLISION_GROUP collisionGroup = CG_DEFAULT, COLLISION_GROUP collisionMask = CG_ALL) = 0;

	// add/remove action
	// convexSweepTest
	// add/remove constraint
};

//#############################################################################

// {B80CD682-53BF-4173-AD21-20983C524784}
#define IXPHYSICS_GUID DEFINE_XGUID(0xb80cd682, 0x53bf, 0x4173, 0xad, 0x21, 0x20, 0x98, 0x3c, 0x52, 0x47, 0x84)

class IXPhysics: public IXUnknown
{
public:
	virtual void XMETHODCALLTYPE newBoxShape(const float3 &vHalfExtents, IXBoxShape **ppOut) = 0;
	virtual void XMETHODCALLTYPE newSphereShape(float fRadius, IXSphereShape **ppOut) = 0;
	virtual void XMETHODCALLTYPE newCapsuleShape(float fRadius, float fHeight, IXCapsuleShape **ppOut) = 0;
	virtual void XMETHODCALLTYPE newCylinderShape(float fRadius, float fHeight, IXCylinderShape **ppOut) = 0;
	virtual void XMETHODCALLTYPE newStaticPlaneShape(const SMPLANE &plane, IXStaticPlaneShape **ppOut) = 0;
	/*virtual void XMETHODCALLTYPE newTerrainShape(int heightStickWidth, int heightStickLength,
		const void* heightfieldData, float heightScale,
		float minHeight, float maxHeight,
		char upAxis / * x/y/z * /, PHY_ScalarType heightDataType,
		bool flipQuadEdges, 
		ITerrainShape **ppOut) = 0;*/
	virtual void XMETHODCALLTYPE newConvexHullShape(UINT uPoints, const float3_t *pPoints, IXConvexHullShape **ppOut, byte u8Stride = sizeof(float3_t), bool bOptimize = true) = 0;
	virtual void XMETHODCALLTYPE newTrimeshShape(UINT uVertices, const float3_t *pVertices, UINT uIndices, UINT *pIndices, IXTrimeshShape **ppOut, byte u8Stride = sizeof(float3_t)) = 0;
	virtual void XMETHODCALLTYPE newCompoundShape(IXCompoundShape **ppOut, UINT uShapes = 0) = 0;
	
	virtual void XMETHODCALLTYPE newRigidBody(const XRIDIGBODY_DESC &desc, IXRigidBody **ppOut) = 0;
	virtual void XMETHODCALLTYPE newGhostObject(IXGhostObject **ppOut, bool isPairCaching = true) = 0;

	virtual void XMETHODCALLTYPE newCharacterController(IXGhostObject *pGhostObject, float fStepHeight, IXCharacterController **ppOut) = 0;

	virtual void XMETHODCALLTYPE newMutationObserver(IXMutationObserver **ppOut) = 0;

	virtual IXPhysicsWorld* XMETHODCALLTYPE getWorld(void *pReserved = NULL) = 0;
};

#endif
