#ifndef __IXCAMERA_H
#define __IXCAMERA_H

#include <gdefines.h>
#include "render/IXFrustum.h"

enum XCAMERA_POJECTION_MODE
{
	XCPM_PERSPECTIVE,
	XCPM_ORTHOGONAL,
};

class IXRenderableVisibility;
class IXCamera: public IXUnknown
{
public:
	//! установить полное вращение
	virtual void XMETHODCALLTYPE setOrientation(const SMQuaternion &qOrientation) = 0;

	virtual const SMQuaternion& XMETHODCALLTYPE getOrientation() = 0;

	//! получаем матрицу вида
	virtual const SMMATRIX& XMETHODCALLTYPE getViewMatrix() const = 0;

	/*! \name Базис
	@{
	*/

	//! в pos записывает текущую позицию в мире
	virtual const float3& XMETHODCALLTYPE getPosition() const = 0;

	//! устанавливает позицию в мире
	virtual void XMETHODCALLTYPE setPosition(const float3 &vPos) = 0;


	//! в right записывает парвый вектор
	virtual const float3& XMETHODCALLTYPE getRight() const = 0;

	//! в up записывает верхний вектор
	virtual const float3& XMETHODCALLTYPE getUp() const = 0;

	//! в look записывает вектор направление взгляда
	virtual const float3& XMETHODCALLTYPE getLook() const = 0;


	//! в rot записывает углы поворотов по осям, в радианах
	virtual const float3& XMETHODCALLTYPE getRotation() const = 0;

	//!@}

	//! устанавливает FOV камеры
	virtual void XMETHODCALLTYPE setFOV(float fFOV) = 0;
	virtual void XMETHODCALLTYPE setFOVmultiplier(float fFOVmultiplier) = 0;

	//! возвращает FOV камеры
	virtual float XMETHODCALLTYPE getFOV() const = 0;
	virtual float XMETHODCALLTYPE getFOVmultiplier() const = 0;

	virtual float XMETHODCALLTYPE getEffectiveFOV() const = 0;

	//! обновление фрустума камеры
	virtual void XMETHODCALLTYPE updateFrustum(const float4x4 &mProjection) = 0;

	//! возвращает указатель фрустума
	virtual IXFrustum* XMETHODCALLTYPE getFrustum() = 0;

	virtual void XMETHODCALLTYPE setNear(float fNear) = 0;
	virtual void XMETHODCALLTYPE setFar(float fFar) = 0;
	virtual float XMETHODCALLTYPE getNear() const = 0;
	virtual float XMETHODCALLTYPE getFar() const = 0;

	virtual void XMETHODCALLTYPE getVisibility(IXRenderableVisibility **ppVisibility) = 0;
	virtual void XMETHODCALLTYPE updateVisibility() = 0;

	virtual void XMETHODCALLTYPE setProjectionMode(XCAMERA_POJECTION_MODE mode) = 0;
	virtual XCAMERA_POJECTION_MODE XMETHODCALLTYPE getProjectionMode() const= 0;

	virtual void XMETHODCALLTYPE setScale(float fScale) = 0;
	virtual float XMETHODCALLTYPE getScale() const = 0;

	//! получаем матрицу проекции
	virtual const SMMATRIX& XMETHODCALLTYPE getProjMatrix() const = 0;
	virtual void XMETHODCALLTYPE updateProjection(UINT uTargetWidth, UINT uTargetHeight) = 0;

	virtual void XMETHODCALLTYPE setLayerMask(UINT bmLayerMask) = 0;
	virtual UINT XMETHODCALLTYPE getLayerMask() = 0;
};

#endif
