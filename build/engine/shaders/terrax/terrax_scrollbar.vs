
/*
terrax_colored.vs

*/

#include <struct.h>
#include <const.h>

//##########################################################################

cbuffer CDataTramsform: register(b6)
{
	float4 g_vHighlight; // x: top, y: bottom, z: handler, w: line
	
	float2 g_vOffset;
	float g_fSize;
	float g_fHandlerPos;
	
	float g_fHandlerSize;
	
	float g_fScale;
};

VSO_TXColored2 main(VSI_TXScrollbar IN)
{
	VSO_TXColored2 OUT = (VSO_TXColored2)0;
	
	const float4 c_vColor = float4(198.0f / 255.0f, 240.0f / 255.0f, 253.0f / 255.0f, 1.0f /*144.0f / 255.0f*/);
	
	float2 vPos = IN.vPosition.xy * g_fScale + g_vOffset - float2(0.0f, IN.vScaleFactor.x * g_fSize + IN.vMarker.z * g_fHandlerPos + IN.vScaleFactor.y * g_fHandlerSize);
		
	OUT.vPosition = mul(float4(vPos, 0.0, 1.0), g_mVP);
	OUT.vColor = c_vColor * dot(g_vHighlight, IN.vMarker);
	// float4(g_vSizeTitleHighlight.w, IN.fHighlightFactor); 
	return(OUT);
}
