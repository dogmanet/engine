
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "EntityManager.h"

#include "BaseEntity.h"

#include "Baseline.h"

#include <core/sxcore.h>

void CEntityManager::CCVarEventListener::onEvent(const XEventCvarChanged *pEvent)
{
	static const bool *game_time_running = m_pCore->getConsole()->getPCVarBool("game_time_running");

	if(game_time_running == pEvent->pCvar)
	{
		m_pMgr->setTimeRunning(*game_time_running);
	}
}

//##########################################################################

CEntityManager::CEntityManager():
	m_cvarListener(this, Core_GetIXCore())
{
	loadDefaults();
	loadDynClasses();

	Core_0RegisterConcmdClsArg("ent_dump", this, (SXCONCMDCLSARG)&CEntityManager::dumpList);
	Core_0RegisterConcmdClsArg("ent_kv", this, (SXCONCMDCLSARG)&CEntityManager::entKV);

	Core_GetIXCore()->getConsole()->registerCVar("game_time_running", false, "Is game time running", FCVAR_NOTIFY);

	Core_GetIXCore()->getEventChannel<XEventCvarChanged>(EVENT_CVAR_CHANGED_GUID)->addListener(&m_cvarListener);
}

CEntityManager::~CEntityManager()
{
	Core_GetIXCore()->getEventChannel<XEventCvarChanged>(EVENT_CVAR_CHANGED_GUID)->removeListener(&m_cvarListener);

	mem_release(m_pDynClassConf);
	mem_release(m_pDefaultsConf);
}

void CEntityManager::update(int thread)
{
	static const bool *game_time_running = Core_GetIXCore()->getConsole()->getPCVarBool("game_time_running");
	if(!*game_time_running)
	{
		return;
	}

	time_point tNow = std::chrono::high_resolution_clock::now();
	timeout_t * t;
	timeout_output_t * to;
	long long mksdt;

	for(int i = thread, l = m_vOutputTimeout.size(); i < l; i += m_iThreadNum)
	{
		to = &m_vOutputTimeout[i];
		if(to->status == TS_WAIT && to->fNextTime < tNow)
		{
			//mksdt = std::chrono::duration_cast<std::chrono::microseconds>(tNow - t->fStartTime).count();
			//(t->pEnt->*(t->func))((float)mksdt / 1000000.0f);
			to->status = TS_DONE;

			inputdata_t inputData = {0};
			inputData.pActivator = to->data.pActivator;
			inputData.pInflictor = to->data.pInflictor;

			for(UINT j = 0, jl = to->pOutput->aOutputs.size(); j < jl; ++j)
			{
				input_t &out = to->pOutput->aOutputs[j];
				if(!out.pTarget)
				{
					continue;
				}

				inputData.type = out.data.type;

				if(inputData.type == PDF_STRING)
				{
					inputData.parameter.str = NULL;
				}

				if(out.useOverrideData)
				{
					inputData.setParameter(out.data);
				}
				else
				{
					inputData.setParameter(to->data);
				}

				(out.pTarget->*(out.fnInput))(&inputData);

				if(inputData.type == PDF_STRING && inputData.parameter.str != GetEmptyString())
				{
					delete[] inputData.parameter.str;
				}

				// Update pointer. Array can be reallocated during previous function call
				to = &m_vOutputTimeout[i];
			}

			if(to->data.type == PDF_STRING && to->data.parameter.str != GetEmptyString())
			{
				delete[] to->data.parameter.str;
			}
		}
	}

	for(int i = thread, l = m_vTimeout.size(); i < l; i += m_iThreadNum)
	{
		t = &m_vTimeout[i];
		if(t->status == TS_WAIT && t->fNextTime < tNow)
		{
			mksdt = std::chrono::duration_cast<std::chrono::microseconds>(tNow - t->fStartTime).count();
			t->status = TS_DONE;

			// m_vTimeout array can be reallocated during this function call
			(t->pEnt->*(t->func))((float)mksdt / 1000000.0f);
		}
	}

	for(int i = thread, l = m_vInterval.size(); i < l; i += m_iThreadNum)
	{
		t = &m_vInterval[i];
		if(t->status == TS_WAIT && t->fNextTime < tNow)
		{
			mksdt = std::chrono::duration_cast<std::chrono::microseconds>(tNow - t->fStartTime).count();
			t->fNextTime = t->fNextTime - t->fStartTime + tNow;
			t->fStartTime = tNow;

			// m_vInterval array can be reallocated during this function call
			(t->pEnt->*(t->func))((float)mksdt / 1000000.0f);
		}
	}
}

void CEntityManager::setThreadNum(int num)
{
	if(num > 0)
	{
		m_iThreadNum = num;
	}
}

void CEntityManager::finalRemove()
{
	CBaseEntity * pEnt;
	// do not store m_vEntRemoveList.size()! It can change during iteration
	for(int i = 0; i < (int)m_vEntRemoveList.size(); ++i)
	{
		pEnt = m_vEntRemoveList[i];
		if(pEnt)
		{
			pEnt->_cleanup();
			delete pEnt;
		}
	}
	m_vEntRemoveList.clearFast();
}

