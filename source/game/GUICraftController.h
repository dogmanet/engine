#ifndef __GUI_CRAFT_CONTROLLER_H
#define __GUI_CRAFT_CONTROLLER_H

#include <gui/guimain.h>
#include "CraftSystem.h"

class CGUICraftController
{
public:
	CGUICraftController(CCraftSystem *pCraftSystem, CCharacterInventory *pInventory);

	void showScreen();

	void hideScreen();

	bool isActive();

	void update();

	void pickCraftItem(gui::IEvent *ev);

	void removeNodeItems(bool isUpdate = false);

private:
	gui::IDesktop *m_pCraftDesktop = NULL;
	CCraftSystem *m_pCraftSystem = NULL;
	CCharacterInventory *m_pInventory = NULL;

	gui::dom::IDOMnode *m_pLeftAreaNode = NULL;
	gui::dom::IDOMnode *m_pRecipeItems = NULL;
	gui::dom::IDOMnode *m_pActiveNode = NULL;
	gui::dom::IDOMnode *m_pMainCellNode = NULL;
};

#endif 