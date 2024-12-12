
/*
gui_main.vs

*/

#include <struct.h>

//##########################################################################

float4x4 g_mWVP1: register(b0);

//##########################################################################

VSO_GUITextured main(VSI_GUITextured IN)
{
	VSO_GUITextured OUT = (VSO_GUITextured)0;
	OUT.vPosition = mul(float4(IN.vPosition.xyz, 1), g_mWVP1);
	OUT.vTexUV = IN.vTexUV;
	return(OUT);
}
