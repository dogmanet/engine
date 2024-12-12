
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "NPCBase.h"
#include "Tracer.h"

/*! \skydocent npc_base
Базовый класс для NPC
*/

BEGIN_PROPTABLE(CNPCBase)
	//! Угол конуса взгляда, рад
	DEFINE_FIELD_FLOAT(m_fViewConeAngle, PDFF_NOEDIT | PDFF_NOEXPORT, "view_angle", "", EDITOR_NONE)
	//! Дальность видимости, м
	DEFINE_FIELD_FLOAT(m_fViewDistance, PDFF_NOEDIT | PDFF_NOEXPORT, "view_distance", "", EDITOR_NONE)
END_PROPTABLE()

REGISTER_ENTITY_NOLISTING(CNPCBase, npc_base);

CNPCBase::CNPCBase():
	m_idQuadGoingTo(-1),
	m_idFindPathInterval(-1),
	m_idPathFollowInterval(-1),
	m_fSpeedWalk(0.36f), // подогнанно под анимацию
	m_fSpeedRun(0.9f),
	m_iStuckCount(0),
	m_isGoing(false),
	m_fViewConeAngle(SMToRadian(60.0f)),
	m_fViewDistance(50.0f),
	m_isPathInterrupted(false),
	m_idOrientAtInterval(-1),
	m_bRunMode(false)
{
	m_iCurrQuaidInPath = -1;
	// m_fBaseScale = 0.01f;
	m_idQuadCurr = -1;
	m_statePath = NPC_STATE_PATH_NOTFOUND;

	m_idQueueFindPath = -1;

	/*m_fAngleYLast = 0;
	m_fAngleYNext = 0;
	m_ulTimeAllRot = 0;
	m_ulTimeRot = 0;
	m_canJump = false;*/
	m_fStepHeight = 0.4f;
	m_ulColor = GX_COLOR_ARGB(128, rand() % 255, rand() % 255, rand() % 255);
	//m_ulTimeReturnInGrid = m_ulTimeAllReturnInGrid = 0;

	m_uMoveDir &= ~PM_OBSERVER;

	//m_flashlight->setEnable(true);
}

CNPCBase::~CNPCBase()
{

}

void CNPCBase::setModel(const char * mdl)
{
	BaseClass::setModel(mdl);

	if(m_pModel && m_pHeadEnt && m_pModel->asAnimatedModel())
	{
		ID idBone = m_pModel->asAnimatedModel()->getBoneId("bip01_ponytail1");

		m_pHeadEnt->setParent(this, idBone);

		m_pHeadEnt->setOffsetOrient(SMQuaternion());
		m_pHeadEnt->setOffsetPos(float3());
	}
}

void CNPCBase::onDeath(CBaseEntity *pAttacker, CBaseEntity *pInflictor)
{
	stopMotion(false);
	playActivity("ACT_DEATH", 300);
	BaseClass::onDeath(pAttacker, pInflictor);
}

void CNPCBase::setPos(const float3 &pos)
{
	float3 tpos = pos;
	m_idQuadCurr = SAIG_QuadGet(&tpos, true);

#if 0
	if(ID_VALID(m_idQuadCurr))
	{
		if(SAIG_QuadGetState(m_idQuadCurr) == AIQUAD_STATE_FREE)
		{
			SAIG_QuadSetState(m_idQuadCurr, AIQUAD_STATE_BUSY);
			SAIG_QuadSetStateWho(m_idQuadCurr, getId());
		}
		else if(SAIG_QuadGetStateWho(m_idQuadCurr) != getId())
		{
			ID idquad = SAIG_QuadGetNear(&pos, 2.0f, true, 2);
			if(ID_VALID(idquad))
			{
				m_idQuadCurr = idquad;
				SAIG_QuadSetState(m_idQuadCurr, AIQUAD_STATE_BUSY);
				SAIG_QuadSetStateWho(m_idQuadCurr, getId());
				SAIG_QuadGetPos(m_idQuadCurr, &tpos);
				tpos.y += 0.7f;
				setPos(tpos);
				return;
			}
		}
	}
#endif
	if(ID_VALID(m_idQuadGoingTo))
	{
		m_idQuadGoingTo = -1;
		goTo(m_idQuadGoingTo);
	}

	m_vLastFramePos = tpos;

	BaseClass::setPos(tpos);
}

