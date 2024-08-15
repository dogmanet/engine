#include "Renderable.h"

CRenderable::CRenderable(ID idPlugin, CAmbient *pAmbient):
	m_idPlugin(idPlugin),
	m_pAmbient(pAmbient)
{
}

X_RENDER_STAGE XMETHODCALLTYPE CRenderable::getStages()
{
	return(XRS_GBUFFER | XRS_GI);
}

UINT XMETHODCALLTYPE CRenderable::getPriorityForStage(X_RENDER_STAGE stage)
{
	return(50);
}

void XMETHODCALLTYPE CRenderable::renderStage(X_RENDER_STAGE stage, IXRenderableVisibility *pVisibility)
{
	UNREFERENCED_PARAMETER(pVisibility);

	switch(stage)
	{
	case XRS_PREPARE:
		break;
	case XRS_GBUFFER:
		m_pAmbient->render();
		break;
	case XRS_SHADOWS:
		break;
	case XRS_GI:
		m_pAmbient->render();
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

void XMETHODCALLTYPE CRenderable::startup(IXRender *pRender, IXMaterialSystem *pMaterialSystem)
{
	m_pDevice = pRender->getDevice();
	m_pMaterialSystem = pMaterialSystem;

	m_pAmbient->setDevice(m_pDevice);
	m_pAmbient->setMaterialSystem(pMaterialSystem);
}
void XMETHODCALLTYPE CRenderable::shutdown()
{
	m_pAmbient->setDevice(NULL);
}

void XMETHODCALLTYPE CRenderable::newVisData(IXRenderableVisibility **ppVisibility)
{
	*ppVisibility = NULL;
}

IXMaterialSystem* CRenderable::getMaterialSystem()
{
	return(m_pMaterialSystem);
}
