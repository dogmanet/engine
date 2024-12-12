
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#ifndef __ENTITY_FACTORY_H
#define __ENTITY_FACTORY_H

#include <gdefines.h>
#include <common/AssotiativeArray.h>
#include <common/String.h>
#include <common/AAString.h>
#include <common/Array.h>

#include "proptable.h"

typedef AssotiativeArray<AAString, const char *> EntDefaultsMap;

class CBaseEntity;
class CEntityManager;
class IEntityFactory
{
public:
	virtual CBaseEntity* create() = 0;
	virtual void destroy(CBaseEntity * pEnt) = 0;
	virtual const char* getClassName() = 0;
	virtual proptable_t* getPropTable() = 0;
	virtual bool isEditorHidden() = 0;
	virtual EntDefaultsMap* getDefaults() = 0;
	virtual IEntityFactory* copy(const char * szName, bool showInListing) = 0;
	virtual const char* getKV(const char *szKey) = 0;
};

class CEntityFactoryMap
{
	friend class CEntityManager;
public:
	CEntityFactoryMap();

	void addFactory(IEntityFactory *pFactory, const char *szName);
	CBaseEntity* create(const char *szName, CEntityManager *pMgr, bool bDelayPostLoad=false, const XGUID *pGUID=NULL);
	void destroy(CBaseEntity *pEnt);

	static CEntityFactoryMap* GetInstance();
	proptable_t* getPropTable(const char *szClass);
	bool isEditorHidden(const char *szClass);
	EntDefaultsMap* getDefaults(const char *szClass);

	int getListCount();
	void getListing(const char **pszOut, int size);

	const char* getClassNamePtr(const char *szClassName);

	static bool IsEntityOfClass(CBaseEntity *pEnt, const char *szClassName);

	template<typename T>
	static bool IsEntityOfClass(CBaseEntity *pEnt)
	{
		static_assert(std::is_base_of<CBaseEntity, T>::value, "T must be subclass of CBaseEntity");

		proptable_t *pPropTable0 = T::SGetPropTable();

		proptable_t *pPropTable1 = pEnt->getPropTable();

		do
		{
			if(pPropTable0 == pPropTable1)
			{
				return(true);
			}

			pPropTable1 = pPropTable1->pBaseProptable;
		}
		while(pPropTable1);

		return(false);
	}

	IEntityFactory* getFactory(const char *szName);
private:
	AssotiativeArray<AAString, IEntityFactory*> m_mFactories;
	int m_iShowInListCount;
};

template<class T>
class CEntityFactory: public IEntityFactory
{
public:
	CEntityFactory(const char *szName, std::initializer_list<const char*> listArgs)
	{
		m_szClassName = szName;
		m_pPropTable = T::SGetPropTable();

		const char *szKey, *szVal;
		const char * const * pArg = listArgs.begin();
		while(pArg != listArgs.end())
		{
			szKey = *pArg;
			++pArg;
			if(pArg != listArgs.end())
			{
				szVal = *pArg;
				++pArg;

				setKV(szKey, szVal);
			}
		}

		//m_bShowInListing = showInListing;
		CEntityFactoryMap::GetInstance()->addFactory(this, szName);
	}

	CEntityFactory(const CEntityFactory<T> &other)
	{
		*this = other;
		m_vDerivatives.clear();
	}

	~CEntityFactory()
	{
		T::ReleasePropData();
		for(int i = 0, l = m_vDerivatives.size(); i < l; ++i)
		{
			delete m_vDerivatives[i];
		}
	}

	IEntityFactory* copy(const char *szName, bool showInListing) override
	{
		CEntityFactory<T> * newF = new CEntityFactory<T>(*this);
		newF->m_szClassName = szName;
		newF->m_bShowInListing = showInListing;
		m_vDerivatives.push_back(newF);
		return(newF);
	}

	CBaseEntity* create() override
	{
		return(new T());
	}

	void destroy(CBaseEntity *pEnt) override
	{
		if(pEnt)
		{
			pEnt->getManager()->sheduleDestroy(pEnt);
		}
	}

	const char* getClassName() override
	{
		return(m_szClassName);
	}

	proptable_t* getPropTable() override
	{
		return(m_pPropTable);
	}

	bool isEditorHidden() override
	{
		return(!m_bShowInListing);
	}

	EntDefaultsMap* getDefaults() override
	{
		return(&m_mDefaults);
	}

	const char* getKV(const char *szKey) override
	{
		const Map<AAString, const char*>::Node *pNode;
		if(m_mapMeta.KeyExists(szKey, &pNode))
		{
			return(*pNode->Val);
		}
		return(NULL);
	}

private:
	const char *m_szClassName;
	proptable_t *m_pPropTable;
	bool m_bShowInListing = true;
	EntDefaultsMap m_mDefaults;
	Array<CEntityFactory<T>*> m_vDerivatives;

	Map<AAString, const char*> m_mapMeta;

private:
	void setKV(const char *szKey, const char *szVal)
	{
		if(!fstrcmp("show_in_listing", szKey))
		{
			m_bShowInListing = szVal[0] != '0';
		}
		m_mapMeta[szKey] = szVal;
	}
};

#define REC_KV(k, v) k, v
#define REC_NOLISTING() REC_KV("show_in_listing", "0")
#define REC_ICON(s) REC_KV("icon", s)
#define REC_MODEL(s) REC_KV("model", s)
#define REC_MODEL_FIELD(s) REC_KV("model_field", s)

#define REGISTER_ENTITY(cls, name, ...) \
	CEntityFactory<cls> ent_ ## name ## _factory(#name, {__VA_ARGS__})

#define REGISTER_ENTITY_NOLISTING(cls, name, ...) \
	REGISTER_ENTITY(cls, name, REC_NOLISTING(), __VA_ARGS__)

#define CREATE_ENTITY(cls, mgr) CEntityFactoryMap::GetInstance()->create(cls, mgr)
#define CREATE_ENTITY_NOPOST(cls, mgr) CEntityFactoryMap::GetInstance()->create(cls, mgr, 1)
#define REMOVE_ENTITY(ent) CEntityFactoryMap::GetInstance()->destroy(ent)

#endif
