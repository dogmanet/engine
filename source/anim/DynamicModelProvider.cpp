#include "DynamicModelProvider.h"
#include <xcommon/IPluginManager.h>
#include <core/sxcore.h>

CMaterialChangedEventListener::CMaterialChangedEventListener(CDynamicModelProvider *pProvider):
	m_pProvider(pProvider)
{
}
void CMaterialChangedEventListener::onEvent(const XEventMaterialChanged *pData)
{
	if(pData->type == XEventMaterialChanged::TYPE_TRANSPARENCY)
	{
		m_pProvider->onMaterialTransparencyChanged(pData->pMaterial);
	}
	if(pData->type == XEventMaterialChanged::TYPE_EMISSIVITY)
	{
		m_pProvider->onMaterialEmissivityChanged(pData->pMaterial);
	}
}
/*
//##########################################################################

class CRenderTask: public IParallelForBody
{
public:
	CRenderTask(CRenderableVisibility *pVisibility, Array<CDynamicModel*> *pModels, IGXDevice *pDevice, IGXContextState *pState):
		m_pVisibility(pVisibility),
		m_pModels(pModels),
		m_pDevice(pDevice),
		m_pState(pState)
	{
	}

	void forLoop(int iStart, int iEnd) const
	{
		//printf("%d %d\n", iStart, iEnd);
		//printf("%d\n", Core_MGetThreadID());
		m_pDevice->getThreadContext()->setFullState(m_pState);

		m_pDevice->getThreadContext()->beginIndirect();

		for(UINT i = iStart, l = iEnd; i < l; ++i)
		{
			//if(iStart != 279 && iStart != 558 && iStart != 837)
			//{
			//	break;
			//}
			//if(iStart == 558)
			//{
			//	//break;
			//}
			//if(iStart == 837)
			//{
			//	//break;
			//}
			if(m_pVisibility)
			{
				auto pItem = m_pVisibility->getItemDynamic(i, false);
				if(pItem && pItem->isVisible)
				{
					(*m_pModels)[i]->render(pItem->uLod, false);
					//break;
				}
			}
			else
			{
				(*m_pModels)[i]->render(0, false);
			}
		}

		//Sleep(2);
		m_pDevice->getThreadContext()->endIndirect();
	};

protected:
	CRenderableVisibility *m_pVisibility;
	Array<CDynamicModel*> *m_pModels;
	IGXDevice *m_pDevice;
	IGXContextState *m_pState;
};
*/

#if 0
class CVisUpdate: public IParallelForBody
{
public:
	CVisUpdate(const IXFrustum *pFrustum, CRenderableVisibility *pVisibility, CRenderableVisibility *pReference, Array<CDynamicModel*> &m_apModels):
		m_pFrustum(pFrustum),
		m_pVisibility(pVisibility),
		m_pReference(pReference),
		m_apModels(m_apModels)
	{
	}

	void forLoop(int iStart, int iEnd) const
	{
		CDynamicModel *pMdl;
		for(int i = iStart; i < iEnd; ++i)
		{
			pMdl = m_apModels[i];
			auto *pItem = m_pVisibility->getItemDynamic(i);
			if(pMdl->isEnabled())
			{
				float3 vDelta = pMdl->getPosition();
				pItem->isVisible = (m_pReference ? m_pReference->getItemDynamic(i)->isVisible : true)
					&& m_pFrustum->boxInFrustum(pMdl->getLocalBound() + vDelta);

				if(pItem->isVisible)
				{
					pItem->isEmissive = pMdl->hasEmissiveSubsets(pItem->uLod);
					pItem->isTransparent = pMdl->hasTransparentSubsets(pItem->uLod);
					if(pItem->isTransparent)
					{
						IXMaterial *pMaterial = pMdl->getTransparentMaterial(pItem->uLod);
						UINT uPSPcount = pMdl->getPSPcount(pItem->uLod);
						{
							ScopedLock lock(m_transparency);
							if(uPSPcount)
							{
								m_pVisibility->addItemTransparentDynamic({pMdl, true, pMdl->getPSP(pItem->uLod, 0), 0, pMaterial});
							}
							else
							{
								m_pVisibility->addItemTransparentDynamic({pMdl, false, SMPLANE(), 0, pMaterial});
							}
						}
						mem_release(pMaterial);
					}
				}
			}
			else
			{
				pItem->isVisible = false;
			}
		}
	};

protected:
	const IXFrustum *m_pFrustum;
	CRenderableVisibility *m_pVisibility;
	CRenderableVisibility *m_pReference;
	Array<CDynamicModel*> &m_apModels;
	mutable mutex m_transparency;
};
#endif 