void CEntityManager::sync()
{
	finalRemove();

	for(int i = 0, l = m_vTimeout.size(); i < l; ++i)
	{
		if(m_vTimeout[i].status == TS_DONE)
		{
			m_vTimeout[i].status = TS_EMPTY;
			m_vFreeTimeout.push_back(i);
		}
	}
	for(int i = 0, l = m_vInterval.size(); i < l; ++i)
	{
		if(m_vInterval[i].status == TS_DONE)
		{
			m_vInterval[i].status = TS_EMPTY;
			m_vFreeInterval.push_back(i);
		}
	}
	for(int i = 0, l = m_vOutputTimeout.size(); i < l; ++i)
	{
		if(m_vOutputTimeout[i].status == TS_DONE)
		{
			m_vOutputTimeout.erase(i);
			--i;
			--l;
		}
	}
	//static time_point tOld = std::chrono::high_resolution_clock::now();
	//float dt;
	//dt = (float)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - tOld).count() / 1000000.0f;
	
	//tOld = std::chrono::high_resolution_clock::now();
}

void CEntityManager::unloadObjLevel()
{
	CBaseEntity * pEnt;
	for (int i = 0, l = m_vEntList.size(); i < l; ++i)
	{
		pEnt = m_vEntList[i];
		if (pEnt && (pEnt->getFlags() & EF_LEVEL))
		{
			REMOVE_ENTITY(pEnt);
		}
	}
}

const XGUID* CEntityManager::reg(CBaseEntity *pEnt, const XGUID *pGUID)
{
	if(!pEnt)
	{
		return(NULL);
	}

	XGUID guid;
	if(pGUID)
	{
		guid = *pGUID;
	}
	else
	{
		XCreateGUID(&guid);
	}

	m_mEnts[guid] = pEnt;

	const XGUID *pNewGUID = &m_mEnts.TmpNode->Key;

	pEnt->setGUID(pNewGUID);
	pEnt->setWorld(this);

	m_vEntList.push_back(pEnt);

	if(pGUID)
	{
		notifyWaitForGUID(*pGUID, pEnt);

		char tmp[64];
		XGUIDToSting(*pGUID, tmp, sizeof(tmp));
		onEntityNameChanged(pEnt, "", tmp);
	}

	return(pNewGUID);
}
void CEntityManager::unreg(CBaseEntity *pEnt)
{
	assert(pEnt && pEnt->getGUID());

	int iKey = m_vEntList.indexOf(pEnt);
	assert(iKey >= 0);
	UINT uLast = m_vEntList.size() - 1;
	m_vEntList[iKey] = m_vEntList[uLast];
	m_vEntList[uLast] = NULL;
	m_vEntList.erase(uLast);

	timeout_t * t;
	timeout_output_t * to;

	for(int i = 0, l = m_vOutputTimeout.size(); i < l; ++i)
	{
		to = &m_vOutputTimeout[i];
		if(to->status == TS_WAIT)
		{
			UINT iRemoved = 0;
			for(UINT j = 0, jl = to->pOutput->aOutputs.size(); j < jl; ++j)
			{
				input_t &out = to->pOutput->aOutputs[j];

				if(out.pTarget == pEnt)
				{
					out.pTarget = NULL;
					++iRemoved;
				}
			}
			if(iRemoved == to->pOutput->aOutputs.size())
			{
				to->status = TS_DONE;
			}
		}
	}

	for(int i = 0, l = m_vTimeout.size(); i < l; ++i)
	{
		t = &m_vTimeout[i];
		if(t->status == TS_WAIT && t->pEnt == pEnt)
		{
			t->status = TS_DONE;
		}
	}

	for(int i = 0, l = m_vInterval.size(); i < l; ++i)
	{
		t = &m_vInterval[i];
		if(t->status == TS_WAIT && t->pEnt == pEnt)
		{
			t->status = TS_DONE;
		}
	}

	m_mEnts.erase(*pEnt->getGUID());
}

ID CEntityManager::setTimeout(void(CBaseEntity::*func)(float dt), CBaseEntity *pEnt, float delay)
{
	timeout_t t;
	t.status = TS_WAIT;
	t.func = func;
	t.pEnt = pEnt;

	t.fStartTime = t.fNextTime = m_isTimeRunning ? std::chrono::high_resolution_clock::now() : m_tStopPoint;
	t.fNextTime += std::chrono::microseconds((long long)(delay * 1000000.0f));
	
	ID id;
	{
		ScopedSpinLock lock(m_mxTimeout);

		if(m_vFreeTimeout.size())
		{
			id = m_vFreeTimeout[0];
			m_vFreeTimeout.erase(0);
		}
		else
		{
			id = m_vTimeout.size();
		}
		m_vTimeout[id] = t;
	}

	return(id);
}

ID CEntityManager::setInterval(void(CBaseEntity::*func)(float dt), CBaseEntity *pEnt, float delay)
{
	timeout_t t;
	t.status = TS_WAIT;
	t.func = func;
	t.pEnt = pEnt;

	t.fStartTime = t.fNextTime = m_isTimeRunning ? std::chrono::high_resolution_clock::now() : m_tStopPoint;
	t.fNextTime += std::chrono::microseconds((long long)(delay * 1000000.0f));

	ID id;
	{
		ScopedSpinLock lock(m_mxInterval);
		if(m_vFreeInterval.size())
		{
			id = m_vFreeInterval[0];
			m_vFreeInterval.erase(0);
		}
		else
		{
			id = m_vInterval.size();
		}
		m_vInterval[id] = t;
	}

	return(id);
}

