#include "FuncLadder.h"
#include "BaseCharacter.h"
#include "LadderMovementController.h"


BEGIN_PROPTABLE(CInfoLadderDismount)
	// empty
END_PROPTABLE()

REGISTER_ENTITY(CInfoLadderDismount, info_ladder_dismount);

//##########################################################################

BEGIN_PROPTABLE(CFuncLadder)
	DEFINE_FIELD_VECTORFN(m_vUpPoint, PDFF_USE_GIZMO, "up_point", "Up point", setUpPoint, EDITOR_POINTCOORD)

	//! Плеер залез на лестницу
	DEFINE_OUTPUT(m_onPlayerGetOn, "OnPlayerGetOn", "On player get on")
	//! Плеер слез с лестницы
	DEFINE_OUTPUT(m_onPlayerGetOff, "OnPlayerGetOff", "On player get off")

	//! Включает лестницу
	DEFINE_INPUT(turnOn, "turnOn", "Turn on", PDF_NONE)
	//! Выключает лестницу
	DEFINE_INPUT(turnOff, "turnOff", "Turn off", PDF_NONE)
	//! Переключает состояние лестницы
	DEFINE_INPUT(toggle, "toggle", "Toggle", PDF_NONE)

	//! Изначально выключена
	DEFINE_FLAG(LADDER_INITIALLY_DISABLED, "Start disabled")
END_PROPTABLE()

REGISTER_ENTITY(CFuncLadder, func_ladder);

IEventChannel<XEventPhysicsStep> *CFuncLadder::m_pTickEventChannel = NULL;

CFuncLadder::CFuncLadder():
	m_physicsTicker(this)
{
	if(!m_pTickEventChannel)
	{
		m_pTickEventChannel = Core_GetIXCore()->getEventChannel<XEventPhysicsStep>(EVENT_PHYSICS_STEP_GUID);
	}
}

CFuncLadder::~CFuncLadder()
{
	disable();
	mem_release(m_pCollideShape);
	mem_release(m_pGhostObject);
}

void CFuncLadder::setUpPoint(const float3 &vUp)
{
	m_isUpSet = true;
	m_vUpPoint = vUp;

	initPhysics();
}

void CFuncLadder::createPhysBody()
{
	if(!m_pGhostObject)
	{
		GetPhysics()->newGhostObject(&m_pGhostObject);
		m_pGhostObject->setPosition(getPos());
		m_pGhostObject->setUserPointer(this);
		m_pGhostObject->setUserTypeId(1);
		m_pGhostObject->setCollisionFlags(m_pGhostObject->getCollisionFlags() ^ XCF_NO_CONTACT_RESPONSE);
		m_pGhostObject->setCollisionShape(m_pCollideShape);
		if(m_isEnabled)
		{
			GetPhysWorld()->addCollisionObject(m_pGhostObject, CG_LADDER, CG_CHARACTER);
		}
	}
	else
	{
		m_pGhostObject->setCollisionShape(m_pCollideShape);
	}
}

void CFuncLadder::setPos(const float3 &pos)
{
	BaseClass::setPos(pos);
	if(!m_isUpSet)
	{
		m_vUpPoint = (float3)(pos + float3(0.0f, 2.0f, 0.0f));
		initPhysics();
	}
	SAFE_CALL(m_pGhostObject, setPosition, pos);
}

void CFuncLadder::updateFlags()
{
	BaseClass::updateFlags();

	if(getFlags() & LADDER_INITIALLY_DISABLED)
	{
		disable();
	}
	else
	{
		enable();
	}
}

void CFuncLadder::turnOn(inputdata_t *pInputdata)
{
	enable();
}

void CFuncLadder::turnOff(inputdata_t *pInputdata)
{
	disable();
}

void CFuncLadder::toggle(inputdata_t *pInputdata)
{
	if(m_isEnabled)
	{
		turnOff(pInputdata);
	}
	else
	{
		turnOn(pInputdata);
	}
}

