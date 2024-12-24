#ifndef __IXCHARACTERCONTROLLER_H
#define __IXCHARACTERCONTROLLER_H

#include "IXCollisionShape.h"

class IXPhysicsWorld;
class IXCharacterController: public IXUnknown
{
public:
	virtual void XMETHODCALLTYPE setVelocityForTimeInterval(const float3 &vSpeed, float fTime) = 0;
	virtual void XMETHODCALLTYPE setWalkDirection(const float3 &vSpeed) = 0;

	virtual bool XMETHODCALLTYPE onGround() const = 0;
	virtual bool XMETHODCALLTYPE canStandUp(float fDelta) const = 0;
	virtual bool XMETHODCALLTYPE canJump() const = 0;
	virtual void XMETHODCALLTYPE jump() = 0;

	virtual float3 XMETHODCALLTYPE getLinearVelocity() const = 0;

	virtual void XMETHODCALLTYPE registerInWorld(IXPhysicsWorld *pWorld) = 0;
	virtual void XMETHODCALLTYPE unregisterInWorld() = 0;
	
	virtual void XMETHODCALLTYPE setMaxJumpHeight(float fHeight) = 0;
	virtual void XMETHODCALLTYPE setJumpSpeed(float fSpeed) = 0;
	virtual void XMETHODCALLTYPE setGravity(const float3_t &vSpeed) = 0;
	virtual void XMETHODCALLTYPE setFallSpeed(float fSpeed) = 0;
	virtual void XMETHODCALLTYPE setMaxPenetrationDepth(float fMaxDepth) = 0;


	/*

	virtual void XMETHODCALLTYPE setCollisionShape(IXCollisionShape *pCollisionShape) = 0;
	virtual IXCollisionShape* XMETHODCALLTYPE getCollisionShape() = 0;

	virtual void XMETHODCALLTYPE setFriction(float fFriction) = 0;
	virtual float XMETHODCALLTYPE getFriction() const = 0;

	virtual void XMETHODCALLTYPE setRollingFriction(float fFriction) = 0;
	virtual float XMETHODCALLTYPE getRollingFriction() const = 0;

	virtual void XMETHODCALLTYPE setSpinningFriction(float fFriction) = 0;
	virtual float XMETHODCALLTYPE getSpinningFriction() const = 0;

	virtual void XMETHODCALLTYPE setPosition(const float3 &vPosition) = 0;
	virtual float3 XMETHODCALLTYPE getPosition() const = 0;

	virtual void XMETHODCALLTYPE setRotation(const SMQuaternion &qRotation) = 0;
	virtual SMQuaternion XMETHODCALLTYPE getRotation() const = 0;

	virtual void XMETHODCALLTYPE setCollisionFlags(XCOLLISION_FLAGS flags) = 0;
	virtual XCOLLISION_FLAGS XMETHODCALLTYPE getCollisionFlags() const = 0;

	virtual bool XMETHODCALLTYPE hasContactResponse() const = 0;
	virtual bool XMETHODCALLTYPE isStaticOrKinematic() const = 0;

	virtual void XMETHODCALLTYPE setUserPointer(void *pUser) = 0;
	virtual void* XMETHODCALLTYPE getUserPointer() const = 0;

	virtual void XMETHODCALLTYPE setUserTypeId(int iUser) = 0;
	virtual int XMETHODCALLTYPE getUserTypeId() const = 0;

	virtual void XMETHODCALLTYPE setUserIndex(int iUser) = 0;
	virtual int XMETHODCALLTYPE getUserIndex() const = 0;

	virtual void XMETHODCALLTYPE setCCDsweptSphereRadius(float fRadius) = 0;
	virtual float XMETHODCALLTYPE getCCDsweptSphereRadius() const = 0;

	virtual void XMETHODCALLTYPE setCCDmotionThreshold(float fRadius) = 0;
	virtual float XMETHODCALLTYPE getCCDmotionThreshold() const = 0;

	virtual SMAABB XMETHODCALLTYPE getAABB() const = 0;

	virtual COLLISION_GROUP XMETHODCALLTYPE getFilterGroup() const = 0;
	virtual COLLISION_GROUP XMETHODCALLTYPE getFilterMask() const = 0;

	virtual IXRigidBody* XMETHODCALLTYPE asRigidBody() = 0;
	virtual IXGhostObject* XMETHODCALLTYPE asGhostObject() = 0;*/
};

#endif
