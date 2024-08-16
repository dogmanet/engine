
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include <stdio.h>

#include "BaseEntity.h"

#include "EntityManager.h"

#include "GameData.h"

/*! \skydocent base_entity
Базовый объект. От него наследуются все остальные объекты
*/

BEGIN_PROPTABLE_NOBASE(CBaseEntity)
	//! Имя объекта
	DEFINE_FIELD_STRINGFN(m_szName, 0, "name", "Name", setName, EDITOR_TEXTFIELD)
	//! Позиция в мире
	DEFINE_FIELD_VECTORFN(m_vPosition, 0, "origin", "Origin", setPos, EDITOR_TEXTFIELD)
	//! Ориентация в мире, углы эйлера или кватернион
	DEFINE_FIELD_ANGLESFN(m_qOrientation, 0, "rotation", "Rotation", setOrient, EDITOR_TEXTFIELD)
	//! Родительский объект в иерархии движения
	DEFINE_FIELD_ENTITY(CBaseEntity, m_pParent, 0, "parent", "Parent entity", EDITOR_TEXTFIELD)
	//! Флаги объекта
	DEFINE_FIELD_FLAGS(m_iFlags, 0, "flags", "Flags", EDITOR_FLAGS)
	//! Объект-владелец
	DEFINE_FIELD_ENTITY(CBaseEntity, m_pOwner, PDFF_NOEXPORT | PDFF_NOEDIT, "owner", "", EDITOR_NONE)
	//! Здоровье
	DEFINE_FIELD_FLOAT(m_fHealth, PDFF_NOEXPORT | PDFF_NOEDIT, "health", "", EDITOR_NONE)

	//DEFINE_FIELD_STRING(m_szName, 0, "some opt", "Option", EDITOR_COMBOBOX)
	//	COMBO_OPTION("Option 1", "value 1")
	//	COMBO_OPTION("Option 2", "value 2")
	//	COMBO_OPTION("Option 3", "value 3")
	//EDITOR_COMBO_END()
END_PROPTABLE()

REGISTER_ENTITY_NOLISTING(CBaseEntity, base_entity);

void CBaseEntity::setDefaults()
{
	proptable_t *pt = getPropTable();
	const char *estr = GetEmptyString();
	while(pt)
	{
		for(int i = 0; i < pt->numFields; ++i)
		{
			if(!(pt->pData[i].flags & PDFF_INPUT))
			{
				if(pt->pData[i].type == PDF_STRING)
				{
					this->*((const char* ThisClass::*)pt->pData[i].pField) = estr;
				}
				else if(pt->pData[i].type == PDF_ENTITY)
				{
					(this->*((CEntityPointer<CBaseEntity> ThisClass::*)pt->pData[i].pField)).init(this);
				}
			}
		}
		pt = pt->pBaseProptable;
	}
}

CBaseEntity::CBaseEntity()
{
	m_pLightSystem = GameData::m_pLightSystem;

	m_pParent.setLinkEstablishedListener(&CBaseEntity::onParentSet);
	m_pParent.setLinkBrokenListener(&CBaseEntity::onParentUnset);
}

/*void CBaseEntity::setDefaults()
{
	proptable_t * pt = getPropTable();
	const char * estr = GetEmptyString();
	const char * defstr = NULL;
	while(pt)
	{
		for(int i = 0; i < pt->numFields; ++i)
		{
			if(pt->pData[i].type == PDF_STRING)
			{
				this->*((const char * ThisClass::*)pt->pData->pField) = estr;
			}
		}
		pt = pt->pBaseProptable;
	}
}*/

CBaseEntity::~CBaseEntity()
{
	m_pMgr->unreg(this);

	proptable_t * pt = getPropTable();
	const char * estr = GetEmptyString();
	while(pt)
	{
		for(int i = 0; i < pt->numFields; ++i)
		{
			if(pt->pData[i].type == PDF_STRING && !(pt->pData[i].flags & PDFF_INPUT))
			{
				const char * str = this->*((const char * ThisClass::*)pt->pData[i].pField);
				if(str && str != estr)
				{
					delete[] str;
				}
			}
		}
		pt = pt->pBaseProptable;
	}

	if(m_pParent)
	{
		m_pParent->removeChild(this);
	}
}

