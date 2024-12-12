
/*
dev_lines.vs

*/

#include <struct.h>
#include <const.h>

//##########################################################################

VSO_DevLines main(VSI_DevLines IN)
{
	VSO_DevLines OUT = (VSO_DevLines)0;
	float3 vPos = IN.vPosition.xyz;
	
#ifdef IS_ORTHO
	float3 vLook = /* normalize( */g_mObserverInvV._m20_m21_m22/* ) */;
	
	float3 vRight = normalize(cross(vLook, IN.vDir));
#else
	float3 vRight = normalize(cross(vPos - g_vObserverPosCam.xyz, IN.vDir));
#endif

	float fOffset = IN.vPosition.w * (IN.vTexUV.y - 0.5);
	
#ifdef IS_FIXED

#ifdef IS_ORTHO
	float4x4 mProj = mul(g_mObserverInvV, g_mObserverVP);	
	fOffset *= g_vObserverNearFar.z / mProj._m00 * 2.0;
#else
	fOffset *= length(vPos - g_vObserverPosCam.xyz) * 2.0 / g_vObserverNearFar.y;
#endif

#endif

	vPos += vRight * fOffset;
	
	OUT.vPosition = mul(float4(vPos, 1.0), g_mObserverVP);
	// OUT.vPosition = mul(OUT.vPosition, g_mObserverVP);
	OUT.vTexUV = IN.vTexUV;
	OUT.vColor = IN.vColor;
	return(OUT);
}