void CEntityManager::clearInterval(ID id)
{
	if(id < 0 || (UINT)id >= m_vInterval.size())
	{
		return;
	}
	if(m_vInterval[id].status == TS_WAIT)
	{
		m_vInterval[id].status = TS_DONE;
	}
}
void CEntityManager::clearTimeout(ID id)
{
	if(id < 0 || (UINT)id >= m_vTimeout.size())
	{
		return;
	}
	if(m_vTimeout[id].status == TS_WAIT)
	{
		m_vTimeout[id].status = TS_DONE;
	}
}

bool CEntityManager::exportList(const char * file)
{
	ISXConfig * conf = Core_CrConfig();
	conf->New(file);
	char buf[4096], sect[64];
	CBaseEntity * pEnt;
	proptable_t * pTbl;
	int ic = 0;

	//if(m_isOldImported)
	{
		FILE *fp = fopen(file, "w");
		if(fp)
		{
			fclose(fp);
		}
		m_isOldImported = false;
	}

	// conf->set("meta", "count", "0");

	for(int i = 0, l = m_vEntList.size(); i < l; ++i)
	{
		pEnt = m_vEntList[i];
		if(!pEnt)
		{
			continue;
		}

		if(!(pEnt->getFlags() & EF_EXPORT))
		{
			continue;
		}

		XGUIDToSting(*pEnt->getGUID(), sect, sizeof(sect));

		conf->set(sect, "classname", pEnt->getClassName());
		pTbl = CEntityFactoryMap::GetInstance()->getPropTable(pEnt->getClassName());
		do
		{
			for(int j = 0; j < pTbl->numFields; ++j)
			{
				if(pTbl->pData[j].szKey && !(pTbl->pData[j].flags & (PDFF_INPUT | PDFF_NOEXPORT | PDFF_MESSAGE)) && !conf->keyExists(sect, pTbl->pData[j].szKey))
				{
					pEnt->getKV(pTbl->pData[j].szKey, buf, sizeof(buf));
					conf->set(sect, pTbl->pData[j].szKey, buf);
				}
			}
		}
		while((pTbl = pTbl->pBaseProptable));
		++ic;
	}

	// sprintf(buf, "%d", ic);
	// conf->set("meta", "count", buf);

	bool ret = !conf->save();
	mem_release(conf);
	return(ret);
}

bool CEntityManager::import(const char * file, bool shouldSendProgress)
{
	IEventChannel<XEventLevelProgress> *pEventChannel = NULL;
	if(shouldSendProgress)
	{
		pEventChannel = Core_GetIXCore()->getEventChannel<XEventLevelProgress>(EVENT_LEVEL_PROGRESS_GUID);
	}
	ISXConfig *conf = Core_CrConfig();
	const char *sect;
	CBaseEntity *pEnt = NULL;
	Array<CBaseEntity*> tmpList;

	char szFullPath[1024];
	if(!Core_GetIXCore()->getFileSystem()->resolvePath(file, szFullPath, sizeof(szFullPath)))
	{
		goto err;
	}

	if(conf->open(szFullPath))
	{
		goto err;
	}

	int ic = conf->getSectionCount();
	tmpList.reserve(ic);

	m_isOldImported = conf->getKeyCount("meta") != 0;

	{
		XGUID guid;

		XEventLevelProgress ev;
		ev.idPlugin = -3;
		ev.szLoadingText = "Загрузка объектов игрового мира";
		ev.type = XEventLevelProgress::TYPE_PROGRESS_VALUE;
		ev.fProgress = 0.0f;
		for(int i = 0; i < ic; ++i)
		{
			sect = conf->getSectionName(i);

			if(sect[0] == '{' || (sect[0] == 'e' && sect[1] == 'n' && sect[2] == 't' && sect[3] == '_'))
			{
				if(!conf->keyExists(sect, "classname"))
				{
					printf(COLOR_LRED "Unable to load entity #%d, classname undefined\n" COLOR_RESET, i);
					tmpList[i] = NULL;
					continue;
				}

				XGUID *pGUID = NULL;
				if(XGUIDFromString(&guid, sect))
				{
					pGUID = &guid;
				}

				if(!(pEnt = CEntityFactoryMap::GetInstance()->create(conf->getKey(sect, "classname"), this, true, pGUID)))
				{
					printf(COLOR_LRED "Unable to load entity #%d, classname '%s' undefined\n" COLOR_RESET, i, conf->getKey(sect, "classname"));
					tmpList[i] = NULL;
					continue;
				}
				if(conf->keyExists(sect, "name"))
				{
					pEnt->setKV("name", conf->getKey(sect, "name"));
				}
				if(conf->keyExists(sect, "origin"))
				{
					pEnt->setKV("origin", conf->getKey(sect, "origin"));
				}
				if(conf->keyExists(sect, "rotation"))
				{
					pEnt->setKV("rotation", conf->getKey(sect, "rotation"));
				}
				pEnt->setFlags(pEnt->getFlags() | EF_EXPORT | EF_LEVEL);
				tmpList[i] = pEnt;

				if((i & 0xF) == 0 && pEventChannel)
				{
					ev.fProgress = (float)i / (float)ic * 0.1f;
					pEventChannel->broadcastEvent(&ev);
				}
			}
			else
			{
				tmpList[i] = NULL;
			}
		}

		for(int i = 0; i < ic; ++i)
		{
			sect = conf->getSectionName(i);

			if(sect[0] == '{' || (sect[0] == 'e' && sect[1] == 'n' && sect[2] == 't' && sect[3] == '_'))
			{
				if(!(pEnt = tmpList[i]))
				{
					continue;
				}
				int keyc = conf->getKeyCount(sect);
				const char *key;
				for(int j = 0; j < keyc; ++j)
				{
					key = conf->getKeyName(sect, j);
					if(strcmp(key, "classname") && strcmp(key, "origin") && strcmp(key, "name") && strcmp(key, "rotation"))
					{
						pEnt->setKV(key, conf->getKey(sect, key));
					}
				}
				pEnt->onPostLoad();

				if(/*(i & 0xF) == 0 && */pEventChannel)
				{
					ev.fProgress = (float)i / (float)ic * 0.9f + 0.1f;
					pEventChannel->broadcastEvent(&ev);
				}
			}
			else
			{
				tmpList[i] = NULL;
			}
		}
	}

	mem_release(conf);
	return(true);

err:
	mem_release(conf);
	return(false);
}