ID CNPCBase::getAIQuad()
{
	return m_idQuadCurr;
}

void CNPCBase::goTo(ID idQuad, bool bRun)
	{
	m_bRunMode = bRun;

	stopOrientAt();

	if(!ID_VALID(idQuad))
		{
		stopMotion();
		m_idQuadGoingTo = -1;
		m_statePath = NPC_STATE_PATH_NOTFOUND;

		if(ID_VALID(m_idFindPathInterval))
		{
			CLEAR_INTERVAL(m_idFindPathInterval);
			m_idFindPathInterval = -1;
		}
		return;
	}

	// если запрос идти туда, куда уже идем, то все хорошо
	if(m_idQuadGoingTo == idQuad && !m_isPathInterrupted)
	{
		return;
	}
	m_isPathInterrupted = false;

	m_idQuadGoingTo = idQuad;

	if(ID_VALID(m_idQueueFindPath))
	{
		m_isGoing = false;
		//@TODO: Cancel find query
		m_idQueueFindPath = -1;
	}

	if(!ID_VALID(m_idQuadLast))
	{
		return;
	}
	m_isGoing = true;

	//m_statePath = NPC_STATE_PATH_NOTFOUND;

	// создаем запрос на поиск пути
	m_idQueueFindPath = SAIG_GridQueryFindPath(m_idQuadLast, m_idQuadGoingTo);

	// функция проверки готовности пути
	if(!ID_VALID(m_idFindPathInterval))
	{
		m_idFindPathInterval = SET_INTERVAL(findPathThinker, 0);
	}
}

void CNPCBase::findPathThinker(float fDelta)
{
	if(!ID_VALID(m_idQueueFindPath))
	{
		CLEAR_INTERVAL(m_idFindPathInterval);
		m_idFindPathInterval = -1;
		return;
	}

	// path found
	if(SAIG_GridGetSizePath(m_idQueueFindPath) >= 0)
	{
		if(m_aPathQuads.size() > 0)
			SAIG_GridSetColorArr(&(m_aPathQuads[0]), 0, m_aPathQuads.size());
		m_statePath = NPC_STATE_PATH_FOUND;
		int iCount = SAIG_GridGetSizePath(m_idQueueFindPath);
		m_aPathQuads.resize(iCount);
		SAIG_GridGetPath(m_idQueueFindPath, &(m_aPathQuads[0]), m_aPathQuads.size(), true);
		SAIG_GridSetColorArr(&(m_aPathQuads[0]), m_ulColor, m_aPathQuads.size());
		m_vLastPathPos = getPos();
		m_idQueueFindPath = -1;

		CLEAR_INTERVAL(m_idFindPathInterval);
		m_idFindPathInterval = -1;

		m_iCurrQuaidInPath = 0;

		m_iStuckCount = 0;

		if(!ID_VALID(m_idPathFollowInterval))
		{
			m_idPathFollowInterval = SET_INTERVAL(pathFollowThinker, 1.0f / 10.0f);
			playActivity(m_bRunMode ? "ACT_RUN" : "ACT_WALK", 000);
		}
		printf("Path found!\n");
	}
}

