
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

/*!
\file
Класс игрока
*/

/*! \ingroup cbaseanimating
@{
*/

#ifndef __PLAYER_H
#define __PLAYER_H

#include "BaseCharacter.h"
#include "PointCamera.h"
#include "crosshair.h"
#include "BaseTool.h"
#include "CraftSystem.h"

//! Класс игрока  \ingroup cbaseanimating
class CPlayer: public CBaseCharacter
{
	DECLARE_CLASS(CPlayer, CBaseCharacter);
	DECLARE_PROPTABLE();
public:
	DECLARE_TRIVIAL_CONSTRUCTOR();
	~CPlayer();
	
	//! Возвращает камеру игрока
	CPointCamera * getCamera();

	//! Переключает в режим наблюдателя
	void observe();

	//! Помещает игрока в мир
	void spawn();

	/*! Запускает/останавливает движение в заданном направлении
		@param [in] dir направление движения из #PLAYER_MOVE
	*/
	void move(UINT dir, bool start);

	//! Обновляет инпут от игрока
	virtual void updateInput(float dt);
	
	//! Получает смещения для задержки движения модели оружия при вращении игрока
	float3_t & getWeaponDeltaAngles();

	//! Команды для подбора положения визуальной модели оружия на экране
	void _ccmd_slot_on(int argc, const char ** argv);
	void _ccmd_slot_off();

	//! Получает объект перекрестия
	CCrosshair * getCrosshair();

	CHUDcontroller * getHUDcontroller();

	void dispatchDamage(CTakeDamageInfo &takeDamageInfo);

	//! Temp method
	void setActiveTool(CBaseTool *pTool)
	{
		m_pActiveTool = pTool;
	}

	void attack(BOOL state);

	void onDeath(CBaseEntity *pAttacker, CBaseEntity *pInflictor);

	void respawn();
	
	void setOrient(const SMQuaternion &q) override;

	SMQuaternion getOrient() override;

	CCraftSystem* getCraftSystem();

protected:
	//! Камера
	CPointCamera * m_pCamera;

	//! ID интервала обновления
	ID m_iUpdIval;

	//! Может ли прыгать
	bool m_canJump = true;

	//! Для качания камеры @{
	float m_fViewbobStep = 0.0f;
	float m_fViewbobY = 0.0f;
	float m_fViewbobStrafe = 0.0f;
	float3_t m_vWpnShakeAngles = float3_t(0.0f, 0.0f, 0.0f);
	//! @}

	int m_iDSM = DSM_NONE;

	//! Перекрестие
	CCrosshair *m_pCrosshair = NULL;

	//! Обновляет разброса значение
	virtual void updateSpread(float dt);

	void onPostLoad() override;

	bool m_bCanRespawn = false;

	float3 getHeadOffset() override;

private:
	float3_t m_vTargetSpeed;
	float3_t m_vCurrentSpeed;

	CCraftSystem *m_pCraftSystem = NULL;
};

#endif

//! @}
