#include "Renderable.h"
#include "RenderableVisibility.h"

CRenderable::CRenderable(ID idPlugin, CAnimatedModelProvider *pProviderAnimated, CDynamicModelProvider *pProviderDynamic):
	m_idPlugin(idPlugin),
	m_pAnimatedModelProvider(pProviderAnimated),
	m_pDynamicModelProvider(pProviderDynamic)
{
}

X_RENDER_STAGE XMETHODCALLTYPE CRenderable::getStages()
{
	return(XRS_GBUFFER | XRS_SHADOWS | XRS_EDITOR_2D | XRS_TRANSPARENT | XRS_GI);
}

UINT XMETHODCALLTYPE CRenderable::getPriorityForStage(X_RENDER_STAGE stage)
{
	return(20);
}

void XMETHODCALLTYPE CRenderable::renderStage(X_RENDER_STAGE stage, IXRenderableVisibility *pVisibility)
{
	if(!m_isEnabled)
	{
		return;
	}

	CRenderableVisibility *pVis = NULL;
	if(pVisibility)
	{
		assert(pVisibility->getPluginId() == m_idPlugin);

		pVis = (CRenderableVisibility*)pVisibility;
	}

	switch(stage)
	{
	case XRS_PREPARE:
		break;
	case XRS_GBUFFER:
		m_pAnimatedModelProvider->render(pVis);
		m_pDynamicModelProvider->render(false, pVis);
		break;
	case XRS_SHADOWS:
		m_pAnimatedModelProvider->render(pVis);
		m_pDynamicModelProvider->render(false, pVis);
		break;
	case XRS_GI:
		m_pDynamicModelProvider->renderEmissive(pVis);
		break;
	case XRS_POSTPROCESS_MAIN:
		break;
	case XRS_TRANSPARENT:
		//m_pDynamicModelProvider->render(true, pVis);
		break;
	case XRS_POSTPROCESS_FINAL:
		break;
	case XRS_EDITOR_2D:
		m_pAnimatedModelProvider->render(pVis);
		//m_pDynamicModelProvider->render(false, pVis);
		m_pDynamicModelProvider->render(true, pVis);
		break;
	}
}

UINT XMETHODCALLTYPE CRenderable::getTransparentCount(IXRenderableVisibility *pVisibility)
{
	assert(pVisibility && pVisibility->getPluginId() == m_idPlugin);

	return(m_pDynamicModelProvider->getTransparentCount((CRenderableVisibility*)pVisibility));
}
void XMETHODCALLTYPE CRenderable::getTransparentObject(IXRenderableVisibility *pVisibility, UINT uIndex, XTransparentObject *pObject)
{
	assert(pVisibility && pVisibility->getPluginId() == m_idPlugin);

	m_pDynamicModelProvider->getTransparentObject((CRenderableVisibility*)pVisibility, uIndex, pObject);
}
void XMETHODCALLTYPE CRenderable::renderTransparentObject(IXRenderableVisibility *pVisibility, UINT uIndex, UINT uSplitPlanes)
{
	assert(pVisibility && pVisibility->getPluginId() == m_idPlugin);

	m_pDynamicModelProvider->renderTransparentObject((CRenderableVisibility*)pVisibility, uIndex, uSplitPlanes);
}

void XMETHODCALLTYPE CRenderable::startup(IXRender *pRender, IXMaterialSystem *pMaterialSystem)
{
	m_pDevice = pRender->getDevice();
	m_pMaterialSystem = pMaterialSystem;

	m_pAnimatedModelProvider->setDevice(m_pDevice);
	m_pDynamicModelProvider->setDevice(m_pDevice);
}
void XMETHODCALLTYPE CRenderable::shutdown()
{
}

void XMETHODCALLTYPE CRenderable::newVisData(IXRenderableVisibility **ppVisibility)
{
	*ppVisibility = new CRenderableVisibility(m_idPlugin, m_pAnimatedModelProvider, m_pDynamicModelProvider);
}

IXMaterialSystem* CRenderable::getMaterialSystem()
{
	return(m_pMaterialSystem);
}
