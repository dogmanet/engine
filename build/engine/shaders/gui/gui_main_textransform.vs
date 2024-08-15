
/*
gui_main_textransform.vs

*/

#include <struct.h>

//##########################################################################

cbuffer CBMain: register(b0)
{
	float4x4 g_mWVP1;
};
cbuffer CBTransform: register(b1)
{
	float2 g_vTxTransform;
};

//##########################################################################

VSO_GUITextured main(VSI_GUITextured IN)
{
	VSO_GUITextured OUT = (VSO_GUITextured)0;
	OUT.vPosition = mul(float4(IN.vPosition.xyz, 1), g_mWVP1);
	OUT.vTexUV = IN.vTexUV + g_vTxTransform;
	return(OUT);
}
