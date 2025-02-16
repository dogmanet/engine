
#include "ZombieHands.h"
#include "BaseCharacter.h"
#include "Tracer.h"

/*! \skydocent wpn_zombie_hands
Руки зомби
*/

BEGIN_PROPTABLE(CZombieHands)
	//! Наносимый урон
	DEFINE_FIELD_FLOAT(m_fDamage, PDFF_NOEDIT | PDFF_NOEXPORT, "damage", "", EDITOR_NONE)
END_PROPTABLE()

REGISTER_ENTITY_NOLISTING(CZombieHands, wpn_zombie_hands);


void CZombieHands::taskShoot(float dt)
{
	extern CTracer *g_pTracer;

	CBaseCharacter * pOwner = (CBaseCharacter*)getOwner();

	pOwner->playActivity("ACT_SHOOT", 200);
	pOwner->playActivityNext("ACT_IDLE", 200);

	SAFE_CALL(m_pSndShoot, setWorldPos, getPos());
	SAFE_CALL(m_pSndShoot, play);

	m_idActualShootTimeout = SET_TIMEOUT(actualShoot, 0.6f);
}

void CZombieHands::actualShoot(float dt)
{
	m_idActualShootTimeout = -1;

	CBaseCharacter * pOwner = (CBaseCharacter*)getOwner();

	float3 start = getPos() + float3(0.0f, -0.2f, 0.0f);
	float3 dir = m_pParent->getParent()->getOrient() * float3(0.0f, 0.0f, -1.0f);

	dir = applySpread(dir, SMToRadian(pOwner->getCurrentSpread()));

	float3 end = start + dir * m_fEffectiveDistance;
	CClosestNotMeRayResultCallback cb(pOwner->getBtCollisionObject());
	GetPhysWorld()->rayTest(start, end, &cb);

	//g_pTracer->begin(start);
	//g_pTracer->lineTo(end);
	//g_pTracer->end();

	if(cb.hasHit())
	{
		SAFE_CALL(GetDecalProvider(), shootDecal, XDT_BLOOD_BIG, cb.m_result.vHitPoint, cb.m_result.vHitNormal);

		if(cb.m_result.pCollisionObject->getUserPointer() && cb.m_result.pCollisionObject->getUserTypeId() == 1)
		{
			CTakeDamageInfo takeDamageInfo(pOwner, m_fDamage);
			takeDamageInfo.m_pInflictor = this;

			((CBaseEntity*)cb.m_result.pCollisionObject->getUserPointer())->dispatchDamage(takeDamageInfo);
		}
	}
}

void CZombieHands::stopAction()
{
	if(ID_VALID(m_idActualShootTimeout))
	{
		CLEAR_TIMEOUT(m_idActualShootTimeout);
	}
}
