
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

/*!
\file
Базовый игровой объект
*/

/*! \defgroup cbaseentity Объекты. Базовый объект
\ingroup sxgame
Базовый объект, от которого наследуются все остальные классы объектов.
@{
*/

#ifndef __BASE_ENTITY_H
#define __BASE_ENTITY_H

#include <gdefines.h>
#include <common/Math.h>
#include <core/sxcore.h>
#include <xcommon/physics/IXPhysics.h>
#include <xcommon/IXSoundSystem.h>
#include <xcommon/render/IXRenderUtils.h>
#include <xcommon/render/IXRender.h>
#include <xcommon/particles/IXParticleSystem.h>
#include "physics_util.h"
#include "EntityFactory.h"
#include "EntityManager.h"

#include "proptable.h"

#include "TakeDamageInfo.h"

#include <light/IXLightSystem.h>

#include <xcommon/resource/IXDecalProvider.h>

#pragma pointers_to_members(full_generality, virtual_inheritance)

IXRender* GetRender();
IXParticleSystem* GetParticleSystem();
IXDecalProvider* GetDecalProvider();



#pragma warning(push)
#pragma warning(disable:4251)

//! Базовый класс, от которого наследуются все остальные классы объектов.
class SXGAME_EXPORT CBaseEntity
{
	DECLARE_CLASS_NOBASE(CBaseEntity);
	DECLARE_PROPTABLE();

	friend class CEntityManager;

	template<typename T>
	friend class CEntityPointer;
	friend class CEntityList;
public:
	//! Конструктор
	CBaseEntity();
	virtual ~CBaseEntity();

	//! Возвращает имя движкового класса объекта
	const char* getClassName();
	
	//! Возвращает имя объекта
	const char* getName();
	
	//! Возвращает баунд объекта
	virtual void getMinMax(float3 *min, float3 *max);
	
	//! Устанавливает мировую позицию объекта
	virtual void setPos(const float3 &pos);
	//! Устанавливает относительное смещение объекта
	virtual void setOffsetPos(const float3 &pos);
	//! Получает мировую позицию объекта
	float3 getPos();
	float3 getOffsetPos();

	//! Получает флаги объекта
	UINT getFlags();
	//! Устанавливает флаги объекта
	void setFlags(UINT f);

	//! Получает мировую матрицу трансформации для объекта
	XDEPRECATED SMMATRIX getWorldTM();

	//! Устанавливает вращение объекта
	virtual void setOrient(const SMQuaternion &q);
	//! Устанавливает относительное вращение объекта
	virtual void setOffsetOrient(const SMQuaternion &q);
	//! Возвращает вращение объекта
	virtual SMQuaternion getOrient();

	void setXform(const float3 &vPos, const SMQuaternion &q);
	void setOffsetXform(const float3 &vPos, const SMQuaternion &q);

	//! Устанавливает свойство объекта
	virtual bool setKV(const char *name, const char *value);
	//! Получает свойство объекта
	virtual bool getKV(const char *name, char *out, int bufsize);

	//! Устанавливает родительский объект для объекта
	void setParent(CBaseEntity *pEnt, int attachment = -1);
	//! Возвращает родительский объект объекта
	CBaseEntity* getParent();

	//! Устанавливает владельца
	void setOwner(CBaseEntity *pEnt);
	//! Возвращает владельца объекта
	CBaseEntity* getOwner();

	//! Получает объект менеджера
	CEntityManager* getManager();

	//void updateDiscreteLinearVelocity(int step, float dt);
	//const float3_t & getDiscreteLinearVelocity() const;

	virtual void dispatchDamage(CTakeDamageInfo &takeDamageInfo);

	virtual void onDeath(CBaseEntity *pAttacker, CBaseEntity *pInflictor);