//##########################################################################

CDynamicModelProvider::CDynamicModelProvider(IXCore *pCore):
	m_pCore(pCore)
{
	m_pMaterialChangedEventListener = new CMaterialChangedEventListener(this);
	pCore->getEventChannel<XEventMaterialChanged>(EVENT_MATERIAL_CHANGED_GUID)->addListener(m_pMaterialChangedEventListener);
	m_pModelChangedEventChannel = pCore->getEventChannel<XEventModelChanged>(EVENT_MODEL_CHANGED_GUID);

	m_pScene = (IXScene*)pCore->getPluginManager()->getInterface(IXSCENE_GUID);
	m_pObjectType = m_pScene->registerObjectType("xDynamic");
	m_pFeatureOpaque = m_pScene->registerObjectFeature("xOpaque");
	m_pFeatureTransparent = m_pScene->registerObjectFeature("xTransparent");
	m_pFeatureSelfillum = m_pScene->registerObjectFeature("xSelfillum");

	m_pOpaqueQuery = m_pObjectType->newQuery();
	m_pOpaqueQuery->setFeature(m_pFeatureOpaque, SQF_SET);

	m_pTransparentQuery = m_pObjectType->newQuery();
	m_pTransparentQuery->setFeature(m_pFeatureTransparent, SQF_SET);

	m_pSelfillumQuery = m_pObjectType->newQuery();
	m_pSelfillumQuery->setFeature(m_pFeatureSelfillum, SQF_SET);
}

CDynamicModelProvider::~CDynamicModelProvider()
{
	m_pCore->getEventChannel<XEventMaterialChanged>(EVENT_MATERIAL_CHANGED_GUID)->removeListener(m_pMaterialChangedEventListener);
	mem_delete(m_pMaterialChangedEventListener);
	mem_release(m_pVertexDeclaration);
	mem_release(m_pOpaqueQuery);
	mem_release(m_pTransparentQuery);
	mem_release(m_pSelfillumQuery);
}

IGXVertexDeclaration *CDynamicModelProvider::getVertexDeclaration()
{
	return(m_pVertexDeclaration);
}

IGXDevice* CDynamicModelProvider::getDevice()
{
	return(m_pRenderContext);
}

IXCore* CDynamicModelProvider::getCore()
{
	return(m_pCore);
}

void CDynamicModelProvider::setDevice(IGXDevice *pDevice)
{
	m_pRenderContext = pDevice;

#if 0
	// 56 bytes
	GXVertexElement layoutStaticEx[] =
	{
		{0, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_POSITION},
		{0, 12, GXDECLTYPE_FLOAT2, GXDECLUSAGE_TEXCOORD},
		{0, 20, GXDECLTYPE_FLOAT3, GXDECLUSAGE_NORMAL},
		{0, 32, GXDECLTYPE_FLOAT3, GXDECLUSAGE_TANGENT},
		{0, 44, GXDECLTYPE_FLOAT3, GXDECLUSAGE_BINORMAL},
		GX_DECL_END()
	};
#endif
	// 40 bytes/ 28?
	GXVertexElement layoutStaticEx[] =
	{
		{0, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_POSITION},
		{0, 12, GXDECLTYPE_FLOAT2, GXDECLUSAGE_TEXCOORD},
		{0, 20, GXDECLTYPE_SHORT4N, GXDECLUSAGE_NORMAL},
		{0, 28, GXDECLTYPE_SHORT4N, GXDECLUSAGE_TANGENT},
		{0, 36, GXDECLTYPE_SHORT4N, GXDECLUSAGE_BINORMAL},
		GX_DECL_END()
	};

	m_pVertexDeclaration = pDevice->createVertexDeclaration(layoutStaticEx);

	m_pMaterialSystem = (IXMaterialSystem*)m_pCore->getPluginManager()->getInterface(IXMATERIALSYSTEM_GUID);
	XVertexFormatHandler *pFormat = m_pMaterialSystem->getVertexFormat("xSceneGeneric");
	m_pVertexShaderHandler = m_pMaterialSystem->registerVertexShader(pFormat, "base/static.vs");

	GXMacro macroInstanced[] = {
		{"USE_INSTANCING", "1"},
		{"MAX_INSTANCES", MACRO_TEXT(MAX_INSTANCES)},
		GX_MACRO_END()
	};

	m_pVertexShaderInstancedHandler = m_pMaterialSystem->registerVertexShader(pFormat, "base/static.vs", macroInstanced);
}

