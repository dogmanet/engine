#include <input/sxinput.h>

#include "GameData.h"
#include "GUICraftController.h"

CGUICraftController::CGUICraftController(CCraftSystem *pCraftSystem, CCharacterInventory *pInventory) :
	m_pCraftSystem(pCraftSystem), 
	m_pInventory(pInventory)
{
	m_pCraftDesktop = GameData::m_pGUIStack->createDesktopA("game_craft", "game/craft.html");
	gui::dom::IDOMdocument *pDocument = m_pCraftDesktop->getDocument();

	m_pLeftAreaNode = pDocument->getElementById(L"left_area");
	m_pRecipeItems = pDocument->getElementById(L"recipe_items");
	m_pMainCellNode = pDocument->getElementById(L"main_cell");
	m_pCreateButton = pDocument->getElementById(L"create_button");
}

void CGUICraftController::showScreen()
{
	update();
	SSInput_SetEnable(false);
	Core_0ConsoleExecCmd("cl_grab_cursor 0");
	GameData::m_pGUIStack->pushDesktop(m_pCraftDesktop);
}

void CGUICraftController::hideScreen()
{
	Core_0ConsoleExecCmd("cl_grab_cursor 1");
	SSInput_SetEnable(true);
}

bool CGUICraftController::isActive()
{
	return(GameData::m_pGUIStack->getActiveDesktop() == m_pCraftDesktop);
}

void CGUICraftController::update()
{
	m_pActiveNode = NULL;

	removeNodeItems(true);

	const Array<CBaseRecipe*> &aRecipes = m_pInventory->getRecipes(); 

	m_pCreateButton->addPseudoclass(4);


	gui::css::ICSSstyle *pStyle = m_pMainCellNode->getStyleSelf();
	pStyle->background_image->unset();
	m_pMainCellNode->updateStyles();

	if(m_pLeftAreaNode)
	{
		wchar_t wsNode[256];

		fora(i, aRecipes)
		{
			swprintf(wsNode, L"<div class=\"list_item\" onclick=\"list_item_click\" item_id=\"%d\">%s</div>", i, CMB2WC(aRecipes[i]->getItemName()).m_szBuffer);
			m_pLeftAreaNode->appendHTML(wsNode);
		}
	}
}

void CGUICraftController::updateCurrentRecipe()
{
	if(!m_pActiveNode)
	{
		return;
	}

	gui::dom::IDOMdocument *pDocument = m_pCraftDesktop->getDocument();
	int iIndex = m_pActiveNode->getAttribute(L"item_id").toInt();
	const Array<CBaseRecipe*> &aRecipes = m_pInventory->getRecipes();
	UINT uCount = aRecipes[iIndex]->getRecipeItemsCount();

	wchar_t wsCount[16];
	wchar_t wsItemId[32];

	if(!m_pCraftSystem->canCreate(aRecipes[iIndex]))
	{
		m_pCreateButton->addPseudoclass(4);
	}
	else
	{
		m_pCreateButton->removePseudoclass(4);
	}

	while(uCount--)
	{
		const RecipeItem &item = aRecipes[iIndex]->getRecipeItems()[uCount];
		const UINT uCurrentCount = m_pInventory->getItemCount(item.sItemName.c_str());
		const wchar_t *wsClass = (uCurrentCount >= item.uCount ? L"cell" : L"cell_no_item");

		swprintf(wsItemId, L"recipe_item_%i", uCount);

		gui::dom::IDOMnode *pItem = pDocument->getElementById(wsItemId);

		if(pItem)
		{
			pItem->setAttribute(L"class", wsClass);
			pItem->updateStyles();
			gui::dom::IDOMnode *pCounter = pItem->getChilds()[0][0];

			if(pCounter)
			{
				swprintf(wsCount, L"%i/%i", uCurrentCount, item.uCount);
				pCounter->setText(wsCount, true);
			}		
		}
	}
}

