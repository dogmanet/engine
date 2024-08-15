
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "TriggerTeleport.h"

/*! \skydocent trigger_teleport
Триггер телепортации
*/

BEGIN_PROPTABLE(CTriggerTeleport)
	//! Локальный маркер для относительных координат
	DEFINE_FIELD_ENTITY(CBaseEntity, m_pLandmark, PDFF_NONE, "landmark", "Landmark object", EDITOR_TEXTFIELD)
	//! Цель
	DEFINE_FIELD_ENTITY(CBaseEntity, m_pDestination, PDFF_NONE, "destination", "Destination landmark object", EDITOR_TEXTFIELD)
END_PROPTABLE()

REGISTER_ENTITY(CTriggerTeleport, trigger_teleport, REC_MODEL_FIELD("model"));

void CTriggerTeleport::onTouchStart(CBaseEntity *pActivator)
{
	BaseClass::onTouchStart(pActivator);

	if(!m_pDestination)
	{
		return;
	}
	CBaseEntity *pEnt = pActivator;
	while(pEnt->getParent())
	{
		pEnt = pEnt->getParent();
	}
	if(m_pLandmark)
	{
		pEnt->setPos(pEnt->getPos() - m_pLandmark->getPos() + m_pDestination->getPos());
		pEnt->setOrient(pEnt->getOrient() * m_pLandmark->getOrient().Conjugate() * m_pDestination->getOrient());
	}
	else
	{
		pEnt->setPos(m_pDestination->getPos());
		pEnt->setOrient(m_pDestination->getOrient());
	}
}