bool XMETHODCALLTYPE CDynamicModelProvider::createModel(IXResourceModel *pResource, IXDynamicModel **ppModel)
{
	CDynamicModelShared *pShared = NULL;
	CDynamicModelShared **ppShared = NULL;

	{
		ScopedSpinLock lock(m_slModels);
		const AssotiativeArray<IXResourceModel*, CDynamicModelShared*>::Node *pNode;
		if(m_mModels.KeyExists(pResource, &pNode))
		{
			pShared = *(pNode->Val);
		}

		ppShared = &m_mModels[pResource];
	}

	if(!pShared)
	{
		if(pResource->getType() != XMT_STATIC)
		{
			return(false);
		}

		pShared = new CDynamicModelShared(this);
		if(!pShared->init(pResource->asStatic()))
		{
			mem_delete(pShared);
			return(false);
		}
		*ppShared = pShared;
	}
	else
	{
		pShared->AddRef();
	}

	CDynamicModel *pModel = new CDynamicModel(this, pShared);
	m_apModels.push_back(pModel);

	pShared->Release();

	*ppModel = pModel;
	return(true);
}

void CDynamicModelProvider::onSharedModelReady(CDynamicModelShared *pShared)
{
	// CDynamicModel *pTmp;
	// for(UINT i = 0, l = m_apModels.size(); i < l; ++i)
	// {
	// 	pTmp = m_apModels[i];
	// 	if(pTmp->getShared() == pShared)
	// 	{
	// 		notifyModelCreated(pTmp);
	// 	}
	// }
}
void CDynamicModelProvider::onSharedModelRelease(CDynamicModelShared *pShared)
{
	ScopedSpinLock lock(m_slModels);
	m_mModels.erase(pShared->getResource());
}
void CDynamicModelProvider::onSharedModelFeaturesChanged(CDynamicModelShared *pShared)
{
	CDynamicModel *pTmp;
	for(UINT i = 0, l = m_apModels.size(); i < l; ++i)
	{
		pTmp = m_apModels[i];
		if(pTmp->getShared() == pShared)
		{
			pTmp->onFeaturesChanged();
		}
	}
}

void CDynamicModelProvider::onModelRelease(CDynamicModel *pModel)
{
	for(UINT i = 0, l = m_apModels.size(); i < l; ++i)
	{
		if(m_apModels[i] == pModel)
		{
			m_apModels.erase(i);
			return;
		}
	}

	assert(!"Something went wrong! Should never get here!");
}

IXMaterialSystem *CDynamicModelProvider::getMaterialSystem()
{
	return((IXMaterialSystem*)m_pCore->getPluginManager()->getInterface(IXMATERIALSYSTEM_GUID));
}

void CDynamicModelProvider::bindVertexFormat(bool forInstancing)
{
	m_pMaterialSystem->bindVS(forInstancing ? m_pVertexShaderInstancedHandler : m_pVertexShaderHandler);
}

void CDynamicModelProvider::render(Array<CDynamicModel*> &aRenderList, XMODEL_FEATURE bmWhat)
{
	bindVertexFormat();
	auto &rl = aRenderList;

	CDynamicModel *pFirstInGroup = NULL;
	CDynamicModel *pCur = NULL;
	bool isInstancingStarted = false;
	for(UINT i = 0, l = rl.size(); i < l; ++i)
	{
		pCur = rl[i];
		if(!pFirstInGroup)
		{
			pFirstInGroup = pCur;
		}
		else
		{
			if(pFirstInGroup->getShared() != pCur->getShared() || pFirstInGroup->getSkin() != pCur->getSkin())
			{
				// new group
				if(isInstancingStarted)
				{
					isInstancingStarted = false;
					pFirstInGroup->getShared()->endInstancing();
				}
				else
				{
					pFirstInGroup->render(0, bmWhat);
				}
				pFirstInGroup = pCur;
			}
			else
			{
				// start instancing
				if(!isInstancingStarted)
				{
					isInstancingStarted = true;
					pFirstInGroup->getShared()->beginInstancing(pFirstInGroup->getSkin(), 0, bmWhat);
					pFirstInGroup->render(0, bmWhat);
				}
				pCur->render(0, bmWhat);
			}
		}
	}
	if(isInstancingStarted)
	{
		pFirstInGroup->getShared()->endInstancing();
	}
	else if(pFirstInGroup)
	{
		pFirstInGroup->render(0, bmWhat);
	}

	m_pMaterialSystem->bindVS(NULL);
}

