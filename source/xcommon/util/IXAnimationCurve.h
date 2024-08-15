#ifndef __IXANIMATION_CURVE_H
#define __IXANIMATION_CURVE_H

#include <gdefines.h>

enum XCURVE_WRAP_MODE
{
	XCWM_DEFAULT,
	XCWM_CLAMP_FOREVER,
	XCWM_ONCE,
	XCWM_LOOP,
	XCWM_PING_PONG
};

enum XKEYFRAME_WEIGHTED_MODE
{
	XKWM_NONE = 0,
	XKWM_IN = 1,
	XKWM_OUT = 2,
	XKWM_BOTH = XKWM_IN | XKWM_OUT
};

XDEFINE_ENUM_FLAG_OPERATORS(XKEYFRAME_WEIGHTED_MODE);

struct XKeyframe
{
	float fTime = 0.0f;
	float fValue = 0.0f;
	float fInTangent = 0.0f;
	float fOutTangent = 0.0f;

	float fInWeight = 0.0f;
	float fOutWeight = 0.0f;

	XKEYFRAME_WEIGHTED_MODE weightedMode = XKWM_NONE;
};

class IXAnimationCurve
{
public:
	virtual float XMETHODCALLTYPE evaluate(float fTime) const = 0;

	virtual UINT XMETHODCALLTYPE addKey(float fTime, float fValue) = 0;

	virtual int XMETHODCALLTYPE moveKey(UINT uIdx, const XKeyframe &key) = 0;

	virtual void XMETHODCALLTYPE setKey(UINT uIndex, const XKeyframe &key) = 0;

	virtual void XMETHODCALLTYPE setKeyCount(UINT uKeys) = 0;

	virtual const XKeyframe* XMETHODCALLTYPE getKeyAt(UINT uIdx) const = 0;

	virtual UINT XMETHODCALLTYPE getKeyframeCount() const = 0;

	virtual void XMETHODCALLTYPE smoothTangents(UINT uIdx, float fWeight) = 0;

	virtual void XMETHODCALLTYPE removeKey(UINT uIdx) = 0;

	virtual XCURVE_WRAP_MODE XMETHODCALLTYPE getPreWrapMode() const = 0;
	virtual void XMETHODCALLTYPE setPreWrapMode(XCURVE_WRAP_MODE mode) = 0;
	virtual XCURVE_WRAP_MODE XMETHODCALLTYPE getPostWrapMode() const = 0;
	virtual void XMETHODCALLTYPE setPostWrapMode(XCURVE_WRAP_MODE mode) = 0;

	virtual void XMETHODCALLTYPE setFrom(const IXAnimationCurve *pOther) = 0;
};

#endif
