#ifndef __XMAT_H
#define __XMAT_H

#ifdef XMAT_MS_TEXTURES
XMAT_MS_TEXTURES();
#endif
#ifdef XMAT_MS_SAMPLERS
XMAT_MS_SAMPLERS();
#endif

struct XMaterial
{
#ifdef XMATERIAL_OUTPUT_STRUCT
	XMATERIAL_OUTPUT_STRUCT();
#endif
#ifdef WANT_WRITE_DEPTH
	float fDepth;
#endif
};

struct PSI_XMaterial
{
	XMAT_PS_STRUCT();
};

XMATERIAL_DEFAULT_LOADER();

//##########################################################################

#ifdef XMATERIAL_SHADER
#include XMATERIAL_SHADER
#endif



#endif