void CDynamicModelProvider::render(bool isTransparent, CRenderableVisibility *pVisibility)
{
	if(!pVisibility/* || isTransparent*/)
	{
		return;
	}

	XPROFILE_FUNCTION();

	render(pVisibility->getRenderList(), MF_OPAQUE);

	if(isTransparent)
	{
		render(pVisibility->getTransparentList(), MF_TRANSPARENT);
	}

#if 0
	return;

	bindVertexFormat();

	auto &rl = pVisibility->getRenderList();

	CDynamicModel *pFirstInGroup = NULL;
	CDynamicModel *pCur = NULL;
	bool isInstancingStarted = false;
	for(UINT i = 0, l = rl.size(); i < l; ++i)
	{
		pCur = rl[i];
		if(!pFirstInGroup)
		{
			pFirstInGroup = pCur;
		}
		else
		{
			if(pFirstInGroup->getShared() != pCur->getShared() || pFirstInGroup->getSkin() != pCur->getSkin())
			{
				// new group
				if(isInstancingStarted)
				{
					isInstancingStarted = false;
					pFirstInGroup->getShared()->endInstancing();
				}
				else
				{
					pFirstInGroup->render(0, MF_OPAQUE);
				}
				pFirstInGroup = pCur;
			}
			else
			{
				// start instancing
				if(!isInstancingStarted)
				{
					isInstancingStarted = true;
					pFirstInGroup->getShared()->beginInstancing(pFirstInGroup->getSkin(), 0);
					pFirstInGroup->render(0, MF_OPAQUE);
				}
				pCur->render(0, MF_OPAQUE);
			}
		}
	}
	if(isInstancingStarted)
	{
		pFirstInGroup->getShared()->endInstancing();
	}
	else if(pFirstInGroup)
	{
		pFirstInGroup->render(0, MF_OPAQUE);
	}

#if 0
	//if(isTransparent/* || m_apModels.size() < 1000*/)
	//{
		for(UINT i = 0, l = m_apModels.size(); i < l; ++i)
		{
			if(pVisibility)
			{
				auto pItem = pVisibility->getItemDynamic(i);
				if(pItem->isVisible)
				{
					if(isTransparent && !pItem->isTransparent)
					{
						continue;
					}
					m_apModels[i]->render(pItem->uLod, isTransparent);
				}
			}
			else
			{
				m_apModels[i]->render(0, isTransparent);
			}
		}
	//}
#endif
	/*else
	{
		IGXContextState *pState = m_pRenderContext->getThreadContext()->getCurrentState();
		CRenderTask task(pVisibility, &m_apModels, m_pRenderContext, pState);

		int iThreads = Core_MGetThreadCount();
		int iChunkSize = ((int)m_apModels.size() + (iThreads - 1)) / iThreads;

		ID idTask = Core_MForLoop(iChunkSize, m_apModels.size(), &task);

		for(UINT i = 0, l = iChunkSize; i < l; ++i)
		{
			if(pVisibility)
			{
				auto pItem = pVisibility->getItemDynamic(i);
				if(pItem->isVisible)
				{
					if(isTransparent && !pItem->isTransparent)
					{
						continue;
					}
					m_apModels[i]->render(pItem->uLod, isTransparent);
				}
			}
			else
			{
				m_apModels[i]->render(0, isTransparent);
			}
		}
		
		Core_MWaitFor(idTask);

		mem_release(pState);

		m_pRenderContext->executeThreadContexts();
		//printf("\n");
	}*/
	m_pMaterialSystem->bindVS(NULL);
#endif
}

void CDynamicModelProvider::renderEmissive(CRenderableVisibility *pVisibility)
{
	if(!pVisibility/* || isTransparent*/)
	{
		return;
	}

	render(pVisibility->getSelfillumList(), MF_SELFILLUM);
}

