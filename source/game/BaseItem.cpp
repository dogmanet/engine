
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "BaseItem.h"
#include "BaseCharacter.h"
#include "TriggerItemUse.h"
#include <xcommon/resource/IXResourceManager.h>

/*! \skydocent base_item
Базовый объект элемента инвентаря игрока
*/

BEGIN_PROPTABLE(CBaseItem)
	//! Иконка в инвентаре (только имя файла, без расширения и пути)
	DEFINE_FIELD_STRING(m_szInvIcon, PDFF_NOEDIT | PDFF_NOEXPORT, "inv_icon", "", EDITOR_NONE)
	//! Имя в инвентаре
	DEFINE_FIELD_STRING(m_szInvName, PDFF_NOEDIT | PDFF_NOEXPORT, "inv_name", "", EDITOR_NONE)
	//! Может ли стакаться
	DEFINE_FIELD_BOOL(m_bInvStackable, PDFF_NOEDIT | PDFF_NOEXPORT, "inv_stackable", "", EDITOR_NONE)
	//! Текущий размер стака
	DEFINE_FIELD_INT(m_iInvStackCurSize, PDFF_NOEDIT, "inv_stack_cur", "", EDITOR_NONE)
	//! Максимальное число объектов в стаке
	DEFINE_FIELD_INT(m_iInvStackMaxSize, PDFF_NOEDIT | PDFF_NOEXPORT, "inv_stack_max", "", EDITOR_NONE)
	//! Масса одного объекта, кг
	DEFINE_FIELD_FLOAT(m_iInvWeight, PDFF_NOEDIT | PDFF_NOEXPORT, "inv_weight", "", EDITOR_NONE)
	//! Можно ли поднимать объект
	DEFINE_FIELD_BOOLFN(m_bPickable, PDFF_NOEDIT | PDFF_NOEXPORT, "inv_pickable", "", onIsPickableChanged, EDITOR_NONE)
	//! Размер иконки по X
	DEFINE_FIELD_INT(m_iIconSizeX, PDFF_NOEDIT | PDFF_NOEXPORT, "inv_icon_size_x", "", EDITOR_NONE)
	//! Размер иконки по Y
	DEFINE_FIELD_INT(m_iIconSizeY, PDFF_NOEDIT | PDFF_NOEXPORT, "inv_icon_size_y", "", EDITOR_NONE)

	DEFINE_OUTPUT(m_onPickUp, "OnPickUp", "On pickup")
	DEFINE_OUTPUT(m_onDrop, "OnDrop", "On drop")

	DEFINE_FIELD_STRINGFN(m_szViewModelFile, 0, "model_view", "View model file", onSetViewModel, EDITOR_MODEL)
END_PROPTABLE()

REGISTER_ENTITY_NOLISTING(CBaseItem, base_item);

void CBaseItem::onPostLoad()
{
	BaseClass::onPostLoad();

	if(m_bPickable && !m_pTriggerUse)
	{
		m_pTriggerUse = (CTriggerItemUse *)CREATE_ENTITY("trigger_itemuse", m_pMgr);
		m_pTriggerUse->setItem(this);
		m_pTriggerUse->setModel("meshes/dev/item_trigger.dse");
		m_pTriggerUse->setPos(getPos());
	}
}

CBaseItem::~CBaseItem()
{
	mem_release(m_pViewModel);
	mem_release(m_pViewModelResource);
	mem_release(m_pHandsModelResource);
	if(m_pTriggerUse)
	{
		REMOVE_ENTITY(m_pTriggerUse);
	}
}

float CBaseItem::getWeight()
{
	return(m_iInvWeight);
}

void CBaseItem::onUse(CBaseEntity *pUser)
{
	BaseClass::onUse(pUser);
	if(!m_bPickable)
	{
		return;
	}

	CBaseCharacter *pCharacter = (CBaseCharacter*)pUser;
	pCharacter->getInventory()->putItem(this);
	FIRE_OUTPUT(m_onPickUp, pUser);

	if(m_bInvStackable)
	{
		REMOVE_ENTITY(this);
	}
	else
	{
		setMode(IIM_INVENTORY);
	}
}

void CBaseItem::setMode(INVENTORY_ITEM_MODE mode)
{
	if(mode == m_inventoryMode)
	{
		return;
	}
	INVENTORY_ITEM_MODE oldMode = m_inventoryMode;
	m_inventoryMode = mode;
	onModeChanged(oldMode, mode);
}

INVENTORY_ITEM_MODE CBaseItem::getMode()
{
	return(m_inventoryMode);
}

