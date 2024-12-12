#include "CraftSystem.h"

CCraftSystem::CCraftSystem(CCharacterInventory *pInventory)
	:m_pInventory(pInventory)
{
	assert(m_pInventory);
}

bool CCraftSystem::canCreate(CBaseRecipe *pRecipe)
{
	assert(pRecipe);
	if(!pRecipe)
	{
		return(false);
	}

	const Array<RecipeItem> &aItems = pRecipe->getRecipeItems();
	assert(aItems.size());

	fora(i, aItems)
	{
		if (!(m_pInventory->hasItems(aItems[i].sItemName.c_str(), aItems[i].uCount)))
		{
			return(false);
		}
	}
	return(true);
}

bool CCraftSystem::createItem(CBaseRecipe *pRecipe)
{
	assert(pRecipe);

	if(!pRecipe)
	{
		return(false);
	}

	if(!canCreate(pRecipe))
	{
		return(false);
	}

	const Array<RecipeItem> &aItems = pRecipe->getRecipeItems();
	assert(aItems.size());

	fora(i, aItems)
	{
		m_pInventory->consumeItems(aItems[i].sItemName.c_str(), aItems[0].uCount);
	}

	m_pInventory->putItems(pRecipe->getOutItemName(), pRecipe->getOutItemCount());

	return(true);
}