void CNPCBase::pathFollowThinker(float fDelta)
{
	extern CTracer *g_pTracer2;

	if(m_statePath != NPC_STATE_PATH_FOUND)
	{
		CLEAR_INTERVAL(m_idPathFollowInterval);
		m_idPathFollowInterval = -1;
	}
	float fMoveSpeed = m_bRunMode ? m_fSpeedRun : m_fSpeedWalk;

	float fDirLen = fMoveSpeed * fDelta;
	float fMovDirLen = fDirLen;
	float fDist2 = SMVector3Length2(m_vLastPathPos - getPos()); // Расстояние между актуальным положением NPC и точкой следования
	
	if(m_iCurrQuaidInPath < (int)m_aPathQuads.size())
	{
		float3 vEnd;
		SAIG_QuadGetPos(m_aPathQuads[m_iCurrQuaidInPath], &vEnd);

		//m_fSpeedWalk
		// lerp from m_iCurrQuaidInPath to m_iNextQuaidInPath

		bool is2Speed = fDist2 < 1.0f * 1.0f;

		if(is2Speed)
		{
			fDirLen *= 5.0f;
		}
		float fMaxLen = SMVector3Length(vEnd - m_vLastPathPos);

		if(fDirLen > fMaxLen)
		{
			fDirLen = fMaxLen;

			++m_iCurrQuaidInPath;
		}

		float3 vDir = SMVector3Normalize(vEnd - m_vLastPathPos) * fDirLen;

		g_pTracer2->begin(m_vLastPathPos);

		m_vLastPathPos = (float3)(m_vLastPathPos + vDir);

		g_pTracer2->lineTo(m_vLastPathPos);
		g_pTracer2->end();
	}

	//########################################

	//m_vPosition = m_vLastPathPos;
	//m_pGhostObject->getWorldTransform().setOrigin(F3_BTVEC(m_vLastPathPos));

	float3 vDir = m_vLastPathPos - getPos();
	float fDist = SMVector3Length(vDir);
	vDir /= fDist;

	if(fMovDirLen > fDist)
	{
		fMovDirLen = fDist;
	}

	bool bMayJump = vDir.y > m_fStepHeight && m_pCharacter->canJump();
	
	vDir.y = 0.0f;
	m_pCharacter->setWalkDirection(vDir * fMovDirLen);
	//g_pTracer2->begin(m_vPosition, 1);
	//g_pTracer2->lineTo(m_vPosition + vDir * fMovDirLen, 1);
	//g_pTracer2->end();
	//SPhysics_GetDynWorld()->getDebugDrawer()->drawLine(F3_BTVEC(m_vLastPathPos), F3_BTVEC(m_vPosition), btVector3(0, 1, 0));

	m_qOrientTo = SMQuaternion(NPC_BASE_DIR, vDir);
	setOrient(SMquaternionSlerp(getOrient(), m_qOrientTo, clampf(fDelta, 0.1f, 1.0f)));

	if(SMVector3Length(getPos() - m_vLastFramePos) < fDelta * fMoveSpeed * 0.1f) // застряли, похоже
	{
		if(bMayJump)
		{
			m_pCharacter->jump();
		}
		if(m_iStuckCount++ > 5)
		{
			//if(ID_VALID(m_idQuadGoingTo))
			//{
			//	printf(COLOR_LRED "I'm stuck, trying again" COLOR_RESET "\n");
			//	stopMotion();
			//	ID idTmp = m_idQuadGoingTo;
			//	m_idQuadGoingTo = -1;
			//	goTo(idTmp);
			//}
			//else
			//{
				printf(COLOR_LRED "I'm stuck, giving up" COLOR_RESET "\n");
				m_isPathInterrupted = true;
				stopMotion();
			//}
			return;
		}
		}
		else
	{
		m_iStuckCount = 0;
	}
	m_vLastFramePos = getPos();

	// это последний, дошли!
	if(m_iCurrQuaidInPath >= (int)m_aPathQuads.size() && fDist < 0.5)
	{
		stopMotion();
	}
}

void CNPCBase::stopMotion(bool runIdleAnim)
{
	m_isGoing = false;
	if(runIdleAnim)
	{
		playActivity("ACT_IDLE", 300);
	}

	m_pCharacter->setWalkDirection(float3(0, 0, 0));

	CLEAR_INTERVAL(m_idPathFollowInterval);
	m_idPathFollowInterval = -1;
}

void CNPCBase::goTo(const float3 &vPos, bool bRun)
{
	goTo(SAIG_QuadGetNear(&vPos, 3.0f, true, 3), bRun);
}

float CNPCBase::canSee(CBaseCharacter *pOther)
{
	if(pOther->isObserver())
	{
		return(0.0f);
	}
	return(canSee(pOther->getHead()));
}