CBaseEntity* CEntityManager::findEntityByName(const char *szName, CBaseEntity *pStart)
{
	if(!szName[0])
	{
		return(NULL);
	}
	bool bFound = !pStart;
	CBaseEntity * pEnt;
	for(int i = 0, l = m_vEntList.size(); i < l; ++i)
	{
		pEnt = m_vEntList[i];
		if(!pEnt)
		{
			continue;
		}
		if(bFound)
		{
			if(!strcmp(pEnt->getName(), szName))
			{
				return(pEnt);
			}
		}
		else
		{
			if(pEnt == pStart)
			{
				bFound = true;
			}
		}
	}
	return(NULL);
}

int CEntityManager::countEntityByName(const char *name)
{
	if(!name[0])
	{
		return(0);
	}
	int c = 0;
	for(int i = 0, l = m_vEntList.size(); i < l; ++i)
	{
		if(m_vEntList[i] && !strcmp(m_vEntList[i]->getName(), name))
		{
			++c;
		}
	}
	return(c);
}

CBaseEntity* CEntityManager::findEntityByClass(const char *name, CBaseEntity *pStart)
{
	bool bFound = !pStart;
	CBaseEntity * pEnt;
	for(int i = 0, l = m_vEntList.size(); i < l; ++i)
	{
		pEnt = m_vEntList[i];
		if(!pEnt)
		{
			continue;
		}
		if(bFound)
		{
			if(!strcmp(pEnt->getClassName(), name))
			{
				return(pEnt);
			}
		}
		else
		{
			if(pEnt == pStart)
			{
				bFound = true;
			}
		}
	}
	return(NULL);
}

CBaseEntity* CEntityManager::findEntityInSphere(const float3 &f3Origin, float fRadius, CBaseEntity *pStart)
{
	bool bFound = !pStart;
	CBaseEntity * pEnt;
	for(int i = 0, l = m_vEntList.size(); i < l; ++i)
	{
		pEnt = m_vEntList[i];
		if(!pEnt)
		{
			continue;
		}
		if(bFound)
		{
			if(SMVector3Length2(pEnt->getPos() - f3Origin) < fRadius * fRadius)
			{
				return(pEnt);
			}
		}
		else
		{
			if(pEnt == pStart)
			{
				bFound = true;
			}
		}
	}
	return(NULL);
}

void CEntityManager::loadDefaults()
{
	m_pDefaultsConf = Core_CrConfig();
	if(m_pDefaultsConf->open("config/entities/defaults.ent") < 0)
	{
		mem_release(m_pDefaultsConf);
		return;
	}

	const char * sect, * key;
	EntDefaultsMap * defs;

	for(int i = 0, l = m_pDefaultsConf->getSectionCount(); i < l; ++i)
	{
		sect = m_pDefaultsConf->getSectionName(i);
		if(!(defs = CEntityFactoryMap::GetInstance()->getDefaults(sect)))
		{
			continue;
		}
		for(int j = 0, jl = m_pDefaultsConf->getKeyCount(sect); j < jl; ++j)
		{
			key = m_pDefaultsConf->getKeyName(sect, j);
			defs[0][AAString(key)] = m_pDefaultsConf->getKey(sect, key);
		}
	}
}

