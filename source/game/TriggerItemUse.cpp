
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "TriggerItemUse.h"

/*! \skydocent trigger_itemuse
Триггер телепортации
*/

BEGIN_PROPTABLE(CTriggerItemUse)
END_PROPTABLE()

REGISTER_ENTITY_NOLISTING(CTriggerItemUse, trigger_itemuse);

void CTriggerItemUse::setItem(CBaseItem *pItem)
{
	m_pItem = pItem;
}

void CTriggerItemUse::onTouchStart(CBaseEntity *pActivator)
{
	if(!m_pItem || fstrcmp(pActivator->getClassName(), "player")/* || !pActivator->isCharacter()*/)
	{
		return;
	}
	disable();

	m_pItem->onUse(pActivator);
}
