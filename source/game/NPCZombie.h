
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

/*!
\file
Базовый класс npc
*/

/*! \ingroup cbaseanimating
@{
*/

#ifndef __NPCZOMBIE_H
#define __NPCZOMBIE_H

#include "NPCBase.h"

//! состояния опасности
enum NPC_STATE_DANGER
{
	NPC_STATE_DANGER_CALM = 0,	//!< спокоен
	NPC_STATE_DANGER_UNVISIBLE = 1,	//!< беспокоен, но не знает где опасность
	NPC_STATE_DANGER_VISIBLE = 2,	//!< беспокоен и знает где опасность
};

//! Класс NPC зомби
class CNPCZombie: public CNPCBase
{
	DECLARE_CLASS(CNPCZombie, CNPCBase);
	DECLARE_PROPTABLE();

public:
	DECLARE_CONSTRUCTOR();
	~CNPCZombie();

	void onDeath(CBaseEntity *pAttacker, CBaseEntity *pInflictor);

	void dispatchDamage(CTakeDamageInfo &takeDamageInfo);

	void setPos(const float3 & pos) override;

protected:
	void think(float fDelta);
	void removeThis(float fDelta);

	void msgKnowEnemyHere(inputdata_t *pInputdata);
	void msgFiringHere(inputdata_t *pInputdata);

	void onPostLoad() override;

	//virtual void initPhysics();

	void randWalk();
	IXSoundPlayer *m_pSndIdle = NULL;
	IXSoundPlayer *m_pSndIdle2 = NULL;
	IXSoundPlayer *m_pSndDeath = NULL;

	NPC_STATE_DANGER m_stateDanger;
	float3_t m_vLastDangerPos;

	void rotateThink(float fDT);
	void startRotateThink(float fDelta);
	ID m_idRotateInterval;
	int m_iObserveRotateStep;
	float m_fRotationAngle;

	bool m_isGoingToDangerPos;
	float3_t m_vGoingToDangerPos;
	float m_fGoingToDangerPosTime;
};

#endif

//! @}
