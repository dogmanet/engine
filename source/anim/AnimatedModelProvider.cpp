#include "AnimatedModelProvider.h"
#include <xcommon/IPluginManager.h>

CAnimatedModelProvider::CAnimatedModelProvider(IXCore *pCore):
	m_pCore(pCore)
{
}

CAnimatedModelProvider::~CAnimatedModelProvider()
{
	mem_release(m_pVertexDeclaration);
}

IGXVertexDeclaration *CAnimatedModelProvider::getVertexDeclaration()
{
	return(m_pVertexDeclaration);
}

IGXDevice *CAnimatedModelProvider::getDevice()
{
	return(m_pRenderContext);
}

IXCore* CAnimatedModelProvider::getCore()
{
	return(m_pCore);
}

void CAnimatedModelProvider::setDevice(IGXDevice *pDevice)
{
	m_pRenderContext = pDevice;

#if 0
	// 72 bytes
	GXVertexElement layoutDynamicEx[] =
	{
		{0, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_POSITION},
		{0, 12, GXDECLTYPE_FLOAT2, GXDECLUSAGE_TEXCOORD},
		{0, 20, GXDECLTYPE_FLOAT3, GXDECLUSAGE_NORMAL},
		{0, 32, GXDECLTYPE_FLOAT3, GXDECLUSAGE_TANGENT},
		{0, 44, GXDECLTYPE_FLOAT3, GXDECLUSAGE_BINORMAL},
		{0, 56, GXDECLTYPE_UBYTE4, GXDECLUSAGE_BLENDINDICES},
		{0, 60, GXDECLTYPE_FLOAT4, GXDECLUSAGE_BLENDWEIGHT},
		GX_DECL_END()
	};
#endif

	// 52 bytes
	GXVertexElement layoutDynamicEx[] =
	{
		{0, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_POSITION},
		{0, 12, GXDECLTYPE_FLOAT2, GXDECLUSAGE_TEXCOORD},
		{0, 20, GXDECLTYPE_SHORT4N, GXDECLUSAGE_NORMAL},
		{0, 28, GXDECLTYPE_SHORT4N, GXDECLUSAGE_TANGENT},
		{0, 36, GXDECLTYPE_SHORT4N, GXDECLUSAGE_BINORMAL},
		{0, 44, GXDECLTYPE_UBYTE4, GXDECLUSAGE_BLENDINDICES},
		{0, 48, GXDECLTYPE_UBYTE4N, GXDECLUSAGE_BLENDWEIGHT},
		GX_DECL_END()
	};

	m_pVertexDeclaration = getDevice()->createVertexDeclaration(layoutDynamicEx);
	
	m_pMaterialSystem = (IXMaterialSystem*)m_pCore->getPluginManager()->getInterface(IXMATERIALSYSTEM_GUID);
	XVertexFormatHandler *pFormat = m_pMaterialSystem->getVertexFormat("xSceneGeneric");
	m_pVertexShaderHandler = m_pMaterialSystem->registerVertexShader(pFormat, "base/anim.vs");

}

bool XMETHODCALLTYPE CAnimatedModelProvider::createModel(UINT uResourceCount, IXResourceModelAnimated **ppResources, IXAnimatedModel **ppModel)
{
	IXResourceModelAnimated *pMinPtr = ppResources[0];
	for(UINT i = 1; i < uResourceCount; ++i)
	{
		if(ppResources[i] < pMinPtr)
		{
			pMinPtr = ppResources[i];
		}
	}
	Array<CAnimatedModelShared*> *paModels = NULL;
	{
		ScopedSpinLock lock(m_slModels);
		paModels = &m_mModels[pMinPtr];
	}
	auto &aModels = *paModels;

	CAnimatedModelShared *pShared = NULL;
	for(UINT i = 0, l = aModels.size(); i < l; ++i)
	{
		if(aModels[i]->isSame(uResourceCount, ppResources))
		{
			pShared = aModels[i];
			break;
		}
	}

	if(!pShared)
	{
		pShared = new CAnimatedModelShared(this);
		aModels.push_back(pShared);
		if(!pShared->init(uResourceCount, ppResources))
		{
			mem_delete(pShared);
			return(false);
		}
	}
	else
	{
		pShared->AddRef();
	}

	CAnimatedModel *pModel = new CAnimatedModel(this, pShared);
	m_apModels.push_back(pModel);

	pShared->Release();

	*ppModel = pModel;
	return(true);
}

