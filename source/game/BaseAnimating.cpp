
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "BaseAnimating.h"

#include <xcommon/resource/IXResourceManager.h>
#include <xcommon/resource/IXResourceModel.h>

/*! \skydocent base_animating
Базовый класс для объектов, имеющих объем в игровом мире
*/

BEGIN_PROPTABLE(CBaseAnimating)
	//! Файл модели. Поддерживаются статические и анимированные модели
	DEFINE_FIELD_STRINGFN(m_szModelFile, 0, "model", "Model file", setModel, EDITOR_MODEL)

	//! Масштаб модели
	DEFINE_FIELD_FLOATFN(m_fBaseScale, 0, "scale", "Scale", setScale, EDITOR_TEXTFIELD)

	//! Объект референса для цвета свечения
	DEFINE_FIELD_ENTITY(CBaseEntity, m_pEntColorRef, 0, "glow_color_ref", "Glow color reference", EDITOR_TEXTFIELD)
	//! Цвет свечения
	DEFINE_FIELD_VECTORFN(m_vGlowColor, 0, "glow_color", "Glow color", onColorChanged, EDITOR_TEXTFIELD)

	DEFINE_FIELD_BOOLFN(m_isStatic, 0, "is_static", "Is static", onIsStaticChange, EDITOR_YESNO)

	DEFINE_FIELD_INTFN(m_iSkin, 0, "skin", "Skin", setSkin, EDITOR_TEXTFIELD)

	DEFINE_FIELD_BOOLFN(m_useAutoPhysbox, 0, "auto_physbox", "Auto generate physbox", onSetUseAutoPhysbox, EDITOR_YESNO)

	DEFINE_INPUT(inputPlayAnim, "playAnim", "Play animation", PDF_STRING)
	DEFINE_INPUT(inputPlayAnimNext, "playAnimNext", "Play animation next", PDF_STRING)

	DEFINE_INPUT(inputPlayActivity, "playActivity", "Play activity", PDF_STRING)
	DEFINE_INPUT(inputPlayActivityNext, "playActivityNext", "Play activity next", PDF_STRING)

	DEFINE_INPUT(inputSetSkin, "setSkin", "Set skin", PDF_INT)
END_PROPTABLE()

REGISTER_ENTITY_NOLISTING(CBaseAnimating, base_animating);

//##########################################################################

class CAnimationCallback: public IAnimationCallback
{
public:
	CAnimationCallback(CBaseAnimating *pBaseAnimating):
		m_pBaseAnimating(pBaseAnimating)
	{
	}

	void onPlay(UINT uLayer) override
	{
		m_pBaseAnimating->onAnimationStart(uLayer);
	}
	void onStop(UINT uLayer) override
	{
		m_pBaseAnimating->onAnimationStop(uLayer);
	}
	void onLoop(UINT uLayer) override
	{
		m_pBaseAnimating->onAnimationLoop(uLayer);
	}

	void onProgress(UINT uLayer, float fProgress) override
	{
		m_pBaseAnimating->onAnimationProgress(uLayer, fProgress);
	}

private:
	CBaseAnimating *m_pBaseAnimating;
};

//##########################################################################

void CMotionState::setWorldTransform(const float3 &vPos, const SMQuaternion &q)
{
	m_pEntity->_setXform(vPos, q);
}

//##########################################################################

CBaseAnimating::CBaseAnimating():
	m_motionState(this),
	m_pAnimationCallback(new CAnimationCallback(this))
{
	memset(m_vNextAnim, 0, sizeof(m_vNextAnim));

	m_vGlowColor.x = 0.0f;
	m_vGlowColor.y = randf(0.1f, 1.0f);
	m_vGlowColor.z = randf(0.1f, 1.0f);
}

CBaseAnimating::~CBaseAnimating()
{
	releasePhysics();
	mem_release(m_pModel);
	mem_delete(m_pAnimationCallback);
}

void CBaseAnimating::getMinMax(float3 * min, float3 * max)
{
	if(m_pModel)
	{
		*min = m_pModel->getLocalBoundMin();
		*max = m_pModel->getLocalBoundMax();
	}
}

void CBaseAnimating::onSetUseAutoPhysbox(bool use)
{
	if(m_useAutoPhysbox != use)
	{
		m_useAutoPhysbox = use;
		releasePhysics();
		initPhysics();
	}
}

