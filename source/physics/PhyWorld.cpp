
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "PhyWorld.h"
#include <core/sxcore.h>

#include "sxphysics.h"

#include "CollisionObject.h"

//#include <BulletDynamics/MLCPSolvers/btDantzigSolver.h>
//#include <BulletDynamics/MLCPSolvers/btMLCPSolver.h>

//#include <../Extras/Serialize/BulletWorldImporter/btBulletWorldImporter.h>


struct XRayResultCallback: public btCollisionWorld::RayResultCallback
{
	XRayResultCallback(IXRayCallback *pCallback, const btVector3 &rayFromWorld, const btVector3 &rayToWorld):
		m_pCallback(pCallback),
		m_rayFromWorld(rayFromWorld),
		m_rayToWorld(rayToWorld)
	{
		m_result._reserved = NULL;
	}

	IXRayCallback *m_pCallback;

	btVector3 m_rayFromWorld;//used to calculate hitPointWorld from hitFraction
	btVector3 m_rayToWorld;

	btVector3 m_hitNormalWorld;
	btVector3 m_hitPointWorld;

	XRayResult m_result;

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override
	{
		//caller already does the filter on the m_closestHitFraction
		btAssert(rayResult.m_hitFraction <= m_closestHitFraction);

		//m_closestHitFraction = rayResult.m_hitFraction;
		m_collisionObject = rayResult.m_collisionObject;
		if(normalInWorldSpace)
		{
			m_hitNormalWorld = rayResult.m_hitNormalLocal;
		}
		else
		{
			///need to transform normal into worldspace
			m_hitNormalWorld = m_collisionObject->getWorldTransform().getBasis() * rayResult.m_hitNormalLocal;
		}
		m_hitPointWorld.setInterpolate3(m_rayFromWorld, m_rayToWorld, rayResult.m_hitFraction);

		m_result.vHitPoint = BTVEC_F3(m_hitPointWorld);
		m_result.vHitNormal = BTVEC_F3(m_hitPointWorld);
		m_result.pCollisionObject = m_collisionObject->getUserIndex() == 2 ? (IXCollisionObject*)m_collisionObject->getUserPointer() : NULL;
		m_result.fHitFraction = rayResult.m_hitFraction;


		m_closestHitFraction = m_pCallback->addSingleResult(m_result);

		return(m_closestHitFraction);
	}
};

//#############################################################################

struct XConvexResultCallback: public btCollisionWorld::ConvexResultCallback
{
	XConvexResultCallback(IXConvexCallback *pCallback):
		m_pCallback(pCallback)
	{
		m_result._reserved = NULL;
	}

	IXConvexCallback *m_pCallback;

	btVector3 m_hitNormalWorld;

	XConvexResult m_result;

	btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace) override
	{
		//caller already does the filter on the m_closestHitFraction
		btAssert(convexResult.m_hitFraction <= m_closestHitFraction);

		//m_closestHitFraction = rayResult.m_hitFraction;
		// m_hitCollisionObject = convexResult.m_hitCollisionObject;
		if(normalInWorldSpace)
		{
			m_hitNormalWorld = convexResult.m_hitNormalLocal;
		}
		else
		{
			///need to transform normal into worldspace
			m_hitNormalWorld = convexResult.m_hitCollisionObject->getWorldTransform().getBasis() * convexResult.m_hitNormalLocal;
		}

		m_result.vHitPoint = BTVEC_F3(convexResult.m_hitPointLocal);
		m_result.vHitNormal = BTVEC_F3(m_hitNormalWorld);
		m_result.pCollisionObject = convexResult.m_hitCollisionObject->getUserIndex() == 2 ? (IXCollisionObject*)convexResult.m_hitCollisionObject->getUserPointer() : NULL;
		m_result.fHitFraction = convexResult.m_hitFraction;


		m_closestHitFraction = m_pCallback->addSingleResult(m_result);

		return(m_closestHitFraction);
	}
};

//#############################################################################