void CBaseEntity::setClassName(const char * name)
{
	m_szClassName = name;
}

const char* CBaseEntity::getName()
{
	return(m_szName);
}

const char* CBaseEntity::getClassName()
{
	return(m_szClassName);
}

void CBaseEntity::getMinMax(float3 * min, float3 * max)
{
	if (min)
		min->x = min->y = min->z = 0;

	if (max)
		max->x = max->y = max->z = 0;
}

void CBaseEntity::setPos(const float3 &pos)
{
	m_vPosition = pos;

	onParentMoved(true);

	{
		ScopedSpinLock lock(m_slChildren);
		for(UINT i = 0, l = m_aChildren.size(); i < l; ++i)
		{
			m_aChildren[i]->onParentMoved(m_isSeparateMovement);
		}
	}
}

float3 CBaseEntity::getPos()
{
	return(m_vPosition);
}

void CBaseEntity::setOrient(const SMQuaternion & q)
{
	m_qOrientation = q;

	onParentMoved(true);

	{
		ScopedSpinLock lock(m_slChildren);
		for(UINT i = 0, l = m_aChildren.size(); i < l; ++i)
		{
			m_aChildren[i]->onParentMoved(m_isSeparateMovement);
		}
	}
}
void CBaseEntity::setOffsetOrient(const SMQuaternion & q)
{
	m_qOffsetOrient = q;
	onParentMoved();
}

void CBaseEntity::setOffsetPos(const float3 & pos)
{
	m_vOffsetPos = pos;
	onParentMoved();
}

void CBaseEntity::setXform(const float3 &vPos, const SMQuaternion &q)
{
	bool bOld = m_isInOnParentMoved;
	m_isInOnParentMoved = true;
	setPos(vPos);
	m_isInOnParentMoved = bOld;

	setOrient(q);
}
void CBaseEntity::setOffsetXform(const float3 &vPos, const SMQuaternion &q)
{
	bool bOld = m_isInOnParentMoved;
	m_isInOnParentMoved = true;
	setOffsetPos(vPos);
	m_isInOnParentMoved = bOld;

	setOffsetOrient(q);
}

float3 CBaseEntity::getOffsetPos()
{
	return(m_vOffsetPos);
}

SMQuaternion CBaseEntity::getOrient()
{
	return(m_qOrientation);
}

void CBaseEntity::onParentMoved(bool bAdjustOffsets)
{
	if(!m_pParent || m_isInOnParentMoved)
	{
		return;
	}

	m_isInOnParentMoved = true;

	float3 vParentPos = ID_VALID(m_iParentAttachment) ? m_pParent->getAttachmentPos(m_iParentAttachment) : m_pParent->m_vPosition;
	SMQuaternion qParentOrient = ID_VALID(m_iParentAttachment) ? m_pParent->getAttachmentRot(m_iParentAttachment) : m_pParent->m_qOrientation;

	if(bAdjustOffsets)
	{
		m_vOffsetPos = (float3)(qParentOrient.Conjugate() * (m_vPosition - vParentPos));
		m_qOffsetOrient = m_qOrientation * qParentOrient.Conjugate();
	}
	else
	{
		setPos(vParentPos + qParentOrient * m_vOffsetPos);
		setOrient(m_qOffsetOrient * qParentOrient);
	}

	m_isInOnParentMoved = false;
}

UINT CBaseEntity::getFlags()
{
	return(m_iFlags);
}

void CBaseEntity::setFlags(UINT f)
{
	m_iFlags = f;
}

SMMATRIX CBaseEntity::getWorldTM()
{
	return(m_qOrientation.GetMatrix() * SMMatrixTranslation(m_vPosition));
}

