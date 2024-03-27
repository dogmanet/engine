#include <input/sxinput.h>

#include "EntityFactory.h"
#include "GameData.h"
#include "GUIInventoryController.h"

CGUIInventoryController::CGUIInventoryController(CCharacterInventory *pInventory) :
m_pInventory(pInventory)
{
	m_pInventoryDesktop = GameData::m_pGUIStack->createDesktopA("game_inventory", "game/inventory.html");
	m_pContextMenuNode = m_pInventoryDesktop->getDocument()->getElementById(L"context_menu");
	m_pItemContainerNode = m_pInventoryDesktop->getDocument()->getElementById(L"item_container");
	m_pEquipContainerNode = m_pInventoryDesktop->getDocument()->getElementById(L"equip_container");
	m_pFrameNode = m_pInventoryDesktop->getDocument()->getElementById(L"frame");

	m_uRows = m_pFrameNode->getChilds()->size() - 1;
	m_uCells = m_pFrameNode->getChilds()[0][0]->getChilds()->size();

	m_aOcclussionMap.resize(m_uRows * m_uCells);

	const gui::dom::IDOMnodeCollection *pEquipAreasNodes = m_pInventoryDesktop->getDocument()->getElementsByClass(L"equip_unit_grid");

	m_aEquipAreas.resize(pEquipAreasNodes->size());
	auto &enumReflector = EnumReflector::Get<EQUIP_ITEM_TYPE>();

	UINT *pTypeCounts = (UINT*)alloca(sizeof(UINT) * enumReflector.getCount());

	memset(pTypeCounts, 0, sizeof(UINT) * enumReflector.getCount());

	fora(i, *pEquipAreasNodes)
	{
		m_aEquipAreas[i].pNode = (*pEquipAreasNodes)[i];

		CWC2MB szTmp((*pEquipAreasNodes)[i]->getAttribute(L"equip_type").c_str());
		auto enumerator = enumReflector.find(szTmp);

		if(enumerator != enumReflector.end())
		{
			m_aEquipAreas[i].type = (EQUIP_ITEM_TYPE)enumerator.getValue();
			m_aEquipAreas[i].uIndex = pTypeCounts[enumerator.getValue()]++;
			m_aEquipAreas[i].pNode->setAttribute(L"equip_index", (unsigned long)m_aEquipAreas[i].uIndex);

			if(m_aEquipAreas[i].pNode->getAttribute(L"split_container").toBool())
			{
				const auto &aRows = *(m_aEquipAreas[i].pNode->getChilds());
				EquipArea newArea = m_aEquipAreas[i];
				fora(j, aRows)
				{
					const auto &aCells = *(aRows[j]->getChilds());
					fora(k, aCells)
					{
						if(j == 0 && k == 0)
						{
							m_aEquipAreas[i].pCellNode = aCells[k];
							m_aEquipAreas[i].pCellNode->setAttribute(L"equip_index", (unsigned long)m_aEquipAreas[i].uIndex);
						}
						else
						{
							newArea.uIndex = pTypeCounts[enumerator.getValue()]++;
							newArea.pCellNode = aCells[k];
							newArea.pCellNode->setAttribute(L"equip_index", (unsigned long)newArea.uIndex);

							m_aEquipAreas.push_back(newArea);
						}
					}
				}
			}
		}
		else
		{
			LogError("Unknown equip type: %s!\n", szTmp);
			assert(!"Unknown equip type");
			m_aEquipAreas[i].pNode = NULL;
		}
	}
	
	forar(i, m_aEquipAreas)
	{
		if(!m_aEquipAreas[i].pNode)
		{
			m_aEquipAreas.erase(i);
		}
	}

	if(!m_pContextMenuNode)
	{
		LogFatal("Context menu is not exist\n");
	}
}

void CGUIInventoryController::showScreen()
{
	update();
	SSInput_SetEnable(false);
	Core_0ConsoleExecCmd("cl_grab_cursor 0");
	GameData::m_pGUIStack->pushDesktop(m_pInventoryDesktop);
}

void CGUIInventoryController::hideScreen()
{
	Core_0ConsoleExecCmd("cl_grab_cursor 1");
	SSInput_SetEnable(true);
	m_pDragNode = NULL;
}

bool CGUIInventoryController::isActive()
{
	return(GameData::m_pGUIStack->getActiveDesktop() == m_pInventoryDesktop);
}

