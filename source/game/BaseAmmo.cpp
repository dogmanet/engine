
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "BaseAmmo.h"
#include "Tracer.h"
#include "BaseTool.h"
#include <particles/sxparticles.h>
#include <mtrl/IXMaterial.h>
#include <decals/sxdecals.h>
//#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>

//! TODO Reimplement me!
#define SMtrl_MtlGetPhysicMaterial(id) MTLTYPE_PHYSIC_METAL
#define SMtrl_MtlGetDurability(id) 1.0f
#define SMtrl_MtlGetDensity(id) 1.0f
#define SMtrl_MtlGetHitChance(id) 1.0f

/*! \skydocent base_ammo
Базовый класс для патронов
*/

BEGIN_PROPTABLE(CBaseAmmo)
	//! Начальная скорость пули
	DEFINE_FIELD_FLOAT(m_fStartSpeed, PDFF_NOEDIT | PDFF_NOEXPORT, "start_speed", "", EDITOR_NONE)
	//! Масса пули, кг
	DEFINE_FIELD_FLOAT(m_fBulletMass, PDFF_NOEDIT | PDFF_NOEXPORT, "bullet_mass", "", EDITOR_NONE)
	//! Бронебойность
	DEFINE_FIELD_FLOAT(m_fArmorPiercing, PDFF_NOEDIT | PDFF_NOEXPORT, "armor_piercing", "", EDITOR_NONE)
	//! Экспансивная?
	DEFINE_FIELD_BOOL(m_isExpansive, PDFF_NOEDIT | PDFF_NOEXPORT, "is_expansive", "", EDITOR_NONE)
	//! Разрывная?
	DEFINE_FIELD_BOOL(m_isBursting, PDFF_NOEDIT | PDFF_NOEXPORT, "is_bursting", "", EDITOR_NONE)
END_PROPTABLE()

REGISTER_ENTITY_NOLISTING(CBaseAmmo, base_ammo);

void CBaseAmmo::fire(const float3 &vStart, const float3 &vDir, CBaseCharacter *pAttacker)
{
	extern CTracer *g_pTracer;

	broadcastMessage("firingHere", 50.0f);

	fire(vStart, vDir, pAttacker, m_fStartSpeed);
}