bool CBaseEntity::setKV(const char * name, const char * value)
{
	propdata_t * field = getField(name);
	if(!field || (field->flags & PDFF_INPUT))
	{
		return(false);
	}
	float3_t f3;
	float4_t f4;
	SMQuaternion q;
	int d;
	float f;
	UINT u;
	switch(field->type)
	{
	case PDF_INT:
		if(1 == sscanf(value, "%d", &d))
		{
			if(field->fnSet.i)
			{
				(this->*(field->fnSet.i))(d);
			}
			else
			{
				this->*((int ThisClass::*)field->pField) = d;
			}
			return(true);
		}
		return(false);
	case PDF_UINT:
		if(1 == sscanf(value, "%u", &u))
		{
			if(field->fnSet.i)
			{
				(this->*(field->fnSet.i))(u);
			}
			else
			{
				this->*((UINT ThisClass::*)field->pField) = u;
			}
			return(true);
		}
		return(false);
	case PDF_FLOAT:
		if(1 == sscanf(value, "%f", &f))
		{
			if(field->fnSet.f)
			{
				(this->*(field->fnSet.f))(f);
			}
			else
			{
				this->*((float ThisClass::*)field->pField) = f;
			}
			return(true);
		}
		return(false);
	case PDF_VECTOR:
		if(3 == sscanf(value, "%f %f %f", &f3.x, &f3.y, &f3.z))
		{
			if(field->fnSet.v3)
			{
				(this->*(field->fnSet.v3))(f3);
			}
			else
			{
				this->*((float3_t ThisClass::*)field->pField) = f3;
			}
			return(true);
		}
		return(false);
	case PDF_VECTOR4:
		{
			int iPrm = sscanf(value, "%f %f %f %f", &f4.x, &f4.y, &f4.z, &f4.w);
			if(iPrm > 2)
			{
				if(iPrm == 3)
				{
					f4.w = 1.0f;
				}
				if(field->fnSet.v4)
				{
					(this->*(field->fnSet.v4))(f4);
				}
				else
				{
					this->*((float4_t ThisClass::*)field->pField) = f4;
				}
				return(true);
			}
		}
		return(false);
	case PDF_BOOL:
		if(1 == sscanf(value, "%d", &d))
		{
			if(field->fnSet.b)
			{
				(this->*(field->fnSet.b))(d != 0);
			}
			else
			{
				this->*((bool ThisClass::*)field->pField) = d != 0;
			}
			return(true);
		}
		return(false);
	case PDF_STRING:
		if(field->fnSet.sz)
		{
			(this->*(field->fnSet.sz))(value);
		}
		else
		{
			_setStrVal(&(this->*((const char * ThisClass::*)field->pField)), value);
		}
		break;
	case PDF_ANGLES:
		if(4 == sscanf(value, "%f %f %f %f", &q.x, &q.y, &q.z, &q.w))
		{
			if(field->fnSet.q)
			{
				(this->*(field->fnSet.q))(q);
			}
			else
			{
				this->*((SMQuaternion ThisClass::*)field->pField) = q;
			}
			return(true);
		}
		if(3 == sscanf(value, "%f %f %f", &f3.x, &f3.y, &f3.z))
		{
			SMQuaternion q;
			q = SMQuaternion(SMToRadian(f3.x), 'x') * SMQuaternion(SMToRadian(f3.y), 'y') * SMQuaternion(SMToRadian(f3.z), 'z');
			if(field->fnSet.q)
			{
				(this->*(field->fnSet.q))(q);
			}
			else
			{
				this->*((SMQuaternion ThisClass::*)field->pField) = q;
			}
			return(true);
		}
		return(false);
	case PDF_ENTITY:
		(this->*((CEntityPointer<CBaseEntity> ThisClass::*)field->pField)).setEntityName(value);
		return(true);
	case PDF_FLAGS:
		if(1 == sscanf(value, "%d", &d))
		{
			UINT * f = &(this->*((UINT ThisClass::*)field->pField));
			*f &= 0xFFFF;
			*f |= d & 0xFFFF0000;
			updateFlags();
			return(true);
		}
		return(false);
	case PDF_OUTPUT:
		{
			size_t len = sizeof(char)* (1 + strlen(value));
			char * str = new char[len];
			memcpy(str, value, len);
			int iConns = parse_str(str, NULL, 0, ',');
			char ** parts = (char**)alloca(sizeof(char*)*iConns);
			iConns = parse_str(str, parts, iConns, ',');

			output_t *pOutput = &(this->*((output_t ThisClass::*)field->pField));
			{
				char *pTmpData = (char*)pOutput->pData;
				mem_delete_a(pTmpData);
			}
			pOutput->pData = str;
			pOutput->aOutputs.clearFast();
			pOutput->aOutputs.reserve(iConns);

			int curr = 0;
			for(int i = 0; i < iConns; ++i)
			{
				char *fields[5];
				int iParts = parse_str(parts[i], fields, 5, ':');
				char *param = NULL;
				if(iParts != 5 && iParts != 4 && iParts != 3)
				{
					printf(COLOR_LRED "Unable to parse output '%s' ent %s\n" COLOR_RESET, name, m_szName);
					continue;
				}
				if(iParts >= 4 && fstrcmp(fields[3], ENT_OUTPUT_PARAM_NONE))
				{
					param = fields[3];
				}
				int iFireLimit = -1;
				if(iParts >= 5)
				{
					sscanf(fields[4], "%d", &iFireLimit);
				}
				float fDelayFrom;
				float fDelayTo;
				switch(sscanf(fields[2], "%f-%f", &fDelayFrom, &fDelayTo))
				{
				case 1:
					fDelayTo = fDelayFrom;
					break;
				case 2:
					break;
				default:
					printf(COLOR_LRED "Unable to parse output delay '%s' ent %s\n" COLOR_RESET, name, m_szName);
					continue;
				}
				named_output_t &out = pOutput->aOutputs[curr];

				out.fDelay = fDelayFrom;
				out.fDelayTo = fDelayTo;
				if(fDelayFrom < fDelayTo)
				{
					out.useRandomDelay = true;
				}
				if(fDelayFrom > fDelayTo)
				{
					printf(COLOR_LRED "Unable to parse output delay '%s' ent %s. min delay cannot be more than max delay\n" COLOR_RESET, name, m_szName);
					continue;
				}

				out.szTargetName = fields[0];
				out.szTargetInput = fields[1];
				out.szTargetData = param;
				out.iFireLimit = iFireLimit;
				
				out.init(this);

				++curr;
			}
		}
		// target_name:input:delay:parameter\n<repeat>
		return(false);
	default:
		return(false);
	}
	return(true);
}
bool CBaseEntity::getKV(const char * name, char * out, int bufsize)
{
	propdata_t *field = getField(name);
	if(!field)
	{
		return(false);
	}
	SMQuaternion q;
	float3_t f3;
	switch(field->type)
	{
	case PDF_INT:
		sprintf_s(out, bufsize, "%d", this->*((int ThisClass::*)field->pField));
		break;
	case PDF_UINT:
		sprintf_s(out, bufsize, "%u", this->*((int ThisClass::*)field->pField));
		break;
	case PDF_FLAGS:
		sprintf_s(out, bufsize, "%u", this->*((int ThisClass::*)field->pField) & 0xFFFF0000);
		break;
	case PDF_FLOAT:
		sprintf_s(out, bufsize, "%f", this->*((float ThisClass::*)field->pField));
		break;
	case PDF_VECTOR:
		sprintf_s(out, bufsize, "%f %f %f", (this->*((float3_t ThisClass::*)field->pField)).x, (this->*((float3_t CBaseEntity::*)field->pField)).y, (this->*((float3_t CBaseEntity::*)field->pField)).z);
		break;
	case PDF_VECTOR4:
		sprintf_s(out, bufsize, "%f %f %f %f", (this->*((float4_t ThisClass::*)field->pField)).x, (this->*((float4_t CBaseEntity::*)field->pField)).y, (this->*((float4_t CBaseEntity::*)field->pField)).z, (this->*((float4_t CBaseEntity::*)field->pField)).w);
		break;
	case PDF_BOOL:
		sprintf_s(out, bufsize, "%d", this->*((bool ThisClass::*)field->pField) ? 1 : 0);
		break;
	case PDF_STRING:
		sprintf_s(out, bufsize, "%s", this->*((const char* ThisClass::*)field->pField));
		break;
	case PDF_ANGLES:
		q = this->*((SMQuaternion ThisClass::*)field->pField);
		//f3 = SMMatrixToEuler(q.GetMatrix());
		sprintf_s(out, bufsize, "%f %f %f %f", q.x, q.y, q.z, q.w);
		break;
	case PDF_ENTITY:
		(this->*((CEntityPointer<CBaseEntity> ThisClass::*)field->pField)).getEntityName(out, bufsize);
		break;
	case PDF_OUTPUT:
		{
			output_t *pOutput = &(this->*((output_t ThisClass::*)field->pField));
			int iWritten = 0;
			char * szOutBuf = out;
			*out = 0;
			if(pOutput->aOutputs.size() == 0)
			{
				*out = 0;
			}
			for(UINT i = 0, l = pOutput->aOutputs.size(); i < l; ++i)
			{
				named_output_t &out = pOutput->aOutputs[i];
				if(i > 0)
				{
					*szOutBuf = ',';
					++szOutBuf;
				}
				int c;
				if(out.useRandomDelay)
				{
					c = _snprintf(szOutBuf, bufsize - iWritten, "%s:%s:%f-%f:%s:%d", 
						out.szTargetName, 
						out.szTargetInput, 
						out.fDelay,
						out.fDelayTo,
						out.szTargetData ? out.szTargetData : ENT_OUTPUT_PARAM_NONE,
						out.iFireLimit
						);
				}
				else
				{
					c = _snprintf(szOutBuf, bufsize - iWritten, "%s:%s:%f:%s:%d", 
						out.szTargetName, 
						out.szTargetInput, 
						out.fDelay, 
						out.szTargetData ? out.szTargetData : ENT_OUTPUT_PARAM_NONE,
						out.iFireLimit
						);
				}
				iWritten += c + 1;
				szOutBuf += c;
				//iBufSize += 1 + _snprintf(NULL, 0, "%s:%s:%f:%s", pOutput->pOutputs[i].szTargetName, pOutput->pOutputs[i].szTargetInput, pOutput->pOutputs[i].fDelay, pOutput->pOutputs[i].szTargetData ? pOutput->pOutputs[i].szTargetData : "");
			}
		}
		break;
	default:
		return(false);
	}
	return(true);
}

