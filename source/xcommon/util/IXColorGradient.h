#ifndef __IXCOLOR_GRADIENT_H
#define __IXCOLOR_GRADIENT_H

#include <gdefines.h>

struct XColorKey
{
	float3_t vColor;
	float fTime;
};

struct XAlphaKey
{
	float fAlpha;
	float fTime;
};

class IXColorGradient
{
public:
	//! Evaluate the color at time.
	virtual float4_t XMETHODCALLTYPE evaluate(float fTime) const = 0;

	//! Add a new key to the gradient.
	virtual UINT XMETHODCALLTYPE addColorKey(float fTime, const float3_t &vValue) = 0;
	virtual UINT XMETHODCALLTYPE addAlphaKey(float fTime, float fValue) = 0;

	//!	Removes the keyframe at index and inserts key.
	virtual int XMETHODCALLTYPE moveColorKey(UINT uIdx, float fNewTime) = 0;
	virtual int XMETHODCALLTYPE moveAlphaKey(UINT uIdx, float fNewTime) = 0;

	//! Set frame in array by index
	virtual void XMETHODCALLTYPE setColorKey(UINT uIndex, const XColorKey &key) = 0;
	virtual void XMETHODCALLTYPE setAlphaKey(UINT uIndex, const XAlphaKey &key) = 0;

	virtual UINT XMETHODCALLTYPE getColorKeyCount() const = 0;
	virtual UINT XMETHODCALLTYPE getAlphaKeyCount() const = 0;

	virtual void XMETHODCALLTYPE setColorKeyCount(UINT uKeys) = 0;
	virtual void XMETHODCALLTYPE setAlphaKeyCount(UINT uKeys) = 0;

	virtual const XColorKey* XMETHODCALLTYPE getColorKeyAt(UINT uIdx) const = 0;
	virtual const XAlphaKey* XMETHODCALLTYPE getAlphaKeyAt(UINT uIdx) const = 0;

	//! Removes a key.
	virtual void XMETHODCALLTYPE removeColorKey(UINT uIdx) = 0;
	virtual void XMETHODCALLTYPE removeAlphaKey(UINT uIdx) = 0;

	virtual void XMETHODCALLTYPE setFrom(const IXColorGradient *pOther) = 0;

	virtual void XMETHODCALLTYPE sortKeys() = 0;
};

#endif