void CBaseAnimating::onColorChanged(const float3 &vColor)
{
	m_vGlowColor = vColor;
	if(SMVector3Length(vColor) < 0.1f)
	{
		m_vGlowColor.x = 0.0f;
		m_vGlowColor.y = randf(0.1f, 1.0f);
		m_vGlowColor.z = randf(0.1f, 1.0f);
	}
	SAFE_CALL(m_pModel, setColor, (float3)m_vGlowColor);
}

void CBaseAnimating::setModel(const char * mdl)
{
	_setStrVal(&m_szModelFile, mdl);
	releasePhysics();
	mem_release(m_pModel);
	if(!mdl[0] /*&& m_pAnimPlayer*/)
	{
		return;
	}

	IXResourceManager *pResourceManager = Core_GetIXCore()->getResourceManager();
	IXModelProvider *pProvider = (IXModelProvider*)Core_GetIXCore()->getPluginManager()->getInterface(IXMODELPROVIDER_GUID);
	
	IXResourceModel *pResource;
	if(pResourceManager->getModel(mdl, &pResource))
	{
		IXDynamicModel *pModel;
		if(pProvider->createDynamicModel(pResource, &pModel))
		{
			m_pModel = pModel;
			m_pModel->setSkin(m_iSkin);
			m_pModel->setScale(m_fBaseScale);
			m_pModel->setPosition(getPos());
			m_pModel->setOrientation(getOrient());
			m_pModel->setColor((float4)m_vGlowColor);
			
			auto pAnimating = m_pModel->asAnimatedModel();
			if(pAnimating)
			{
				pAnimating->setCallback(m_pAnimationCallback);
				pAnimating->play("idle");
			}
		}
		mem_release(pResource);
	}

	initPhysics();
}

void CBaseAnimating::setScale(float fScale)
{
	m_fBaseScale = fScale;

	releasePhysics();
	if(m_pModel)
	{
		m_pModel->setScale(m_fBaseScale);
	}
	initPhysics();
}

float3 CBaseAnimating::getAttachmentPos(int id)
{
	float3 pos;
	if(m_pModel && m_pModel->asAnimatedModel() && id >= 0)
	{
		pos = m_pModel->asAnimatedModel()->getBoneTransformPos(id);
	}

	return(/*getOrient() * */pos/* + getPos()*/);
}

SMQuaternion CBaseAnimating::getAttachmentRot(int id)
{
	SMQuaternion rot;
	if(m_pModel && m_pModel->asAnimatedModel() && id >= 0)
	{
		rot = m_pModel->asAnimatedModel()->getBoneTransformRot(id);
	}

	return(/*getOrient() * */rot);
}

#if 0
void CBaseAnimating::onSync()
{
	BaseClass::onSync();
	if(m_pModel && m_pModel->isEnabled())
	{
		float3_t vGlowColor = m_vGlowColor;
		//bool isGlowEnabled = m_pEntColorRef ? m_pEntColorRef->getMainColor(&vGlowColor) : m_vGlowColor.x != 0.0f || m_vGlowColor.y != 0.0f || m_vGlowColor.z != 0.0f;
		m_pModel->setColor(float4(vGlowColor));
		//m_pAnimPlayer->setGlowEnabled(isGlowEnabled);
	}
}
#endif

void CBaseAnimating::playAnimation(const char * name, UINT iFadeTime, UINT slot)
{
	if(m_pModel && m_pModel->asAnimatedModel())
	{
		m_pModel->asAnimatedModel()->play(name, iFadeTime, slot);
	}
}

bool CBaseAnimating::playingAnimations(const char* name)
{
	if(m_pModel && m_pModel->asAnimatedModel())
	{
		return(m_pModel->asAnimatedModel()->isPlayingAnimation(name));
	}
	return(false);
}

void CBaseAnimating::playActivity(const char * name, UINT iFadeTime, UINT slot)
{
	if(m_pModel && m_pModel->asAnimatedModel())
	{
		m_pModel->asAnimatedModel()->startActivity(name, iFadeTime, slot);
	}
}