void CDynamicModelProvider::computeVisibility(const IXFrustum *pFrustum, const float3 &vHintDir, CRenderableVisibility *pVisibility, UINT bmLayerMask, CRenderableVisibility *pReference, IXCamera *pCamera)
{
	XPROFILE_FUNCTION();

	if(pCamera)
	{
		FIXME("Use actual target width!");
		static const int *r_win_width = GET_PCVAR_INT("r_win_width");
		//static const float *r_default_fov = GET_PCVAR_FLOAT("r_default_fov");
		m_pOpaqueQuery->setScreenSizeCulling(pCamera->getPosition(), pCamera->getEffectiveFOV(), *r_win_width, 4.0f);
	}
	else
	{
		m_pOpaqueQuery->unsetScreenSizeCulling();
	}

	void **ppData;
	m_pOpaqueQuery->setLayerMask(bmLayerMask);
	UINT uCount = m_pOpaqueQuery->execute(pFrustum, vHintDir, &ppData, pVisibility->getCuller());
	pVisibility->setRenderList(ppData, uCount);

	m_pSelfillumQuery->setLayerMask(bmLayerMask);
	uCount = m_pSelfillumQuery->execute(pFrustum, vHintDir, &ppData, pVisibility->getCuller());
	pVisibility->setSelfillumList(ppData, uCount);

	m_pTransparentQuery->setLayerMask(bmLayerMask);
	uCount = m_pTransparentQuery->execute(pFrustum, vHintDir, &ppData, pVisibility->getCuller());
	pVisibility->setTransparentList(ppData, uCount);

	{
		//pVisibility->resetItemTransparentDynamic();
		pVisibility->setItemTransparentCountDynamic(uCount);
		auto &rl = pVisibility->getTransparentList();
		CDynamicModel *pMdl;
		for(UINT i = 0, l = rl.size(); i < l; ++i)
		{
			pMdl = rl[i];

			float3 vDelta = pMdl->getPosition();
			
			IXMaterial *pMaterial = pMdl->getTransparentMaterial(0);
			UINT uPSPcount = pMdl->getPSPcount(0);
			auto *pItem = pVisibility->getItemTransparentDynamic(i);
			pItem->pReferenceMdl = pMdl;
			pItem->uLod = 0;
			pItem->pMaterial = pMaterial;
			if(uPSPcount)
			{
				pItem->hasPSP = true;
				pItem->psp = pMdl->getPSP(0, 0);
			}
			else
			{
				pItem->hasPSP = false;
			}
			mem_release(pMaterial);
		}
	}

#if 0
	return;
#if 1
	CVisUpdate cycle(pFrustum, pVisibility, pReference, m_apModels);
	ID id = m_pCore->forLoop(0, m_apModels.size(), &cycle, 500);
	m_pCore->waitForLoop(id);
#else
	CDynamicModel *pMdl;
	for(UINT i = 0, l = m_apModels.size(); i < l; ++i)
	{
		pMdl = m_apModels[i];
		auto *pItem = pVisibility->getItemDynamic(i);
		if(pMdl->isEnabled())
		{
			float3 vDelta = pMdl->getPosition();
			pItem->isVisible = (pReference ? pReference->getItemDynamic(i)->isVisible : true)
				&& pFrustum->boxInFrustum(&float3(pMdl->getLocalBoundMin() + vDelta), &float3(pMdl->getLocalBoundMax() + vDelta));

			if(pItem->isVisible)
			{
				pItem->isTransparent = pMdl->hasTransparentSubsets(pItem->uLod);
				pItem->isEmissive = pMdl->hasEmissiveSubsets(pItem->uLod);
				if(pItem->isTransparent)
				{
					IXMaterial *pMaterial = pMdl->getTransparentMaterial(pItem->uLod);
					UINT uPSPcount = pMdl->getPSPcount(pItem->uLod);
					if(uPSPcount)
					{
						pVisibility->addItemTransparentDynamic({pMdl, true, pMdl->getPSP(pItem->uLod, 0), 0, pMaterial});
					}
					else
					{
						pVisibility->addItemTransparentDynamic({pMdl, false, SMPLANE(), 0, pMaterial});
					}
					mem_release(pMaterial);
				}
			}
		}
		else
		{
			pItem->isVisible = false;
		}
	}
#endif
#endif
}

void CDynamicModelProvider::getLevelSize(const XEventLevelSize *pData)
{
	CDynamicModel *pMdl;
	float3 vMax, vMin;
	for(UINT i = 0, l = m_apModels.size(); i < l; ++i)
	{
		pMdl = m_apModels[i];
		if(pMdl->isEnabled())
		{
			float3 vDelta = pMdl->getPosition();
			vMin = pMdl->getLocalBoundMin() + vDelta;
			vMax = pMdl->getLocalBoundMax() + vDelta;

			if(pData->vMax == pData->vMin)
			{
				pData->vMax = vMax;
				pData->vMin = vMin;
			}
			else
			{
				pData->vMax = SMVectorMax(pData->vMax, vMax);
				pData->vMin = SMVectorMin(pData->vMin, vMin);
			}

		}
	}
}

