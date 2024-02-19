
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

VSO_DevPoints main(VSI_DevPoints IN)
{
	VSO_DevPoints OUT = (VSO_DevPoints)0;
	
	float3 vRight = normalize(g_mObserverInvV._m00_m01_m02);
	
	float3 vPos = IN.vPosition.xyz;
	// float3 vRight = g_vCamRight;
// #ifdef IS_ORTHO
	float3 vUp = -normalize(g_mObserverInvV._m10_m11_m12);
// #else
	// float3 vUp = normalize(cross(vRight, vPos - g_vObserverPosCam.xyz));
// #endif
	
	float2 vOffset = IN.vPosition.w * (IN.vTexUVMode.xy - 0.5);
	
#ifdef IS_FIXED

#ifdef IS_ORTHO
	float4x4 mProj = mul(g_mObserverInvV, g_mObserverVP);	
	vOffset.x *= g_vObserverNearFar.z / mProj._m00 * 2.0;
	vOffset.y *= g_vObserverNearFar.w / mProj._m11 * 2.0;
#else
	vOffset *= length(vPos - g_vObserverPosCam.xyz) * 2.0 / g_vObserverNearFar.y;
#endif

#endif

	vPos += vRight * vOffset.x + vUp * vOffset.y;
	
	OUT.vPosition = mul(float4(vPos, 1.0), g_mObserverVP);
	// OUT.vPosition = mul(OUT.vPosition, g_mObserverVP);
	OUT.vTexUVMode = IN.vTexUVMode;
	OUT.vColor = IN.vColor;
	return(OUT);
}