void CBaseAnimating::initPhysics()
{
	if(!m_pModel)
	{
		return;
	}

	UINT uShapesCount = m_pModel->getPhysboxCount();


	IXCompoundShape *pShape;
	GetPhysics()->newCompoundShape(&pShape, uShapesCount);
	for(UINT i = 0; i < uShapesCount; ++i)
	{
		auto pPhysbox = m_pModel->getPhysBox(i);
		IXCollisionShape *pLocalShape = NULL;
		switch(pPhysbox->getType())
		{
		case XPBT_BOX:
			IXBoxShape *pBox;
			GetPhysics()->newBoxShape(pPhysbox->asBox()->getSize(), &pBox);
			pLocalShape = pBox;
			break;
		case XPBT_SPHERE:
			IXSphereShape *pSphere;
			GetPhysics()->newSphereShape(pPhysbox->asSphere()->getRadius(), &pSphere);
			pLocalShape = pSphere;
			break;
		case XPBT_CAPSULE:
			IXCapsuleShape *pCaps;
			GetPhysics()->newCapsuleShape(pPhysbox->asCapsule()->getRadius(), pPhysbox->asCapsule()->getHeight(), &pCaps);
			pLocalShape = pCaps;
			break;
		case XPBT_CYLINDER:
			IXCylinderShape *pCyl;
			GetPhysics()->newCylinderShape(pPhysbox->asCylinder()->getRadius(), pPhysbox->asCylinder()->getHeight(), &pCyl);
			pLocalShape = pCyl;
			break;
		case XPBT_CONVEX:
			IXConvexHullShape *pConvexHull;
			GetPhysics()->newConvexHullShape(pPhysbox->asConvex()->getVertexCount(), pPhysbox->asConvex()->getData(), &pConvexHull);
			pLocalShape = pConvexHull;
			break;
		}

		if(pLocalShape)
		{
			pShape->addChildShape(pLocalShape, pPhysbox->getPosition(), pPhysbox->getOrientation());
		}
	}

	if(!uShapesCount && m_useAutoPhysbox)
	{
		{
			auto pResource = m_pModel->getResource()->asStatic();
			if(pResource)
			{
				UINT uUsedLod = pResource->getLodCount() - 1;
				for(UINT i = 0, l = pResource->getSubsetCount(uUsedLod); i < l; ++i)
				{
					IXConvexHullShape *pLocalShape = NULL;
					auto pSubset = pResource->getSubset(uUsedLod, i);

					GetPhysics()->newConvexHullShape(pSubset->iVertexCount, (float3_t*)pSubset->pVertices, &pLocalShape, sizeof(pSubset->pVertices[0]));

#if 0
					btConvexHullShape tmpShape((float*)pSubset->pVertices, pSubset->iVertexCount, sizeof(pSubset->pVertices[0]));
					tmpShape.setMargin(0);
					btVector3 *pData;
					int iVertexCount;
					SPhysics_BuildHull(&tmpShape, &pData, &iVertexCount);
					for(int i = 0; i < iVertexCount; ++i)
					{
						pData[i] *= m_fBaseScale;
					}
					if(iVertexCount)
					{
						pLocalShape = new btConvexHullShape((float*)pData, iVertexCount, sizeof(btVector3));
					}
					else
					{
						LibReport(REPORT_MSG_LEVEL_WARNING, "Failed to build convex hull for model '%s'!\n", m_szModelFile);
					}
					SPhysics_ReleaseHull(pData, iVertexCount);
#endif

					if(pLocalShape)
					{
						pShape->addChildShape(pLocalShape, float3(), SMQuaternion());
					}
				}
			}
		}
		{
			auto pResource = m_pModel->getResource()->asAnimated();
			if(pResource && pResource->getLodCount())
			{
				UINT uUsedLod = pResource->getLodCount() - 1;
				for(UINT i = 0, l = pResource->getSubsetCount(uUsedLod); i < l; ++i)
				{
					IXConvexHullShape *pLocalShape = NULL;
					auto pSubset = pResource->getSubset(uUsedLod, i);

					GetPhysics()->newConvexHullShape(pSubset->iVertexCount, (float3_t*)pSubset->pVertices, &pLocalShape, sizeof(pSubset->pVertices[0]));

					if(pLocalShape)
					{
						pShape->addChildShape(pLocalShape, float3(), SMQuaternion());
					}
				}
			}
		}
	}
	pShape->setLocalScaling(m_fBaseScale);
	pShape->recalculateLocalAabb();
	m_pCollideShape = pShape;
	createPhysBody();
}

void CBaseAnimating::createPhysBody()
{
	if(m_pCollideShape)
	{
		const float fMass = 1.0f;

		XRIDIGBODY_DESC desc;
		desc.vLocalInertia = m_pCollideShape->calculateLocalInertia(fMass);
		desc.fMass = fMass;
		desc.pMotionCallback = &m_motionState;
		desc.vStartWorldPosition = getPos();
		desc.qStartWorldRotation = getOrient();
		desc.pCollisionShape = m_pCollideShape;

		GetPhysics()->newRigidBody(desc, &m_pRigidBody);

		//m_pRigidBody->setFriction(100.0f);
		m_pRigidBody->setUserPointer(this);
		m_pRigidBody->setUserTypeId(1);
		COLLISION_GROUP colGroup = m_collisionGroup;
		COLLISION_GROUP colMask = m_collisionMask;
		if(m_isStatic)
		{
			colGroup = CG_STATIC;
			colMask = CG_STATIC_MASK;
		}
		GetPhysWorld()->addCollisionObject(m_pRigidBody, colGroup, colMask);

		if(m_isStatic)
		{
			m_pRigidBody->setLinearFactor(float3(0.0f, 0.0f, 0.0f));
			m_pRigidBody->setAngularFactor(float3(0.0f, 0.0f, 0.0f));
		}
	}
}