class CTaskScheduler: public btITaskScheduler
{
public:
	CTaskScheduler(): btITaskScheduler("SkyXEngine parallel")
	{
	}
	virtual int getMaxNumThreads() const BT_OVERRIDE{return(Core_MGetThreadCount()); }
	virtual int getNumThreads() const BT_OVERRIDE{return(Core_MGetThreadCount()); }
	virtual void setNumThreads(int numThreads) BT_OVERRIDE{}

	class CBody: public IParallelForBody
	{
	public:
		CBody(const btIParallelForBody *pBody):
			m_pBody(pBody)
		{
		}

		void forLoop(int iStart, int iEnd) const
		{
			XPROFILE_FUNCTION();
		//	Core_PStartSection(PERF_SECTION_PHYS_UPDATE);
			m_pBody->forLoop(iStart, iEnd);
		//	Core_PEndSection(PERF_SECTION_PHYS_UPDATE);
		};

	protected:
		const btIParallelForBody *m_pBody;
	};

	virtual void parallelFor(int iBegin, int iEnd, int grainSize, const btIParallelForBody& body) BT_OVERRIDE
	{
		XPROFILE_FUNCTION();
		BT_PROFILE("parallelFor_SkyXEngine");

		if(iBegin >= iEnd)
		{
			return;
		}

		if(Core_MGetThreadID() == 0 && (iEnd - iBegin) > grainSize)
		{
			CBody cbody(&body);
			Core_MWaitFor(Core_MForLoop(iBegin, iEnd, &cbody, grainSize));
		}
		else
		{
			body.forLoop(iBegin, iEnd);
		}
	}
	virtual btScalar parallelSum(int iBegin, int iEnd, int grainSize, const btIParallelSumBody& body) BT_OVERRIDE
	{
		XPROFILE_FUNCTION();
		BT_PROFILE("parallelSum_sequential");
		return(body.sumLoop(iBegin, iEnd));
	}
};

//#############################################################################

CPhyWorld::CPhyWorld()
{
	printf("Initializing physics engine...   ");
	m_isUpdating = false;
	m_pBroadphase = new btDbvtBroadphase();
	m_pCollisionConfiguration = new btDefaultCollisionConfiguration();
	m_pDispatcher = new btCollisionDispatcherMt(m_pCollisionConfiguration);
	m_pSolver = new btSequentialImpulseConstraintSolverMt();
	m_pGHostPairCallback = new btGhostPairCallback();
	m_pBroadphase->getOverlappingPairCache()->setInternalGhostPairCallback(m_pGHostPairCallback);

	btConstraintSolver *aSolvers[BT_MAX_THREAD_COUNT];
	int maxThreadCount = BT_MAX_THREAD_COUNT;
	for(int i = 0; i < maxThreadCount; ++i)
	{
		//btDantzigSolver* mlcp = new btDantzigSolver();
		//btMLCPSolver* sol = new btMLCPSolver(mlcp);
		aSolvers[i] = new btSequentialImpulseConstraintSolverMt();
	}
	btConstraintSolverPoolMt *pSolverPool = new btConstraintSolverPoolMt(aSolvers, maxThreadCount);
	
	m_pDynamicsWorld = new btDiscreteDynamicsWorldMt(m_pDispatcher, m_pBroadphase, pSolverPool, m_pSolver, m_pCollisionConfiguration);

	m_pDynamicsWorld->setForceUpdateAllAabbs(false);

	m_pDynamicsWorld->getSolverInfo().m_numIterations = 30;

	// typedef void (*btInternalTickCallback)(btDynamicsWorld *world, btScalar timeStep);

	m_pDynamicsWorld->setInternalTickCallback(TickCallback, this);

	//btCreateDefaultTaskScheduler();
	static CTaskScheduler taskSheduler;
	btSetTaskScheduler(&taskSheduler);
	
	Core_0RegisterCVarString("phy_world_gravity", "0 -10 0", "World gravity (x y z)");
	m_pDynamicsWorld->setGravity(btVector3(0, -10, 0));
			
	m_pDebugDrawer = new CDebugDrawer();
	m_pDebugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawConstraints | btIDebugDraw::DBG_DrawConstraintLimits);
	//m_pDebugDrawer->setDebugMode(btIDebugDraw::DBG_FastWireframe);
	m_pDynamicsWorld->setDebugDrawer(m_pDebugDrawer);

	Core_GetIXCore()->getPluginManager()->registerInterface(IXRENDERABLE_GUID, new CRenderable(this));

	Core_0RegisterCVarBool("r_physdebug", false, "Debug drawing physics shapes");
	m_bDebugDraw = GET_PCVAR_BOOL("r_physdebug");

	btSetCustomEnterProfileZoneFunc(CProfileManager::Start_Profile);
	btSetCustomLeaveProfileZoneFunc(CProfileManager::Stop_Profile);

	m_pTickEventChannel = Core_GetIXCore()->getEventChannel<XEventPhysicsStep>(EVENT_PHYSICS_STEP_GUID);
	
	printf("Done!\n");
}

