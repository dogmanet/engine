#ifndef __FUNC_LADDER_H
#define __FUNC_LADDER_H

#include "PointEntity.h"

class CInfoLadderDismount: public CPointEntity
{
	DECLARE_CLASS(CInfoLadderDismount, CPointEntity);
	DECLARE_PROPTABLE();
public:
	DECLARE_TRIVIAL_CONSTRUCTOR();
};

//##########################################################################

#define LADDER_INITIALLY_DISABLED ENT_FLAG_0

class CFuncLadder;
class CPhysicsLadderTickEventListener final : public IEventListener<XEventPhysicsStep>
{
public:
	CPhysicsLadderTickEventListener(CFuncLadder *pLadder) :
		m_pLadder(pLadder)
	{
	}
	void onEvent(const XEventPhysicsStep *pData) override;

private:
	CFuncLadder *m_pLadder;
};

class CFuncLadder: public CPointEntity
{
	DECLARE_CLASS(CFuncLadder, CPointEntity);
	DECLARE_PROPTABLE();
	friend class CPhysicsLadderTickEventListener;
public:
	DECLARE_CONSTRUCTOR();
	~CFuncLadder();

	void setPos(const float3 &pos) override;

	void renderEditor(bool is3D, bool bRenderSelection, IXGizmoRenderer *pRenderer) override;

	void getMinMax(float3 *min, float3 *max) override;

	bool rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut = NULL, float3 *pvNormal = NULL, bool isRayInWorldSpace = true, bool bReturnNearestPoint = false) override;

	void onUse(CBaseEntity *pUser) override;

	float3 getUpPos();

private:
	void connectToLadder(CBaseEntity *pEntity);
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

private:
	float3_t m_vUpPoint = float3_t(0.0f, 2.0f, 0.0f);
	bool m_isEnabled = false;

	output_t m_onPlayerGetOn;
	output_t m_onPlayerGetOff;

	IXGhostObject *m_pGhostObject = NULL;
	IXConvexHullShape *m_pCollideShape = NULL;
	static IEventChannel<XEventPhysicsStep> *m_pTickEventChannel;
	CPhysicsLadderTickEventListener m_physicsTicker;

	Array<CBaseEntity*> m_aTouchedEntities;
};

#endif
