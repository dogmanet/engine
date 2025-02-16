#ifndef __IXDECALPROVIDER_H
#define __IXDECALPROVIDER_H

#include "IXDecal.h"

// {878E7A1E-86D4-4967-9A6D-6054B4363119}
#define IXDECALPROVIDER_GUID DEFINE_XGUID(0x878e7a1e, 0x86d4, 0x4967, 0x9a, 0x6d, 0x60, 0x54, 0xb4, 0x36, 0x31, 0x19)

enum XDECAL_TYPE
{
	XDT_CONCRETE = 0,
	XDT_METAL,
	XDT_GLASS,
	XDT_PLASTIC,
	XDT_WOOD,
	XDT_FLESH,
	XDT_EARTH,
	XDT_BLOOD_BIG,

	XDT__COUNT
};

// Implemented in anim plugin
class IXDecalProvider: public IXUnknown
{
public:
	// create custom decal
	//virtual bool XMETHODCALLTYPE newDecal(IXDecal **ppDecal) = 0;
	
	// create temporal standard decal at point/normal
	virtual void XMETHODCALLTYPE shootDecal(XDECAL_TYPE type, const float3 &vWorldPos, const float3 &vNormal, float fScale = 1.0f, const float3 *pvSAxis = NULL) = 0;
};

#endif