	void broadcastMessage(const char *szInputName, inputdata_t inputData, float fRadius);
	void broadcastMessage(const char *szInputName, float fArg, float fRadius);
	void broadcastMessage(const char *szInputName, int iArg, float fRadius);
	void broadcastMessage(const char *szInputName, UINT uArg, float fRadius);
	void broadcastMessage(const char *szInputName, bool bArg, float fRadius);
	void broadcastMessage(const char *szInputName, const char *szArg, float fRadius);
	void broadcastMessage(const char *szInputName, const float3_t &f3Arg, float fRadius);
	void broadcastMessage(const char *szInputName, float fRadius);

	virtual void onUse(CBaseEntity *pUser);

	virtual bool isBloody()
	{
		return(false);
	}

	virtual bool getMainColor(float3_t *pOut)
	{
		return(false);
	}

	//! Обновляет действие флагов в режиме редактора уровня
	virtual void updateFlags(){}

	virtual void renderEditor(bool is3D, bool bRenderSelection, IXGizmoRenderer *pRenderer);


	CBaseEntity* getEntByName(const char *szName, CBaseEntity *pStartFrom);
	int countEntByName(const char *szName);

	const XGUID* getGUID()
	{
		return(m_pGUID);
	}

	void setSeparateMovement(bool set)
	{
		m_isSeparateMovement = set;
	}

	void setName(const char *szName);

	virtual bool rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut = NULL, float3 *pvNormal = NULL, bool isRayInWorldSpace = true, bool bReturnNearestPoint = false)
	{
		return(false);
	}

private:
	void setClassName(const char *name);
	void setDefaults();

	void setGUID(const XGUID *pGUID)
	{
		m_pGUID = pGUID;
	}
	void setWorld(CEntityManager *pWorld)
	{
		m_pMgr = pWorld;
	}

	const char *m_szClassName = NULL;
	const XGUID *m_pGUID = NULL;

	SpinLock m_slPointers;
	Array<IEntityPointer*> m_aPointers;
	void registerPointer(IEntityPointer *pPtr);
	void unregisterPointer(IEntityPointer *pPtr);
	void notifyPointers();

	void onParentSet(CBaseEntity *pNewParent);
	void onParentUnset(CBaseEntity *pOldParent);

	Array<CBaseEntity*> m_aChildren;
	SpinLock m_slChildren;
	void addChild(CBaseEntity *pEnt);
	void removeChild(CBaseEntity *pEnt);

	bool m_isSeparateMovement = false;
	bool m_isInOnParentMoved = false;
	void onParentMoved(bool bAdjustOffsets = false);


	//! Позиция объекта
	float3_t m_vPosition;
	//! вращение
	SMQuaternion m_qOrientation;

	//! Позиция смещения (для иерархической структуры)
	float3_t m_vOffsetPos;
	//! Вращение смещения (для иерархической структуры)
	SMQuaternion m_qOffsetOrient;

	//! Имя объекта
	const char *m_szName = NULL;

	//! Флаги
	UINT m_iFlags = 0;
protected:
	virtual void _cleanup();

	CEntityManager *m_pMgr = NULL;
	
	//! Родитель
	// CBaseEntity *m_pParent = NULL;
	CEntityPointer<CBaseEntity> m_pParent;
	//! Индекс кости родителя
	int m_iParentAttachment = -1;

	//! Владелец
	CEntityPointer<CBaseEntity> m_pOwner;

	//! Вызывается при создании после установки всех свойств
	virtual void onPostLoad();

	//! Получает смещение для кости
	virtual float3 getAttachmentPos(int id);
	//! Получает вращение для кости
	virtual SMQuaternion getAttachmentRot(int id);

	//! здоровье [0,+inf]
	float m_fHealth = 100.0f;

	void takeHealth(float fVal, CBaseEntity *pAttacker, CBaseEntity *pInflictor=NULL);
	
	IXLightSystem *m_pLightSystem = NULL;
};

#pragma warning(pop)

#endif

/*! @} cbaseentity*/