void CBaseEntity::onParentSet(CBaseEntity *pNewParent)
{
	if(pNewParent)
	{
		onParentMoved(true);
		
		pNewParent->addChild(this);
	}
}

void CBaseEntity::onParentUnset(CBaseEntity *pOldParent)
{
	if(pOldParent)
	{
		pOldParent->removeChild(this);
	}
}

void CBaseEntity::addChild(CBaseEntity *pEnt)
{
	ScopedSpinLock lock(m_slChildren);
	m_aChildren.push_back(pEnt);
}
void CBaseEntity::removeChild(CBaseEntity *pEnt)
{
	ScopedSpinLock lock(m_slChildren);
	int idx = m_aChildren.indexOf(pEnt);
	assert(idx >= 0);
	if(idx >= 0)
	{
		m_aChildren.erase(idx);
	}
}

void CBaseEntity::setParent(CBaseEntity *pEnt, int attachment)
{
	m_pParent = pEnt;
	if(pEnt)
	{
		m_iParentAttachment = attachment;
	}
}

CBaseEntity* CBaseEntity::getParent()
{
	return(m_pParent);
}

/*void CBaseEntity::updateDiscreteLinearVelocity(int step, float dt)
{
	if(step == 0)
	{
		m_vDiscreteLinearVelocity = (float3)((m_vPosition - m_vOldPosition) / dt);
		m_vOldPosition = m_vPosition;
	}
	return;
	if(m_pParent || 1)
	{
		if(step) // post sync
		{
			m_vDiscreteLinearVelocity = (float3)((m_vPosition - m_vOldPosition) / dt);
		}
		else // pre sync
		{
			m_vOldPosition = m_vPosition;
		}
	}
	else
	{
		if(step) // post sync
		{
			m_vOldPosition = m_vPosition;
		}
		else // pre sync
		{
			m_vDiscreteLinearVelocity = (float3)((m_vPosition - m_vOldPosition) / dt);
		}
	}
}*/

