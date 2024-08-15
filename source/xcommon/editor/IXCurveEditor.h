#ifndef __IXCURVEEDITOR_H
#define __IXCURVEEDITOR_H

#include <gdefines.h>
#include <xcommon/util/IXMinMaxCurve.h>

class IXCurveEditorCallback
{
public:
	virtual void XMETHODCALLTYPE onAccept() = 0;
	virtual void XMETHODCALLTYPE onCancel() = 0;
};

//##########################################################################

// {242CA6B7-6025-4731-BBFE-54555A51016A}
#define IXCURVEEDITOR_GUID DEFINE_XGUID(0x242ca6b7, 0x6025, 0x4731, 0xbb, 0xfe, 0x54, 0x55, 0x5a, 0x51, 0x1, 0x6a)

class IXCurveEditor: public IXUnknown
{
public:
	virtual void XMETHODCALLTYPE edit(IXMinMaxCurve *pCurve, IXCurveEditorCallback *pCallback) = 0;
	virtual void XMETHODCALLTYPE abort() = 0;
};


#endif
