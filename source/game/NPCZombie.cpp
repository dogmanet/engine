
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "NPCZombie.h"

#include "GameData.h"
#include "BaseTool.h"


/*! \skydocent npc_zombie
NPC зомби
*/

BEGIN_PROPTABLE(CNPCZombie)
	DEFINE_MESSAGE(msgKnowEnemyHere, "knowEnemyHere", "Enemy Here", PDF_NONE)
	DEFINE_MESSAGE(msgFiringHere, "firingHere", "Firing Here", PDF_VECTOR)
END_PROPTABLE()

REGISTER_ENTITY_NOLISTING(CNPCZombie, npc_zombie);

CNPCZombie::CNPCZombie():
	m_stateDanger(NPC_STATE_DANGER_CALM),
	m_iObserveRotateStep(0),
	m_idRotateInterval(-1),
	m_fRotationAngle(0.0f),
	m_isGoingToDangerPos(false),
	m_fGoingToDangerPosTime(0.0f)
{
	//m_fSpeedWalk = 0.07f;
	//m_fSpeedRun = 0.12f;
	IXSoundSystem *pSound = (IXSoundSystem*)(Core_GetIXCore()->getPluginManager()->getInterface(IXSOUNDSYSTEM_GUID));
	if(pSound)
	{
		IXSoundLayer *pGameLayer = pSound->findLayer("xGame");
		m_pSndIdle = pGameLayer->newSoundPlayer("mobs/zombie/zombie_idle_16.ogg", SOUND_SPACE_3D);
		m_pSndIdle2 = pGameLayer->newSoundPlayer("mobs/zombie/zombie_idle_17.ogg", SOUND_SPACE_3D);
		m_pSndDeath = pGameLayer->newSoundPlayer("mobs/zombie/zombie_die_1.ogg", SOUND_SPACE_3D);
	}

}

CNPCZombie::~CNPCZombie()
{
	if(m_pActiveTool)
	{
		REMOVE_ENTITY(m_pActiveTool);
	}

	mem_release(m_pSndIdle);
	mem_release(m_pSndIdle2);
	mem_release(m_pSndDeath);
}

void CNPCZombie::onPostLoad()
{
	BaseClass::onPostLoad();

	m_pActiveTool = (CBaseTool*)CREATE_ENTITY("wpn_zombie_hands", m_pMgr);
	m_pActiveTool->setOwner(this);
	m_pActiveTool->setPos(getPos() + float3(1.0f, m_fCapsHeight - 0.1f, 1.0f));
	m_pActiveTool->setMode(IIM_IN_HANDS);
	m_pActiveTool->setOrient(m_pHeadEnt->getOrient());
	m_pActiveTool->setParent(m_pHeadEnt);

	SET_TIMEOUT(think, randf(1.0f, 3.0f));
}

void CNPCZombie::onDeath(CBaseEntity *pAttacker, CBaseEntity *pInflictor)
{
	SAFE_CALL(m_pSndDeath, play);

	BaseClass::onDeath(pAttacker, pInflictor);

	SET_TIMEOUT(removeThis, randf(10.0f, 30.0f));

	SAFE_CALL(m_pActiveTool, stopAction);
}

void CNPCZombie::setPos(const float3 &pos)
{
	SAFE_CALL(m_pSndIdle, setWorldPos, pos);
	SAFE_CALL(m_pSndIdle2, setWorldPos, pos);
	SAFE_CALL(m_pSndDeath, setWorldPos, pos);
}

void CNPCZombie::rotateThink(float dt)
{
	if(m_stateDanger != NPC_STATE_DANGER_UNVISIBLE)
	{
		CLEAR_INTERVAL(m_idRotateInterval);
		m_idRotateInterval = -1;
		m_fRotationAngle = 0.0f;
		m_iObserveRotateStep = 0;
		return;
	}
	m_fRotationAngle += dt;
	setOrient(SMQuaternion(dt, 'y') * getOrient());

	if(m_fRotationAngle > SM_PIDIV2)
	{
		playActivity("ACT_IDLE", 200);
		CLEAR_INTERVAL(m_idRotateInterval);
		m_idRotateInterval = -1;
		m_fRotationAngle = 0.0f;

		if(m_iObserveRotateStep < 4)
		{
			SET_TIMEOUT(startRotateThink, randf(1.5f, 3.0f));
		}
		else
		{
			m_stateDanger = NPC_STATE_DANGER_CALM;
			m_iObserveRotateStep = 0;
		}
	}
}

