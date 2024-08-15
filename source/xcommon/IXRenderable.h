#ifndef __IXRENDERABLE_H
#define __IXRENDERABLE_H

#include <gdefines.h>
#include <graphix/graphix.h>
#include <mtrl/IXMaterialSystem.h>
#include "render/IXRender.h"
#include "render/IXFrustum.h"
#include "render/IXOcclusionCuller.h"

// {FDD30038-7D32-4EC6-911F-635376B1450D}
#define IXRENDERABLE_GUID DEFINE_XGUID(0xfdd30038, 0x7d32, 0x4ec6, 0x91, 0x1f, 0x63, 0x53, 0x76, 0xb1, 0x45, 0xd)
#define IXRENDERABLE_VERSION 2

class IXCamera;
class IXFrustum;

//! Данные о видимости для конкретной системы
class IXRenderableVisibility: public IXUnknown
{
public:
	virtual ID getPluginId() const = 0;

	//! Установка отсекателя по перекрытию
	virtual void setOcclusionCuller(IXOcclusionCuller *pOcclusionCuller) = 0;

	//! Выполнение проверки видимости для заданной камеры. Если pReference задан - проверка ограничивается только уже рассчитанным множеством
	virtual void updateForCamera(IXCamera *pCamera, const IXRenderableVisibility *pReference = NULL) = 0;

	//! Выполнение проверки видимости для заданного фрустума. Если pReference задан - проверка ограничивается только уже рассчитанным множеством
	virtual void updateForFrustum(const IXFrustum *pFrustum, UINT bmLayers, const IXRenderableVisibility *pReference = NULL) = 0;

	/*! Выполнение проверки видимости для заданной камеры в режиме оптимизации для многопоточности. 
	    Возвращает ID задачи из менеджера задач для отслеживания завершения внутренних задач
	*/
//	virtual ID updateForCameraThreaded(IXCamera *pCamera) = 0;

	//! Добввление к текущему набору видимости другого набора
	virtual void append(const IXRenderableVisibility *pOther) = 0;

	//! Вычитание из текущего набора видимости другого набора
	virtual void substract(const IXRenderableVisibility *pOther) = 0;
};

class IXRenderable: public IXUnknown
{
public:
	//! Возвращает поддерживаемые стадии рендера
	virtual X_RENDER_STAGE XMETHODCALLTYPE getStages() = 0;

	//! Возвращает приоритет рендера внутри стадии. Чем меньше число - тем раньше будет рендер
	virtual UINT XMETHODCALLTYPE getPriorityForStage(_in X_RENDER_STAGE stage) = 0;

	//! Выполняет отрисовку согласно заданной стадии с учетом видимости. Если pVisibility == NULL - рисуется все
	virtual void XMETHODCALLTYPE renderStage(_in X_RENDER_STAGE stage, _in _opt IXRenderableVisibility *pVisibility) = 0;

	//! Отрисовка прозрачных объектов
	//! @{
	virtual UINT XMETHODCALLTYPE getTransparentCount(_in IXRenderableVisibility *pVisibility) = 0;
	virtual void XMETHODCALLTYPE getTransparentObject(_in IXRenderableVisibility *pVisibility, _in UINT uIndex, _out XTransparentObject *pObject) = 0;
	virtual void XMETHODCALLTYPE renderTransparentObject(_in IXRenderableVisibility *pVisibility, _in UINT uIndex, _in UINT uSplitPlanes) = 0;
	//! @}

	virtual void XMETHODCALLTYPE startup(_in IXRender *pRender, _in IXMaterialSystem *pMaterialSystem) = 0;
	virtual void XMETHODCALLTYPE shutdown() = 0;

	//! Создает новый объект просчета видимости для системы
	virtual void XMETHODCALLTYPE newVisData(_out IXRenderableVisibility **ppVisibility) = 0;

	//! Оптимизирован ли алгоритм расчета видимости для параллельного выполнения?
	virtual bool XMETHODCALLTYPE isVisThreadOptimized() = 0;
};

#endif
