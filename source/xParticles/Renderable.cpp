#include "Renderable.h"
#include "RenderableVisibility.h"

CRenderable::CRenderable(ID idPlugin, CParticleSystem *pParticleSystem):
	m_idPlugin(idPlugin),
	m_pParticleSystem(pParticleSystem)
{
}

X_RENDER_STAGE XMETHODCALLTYPE CRenderable::getStages()
{
	return(XRS_GBUFFER | XRS_SHADOWS | XRS_TRANSPARENT | XRS_GI);
}

UINT XMETHODCALLTYPE CRenderable::getPriorityForStage(X_RENDER_STAGE stage)
{
	return(60);
}

void XMETHODCALLTYPE CRenderable::renderStage(X_RENDER_STAGE stage, IXRenderableVisibility *pVisibility)
{
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
		m_pParticleSystem->render(pVis);
		break;
	case XRS_SHADOWS:
		m_pParticleSystem->render(pVis);
		break;
	case XRS_GI:
		m_pParticleSystem->renderEmissive(pVis);
		break;
	case XRS_POSTPROCESS_MAIN:
		break;
	case XRS_TRANSPARENT:
		break;
	case XRS_POSTPROCESS_FINAL:
		break;
	case XRS_EDITOR_2D:
		break;
	}
}

UINT XMETHODCALLTYPE CRenderable::getTransparentCount(IXRenderableVisibility *pVisibility)
{
	assert(pVisibility && pVisibility->getPluginId() == m_idPlugin);

	return(((CRenderableVisibility*)pVisibility)->getItemTransparentDynamicCount());
}
void XMETHODCALLTYPE CRenderable::getTransparentObject(IXRenderableVisibility *pVisibility, UINT uIndex, XTransparentObject *pObject)
{
	assert(pVisibility && pVisibility->getPluginId() == m_idPlugin);

	CRenderableVisibility::TransparentModel *pMdl = ((CRenderableVisibility*)pVisibility)->getItemTransparentDynamic(uIndex);


	pObject->hasPSP = false;
	pObject->pMaterial = pMdl->pMaterial;
	const SMAABB &aabb = pMdl->pReferenceMdl->getBounds();
	pObject->vMin = aabb.vMin;
	pObject->vMax = aabb.vMax;

	//m_pParticleSystem->getTransparentObject((CRenderableVisibility*)pVisibility, uIndex, pObject);
}
void XMETHODCALLTYPE CRenderable::renderTransparentObject(IXRenderableVisibility *pVisibility, UINT uIndex, UINT uSplitPlanes)
{
	assert(pVisibility && pVisibility->getPluginId() == m_idPlugin);

	m_pParticleSystem->renderTransparentObject((CRenderableVisibility*)pVisibility, uIndex, uSplitPlanes);
}

void XMETHODCALLTYPE CRenderable::startup(IXRender *pRender, IXMaterialSystem *pMaterialSystem)
{
	m_pDevice = pRender->getDevice();
	m_pMaterialSystem = pMaterialSystem;

	m_pParticleSystem->setDevice(m_pDevice);
	m_pParticleSystem->setMaterialSystem(pMaterialSystem);
}
void XMETHODCALLTYPE CRenderable::shutdown()
{
	m_pParticleSystem->setDevice(NULL);
}

void XMETHODCALLTYPE CRenderable::newVisData(IXRenderableVisibility **ppVisibility)
{
	*ppVisibility = new CRenderableVisibility(m_idPlugin, m_pParticleSystem);
}

IXMaterialSystem* CRenderable::getMaterialSystem()
{
	return(m_pMaterialSystem);
}
