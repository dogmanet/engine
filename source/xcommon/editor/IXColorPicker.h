#ifndef __IXCOLOR_PICKER_H
#define __IXCOLOR_PICKER_H

#include <gdefines.h>

class IXColorPickerCallback
{
public:
	virtual void XMETHODCALLTYPE onAccept(const float4_t &vColor) = 0;
	virtual void XMETHODCALLTYPE onPreview(const float4_t &vColor) = 0;
	virtual void XMETHODCALLTYPE onCancel(const float4_t &vStartColor) = 0;
};

//##########################################################################

// {96004BC3-385E-45F0-AD14-7169ACC6F6BA}
#define IXCOLORPICKER_GUID DEFINE_XGUID(0x96004bc3, 0x385e, 0x45f0, 0xad, 0x14, 0x71, 0x69, 0xac, 0xc6, 0xf6, 0xba)

class IXColorPicker: public IXUnknown
{
public:
	virtual void XMETHODCALLTYPE pick(const float4_t &vStartColor, IXColorPickerCallback *pCallback) = 0;
	virtual void XMETHODCALLTYPE abort() = 0;
};


#endif
