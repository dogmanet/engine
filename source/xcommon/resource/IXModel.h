#ifndef __IXMODEL_H
#define __IXMODEL_H

#include <gdefines.h>
#include "IXResourceModel.h"

enum XMODEL_FEATURE
{
	MF_NONE = 0x0000,
	MF_OPAQUE = 0x0001,
	MF_TRANSPARENT = 0x0002,
	MF_SELFILLUM = 0x0004
};
DEFINE_ENUM_FLAG_OPERATORS(XMODEL_FEATURE);

enum XMODEL_BONE_TRANSFORM
{
	XMBT_WORLD,
	XMBT_RENDER,
	// XMBT_BIND
};

class IXStaticModel;
class IXDynamicModel;
class IXAnimatedModel;
// No direct implementation
class IXModel: public IXUnknown
{
public:
	virtual IXAnimatedModel* XMETHODCALLTYPE asAnimatedModel() = 0;
	virtual IXDynamicModel* XMETHODCALLTYPE asDynamicModel() = 0;
	virtual IXStaticModel* XMETHODCALLTYPE asStaticModel() = 0;


	virtual float3 XMETHODCALLTYPE getPosition() const = 0;
	virtual void XMETHODCALLTYPE setPosition(const float3 &vPos) = 0;

	virtual float XMETHODCALLTYPE getScale() const = 0;
	virtual void XMETHODCALLTYPE setScale(float fScale)  = 0;

	virtual SMQuaternion XMETHODCALLTYPE getOrientation() const = 0;
	virtual void XMETHODCALLTYPE setOrientation(const SMQuaternion &qRot) = 0;

	virtual UINT XMETHODCALLTYPE getSkin() const = 0;
	virtual void XMETHODCALLTYPE setSkin(UINT uSkin) = 0;

	virtual float3 XMETHODCALLTYPE getLocalBoundMin() const = 0;
	virtual float3 XMETHODCALLTYPE getLocalBoundMax() const = 0;
	virtual SMAABB XMETHODCALLTYPE getLocalBound() const = 0;

	virtual float4 XMETHODCALLTYPE getColor() const = 0;
	virtual void XMETHODCALLTYPE setColor(const float4 &vColor) = 0;

	virtual UINT XMETHODCALLTYPE getPhysboxCount(UINT uPartIndex = 0) const = 0;
	virtual const IModelPhysbox* XMETHODCALLTYPE getPhysBox(UINT id, UINT uPartIndex = 0) const = 0;
	virtual const IXResourceModel* XMETHODCALLTYPE getResource(UINT uIndex = 0) = 0;

	virtual bool XMETHODCALLTYPE isEnabled() const = 0;
	virtual void XMETHODCALLTYPE enable(bool yesNo) = 0;

	virtual void XMETHODCALLTYPE render(UINT uLod, XMODEL_FEATURE bmFeatures) = 0;

	virtual bool XMETHODCALLTYPE rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut = NULL, float3 *pvNormal = NULL, bool isRayInWorldSpace = true, bool bReturnNearestPoint = false) = 0;

	virtual void XMETHODCALLTYPE setLayer(UINT uLayer) = 0;
	virtual UINT XMETHODCALLTYPE getLayer() = 0;
};

// Implemented in geom plugin
class IXStaticModel: public IXModel
{
public:
};

// Implemented in anim plugin
class IXDynamicModel: public IXStaticModel
{
public:
};

class IAnimationCallback
{
public:
	virtual void onPlay(UINT uLayer) = 0;
	virtual void onStop(UINT uLayer) = 0;
	virtual void onLoop(UINT uLayer) = 0;

	virtual void onProgress(UINT uLayer, float fProgress) = 0;
};

// Implemented in anim plugin
class IXAnimatedModel: public IXDynamicModel
{
public:
	virtual UINT XMETHODCALLTYPE getPartsCount() const = 0;
	virtual const char* XMETHODCALLTYPE getPartName(UINT uIndex) const = 0;
	virtual UINT XMETHODCALLTYPE getPartIndex(const char *szName) = 0;
	virtual XMODEL_PART_FLAGS XMETHODCALLTYPE getPartFlags(UINT uIndex) const = 0;
	virtual bool XMETHODCALLTYPE isPartEnabled(UINT uIndex) const = 0;
	virtual void XMETHODCALLTYPE enablePart(UINT uIndex, bool yesNo) = 0;

	virtual UINT XMETHODCALLTYPE getHitboxCount(UINT uPartIndex = 0) const = 0;
	virtual const IModelPhysbox* XMETHODCALLTYPE getHitbox(UINT id, UINT uPartIndex = 0) const = 0;
	virtual int XMETHODCALLTYPE getHitboxBone(UINT id, UINT uPartIndex = 0) const = 0;
	virtual XHITBOXBODYPART XMETHODCALLTYPE getHitboxBodyPart(UINT id, UINT uPartIndex = 0) const = 0;

