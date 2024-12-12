
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include <particles/sxparticles.h>
#include "BaseWeapon.h"
#include "Player.h"
#include "BaseAmmo.h"
#include "Random.h"
#include "HUDcontroller.h"

/*! \skydocent base_weapon
Базовый класс для оружия
*/


BEGIN_PROPTABLE(CBaseWeapon)
	//! Совместимые прицелы, классы через запятую
	DEFINE_FIELD_STRING(m_szAddonScopes, PDFF_NOEDIT | PDFF_NOEXPORT, "addon_scopes", "", EDITOR_NONE)
	//! Совместимые глушители
	DEFINE_FIELD_STRING(m_szAddonSilencers, PDFF_NOEDIT | PDFF_NOEXPORT, "addon_silencers", "", EDITOR_NONE)
	//! Совместимые магазины
	DEFINE_FIELD_STRING(m_szAddonMags, PDFF_NOEDIT | PDFF_NOEXPORT, "addon_mags", "", EDITOR_NONE)
	//! Совместимые ручки
	DEFINE_FIELD_STRING(m_szAddonHandlers, PDFF_NOEDIT | PDFF_NOEXPORT, "addon_handlers", "", EDITOR_NONE)

	//! Доступные режимы стрельбы
	DEFINE_FIELD_STRING(m_szFireModes, PDFF_NOEDIT | PDFF_NOEXPORT, "fire_modes", "", EDITOR_NONE)
	//! Скорострельность одиночными
	DEFINE_FIELD_INT(m_iSingleRate, PDFF_NOEDIT | PDFF_NOEXPORT, "single_rate", "", EDITOR_NONE)
	//! Скорострельность в автоматическом режиме
	DEFINE_FIELD_INT(m_iBurstRate, PDFF_NOEDIT | PDFF_NOEXPORT, "burst_rate", "", EDITOR_NONE)
	//! Скорострельность отсечками
	DEFINE_FIELD_INT(m_iCutoffRate, PDFF_NOEDIT | PDFF_NOEXPORT, "cutoff_rate", "", EDITOR_NONE)
	//! Патронов в отсечке
	DEFINE_FIELD_INT(m_iCutoffSize, PDFF_NOEDIT | PDFF_NOEXPORT, "cutoff_size", "", EDITOR_NONE)
	//! Текущий режим стрельбы
	DEFINE_FIELD_ENUM(FIRE_MODE, m_fireMode, PDFF_NOEDIT, "fire_mode", "", EDITOR_NONE)

	//! Звук извлечения
	DEFINE_FIELD_STRING(m_szSndDraw, PDFF_NOEDIT | PDFF_NOEXPORT, "snd_draw", "", EDITOR_NONE)
	//! Звук убирания
	DEFINE_FIELD_STRING(m_szSndHolster, PDFF_NOEDIT | PDFF_NOEXPORT, "snd_holster", "", EDITOR_NONE)
	//! Звук выстрела
	DEFINE_FIELD_STRING(m_szSndShoot, PDFF_NOEDIT | PDFF_NOEXPORT, "snd_shoot", "", EDITOR_NONE)
	//! Звук попытки выстрела без патрона
	DEFINE_FIELD_STRING(m_szSndEmpty, PDFF_NOEDIT | PDFF_NOEXPORT, "snd_empty", "", EDITOR_NONE)
	//! Звук перезарядки
	DEFINE_FIELD_STRING(m_szSndReload, PDFF_NOEDIT | PDFF_NOEXPORT, "snd_reload", "", EDITOR_NONE)
	//! Звук переключения
	DEFINE_FIELD_STRING(m_szSndSwitch, PDFF_NOEDIT | PDFF_NOEXPORT, "snd_switch", "", EDITOR_NONE)

	//! Эффективная дистанция, км
	DEFINE_FIELD_FLOAT(m_fEffectiveDistance, PDFF_NOEDIT | PDFF_NOEXPORT, "effective_distance", "", EDITOR_NONE)

	//! Вместимость патронов без магазина
	DEFINE_FIELD_INT(m_iCapacity, PDFF_NOEDIT | PDFF_NOEXPORT, "capacity", "", EDITOR_NONE)
	//! Текущая загрузка без учета магазина
	DEFINE_FIELD_INT(m_iCurrentLoad, PDFF_NOEDIT, "current_load", "", EDITOR_NONE)


	//! угол (в градусах) базовой дисперсии оружия (оружия, зажатого в тисках)
	DEFINE_FIELD_FLOAT(m_fBaseSpread, PDFF_NOEDIT | PDFF_NOEXPORT, "spread_base", "", EDITOR_NONE)
	//! коэффициент разброса в стоя
	DEFINE_FIELD_FLOAT(m_fSpreadIdle, PDFF_NOEDIT | PDFF_NOEXPORT, "spread_idle", "", EDITOR_NONE)
	//! коэффициент разброса пригнувшись
	DEFINE_FIELD_FLOAT(m_fSpreadCrouch, PDFF_NOEDIT | PDFF_NOEXPORT, "spread_crouch", "", EDITOR_NONE)
	//! коэффициент разброса лежа
	DEFINE_FIELD_FLOAT(m_fSpreadCrawl, PDFF_NOEDIT | PDFF_NOEXPORT, "spread_crawl", "", EDITOR_NONE)
	//! коэффициент разброса в ходьбе
	DEFINE_FIELD_FLOAT(m_fSpreadWalk, PDFF_NOEDIT | PDFF_NOEXPORT, "spread_walk", "", EDITOR_NONE)
	//! коэффициент разброса в беге
	DEFINE_FIELD_FLOAT(m_fSpreadRun, PDFF_NOEDIT | PDFF_NOEXPORT, "spread_run", "", EDITOR_NONE)
	//! коэффициент разброса в полете (прыжок)
	DEFINE_FIELD_FLOAT(m_fSpreadAirborne, PDFF_NOEDIT | PDFF_NOEXPORT, "spread_airborne", "", EDITOR_NONE)
	//! коэффициент разброса от состояния оружия
	DEFINE_FIELD_FLOAT(m_fSpreadCondition, PDFF_NOEDIT | PDFF_NOEXPORT, "spread_condition", "", EDITOR_NONE)
	//! коэффициент разброса от состояния рук
	DEFINE_FIELD_FLOAT(m_fSpreadArm, PDFF_NOEDIT | PDFF_NOEXPORT, "spread_arm", "", EDITOR_NONE)
	//! коэффициент разброса в прицеливании
	DEFINE_FIELD_FLOAT(m_fSpreadIronSight, PDFF_NOEDIT | PDFF_NOEXPORT, "spread_ironsight", "", EDITOR_NONE)

	//! Дальность пристрелки
	DEFINE_FIELD_FLOAT(m_fAimingRange, PDFF_NOEDIT | PDFF_NOEXPORT, "aiming_range", "", EDITOR_NONE)

	//! тип нарезки ствола: 0 - гладкоствольное; -1 - левая; 1 - правая
	DEFINE_FIELD_ENUM(RIFLE_TYPE, m_rifleType, PDFF_NOEDIT | PDFF_NOEXPORT, "rifle_type", "", EDITOR_NONE)
	//! шаг нарезки ствола (мм)
	DEFINE_FIELD_FLOAT(m_fRifleStep, PDFF_NOEDIT | PDFF_NOEXPORT, "rifle_step", "", EDITOR_NONE)
