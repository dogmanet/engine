#ifndef __BASE_MOVER_H
#define __BASE_MOVER_H

#include "PointEntity.h"

#define MOVER_INITIALLY_DISABLED ENT_FLAG_0
#define MOVER_NO_AUTOMOUNT ENT_FLAG_1

class CBaseMover;
class CPhysicsMoverTickEventListener final: public IEventListener<XEventPhysicsStep>
{
public:
	CPhysicsMoverTickEventListener(CBaseMover *pMover):
		m_pMover(pMover)
	{
	}
	void onEvent(const XEventPhysicsStep *pData) override;

private:
	CBaseMover *m_pMover;
};

class IMovementController;
class CBaseMover: public CPointEntity
{
	DECLARE_CLASS(CBaseMover, CPointEntity);
	DECLARE_PROPTABLE();
	friend class CPhysicsMoverTickEventListener;
public:
	DECLARE_CONSTRUCTOR();
	~CBaseMover();

	void setPos(const float3 &pos) override;

	void renderEditor(bool is3D, bool bRenderSelection, IXGizmoRenderer *pRenderer) override;

	void getMinMax(float3 *min, float3 *max) override;

	bool rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut = NULL, float3 *pvNormal = NULL, bool isRayInWorldSpace = true, bool bReturnNearestPoint = false) override;

	void onUse(CBaseEntity *pUser) override;

	float3 getUpPos();

	float getSpeed();

private:
	void handleCharacterMount(CBaseEntity *pEntity);
	void createPhysBody();
	void setUpPoint(const float3 &vUp);
	void updateFlags() override;

	void turnOn(inputdata_t *pInputdata);
	void turnOff(inputdata_t *pInputdata);
	void toggle(inputdata_t *pInputdata);
	void initPhysics();
	void enable();
	void disable();
	void onPhysicsStep();

	SMAABB getBound();

	virtual void newMovementController(IMovementController **ppOut);

private:
	float3_t m_vUpPoint = float3_t(0.0f, 2.0f, 0.0f);
	bool m_isEnabled = false;

	output_t m_onPlayerGetOn;
	output_t m_onPlayerGetOff;

	IXGhostObject *m_pGhostObject = NULL;
	IXConvexHullShape *m_pCollideShape = NULL;
	static IEventChannel<XEventPhysicsStep> *m_pTickEventChannel;
	CPhysicsMoverTickEventListener m_physicsTicker;

	Array<CBaseEntity*> m_aTouchedEntities;

	float m_fSpeed = 3.0f;
};

#endif