void CNPCZombie::startRotateThink(float fDelta)
{
	if(m_stateDanger == NPC_STATE_DANGER_UNVISIBLE)
	{
		++m_iObserveRotateStep;
		playAnimation("turn_left", 200);
		m_idRotateInterval = SET_INTERVAL(rotateThink, 0);
	}
}

void CNPCZombie::think(float fDelta)
{
	if(m_fHealth <= 0.f)
		return;

	CBaseCharacter *pTargetEnemy = GameData::m_pPlayer;
	
	float fNextThink = randf(1.0f, 3.0f);

	float fPlayerVisCoeff = canSee(pTargetEnemy);
	float fPlayerDist = SMVector3Length(pTargetEnemy->getPos() - getPos());

	//printf("  %.2f \r", fPlayerVisCoeff);

	if(m_stateDanger == NPC_STATE_DANGER_CALM)
	{
		//printf(COLOR_LGREEN "NPC_STATE_DANGER_CALM" COLOR_RESET "\n");
		// Игрок виден
		if(fPlayerVisCoeff > 0.0f)
		{
			m_stateDanger = NPC_STATE_DANGER_VISIBLE;
			m_vLastDangerPos = pTargetEnemy->getPos();
		}
		else
		{
			if(!m_isGoing && randf(0.0f, 1.0f) > 0.5f)
			{
				randWalk();
			}
		}
		
	}


	if(m_stateDanger == NPC_STATE_DANGER_VISIBLE)
	{
		/*
			НПС обладающие непосредственным состоянием опасности, способны случайным 
			образом заражать посредственным состоянием опасности других (подобных) 
			НПС в радиусе 5 метров в течении 2-3 секунд. Процент случайности 50%.
		*/
		//printf(COLOR_LRED "NPC_STATE_DANGER_VISIBLE" COLOR_RESET "\n");
		broadcastMessage("knowEnemyHere", 5.0f);

		if(!m_isGoing)
		{
			m_isGoingToDangerPos = false;
		}

		bool isTargetMoved = SMVector3Length2(m_vGoingToDangerPos - m_vLastDangerPos) > 0.5f;

		if(m_isGoingToDangerPos)
		{
			m_fGoingToDangerPosTime += fDelta;
		}

		if((!m_isGoingToDangerPos && (isTargetMoved || fPlayerDist >= 2.0f)) || m_isPathInterrupted)
		{
			//printf(COLOR_LRED "goTo(m_vLastDangerPos);" COLOR_RESET "\n");
			goTo(m_vLastDangerPos, true);
			m_vGoingToDangerPos = m_vLastDangerPos;
			m_isGoingToDangerPos = true;
			m_fGoingToDangerPosTime = 0.0f;
		}

		if(fPlayerDist < 2.0f)
		{
			if(m_isGoingToDangerPos)
			{
				stopMotion();
				m_isGoingToDangerPos = false;
			}
			//printf(COLOR_LRED "orientAtPoint(pTargetEnemy->getPos());" COLOR_RESET "\n");
			// orient if necessary
			orientAtPoint(pTargetEnemy->getPos());
			attack(TRUE);
			attack(FALSE);
		}

		if(fPlayerVisCoeff > 0.0f)
		{
			m_vLastDangerPos = pTargetEnemy->getPos();
			m_fGoingToDangerPosTime = 0.0f;
		}
		else
		/*
			После потери из виду источника угрозы, НПС которые имели непосредственнее 
			состояние опасности, следуют в позицию(с разбросом до 5 метров) где последний 
			раз видели источник угрозы, поворачиваются вокруг на 360 градусов(4 раза по 
			90 градусов с задержкой[1500, 3000] миллисекунд), если не находят источник угрозы, 
			выходят из состояния опасности.Если НПС не смог прийти в последнюю позиции источника 
			угрозы, то он выходит из состояния опасности в течении 5 секунд.
		*/
		{
			if(!m_isGoingToDangerPos)
			{
				m_fGoingToDangerPosTime += fDelta;
			}
			if(!m_isGoingToDangerPos || m_fGoingToDangerPosTime > 5.0f)
			{
				m_stateDanger = NPC_STATE_DANGER_UNVISIBLE;
			}
		}

		fNextThink = randf(0.1f, 0.2f);
	}

	/*
		В случае не обнаружения источника угрозы, НПС имеющие посредственное 
		состояние опасности, выходят из него после получения тогда, когда 
		сделают постепенный поворот в 360 градусов, то есть 4 раза по 90 
		градусов с задержками [1500, 3000] миллисекунд.
	*/
	if(m_stateDanger == NPC_STATE_DANGER_UNVISIBLE)
	{
		//printf(COLOR_YELLOW "NPC_STATE_DANGER_UNVISIBLE" COLOR_RESET "\n");

		if(fPlayerVisCoeff > 0.0f)
		{
			m_stateDanger = NPC_STATE_DANGER_VISIBLE;
			m_vLastDangerPos = pTargetEnemy->getPos();
		}
		else if(m_iObserveRotateStep == 0)
		{
			if(m_isGoing)
			{
				stopMotion();
			}
			else
			{
				playActivity("ACT_IDLE", 300);
			}
			startRotateThink(0.0f);
		}

		fNextThink = randf(0.01f, 0.1f);
	}
		
	SET_TIMEOUT(think, fNextThink);
}