void CGUIInventoryController::update()
{
	char szBuffer[1024];
	char szItemCount[128];

	fora(i, m_aOcclussionMap)
	{
		m_aOcclussionMap[i] = false;
	}

	while(m_pItemContainerNode->getChilds()->size())
	{
		m_pItemContainerNode->removeChild((*(m_pItemContainerNode->getChilds()))[0]);
	}

	while(m_pEquipContainerNode->getChilds()->size())
	{
		m_pEquipContainerNode->removeChild((*(m_pEquipContainerNode->getChilds()))[0]);
	}

	for(int i = 0, l = m_pInventory->getSlotCount(); i < l; ++i)
	{
		CBaseItem *pItem = m_pInventory->getSlot(i);
		gui::dom::IDOMnode *pCurrentContainer = NULL;

		if(pItem)
		{
			bool isEquipped = m_pInventory->isEquipped(pItem);
			int x = 0, y = 0;

			if(isEquipped)
			{
				pCurrentContainer = m_pEquipContainerNode;
				int idx = m_aEquipAreas.indexOf(m_pInventory->getEquipSlot(pItem), [](const EquipArea &area, const EquipItem *pItem)
				{
					return(area.type == pItem->type && area.uIndex == pItem->uIndex);
				});
				if(idx >= 0)
				{
					RECT rect;
					if(m_aEquipAreas[idx].pCellNode)
					{
						rect = m_aEquipAreas[idx].pCellNode->getClientRect();
					}
					else
					{
						rect = m_aEquipAreas[idx].pNode->getClientRect();
					}

					RECT rectEquip = m_pEquipContainerNode->getClientRect();

					x = rect.left - rectEquip.left;
					y = rect.top - rectEquip.top;
				}
			}
			else
			{
				pCurrentContainer = m_pItemContainerNode;

				POINT pt = getPointsForMap(pItem->getIconSizeX(), pItem->getIconSizeY());
				if(pt.x != -1)
				{
					updateOcclusionMap(pt, pItem->getIconSizeX(), pItem->getIconSizeY());
				}

				x = pt.x * INV_CELL_SIZE;
				y = pt.y * INV_CELL_SIZE;
			}

			int iCount = pItem->getStackCount();
			const char *szName = pItem->getItemName();
			const char *szIcon = pItem->getIcon();
			const char *szDim = isEquipped ? "px" : "vw";

			if(pItem->isStackable())
			{
				sprintf_s(szItemCount, "<div class=\"item_count\">%i</div>", iCount);
			}
			else
			{
				szItemCount[0] = 0;
			}

			sprintf_s(szBuffer, "<div inventoryid=\"%i\" onmousedown=\"begin_drag\" onclick=\"open_menu\" class=\"icon\" style=\"background-image: /hud/items/%s.png; top: %i%s; left: %i%s; width: %ivw; height: %ivw;\"><div class=\"item_name\">%s</div>%s</div>",
				i, szIcon, y, szDim, x, szDim, INV_CELL_SIZE * pItem->getIconSizeX(), INV_CELL_SIZE * pItem->getIconSizeY(), szName, szItemCount);

			gui::dom::IDOMnodeCollection newItems = m_pInventoryDesktop->createFromText(StringW(CMB2WC(szBuffer)));

			pCurrentContainer->appendChild(newItems[0]);
		}
	}

	const EquipItem *pItems = m_pInventory->getEquipSlots();
	for(int i = 0, l = m_pInventory->getEquipSlotsCount(); i < l; ++i)
	{
		if(pItems[i].pItem)
		{
			int idx = m_aEquipAreas.indexOf(pItems[i], [](const EquipArea &area, const EquipItem &item)
			{
				return(area.type == item.type && area.uIndex == item.uIndex);
			});

			if(idx >= 0)
			{
				RECT rect = m_aEquipAreas[idx].pNode->getClientRect();
			}
		}
	}

	gui::dom::IDOMnode *pCurrentWeightNode = m_pInventoryDesktop->getDocument()->getElementById(L"capacity_current");
	if(pCurrentWeightNode)
	{
		pCurrentWeightNode->setText(m_pInventory->getTotalWeight(), TRUE);
	}
}

void CGUIInventoryController::openContextMenu(gui::IEvent *ev)
{
	if(ev->key == KEY_RBUTTON)
	{
		gui::css::ICSSstyle *pStyle = m_pContextMenuNode->getStyleSelf();

		pStyle->visibility->set(gui::css::ICSSproperty::VISIBILITY_VISIBLE);
		pStyle->top->set(ev->clientY);
		pStyle->top->setDim(gui::css::ICSSproperty::DIM_PX);
		pStyle->left->set(ev->clientX);
		pStyle->left->setDim(gui::css::ICSSproperty::DIM_PX);

		m_pSelectedNode = ev->currentTarget;;

		m_pContextMenuNode->updateStyles();
	}
}

void CGUIInventoryController::closeContextMenu(gui::IEvent *ev)
{
	if(ev->key == KEY_LBUTTON)
	{
		gui::css::ICSSstyle *pStyle = m_pContextMenuNode->getStyleSelf();

		pStyle->visibility->set(gui::css::ICSSproperty::VISIBILITY_HIDDEN);

		m_pSelectedNode = NULL;

		m_pContextMenuNode->updateStyles();
	}
}