void CBaseEntity::onPostLoad()
{
	updateFlags();
}

float3 CBaseEntity::getAttachmentPos(int id)
{
	return(float3());
}

SMQuaternion CBaseEntity::getAttachmentRot(int id)
{
	return(SMQuaternion());
}

void CBaseEntity::setOwner(CBaseEntity * pEnt)
{
	m_pOwner = pEnt;
}

CBaseEntity * CBaseEntity::getOwner()
{
	return(m_pOwner);
}

CEntityManager * CBaseEntity::getManager()
{
	return(m_pMgr);
}

/*const float3_t & CBaseEntity::getDiscreteLinearVelocity() const
{
	return(m_vDiscreteLinearVelocity);
}*/

CBaseEntity *CBaseEntity::getEntByName(const char *szName, CBaseEntity *pStartFrom)
{
	if(!strcmp(szName, ENT_SPECIAL_NAME_THIS))
	{
		return(pStartFrom ? NULL : this);
	}
	if(!strcmp(szName, ENT_SPECIAL_NAME_PARENT))
	{
		return(pStartFrom ? NULL : getParent());
	}

	return(m_pMgr->findEntityByName(szName, pStartFrom));
}

int CBaseEntity::countEntByName(const char *szName)
{
	if(!strcmp(szName, ENT_SPECIAL_NAME_THIS))
	{
		return(1);
	}
	if(!strcmp(szName, ENT_SPECIAL_NAME_PARENT))
	{
		return(getParent() ? 1 : 0);
	}

	return(m_pMgr->countEntityByName(szName));
}