void CAnimatedModelProvider::onSharedModelRelease(CAnimatedModelShared *pShared)
{
	ScopedSpinLock lock(m_slModels);

	auto &aModels = m_mModels[pShared->getResources()[0]];

	for(UINT i = 0, l = aModels.size(); i < l; ++i)
	{
		if(aModels[i] == pShared)
		{
			aModels.erase(i);
			return;
		}
	}

	assert(!"Something wrong! Should never get here!");
}

void CAnimatedModelProvider::onModelRelease(CAnimatedModel *pModel)
{
	for(UINT i = 0, l = m_apModels.size(); i < l; ++i)
	{
		if(m_apModels[i] == pModel)
		{
			m_apModels.erase(i);
			return;
		}
	}

	assert(!"Something wrong! Should never get here!");
}

IXMaterialSystem *CAnimatedModelProvider::getMaterialSystem()
{
	return((IXMaterialSystem*)m_pCore->getPluginManager()->getInterface(IXMATERIALSYSTEM_GUID));
}

void CAnimatedModelProvider::update(float fDT)
{
	typedef std::chrono::high_resolution_clock::time_point time_point;
	time_point tStart = std::chrono::high_resolution_clock::now();

	CAnimatedModelShared *pShared;
	while(m_queueGPUinitShared.tryPop(pShared))
	{
		pShared->initGPUresources();
		mem_release(pShared);

		if(((float)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - tStart).count() / 1000000.0f) > 0.0001f)
		{
			break;
		}
	}

	CAnimatedModel *pModel;
	while(m_queueGPUinitModel.tryPop(pModel))
	{
		pModel->initGPUresources();
		mem_release(pModel);

		if(((float)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - tStart).count() / 1000000.0f) > 0.0001f)
		{
			break;
		}
	}

	for(UINT i = 0, l = m_apModels.size(); i < l; ++i)
	{
		m_apModels[i]->update(fDT);
	}
}

void CAnimatedModelProvider::sync()
{
	for(UINT i = 0, l = m_apModels.size(); i < l; ++i)
	{
		m_apModels[i]->sync();
	}
}

void CAnimatedModelProvider::bindVertexFormat()
{
	m_pMaterialSystem->bindVS(m_pVertexShaderHandler);
}

void CAnimatedModelProvider::render(CRenderableVisibility *pVisibility)
{
	XPROFILE_FUNCTION();

	m_pMaterialSystem->bindVS(m_pVertexShaderHandler);
	for(UINT i = 0, l = m_apModels.size(); i < l; ++i)
	{
		if(pVisibility)
		{
			auto pItem = pVisibility->getItem(i);
			if(pItem->isVisible)
			{
				m_apModels[i]->render(pItem->uLod, MF_OPAQUE);
			}
		}
		else
		{
			m_apModels[i]->render(0, MF_OPAQUE);
		}
	}
	m_pMaterialSystem->bindVS(NULL);
}

void CAnimatedModelProvider::computeVisibility(const IXFrustum *pFrustum, CRenderableVisibility *pVisibility, UINT bmLayerMask, CRenderableVisibility *pReference)
{
	XPROFILE_FUNCTION();
	pVisibility->setItemCount(m_apModels.size());

	IXOcclusionCuller *pCuller = pVisibility->getCuller();

	CAnimatedModel *pMdl;
	for(UINT i = 0, l = m_apModels.size(); i < l; ++i)
	{
		pMdl = m_apModels[i];
		if(pMdl->isEnabled() && ((1 << pMdl->getLayer()) & bmLayerMask))
		{
			float3 vDelta = pMdl->getPosition();
			pVisibility->getItem(i)->isVisible = (pReference ? pReference->getItem(i)->isVisible : true)
				&& (pCuller ? pCuller->isAABBvisible(pMdl->getLocalBound() + vDelta) : pFrustum->boxInFrustum(pMdl->getLocalBound() + vDelta));
		}
		else
		{
			pVisibility->getItem(i)->isVisible = false;
		}
	}
}

void CAnimatedModelProvider::getLevelSize(const XEventLevelSize *pData)
{
	CAnimatedModel *pMdl;
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

void CAnimatedModelProvider::scheduleSharedGPUinit(CAnimatedModelShared *pShared)
{
	pShared->AddRef();
	m_queueGPUinitShared.push(pShared);
}

void CAnimatedModelProvider::scheduleModelGPUinit(CAnimatedModel *pModel)
{
	pModel->AddRef();
	m_queueGPUinitModel.push(pModel);
}