END_PROPTABLE()

REGISTER_ENTITY_NOLISTING(CBaseWeapon, base_weapon);

CBaseWeapon::CBaseWeapon()
{
	m_bIsWeapon = true;
}

CBaseWeapon::~CBaseWeapon()
{
	mem_release(m_pSndDraw);
	mem_release(m_pSndHolster);
	mem_release(m_pSndShoot);
	mem_release(m_pSndEmpty);
	mem_release(m_pSndReload);
	mem_release(m_pSndSwitch);
}

void CBaseWeapon::onPostLoad()
{
	BaseClass::onPostLoad();

	IXSoundSystem *pSound = (IXSoundSystem*)(Core_GetIXCore()->getPluginManager()->getInterface(IXSOUNDSYSTEM_GUID));
	if(pSound)
	{
		IXSoundLayer *pGameLayer = pSound->findLayer("xGame");

		if(pGameLayer){
			if(m_szSndDraw[0])
			{
				m_pSndDraw = pGameLayer->newSoundPlayer(m_szSndDraw, SOUND_SPACE_3D);
			}
			if(m_szSndHolster[0])
			{
				m_pSndHolster = pGameLayer->newSoundPlayer(m_szSndHolster, SOUND_SPACE_3D);
			}
			if(m_szSndShoot[0])
			{
				m_pSndShoot = pGameLayer->newSoundEmitter(m_szSndShoot, SOUND_SPACE_3D);
			}
			if(m_szSndEmpty[0])
			{
				m_pSndEmpty = pGameLayer->newSoundPlayer(m_szSndEmpty, SOUND_SPACE_3D);
			}
			if(m_szSndReload[0])
			{
				m_pSndReload = pGameLayer->newSoundPlayer(m_szSndReload, SOUND_SPACE_3D);
			}
			if(m_szSndSwitch[0])
			{
				m_pSndSwitch = pGameLayer->newSoundPlayer(m_szSndSwitch, SOUND_SPACE_3D);
			}
		}
	}
}

