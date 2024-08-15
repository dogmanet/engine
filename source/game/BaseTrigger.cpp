
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "BaseTrigger.h"

/*! \skydocent trigger
Базовый класс триггера. Триггер - область, задаваемая моделью, при входе/выходе из которой игрока или объектов, вызывает заданные события
*/

BEGIN_PROPTABLE(CBaseTrigger)
	//! Срабатывает при начале контакта
	DEFINE_OUTPUT(m_onTouchStart, "OnTouchStart", "On touch start")
	//! Срабатывает при прекращении контакта
	DEFINE_OUTPUT(m_onTouchEnd, "OnTouchEnd", "On touch end")
	//! Срабатывает, когда не осталось контактирующих объектов
	DEFINE_OUTPUT(m_onTouchEndAll, "OnTouchEndAll", "On touch end all")

	//! Включает триггер
	DEFINE_INPUT(inEnable, "enable", "Enable", PDF_NONE)
	//! Выключает триггер
	DEFINE_INPUT(inDisable, "disable", "Disable", PDF_NONE)
	//! Переключает состояние триггера
	DEFINE_INPUT(inToggle, "toggle", "Toggle", PDF_NONE)
END_PROPTABLE()

REGISTER_ENTITY(CBaseTrigger, trigger, REC_MODEL_FIELD("model"));

IEventChannel<XEventPhysicsStep> *CBaseTrigger::m_pTickEventChannel = NULL;

CBaseTrigger::CBaseTrigger():
	m_physicsTicker(this)
{
	if(!m_pTickEventChannel)
	{
		m_pTickEventChannel = Core_GetIXCore()->getEventChannel<XEventPhysicsStep>(EVENT_PHYSICS_STEP_GUID);
	}
}

CBaseTrigger::~CBaseTrigger()
{
	removePhysBody();
}

void CBaseTrigger::onPostLoad()
{
	BaseClass::onPostLoad();

	if(m_pModel)
	{
		//m_pModel->enable(false);
		//m_pAnimPlayer->setOverrideMaterial("dev_trigger.dds");
	}
}

void CBaseTrigger::enable()
{
	if(!m_bEnabled)
	{
		m_bEnabled = true;
		if(m_pGhostObject)
		{
			GetPhysWorld()->addCollisionObject(m_pGhostObject, CG_TRIGGER, CG_CHARACTER);
			m_pTickEventChannel->addListener(&m_physicsTicker);
		}
	}
}
void CBaseTrigger::disable()
{
	if(m_bEnabled)
	{
		m_bEnabled = false;
		if(m_pGhostObject)
		{
			m_pTickEventChannel->removeListener(&m_physicsTicker);
			GetPhysWorld()->removeCollisionObject(m_pGhostObject);
		}
	}
}
void CBaseTrigger::toggle()
{
	if(m_bEnabled)
	{
		disable();
	}
	else
	{
		enable();
	}
}

void CBaseTrigger::inEnable(inputdata_t *pInputdata)
{
	enable();
}
void CBaseTrigger::inDisable(inputdata_t *pInputdata)
{
	disable();
}
void CBaseTrigger::inToggle(inputdata_t *pInputdata)
{
	toggle();
}

void CBaseTrigger::createPhysBody()
{
	if(m_pCollideShape)
	{
		float3 vPos = getPos();
		SMQuaternion qRot = getOrient();

		GetPhysics()->newGhostObject(&m_pGhostObject);
		m_pGhostObject->setPosition(vPos);
		m_pGhostObject->setRotation(qRot);
		m_pGhostObject->setUserPointer(this);
		m_pGhostObject->setUserTypeId(1);
		m_pGhostObject->setCollisionShape(m_pCollideShape);
		m_pGhostObject->setCollisionFlags(m_pGhostObject->getCollisionFlags() ^ XCF_NO_CONTACT_RESPONSE);

		GetPhysWorld()->addCollisionObject(m_pGhostObject, CG_TRIGGER, CG_CHARACTER);
		m_pTickEventChannel->addListener(&m_physicsTicker);
		m_bEnabled = true;
	}
}