float CNPCBase::canSee(CBaseEntity *pOther)
{
	float3 vDir = pOther->getPos() - m_pHeadEnt->getPos();
	float fDirLen = SMVector3Length(vDir);

	if(fDirLen > m_fViewDistance || fDirLen <= 0.00001f)
	{
		return(0.0f);
	}

	vDir /= fDirLen;

	float3 vEyeDir = m_pHeadEnt->getOrient() * float3(0, 0, 1);

	if(SMVector3Dot(vDir, vEyeDir) < cosf(m_fViewConeAngle))
	{
		return(0.0f);
}

	CClosestNotMeRayResultCallback cb(getBtCollisionObject());
	GetPhysWorld()->rayTest(m_pHeadEnt->getPos(), pOther->getPos(), &cb, CG_NPCVIEW, CG_ALL & ~(CG_DEBRIS | CG_TRIGGER | CG_CHARACTER));

	if(cb.hasHit() && cb.m_result.pCollisionObject->getUserPointer() && cb.m_result.pCollisionObject->getUserTypeId() == 1)
	{
		CBaseEntity *pEnt = (CBaseEntity*)cb.m_result.pCollisionObject->getUserPointer();
		if(!pEnt || pEnt != pOther)
		{
			return(0.0f);
		}
	}

	return(1.0f - fDirLen / m_fViewDistance);
}

#if 0
bool CNPCBase::pathFind(ID endq)
{
	if(m_idQuadCurr == endq)
	{
		// already here
		return(true);
	}
	if(ID_VALID(m_idQuadCurr))
	{
		if(ID_VALID(m_idQueueFindPath))
	{
			if(SAIG_GridGetSizePath(m_idQueueFindPath) >= 0)
		{
				if(m_aPathQuads.size() > 0)
				SAIG_GridSetColorArr(&(m_aPathQuads[0]), 0, m_aPathQuads.size());
			m_statePath = NPC_STATE_PATH_FOUND;
			int iCount = SAIG_GridGetSizePath(m_idQueueFindPath);
			m_aPathQuads.resize(iCount);
			SAIG_GridGetPath(m_idQueueFindPath, &(m_aPathQuads[0]), m_aPathQuads.size(), true);
			SAIG_GridSetColorArr(&(m_aPathQuads[0]), m_ulColor, m_aPathQuads.size());
			m_vLastPathPos = m_vPosition;
			m_idQueueFindPath = -1;
				return(true);
			}
		}
		else
		{
			m_idQueueFindPath = SAIG_GridQueryFindPath(m_idQuadCurr, endq);
		}
	}
	
	m_statePath = NPC_STATE_PATH_NOTFOUND;
	return(false);
}

void CNPCBase::think(float fDelta)
{
	if(m_fHealth <= 0.f)
		return;

	//определяем текущий id квада аи сетки
	m_idQuadCurr = SAIG_QuadGet(&(float3)m_vPosition, true);

	//если квад найти не удалось и состояние пути не равно немного/полностью вышел за пределы
	if(m_idQuadCurr < 0 && (m_statePath != NPC_STATE_PATH_BITBEYOND || m_statePath != NPC_STATE_PATH_BEYOND))
	{
		//если мы не движемся по пути
		if(m_iCurrQuaidInPath < 0 || m_iCurrQuaidInPath >= m_aPathQuads.size())
		{
			gridCheckBeyond();
		}
		//иначе мы движемся по пути, проверяем как далеко лежат предыдущий и следующий квады
		else
		{
			//если это не первый квад на пути, и дистанция между центром предыдущего квада и позицией нпс лежит в пределах допустимого
			if(m_iCurrQuaidInPath > 0 && SMVector3Distance(m_vPosition, m_vPosQuadInPathLast) <= NPC_QUAD_DIST_NOTBEYOND)
				m_idQuadCurr = m_aPathQuads[m_iCurrQuaidInPath - 1];
			//иначе если дистанция между центром следующего квада и позицией нпс лежит в пределах допустимого
			else if(SMVector3Distance(m_vPosition, m_vPosQuadInPathNext) <= NPC_QUAD_DIST_NOTBEYOND)
				m_idQuadCurr = m_aPathQuads[m_iCurrQuaidInPath];
			else //иначе что-то не так
			{
				gridCheckBeyond();
			}
		}
	}

	//если состояние пути немного вышел за пределы и установлено нужное время для прихода на сетку
	if(m_statePath == NPC_STATE_PATH_BITBEYOND && m_ulTimeReturnInGrid > 0)
	{
		int iTimeDelta = Core_RIntGet(G_RI_INT_TIME_DELTA);
		m_ulTimeAllReturnInGrid += iTimeDelta;

		//если общее время движения к сетке превысело предел
		if(m_ulTimeAllReturnInGrid >= m_ulTimeReturnInGrid)
		{
			//значит скорее всего нпс уже не вернется, устанавливаем состояние вышел за пределы
			m_statePath = NPC_STATE_PATH_BEYOND;
			m_ulTimeReturnInGrid = m_ulTimeAllReturnInGrid = 0;
			m_pCharacter->setWalkDirection(F3_BTVEC(float3(0, 0, 0)));
			//если был предыдущий путь
			if(m_aPathQuads.size() > 0)
				SAIG_GridSetColorArr(&(m_aPathQuads[0]), 0, m_aPathQuads.size());
		}
	}

	//если возвращение на сетку не допустимо
	if(m_statePath == NPC_STATE_PATH_BEYOND)
	{

		if(m_fHealth > 0.f)
		{
			//минусуем здоровье нпс, делаем -1 чтобы предки поняли что только что умер нпс
			takeHealth(m_fHealth);
			return;
		}
	}
	
	m_idQuadCurr = SAIG_QuadGet(&float3(m_vPosition), true);

	updateOrientLerp();

	pathWalk();

	if(m_statePath != NPC_STATE_PATH_NOTFOUND)
	{
		SET_TIMEOUT(think, 0.1f);
	}
}