	/*! Запускает воспроизведения анимации
		@param[in] szName Имя анимации
		@param[in] uFadeTime Время перехода от предыдущей анимации к новой в ms
		@param[in] uSlot Слот для воспроизведения. От 0 до getLayersCount()
		@param[in] bReplaceActivity Остановить ли активность в заданном слоте?
	*/
	virtual void XMETHODCALLTYPE play(const char *szName, UINT uFadeTime = 0, UINT uSlot = 0, bool bReplaceActivity = true) = 0;

	/*! Останавливает воспроизведения для указанного слота
		@param[in] uSlot Слот для остановки. От 0 до getLayersCount()
	*/
	virtual void XMETHODCALLTYPE stop(UINT uSlot = 0) = 0;

	/*! Возобновляет воспроизведения для указанного слота
		@param[in] uSlot Слот для возобновления. От 0 до getLayersCount()
	*/
	virtual void XMETHODCALLTYPE resume(UINT uSlot = 0) = 0;

	/*! Устанавливает прогресс воспроизведения в конкретном слоте
		@warning Если в конкретном слоте не происходит воспроизведения, не будет эффекта
		@param[in] fProgress Значение прогресса. От 0 до 1
		@param[in] uSlot Слот. От 0 до getLayersCount()
	*/
	virtual void XMETHODCALLTYPE setProgress(float fProgress, UINT uSlot = 0) = 0;

	/*! Запускает воспроизведение указанной активности в заданном слоте
		@param[in] szName Имя анимации
		@param[in] uFadeTime Время перехода от предыдущей анимации к новой в ms
		@param[in] uSlot Слот для воспроизведения. От 0 до getLayersCount()
	*/
	virtual void XMETHODCALLTYPE startActivity(const char *szName, UINT uFadeTime = 0, UINT uSlot = 0) = 0;

	/*! Возвращает смещение указанной кости
		@param[in] id Номер кости
		@return Смещение кости
	*/
	virtual float3 XMETHODCALLTYPE getBoneTransformPos(UINT id, XMODEL_BONE_TRANSFORM boneTranform = XMBT_WORLD) = 0;

	/*! Возвращает вращение указанной кости
		@param[in] id Номер кости
		@return Вращение кости
	*/
	virtual SMQuaternion XMETHODCALLTYPE getBoneTransformRot(UINT id, XMODEL_BONE_TRANSFORM boneTranform = XMBT_WORLD) = 0;

	/*! Возвращает трансформацию указанной кости
		@param[in] id Номер кости
		@return Матрица трансформации кости
	*/
	virtual SMMATRIX XMETHODCALLTYPE getBoneTransform(UINT id) = 0;

	/*! Возвращает идентификатор указанной кости
		@param[in] szName Имя кости
		@return Номер кости
	*/
	virtual UINT XMETHODCALLTYPE getBoneId(const char *szName) = 0;

	/*! Возвращает количество костей
		@return Количество костей
	*/
	virtual UINT XMETHODCALLTYPE getBoneCount() const = 0;

	/*! Возвращает имя указанной кости
		@param[in] id Номер кости
	*/
	virtual const char* XMETHODCALLTYPE getBoneName(UINT id) const = 0;

	/*! Проверяет, воспроизводится ли анимация
	*/
	virtual bool XMETHODCALLTYPE isPlayingAnimations() = 0;

	/*! Проверяет, воспроизводится ли анимация
		@param[in] szName Имя анимации
	*/
	virtual bool XMETHODCALLTYPE isPlayingAnimation(const char *szName) = 0;

	/*! Останавливает воспроизведение всех анимаций
	*/
	virtual void XMETHODCALLTYPE stopAll() = 0;

	/*! Устанавливает значение для заданного контроллера
		@param[in] id Идентификатор контроллера
		@param[in] fValue Значение от 0 до 1
	*/
	virtual void XMETHODCALLTYPE setController(UINT id, float fValue) = 0;

	virtual UINT XMETHODCALLTYPE getControllersCount() const = 0;
	virtual const char* XMETHODCALLTYPE getControllerName(UINT id) = 0;
	virtual UINT XMETHODCALLTYPE getControllerId(const char *szName) = 0;

	//! Коллбек на изменение состояния анимации
	virtual void XMETHODCALLTYPE setCallback(IAnimationCallback *pCallback) = 0;

	//! получение идентификатора родительской кости
	virtual int XMETHODCALLTYPE getBoneParent(UINT id) = 0;

	/*! Получает количество слоев анимации
	*/
	virtual UINT XMETHODCALLTYPE getLayersCount() const = 0;

	/*! Устанавливает количество слоев анимации
	*/
	virtual void XMETHODCALLTYPE setLayersCount(UINT uCount) = 0;

	/*! Получает вес слоя при смешении анимации.
	*/
	virtual float XMETHODCALLTYPE getLayerBlendWeight(UINT uLayer) const = 0;

	/*! Устанавливает вес слоя при смешении анимации. Суммарный вес активных слоев должен равняться 1.0f
	*/
	virtual void XMETHODCALLTYPE setLayerBlendWeight(UINT uLayer, float fWeight) = 0;
};

#endif
