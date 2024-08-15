
/*
particles.vs
Рендер частиц
*/

#include <struct.h>
#include <const.h>

#ifdef WITH_TRANSFORM
cbuffer CDataParticleObject: register(b1)
{
	float4 g_qW;
	float3 g_vW;
};
#endif

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

// Quaternion multiplication
// http://mathworld.wolfram.com/Quaternion.html
float4 QMul(float4 q1, float4 q2)
{
	return float4(
		q2.xyz * q1.w + q1.xyz * q2.w + cross(q1.xyz, q2.xyz),
		q1.w * q2.w - dot(q1.xyz, q2.xyz)
	);
}

//##########################################################################

VSO_SceneCommon main(VSI_Particle IN)
{
	VSO_SceneCommon OUT = (VSO_SceneCommon)0;

	float3 vRight = normalize(g_mInvV._m00_m01_m02);
	float3 vUp = normalize(g_mInvV._m10_m11_m12);
	float3 vForward = normalize(g_mInvV._m20_m21_m22);
		
	IN.qRotLocal.xyz = mul(IN.qRotLocal.xyz, float3x3(vRight, vUp, vForward));
	
	float4 qResult = QMul(IN.qRotGlobal, IN.qRotLocal);
	
	vRight = RotateVec(qResult, vRight);
	vUp = RotateVec(qResult, vUp);
	vForward = RotateVec(qResult, vForward);
	
	float2 vOffset = IN.vSize.xy * (IN.vTexUV - 0.5f);

	float3 vTotalOffset = vRight * vOffset.x + vUp * vOffset.y;

	float3 vPos = 
#ifdef WITH_TRANSFORM
		g_vW + RotateVec(g_qW, IN.vPosition.xyz)
#else
		IN.vPosition.xyz
#endif
		+ vTotalOffset;

	OUT.vPosition = mul(float4(vPos, 1.0f), g_mVP);
	
	OUT.vNormal = -vForward;
	//OUT.vNormal = normalize(vTotalOffset);
	
	OUT.vTangent = -vUp;
	OUT.vBinormal = vRight;
	OUT.vTexUV = float2(IN.vTexUV.x, 1.0f - IN.vTexUV.y);
	OUT.vColor = IN.vColor;

	OUT.vPos = OUT.vPosition;

	return(OUT);
}
