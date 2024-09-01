#ifndef __GUI_INVENTORY_CONTROLLER_H
#define __GUI_INVENTORY_CONTROLLER_H

#include <gui/guimain.h>

#include "CharacterInventory.h"

#define INV_ROW_SIZE 14
#define INV_CELL_SIZE 5

class CGUIInventoryController
{
public:
	CGUIInventoryController(CCharacterInventory *pInventory);

	void showScreen();

	void hideScreen();

	bool isActive();

	void update();

	void openContextMenu(gui::IEvent *ev);

	void closeContextMenu(gui::IEvent *ev);

	void beginDrag(gui::IEvent *ev);

	void dragMove(gui::IEvent *ev);

	void endDrag(gui::IEvent *ev);

	void dropItem(gui::IEvent *ev);
private:
	POINT getPointsForMap(UINT uSizeX, UINT uSizeY);

	void updateOcclusionMap(const POINT &pt, UINT uSizeX, UINT uSizeY);
private:
	struct EquipArea
	{
		gui::dom::IDOMnode *pNode = NULL;
		gui::dom::IDOMnode *pCellNode = NULL;
		EQUIP_ITEM_TYPE type;
		UINT uIndex = 0;
	};

	Array<EquipArea> m_aEquipAreas;

	CCharacterInventory *m_pInventory = NULL;

	gui::IDesktop *m_pInventoryDesktop = NULL;
	gui::dom::IDOMnode *m_pContextMenuNode = NULL;
	gui::dom::IDOMnode *m_pDragNode = NULL;
	gui::dom::IDOMnode *m_pSelectedNode = NULL;
	gui::dom::IDOMnode *m_pItemContainerNode = NULL;
	gui::dom::IDOMnode *m_pEquipContainerNode = NULL;
	gui::dom::IDOMnode *m_pOriginalContainerNode = NULL;
	gui::dom::IDOMnode *m_pFrameNode = NULL;

	Array<bool> m_aOcclussionMap;

	UINT m_uRows = 0;
	UINT m_uCells = 0;

	int m_iOffsetY = 0;
	int m_iOffsetX = 0;
	int m_iStartDragY = 0;
	int m_iStartDragX = 0;
};

#endif 