void CGUIInventoryController::beginDrag(gui::IEvent *ev)
{
	if(ev->key == KEY_LBUTTON)
	{
		if(m_pDragNode)
		{
			return;
		}

		m_pDragNode = ev->currentTarget;
		RECT itemRect = m_pDragNode->getClientRect();

		m_iOffsetY = ev->clientY - itemRect.top;
		m_iOffsetX = ev->clientX - itemRect.left;

		m_iStartDragY = m_pDragNode->getClientRect().top;
		m_iStartDragX = m_pDragNode->getClientRect().left;

		m_pOriginalContainerNode = m_pDragNode->parentNode();
		m_pOriginalContainerNode->takeChild(m_pDragNode);
		gui::dom::IDOMnode *pNode = m_pInventoryDesktop->getDocument()->getElementsByTag(L"body")[0][0];
		pNode->appendChild(m_pDragNode);
		dragMove(ev);
	}
}

void CGUIInventoryController::dragMove(gui::IEvent *ev)
{
	if(m_pDragNode)
	{
		gui::css::ICSSstyle *pStyle = m_pDragNode->getStyleSelf();
		
		pStyle->top->set(ev->clientY /*- parentRect.top*/ - m_iOffsetY);
		pStyle->top->setDim(gui::css::ICSSproperty::DIM_PX);
		pStyle->left->set(ev->clientX /*- parentRect.left*/ - m_iOffsetX);
		pStyle->left->setDim(gui::css::ICSSproperty::DIM_PX);

		m_pContextMenuNode->updateStyles();
	}
}

POINT CGUIInventoryController::getPointsForMap(UINT uSizeX, UINT uSizeY)
{
#define INDEX(x,y) ((x)+(y)*m_uCells)
	bool bSkip = false;
	if(uSizeX > m_uCells || uSizeY > m_uRows)
	{
		assert(!"Invalid item size");
		return(POINT{-1, -1});
	}
	for(UINT i = 0; i < m_uRows - uSizeY + 1; ++i)
	{
		for(UINT j = 0; j < m_uCells - uSizeX + 1; ++j)
		{
			if(!m_aOcclussionMap[INDEX(j, i)])
			{
				bSkip = false;
				for(UINT k = 0; k < uSizeX && !bSkip; ++k)
				{
					for(UINT s = 0; s < uSizeY && !bSkip; ++s)
					{
						if(m_aOcclussionMap[INDEX(k + j, s + i)])
						{
							bSkip = true;
						}
					}
				}
				if(!bSkip)
				{
					return(POINT{j, i});
				}
			}
		}
	}
	return(POINT{-1, -1});
}

void CGUIInventoryController::updateOcclusionMap(const POINT &pt, UINT uSizeX, UINT uSizeY)
{
	for(UINT i = 0; i < uSizeX; ++i)
	{
		for(UINT j = 0; j < uSizeY; ++j)
		{
			assert(!m_aOcclussionMap[INDEX(i + pt.x, j + pt.y)]);
			m_aOcclussionMap[INDEX(i + pt.x, j + pt.y)] = true;
		}
	}
#undef INDEX
}

//FIXME: убрать с глаз долой 
static bool IsPointInRect(const RECT &rect, int iX, int iY)
{
	if(rect.bottom >= iY && rect.top <= iY && rect.right >= iX && rect.left <= iX)
	{
		return(true);
	}
	return(false);
}

static bool IsRectsIsIntersecting(const RECT &a, const RECT &b)
{
	if(a.top >= b.bottom || a.bottom <= b.top)
	{
		return(false);
	}

	if(a.right <= b.left || a.left >= b.right)
	{
		return(false);
	}
	return(true);
}

