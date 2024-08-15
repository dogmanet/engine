
/*
dev_lines.vs

*/

#include <struct.h>
#include <const.h>

//##########################################################################

/* cbuffer CDataFrame: register(b6)
{
	float3 g_vCamRight;
}; */

//##########################################################################

VSO_DevText main(VSI_DevText IN)
{
	VSO_DevText OUT = (VSO_DevText)0;
	
	float3 vRight = normalize(g_mObserverInvV._m00_m01_m02);
	float3 vUp = -normalize(g_mObserverInvV._m10_m11_m12);
	
	// float3 vPos = /* IN.vRefPos +  */float3(IN.vPosTexUV.xy + float2(100, 100), 0.0f);
	
#ifdef IS_ORTHO
	IN.vRefPos.xy = round(IN.vRefPos.xy);
#endif
	
// #ifdef IS_FIXED

	float4x4 mProj = mul(g_mObserverInvV, g_mObserverVP);	
#ifdef IS_ORTHO
	IN.vPosTexUV.x *= g_vObserverNearFar.z / mProj._m00 * 2.0;
	IN.vPosTexUV.y *= g_vObserverNearFar.w / mProj._m11 * 2.0;
#else
	// IN.vPosTexUV.xy *= length(IN.vRefPos - g_vObserverPosCam.xyz) * 2.0 / g_vObserverNearFar.y;
	
	IN.vPosTexUV.xy *= (length(IN.vRefPos - g_vObserverPosCam.xyz) / g_vObserverNearFar.x) * g_vObserverNearFar.w / mProj._m11 / 20.0f;
#endif

// #endif
	
	float3 vPos = IN.vRefPos + (IN.vPosTexUV.x * vRight) + (IN.vPosTexUV.y * vUp);
	
	
	OUT.vPosition = mul(float4(vPos, 1.0), g_mObserverVP);
	// OUT.vPosition = mul(OUT.vPosition, g_mObserverVP);
	OUT.vTexUV = IN.vPosTexUV.zw;
	OUT.vColor = IN.vColor;
	return(OUT);
}
