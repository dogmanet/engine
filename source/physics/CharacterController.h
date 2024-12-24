#ifndef __CHARACTERCONTROLLER_H
#define __CHARACTERCONTROLLER_H

#include <xcommon/physics/IXCharacterController.h>
//#include "CollisionObject.h"
#include "sxphysics.h"

class CCharacterController: public IXUnknownImplementation<IXCharacterController>
{
public:
	CCharacterController(IXGhostObject *pGhostObject, float fStepHeight);
	~CCharacterController();

	void XMETHODCALLTYPE setVelocityForTimeInterval(const float3 &vSpeed, float fTime) override;
	void XMETHODCALLTYPE setWalkDirection(const float3 &vSpeed) override;

	bool XMETHODCALLTYPE onGround() const override;
	bool XMETHODCALLTYPE canStandUp(float fDelta) const override;
	bool XMETHODCALLTYPE canJump() const override;
	void XMETHODCALLTYPE jump() override;

	float3 XMETHODCALLTYPE getLinearVelocity() const override;

	void XMETHODCALLTYPE registerInWorld(IXPhysicsWorld *pWorld) override;
	void XMETHODCALLTYPE unregisterInWorld() override;

	void XMETHODCALLTYPE setMaxJumpHeight(float fHeight) override;
	void XMETHODCALLTYPE setJumpSpeed(float fSpeed) override;
	void XMETHODCALLTYPE setGravity(const float3_t &vSpeed) override;
	void XMETHODCALLTYPE setFallSpeed(float fSpeed) override;
	void XMETHODCALLTYPE setMaxPenetrationDepth(float fMaxDepth) override;

private:
	btKinematicCharacterController *m_pController;
	IXGhostObject *m_pGhostObject;
	btDiscreteDynamicsWorldMt *m_pWorld = NULL;
};

#endif