CPhyWorld::~CPhyWorld()
{
	mem_delete(m_pDynamicsWorld);
	mem_delete(m_pGHostPairCallback);
	mem_delete(m_pSolver);
	mem_delete(m_pDispatcher);
	mem_delete(m_pCollisionConfiguration);
	mem_delete(m_pBroadphase);
}

void CPhyWorld::render()
{
	if(*m_bDebugDraw)
	{
		CDebugDrawer *pDebugDrawer = (CDebugDrawer*)m_pDynamicsWorld->getDebugDrawer();
		pDebugDrawer->begin();
		m_pDynamicsWorld->debugDrawWorld();
		pDebugDrawer->commit();
	}
}


void CPhyWorld::update()
{
	XPROFILE_FUNCTION();

	runQueue();

	if(!m_isRunning)
	{
		return;
	}
	if(--m_iSkipFrames >= 0)
	{
		return;
	}

	static UINT time0 = GetTickCount();
	UINT time1 = GetTickCount();

	if(time1 == time0)
	{
		return;
	}

	//printf("%.3fs\n", (float)(time1 - time0) / 1000.0f);

	m_isUpdating = true;
	
	m_pDynamicsWorld->stepSimulation((float)(time1 - time0) / 1000.0f, 2, 1.0f / 60.0f);
	
	m_isUpdating = false;

	time0 = time1;
}

void XMETHODCALLTYPE CPhyWorld::disableSimulation()
{
	m_isRunning = false;
}
void XMETHODCALLTYPE CPhyWorld::enableSimulation()
{
	m_isRunning = true;
	m_iSkipFrames = 3;
}

void CPhyWorld::TickCallback(btDynamicsWorld *world, btScalar timeStep)
{
	CPhyWorld *pThis = (CPhyWorld*)world->getWorldUserInfo();

	XEventPhysicsStep ev;
	ev.fTimeStep = timeStep;
	ev.pPhysics = NULL;

	pThis->m_pTickEventChannel->broadcastEvent(&ev);
}

