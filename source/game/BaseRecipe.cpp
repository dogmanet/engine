#include "BaseRecipe.h"
//#include "BaseCharacter.h"
//#include "TriggerItemUse.h"
//#include <xcommon/resource/IXResourceManager.h>

/*! \skydocent base_item
Базовый объект элемента инвентаря игрока
*/

BEGIN_PROPTABLE(CBaseRecipe)
	//! Необходимые для рецепта предметы
	DEFINE_FIELD_STRINGFN(m_szRecipeItems, PDFF_NOEDIT | PDFF_NOEXPORT, "recipe_items", "", setRecipeItems, EDITOR_NONE)
	//! Выходное количество предметов
	DEFINE_FIELD_INT(m_iItemCount, PDFF_NOEDIT | PDFF_NOEXPORT, "item_count", "", EDITOR_NONE)
	//! Выходной предмет
	DEFINE_FIELD_STRING(m_szOutItem, PDFF_NOEDIT | PDFF_NOEXPORT, "recipe_out_item", "", EDITOR_NONE)
END_PROPTABLE()

REGISTER_ENTITY_NOLISTING(CBaseRecipe, base_recipe);

CBaseRecipe::~CBaseRecipe()
{
}

const Array<RecipeItem>& CBaseRecipe::getRecipeItems()
{
	return(m_aItems);
}

const char* CBaseRecipe::getOutItemName()
{
	return(m_szOutItem);
}

UINT CBaseRecipe::getOutItemCount()
{
	return(m_iItemCount);
}

UINT CBaseRecipe::getRecipeItemsCount()
{
	return(m_aItems.size());
}

void CBaseRecipe::setRecipeItems(const char *szInput)
{
	size_t sizeLen = strlen(szInput) + 1;
	char *szBuf = (char*)alloca(sizeLen);
	memcpy(szBuf, szInput, sizeLen);

	int iParts = parse_str(szBuf, NULL, 0, ',');

	char **pszParts = (char**)alloca(iParts * sizeof(char*));
	parse_str(szBuf, pszParts, iParts, ',');

	char *aszItem[2];

	for(int i = 0; i < iParts; ++i)
	{
		int iCount = parse_str(pszParts[i], aszItem, ARRAYSIZE(aszItem), ':');
		RecipeItem item;
		item.sItemName = aszItem[0];
		item.uCount = 1;
		if(iCount > 1)
		{
			if(sscanf(aszItem[1], "%d", &item.uCount) != 1)
			{
				LogError("Invalid input: item count expected '%s'\n", szInput);
			}
		}
		//! TODO: emplace back
		m_aItems.push_back(item);
	}
	_setStrVal(&m_szRecipeItems, szInput);
}