void CBaseAmmo::fire(const float3 &_vStart, const float3 &_vDir, CBaseCharacter *pAttacker, float fSpeed)
{
	extern CTracer *g_pTracer;

	int iJump = 0;

	float3 vStart = _vStart;
	float3 vDir = _vDir;
	float fMaxDistance = ((CBaseTool*)getParent())->getMaxDistance();

	float3 end;
	float t0 = 0.0f;
	float fY0;
	bool isY0set = false;
	float fSpeedY0;
	g_pTracer->begin(vStart);

	bool isBloody = false;

	while(iJump < BULLET_MAX_JUMPS && fSpeed > 0.5f && fMaxDistance > 0.0f)
	{
		end = vStart + SMVector3Normalize(vDir) * min(fMaxDistance, BULLET_STEP_DISTANCE);
		if(!isY0set)
		{
			fY0 = end.y;
			isY0set = true;
			fSpeedY0 = fSpeed * (vDir.y / SMVector3Length(vDir));
		}
		CAllHitsNotMeRayResultCallback cb(pAttacker ? pAttacker->getBtCollisionObject() : NULL);
		//cb.m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;
		GetPhysWorld()->rayTest(vStart, end, &cb, CG_BULLETFIRE, CG_ALL & ~(CG_DEBRIS | CG_TRIGGER | CG_CHARACTER));

		g_pTracer->lineTo(vStart, 0.0f);
		//g_pTracer->begin(vStart);
		if(cb.hasHit())
		{
			CAllHitsNotMeRayResultCallback cbBack(pAttacker ? pAttacker->getBtCollisionObject() : NULL);
			cbBack.m_aResults.reserve(cb.m_aResults.size());
			//cbBack.m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;
			GetPhysWorld()->rayTest(end, vStart, &cbBack, CG_BULLETFIRE, CG_ALL & ~(CG_DEBRIS | CG_TRIGGER | CG_CHARACTER));

			Array<HitPoint> aHitPoints;
			aHitPoints.reserve(cb.m_aResults.size() + cbBack.m_aResults.size());

			for(int i = 0, l = cb.m_aResults.size(); i < l; ++i)
			{
				XRayResult &r = cb.m_aResults[i];
				aHitPoints.push_back({r.vHitPoint, r.vHitNormal, r.pCollisionObject, r.fHitFraction, false});
			}
			for(int i = 0, l = cbBack.m_aResults.size(); i < l; ++i)
			{
				XRayResult &r = cbBack.m_aResults[i];
				aHitPoints.push_back({r.vHitPoint, r.vHitNormal, r.pCollisionObject, 1.0f - r.fHitFraction, true});
			}

			aHitPoints.quickSort([](const HitPoint &a, const HitPoint &b)
			{
				return(a.fFraction < b.fFraction);
			});
			for(int i = 0, l = aHitPoints.size(); i < l; ++i)
			{

				ID idMtl = -1; // SPhysics_GetMtlID(aHitPoints[i].pCollisionObject, &aHitPoints[i].shapeInfo);
				if(ID_VALID(idMtl) && !aHitPoints[i].isExit)
				{
					float fHitChance = SMtrl_MtlGetHitChance(idMtl);
					if(fHitChance < randf(0.0f, 1.0f))
					{
						int c = 0;
						for(int j = i + 1; j < l; ++j)
						{
							if(aHitPoints[j].isExit)
							{
								if(c == 0)
								{
									aHitPoints.erase(j);
									--l;
									break;
								}
								--c;
							}
							else
							{
								++c;
							}
						}
						aHitPoints.erase(i--);
						--l;
						//skip that
						continue;
					}
				}
				g_pTracer->lineTo(aHitPoints[i].vPosition, aHitPoints[i].isExit ? 0.0f : 1.0f);
				if(isBloody)
				{
					shootBlood(aHitPoints[i].vPosition, aHitPoints[i].vNormal);
				}
				else
				{
					shootDecal(aHitPoints[i].vPosition, aHitPoints[i].vNormal, idMtl);
				}
				if(!aHitPoints[i].isExit)
				{
					m_fNextBarrierDepth = 0.0f;
					// find next exit
					// calculate can we hole 
					for(int j = i + 1; j < l; ++j)
					{
						if(aHitPoints[j].isExit)
						{
							m_fNextBarrierDepth = SMVector3Length(aHitPoints[j].vPosition - aHitPoints[i].vPosition);
							break;
						}
					}
					float3 vNewDir;
					float fNewSpeed = fSpeed;
					if(shouldRecochet(aHitPoints[i].vPosition, aHitPoints[i].vNormal, vDir, idMtl, fSpeed, &vNewDir, &fNewSpeed))
					{
						// @TODO: play recochet sound
					}
					
					float fEnergyDelta = m_fBulletMass * 0.001f * 0.5f * (fSpeed * fSpeed - fNewSpeed * fNewSpeed);
					
					if(aHitPoints[i].pCollisionObject->getUserPointer() && aHitPoints[i].pCollisionObject->getUserTypeId() == 1)
					{
						CTakeDamageInfo takeDamageInfo(pAttacker, fEnergyDelta);
						takeDamageInfo.m_pInflictor = getParent();

						CBaseEntity *pEnt = ((CBaseEntity*)aHitPoints[i].pCollisionObject->getUserPointer());
						isBloody = pEnt->isBloody();

						pEnt->dispatchDamage(takeDamageInfo);
					}
					

					// restart fire with new dir and speed

					float3 vStart2 = aHitPoints[i].vPosition + SMVector3Normalize(vDir) * 0.001f;
					vDir = vNewDir;
					fSpeed = fNewSpeed;

					fMaxDistance -= SMVector3Length(vStart - vStart2);
					vStart = vStart2;

					t0 = 0.0f;
					isY0set = false;
					++iJump;
					//fire(aHitPoints[i].vPosition + SMVector3Normalize(vDir) * 0.001f, vNewDir, pAttacker, fNewSpeed, iJump + 1);
					break;
				}

			}



			//shoot decal
			//SXDecals_ShootDecal(DECAL_TYPE_CONCRETE, BTVEC_F3(cb.m_hitPointWorld[0]), BTVEC_F3(cb.m_hitNormalWorld[0]));
			//SPE_EffectPlayByName("fire", &BTVEC_F3(cb.m_hitPointWorld), &BTVEC_F3(cb.m_hitNormalWorld));
			/*if(!cb.m_collisionObject->isStaticOrKinematicObject())
			{
			((btRigidBody*)cb.m_collisionObject)->applyCentralImpulse(F3_BTVEC(vDir * 10.0f));
			cb.m_collisionObject->activate();
			}*/
			//g_pTracer->lineTo(BTVEC_F3(cb.m_hitPointWorld[0]) + SMVector3Normalize(BTVEC_F3(cb.m_hitNormalWorld[0])) * 0.1f, 0.0f);
		}
		else
		{
			g_pTracer->lineTo(end, 1.0f);
			fMaxDistance -= min(fMaxDistance, BULLET_STEP_DISTANCE);
			float t1 = t0 + (min(fMaxDistance, BULLET_STEP_DISTANCE) / fSpeed);
			float3 vNewStart = end;
			// apply derivation
			// and...
			// apply wind
			// end...

			// apply gravity
			//end.y += (fSpeed * (vDir.y / SMVector3Length(vDir)) * (t1 - t0) - 10.0f * 0.5f * (t1 * t1 - t0 * t0)); // 10.0f - gravity
			end.y = fY0 + fSpeedY0 * t0 - 10.0f * 0.5f * t0 * t0;

			vDir = SMVector3Normalize(end - vStart);

			vStart = vNewStart;
			t0 = t1;
		}
	}
	g_pTracer->end();
}