void CEntityManager::loadDynClasses()
{
	m_pDynClassConf = Core_CrConfig();
	if(m_pDynClassConf->open("config/entities/classes.ent") < 0)
	{
		mem_release(m_pDynClassConf);
		return;
	}

	const char * newClass, *key, *baseClass;
	EntDefaultsMap * defs, *baseDefs;
	bool bShow = true;

	for(int i = 0, l = m_pDynClassConf->getSectionCount(); i < l; ++i)
	{
		newClass = m_pDynClassConf->getSectionName(i);
		if(!(baseClass = m_pDynClassConf->getKey(newClass, "base_class")))
		{
			printf(COLOR_LRED "Couldn't create entity class '%s': Unknown base class\n" COLOR_RESET, newClass);
			continue;
		}
		IEntityFactory * pOldFactory = CEntityFactoryMap::GetInstance()->getFactory(baseClass);
		if(!pOldFactory)
		{
			printf(COLOR_LRED "Couldn't create entity class '%s': Base class '%s' is undefined\n" COLOR_RESET, newClass, baseClass);
			continue;
		}
		if((key = m_pDynClassConf->getKey(newClass, "show_in_listing")))
		{
			bShow = strcmp(key, "0") && strcmp(key, "false");
		}
		IEntityFactory * newFactory = pOldFactory->copy(newClass, bShow);
		CEntityFactoryMap::GetInstance()->addFactory(newFactory, newClass);

		if(!(defs = CEntityFactoryMap::GetInstance()->getDefaults(newClass)))
		{
			continue;
		}
		if((baseDefs = CEntityFactoryMap::GetInstance()->getDefaults(baseClass)))
		{
			for(EntDefaultsMap::Iterator i = baseDefs->begin(); i; ++i)
			{
				defs[0][*i.first] = *i.second;
			}
		}
		for(int j = 0, jl = m_pDynClassConf->getKeyCount(newClass); j < jl; ++j)
		{
			key = m_pDynClassConf->getKeyName(newClass, j);
			defs[0][AAString(key)] = m_pDynClassConf->getKey(newClass, key);
		}
	}
}

void CEntityManager::dumpList(int argc, const char **argv)
{
	const char * filter = "";
	if(argc > 1)
	{
		filter = argv[1];
	}

	printf(COLOR_GREEN "---------------------------------------------------------------------------------------\n"
		COLOR_GREEN "    Filter: " COLOR_LGREEN "%s\n"
		COLOR_GREEN "    Total count: " COLOR_LGREEN "%u\n"
		COLOR_GREEN "---------------------------------------------------------------------------------------\n"
		"                  guid                  |          class           |       name       |\n"
		"----------------------------------------|--------------------------|------------------|\n"
		, filter, m_vEntList.size());
	char tmp[64];
	for(int i = 0, l = m_vEntList.size(); i < l; ++i)
	{
		CBaseEntity * pEnt = m_vEntList[i];
		if(!pEnt)
		{
			continue;
		}
		if(!filter[0] || strstr(pEnt->getClassName(), filter))
		{
			XGUIDToSting(*pEnt->getGUID(), tmp, sizeof(tmp));
			printf(" " COLOR_LGREEN "%s" COLOR_GREEN " | " COLOR_LGREEN "%-24s" COLOR_GREEN " | " COLOR_LGREEN "%-16s" COLOR_GREEN " |\n", tmp, pEnt->getClassName(), pEnt->getName());
		}
	}

	printf("---------------------------------------------------------------------------------------\n" COLOR_RESET);
}

void CEntityManager::entKV(int argc, const char **argv)
{
	int id = 0;
	if(argc == 1)
	{
		printf("Usage: \n    ent_kv <entid> [keyname [new_value]]\n");
		return;
	}
	if(!sscanf(argv[1], "%d", &id))
	{
		printf("Usage: \n    ent_kv <entid> [keyname [new_value]]\n");
		return;
	}
	CBaseEntity * pEnt;
	if(id < 0 || (UINT)id >= m_vEntList.size() || !(pEnt = m_vEntList[id]))
	{
		printf(COLOR_LRED "Invalid entity id\n" COLOR_RESET);
		return;
	}
	char buf[128];
	buf[0] = 0;
	switch(argc)
	{
	case 2: // dump all KVs
		{
			proptable_t * pt = CEntityFactoryMap::GetInstance()->getPropTable(pEnt->getClassName());
			while(pt)
			{
				for(int i = 0; i < pt->numFields; ++i)
				{
					if(pt->pData[i].szKey && !(pt->pData[i].flags & PDFF_INPUT))
					{
						pEnt->getKV(pt->pData[i].szKey, buf, sizeof(buf));
						printf("%s = %s\n", pt->pData[i].szKey, buf);
					}
				}
				pt = pt->pBaseProptable;
			}
		}
		break;
	case 3: // show KV
		pEnt->getKV(argv[2], buf, sizeof(buf));
		printf("%s = %s\n", argv[2], buf);
		break;

	case 4: // set new KV
		pEnt->setKV(argv[2], argv[3]);
		break;
	}
}

int CEntityManager::getCount()
{
	return(m_vEntList.size());
}

CBaseEntity* CEntityManager::getById(ID id)
{
	if(id < 0 || (UINT)id >= m_vEntList.size())
	{
		return(NULL);
	}
	CBaseEntity *pEnt = m_vEntList[id];
	return(pEnt ? (pEnt->getFlags() & EF_REMOVED ? NULL : pEnt) : NULL);
}