void CBaseItem::onModeChanged(INVENTORY_ITEM_MODE oldMode, INVENTORY_ITEM_MODE newMode)
{
	if(m_pModel)
	{
		m_pModel->enable(newMode == IIM_WORLD);
	}
	if(m_pViewModel)
	{
		m_pViewModel->enable(newMode == IIM_IN_HANDS);
		if(newMode == IIM_IN_HANDS)
		{
			m_pViewModel->startActivity("ACT_HOLSTER");
		}
	}

	if(newMode == IIM_WORLD)
	{
		initPhysics();
		if(m_bPickable && !m_pTriggerUse)
		{
			m_pTriggerUse = (CTriggerItemUse*)CREATE_ENTITY("trigger_itemuse", m_pMgr);
			m_pTriggerUse->setItem(this);
			m_pTriggerUse->setModel("meshes/dev/item_trigger.dse");
			m_pTriggerUse->setPos(getPos());
		}
	}
	else
	{
		releasePhysics();
		if(m_pTriggerUse)
		{
			REMOVE_ENTITY(m_pTriggerUse);
			m_pTriggerUse = NULL;
		}
	}
}

void CBaseItem::setScale(float fScale)
{
	BaseClass::setScale(fScale);

	if(m_pViewModel)
	{
		m_pViewModel->setScale(fScale);
	}
}

void CBaseItem::onSetViewModel(const char *mdl)
{
	_setStrVal(&m_szViewModelFile, mdl);
	mem_release(m_pViewModel);
	mem_release(m_pViewModelResource);
	if(!mdl[0])
	{
		return;
	}

	IXResourceManager *pResourceManager = Core_GetIXCore()->getResourceManager();
	if(!pResourceManager->getModelAnimated(mdl, &m_pViewModelResource))
	{
		return;
	}
	
	onModelChanged();
}

void CBaseItem::setHandsResource(IXResourceModelAnimated *pResource)
{
	if(pResource)
	{
		pResource->AddRef();
	}

	mem_release(m_pHandsModelResource);
	m_pHandsModelResource = pResource;

	onModelChanged();
}

void CBaseItem::onModelChanged()
{
	mem_release(m_pViewModel);
	if(!m_pViewModelResource || !m_pHandsModelResource)
	{
		return;
	}

	IXAnimatedModelProvider *pProvider = (IXAnimatedModelProvider*)Core_GetIXCore()->getPluginManager()->getInterface(IXANIMATEDMODELPROVIDER_GUID);
	if(pProvider)
	{
		IXResourceModelAnimated *pAnimatedResources[] = {
			m_pViewModelResource,
			m_pHandsModelResource
		};

		if(pProvider->createModel(2, pAnimatedResources, &m_pViewModel))
		{
			m_pViewModel->play("IDLE");
			m_pViewModel->enable(m_inventoryMode == IIM_IN_HANDS);
			m_pViewModel->setScale(m_fBaseScale);
		}
	}
}

void CBaseItem::setPos(const float3 &pos)
{
	BaseClass::setPos(pos);

	SAFE_CALL(m_pViewModel, setPosition, pos);
	SAFE_CALL(m_pTriggerUse, setPos, pos);
}

void CBaseItem::setOrient(const SMQuaternion &q)
{
	BaseClass::setOrient(q);

	SAFE_CALL(m_pViewModel, setOrientation, q);
}

int CBaseItem::getIconSizeX()
{
	return(m_iIconSizeX);
}

int CBaseItem::getIconSizeY()
{
	return(m_iIconSizeY);
}

int CBaseItem::getStackCount()
{
	return(m_iInvStackCurSize);
}

bool CBaseItem::isStackable()
{
	return(m_bInvStackable);
}

const char* CBaseItem::getItemName()
{
	return(m_szInvName);
}

const char* CBaseItem::getIcon()
{
	return(m_szInvIcon);
}

void CBaseItem::onIsPickableChanged(bool isPickable)
{
	if(isPickable && !m_pTriggerUse)
	{
		m_pTriggerUse = (CTriggerItemUse*)CREATE_ENTITY("trigger_itemuse", m_pMgr);
		m_pTriggerUse->setItem(this);
		m_pTriggerUse->setModel("meshes/dev/item_trigger.dse");
		m_pTriggerUse->setPos(getPos());
	}
	else if(!isPickable && m_pTriggerUse)
	{
		REMOVE_ENTITY(m_pTriggerUse);
		m_pTriggerUse = NULL;
	}
	m_bPickable = isPickable;
}
