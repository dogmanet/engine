
/*
terrax_textured.vs

*/

#include <struct.h>
#include <const.h>

//##########################################################################

cbuffer CDataTramsform: register(b6)
{
	float4 g_vSizeTitleHighlight; // xy: size, z: title, w: highlight
	
	float2 g_vOffset;
	
	float g_fScale;
};

VSO_TXTextured main(VSI_TXMatInner IN)
{
	VSO_TXTextured OUT = (VSO_TXTextured)0;
	
	float2 vPos = IN.vPosition.xy + g_vOffset + g_vSizeTitleHighlight.xy * IN.vScaleFactor;
	
	OUT.vPosition = mul(float4(vPos * g_fScale, 0.0, 1.0), g_mVP);
	OUT.vTexUV = IN.vTexUV;
	return(OUT);
}