CBaseEntity* CEntityManager::getByGUID(const XGUID &guid)
{
	CBaseEntity *const *ppEnt = m_mEnts.at(guid);
	return(ppEnt ? *ppEnt : NULL);
}

CBaseEntity* CEntityManager::cloneEntity(CBaseEntity *pOther)
{
	if(!pOther)
	{
		return(NULL);
	}
	CBaseEntity *pEnt = CREATE_ENTITY_NOPOST(pOther->getClassName(), this);

	pEnt->setKV("name", pOther->getName());
	pEnt->setPos(pOther->getPos());
	pEnt->setOrient(pOther->getOrient());
	pEnt->setParent(pOther->getParent());
	pEnt->setOwner(pOther->getOwner());
	char buf[4096];

	proptable_t *pTbl = CEntityFactoryMap::GetInstance()->getPropTable(pOther->getClassName());
	do
	{
		for(int j = 0; j < pTbl->numFields; ++j)
		{
			if(pTbl->pData[j].szKey && !(pTbl->pData[j].flags & PDFF_INPUT))
			{
				if(fstrcmp(pTbl->pData[j].szKey, "origin") 
					&& fstrcmp(pTbl->pData[j].szKey, "name") 
					&& fstrcmp(pTbl->pData[j].szKey, "rotation") 
					&& fstrcmp(pTbl->pData[j].szKey, "parent") 
					&& fstrcmp(pTbl->pData[j].szKey, "owner"))
				{
					pOther->getKV(pTbl->pData[j].szKey, buf, sizeof(buf));
					pEnt->setKV(pTbl->pData[j].szKey, buf);
				}
			}
		}
	}
	while((pTbl = pTbl->pBaseProptable));

	pEnt->setFlags(pOther->getFlags());

	pEnt->onPostLoad();

	return(pEnt);
}

void CEntityManager::setOutputTimeout(named_output_t *pOutput, inputdata_t *pData)
{
	timeout_output_t t;
	t.status = TS_WAIT;
	t.pOutput = pOutput;
	t.data = *pData;

	t.fStartTime = t.fNextTime = m_isTimeRunning ? std::chrono::high_resolution_clock::now() : m_tStopPoint;
	t.fNextTime += std::chrono::microseconds((long long)((pOutput->useRandomDelay ? randf(pOutput->fDelay, pOutput->fDelayTo) : pOutput->fDelay) * 1000000.0f));

	m_vOutputTimeout.push_back(t);
}

void CEntityManager::sheduleDestroy(CBaseEntity *pEnt)
{
	if(pEnt->getFlags() & EF_REMOVED)
	{
		return;
	}
	pEnt->setFlags(pEnt->getFlags() | EF_REMOVED);
	m_vEntRemoveList.push_back(pEnt);
	
	pEnt->notifyPointers();
}

void CEntityManager::setEditorMode(bool isEditor)
{
	if(m_isEditorMode == isEditor)
	{
		return;
	}
	m_isEditorMode = isEditor;
}

bool CEntityManager::isEditorMode()
{
	return(m_isEditorMode);
}

void CEntityManager::setTimeRunning(bool set)
{
	time_point tNow = std::chrono::high_resolution_clock::now();
	m_isTimeRunning = set;
	if(set)
	{
		auto tDelta = tNow - m_tStopPoint;

		timeout_t *t;
		timeout_output_t *to;

		for(UINT i = 0, l = m_vOutputTimeout.size(); i < l; ++i)
		{
			to = &m_vOutputTimeout[i];
			if(to->status == TS_WAIT)
			{
				to->fNextTime += tDelta;
				to->fStartTime += tDelta;
			}
		}

		for(UINT i = 0, l = m_vTimeout.size(); i < l; ++i)
		{
			t = &m_vTimeout[i];
			if(t->status == TS_WAIT)
			{
				t->fNextTime += tDelta;
				t->fStartTime += tDelta;
			}
		}

		for(UINT i = 0, l = m_vInterval.size(); i < l; ++i)
		{
			t = &m_vInterval[i];
			if(t->status == TS_WAIT)
			{
				t->fNextTime += tDelta;
				t->fStartTime += tDelta;
			}
		}
	}
	else
	{
		m_tStopPoint = tNow;
	}
}