bool CBaseWeapon::setKV(const char * name, const char * value)
{
	if(!BaseClass::setKV(name, value))
	{
		return(false);
	}

	if(!strcmp(name, "fire_modes"))
	{
		size_t len = sizeof(char) * (strlen(value) + 1);
		char * tmpstr = (char*)alloca(len);
		memcpy(tmpstr, value, len);
		char * str[FIRE_MODE_COUNT];
		int c = parse_str(tmpstr, str, FIRE_MODE_COUNT);
		m_iFireModes = 0;
		for(int i = 0; i < c; ++i)
		{
			if(!strcmp(str[i], "single"))
			{
				m_iFireModes |= FIRE_MODE_SINGLE;
			}
			else if(!strcmp(str[i], "burst"))
			{
				m_iFireModes |= FIRE_MODE_BURST;
			}
			else if(!strcmp(str[i], "cutoff"))
			{
				m_iFireModes |= FIRE_MODE_CUTOFF;
			}
			else
			{
				printf(COLOR_LRED "Unknown fire mode '%s'\n" COLOR_RESET, str[i]);
			}
		}
		if(!m_iFireModes)
		{
			printf(COLOR_LRED "No firemodes defined for '%s'\n" COLOR_RESET, getClassName());
		}
	}
	return(true);
}

void CBaseWeapon::primaryAction(BOOL st)
{
	//m_bInPrimaryAction = st != FALSE;
	
	if(st)
	{
		if(canUse())
		{
			m_bInPrimaryAction = true;
			switch(m_fireMode)
			{
			case FIRE_MODE_BURST:
				m_idTaskShoot = SET_INTERVAL(taskShoot, 60.0f / (float)m_iBurstRate);
				break;
			case FIRE_MODE_CUTOFF:
				m_iCutoffCurrent = 0;
				m_idTaskShoot = SET_INTERVAL(taskShoot, 60.0f / (float)m_iCutoffRate);
				break;
			case FIRE_MODE_SINGLE:
				setNextUse(60.0f / (float)m_iSingleRate);
				break;
			}
			taskShoot(0.0f);
		}
	}
	else
		{
		if(m_fireMode != FIRE_MODE_CUTOFF)
			{
			if(ID_VALID(m_idTaskShoot))
			{
				CLEAR_INTERVAL(m_idTaskShoot);
				m_idTaskShoot = -1;
			}
			m_bInPrimaryAction = false;
		}
	}
}

void CBaseWeapon::secondaryAction(BOOL st)
{
	m_bInSecondaryAction = st != FALSE;
	if(m_iZoomable)
	{
		((CPlayer*)m_pOwner.getEntity())->getCrosshair()->enable(!st);
	}
}

