#ifndef __IXOCCLUSIONCULLER_H
#define __IXOCCLUSIONCULLER_H

#include <gdefines.h>
#include <common/math.h>

class IXCamera;
class IXOcclusionCuller: public IXUnknown
{
public:
	//! обновление
	virtual void XMETHODCALLTYPE update(IXCamera *pCamera, float fFOV, float fAspectRatio, float fFarPlane) = 0;

	//! видна ли точка
	virtual bool XMETHODCALLTYPE isPointVisible(const float3 &vPoint) const = 0;

	//! находится ли параллелепипед во фрустуме
	virtual bool XMETHODCALLTYPE isAABBvisible(const SMAABB &aabb, bool *pisFullyVisible = NULL) const = 0;

	//! виден ли треугольник
	//virtual bool XMETHODCALLTYPE isPolyVisible(const float3 &vPoint1, const float3 &vPoint2, const float3 &vPoint3) const = 0;

	//! видна ли сфера
	//virtual bool XMETHODCALLTYPE isSphereVisible(const float3 &vOrigin, float fRadius) const = 0;
};

#endif
