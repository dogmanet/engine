#ifndef __IXDECAL_H
#define __IXDECAL_H

#include <gdefines.h>
#include "IXResourceModel.h"

class IXDecal: public IXUnknown
{
public:
	virtual float3 XMETHODCALLTYPE getPosition() const = 0;
	virtual void XMETHODCALLTYPE setPosition(const float3 &vPos) = 0;

	virtual SMQuaternion XMETHODCALLTYPE getOrientation() const = 0;
	virtual void XMETHODCALLTYPE setOrientation(const SMQuaternion &qRot) = 0;

	virtual float3 XMETHODCALLTYPE getLocalBoundMin() const = 0;
	virtual float3 XMETHODCALLTYPE getLocalBoundMax() const = 0;
	virtual SMAABB XMETHODCALLTYPE getLocalBound() const = 0;

	virtual bool XMETHODCALLTYPE isEnabled() const = 0;
	virtual void XMETHODCALLTYPE enable(bool yesNo) = 0;

	virtual bool XMETHODCALLTYPE rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut = NULL, float3 *pvNormal = NULL, bool isRayInWorldSpace = true, bool bReturnNearestPoint = false) = 0;

	virtual void XMETHODCALLTYPE setLayer(UINT uLayer) = 0;
	virtual UINT XMETHODCALLTYPE getLayer() = 0;

	virtual void XMETHODCALLTYPE setHeight(float fHeight) = 0;
	virtual float XMETHODCALLTYPE getHeight() = 0;

	virtual void XMETHODCALLTYPE setTextureRangeU(const float2_t &vRange) = 0;
	virtual float2_t XMETHODCALLTYPE getTextureRangeU() = 0;

	virtual void XMETHODCALLTYPE setTextureRangeV(const float2_t &vRange) = 0;
	virtual float2_t XMETHODCALLTYPE getTextureRangeV() = 0;
};

#endif