void CDynamicModelProvider::update()
{
	typedef std::chrono::high_resolution_clock::time_point time_point;
	time_point tStart = std::chrono::high_resolution_clock::now();

	UINT uCnt = 0;

	CDynamicModelShared *pShared;
	while(m_queueGPUinitShared.pop(&pShared))
	{
		pShared->initGPUresources();
		mem_release(pShared);

		if((++uCnt & 0xff) == 0 && ((float)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - tStart).count() / 1000000.0f) > 0.0001f)
		{
			break;
		}
	}

	CDynamicModel *pModel;
	while(m_queueGPUinitModel.pop(&pModel))
	{
		pModel->initGPUresources();
		mem_release(pModel);

		if((++uCnt & 0xff) == 0 && ((float)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - tStart).count() / 1000000.0f) > 0.0001f)
		{
			break;
		}
	}
}

void CDynamicModelProvider::sync()
{
	CDynamicModel *pMdl;
	while(m_qModelDelete.pop(&pMdl))
	{
		mem_delete(pMdl);
	}
}

void CDynamicModelProvider::scheduleSharedGPUinit(CDynamicModelShared *pShared)
{
	pShared->AddRef();
	m_queueGPUinitShared.push(pShared);
}

void CDynamicModelProvider::scheduleModelGPUinit(CDynamicModel *pModel)
{
	pModel->AddRef();
	m_queueGPUinitModel.push(pModel);
}

void CDynamicModelProvider::onMaterialTransparencyChanged(const IXMaterial *pMaterial)
{
	ScopedSpinLock lock(m_slModels);

	for(auto i = m_mModels.begin(); i; i++)
	{
		if(*i.second)
		{
			(*i.second)->onMaterialTransparencyChanged(pMaterial);
		}
	}
}

void CDynamicModelProvider::onMaterialEmissivityChanged(const IXMaterial *pMaterial)
{
	ScopedSpinLock lock(m_slModels);

	for(auto i = m_mModels.begin(); i; i++)
	{
		if(*i.second)
		{
			(*i.second)->onMaterialEmissivityChanged(pMaterial);
		}
	}
}

UINT CDynamicModelProvider::getTransparentCount(CRenderableVisibility *pVisibility)
{
	return(pVisibility->getItemTransparentDynamicCount());
}
void CDynamicModelProvider::getTransparentObject(CRenderableVisibility *pVisibility, UINT uIndex, XTransparentObject *pObject)
{
	CRenderableVisibility::TransparentModel *pMdl = pVisibility->getItemTransparentDynamic(uIndex);
	pObject->hasPSP = pMdl->hasPSP;
	if(pMdl->hasPSP)
	{
		pObject->psp = pMdl->psp;
	}
	pObject->pMaterial = pMdl->pMaterial;
	//! @fixme use transparent local bound
	pObject->vMin = pMdl->pReferenceMdl->getLocalBoundMin() + pMdl->pReferenceMdl->getPosition();
	pObject->vMax = pMdl->pReferenceMdl->getLocalBoundMax() + pMdl->pReferenceMdl->getPosition();
}
void CDynamicModelProvider::renderTransparentObject(CRenderableVisibility *pVisibility, UINT uIndex, UINT uSplitPlanes)
{
	m_pMaterialSystem->bindVS(m_pVertexShaderHandler);
	CRenderableVisibility::TransparentModel *pMdl = pVisibility->getItemTransparentDynamic(uIndex);
	pMdl->pReferenceMdl->render(pMdl->uLod, MF_TRANSPARENT);
}

void CDynamicModelProvider::notifyModelChanged(CDynamicModel *pModel, XEventModelChanged::TYPE type)
{
	XEventModelChanged ev;
	ev.type = type;
	ev.pModel = pModel;
	m_pModelChangedEventChannel->broadcastEvent(&ev);
}

IXSceneObjectType* CDynamicModelProvider::getSceneObjectType()
{
	return(m_pObjectType);
}

IXSceneFeature* CDynamicModelProvider::getFeature(XMODEL_FEATURE bmFeature)
{
	switch(bmFeature)
	{
	case MF_OPAQUE:
		return(m_pFeatureOpaque);
	case MF_TRANSPARENT:
		return(m_pFeatureTransparent);
	case MF_SELFILLUM:
		return(m_pFeatureSelfillum);
	}
	return(NULL);
}

void CDynamicModelProvider::enqueueModelDelete(CDynamicModel* pModel)
{
	m_qModelDelete.push(pModel);
}