void CPhyWorld::enqueue(QueueItem &&item)
{
	m_queue.emplace(std::move(item));

	if(Core_GetIXCore()->isOnMainThread() && !m_isUpdating)
	{
		runQueue();
	}
	else
	{
		int a =0;
	}
}
void CPhyWorld::runQueue()
{
	QueueItem i;

	while(m_queue.pop(&i))
	{
		switch(i.type)
		{
		case QIT_ADD_COLLISION_OBJECT:
			switch(i.pObj->getType())
			{
			case XCOT_INVALID:
				assert(!"Invalid type!");
				break;
			case XCOT_RIGID_BODY:
				{
					CRigidBody *pBody = (CRigidBody*)i.pObj->asRigidBody();
					pBody->setPhysWorld(this);
					m_pDynamicsWorld->addRigidBody(pBody->getBtRigidBody(), i.iGroup, i.iMask);
				}
				break;
			case XCOT_GHOST_OBJECT:
				{
					CGhostObject *pGhost = (CGhostObject*)i.pObj->asGhostObject();
					pGhost->setPhysWorld(this);
					m_pDynamicsWorld->addCollisionObject(pGhost->getBtGhostObject(), i.iGroup, i.iMask);
				}
				break;
			default:
				assert(!"Unknown type!");
			}
			break;

		case QIT_REMOVE_COLLISION_OBJECT:
			switch(i.pObj->getType())
			{
			case XCOT_INVALID:
				assert(!"Invalid type!");
				break;
			case XCOT_RIGID_BODY:
				{
					CRigidBody *pBody = (CRigidBody*)i.pObj->asRigidBody();
					assert(pBody->getPhysWorld());
					if(pBody->getPhysWorld())
					{
						pBody->setPhysWorld(NULL);
						m_pDynamicsWorld->removeRigidBody(pBody->getBtRigidBody());
					}
				}
				break;
			case XCOT_GHOST_OBJECT:
				{
					CGhostObject *pGhost = (CGhostObject*)i.pObj->asGhostObject();
					assert(pGhost->getPhysWorld());
					if(pGhost->getPhysWorld())
					{
						pGhost->setPhysWorld(NULL);
						m_pDynamicsWorld->removeCollisionObject(pGhost->getBtGhostObject());
					}
				}
				break;
			default:
				assert(!"Unknown type!");
			}
			i.pObj->Release();
			break;

		case QIT_UPDATE_SINGLE_AABB:
			if(i.pBtObj->getBroadphaseHandle())
			{
				m_pDynamicsWorld->updateSingleAabb(i.pBtObj);
			}
			break;
		}
		mem_release(i.pObj);
	}
}

void XMETHODCALLTYPE CPhyWorld::addCollisionObject(IXCollisionObject *pCollisionObject, COLLISION_GROUP collisionGroup, COLLISION_GROUP collisionMask)
{
	assert(pCollisionObject);

	// One ref for world, the second for queue
	add_ref(pCollisionObject);
	add_ref(pCollisionObject);
	enqueue({QIT_ADD_COLLISION_OBJECT, pCollisionObject, collisionGroup, collisionMask});
}
void XMETHODCALLTYPE CPhyWorld::removeCollisionObject(IXCollisionObject *pCollisionObject)
{
	if(!pCollisionObject)
	{
		return;
	}
	
	add_ref(pCollisionObject);
	enqueue({QIT_REMOVE_COLLISION_OBJECT, pCollisionObject});
}

void XMETHODCALLTYPE CPhyWorld::rayTest(const float3 &vFrom, const float3 &vTo, IXRayCallback *pCallback, COLLISION_GROUP collisionGroup, COLLISION_GROUP collisionMask)
{
	btVector3 from(F3_BTVEC(vFrom));
	btVector3 to(F3_BTVEC(vTo));
	XRayResultCallback cb(pCallback, from, to);

	cb.m_collisionFilterGroup = collisionGroup;
	cb.m_collisionFilterMask = collisionMask;

	m_pDynamicsWorld->rayTest(from, to, cb);
}

void XMETHODCALLTYPE CPhyWorld::convexSweepTest(IXConvexShape *pShape, const transform_t &xfFrom, const transform_t &xfTo, IXConvexCallback *pCallback, COLLISION_GROUP collisionGroup, COLLISION_GROUP collisionMask)
{
	btTransform xFormFrom;
	xFormFrom.setOrigin(F3_BTVEC(xfFrom.vPos));
	xFormFrom.getBasis().setRotation(Q4_BTQUAT(xfFrom.qRot));
	btTransform xFormTo;
	xFormTo.setOrigin(F3_BTVEC(xfTo.vPos));
	xFormTo.getBasis().setRotation(Q4_BTQUAT(xfTo.qRot));

	XConvexResultCallback cb(pCallback);

	cb.m_collisionFilterGroup = collisionGroup;
	cb.m_collisionFilterMask = collisionMask;

	m_pDynamicsWorld->convexSweepTest((btConvexShape*)GetCollisionShape(pShape), xFormFrom, xFormTo, cb);
}

void CPhyWorld::updateSingleAABB(IXCollisionObject *pObj, btCollisionObject *pBtObj)
{
	add_ref(pObj);
	enqueue({QIT_UPDATE_SINGLE_AABB, pObj, 0, 0, pBtObj});
}