void CBaseAnimating::removePhysBody()
{
	GetPhysWorld()->removeCollisionObject(m_pRigidBody);
	mem_release(m_pRigidBody);
}

void CBaseAnimating::releasePhysics()
{
	removePhysBody();
	
	mem_release(m_pCollideShape);
}

void CBaseAnimating::setCollisionGroup(COLLISION_GROUP group, COLLISION_GROUP mask)
{
	if(m_collisionGroup == group && m_collisionMask == mask)
	{
		return;
	}
	m_collisionGroup = group;
	m_collisionMask = mask;
	if(m_pRigidBody)
	{
		GetPhysWorld()->removeCollisionObject(m_pRigidBody);
		GetPhysWorld()->addCollisionObject(m_pRigidBody, m_collisionGroup, m_collisionMask);
	}
}
COLLISION_GROUP CBaseAnimating::getCollisionGroup()
{
	return(m_collisionGroup);
}

void CBaseAnimating::setPos(const float3 &pos)
{
	BaseClass::setPos(pos);

	if(!m_bTransformFromCallback)
	{
		SAFE_CALL(m_pRigidBody, setPosition, pos);
	}

	SAFE_CALL(m_pModel, setPosition, pos);
}

void CBaseAnimating::setOrient(const SMQuaternion &q)
{
	BaseClass::setOrient(q);

	if(!m_bTransformFromCallback)
	{
		SAFE_CALL(m_pRigidBody, setRotation, q);
	}

	SAFE_CALL(m_pModel, setOrientation, q);
}

void CBaseAnimating::_cleanup()
{
	releasePhysics();

	BaseClass::_cleanup();
}

void CBaseAnimating::onAnimationStart(UINT uLayer)
{
}
void CBaseAnimating::onAnimationStop(UINT uLayer)
{
	if(m_vNextAnim[uLayer].szName[0] != 0)
	{
		if(m_vNextAnim[uLayer].isActivity)
		{
			playActivity(m_vNextAnim[uLayer].szName, m_vNextAnim[uLayer].uFadeTime, uLayer);
		}
		else
		{
			playAnimation(m_vNextAnim[uLayer].szName, m_vNextAnim[uLayer].uFadeTime, uLayer);
		}

		m_vNextAnim[uLayer].szName[0] = 0;
	}
}
void CBaseAnimating::onAnimationLoop(UINT uLayer)
{
}
void CBaseAnimating::onAnimationProgress(UINT uLayer, float fProgress)
{
}

void CBaseAnimating::playAnimationNext(const char * name, UINT uFadeTime, UINT slot)
{
	assert(slot < BLEND_MAX);

	strncpy(m_vNextAnim[slot].szName, name, XMODEL_MAX_NAME);
	m_vNextAnim[slot].szName[XMODEL_MAX_NAME - 1] = 0;
	m_vNextAnim[slot].isActivity = false;
	m_vNextAnim[slot].uFadeTime = uFadeTime;
}

void CBaseAnimating::playActivityNext(const char * name, UINT uFadeTime, UINT slot)
{
	assert(slot < BLEND_MAX);

	strncpy(m_vNextAnim[slot].szName, name, XMODEL_MAX_NAME);
	m_vNextAnim[slot].szName[XMODEL_MAX_NAME - 1] = 0;
	m_vNextAnim[slot].isActivity = true;
	m_vNextAnim[slot].uFadeTime = uFadeTime;
}

void CBaseAnimating::cancelNextAnimation(int iSlot)
{
	assert(iSlot < BLEND_MAX);

	if(iSlot >= 0)
	{
		m_vNextAnim[iSlot].szName[0] = 0;
	}
	else
	{
		for(int i = 0; i < BLEND_MAX; ++i)
		{
			m_vNextAnim[i].szName[0] = 0;
		}
	}
}