void CBaseWeapon::reload()
{
	if(!m_pMag)
	{
		printf(COLOR_MAGENTA "Cannot reload without mag!\n" COLOR_RESET);
		return;
	}
	if(!m_pOwner)
	{
		printf(COLOR_MAGENTA "Cannot reload without owner!\n" COLOR_RESET);
		return;
	}
	if(canUse())
	{
		int iWantLoad = m_pMag->getCapacity() - m_pMag->getLoad()/* + m_iCapacity - m_iCurrentLoad */;
		if(iWantLoad <= 0)
		{
			printf(COLOR_MAGENTA "Mag full!\n" COLOR_RESET);
			return;
		}
		int count = ((CBaseCharacter*)m_pOwner.getEntity())->getInventory()->consumeItems(m_szLoadedAmmo, iWantLoad);
		if(count)
		{
			bool isFast = m_iCapacity == m_iCurrentLoad;

			count += m_iCurrentLoad;
			m_iCurrentLoad = min(count, m_iCapacity);
			count -= m_iCurrentLoad;
			m_pMag->load(count);

			setNextUse(m_fReloadTime);
			playAnimation(isFast ? "reload_fast" : "reload");
			if(isFast)
			{
				/*if(ID_VALID(m_idSndReloadFast))
				{
				SSCore_SndInstancePlay3d(m_idSndReloadFast, &getPos());
				}*/
			}
			else
			{
				SAFE_CALL(m_pSndReload, setWorldPos, getPos());
				SAFE_CALL(m_pSndReload, play);
			}

			CHUDcontroller * pHUD = ((CBaseCharacter*)getOwner())->getHUDcontroller();
			if(pHUD)
			{
				pHUD->setWeaponCurrentLoad((m_pMag ? m_pMag->getLoad() : 0) + m_iCurrentLoad);
				pHUD->setWeaponMaxAmmo(((CBaseCharacter*)m_pOwner.getEntity())->getInventory()->getItemCount(m_szLoadedAmmo));
			}
		}
		else
		{
			printf(COLOR_MAGENTA "No more bullets!\n" COLOR_RESET);
			return;
		}
	}
}

void CBaseWeapon::setFireMode(FIRE_MODE mode)
{
	if((m_iFireModes & mode) && canUse())
	{
		m_fireMode = mode;
		SAFE_CALL(m_pSndSwitch, setWorldPos, getPos());
		SAFE_CALL(m_pSndSwitch, play);
	}
}

void CBaseWeapon::nextFireMode()
{
	int cur = (int)log2f((float)m_fireMode);
	int newMode = cur;
	do
	{
		newMode = (newMode + 1) % FIRE_MODE_COUNT;
	}
	while(newMode != cur && !(m_iFireModes & (1 << newMode)));
	if(newMode != cur)
	{
		setFireMode((FIRE_MODE)(1 << newMode));
	}
}

bool CBaseWeapon::canShoot()
{
	return(m_iCurrentLoad > 0 || (m_pMag && m_pMag->getLoad() > 0));
}

float CBaseWeapon::getWeight()
{
	return(m_iInvWeight +
		(m_pHandle ? m_pHandle->getWeight() : 0.0f) +
		(m_pScope ? m_pScope->getWeight() : 0.0f) +
		(m_pMag ? m_pMag->getWeight() : 0.0f) +
		(m_pSilencer ? m_pSilencer->getWeight() : 0.0f)
	);
}

float CBaseWeapon::getBaseSpread() const
{
	return(m_fBaseSpread);
}

bool CBaseWeapon::isIronSight() const
{
	return(m_iZoomable && m_bInSecondaryAction);
}

float CBaseWeapon::getSpreadCoeff(SPREAD_COEFF what) const
{
	switch(what)
	{
	case SPREAD_COEFF_IDLE:
		return(m_fSpreadIdle);
	case SPREAD_COEFF_CROUCH:
		return(m_fSpreadCrouch);
	case SPREAD_COEFF_CRAWL:
		return(m_fSpreadCrawl);
	case SPREAD_COEFF_WALK:
		return(m_fSpreadWalk);
	case SPREAD_COEFF_RUN:
		return(m_fSpreadRun);
	case SPREAD_COEFF_AIRBORNE:
		return(m_fSpreadAirborne);
	case SPREAD_COEFF_CONDITION:
		return(m_fSpreadCondition);
	case SPREAD_COEFF_ARM:
		return(m_fSpreadArm);
	case SPREAD_COEFF_IRONSIGHT:
		return(m_fSpreadIronSight);
	}
	return(1.0f);
}

float3 CBaseWeapon::applySpread(const float3 &vDir, float fSpread)
{
	float3 vRight, vUp;

	if(vDir.x == 0.0f && vDir.y == 0.0f)
	{
		vRight = float3(0.0f, -1.0f, 0.0f);
		vUp = float3(-vDir.z, 0.0f, 0.0f);
	}
	else
	{
		vRight = SMVector3Normalize(SMVector3Cross(vDir, float3((0.0f, 0.0f, 1.0f))));
		vUp = SMVector3Normalize(SMVector3Cross(vRight, vDir));
	}



	float x, y, z;
	const float flatness = 0.5f;
	CRandom random;

	do
	{
		x = random.getRandomFloat(-1, 1) * flatness + random.getRandomFloat(-1, 1) * (1 - flatness);
		y = random.getRandomFloat(-1, 1) * flatness + random.getRandomFloat(-1, 1) * (1 - flatness);

		z = x*x + y*y;
	}
	while(z > 1);

	fSpread = tanf(fSpread);

	return(vDir + x * fSpread * vRight + y * fSpread * vUp);
}

