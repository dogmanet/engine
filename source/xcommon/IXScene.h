#ifndef __IXSCENE_H
#define __IXSCENE_H

#include <gdefines.h>
#include "render/IXFrustum.h"

class IXOcclusionCuller;

//! Особенность объекта
class IXSceneFeature: public IXUnknown
{
};

//##########################################################################

//! Объект сцены
class IXSceneObject: public IXUnknown
{
public:
	/*!
		Обновляет AABB объекта
		@threadsafe full
	*/
	virtual void XMETHODCALLTYPE update(const SMAABB &aabb) = 0;

	/*!
		Устанавливает особенность объекта
		@threadsafe full
	*/
	virtual void XMETHODCALLTYPE setFeature(IXSceneFeature *pFeat, bool isSet) = 0;

	/*!
		Устанавливает массив особенностей объекта. Последний элемент должен быть NULL
		@threadsafe full
	*/
	virtual void XMETHODCALLTYPE setFeatures(IXSceneFeature **ppFeatures) = 0;

	/*!
		Устанавливает слой объекта.
		@threadsafe full
	*/
	virtual void XMETHODCALLTYPE setLayer(UINT uLayer) = 0;
};

//##########################################################################

//! Операции запроса
enum XSCENE_QUERY_OP
{
	SQO_AND, //<! Должны выполниться все условия
	SQO_OR   //<! Должно выполниться любое из условий
};

//! Требования к особенностям
enum XSCENE_QUERY_FEATURE
{
	SQF_ANY,  //!< Наличие/отсутствие особенности не важно
	SQF_SET,  //!< Особенность должна присутствовать
	SQF_UNSET //!< Особенность должна отсутствовать
};

//##########################################################################

//! Объект запроса к сцене
class IXSceneQuery: public IXUnknown
{
public:
	/*!
		Выполняет запрос, возвращает количество найденных объектов, 
		в pppObjects записывается указатель на массив пользовательских указателей найденных объектов
		@threadsafe sync
	*/
	virtual UINT XMETHODCALLTYPE execute(const IXFrustum *pFrustum, void ***pppObjects, IXOcclusionCuller *pOcclusionCuller = NULL) = 0;

	/*!
		Выполняет запрос, возвращает количество найденных объектов,
		в pppObjects записывается указатель на массив пользовательских указателей найденных объектов
		в vDir задается предпочтительное направление обхода (данное направление лишь подсказка, но не руководство к действию)
		@threadsafe sync
	*/
	virtual UINT XMETHODCALLTYPE execute(const IXFrustum *pFrustum, const float3 &vDir, void ***pppObjects, IXOcclusionCuller *pOcclusionCuller = NULL) = 0;

	/*!
		Устанавливает операцию выборки
		@threadsafe none
	*/
	virtual void XMETHODCALLTYPE setOP(XSCENE_QUERY_OP op) = 0;

	/*!
		Устанавливает требование к особенности
		@threadsafe none
	*/
	virtual void XMETHODCALLTYPE setFeature(IXSceneFeature *pFeat, XSCENE_QUERY_FEATURE mode) = 0;

	/*!
		Устанавливает отсечение по экранному размеру
		@threadsafe none
	*/
	virtual void XMETHODCALLTYPE setScreenSizeCulling(const float3 &vCamPos, float fFov, float fScreenHeightPx, float fThresholdPx = 4.0f) = 0;
	virtual void XMETHODCALLTYPE unsetScreenSizeCulling() = 0;

	/*!
		Устанавливает маску слоев
		@threadsafe none
	*/
	virtual void XMETHODCALLTYPE setLayerMask(UINT bmLayerMask) = 0;
};

//##########################################################################

//! Тип объекта сцены
class IXSceneObjectType: public IXUnknown
{
public:
	/*!
		Добавляет новый объект в систему
		@threadsafe full
	*/
	virtual IXSceneObject* XMETHODCALLTYPE newObject(const SMAABB &aabb, void *pUserData, IXSceneFeature **ppFeatures = NULL, UINT uLayer = 0) = 0;
	
	/*!
		Создает новый запрос
		@threadsafe full
	*/
	virtual IXSceneQuery* XMETHODCALLTYPE newQuery() = 0;
};

//##########################################################################

// {FF050B41-5158-4380-990B-7E61E9F72B15}
#define IXSCENE_GUID DEFINE_XGUID(0xff050b41, 0x5158, 0x4380, 0x99, 0xb, 0x7e, 0x61, 0xe9, 0xf7, 0x2b, 0x15)

class IXScene: public IXUnknown
{
public:
	/*!
		Регистрирует новый тип объекта, либо возвращает существующий
		@threadsafe none
	*/
	virtual IXSceneObjectType* XMETHODCALLTYPE registerObjectType(const char *szName) = 0;

	/*!
		Возвращает тип объекта по имени
		@threadsafe none
	*/
	virtual IXSceneObjectType* XMETHODCALLTYPE getObjectType(const char *szName) = 0;
	
	/*!
		Регистрирует новую особенность объекта, либо возвращает существующую
		@threadsafe none
	*/
	virtual IXSceneFeature* XMETHODCALLTYPE registerObjectFeature(const char *szName) = 0;

	/*!
		Возвращает особенность объекта по имени
		@threadsafe none
	*/
	virtual IXSceneFeature* XMETHODCALLTYPE getObjectFeature(const char *szName) = 0;

	/*!
		Возвращает высоту дерева
		@threadsafe none
	*/
	virtual UINT XMETHODCALLTYPE getTreeHeight() = 0;
};

#endif