void CBaseAnimating::onIsStaticChange(bool isStatic)
{
	if(m_pRigidBody && m_isStatic != isStatic)
	{
		if(isStatic)
		{
			m_pRigidBody->setLinearFactor(float3(0.0f, 0.0f, 0.0f));
			m_pRigidBody->setAngularFactor(float3(0.0f, 0.0f, 0.0f));

			GetPhysWorld()->removeCollisionObject(m_pRigidBody);
			GetPhysWorld()->addCollisionObject(m_pRigidBody, CG_STATIC, CG_STATIC_MASK);
		}
		else
		{
			m_pRigidBody->setLinearFactor(float3(1.0f, 1.0f, 1.0f));
			m_pRigidBody->setAngularFactor(float3(1.0f, 1.0f, 1.0f));

			GetPhysWorld()->removeCollisionObject(m_pRigidBody);
			GetPhysWorld()->addCollisionObject(m_pRigidBody, m_collisionGroup, m_collisionMask);
		}
	}
	m_isStatic = isStatic;
}

void CBaseAnimating::inputPlayAnim(inputdata_t * pInputdata)
{
	if(pInputdata->type != PDF_STRING)
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "CBaseAnimating::inputPlayAnim() expected parameter type string");
		return;
	}
	playAnimation(pInputdata->parameter.str);
}

void CBaseAnimating::inputPlayAnimNext(inputdata_t * pInputdata)
{
	if(pInputdata->type != PDF_STRING)
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "CBaseAnimating::inputPlayAnimNext() expected parameter type string");
		return;
	}
	playAnimationNext(pInputdata->parameter.str);
}


void CBaseAnimating::inputPlayActivity(inputdata_t * pInputdata)
{
	if(pInputdata->type != PDF_STRING)
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "CBaseAnimating::inputPlayActivity() expected parameter type string");
		return;
	}
	playActivity(pInputdata->parameter.str);
}

void CBaseAnimating::inputPlayActivityNext(inputdata_t * pInputdata)
{
	if(pInputdata->type != PDF_STRING)
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "CBaseAnimating::inputPlayActivityNext() expected parameter type string");
		return;
	}
	playActivityNext(pInputdata->parameter.str);
}

void CBaseAnimating::inputSetSkin(inputdata_t *pInputdata)
{
	assert(pInputdata->type == PDF_INT);

	setSkin(pInputdata->parameter.i);
}

void CBaseAnimating::setSkin(int iSkin)
{
	if(m_pModel)
	{
		m_pModel->setSkin(iSkin);
	}
	m_iSkin = iSkin;
}

void CBaseAnimating::renderEditor(bool is3D, bool bRenderSelection, IXGizmoRenderer *pRenderer)
{
	if(bRenderSelection)
	{
		SAFE_CALL(m_pModel, render, 0, MF_OPAQUE | MF_TRANSPARENT);

		IXAnimatedModel *pAnim;
		if(m_pModel && pRenderer && (pAnim = m_pModel->asAnimatedModel()))
		{
			pRenderer->setPointMode(XGPM_ROUND);
			pRenderer->setPointSize(is3D ? 0.02f : 3.0f);
			pRenderer->setLineWidth(is3D ? 0.01f : 1.5f);

			float3 vPos;
			int iParent;
			UINT uBoneCount = pAnim->getBoneCount();
			for(UINT i = 0; i < uBoneCount; ++i)
			{
				vPos = pAnim->getBoneTransformPos(i);
				pRenderer->setColor(float4(1.0f, 1.0f, 1.0f, 1.0f));
				pRenderer->drawPoint(vPos);
				iParent = pAnim->getBoneParent(i);
				if(iParent >= 0)
				{
					pRenderer->setColor(float4(0.0f, 1.0f, 0.0f, 0.1f));
					pRenderer->jumpTo(pAnim->getBoneTransformPos(iParent));
					pRenderer->setColor(float4(0.0f, 1.0f, 0.0f, 1.0f));
					pRenderer->lineTo(vPos);
				}
			}
			// render skeleton
		}
	}
}

bool CBaseAnimating::rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut, float3 *pvNormal, bool isRayInWorldSpace, bool bReturnNearestPoint)
{
	if(m_pModel && m_pModel->isEnabled())
	{
		return(m_pModel->rayTest(vStart, vEnd, pvOut, pvNormal, isRayInWorldSpace, bReturnNearestPoint));
	}
	return(false);
}

void CBaseAnimating::_setXform(const float3 &vPos, const SMQuaternion &q)
{
	m_bTransformFromCallback = true;
	setPos(vPos);
	setOrient(q);
	m_bTransformFromCallback = false;
}
