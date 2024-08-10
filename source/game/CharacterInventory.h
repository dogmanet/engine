#ifndef __CHARACTER_INVENTORY_H
#define __CHARACTER_INVENTORY_H

#include "BaseItem.h"

class CBaseCharacter;

XENUM(EQUIP_ITEM_TYPE,
	EIT_WEAPON = 0,
	EIT_ARMOR = 1,
	EIT_QUICK_USE = 2
);

struct EquipItem
{
	CBaseItem *pItem = NULL;
	EQUIP_ITEM_TYPE type;
	UINT uIndex = 0;
};

class CCharacterInventory final
{
public: 
	CCharacterInventory(CBaseCharacter *pOwner, int iSlotCount = 8 * 16);
	~CCharacterInventory();

	void initEquipItems(UINT uCount, const EQUIP_ITEM_TYPE *pTypes);

	bool hasItems(const char *szClassName, int iCount = 1);
	int consumeItems(const char *szClassName, int iCount = 1);
	int getItemCount(const char *szClassName);

	void putItems(const char *szClassName, int iCount = 1);
	void putItem(CBaseItem *pItem);
	//void putItems(CBaseItem *pItem);
	
	void takeItem(CBaseItem *pItem);

	int getSlotCount() const;
	CBaseItem* getSlot(int i) const;

	const EquipItem* getEquipSlots() const;
	UINT getEquipSlotsCount() const;

	const EquipItem* getEquipSlot(CBaseItem *pItem) const;

	void equipItem(CBaseItem *pItem, EQUIP_ITEM_TYPE type, UINT uIndex);

	void deequipItem(CBaseItem *pItem);

	bool isEquipped(CBaseItem *pItem);

	float getTotalWeight() const;

	CBaseCharacter* getOwner();

private:
	CBaseCharacter *m_pOwner = NULL;
	CBaseItem **m_ppSlots = NULL;

	Array<EquipItem> m_aEquipItems;

	int m_iSlotCount = 0;
};

#endif
