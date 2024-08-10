#include "CharacterInventory.h"

#include "BaseCharacter.h"
#include "HUDcontroller.h"

CCharacterInventory::CCharacterInventory(CBaseCharacter * pOwner, int iSlotCount):
	m_pOwner(pOwner),
	m_iSlotCount(iSlotCount)
{
	assert(iSlotCount > 0);

	m_ppSlots = new CBaseItem*[iSlotCount];
	memset(m_ppSlots, 0, sizeof(CBaseItem*) * iSlotCount);
}

CCharacterInventory::~CCharacterInventory()
{
	for(int i = 0; i < m_iSlotCount; ++i)
	{
		if(m_ppSlots[i])
		{
			REMOVE_ENTITY(m_ppSlots[i]);
		}
	}
	mem_delete_a(m_ppSlots);
}

void CCharacterInventory::initEquipItems(UINT uCount, const EQUIP_ITEM_TYPE *pTypes)
{
	m_aEquipItems.resize(uCount);

	auto &enumReflector = EnumReflector::Get<EQUIP_ITEM_TYPE>();

	UINT *pTypeCounts = (UINT*)alloca(sizeof(UINT) * enumReflector.getCount());
	memset(pTypeCounts, 0, sizeof(UINT) * enumReflector.getCount());

	fora(i, m_aEquipItems)
	{
		m_aEquipItems[i].type = pTypes[i];
		m_aEquipItems[i].uIndex = pTypeCounts[pTypes[i]]++;
	}
}

bool CCharacterInventory::hasItems(const char * szClassName, int iCount)
{
	assert(iCount > 0);

	for(int i = 0; i < m_iSlotCount; ++i)
	{
		if(m_ppSlots[i] && !fstrcmp(m_ppSlots[i]->getClassName(), szClassName))
		{
			iCount -= m_ppSlots[i]->m_iInvStackCurSize;
			if(iCount <= 0)
			{
				return(true);
			}
		}
	}
	return(iCount <= 0);
}

int CCharacterInventory::consumeItems(const char *szClassName, int iCount)
{
	assert(iCount > 0);

	int iConsumed = 0;

	for(int i = 0; i < m_iSlotCount; ++i)
	{
		if(m_ppSlots[i] && !fstrcmp(m_ppSlots[i]->getClassName(), szClassName))
		{
			if(m_ppSlots[i]->m_iInvStackCurSize >= iCount)
			{
				iConsumed += iCount;
				m_ppSlots[i]->m_iInvStackCurSize -= iCount;
				if(m_ppSlots[i]->m_iInvStackCurSize <= 0)
				{
					fora(j, m_aEquipItems)
					{
						if(m_aEquipItems[j].pItem == m_ppSlots[i])
						{
							m_aEquipItems[j].pItem = NULL;
							break;
						}
					}
					REMOVE_ENTITY(m_ppSlots[i]);
					m_ppSlots[i] = 0;
				}
				break;
			}
			else
			{
				iConsumed += m_ppSlots[i]->m_iInvStackCurSize;
				iCount -= m_ppSlots[i]->m_iInvStackCurSize;
				
				fora(j, m_aEquipItems)
				{
					if(m_aEquipItems[j].pItem == m_ppSlots[i])
					{
						m_aEquipItems[j].pItem = NULL;
						break;
					}
				}

				REMOVE_ENTITY(m_ppSlots[i]);
				m_ppSlots[i] = 0;
			}
		}
	}
	m_pOwner->onInventoryChanged();
	return(iConsumed);
}

