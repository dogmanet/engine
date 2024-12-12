#ifndef __IXMIN_MAX_CURVE_H
#define __IXMIN_MAX_CURVE_H

#include "IXAnimationCurve.h"

enum XMINMAX_CURVE_MODE
{
	XMCM_CONSTANT,
	XMCM_CURVE,
	XMCM_TWO_CONSTANTS,
	XMCM_TWO_CURVES
};

class IXMinMaxCurve
{
public:
	XMETHOD_GETSET(Mode, XMINMAX_CURVE_MODE, mode);
	XMETHOD_GETSET(Min, float, fValue);
	XMETHOD_GETSET(Max, float, fValue);

	XMETHOD_2CONST(IXAnimationCurve*, getMinCurve);
	XMETHOD_2CONST(IXAnimationCurve*, getMaxCurve);

	//! Evaluate the curve at time.
	virtual float XMETHODCALLTYPE evaluate(float fTime, float fLerpFacton = 1.0f) const = 0;

	virtual void XMETHODCALLTYPE setFrom(const IXMinMaxCurve *pOther) = 0;
};

#endif