void CGUICraftController::pickCraftItem(gui::IEvent *ev)
{
	if(m_pActiveNode == ev->target)
	{
		return;
	}

	if(m_pActiveNode)
	{
		m_pActiveNode->removePseudoclass(2);
	}

	m_pActiveNode = ev->target;

	if(m_pActiveNode)
	{
		m_pActiveNode->addPseudoclass(2);

		int iIndex = m_pActiveNode->getAttribute(L"item_id").toInt();
		const Array<CBaseRecipe*> &aRecipes = m_pInventory->getRecipes();
		UINT uCount = aRecipes[iIndex]->getRecipeItemsCount();

		CEntityFactoryMap *pMap = CEntityFactoryMap::GetInstance();
		EntDefaultsMap *mDefaultsMap = NULL;
		const EntDefaultsMap::Node *pNode = NULL;
		wchar_t wsNode[256];
		wchar_t wsCount[128];

		removeNodeItems();

		while(uCount--)
		{
			const RecipeItem &item = aRecipes[iIndex]->getRecipeItems()[uCount];
			const UINT uCurrentCount = m_pInventory->getItemCount(item.sItemName.c_str());
			const wchar_t *wsClass = (uCurrentCount >= item.uCount ? L"cell" : L"cell_no_item");

			mDefaultsMap = pMap->getDefaults(item.sItemName.c_str());
			mDefaultsMap->KeyExists("inv_icon", &pNode);

			swprintf(wsCount, L"<div class=\"item_count\" id=\"item_counter_%i\">%i/%i</div>", uCount, uCurrentCount, item.uCount);
			swprintf(wsNode, L"<div class=\"cell_wrapper\"><div class=\"%s\" id=\"recipe_item_%i\" style=\"background-image: /hud/items/%s.png\">%s</div></div>", wsClass, uCount, CMB2WC(*pNode->Val).m_szBuffer, wsCount);
			m_pRecipeItems->appendHTML(wsNode);
		}

		if(!m_pCraftSystem->canCreate(aRecipes[iIndex]))
		{
			m_pCreateButton->addPseudoclass(4);
		}
		else
		{
			m_pCreateButton->removePseudoclass(4);
		}

		mDefaultsMap = pMap->getDefaults(aRecipes[iIndex]->getOutItemName());
		mDefaultsMap->KeyExists("inv_icon", &pNode);
		swprintf(wsNode, L"/hud/items/%s.png", CMB2WC(*pNode->Val).m_szBuffer);
		swprintf(wsCount, L"<div class=\"item_count\">%i</div>", aRecipes[iIndex]->getOutItemCount());

		gui::css::ICSSstyle *pStyle = m_pMainCellNode->getStyleSelf();
		pStyle->background_image->set(wsNode);
		pStyle->background_size_x->set(100);
		pStyle->background_size_y->set(100);

		m_pMainCellNode->appendHTML(wsCount);
		m_pMainCellNode->updateStyles(true);
	}
}

void CGUICraftController::removeNodeItems(bool isUpdate)
{
	if(isUpdate)
	{
		while(m_pLeftAreaNode->getChilds()->size())
		{
			m_pLeftAreaNode->removeChild((*(m_pLeftAreaNode->getChilds()))[0]);
		}
	}

	while(m_pMainCellNode->getChilds()->size())
	{
		m_pMainCellNode->removeChild((*(m_pMainCellNode->getChilds()))[0]);
	}

	while(m_pRecipeItems->getChilds()->size())
	{
		m_pRecipeItems->removeChild((*(m_pRecipeItems->getChilds()))[0]);
	}
}

void CGUICraftController::createSelectedItem()
{
	if(m_pActiveNode == NULL)
	{
		return;
	}

	int iIndex = m_pActiveNode->getAttribute(L"item_id").toInt();
	const Array<CBaseRecipe*> &aRecipes = m_pInventory->getRecipes();

	m_pCraftSystem->createItem(aRecipes[iIndex]);

	updateCurrentRecipe();
}