CBaseline* CEntityManager::createBaseline(ID id)
{
	if(!ID_VALID(id))
	{
		id = m_aBaselines.size();
	}
	else
	{
		if((UINT)id < m_aBaselines.size())
		{
			mem_delete(m_aBaselines[id]);
		}
		else
		{
			m_aBaselines.reserve(id + 1);
			for(int i = m_aBaselines.size(); i < id + 1; ++i)
			{
				m_aBaselines[i] = 0;
			}
		}

	}
	CBaseline *pBaseline = m_aBaselines[id] = new CBaseline(id);

	CBaseEntity *pEnt;
	proptable_t * pTbl;
	char buf[4096];
	EntDefaultsMap *pDefaults;
	const EntDefaultsMap::Node *pNode;

	for(int i = 0, l = m_vEntList.size(); i < l; ++i)
	{
		pEnt = m_vEntList[i];
		if(pEnt && (pEnt->getFlags() & EF_LEVEL) && !(pEnt->getFlags() & EF_REMOVED))
		{
			CBaseline::ent_record_s record;
			record.m_guid = *pEnt->getGUID();
			record.m_sClassname = pEnt->getClassName();

			pDefaults = CEntityFactoryMap::GetInstance()->getDefaults(pEnt->getClassName());
			pTbl = CEntityFactoryMap::GetInstance()->getPropTable(pEnt->getClassName());
			do
			{
				for(int j = 0; j < pTbl->numFields; ++j)
				{
					if(pTbl->pData[j].szKey && !(pTbl->pData[j].flags & (PDFF_INPUT | PDFF_MESSAGE)) && !record.m_mProps.KeyExists(pTbl->pData[j].szKey))
					{
						pEnt->getKV(pTbl->pData[j].szKey, buf, sizeof(buf));
						//test defaults
						if(!pDefaults->KeyExists(pTbl->pData[j].szKey, &pNode) || strcmp(*(pNode->Val), buf))
						{
							record.m_mProps[pTbl->pData[j].szKey] = buf;
						}
					}
				}
			}
			while((pTbl = pTbl->pBaseProptable));

			pBaseline->m_aRecords.push_back(record);
		}
	}
	return(pBaseline);
}

CBaseline* CEntityManager::getBaseline(ID id)
{
	if(!ID_VALID(id) || (UINT)id >= m_aBaselines.size())
	{
		return(NULL);
	}

	return(m_aBaselines[id]);
}

void CEntityManager::dispatchBaseline(CBaseline *pBaseline)
{
	CBaseEntity *pEnt;
	unloadObjLevel();
	finalRemove();
	//m_vFreeIDs.clearFast();
	//m_vEntList.resize(RESERVED_SLOTS);

	if(!pBaseline->m_aRecords.size())
	{
		return;
	}

#if 0
	UINT idMax = pBaseline->m_aRecords[pBaseline->m_aRecords.size() - 1].m_id;
	if(m_vEntList.size() <= idMax)
	{
		m_vEntList.reserve(idMax + 1);
	}
#endif

	Array<CBaseEntity*> aPostLoadList(pBaseline->m_aRecords.size());

	const AssotiativeArray<String, String>::Node *pNode;
	for(int i = 0, l = pBaseline->m_aRecords.size(); i < l; ++i)
	{
		CBaseline::ent_record_s *pRecord = &pBaseline->m_aRecords[i];

		if(!(pEnt = CEntityFactoryMap::GetInstance()->create(pRecord->m_sClassname.c_str(), this, true, &pRecord->m_guid)))
		{
			printf(COLOR_LRED "Unable to load entity #%d, classname '%s' undefined\n" COLOR_RESET, i, pRecord->m_sClassname.c_str());
			aPostLoadList[i] = NULL;
			continue;
		}

		aPostLoadList[i] = pEnt;

#if 0
		if(pEnt->getId() != pRecord->m_id)
		{
			m_vEntList[pEnt->getId()] = NULL;
			m_vFreeIDs.push_back(pEnt->getId());

			for(UINT j = m_vEntList.size(); j < pRecord->m_id; ++j)
			{
				m_vEntList[j] = NULL;
				m_vFreeIDs.push_back(j);
			}
			if(pRecord->m_id < m_vEntList.size() && m_vEntList[pRecord->m_id])
			{
				ID newId = m_vEntList.size();
				if(m_vFreeIDs.size())
				{
					newId = m_vFreeIDs[m_vFreeIDs.size() - 1];
					m_vFreeIDs.erase(m_vFreeIDs.size() - 1);
				}
				m_vEntList[newId] = m_vEntList[pRecord->m_id];
				m_vEntList[newId]->m_iId = newId;
			}
			else
			{
				int idx = m_vFreeIDs.indexOf(pRecord->m_id);
				if(idx >= 0)
				{
					m_vFreeIDs.erase(idx);
				}
			}
			m_vEntList[pRecord->m_id] = pEnt;
			pEnt->m_iId = pRecord->m_id;
		}
#endif

		if(pRecord->m_mProps.KeyExists("name", &pNode))
		{
			pEnt->setKV("name", pNode->Val->c_str());
		}
		if(pRecord->m_mProps.KeyExists("origin", &pNode))
		{
			pEnt->setKV("origin", pNode->Val->c_str());
		}
		if(pRecord->m_mProps.KeyExists("rotation", &pNode))
		{
			pEnt->setKV("rotation", pNode->Val->c_str());
		}

		pEnt->setFlags(pEnt->getFlags() | EF_LEVEL | EF_EXPORT);
	}

	const char *key;
	for(int i = 0, l = pBaseline->m_aRecords.size(); i < l; ++i)
	{
		CBaseline::ent_record_s *pRecord = &pBaseline->m_aRecords[i];

		if((pEnt = aPostLoadList[i]))
		{
			for(AssotiativeArray<String, String>::Iterator it = pRecord->m_mProps.begin(); it; it++)
			{
				key = it.first->c_str();

				if(strcmp(key, "origin") && strcmp(key, "name") && strcmp(key, "rotation"))
				{
					pEnt->setKV(key, it.second->c_str());
				}
			}
		}
	}

	for(UINT i = 0, l = aPostLoadList.size(); i < l; ++i)
	{
		SAFE_CALL(aPostLoadList[i], onPostLoad);
	}
}

