
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "TriggerHurt.h"

/*! \skydocent trigger_hurt
Триггер нанесения урона
*/

BEGIN_PROPTABLE(CTriggerHurt)
	//! Сколько урона наносится за удар
	DEFINE_FIELD_FLOAT(m_fDamage, PDFF_NONE, "damage", "Damage per interval", EDITOR_TEXTFIELD)
	//! Период нанесения урона
	DEFINE_FIELD_FLOAT(m_fHurtInterval, PDFF_NONE, "hurt_interval", "Hurt interval", EDITOR_TEXTFIELD)
END_PROPTABLE()

REGISTER_ENTITY(CTriggerHurt, trigger_hurt, REC_MODEL_FIELD("model"));

CTriggerHurt::~CTriggerHurt()
{
	CLEAR_INTERVAL(m_idHurtInterval);
}

void CTriggerHurt::onTouchStart(CBaseEntity *pActivator)
{
	if(!ID_VALID(m_idHurtInterval))
	{
		m_idHurtInterval = SET_INTERVAL(think, m_fHurtInterval);
	}
	BaseClass::onTouchStart(pActivator);
}
void CTriggerHurt::onTouchEndAll(CBaseEntity *pActivator)
{
	CLEAR_INTERVAL(m_idHurtInterval);
	BaseClass::onTouchEnd(pActivator);
}

void CTriggerHurt::think(float dt)
{
	if(!m_bEnabled)
	{
		return;
	}

	CTakeDamageInfo tdi(this, m_fDamage);

	for(UINT i = 0, l = m_aTouches.size(); i < l; ++i)
	{
		m_aTouches[i]->dispatchDamage(tdi);
	}
}

