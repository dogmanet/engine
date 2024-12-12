
/*
terrax_colored.vs

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

VSO_TXColored2 main(VSI_TXMatFrame IN)
{
	VSO_TXColored2 OUT = (VSO_TXColored2)0;
	
	float2 vPos = IN.vPosition.xy + g_vOffset + g_vSizeTitleHighlight.xy * IN.vScaleFactor + float2(g_vSizeTitleHighlight.z * IN.fTitleFactor, 0.0);
	
	OUT.vPosition = mul(float4(vPos * g_fScale, 0.0, 1.0), g_mVP);
	OUT.vColor = IN.vColor * float4(1.0f, 1.0f, 1.0f, lerp(1.0, g_vSizeTitleHighlight.w, IN.fHighlightFactor));
	// float4(g_vSizeTitleHighlight.w, IN.fHighlightFactor); 
	return(OUT);
}