void CBaseTrigger::removePhysBody()
{
	if(m_pGhostObject && m_bEnabled)
	{
		GetPhysWorld()->removeCollisionObject(m_pGhostObject);
		m_pTickEventChannel->removeListener(&m_physicsTicker);
	}
	mem_release(m_pGhostObject);
}

void CBaseTrigger::onTouchStart(CBaseEntity *pActivator)
{
	FIRE_OUTPUT(m_onTouchStart, pActivator);
}
void CBaseTrigger::onTouchEnd(CBaseEntity *pActivator)
{
	FIRE_OUTPUT(m_onTouchEnd, pActivator);
}
void CBaseTrigger::onTouchEndAll(CBaseEntity *pActivator)
{
	FIRE_OUTPUT(m_onTouchEndAll, pActivator);
}

void CBaseTrigger::onPhysicsStep()
{
	{
		// TODO move this!
		static const bool *dev_show_triggers = GET_PCVAR_BOOL("dev_show_triggers");

		if(m_pModel && *dev_show_triggers != m_isModelEnabled)
		{
			m_isModelEnabled = *dev_show_triggers;
			m_pModel->enable(m_isModelEnabled);
		}
	}

	if(!m_pGhostObject || !m_bEnabled)
	{
		return;
	}
	m_aNewTouches.clearFast();

	for(UINT i = 0, l = m_pGhostObject->getOverlappingPairCount(); i < l; ++i)
	{
		IXCollisionPair *pair = m_pGhostObject->getOverlappingPair(i);
				
		for(UINT j = 0, jl = pair->getContactManifoldCount(); j < jl; ++j)
		{
			IXContactManifold *pManifold = pair->getContactManifold(j);
			if(pManifold->getContactCount() > 0)
			{
				IXCollisionObject *p0 = pair->getObject0();
				IXCollisionObject *p1 = pair->getObject1();

				const IXCollisionObject *pObject = p0 == m_pGhostObject ? p1 : p0;

				if(pObject->getUserPointer() && pObject->getUserTypeId() == 1)
				{
					CBaseEntity *pEnt = (CBaseEntity*)pObject->getUserPointer();
					if(pEnt)
					{
						m_aNewTouches.push_back(pEnt);
						//printf("touched %s\n", pEnt->getClassName());
					}
				}
				break;
			}
		}
	}
	//m_pGhostObject->getOverlappingObject(0);

	update();
}

void CBaseTrigger::update()
{
	bool bFound;
	for(int i = 0, l = m_aNewTouches.size(); i < l; ++i)
	{
		bFound = false;
		for(int j = 0, jl = m_aTouches.size(); j < jl; ++j)
		{
			if(m_aNewTouches[i] == m_aTouches[j])
			{
				m_aTouches[j] = m_aTouches[jl - 1];
				m_aTouches.erase(jl - 1);
				bFound = true;
				break;
			}
		}
		if(!bFound)
		{
			onTouchStart(m_aNewTouches[i]);
		}
	}
	for(int j = 0, jl = m_aTouches.size(); j < jl; ++j)
	{
		onTouchEnd(m_aTouches[j]);
	}
	if(m_aTouches.size() && !m_aNewTouches.size())
	{
		onTouchEndAll(m_aTouches[0]);
	}
	m_aTouches.swap(m_aNewTouches);
}


void CBaseTrigger::setPos(const float3 &pos)
{
	BaseClass::setPos(pos);

	SAFE_CALL(m_pGhostObject, setPosition, pos);
}

void CBaseTrigger::setOrient(const SMQuaternion &q)
{
	BaseClass::setOrient(q);

	SAFE_CALL(m_pGhostObject, setRotation, q);
}

void CPhysicsTickEventListener::onEvent(const XEventPhysicsStep *pData)
{
	m_pTrigger->onPhysicsStep();
}