void CFuncLadder::initPhysics()
{
	//TODO: сделать обработку ситуации когда m_vUpPoint ниже getPos
	mem_release(m_pCollideShape);

	float3 vDelta = m_vUpPoint - getPos();
	SMAABB aabb = getBound();
	float3 vMinDelta, vMaxDelta;

	float3_t aPointsBot[] = { 
		{aabb.vMin.x, aabb.vMax.y, aabb.vMin.z }, 
		{aabb.vMax.x, aabb.vMax.y, aabb.vMin.z },
		{aabb.vMin.x, aabb.vMax.y, aabb.vMax.z },
		{aabb.vMax.x, aabb.vMax.y, aabb.vMax.z }
	};

	vMinDelta = aabb.vMin + vDelta;
	vMaxDelta = aabb.vMax + vDelta;

	float3_t aPointsTop[] = {
		{ vMinDelta.x, vMinDelta.y, vMinDelta.z },
		{ vMaxDelta.x, vMinDelta.y, vMinDelta.z },
		{ vMinDelta.x, vMinDelta.y, vMaxDelta.z },
		{ vMaxDelta.x, vMinDelta.y, vMaxDelta.z }
	};

	const UINT c_uSize = 14;
	UINT uIndex = 0;
	float3_t aShapePoints[c_uSize] = {};

	if(vDelta.x > 0.0f || vDelta.z > 0.0f)
	{
		aShapePoints[uIndex++] = aPointsBot[0];
		aShapePoints[uIndex++] = aPointsTop[3];
	}
	if(vDelta.x < 0.0f || vDelta.z < 0.0f)
	{
		aShapePoints[uIndex++] = aPointsBot[3];
		aShapePoints[uIndex++] = aPointsTop[0];
	}
	if(vDelta.x < 0.0f || vDelta.z > 0.0f)
	{
		aShapePoints[uIndex++] = aPointsBot[1];
		aShapePoints[uIndex++] = aPointsTop[2];
	}
	if(vDelta.x > 0.0f || vDelta.z < 0.0f)
	{
		aShapePoints[uIndex++] = aPointsBot[2];
		aShapePoints[uIndex++] = aPointsTop[1];
	}

	aShapePoints[uIndex++] = aabb.vMin;
	aShapePoints[uIndex++] = {aabb.vMin.x, aabb.vMin.y, aabb.vMax.z};
	aShapePoints[uIndex++] = {aabb.vMax.x, aabb.vMin.y, aabb.vMax.z};
	aShapePoints[uIndex++] = {aabb.vMax.x, aabb.vMin.y, aabb.vMin.z};

	aShapePoints[uIndex++] = vMaxDelta;
	aShapePoints[uIndex++] = {vMaxDelta.x, vMaxDelta.y, vMinDelta.z};
	aShapePoints[uIndex++] = {vMinDelta.x, vMaxDelta.y, vMinDelta.z};
	aShapePoints[uIndex++] = {vMinDelta.x, vMaxDelta.y, vMaxDelta.z};

	GetPhysics()->newConvexHullShape(uIndex, aShapePoints, &m_pCollideShape, sizeof(float3_t), false);
	createPhysBody();
}

void CFuncLadder::renderEditor(bool is3D, bool bRenderSelection, IXGizmoRenderer *pRenderer)
{
	if(pRenderer)
	{
		const float4 c_vLineColor = bRenderSelection ? float4(1.0f, 0.0f, 0.0f, 1.0f) : float4(1.0f, 0.0f, 1.0f, 1.0f);
		pRenderer->setColor(c_vLineColor);
		pRenderer->setLineWidth(is3D ? 0.02f : 1.0f);

		SMAABB aabb = getBound();
		pRenderer->drawAABB(aabb + getPos());
		pRenderer->drawAABB(aabb + m_vUpPoint);
		pRenderer->jumpTo(getPos());
		pRenderer->lineTo(m_vUpPoint);
	}
}

void CFuncLadder::getMinMax(float3 *min, float3 *max)
{
	SMAABB aabb = getBound();
	aabb = SMAABBConvex(aabb, aabb + (getUpPos() - getPos()));

	if(min)
	{
		*min = aabb.vMin;
	}

	if(max)
	{
		*max = aabb.vMax;
	}
}

SMAABB CFuncLadder::getBound()
{
	return(SMAABB(float3(-0.25f, 0.0f, -0.25f), float3(0.25f, 1.8f, 0.25f)));
}