void CBaseEntity::dispatchDamage(CTakeDamageInfo &takeDamageInfo)
{
	float fHealth = takeDamageInfo.m_fDamage * 0.1f;
	if(takeDamageInfo.m_pInflictor)
	{
		LibReport(REPORT_MSG_LEVEL_NOTICE, "%s damaged (" COLOR_LRED "%.2f" COLOR_RESET ") by " COLOR_YELLOW "%s\n", getClassName(), fHealth, takeDamageInfo.m_pInflictor->getClassName());
	}
	else
	{
		LibReport(REPORT_MSG_LEVEL_NOTICE, "%s damaged (" COLOR_LRED "%.2f" COLOR_RESET ")\n", getClassName(), fHealth);
	}
	takeHealth(fHealth, takeDamageInfo.m_pAttacker, takeDamageInfo.m_pInflictor);
}

void CBaseEntity::takeHealth(float fVal, CBaseEntity *pAttacker, CBaseEntity *pInflictor)
{
	if(m_fHealth <= 0.0f)
	{
		return;
	}
	m_fHealth -= fVal;
	if(m_fHealth <= 0.0f)
	{
		if(!pInflictor)
		{
			pInflictor = pAttacker;
		}
		onDeath(pAttacker, pInflictor);
	}
}

void CBaseEntity::onDeath(CBaseEntity *pAttacker, CBaseEntity *pInflictor)
{
	LibReport(REPORT_MSG_LEVEL_NOTICE, "Entity %s died!\n", getClassName());
	// do nothing
}