#endif

void CNPCBase::orientAtPoint(const float3 &vPoint)
{
	float3 vOrientTo = vPoint - getPos();
	vOrientTo.y = 0.0f;
	m_qOrientTo = SMQuaternion(NPC_BASE_DIR, vOrientTo);
	m_qOldOrient = getOrient();
	m_fOrientAtDelta = 0;

	if(!ID_VALID(m_idOrientAtInterval))
	{
		m_idOrientAtInterval = SET_INTERVAL(orientAtThinker, 0);
	}
}

void CNPCBase::orientAtThinker(float fDelta)
{
	m_fOrientAtDelta += fDelta;
	if(m_fOrientAtDelta > 1.0f)
	{
		m_fOrientAtDelta = 1.0f;
		CLEAR_INTERVAL(m_idOrientAtInterval);
		m_idOrientAtInterval = -1;
	}

	setOrient(SMquaternionSlerp(m_qOldOrient, m_qOrientTo, m_fOrientAtDelta));
}

void CNPCBase::stopOrientAt()
{
	if(ID_VALID(m_idOrientAtInterval))
	{
		CLEAR_INTERVAL(m_idOrientAtInterval);
		m_idOrientAtInterval = -1;
	}
}


#if 0
void CNPCBase::onSync()
{
	BaseClass::onSync();
	/*btTransform transform;
	//SMQuaternion rot = m_pAnimPlayer->getBoneTransformRot(m_pAnimPlayer->getBone("bip01_ponytail1"));
	
	
	SMQuaternion rot = m_pHeadEnt->getOrient();
	float3 delta = rot * float3(0, 0, m_fViewDistance * 0.5f);
	float3 orig = m_pHeadEnt->getPos() + delta;
	transform.setOrigin(F3_BTVEC(orig));

	rot = SMQuaternion(SM_PI, 'y') * rot;

	transform.getBasis().setRotation(Q4_BTQUAT(rot));
	SPhysics_GetDynWorld()->getDebugDrawer()->drawCone(m_fViewDistance * tanf(m_fViewConeAngle), m_fViewDistance, 2, transform, btVector3(1, 1, 1));
	*/

	//SMQuaternion rot = m_pHeadEnt->getOrient();
	//SPhysics_GetDynWorld()->getDebugDrawer()->drawLine(F3_BTVEC(m_pHeadEnt->getPos()), F3_BTVEC(m_pHeadEnt->getPos() + (rot * float3(0,0,1))), btVector3(1,1,1));
}
void CNPCBase::gridCheckBeyond()
{
	//находим ближайший квад к текущей позиции нпс
	ID idq = SAIG_QuadGetNear(&(float3)m_vPosition, 5);
	if(ID_VALID(idq))
	{
		float3 tpos;
		SAIG_QuadGetPos(idq, &tpos);
		float dist = SMVector3Distance(m_vPosition, tpos);
		//если дистанция между текущей позицией нпс и позицией ближайшего квада лежит в допустимых пределах возвращения на сетку
		if(dist <= NPC_QUAD_DIST_BEYOND)
		{
			//просто сообщаем что вышли за пределы
			m_statePath = NPC_STATE_PATH_BITBEYOND;

			//если был предыдущий путь
			if(m_aPathQuads.size() > 0)
			{
				SAIG_GridSetColorArr(&(m_aPathQuads[0]), 0, m_aPathQuads.size());
			}

			//и устанавливаем путь возвращения
			m_aPathQuads.resize(1);
			m_aPathQuads[0] = idq;
			m_iCurrQuaidInPath = 0;
			SAIG_GridSetColorArr(&(m_aPathQuads[0]), m_ulColor, m_aPathQuads.size());

			//просчитываем время хождения по пути до сетки
			m_ulTimeReturnInGrid = (dist * m_fSpeedWalk) * 1000.f * 20.f;
			m_ulTimeAllReturnInGrid = 0;
		}
		else
		{
			m_statePath = NPC_STATE_PATH_BEYOND;
	}
	}
	else
	{
		m_statePath = NPC_STATE_PATH_BEYOND;
}
}
#endif 