bool SMAABBIntersectLine(const SMAABB &aabb, const float3 &vStart, const float3 &vEnd, float3 *pvOut, float3 *pvNormal)
{
	float min_t = 0.0f;
    float max_t = 1.0f;

	float3 vDir = vEnd - vStart;

	for(int i = 0; i < 3; ++i)
	{
		if(SMIsZero(vDir[i]))
		{
			if(vStart[i] < aabb.vMin[i] || vStart[i] > aabb.vMax[i])
			{
				return(false);
			}
		}

		float t1 = (aabb.vMin[i] - vStart[i]) / vDir[i];
		float t2 = (aabb.vMax[i] - vStart[i]) / vDir[i];

		float tmin = min(t1, t2);
		float tmax = max(t1, t2);

		min_t = max(min_t, tmin);
		max_t = min(max_t, tmax);

		if(min_t > max_t)
		{
			return(false);
		}
	}

	if(pvOut)
	{
		*pvOut = vStart + min_t * vDir;
	}

	if(pvNormal)
	{
		*pvNormal = 0.0f;

		if(SMIsZero(aabb.vMin.x - pvOut->x))
		{
			pvNormal->x = -1.0f;
		}
		else if(SMIsZero(aabb.vMax.x - pvOut->x))
		{
			pvNormal->x = 1.0f;
		}
		else if(SMIsZero(aabb.vMin.y - pvOut->y))
		{
			pvNormal->y = -1.0f;
		}
		else if(SMIsZero(aabb.vMax.y - pvOut->y))
		{
			pvNormal->y = 1.0f;
		}
		else if(SMIsZero(aabb.vMin.z - pvOut->z))
		{
			pvNormal->z = -1.0f;
		}
		else
		{
			pvNormal->z = 1.0f;
		}
	}
}

bool CFuncLadder::rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut, float3 *pvNormal, bool isRayInWorldSpace, bool bReturnNearestPoint)
{
	assert(isRayInWorldSpace);

	SMAABB aabb = getBound();

	if(bReturnNearestPoint)
	{
		bool b0, b1;
		float3 vPos0, vPos1;
		float3 vNormal0, vNormal1;
		float3 *pvNormal0 = NULL, *pvNormal1 = NULL;
		if(pvNormal)
		{
			pvNormal0 = &vNormal0;
			pvNormal1 = &vNormal1;
		}

		b0 = SMAABBIntersectLine(aabb + getPos(), vStart, vEnd, &vPos0, pvNormal0);
		b1 = SMAABBIntersectLine(aabb + getUpPos(), vStart, vEnd, &vPos1, pvNormal1);

		if(b0 && b1)
		{
			if(SMVector3Length2(vPos0 - vStart) > SMVector3Length2(vPos1 - vStart))
			{
				b0 = false;
			}
			else
			{
				b1 = false;
			}
		}

		if(b0)
		{
			*pvOut = vPos0;
			if(pvNormal)
			{
				*pvNormal = vNormal0;
			}
		}
		else if(b1)
		{
			*pvOut = vPos1;
			if(pvNormal)
			{
				*pvNormal = vNormal1;
			}
		}

		return(b0 || b1);
	}
	else
	{
		if(SMAABBIntersectLine(aabb + getPos(), vStart, vEnd, pvOut, pvNormal) || SMAABBIntersectLine(aabb + getUpPos(), vStart, vEnd, pvOut, pvNormal))
		{
			return(true);
		}
	}
	
	return(false);
}

void CFuncLadder::onUse(CBaseEntity *pUser)
{
	connectToLadder(pUser);
	BaseClass::onUse(pUser);
}

float3 CFuncLadder::getUpPos()
{
	return(m_vUpPoint);
}

void CFuncLadder::connectToLadder(CBaseEntity *pEntity)
{
	if(fstrcmp(pEntity->getClassName(), "player"))
	{
		return;
	}
	CBaseCharacter *pCharacter = (CBaseCharacter*)pEntity;
	CLadderMovementController *pController = new CLadderMovementController(this);
	pCharacter->setMovementController(pController);
	mem_release(pController);
}

void CFuncLadder::onPhysicsStep()
{
	if(!m_pGhostObject || !m_isEnabled)
	{
		return;
	}

	Array<CBaseEntity*> aCurrentTouchingEntities(m_aTouchedEntities.size());

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
						aCurrentTouchingEntities.push_back(pEnt);
						if(m_aTouchedEntities.indexOf(pEnt) < 0)
						{
							connectToLadder(pEnt);
							//printf("touched %s\n", pEnt->getClassName());
						}
					}
				}
				break;
			}
		}
	}

	m_aTouchedEntities = aCurrentTouchingEntities;
}


void CFuncLadder::enable()
{
	if(!m_isEnabled)
	{
		if(m_pGhostObject)
		{
			GetPhysWorld()->addCollisionObject(m_pGhostObject, CG_LADDER, CG_CHARACTER);
		}
		m_pTickEventChannel->addListener(&m_physicsTicker);
		m_isEnabled = true;
	}
}

void CFuncLadder::disable()
{
	if(m_isEnabled)
	{
		if(m_pGhostObject)
		{
			GetPhysWorld()->removeCollisionObject(m_pGhostObject);
		}
		m_pTickEventChannel->removeListener(&m_physicsTicker);
		m_isEnabled = false;
	}
}

void CPhysicsLadderTickEventListener::onEvent(const XEventPhysicsStep *pData)
{
	m_pLadder->onPhysicsStep();
}