//##############################################################

void XMETHODCALLTYPE CPhyWorld::CRenderable::renderStage(X_RENDER_STAGE stage, IXRenderableVisibility *pVisibility)
{
	m_pWorld->render();
}
void XMETHODCALLTYPE CPhyWorld::CRenderable::startup(IXRender *pRender, IXMaterialSystem *pMaterialSystem)
{
	((CDebugDrawer*)m_pWorld->getBtWorld()->getDebugDrawer())->setDevice(pRender);
}

//##############################################################

CPhyWorld::CDebugDrawer::CDebugDrawer()
{
}

CPhyWorld::CDebugDrawer::~CDebugDrawer()
{
	mem_release(m_pVertexBuffer);
	mem_release(m_pVertexDeclaration);
	mem_release(m_pRenderBuffer);
	mem_release(m_pVSConstantBuffer);
}

void CPhyWorld::CDebugDrawer::setDevice(IXRender *pRender)
{
	m_pDevice = pRender->getDevice();
	m_pRender = pRender;

	if(!m_pDevice)
	{
		return;
	}

	GXVertexElement vertexDecl[] =
	{
		{0, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_POSITION, GXDECLSPEC_PER_VERTEX_DATA},
		{0, 12, GXDECLTYPE_FLOAT4, GXDECLUSAGE_COLOR, GXDECLSPEC_PER_VERTEX_DATA},
		GX_DECL_END()
	};

	m_pVertexDeclaration = m_pDevice->createVertexDeclaration(vertexDecl);
	m_pVertexBuffer = m_pDevice->createVertexBuffer(sizeof(render_point)* m_uDataSize, GXBUFFER_USAGE_STREAM);
	m_pRenderBuffer = m_pDevice->createRenderBuffer(1, &m_pVertexBuffer, m_pVertexDeclaration);
	m_pVSConstantBuffer = m_pDevice->createConstantBuffer(sizeof(SMMATRIX));

	ID idVS = m_pRender->loadShader(SHADER_TYPE_VERTEX, "dbg_colorvertex.vs");
	ID idPS = m_pRender->loadShader(SHADER_TYPE_PIXEL, "dbg_colorvertex.ps");
	m_idShader = m_pRender->createShaderKit(idVS, idPS);
}

void CPhyWorld::CDebugDrawer::drawLine(const btVector3 & from, const btVector3 & to, const btVector3 & color)
{
	if(m_uDataPointer == m_uDataSize)
	{
		render();
	}

	render_point pt = {BTVEC_F3(from), float4(BTVEC_F3(color), 1.0f)};

	m_pDrawData[m_uDataPointer++] = pt;

	pt.pos = BTVEC_F3(to);
	m_pDrawData[m_uDataPointer++] = pt;
}

void CPhyWorld::CDebugDrawer::drawContactPoint(const btVector3 & PointOnB, const btVector3 & normalOnB, btScalar distance, int lifeTime, const btVector3& color)
{

}

