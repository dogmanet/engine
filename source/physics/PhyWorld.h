
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#ifndef __PHY_WORLD_H
#define __PHY_WORLD_H


#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorldMt.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolverMt.h>
#include <BulletCollision/CollisionDispatch/btCollisionDispatcherMt.h>

#include <common/AssotiativeArray.h>
#include <common/Array.h>
#include <common/file_utils.h>
#include <xcommon/IXRenderable.h>

#include <gdefines.h>

#include <common/Math.h>
#include <xcommon/XEvents.h>

#include <common/queue.h>
#include "sxphysics.h"

#define PHY_MAT_FILE_MAGICK 3630267958475905107

#pragma pack(push,1)
struct PhyMatFile
{
	int64_t i64Magick;
	uint32_t uiGeomFaceCount;
	uint32_t uiMatCount;
	//uint32_t uiGreenObjCount;

	PhyMatFile():
		i64Magick(PHY_MAT_FILE_MAGICK)
	{
	}
};
#pragma pack(pop)

class CPhyWorld final: public IXUnknownImplementation<IXPhysicsWorld>
{
public:
	CPhyWorld();
	~CPhyWorld();

	void update();

	void XMETHODCALLTYPE disableSimulation() override;
	void XMETHODCALLTYPE enableSimulation() override;

	void render();

	btDiscreteDynamicsWorldMt * getBtWorld()
	{
		return(m_pDynamicsWorld);
	}

	class CDebugDrawer: public btIDebugDraw
	{
		struct render_point
		{
			float3_t pos;
			float4_t clr;
		};
		int m_iDebugMode = 0;
		render_point m_pDrawData[4096];
		UINT m_uDataSize = 4096;
		UINT m_uDataPointer = 0;

		bool m_bExpectObject = false;

		IGXVertexBuffer *m_pVertexBuffer = NULL;
		IGXVertexDeclaration *m_pVertexDeclaration = NULL;
		IGXRenderBuffer *m_pRenderBuffer = NULL;
		IGXConstantBuffer *m_pVSConstantBuffer = NULL;
		ID m_idShader = -1;

		IGXDevice *m_pDevice = NULL;
		IXRender *m_pRender = NULL;

	public:
		CDebugDrawer();
		~CDebugDrawer();

		void setDevice(IXRender *pRender);

		void drawLine(const btVector3& from, const btVector3& to, const btVector3& color);

		void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color);

		void reportErrorWarning(const char* warningString);

		void draw3dText(const btVector3& location, const char* textString);

		void setDebugMode(int debugMode);

		int getDebugMode() const;

		void begin();
		void render();
		void commit();
	};

	class CRenderable: public IXUnknownImplementation<IXRenderable>
	{
	public:
		CRenderable(CPhyWorld *pWorld):
			m_pWorld(pWorld)
		{
		}

		XIMPLEMENT_VERSION(IXRENDERABLE_VERSION); 

		bool XMETHODCALLTYPE isVisThreadOptimized() override
		{
			return(false);
		}

		X_RENDER_STAGE XMETHODCALLTYPE getStages() override
		{
			return(XRS_GBUFFER);
		}

		UINT XMETHODCALLTYPE getPriorityForStage(X_RENDER_STAGE stage) override
		{
			return(1);
		}

		void XMETHODCALLTYPE renderStage(X_RENDER_STAGE stage, IXRenderableVisibility *pVisibility) override;

		UINT XMETHODCALLTYPE getTransparentCount(IXRenderableVisibility *pVisibility) override
		{
			return(0);
		}
		void XMETHODCALLTYPE getTransparentObject(IXRenderableVisibility *pVisibility, UINT uIndex, XTransparentObject *pObject) override
		{
		}
		void XMETHODCALLTYPE renderTransparentObject(IXRenderableVisibility *pVisibility, UINT uIndex, UINT uSplitPlanes) override
		{
		}


		void XMETHODCALLTYPE startup(IXRender *pRender, IXMaterialSystem *pMaterialSystem) override;
		void XMETHODCALLTYPE shutdown() override
		{
		}
		void XMETHODCALLTYPE newVisData(IXRenderableVisibility **ppVisibility) override
		{
			*ppVisibility = NULL;
		}

	protected:
		CPhyWorld *m_pWorld;
	};
	
	void XMETHODCALLTYPE addCollisionObject(IXCollisionObject *pCollisionObject, COLLISION_GROUP collisionGroup = CG_DEFAULT, COLLISION_GROUP collisionMask = CG_ALL) override;
	void XMETHODCALLTYPE removeCollisionObject(IXCollisionObject *pCollisionObject) override;

	void XMETHODCALLTYPE rayTest(const float3 &vFrom, const float3 &vTo, IXRayCallback *pCallback, COLLISION_GROUP collisionGroup = CG_DEFAULT, COLLISION_GROUP collisionMask = CG_ALL) override;

	void XMETHODCALLTYPE convexSweepTest(IXConvexShape *pShape, const transform_t &xfFrom, const transform_t &xfTo, IXConvexCallback *pCallback, COLLISION_GROUP collisionGroup = CG_DEFAULT, COLLISION_GROUP collisionMask = CG_ALL) override;

	template<class T>
	void updateSingleAABB(T *pObj)
	{
		updateSingleAABB(pObj, pObj->getBtCollisionObject());
	}

private:
	btDefaultCollisionConfiguration *m_pCollisionConfiguration;
	btCollisionDispatcherMt *m_pDispatcher;
	btBroadphaseInterface *m_pBroadphase;
	btSequentialImpulseConstraintSolverMt *m_pSolver;
	btDiscreteDynamicsWorldMt *m_pDynamicsWorld;
	btGhostPairCallback *m_pGHostPairCallback;

	std::atomic_bool m_isUpdating;

	const bool *m_bDebugDraw;
	CDebugDrawer *m_pDebugDrawer;

	bool m_isRunning = false;
	int m_iSkipFrames = 3;

	IEventChannel<XEventPhysicsStep> *m_pTickEventChannel = NULL;

	static void TickCallback(btDynamicsWorld *world, btScalar timeStep);

	enum QUEUE_ITEM_TYPE
	{
		QIT_ADD_COLLISION_OBJECT,
		QIT_REMOVE_COLLISION_OBJECT,
		QIT_UPDATE_SINGLE_AABB
	};

	struct QueueItem
	{
		QUEUE_ITEM_TYPE type;
		IXCollisionObject *pObj;
		int iGroup;
		int iMask;
		btCollisionObject *pBtObj;
	};

	Queue<QueueItem> m_queue;

private:
	void enqueue(QueueItem &&item);
	void runQueue();

	void updateSingleAABB(IXCollisionObject *pObj, btCollisionObject *pBtObj);
};

#endif