void CBaseWeapon::taskShoot(float dt)
{
	if(m_fireMode == FIRE_MODE_CUTOFF)
	{
		++m_iCutoffCurrent;
		if(m_iCutoffCurrent > m_iCutoffSize)
		{
			if(ID_VALID(m_idTaskShoot))
			{
				CLEAR_INTERVAL(m_idTaskShoot);
				m_idTaskShoot = -1;
			}
			m_bInPrimaryAction = false;
			return;
		}
	}

	CBaseAmmo *pAmmo = NULL;
	if(canShoot() && getAmmo() && getAmmo()->isAmmo())
	{
		pAmmo = (CBaseAmmo*)getAmmo();
	}
	else
	{
		SAFE_CALL(m_pSndEmpty, setWorldPos, getPos());
		SAFE_CALL(m_pSndEmpty, play);

		if(ID_VALID(m_idTaskShoot))
		{
			CLEAR_INTERVAL(m_idTaskShoot);
			m_idTaskShoot = -1;
		}
		m_bInPrimaryAction = false;
		return;
	}

	playAnimation("shoot1");
	if(ID_VALID(m_iMuzzleFlash))
	{
		SPE_EffectSetEnable(m_iMuzzleFlash, true);
	}
	SAFE_CALL(m_pSndShoot, setWorldPos, getPos());
	SAFE_CALL(m_pSndShoot, play);

	//((CPlayer*)m_pOwner)->is

	//trace line
	float3 start = getPos();
	float3 dir = m_pParent->getOrient() * float3(0.0f, 0.0f, 1.0f);

	dir = SMQuaternion(m_pParent->getOrient() * float3(1.0f, 0.0f, 0.0f), asinf(m_fAimingRange * 10.0f / (pAmmo->getStartSpeed() * pAmmo->getStartSpeed())) * 0.5f) * dir;

	CBaseCharacter * pOwner = (CBaseCharacter*)getOwner();

	dir = applySpread(dir, SMToRadian(pOwner->getCurrentSpread()));

	pAmmo->fire(start, dir, pOwner);

	if(m_pMag && m_pMag->getLoad() > 0)
	{
		m_pMag->load(-1);
	}
	else
	{
		--m_iCurrentLoad;
	}

	CHUDcontroller * pHUD = pOwner->getHUDcontroller();
	if(pHUD)
	{
		pHUD->setWeaponCurrentLoad((m_pMag ? m_pMag->getLoad() : 0) + m_iCurrentLoad);
	}
}

void CBaseWeapon::attachMag(CBaseMag * pMag)
{
	m_pMag = pMag;

	int iNeedLoad = m_iCapacity - m_iCurrentLoad;
	if(iNeedLoad > 0)
	{
		if(m_pMag->getLoad() >= iNeedLoad)
		{
			m_iCurrentLoad += iNeedLoad;
			m_pMag->load(-iNeedLoad);
		}
		else
		{
			m_iCurrentLoad += m_pMag->getLoad();
			m_pMag->load(-m_pMag->getLoad());
		}
	}

	updateHUDinfo();
}

void CBaseWeapon::updateHUDinfo()
{
	if(m_pOwner)
	{
		CHUDcontroller *pHUD = ((CBaseCharacter*)m_pOwner.getEntity())->getHUDcontroller();
		if(pHUD)
		{
			pHUD->setWeaponMaxLoad((m_pMag ? m_pMag->getCapacity() : 0)/* + m_iCapacity*/);
			pHUD->setWeaponCurrentLoad((m_pMag ? m_pMag->getLoad() : 0) + m_iCurrentLoad);
			pHUD->setWeaponMaxAmmo(((CBaseCharacter*)m_pOwner.getEntity())->getInventory()->getItemCount(m_szLoadedAmmo));
		}
	}
}