void CPhyWorld::CDebugDrawer::reportErrorWarning(const char * warningString)
{
	if(m_bExpectObject)
	{
		m_bExpectObject = false;
		btCollisionObject *pObj = (btCollisionObject*)warningString;

		btVector3 vOrigin = pObj->getWorldTransform().getOrigin();
		LibReport(REPORT_MSG_LEVEL_WARNING, "Object world position: %.2f, %.2f, %.2f\n", vOrigin.x(), vOrigin.y(), vOrigin.z());

		MessageBoxA(NULL, "found", "bug", MB_OK | MB_ICONSTOP);

		btVector3 minAabb, maxAabb;
		pObj->getCollisionShape()->getAabb(pObj->getWorldTransform(), minAabb, maxAabb);
		LibReport(REPORT_MSG_LEVEL_WARNING, "AABBmin: %.2f, %.2f, %.2f\n", minAabb.x(), minAabb.y(), minAabb.z());
		LibReport(REPORT_MSG_LEVEL_WARNING, "AABBmax: %.2f, %.2f, %.2f\n", maxAabb.x(), maxAabb.y(), maxAabb.z());
		if(pObj->getCollisionShape()->getShapeType() == CAPSULE_SHAPE_PROXYTYPE)
		{
			btCapsuleShape *pCaps = (btCapsuleShape*)pObj->getCollisionShape();
			LibReport(REPORT_MSG_LEVEL_WARNING, "Radius: %.2f; HalfHeight: %.2f\n", pCaps->getRadius(), pCaps->getHalfHeight());
			SMMATRIX mat;
			pObj->getWorldTransform().getOpenGLMatrix((float*)&mat);
			LibReport(REPORT_MSG_LEVEL_WARNING, "Transform:\n");
			LibReport(REPORT_MSG_LEVEL_WARNING, " %5.4f %5.4f %5.4f %5.4f\n", mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3]);
			LibReport(REPORT_MSG_LEVEL_WARNING, " %5.4f %5.4f %5.4f %5.4f\n", mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3]);
			LibReport(REPORT_MSG_LEVEL_WARNING, " %5.4f %5.4f %5.4f %5.4f\n", mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3]);
			LibReport(REPORT_MSG_LEVEL_WARNING, " %5.4f %5.4f %5.4f %5.4f\n", mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]);
		}

		btTransform &trans = pObj->getWorldTransform();

		LibReport(REPORT_MSG_LEVEL_WARNING, " 0x%08xf\n", &trans);
		byte * pByte = (byte*)&trans;
		for(int i = 0, l = sizeof(trans); i < l; ++i)
		{
			LibReport(REPORT_MSG_LEVEL_WARNING, " %02x", pByte[i]);
		}

		if(!pObj->getUserPointer())
		{
			LibReport(REPORT_MSG_LEVEL_WARNING, "getUserPointer() is NULL.\n");
			return;
		}
		Core_0ConsoleExecCmd("ent_dump_info %x", pObj->getUserPointer());
		return;
	}
	if(!strcmp(warningString, "@@@obj"))
	{
		m_bExpectObject = true;
		return;
	}

	LibReport(REPORT_MSG_LEVEL_WARNING, "%s", warningString);
}

void CPhyWorld::CDebugDrawer::draw3dText(const btVector3& location, const char* textString)
{

}

void CPhyWorld::CDebugDrawer::setDebugMode(int debugMode)
{
	m_iDebugMode = debugMode;
}

int CPhyWorld::CDebugDrawer::getDebugMode() const
{
	return(m_iDebugMode);
}

void CPhyWorld::CDebugDrawer::begin()
{
	m_uDataPointer = 0;

	IGXContext *pCtx = m_pDevice->getThreadContext();

	m_pRender->bindShader(pCtx, m_idShader);

	pCtx->setRenderBuffer(m_pRenderBuffer);
	pCtx->setPrimitiveTopology(GXPT_LINELIST);

	// SMMATRIX mViewProj, mWorld;
	// Core_RMatrixGet(G_RI_MATRIX_VIEWPROJ, &mViewProj);
	// Core_RMatrixGet(G_RI_MATRIX_WORLD, &mWorld);
	// m_pVSConstantBuffer->update(&SMMatrixTranspose(mWorld * mViewProj));
	// pCtx->setVSConstant(m_pVSConstantBuffer, 4);
}

void CPhyWorld::CDebugDrawer::commit()
{
	IGXContext *pCtx = m_pDevice->getThreadContext();

	render();

	m_pRender->unbindShader(pCtx);
	pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);
}

void CPhyWorld::CDebugDrawer::render()
{
	if(!m_uDataPointer)
	{
		return;
	}

	render_point *pData = NULL;
	if(m_pVertexBuffer->lock((void**)&pData, GXBL_WRITE))
	{
		memcpy(pData, m_pDrawData, sizeof(render_point) * m_uDataPointer);
		m_pVertexBuffer->unlock();

		m_pDevice->getThreadContext()->drawPrimitive(0, m_uDataPointer / 2);
	}

	m_uDataPointer = 0;
}
