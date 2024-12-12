#ifndef __IUIMULTI_TRACKBAR_H
#define __IUIMULTI_TRACKBAR_H

#include "IUIControl.h"

class IUIMultiTrackbar: public IUIControl
{
public:
	virtual void XMETHODCALLTYPE setMarkerCount(UINT uCount) = 0;
	virtual UINT XMETHODCALLTYPE getMarkerCount() = 0; 
	virtual void XMETHODCALLTYPE setMarkerPos(UINT uIndex, float fPos) = 0;
	virtual float XMETHODCALLTYPE getMarkerPos(UINT uIndex) = 0;
	virtual void XMETHODCALLTYPE removeMarker(UINT uIndex) = 0;
	virtual void XMETHODCALLTYPE setMarkerColor(UINT uIndex, const float4_t &vColor) = 0;
	virtual float4_t XMETHODCALLTYPE getMarkerColor(UINT uIndex) = 0;

	virtual void XMETHODCALLTYPE setSelection(int iMarker) = 0;
	virtual int XMETHODCALLTYPE getSelection() = 0;

	virtual void XMETHODCALLTYPE setInvertedMode(bool yesNo) = 0;
};

#endif
