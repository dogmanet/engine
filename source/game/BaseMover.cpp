#include "BaseMover.h"
#include "BaseCharacter.h"

TODO("Trigger OnPlayerGetOn/OnPlayerGetOff events");
TODO("Handle MOVER_NO_AUTOMOUNT flag");

BEGIN_PROPTABLE(CBaseMover)
	DEFINE_FIELD_VECTORFN(m_vUpPoint, PDFF_USE_GIZMO, "up_point", "Up point", setUpPoint, EDITOR_POINTCOORD)

	//! Игрок присоединился
	DEFINE_OUTPUT(m_onPlayerGetOn, "OnPlayerGetOn", "On player get on")
	//! Игрок отсоединился
	DEFINE_OUTPUT(m_onPlayerGetOff, "OnPlayerGetOff", "On player get off")

	//! Включает
	DEFINE_INPUT(turnOn, "turnOn", "Turn on", PDF_NONE)
	//! Выключает
	DEFINE_INPUT(turnOff, "turnOff", "Turn off", PDF_NONE)
	//! Переключает состояние
	DEFINE_INPUT(toggle, "toggle", "Toggle", PDF_NONE)

	//! Изначально выключена
	DEFINE_FLAG(MOVER_INITIALLY_DISABLED, "Start disabled")
	DEFINE_FLAG(MOVER_NO_AUTOMOUNT, "Disable automount")
END_PROPTABLE()

REGISTER_ENTITY(CBaseMover, base_mover);

IEventChannel<XEventPhysicsStep> *CBaseMover::m_pTickEventChannel = NULL;

CBaseMover::CBaseMover():
	m_physicsTicker(this)
{
	if(!m_pTickEventChannel)
	{
		m_pTickEventChannel = Core_GetIXCore()->getEventChannel<XEventPhysicsStep>(EVENT_PHYSICS_STEP_GUID);
	}
}

CBaseMover::~CBaseMover()
{
	disable();
	mem_release(m_pCollideShape);
	mem_release(m_pGhostObject);
}

void CBaseMover::setUpPoint(const float3 &vUp)
{
	m_vUpPoint = vUp;

	initPhysics();
}

void CBaseMover::createPhysBody()
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

void CBaseMover::setPos(const float3 &pos)
{
	BaseClass::setPos(pos);
	initPhysics();
	SAFE_CALL(m_pGhostObject, setPosition, pos);
}

void CBaseMover::updateFlags()
{
	BaseClass::updateFlags();

	if(getFlags() & MOVER_INITIALLY_DISABLED)
	{
		disable();
	}
	else
	{
		enable();
	}
}

void CBaseMover::turnOn(inputdata_t *pInputdata)
{
	enable();
}

void CBaseMover::turnOff(inputdata_t *pInputdata)
{
	disable();
}

void CBaseMover::toggle(inputdata_t *pInputdata)
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