void CCharacterInventory::putItems(const char *szClassName, int iCount)
{
	assert(iCount > 0);
	for(int i = 0; i < m_iSlotCount; ++i)
	{
		if(m_ppSlots[i] && !fstrcmp(m_ppSlots[i]->getClassName(), szClassName) && m_ppSlots[i]->m_bInvStackable)
		{
			int iCanAdd = m_ppSlots[i]->m_iInvStackMaxSize - m_ppSlots[i]->m_iInvStackCurSize;
			if(iCanAdd > 0)
			{
				if(iCanAdd >= iCount)
				{
					m_ppSlots[i]->m_iInvStackCurSize += iCount;
					iCount = 0;
					break;
				}
				else
				{
					iCount -= iCanAdd;
					m_ppSlots[i]->m_iInvStackCurSize += iCanAdd;
				}
			}
		}
	}

	if(iCount > 0)
	{
		for(int i = 0; i < m_iSlotCount; ++i)
		{
			if(!m_ppSlots[i])
			{
				if((m_ppSlots[i] = (CBaseItem*)CREATE_ENTITY(szClassName, m_pOwner->getManager())))
				{
					m_ppSlots[i]->m_iInvStackCurSize = 0;
					m_ppSlots[i]->setMode(IIM_INVENTORY);
					int iCanAdd = m_ppSlots[i]->m_iInvStackMaxSize;
					if(iCanAdd > 0)
					{
						if(iCanAdd >= iCount)
						{
							m_ppSlots[i]->m_iInvStackCurSize += iCount;
							iCount = 0;
							break;
						}
						else
						{
							iCount -= iCanAdd;
							m_ppSlots[i]->m_iInvStackCurSize += iCanAdd;
						}
					}
				}
				else
				{
					break;
				}
			}
		}
	}
	m_pOwner->onInventoryChanged();
}

void CCharacterInventory::putItem(CBaseItem *pItem)
{
	assert(pItem);

	for(int i = 0; i < m_iSlotCount; ++i)
	{
		if(m_ppSlots[i] && pItem->m_bInvStackable && !fstrcmp(m_ppSlots[i]->getClassName(), pItem->getClassName()))
		{
			int iCanAdd = m_ppSlots[i]->m_iInvStackMaxSize - m_ppSlots[i]->m_iInvStackCurSize;
			if(iCanAdd > 0)
			{
				if(iCanAdd >= pItem->m_iInvStackCurSize)
				{
					m_ppSlots[i]->m_iInvStackCurSize += pItem->m_iInvStackCurSize;
					pItem->m_iInvStackCurSize = 0;
					break;
				}
				else
				{
					pItem->m_iInvStackCurSize -= iCanAdd;
					m_ppSlots[i]->m_iInvStackCurSize += iCanAdd;
				}
			}
		}
	}

	if(pItem->m_iInvStackCurSize > 0 || !pItem->m_bInvStackable)
	{
		for(int i = 0; i < m_iSlotCount; ++i)
		{
			if(!m_ppSlots[i])
			{
				if(pItem->m_bInvStackable)
				{
					if((m_ppSlots[i] = (CBaseItem*)CREATE_ENTITY(pItem->getClassName(), m_pOwner->getManager())))
					{
						m_ppSlots[i]->m_iInvStackCurSize = 0;
						m_ppSlots[i]->setMode(IIM_INVENTORY);
						int iCanAdd = m_ppSlots[i]->m_iInvStackMaxSize;
						if(iCanAdd > 0)
						{
							if(iCanAdd >= pItem->m_iInvStackCurSize)
							{
								m_ppSlots[i]->m_iInvStackCurSize += pItem->m_iInvStackCurSize;
								pItem->m_iInvStackCurSize = 0;
								break;
							}
							else
							{
								pItem->m_iInvStackCurSize -= iCanAdd;
								m_ppSlots[i]->m_iInvStackCurSize += iCanAdd;
							}
						}
					}
					else
					{
						break;
					}
				}
				else
				{
					m_ppSlots[i] = pItem;
					pItem->setMode(IIM_INVENTORY);
					break;
				}
			}
		}
	}

	CHUDcontroller *pHUD = m_pOwner->getHUDcontroller();

	if(pHUD)
	{
		char str[128];
		sprintf(str, "Найден предмет: %s", pItem->m_szInvName);
		pHUD->chatMsg(str);
	}

	m_pOwner->onInventoryChanged();
}

