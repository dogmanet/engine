
/*
mtrlgeom_base.vs
Базовый рендер геометрии
*/

#include <struct.h>
#include <const.h>

//##########################################################################

float3 RotateVec(float4 q, float3 p)
{
	float xxzz = q.x*q.x - q.z*q.z;
	float wwyy = q.w*q.w - q.y*q.y;
	float xw2 = q.x*q.w*2.0;
	float xy2 = q.x*q.y*2.0;
	float xz2 = q.x*q.z*2.0;
	float yw2 = q.y*q.w*2.0;
	float yz2 = q.y*q.z*2.0;
	float zw2 = q.z*q.w*2.0;
	float3 oout = float3((xxzz + wwyy)*p.x		+ (xy2 + zw2)*p.y		+ (xz2 - yw2)*p.z,
					(xy2 - zw2)*p.x			+ (q.y*q.y+q.w*q.w-q.x*q.x-q.z*q.z)*p.y	+ (yz2 + xw2)*p.z,
					(xz2 + yw2)*p.x			+ (yz2 - xw2)*p.y		+ (wwyy - xxzz)*p.z);
	return(oout);
}

//##########################################################################

VSO_SceneCommon main(VSI_Geometry IN
#ifdef USE_INSTANCING
, uint uInstanceId: SV_InstanceId
#endif
)
{
	VSO_SceneCommon OUT = (VSO_SceneCommon)0;

#ifdef USE_INSTANCING
	
	float4 vPosScale = g_instanceData[uInstanceId * 2];
	float4 qRot = g_instanceData[uInstanceId * 2 + 1];
	
	OUT.vPosition = float4(RotateVec(qRot, IN.vPosition * vPosScale.w) + vPosScale.xyz, 1.0f);
	// OUT.vPosition = float4(IN.vPosition + vPosScale.xyz, 1.0f);
	// OUT.vPosition = float4(IN.vPosition, 1.0f);

	OUT.vNormal = RotateVec(qRot, IN.vNormal);
	OUT.vTangent  = RotateVec(qRot, IN.vTangent);
	OUT.vBinormal = RotateVec(qRot, IN.vBinormal);
	OUT.uInstanceId.x = uInstanceId;
	// OUT.vNormal = IN.vNormal;
#else
	OUT.vPosition = mul(float4(IN.vPosition, 1.0f), g_mW);
	OUT.vNormal = /*normalize(*/mul(IN.vNormal, (float3x3)g_mW)/*)*/;
	OUT.vTangent  = /*normalize(*/mul(IN.vTangent, (float3x3)g_mW)/*)*/;
	OUT.vBinormal = /*normalize(*/mul(IN.vBinormal, (float3x3)g_mW)/*)*/;
#endif

	
	OUT.vPosition = mul(OUT.vPosition, g_mVP);
	OUT.vTexUV = IN.vTexUV;
	OUT.vPos = OUT.vPosition;
	OUT.vColor = float4(1.0f, 1.0f, 1.0f, 1.0f);

	return OUT;
}
