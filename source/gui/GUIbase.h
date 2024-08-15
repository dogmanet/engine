#ifndef _GUIbase_H_
#define _GUIbase_H_

//#include <Windows.h>
#include <graphix/graphix.h>

#define SGCore_ShaderSetVRF(...) assert(!"Deprecated!")

#if defined(_DEBUG)
#	pragma comment(lib, "sxcore_d.lib")
#else
#	pragma comment(lib, "sxcore.lib")
#endif

#if defined(_WINDOWS)
#pragma warning(disable:4244)
#endif

//#include <gdefines.h>
#include <new>

__forceinline int _round(double value)
{
   return((int)floor(value + 0.5));
}

__forceinline int _log2(int n)
{
	int log = 0;
	while(n > 1)
	{
		n /= 2;
		log++;
	}
	return(log);
}

#include <common/Math.h>
#include <gdefines.h>
#include <common/array.h>
#include <common/MemAlloc.h>
#include <common/stack.h>
#include <common/assotiativearray.h>
#include <common/string.h>

#include "SXGUI.h"

#include <cstdio>

#pragma comment(lib, "Vfw32.lib")

#define TEX_WHITE L"/dev/white.png"

namespace gui
{
	class IDesktop;
};

#include "Texture.h"

#endif