void CCharacterInventory::takeItem(CBaseItem *pItem)
{
	if(isEquipped(pItem))
	{
		deequipItem(pItem);
	}

	for(UINT i = 0; i < m_iSlotCount; ++i)
	{
		if(m_ppSlots[i] == pItem)
		{
			m_ppSlots[i] = NULL;
			break;
		}
	}

	m_pOwner->onInventoryChanged();
}

int CCharacterInventory::getSlotCount() const
{
	return(m_iSlotCount);
}

CBaseItem * CCharacterInventory::getSlot(int i) const
{
	assert(i >= 0 && i < m_iSlotCount);

	return(m_ppSlots[i]);
}

const EquipItem* CCharacterInventory::getEquipSlots() const
{
	return(m_aEquipItems);
}

UINT CCharacterInventory::getEquipSlotsCount() const
{
	return(m_aEquipItems.size());
}

const EquipItem* CCharacterInventory::getEquipSlot(CBaseItem *pItem) const
{
	int idx = m_aEquipItems.indexOf(pItem, [](const EquipItem &item, const CBaseItem *pItem)
	{
		return(item.pItem == pItem);
	});

	return(idx < 0 ? NULL : &m_aEquipItems[idx]);
}

void CCharacterInventory::equipItem(CBaseItem *pItem, EQUIP_ITEM_TYPE type, UINT uIndex)
{
	if(isEquipped(pItem))
	{
		fora(j, m_aEquipItems)
		{
			if(m_aEquipItems[j].pItem == pItem)
			{
				m_aEquipItems[j].pItem = NULL;
				break;
			}
		}
	}

	fora(i, m_aEquipItems)
	{
		if(m_aEquipItems[i].type == type && uIndex == m_aEquipItems[i].uIndex)
		{
			SAFE_CALL(m_aEquipItems[i].pItem, setMode, IIM_INVENTORY);

			m_aEquipItems[i].pItem = pItem;
			SAFE_CALL(m_aEquipItems[i].pItem, setMode, IIM_EQUIPPED);
			return;
		}
	}

	EquipItem item;
	item.pItem = pItem;
	item.type = type;
	item.uIndex = uIndex;

	m_aEquipItems.push_back(item);
	SAFE_CALL(pItem, setMode, IIM_EQUIPPED);
}

void CCharacterInventory::deequipItem(CBaseItem *pItem)
{
	fora(i, m_aEquipItems)
	{
		if(m_aEquipItems[i].pItem == pItem)
		{
			SAFE_CALL(m_aEquipItems[i].pItem, setMode, IIM_INVENTORY);
			m_aEquipItems[i].pItem = NULL;
			return;
		}
	}
}

bool CCharacterInventory::isEquipped(CBaseItem *pItem)
{
	return(pItem->getMode() == IIM_EQUIPPED || pItem->getMode() == IIM_IN_HANDS);
}

float CCharacterInventory::getTotalWeight() const
{
	float fTotal = 0.0f;

	for(int i = 0; i < m_iSlotCount; ++i)
	{
		if(m_ppSlots[i])
		{
			fTotal += m_ppSlots[i]->getWeight() * m_ppSlots[i]->m_iInvStackCurSize;
		}
	}

	return(fTotal);
}

CBaseCharacter* CCharacterInventory::getOwner()
{
	return(m_pOwner);
}

int CCharacterInventory::getItemCount(const char * szClassName)
{
	int iCount = 0;
	for(int i = 0; i < m_iSlotCount; ++i)
	{
		if(m_ppSlots[i] && !fstrcmp(m_ppSlots[i]->getClassName(), szClassName))
		{
			iCount += m_ppSlots[i]->m_iInvStackCurSize;
		}
	}
	return(iCount);
}