void CGUIInventoryController::endDrag(gui::IEvent *ev)
{
	//@TODO: сделать свап одинаковых по размеру предметов
	//@TODO: refactor
	if(!m_pDragNode)
	{
		return;
	}

	bool bIsDrop = false;

	gui::dom::IDOMnode *pCurrentContainer = NULL;

	gui::dom::IDOMnode *pEquipArea = m_pInventoryDesktop->getDocument()->getElementById(L"equip_area"); 


	RECT frameRect = m_pFrameNode->getClientRect();
	RECT euqipAreaRect = pEquipArea->getClientRect();

	gui::css::ICSSstyle *pStyle = m_pDragNode->getStyleSelf();
	gui::dom::IDOMnode *pCell = NULL;

	const gui::dom::IDOMnodeCollection *pList = m_pInventoryDesktop->getDocument()->getElementsByClass(L"cell");
	fora(i, *pList)
	{
		RECT rect = pList[0][i]->getClientRect();
		if(IsPointInRect(rect, ev->clientX, ev->clientY))
		{
			pCell = pList[0][i];
			break;
		}
	}

	CBaseItem *pItem = m_pInventory->getSlot(m_pDragNode->getAttribute(L"inventoryid").toInt());

	if(IsPointInRect(frameRect, ev->clientX, ev->clientY))
	{
		pCurrentContainer = m_pItemContainerNode;
		m_pInventory->deequipItem(pItem);

		if(pCell)
		{
			int idx = pCell->parentNode()->getChilds()->indexOf(pCell);
			int idy = pCell->parentNode()->parentNode()->getChilds()->indexOf(pCell->parentNode());
			
			if(idx + pItem->getIconSizeX() > m_uCells || idy + pItem->getIconSizeY() > m_uRows)
			{
				pCell = NULL;
			}
		}
	}
	else if(IsPointInRect(euqipAreaRect, ev->clientX, ev->clientY))
	{
		pCurrentContainer = m_pEquipContainerNode;
		gui::dom::IDOMnode *pParentNode = NULL;

		if(pCell)
		{
			pParentNode = pCell->parentNode()->parentNode();
			const StringW &wsItemClass = pParentNode->getAttribute(L"item_class");
			//CBaseItem *pItem = m_pInventory->getSlot(m_pDragNode->getAttribute(L"inventoryid").toInt());

			if(wsItemClass.length() != 0 && !CEntityFactoryMap::IsEntityOfClass(pItem, CWC2MB(wsItemClass.c_str())))
			{
				pCell = NULL;
			}
			else if(!pParentNode->getAttribute(L"split_container").toBool())
			{
				pCell = pParentNode->getChilds()[0][0]->getChilds()[0][0];
			}
		}
		if(pCell)
		{
			const StringW &wsEquipType = pParentNode->getAttribute(L"equip_type");
			auto &enumReflector = EnumReflector::Get<EQUIP_ITEM_TYPE>();
			auto result = enumReflector.find(CWC2MB(wsEquipType.c_str()));

			if(result.isValid())
			{ 
				UINT uIndex = 0;
				if(pParentNode->getAttribute(L"split_container").toBool())
				{
					uIndex = pCell->getAttribute(L"equip_index").toInt();
				}
				else
				{
					uIndex = pParentNode->getAttribute(L"equip_index").toInt();
				}
				m_pInventory->equipItem(pItem, (EQUIP_ITEM_TYPE)result.getValue(), uIndex);
			}
			else
			{
				pCell = NULL;
			}
		}
	}
	else
	{
		pCell = NULL;
		bIsDrop = true;
	}

	if(pCell)
	{
		RECT dragRect = m_pDragNode->getClientRect();
		RECT cellRect = pCell->getClientRect();

		dragRect.right = dragRect.right - dragRect.left + cellRect.left;
		dragRect.left = cellRect.left;
		dragRect.bottom = dragRect.bottom - dragRect.top + cellRect.top;
		dragRect.top = cellRect.top;

		const gui::dom::IDOMnodeCollection *pList = pCurrentContainer->getChilds();
		fora(i, *pList)
		{
			RECT rect = pList[0][i]->getClientRect();
			if(IsRectsIsIntersecting(rect, dragRect))
			{
				pCell = NULL;
				break;
			}
		}
	}

	if(!pCell)
	{
		pCurrentContainer = m_pOriginalContainerNode;
	}

	RECT parentRect = pCurrentContainer->getClientRect();

	if(pCell)
	{
		pStyle->top->set(pCell->getClientRect().top - parentRect.top);	
		pStyle->left->set(pCell->getClientRect().left - parentRect.left);
	}
	else
	{
		pStyle->top->set(m_iStartDragY - parentRect.top);
		pStyle->left->set(m_iStartDragX - parentRect.left);
	}

	pStyle->top->setDim(gui::css::ICSSproperty::DIM_PX);
	pStyle->left->setDim(gui::css::ICSSproperty::DIM_PX);

	m_pDragNode->parentNode()->takeChild(m_pDragNode);
	pCurrentContainer->appendChild(m_pDragNode);

	if(bIsDrop)
	{
		//ev->preventDefault = false;
		//ev->stopPropagation();

		//pCurrentContainer->removeChild(m_pDragNode);
		//pCurrentContainer->updateStyles(true);
		m_pInventory->getOwner()->dropItem(pItem);
	}

	m_pOriginalContainerNode = NULL;
	m_pDragNode = NULL;
}

void CGUIInventoryController::dropItem(gui::IEvent *ev)
{
	assert(m_pSelectedNode);

	int idx = m_pSelectedNode->getAttribute(L"inventoryid").toInt();
	CBaseItem *pItem = m_pInventory->getSlot(idx);
	m_pInventory->getOwner()->dropItem(pItem);
	update();

}