#if 0
void CEntityManager::serializeBaseline(CBaseline *pBaseline, INETbuff *pBuf)
{
	CBaseline::ent_record_s *pRecord;
	const char *key;

	pBuf->writeChar(pBaseline->m_aRecords.size());
	for(int i = 0, l = pBaseline->m_aRecords.size(); i < l; ++i)
	{
		pRecord = &pBaseline->m_aRecords[i];
		pBuf->writeChar(pRecord->m_id);
		pBuf->writeString(pRecord->m_sClassname.c_str());

		pBuf->writeChar(pRecord->m_mProps.Size());
		for(AssotiativeArray<String, String>::Iterator it = pRecord->m_mProps.begin(); it; it++)
		{
			pBuf->writeString(it.first->c_str());
			pBuf->writeString(it.second->c_str());
		}
	}
}

CBaseline *CEntityManager::deserializeBaseline(ID id, INETbuff *pBuf)
{
	assert(ID_VALID(id));

	if(id < m_aBaselines.size())
	{
		mem_delete(m_aBaselines[id]);
	}
	else
	{
		m_aBaselines.reserve(id + 1);
		for(int i = m_aBaselines.size(); i < id + 1; ++i)
		{
			m_aBaselines[i] = 0;
		}
	}
	CBaseline *pBaseline = m_aBaselines[id] = new CBaseline(id);

	UINT uCount = pBuf->readChar();
	char buf[4096], buf2[32];

	for(UINT i = 0; i < uCount; ++i)
	{
		CBaseline::ent_record_s record;
		record.m_id = pBuf->readChar();
		pBuf->readString(buf, sizeof(buf));
		record.m_sClassname = buf;

		//printf("Ent: %s\n", buf);

		UINT uPropCount = pBuf->readChar();
		for(UINT j = 0; j < uPropCount; ++j)
		{
			pBuf->readString(buf2, sizeof(buf2));
			pBuf->readString(buf, sizeof(buf));
			//printf("    %s: \"%s\"\n", buf2, buf);
			record.m_mProps[buf2] = buf;
		}
		//printf("\n");

		pBaseline->m_aRecords.push_back(record);
	}

	return(pBaseline);
}
#endif

void CEntityManager::registerWaitForGUID(const XGUID &guid, IEntityPointer *pPtr)
{
	ScopedSpinLock lock(m_slWaitingPointers);

	m_maWaitingPointers[guid].push_back(pPtr);
}

void CEntityManager::unregisterWaitForGUID(const XGUID &guid, IEntityPointer *pPtr)
{
	ScopedSpinLock lock(m_slWaitingPointers);

	Array<IEntityPointer*> &list = m_maWaitingPointers[guid];

	int idx = list.indexOf(pPtr);
	assert(idx >= 0);
	if(idx >= 0)
	{
		list.erase(idx);
	}
	if(!list.size())
	{
		m_maWaitingPointers.erase(guid);
	}
}

void CEntityManager::notifyWaitForGUID(const XGUID &guid, CBaseEntity *pEnt)
{
	ScopedSpinLock lock(m_slWaitingPointers);

	const Map<XGUID, Array<IEntityPointer*>>::Node *pNode;
	if(m_maWaitingPointers.KeyExists(guid, &pNode))
	{
		Array<IEntityPointer*> &list = m_maWaitingPointers[guid];
		for(UINT i = 0, l = list.size(); i < l; ++i)
		{
			list[i]->onWaitDone(pEnt);
		}
		m_maWaitingPointers.erase(guid);
	}
}

void CEntityManager::registerWaitForName(const char *szName, CEntityList *pList)
{
	ScopedSpinLock lock(m_slWaitingLists);

	Array<CEntityList*> &list = m_maWaitingLists[szName];

	assert(list.indexOf(pList) < 0);

	list.push_back(pList);
}
void CEntityManager::unregisterWaitForName(const char *szName, CEntityList *pList)
{
	ScopedSpinLock lock(m_slWaitingLists);

	Array<CEntityList*> &list = m_maWaitingLists[szName];

	int idx = list.indexOf(pList);
	assert(idx >= 0);
	list[idx] = list[list.size() - 1];
	list.erase(list.size() - 1);
}

void CEntityManager::onEntityNameChanged(CBaseEntity *pEnt, const char *szOldName, const char *szNewName)
{
	const Map<String, Array<CEntityList*>>::Node *pNode;
	if(szOldName[0] && m_maWaitingLists.KeyExists(szOldName, &pNode))
	{
		Array<CEntityList*> &list = *pNode->Val;
		for(UINT i = 0, l = list.size(); i < l; ++i)
		{
			list[i]->removeEntity(pEnt);
		}
	}

	if(szNewName[0] && m_maWaitingLists.KeyExists(szNewName, &pNode))
	{
		Array<CEntityList*> &list = *pNode->Val;
		for(UINT i = 0, l = list.size(); i < l; ++i)
		{
			list[i]->addEntity(pEnt);
		}
	}
}