void CBaseAmmo::shootDecal(const float3 &vPos, const float3 &vNormal, ID idMtl)
{
	if(ID_VALID(idMtl))
	{
		MTLTYPE_PHYSIC type = SMtrl_MtlGetPhysicMaterial(idMtl);
		DECAL_TYPE decalType = DECAL_TYPE_CONCRETE;
		switch(type)
		{
		case MTLTYPE_PHYSIC_METAL:
			decalType = DECAL_TYPE_METAL;
			break;
		case MTLTYPE_PHYSIC_FLESH:
			decalType = DECAL_TYPE_FLESH;
			break;
		case MTLTYPE_PHYSIC_GROUD_SAND:
			decalType = DECAL_TYPE_EARTH;
			break;
		case MTLTYPE_PHYSIC_PLASTIC:
			decalType = DECAL_TYPE_PLASTIC;
			break;
		case MTLTYPE_PHYSIC_TREE:
			decalType = DECAL_TYPE_WOOD;
			break;
		}
		SXDecals_ShootDecal(decalType, vPos, vNormal);

		//SPE_EffectPlayByName("create_decal_test", &aHitPoints[i].vPosition, &aHitPoints[i].vNormal);
	}
}

void CBaseAmmo::shootBlood(const float3 &vPos, const float3 &vNormal)
{
	SXDecals_ShootDecal(DECAL_TYPE_BLOOD_BIG, vPos, vNormal);
}

