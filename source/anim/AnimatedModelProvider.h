#ifndef __ANIMATEDMODELPROVIDER_H
#define __ANIMATEDMODELPROVIDER_H

#include <xcommon/resource/IXModelProvider.h>
#include "AnimatedModel.h"
#include "AnimatedModelShared.h"
#include <xcommon/IXCore.h>
#include <mtrl/IXMaterialSystem.h>
#include "RenderableVisibility.h"
#include <common/ConcurrentQueue.h>

class CAnimatedModelProvider: public IXUnknownImplementation<IXAnimatedModelProvider>
{
public:
	CAnimatedModelProvider(IXCore *pCore);
	~CAnimatedModelProvider();

	bool XMETHODCALLTYPE createModel(UINT uResourceCount, IXResourceModelAnimated **ppResources, IXAnimatedModel **ppModel) override;

	void onSharedModelRelease(CAnimatedModelShared *pShared);
	void onModelRelease(CAnimatedModel *pModel);
	IXMaterialSystem *getMaterialSystem();
	IGXDevice *getDevice();
	IXCore* getCore();
	void setDevice(IGXDevice *pDevice);
	IGXVertexDeclaration *getVertexDeclaration();

	void update(float fDT);
	void sync();
	void render(CRenderableVisibility *pVisibility = NULL);
	void computeVisibility(const IXFrustum *pFrustum, CRenderableVisibility *pVisibility, UINT bmLayerMask, CRenderableVisibility *pReference=NULL);

	void getLevelSize(const XEventLevelSize *pData);

	void scheduleSharedGPUinit(CAnimatedModelShared *pShared);
	void scheduleModelGPUinit(CAnimatedModel *pModel);

	void bindVertexFormat();

protected:
	AssotiativeArray<IXResourceModelAnimated*, Array<CAnimatedModelShared*>> m_mModels;

	Array<CAnimatedModel*> m_apModels;

	IGXVertexDeclaration *m_pVertexDeclaration = NULL;

	IXCore *m_pCore;
	IGXDevice *m_pRenderContext = NULL;

	CConcurrentQueue<CAnimatedModelShared*> m_queueGPUinitShared;
	CConcurrentQueue<CAnimatedModel*> m_queueGPUinitModel;

	IXMaterialSystem *m_pMaterialSystem = NULL;
	XVertexShaderHandler *m_pVertexShaderHandler = NULL;

	SpinLock m_slModels;
};

#endif
