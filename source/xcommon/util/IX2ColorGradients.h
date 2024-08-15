#ifndef __IX2_COLOR_GRADIENTS_H
#define __IX2_COLOR_GRADIENTS_H

#include "IXColorGradient.h"

enum X2COLOR_GRADIENT_MODE
{
	X2CGM_COLOR,
	X2CGM_GRADIENT,
	X2CGM_TWO_COLORS,
	X2CGM_TWO_GRADIENTS
};

class IX2ColorGradients
{
public:
	XMETHOD_GETSET(Mode, X2COLOR_GRADIENT_MODE, mode);

	XMETHOD_GETSET_REF(Color, float4_t, vValue);

	XMETHOD_GETSET_REF(Color0, float4_t, vValue);
	XMETHOD_GETSET_REF(Color1, float4_t, vValue);

	XMETHOD_2CONST(IXColorGradient*, getGradient0);
	XMETHOD_2CONST(IXColorGradient*, getGradient1);

	//! Evaluate the gradient at time.
	virtual float4_t XMETHODCALLTYPE evaluate(float fTime, float fLerpFacton = 1.0f) const = 0;

	virtual void XMETHODCALLTYPE setFrom(const IX2ColorGradients *pOther) = 0;
};

#endif
