#ifndef __RENDERABLE_H
#define __RENDERABLE_H

#include <xcommon/IXRenderable.h>

#include "ParticleSystem.h"

class CRenderable: public IXUnknownImplementation<IXRenderable>
{
public:
	CRenderable(ID idPlugin, CParticleSystem *pParticleSystem);

	XIMPLEMENT_VERSION(IXRENDERABLE_VERSION);

	//! Возвращает поддерживаемые стадии рендера
	X_RENDER_STAGE XMETHODCALLTYPE getStages() override;

	//! Возвращает приоритет рендера внутри стадии. Чем меньше число - тем раньше будет рендер
	UINT XMETHODCALLTYPE getPriorityForStage(X_RENDER_STAGE stage) override;

	//! Выполняет отрисовку согласно заданной стадии с учетом видимости. Если pVisibility == NULL - рисуется все
	void XMETHODCALLTYPE renderStage(X_RENDER_STAGE stage, IXRenderableVisibility *pVisibility) override;

	UINT XMETHODCALLTYPE getTransparentCount(IXRenderableVisibility *pVisibility) override;
	void XMETHODCALLTYPE getTransparentObject(IXRenderableVisibility *pVisibility, UINT uIndex, XTransparentObject *pObject) override;
	void XMETHODCALLTYPE renderTransparentObject(IXRenderableVisibility *pVisibility, UINT uIndex, UINT uSplitPlanes) override;

	void XMETHODCALLTYPE startup(IXRender *pRender, IXMaterialSystem *pMaterialSystem) override;
	void XMETHODCALLTYPE shutdown() override;

	//! Создает новый объект просчета видимости для системы
	void XMETHODCALLTYPE newVisData(IXRenderableVisibility **ppVisibility) override;

	IXMaterialSystem *getMaterialSystem();

	bool XMETHODCALLTYPE isVisThreadOptimized() override
	{
		return(false);
	}

protected:
	IGXDevice *m_pDevice = NULL;
	IXMaterialSystem *m_pMaterialSystem = NULL;
	ID m_idPlugin = -1;


	CParticleSystem *m_pParticleSystem = NULL;
};

#endif