bool CBaseAmmo::shouldRecochet(const float3 &vPos, const float3 &vNormal, const float3 &vDir, ID idMtl, float fSpeed, float3 *pvNewDir, float *pfNewSpeed)
{
	//m_fArmorPiercing ^ => chance ^
	//fDurability ^ => chance ^
	//angle(-dir, normal) > 87 && m_isBursting => chance = 0
	//if(angle(-dir, normal) -> 90)
	//{
	//	fDensity ^ => xAngle ^
	//}
	//else if(angle(-dir, normal) -> 0)
	//{
	//	fDensity ^ => chance ^
	//	if(canHole())
	//	{
	//		chance = 0
	//	}
	//	else
	//	{
	//		chance ^
	//	}
	//}
	//m_isExpansive => chance = 0
	//
	float fDurability = 10;
	float fDensity = 1000;
	bool isNewSpeedSet = false;
	if(ID_VALID(idMtl))
	{
		fDurability = SMtrl_MtlGetDurability(idMtl); // прочность
		fDensity = SMtrl_MtlGetDensity(idMtl); // плотность
		if(fDurability <= 0)
		{
			fDurability = 10;
		}
	}
	float fChance = 0.05f;
	float fXangle = 0.0f;
	float fAngleDirNormal = SMToAngle(acosf(SMVector3Dot(-vDir, vNormal)));

	if(m_isBursting && fAngleDirNormal > 87.0f)
	{
		fChance = 0.0f;
	}
	else
	{
		fChance += (1.0f - pow(0.9f, m_fArmorPiercing)) * 0.4f + (1.0f - pow(0.9f, fDurability)) * 0.4f;
	}

	if(fAngleDirNormal > 80.0f)
	{
		fXangle = (1.0f - pow(0.99f, fDensity)) * 45.0f * (true /* RIFLE_TYPE == RIFLE_TYPE_RIGHT */ ? 1.0f : -1.0f); // RIFLE_TYPE_UNRIFLED : 0.0f
		// @FIXME: consider m_fRifleStep too
	}
	//else if(fAngleDirNormal < 10.0f)
	{
		fChance += (1.0f - pow(0.9f, fDensity)) * 0.5f;
		if(canHole(fDurability, fSpeed, pfNewSpeed))
		{
			fChance = 0.0f;
			isNewSpeedSet = true;
		}
		else
		{
			fChance += 0.05f;
		}
	}

	if(m_isExpansive)
	{
		fChance = 0.0f;
	}

	fChance *= 0.1f;

	bool bRecochet = randf(0.0f, 1.0f) < fChance;

	if(bRecochet)
	{
		*pvNewDir = SMQuaternion(SMVector3Cross(vDir, vNormal), SMToRadian(fAngleDirNormal)) * SMQuaternion(vNormal, fXangle) * vNormal;
	}
	else
	{
		*pvNewDir = vDir;
	}

	if(!isNewSpeedSet)
	{
		if(bRecochet)
		{
			*pfNewSpeed = fSpeed;
			//if(fXangle > 0.0f)
			{
				*pfNewSpeed *= 0.9f;
			}
		}
		else
		{
			*pfNewSpeed = 0.0f;
		}
	}

	//pvNewDir
	//pfNewSpeed

	return(bRecochet);
}

bool CBaseAmmo::canHole(float fDurability, float fCurrentSpeed, float *pfNewSpeed)
{
	// based on m_fNextBarrierDepth

	if(m_fNextBarrierDepth < FLT_EPSILON)
	{
		//*pfNewSpeed = fCurrentSpeed;
		//return(true);
		m_fNextBarrierDepth = 0.001f;
	}

	float fCurrentEnergy = m_fBulletMass * 0.001f * fCurrentSpeed * fCurrentSpeed / 2.0f;

	float fRequiredEnergy = fDurability * m_fNextBarrierDepth * 100.0f;


	fCurrentEnergy -= fRequiredEnergy;

	if(fCurrentEnergy >= 0.0f)
	{
		*pfNewSpeed = sqrtf(fCurrentEnergy * 2.0f / (m_fBulletMass * 0.001f));
		return(true);
	}

	*pfNewSpeed = 0.0f;
	return(false);
}

float CBaseAmmo::getStartSpeed() const
{
	return(m_fStartSpeed);
}