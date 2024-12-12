
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

/*!
\file
Анимированный игровой объект
*/

/*! \defgroup cbaseanimating Моделированные объекты
\ingroup cbaseentity
@{
*/

#ifndef __BASE_ANIMATING_H
#define __BASE_ANIMATING_H

#include "BaseEntity.h"
#include <xcommon/resource/IXModelProvider.h>

#define BLEND_MAX 4

class CBaseAnimating;

class CMotionState: public IXRigidBodyMotionCallback
{
private:
	CBaseAnimating *m_pEntity;

public:
	CMotionState(CBaseAnimating *pEntity):
		m_pEntity(pEntity)
	{
	}
	
	///synchronizes world transform from physics to user
	void XMETHODCALLTYPE setWorldTransform(const float3 &vPos, const SMQuaternion &q) override;
};

//##########################################################################

class CAnimationCallback;
//! Анимированный игровой объект
class CBaseAnimating: public CBaseEntity
{
	DECLARE_CLASS(CBaseAnimating, CBaseEntity);
	DECLARE_PROPTABLE();

	friend class CAnimationCallback;
	friend class CMotionState;
public:
	DECLARE_CONSTRUCTOR();
	~CBaseAnimating();

	void getMinMax(float3 *min, float3 *max);

	virtual void setModel(const char *szMdl);
	virtual void setScale(float fScale);

	float3 getAttachmentPos(int id) override;
	SMQuaternion getAttachmentRot(int id) override;

	void playAnimation(const char * name, UINT uFadeTime = 0, UINT slot = 0);
	void playActivity(const char * name, UINT uFadeTime = 0, UINT slot = 0);

	void playAnimationNext(const char * name, UINT uFadeTime = 0, UINT slot = 0);
	void playActivityNext(const char * name, UINT uFadeTime = 0, UINT slot = 0);

	void cancelNextAnimation(int iSlot = -1);

	bool playingAnimations(const char* name);
	void setPos(const float3 & pos) override;
	void setOrient(const SMQuaternion & q) override;
	
	void setSkin(int iSkin);
	
	void setCollisionGroup(COLLISION_GROUP group, COLLISION_GROUP mask = CG_ALL);
	COLLISION_GROUP getCollisionGroup();

	void renderEditor(bool is3D, bool bRenderSelection, IXGizmoRenderer *pRenderer) override;

	bool rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut = NULL, float3 *pvNormal = NULL, bool isRayInWorldSpace = true, bool bReturnNearestPoint = false) override;

protected:
	void inputPlayAnim(inputdata_t *pInputdata);
	void inputPlayAnimNext(inputdata_t *pInputdata);

	void inputPlayActivity(inputdata_t *pInputdata);
	void inputPlayActivityNext(inputdata_t *pInputdata);

	void inputSetSkin(inputdata_t *pInputdata);

	IXModel *m_pModel = NULL;
	const char * m_szModelFile;
	float m_fBaseScale = 1.0f;
	bool m_isStatic = false;
	bool m_useAutoPhysbox = true;

	CEntityPointer<CBaseEntity> m_pEntColorRef;
	float3_t m_vGlowColor;

	virtual void initPhysics();
	virtual void releasePhysics();
	virtual void createPhysBody();
	virtual void removePhysBody();

	IXCollisionShape *m_pCollideShape = NULL;
	IXRigidBody *m_pRigidBody = NULL;

	void _cleanup() override;

	virtual void onAnimationStart(UINT uLayer);
	virtual void onAnimationStop(UINT uLayer);
	virtual void onAnimationLoop(UINT uLayer);
	virtual void onAnimationProgress(UINT uLayer, float fProgress);

	void onIsStaticChange(bool isStatic);
	void onSetUseAutoPhysbox(bool use);
	void onColorChanged(const float3 &vColor);
	int m_iSkin = 0;

	struct
	{
		char szName[XMODEL_MAX_NAME];
		UINT uFadeTime;
		bool isActivity;
	} m_vNextAnim[BLEND_MAX];

private:
	COLLISION_GROUP m_collisionGroup = CG_DEFAULT;
	COLLISION_GROUP m_collisionMask = CG_ALL;
	CAnimationCallback *m_pAnimationCallback;

	CMotionState m_motionState;

	bool m_bTransformFromCallback = false;
private:

	void _setXform(const float3 &vPos, const SMQuaternion &q);
};

#endif

/*! @} cbaseanimating */