void CBaseEntity::broadcastMessage(const char * szInputName, float fArg, float fRadius)
{
	inputdata_t inputData;
	memset(&inputData, 0, sizeof(inputData));
	inputData.type = PDF_FLOAT;
	inputData.parameter.f = fArg;
	broadcastMessage(szInputName, inputData, fRadius);
}

void CBaseEntity::broadcastMessage(const char * szInputName, int iArg, float fRadius)
{
	inputdata_t inputData;
	memset(&inputData, 0, sizeof(inputData));
	inputData.type = PDF_INT;
	inputData.parameter.i = iArg;
	broadcastMessage(szInputName, inputData, fRadius);
}
void CBaseEntity::broadcastMessage(const char * szInputName, bool bArg, float fRadius)
{
	inputdata_t inputData;
	memset(&inputData, 0, sizeof(inputData));
	inputData.type = PDF_BOOL;
	inputData.parameter.b = bArg;
	broadcastMessage(szInputName, inputData, fRadius);
}
void CBaseEntity::broadcastMessage(const char * szInputName, const char *szArg, float fRadius)
{
	inputdata_t inputData;
	memset(&inputData, 0, sizeof(inputData));
	inputData.type = PDF_STRING;
	inputData.parameter.str = szArg;
	broadcastMessage(szInputName, inputData, fRadius);
}
void CBaseEntity::broadcastMessage(const char * szInputName, const float3_t &f3Arg, float fRadius)
{
	inputdata_t inputData;
	memset(&inputData, 0, sizeof(inputData));
	inputData.type = PDF_VECTOR;
	inputData.v3Parameter = f3Arg;
	broadcastMessage(szInputName, inputData, fRadius);
}
void CBaseEntity::broadcastMessage(const char * szInputName, float fRadius)
{
	inputdata_t inputData;
	memset(&inputData, 0, sizeof(inputData));
	inputData.type = PDF_NONE;
	broadcastMessage(szInputName, inputData, fRadius);
}

void CBaseEntity::broadcastMessage(const char * szInputName, inputdata_t inputData, float fRadius)
{
	inputData.pActivator = this;
	inputData.pInflictor = this;

	CBaseEntity *pEnt = NULL;
	while((pEnt = m_pMgr->findEntityInSphere(getPos(), fRadius, pEnt)))
	{
		if(pEnt == this) continue;
		propdata_t * pField = pEnt->getField(szInputName);
		if(pField && (pField->flags & PDFF_MESSAGE))
		{
			(pEnt->*(pField->fnInput))(&inputData);
		}
	}
}

void CBaseEntity::_cleanup()
{
}

void CBaseEntity::onUse(CBaseEntity *pUser)
{
}

void CBaseEntity::renderEditor(bool is3D, bool bRenderSelection, IXGizmoRenderer *pRenderer)
{

}

void CBaseEntity::registerPointer(IEntityPointer *pPtr)
{
	ScopedSpinLock lock(m_slPointers);
	m_aPointers.push_back(pPtr);
}

void CBaseEntity::unregisterPointer(IEntityPointer *pPtr)
{
	ScopedSpinLock lock(m_slPointers);
	int idx = m_aPointers.indexOf(pPtr);
	if(idx >= 0)
	{
		m_aPointers.erase(idx);
	}
}

void CBaseEntity::notifyPointers()
{
	ScopedSpinLock lock(m_slPointers);
	for(UINT i = 0, l = m_aPointers.size(); i < l; ++i)
	{
		m_aPointers[i]->onTargetRemoved(this);
	}
}

void CBaseEntity::setName(const char *szName)
{
	if(fstrcmp(m_szName, szName))
	{
		m_pMgr->onEntityNameChanged(this, m_szName, szName);
		_setStrVal(&m_szName, szName);
	}
}