#if 0
void CNPCBase::pathWalk()
{
	//если текущий ключ квада пути не известен
	if (m_iCurrQuaidInPath >= m_aPathQuads.size() || m_iCurrQuaidInPath < 0)
	{
		m_iCurrQuaidInPath = -1;
		m_stateMove = NPC_STATE_MOVE_IDLE_START;
		return;
	}

	m_stateMove = NPC_STATE_MOVE_RUN;

	//если текущий квад больше нуля и текущий квад нпс не явлется предыдущим квадом пути либо следующим квадом пути, значит скорее всего нпс сбился с пути
	if(m_iCurrQuaidInPath > 0 && (m_idQuadCurr != m_aPathQuads[m_iCurrQuaidInPath] && m_idQuadCurr != m_aPathQuads[m_iCurrQuaidInPath - 1]))
	{
		//если текущий квад нпс не явлется соседом предыдущему и следующему квадам пути, значит нпс вышел за допустимые пределы, а значит сбился с пути
		if(!SAIG_QuadIs2Neighbors(m_idQuadCurr, m_aPathQuads[m_iCurrQuaidInPath], m_aPathQuads[m_iCurrQuaidInPath - 1]))
		{
			m_statePath = NPC_STATE_PATH_LOST;
			m_pCharacter->setWalkDirection(F3_BTVEC(float3(0,0,0)));
			return;
		}
		else //иначе нпс чуток отклонился от маршрута, поправляем
			orientAtPoint(&(float3)m_vPosQuadInPathNext, NPC_TIME_ORIENT_IN_PATH);
	}

	//если следующий квад аи сетки несвободен и занят не текущим нпс
	if (SAIG_QuadGetState(m_aPathQuads[m_iCurrQuaidInPath]) != AIQUAD_STATE_FREE && SAIG_QuadGetStateWho(m_aPathQuads[m_iCurrQuaidInPath]) != getId())
	{
		//значит он преградил путь и текущий нпс сбился с пути
		m_statePath = NPC_STATE_PATH_LOST;
		m_pCharacter->setWalkDirection(F3_BTVEC(float3(0, 0, 0)));
		return;
	}

	//занимаем следующий квад аи сетки за текущим нпс
	SAIG_QuadSetState(m_aPathQuads[m_iCurrQuaidInPath], AIQUAD_STATE_TEMPBUSY);
	SAIG_QuadSetStateWho(m_aPathQuads[m_iCurrQuaidInPath], getId());

	SAIG_QuadGetPos(m_aPathQuads[m_iCurrQuaidInPath], &(float3)m_vPosQuadInPathNext);

	//если текущая позиция нпс ниже чем позиция квада к которому он стремится, и их разница больше либо равна допустимой
	if (m_vPosQuadInPathNext.y > m_vPosition.y && abs(m_vPosQuadInPathNext.y - m_vPosition.y) >= m_fStepHeight - NPC_STEP_HEIGHT_EPSILON)
	{
		//говорим что надо прыгнуть
		m_canJump = true;
	}

	//если разрешен прыжок
	if (m_canJump)
	{
		if (m_pCharacter->canJump())
		{
				m_pCharacter->jump();
				m_canJump = false;
		}
	}

	if (m_iCurrQuaidInPath == 0)
		orientAtPoint(&(float3)m_vPosQuadInPathNext, NPC_TIME_ORIENT_IN_PATH);

	float r_curr = SMVector3Distance(m_vPosition, m_vPosQuadInPathNext);
	float r_last = SMVector3Distance(m_vLastPathPos, m_vPosQuadInPathNext);

	if (SMVector3Distance(m_vPosition, m_vPosQuadInPathNext) <= 0.05f || r_curr > r_last)
	{
		if (m_iCurrQuaidInPath + 1 < m_aPathQuads.size())
			SAIG_QuadSetState(m_aPathQuads[m_iCurrQuaidInPath], AIQUAD_STATE_FREE);

		++m_iCurrQuaidInPath;

		//если было состояние пути немного вышел за пределы
		if (m_statePath == NPC_STATE_PATH_BITBEYOND)
		{
			//устанавилваем состояние не найденный путь
			m_statePath = NPC_STATE_PATH_NOTFOUND;
			m_ulTimeReturnInGrid = m_ulTimeAllReturnInGrid = 0;
		}

		if (m_iCurrQuaidInPath < m_aPathQuads.size() && m_iCurrQuaidInPath > 0)
		{
			m_vPosQuadInPathLast = m_vPosQuadInPathNext;
			SAIG_QuadGetPos(m_aPathQuads[m_iCurrQuaidInPath], &(float3)m_vPosQuadInPathNext);
			orientAtPoint(&(float3)m_vPosQuadInPathNext, NPC_TIME_ORIENT_IN_PATH);
		}
		else
		{
			m_pCharacter->setWalkDirection(F3_BTVEC(float3(0,0,0)));
		}
	}

	m_vLastPathPos = m_vPosition;
}