void CBaseMover::initPhysics()
{
	//TODO: сделать обработку ситуации когда m_vUpPoint ниже getPos
	mem_release(m_pCollideShape);

	float3 vDelta = getOrient() * m_vUpPoint;
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

void CBaseMover::renderEditor(bool is3D, bool bRenderSelection, IXGizmoRenderer *pRenderer)
{
	if(pRenderer)
	{
		const float4 c_vLineColor = bRenderSelection ? float4(1.0f, 0.0f, 0.0f, 1.0f) : float4(1.0f, 0.0f, 1.0f, 1.0f);
		pRenderer->setColor(c_vLineColor);
		pRenderer->setLineWidth(is3D ? 0.02f : 1.0f);

		SMAABB aabb = getBound();
		pRenderer->drawAABB(aabb + getPos());
		pRenderer->drawAABB(aabb + getUpPos());
		pRenderer->jumpTo(getPos());
		pRenderer->lineTo(getUpPos());
	}
}

void CBaseMover::getMinMax(float3 *min, float3 *max)
{
	SMAABB aabb = getBound();
	aabb = SMAABBConvex(aabb, aabb + getUpPos() - getPos());

	if(min)
	{
		*min = aabb.vMin;
	}

	if(max)
	{
		*max = aabb.vMax;
	}
}

SMAABB CBaseMover::getBound()
{
	return(SMAABB(float3(-0.25f, 0.0f, -0.25f), float3(0.25f, 1.8f, 0.25f)));
}

bool SMAABBIntersectLine(const SMAABB &aabb, const float3 &vStart, const float3 &vEnd, float3 *pvOut, float3 *pvNormal)
{
	float3 vPoint;
	SMPLANE plane;

	plane = SMPLANE(float3(1.0f, 0.0f, 0.0f), -aabb.vMax.x);
	if(plane.intersectLine(&vPoint, vStart, vEnd) && SMIsAABBInsideAABB(SMAABB(vPoint, vPoint), aabb))
	{
		*pvOut = vPoint;
		if(pvNormal)
		{
			*pvNormal = float3(1.0f, 0.0f, 0.0f);
		}
		return(true);
	}
	
	plane = SMPLANE(float3(-1.0f, 0.0f, 0.0f), aabb.vMin.x);
	if(plane.intersectLine(&vPoint, vStart, vEnd) && SMIsAABBInsideAABB(SMAABB(vPoint, vPoint), aabb))
	{
		*pvOut = vPoint;
		if(pvNormal)
		{
			*pvNormal = float3(-1.0f, 0.0f, 0.0f);
		}
		return(true);
	}

	plane = SMPLANE(float3(0.0f, 1.0f, 0.0f), -aabb.vMax.y);
	if(plane.intersectLine(&vPoint, vStart, vEnd) && SMIsAABBInsideAABB(SMAABB(vPoint, vPoint), aabb))
	{
		*pvOut = vPoint;
		if(pvNormal)
		{
			*pvNormal = float3(0.0f, 1.0f, 0.0f);
		}
		return(true);
	}

	plane = SMPLANE(float3(0.0f, -1.0f, 0.0f), aabb.vMin.y);
	if(plane.intersectLine(&vPoint, vStart, vEnd) && SMIsAABBInsideAABB(SMAABB(vPoint, vPoint), aabb))
	{
		*pvOut = vPoint;
		if(pvNormal)
		{
			*pvNormal = float3(0.0f, -1.0f, 0.0f);
		}
		return(true);
	}

	plane = SMPLANE(float3(0.0f, 0.0f, 1.0f), -aabb.vMax.z);
	if(plane.intersectLine(&vPoint, vStart, vEnd) && SMIsAABBInsideAABB(SMAABB(vPoint, vPoint), aabb))
	{
		*pvOut = vPoint;
		if(pvNormal)
		{
			*pvNormal = float3(0.0f, 0.0f, 1.0f);
		}
		return(true);
	}

	plane = SMPLANE(float3(0.0f, 0.0f, -1.0f), aabb.vMin.z);
	if(plane.intersectLine(&vPoint, vStart, vEnd) && SMIsAABBInsideAABB(SMAABB(vPoint, vPoint), aabb))
	{
		*pvOut = vPoint;
		if(pvNormal)
		{
			*pvNormal = float3(0.0f, 0.0f, -1.0f);
		}
		return(true);
	}

	return(false);
}

bool CBaseMover::rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut, float3 *pvNormal, bool isRayInWorldSpace, bool bReturnNearestPoint)
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

void CBaseMover::onUse(CBaseEntity *pUser)
{
	handleCharacterMount(pUser);
	BaseClass::onUse(pUser);
}

float3 CBaseMover::getUpPos()
{
	return(getOrient() * m_vUpPoint + getPos());
}

void CBaseMover::handleCharacterMount(CBaseEntity *pEntity)
{
	if(fstrcmp(pEntity->getClassName(), "player"))
	{
		return;
	}
	CBaseCharacter *pCharacter = (CBaseCharacter*)pEntity;

	IMovementController *pController = NULL;
	newMovementController(&pController);

	if(pController)
	{
		pCharacter->setMovementController(pController);
		mem_release(pController);
	}
}

void CBaseMover::onPhysicsStep()
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
							handleCharacterMount(pEnt);
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


void CBaseMover::enable()
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

void CBaseMover::disable()
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

void CBaseMover::newMovementController(IMovementController **ppOut)
{
	*ppOut = NULL;
}

//##########################################################################

void CPhysicsLadderTickEventListener::onEvent(const XEventPhysicsStep *pData)
{
	m_pMover->onPhysicsStep();
}