void CNPCZombie::msgKnowEnemyHere(inputdata_t *pInputdata)
{
	if(m_stateDanger == NPC_STATE_DANGER_CALM && randf(0.0f, 1.0f) > 0.5f)
	{
		//printf(COLOR_YELLOW "Someone found enemy!" COLOR_RESET "\n");
		m_stateDanger = NPC_STATE_DANGER_UNVISIBLE;
	}
}

void CNPCZombie::msgFiringHere(inputdata_t *pInputdata)
{
	if(m_stateDanger == NPC_STATE_DANGER_CALM && randf(0.0f, 1.0f) > 0.5f)
	{
		printf(COLOR_YELLOW "Enemy shooter near!" COLOR_RESET "\n");
		m_stateDanger = NPC_STATE_DANGER_UNVISIBLE;
		m_vLastDangerPos = pInputdata->pActivator->getPos();
	}
	else if(m_stateDanger == NPC_STATE_DANGER_UNVISIBLE && randf(0.0f, 1.0f) > 0.5f)
	{
		printf(COLOR_YELLOW "Found enemy shooter!" COLOR_RESET "\n");
		m_stateDanger = NPC_STATE_DANGER_VISIBLE;
		m_vLastDangerPos = pInputdata->pActivator->getPos();
	}
}


void CNPCZombie::randWalk()
{
	/*if(!ID_VALID(m_idCurrQuaidInPath))
	{*/
		float rndradius = randf(20.f, 40.f)*0.5f;
		float3 rndpos;

		float3 vPos = getPos();

		rndpos.x = vPos.x + randf(-rndradius, rndradius);
		rndpos.y = vPos.y + randf(-rndradius, rndradius);
		rndpos.z = vPos.z + randf(-rndradius, rndradius);

		//Core_RFloat3Get(G_RI_FLOAT3_OBSERVER_POSITION, &rndpos);

		goTo(SAIG_QuadGetNear(&rndpos, true));

		/*if(pathFind(SAIG_QuadGetNear(&rndpos, true)))
		{
			m_idCurrQuaidInPath = 0;
			m_vLastPathPos = getPos();
		}*/

		//return;
	/*}*/

	/*if(m_stateMove == NPC_STATE_MOVE_IDLE_START && !playingAnimations("idle0"))
	{
		m_stateMove = NPC_STATE_MOVE_IDLE;
		playAnimation("idle0");
		SSCore_SndPlay(m_idSndIdle);
	}
	else if(m_stateMove == NPC_STATE_MOVE_IDLE/ * && !playingAnimations("idle0")* /)
		m_stateMove = NPC_STATE_MOVE_WALK;
	else if(m_stateMove != NPC_STATE_MOVE_IDLE)
	{
		if(!playingAnimations("walk0"))
		{
			playAnimation("walk0");
			SSCore_SndPlay(m_idSndIdle2);
		}

		pathWalk();
	}*/
}

void CNPCZombie::dispatchDamage(CTakeDamageInfo &takeDamageInfo)
{
	m_stateDanger = NPC_STATE_DANGER_VISIBLE;
	m_vLastDangerPos = takeDamageInfo.m_pInflictor->getPos();

	BaseClass::dispatchDamage(takeDamageInfo);
}

void CNPCZombie::removeThis(float fDelta)
{
	REMOVE_ENTITY(this);
}
