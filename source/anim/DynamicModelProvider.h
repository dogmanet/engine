#ifndef __DYNAMICMODELPROVIDER_H
#define __DYNAMICMODELPROVIDER_H

#include <xcommon/resource/IXModelProvider.h>
#include "DynamicModel.h"
#include "DynamicModelShared.h"
#include <xcommon/IXCore.h>
#include <mtrl/IXMaterialSystem.h>
#include "RenderableVisibility.h"
#include <common/ConcurrentQueue.h>
#include <xcommon/XEvents.h>
#include <xcommon/IXScene.h>
#include <common/queue.h>

class CDynamicModelProvider;
class CMaterialChangedEventListener: public IEventListener<XEventMaterialChanged>
{
public:
	CMaterialChangedEventListener(CDynamicModelProvider *pProvider);
	void onEvent(const XEventMaterialChanged *pData) override;

protected:
	CDynamicModelProvider *m_pProvider;
};

class CDynamicModelProvider: public IXUnknownImplementation<IXDynamicModelProvider>
{
public:
	friend class CMaterialChangedEventListener;

	CDynamicModelProvider(IXCore *pCore);
	~CDynamicModelProvider();

	bool XMETHODCALLTYPE createModel(IXResourceModel *pResource, IXDynamicModel **ppModel) override;

	void onSharedModelReady(CDynamicModelShared *pShared);
	void onSharedModelRelease(CDynamicModelShared *pShared);
	void onSharedModelFeaturesChanged(CDynamicModelShared *pShared);
	void onModelRelease(CDynamicModel *pModel);
	IXMaterialSystem* getMaterialSystem();
	IGXDevice* getDevice();
	IXCore* getCore();
	void setDevice(IGXDevice *pContext);
	IGXVertexDeclaration *getVertexDeclaration();

	void render(bool isTransparent, CRenderableVisibility *pVisibility = NULL);
	void renderEmissive(CRenderableVisibility *pVisibility);
	void computeVisibility(const IXFrustum *pFrustum, const float3 &vHintDir, CRenderableVisibility *pVisibility, UINT bmLayerMask, CRenderableVisibility *pReference=NULL, IXCamera *pCamera = NULL);

	void render(Array<CDynamicModel*> &aRenderList, XMODEL_FEATURE bmWhat);

	void getLevelSize(const XEventLevelSize *pData);

	void update();
	void sync();

	void scheduleSharedGPUinit(CDynamicModelShared *pShared);
	void scheduleModelGPUinit(CDynamicModel *pModel);

	UINT getTransparentCount(CRenderableVisibility *pVisibility);
	void getTransparentObject(CRenderableVisibility *pVisibility, UINT uIndex, XTransparentObject *pObject);
	void renderTransparentObject(CRenderableVisibility *pVisibility, UINT uIndex, UINT uSplitPlanes);

	void notifyModelChanged(CDynamicModel *pModel, XEventModelChanged::TYPE type);

	void bindVertexFormat(bool forInstancing = false);

	IXSceneObjectType* getSceneObjectType();

	IXSceneFeature* getFeature(XMODEL_FEATURE bmFeature);

	void enqueueModelDelete(CDynamicModel* pModel);
protected:
	void onMaterialEmissivityChanged(const IXMaterial *pMaterial);
	void onMaterialTransparencyChanged(const IXMaterial *pMaterial);

	CMaterialChangedEventListener *m_pMaterialChangedEventListener;

	IEventChannel<XEventModelChanged> *m_pModelChangedEventChannel;

	AssotiativeArray<IXResourceModel*, CDynamicModelShared*> m_mModels;

	Array<CDynamicModel*> m_apModels;

	IGXVertexDeclaration *m_pVertexDeclaration = NULL;

	IXCore *m_pCore;
	IGXDevice *m_pRenderContext = NULL;

	Queue<CDynamicModelShared*> m_queueGPUinitShared;
	Queue<CDynamicModel*> m_queueGPUinitModel;

	IXMaterialSystem *m_pMaterialSystem = NULL;
	XVertexShaderHandler *m_pVertexShaderHandler = NULL;
	XVertexShaderHandler *m_pVertexShaderInstancedHandler = NULL;

	IXScene *m_pScene = NULL;
	IXSceneObjectType *m_pObjectType = NULL;
	IXSceneFeature *m_pFeatureOpaque = NULL;
	IXSceneFeature *m_pFeatureTransparent = NULL;
	IXSceneFeature *m_pFeatureSelfillum = NULL;
	IXSceneQuery *m_pOpaqueQuery = NULL;
	IXSceneQuery *m_pTransparentQuery = NULL;
	IXSceneQuery *m_pSelfillumQuery = NULL;

	Queue<CDynamicModel*> m_qModelDelete;

	SpinLock m_slModels;
};

#endif
