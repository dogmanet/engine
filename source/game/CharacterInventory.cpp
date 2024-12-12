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

	fora(i, m_aRecipes)
	{
		REMOVE_ENTITY(m_aRecipes[i]);
	}
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
			iCount -= m_ppSlots[i]->m_uInvStackCurSize;
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
			if(m_ppSlots[i]->m_uInvStackCurSize >= iCount)
			{
				iConsumed += iCount;
				m_ppSlots[i]->m_uInvStackCurSize -= iCount;
				if(m_ppSlots[i]->m_uInvStackCurSize <= 0)
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
				iConsumed += m_ppSlots[i]->m_uInvStackCurSize;
				iCount -= m_ppSlots[i]->m_uInvStackCurSize;
				
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
			int iCanAdd = m_ppSlots[i]->m_uInvStackMaxSize - m_ppSlots[i]->m_uInvStackCurSize;
			if(iCanAdd > 0)
			{
				if(iCanAdd >= iCount)
				{
					m_ppSlots[i]->m_uInvStackCurSize += iCount;
					iCount = 0;
					break;
				}
				else
				{
					iCount -= iCanAdd;
					m_ppSlots[i]->m_uInvStackCurSize += iCanAdd;
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
				CBaseItem *pItem = (CBaseItem*)CREATE_ENTITY(szClassName, m_pOwner->getManager());
				if(pItem)
				{
					if(CEntityFactoryMap::IsEntityOfClass(pItem, "base_recipe"))
					{
						addRecipe((CBaseRecipe*)pItem);
						break;
					}
					else
					{
						m_ppSlots[i] = pItem;
						m_ppSlots[i]->m_uInvStackCurSize = 0;
						m_ppSlots[i]->setMode(IIM_INVENTORY);
						int iCanAdd = m_ppSlots[i]->m_uInvStackMaxSize;
						if (iCanAdd > 0)
						{
							if (iCanAdd >= iCount)
							{
								m_ppSlots[i]->m_uInvStackCurSize += iCount;
								iCount = 0;
								break;
							}
							else
							{
								iCount -= iCanAdd;
								m_ppSlots[i]->m_uInvStackCurSize += iCanAdd;
							}
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

	if(CEntityFactoryMap::IsEntityOfClass(pItem, "base_recipe"))
	{
		addRecipe((CBaseRecipe*)pItem);
	}
	else
	{
		for (int i = 0; i < m_iSlotCount; ++i)
		{
			if(m_ppSlots[i] && pItem->m_bInvStackable && !fstrcmp(m_ppSlots[i]->getClassName(), pItem->getClassName()))
			{
				int iCanAdd = m_ppSlots[i]->m_uInvStackMaxSize - m_ppSlots[i]->m_uInvStackCurSize;
				if(iCanAdd > 0)
				{
					if(iCanAdd >= pItem->m_uInvStackCurSize)
					{
						m_ppSlots[i]->m_uInvStackCurSize += pItem->m_uInvStackCurSize;
						pItem->m_uInvStackCurSize = 0;
						break;
					}
					else
					{
						pItem->m_uInvStackCurSize -= iCanAdd;
						m_ppSlots[i]->m_uInvStackCurSize += iCanAdd;
					}
				}
			}
		}

		if (pItem->m_uInvStackCurSize > 0 || !pItem->m_bInvStackable)
		{
			for (int i = 0; i < m_iSlotCount; ++i)
			{
				if(!m_ppSlots[i])
				{
					if(pItem->m_bInvStackable)
					{
						if((m_ppSlots[i] = (CBaseItem*)CREATE_ENTITY(pItem->getClassName(), m_pOwner->getManager())))
						{
							m_ppSlots[i]->m_uInvStackCurSize = 0;
							m_ppSlots[i]->setMode(IIM_INVENTORY);
							int iCanAdd = m_ppSlots[i]->m_uInvStackMaxSize;
							if(iCanAdd > 0)
							{
								if(iCanAdd >= pItem->m_uInvStackCurSize)
								{
									m_ppSlots[i]->m_uInvStackCurSize += pItem->m_uInvStackCurSize;
									pItem->m_uInvStackCurSize = 0;
									break;
								}
								else
								{
									pItem->m_uInvStackCurSize -= iCanAdd;
									m_ppSlots[i]->m_uInvStackCurSize += iCanAdd;
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
	}

	//! TODO: убрать блять что б этого тут небыло
	CHUDcontroller *pHUD = m_pOwner->getHUDcontroller();

	if(pHUD)
	{
		char str[128];
		sprintf(str, "Найден предмет: %s", pItem->m_szInvName);
		pHUD->chatMsg(str);
	}

	m_pOwner->onInventoryChanged();
}

void CCharacterInventory::addRecipe(CBaseRecipe *pRecipe)
{
	if(pRecipe)
	{
		if(m_aRecipes.indexOf(pRecipe, [](CBaseRecipe *pA, CBaseRecipe *pB) 
		{
			//! HACK: Будет работать потому что гладиолус (хак)
			return(pA->getClassName() == pB->getClassName());
		}) < 0)
		{
			pRecipe->setMode(IIM_INVENTORY);
			pRecipe->setFlags(pRecipe->getFlags() & ~EF_LEVEL);

			m_aRecipes.push_back(pRecipe);
		}
	}
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
			fTotal += m_ppSlots[i]->getWeight() * m_ppSlots[i]->m_uInvStackCurSize;
		}
	}

	return(fTotal);
}

CBaseCharacter* CCharacterInventory::getOwner()
{
	return(m_pOwner);
}

const Array<CBaseRecipe*> CCharacterInventory::getRecipes()
{
	return(m_aRecipes);
}

UINT CCharacterInventory::getItemCount(const char * szClassName)
{
	UINT uCount = 0;
	for(int i = 0; i < m_iSlotCount; ++i)
	{
		if(m_ppSlots[i] && !fstrcmp(m_ppSlots[i]->getClassName(), szClassName))
		{
			uCount += m_ppSlots[i]->m_uInvStackCurSize;
		}
	}
	return(uCount);
}

UINT CCharacterInventory::getItemCount(CBaseItem *pItem)
{
	assert(pItem);

	return(getItemCount(pItem->getClassName()));
}
