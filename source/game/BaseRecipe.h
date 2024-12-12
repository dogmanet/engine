/*!
\file
Рецепты крафта
*/

/*! \defgroup cbaseitem Рецепты крафта
\ingroup cbaseanimating
@{
*/

#ifndef __BASE_RECIPE_H
#define __BASE_RECIPE_H

#include "BaseItem.h"

struct RecipeItem
{
	String sItemName;
	UINT uCount;
};

//! Базовый класс рецепта крафта
class CBaseRecipe: public CBaseItem
{
	DECLARE_CLASS(CBaseRecipe, CBaseItem);
	DECLARE_PROPTABLE();
public:
	DECLARE_TRIVIAL_CONSTRUCTOR();

	const Array<RecipeItem> &getRecipeItems();

	const char *getOutItemName();

	UINT getOutItemCount();

	UINT getRecipeItemsCount();
private:
	void setRecipeItems(const char *szInput);

	const char *m_szRecipeItems;
	const char *m_szOutItem;
	int m_iItemCount = 1;

	Array<RecipeItem> m_aItems;
};

#endif

//! @