#ifndef __CRAFT_SYSTEM_H
#define __CRAFT_SYSTEM_H

#include "CharacterInventory.h"
#include "BaseRecipe.h"

class CCraftSystem final
{
public:
	CCraftSystem(CCharacterInventory *pInventory);

	bool canCreate(CBaseRecipe *pRecipe);

	bool createItem(CBaseRecipe *pRecipe);

private:
	CCharacterInventory *m_pInventory = NULL;
};

#endif
