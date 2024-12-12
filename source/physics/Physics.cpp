#include "Physics.h"
#include "CollisionShape.h"
#include "CollisionObject.h"
#include "sxphysics.h"
#include "PhyWorld.h"
#include "CharacterController.h"
#include "MutationObserver.h"
#include <core/sxcore.h>

CPhysics::CPhysics(CPhyWorld *pDefaultWorld):
	m_pDefaultWorld(pDefaultWorld)
{
}

void XMETHODCALLTYPE CPhysics::newBoxShape(const float3 &vHalfExtents, IXBoxShape **ppOut)
{
	*ppOut = new CBoxShape(vHalfExtents);
}
void XMETHODCALLTYPE CPhysics::newSphereShape(float fRadius, IXSphereShape **ppOut)
{
	*ppOut = new CSphereShape(fRadius);
}
void XMETHODCALLTYPE CPhysics::newCapsuleShape(float fRadius, float fHeight, IXCapsuleShape **ppOut)
{
	*ppOut = new CCapsuleShape(fRadius, fHeight);
}
void XMETHODCALLTYPE CPhysics::newCylinderShape(float fRadius, float fHeight, IXCylinderShape **ppOut)
{
	*ppOut = new CCylinderShape(fRadius, fHeight);
}
void XMETHODCALLTYPE CPhysics::newStaticPlaneShape(const SMPLANE &plane, IXStaticPlaneShape **ppOut)
{
	*ppOut = new CStaticPlaneShape(plane);
}
void XMETHODCALLTYPE CPhysics::newConvexHullShape(UINT uPoints, const float3_t *pPoints, IXConvexHullShape **ppOut, byte u8Stride, bool bOptimize)
{
	*ppOut = new CConvexHullShape(uPoints, pPoints, u8Stride, bOptimize);
}
void XMETHODCALLTYPE CPhysics::newTrimeshShape(UINT uVertices, const float3_t *pVertices, UINT uIndices, UINT *pIndices, IXTrimeshShape **ppOut, byte u8Stride)
{
	*ppOut = new CTrimeshShape(uVertices, pVertices, uIndices, pIndices, u8Stride);
}
void XMETHODCALLTYPE CPhysics::newCompoundShape(IXCompoundShape **ppOut, UINT uShapes)
{
	*ppOut = new CCompoundShape(uShapes);
}

void XMETHODCALLTYPE CPhysics::newRigidBody(const XRIDIGBODY_DESC &desc, IXRigidBody **ppOut)
{
	*ppOut = new CRigidBody(desc);
}
void XMETHODCALLTYPE CPhysics::newGhostObject(IXGhostObject **ppOut, bool isPairCaching)
{
	*ppOut = new CGhostObject(isPairCaching);
}

void XMETHODCALLTYPE CPhysics::newCharacterController(IXGhostObject *pGhostObject, float fStepHeight, IXCharacterController **ppOut)
{
	*ppOut = new CCharacterController(pGhostObject, fStepHeight);
}

void XMETHODCALLTYPE CPhysics::newMutationObserver(IXMutationObserver **ppOut)
{
	*ppOut = new CMutationObserver(Core_GetIXCore());
}

IXPhysicsWorld* XMETHODCALLTYPE CPhysics::getWorld(void *pReserved)
{
	assert(pReserved == NULL);

	return(m_pDefaultWorld);
}