void CNPCBase::orientAtPoint(const float3 & pos, DWORD ttime)
{
	float3 curr_pos = getPos();
	curr_pos.y = 0;
	float3 poscam = float3(pos.x, 0, pos.z);
	float3 dircam = poscam - curr_pos;
	dircam = SMVector3Normalize(dircam);
	
	float angle = (acosf(SMVector3Dot(NPC_BASE_DIR, dircam)) * sign2(SMVector3Cross(NPC_BASE_DIR, dircam).y));
	angle = (-angle);
	
	//просчитываем направление движения и скорость
	float3 dir = SMVector3Normalize(SMQuaternion(angle, 'y') * NPC_BASE_DIR);
	float speed = 0;
	if (m_stateMove == NPC_STATE_MOVE_WALK)
		speed = m_fSpeedWalk;
	else if (m_stateMove == NPC_STATE_MOVE_RUN)
		speed = m_fSpeedRun;
	m_pCharacter->setWalkDirection(F3_BTVEC(dir * speed));

	if (ttime > 0)
	{
		m_fAngleYNext = angle;
		m_ulTimeAllRot = 0;
		m_ulTimeRot = ttime;
	}
	else
	{
		m_fAngleYLast = angle;
		m_fAngleYNext = angle;
		m_ulTimeAllRot = 0;
		m_ulTimeRot = 0;
		setOrient(SMQuaternion(angle, 'y'));
	}
}

void CNPCBase::updateOrientLerp()
{
	if (m_ulTimeRot <= 0)
		return;

	int iTimeDelta = Core_RIntGet(G_RI_INT_TIME_DELTA);
	m_ulTimeAllRot += iTimeDelta;
	float fCurrAngle = lerpf(m_fAngleYLast, m_fAngleYNext, saturatef(float(m_ulTimeAllRot) / float(m_ulTimeRot)));
	setOrient(SMQuaternion(fCurrAngle, 'y'));

	if (m_ulTimeAllRot >= m_ulTimeRot)
	{
		m_fAngleYLast = m_fAngleYNext;
		m_ulTimeAllRot = 0;
		m_ulTimeRot = 0;
	}
}
#endif