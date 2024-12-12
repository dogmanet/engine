
/*
skin.h
Функции/данные трансформации анимационных моделей
*/

cbuffer CBoneData: register(ANIM_R_DATA)
{
	float4 g_BufferBoneWorld[128];
};

float3 SkinRotateVec(float4 q, float3 p)
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

VSO_SceneCommon SkinBoneTransform(uint iBone, float weight, VSI_Animation v)
{
	VSO_SceneCommon Output = (VSO_SceneCommon)0;
	iBone *= 2;
	float4 bpos = g_BufferBoneWorld[iBone];
	float4 q = g_BufferBoneWorld[iBone + 1];
	Output.vPosition = float4((SkinRotateVec(q, v.vPosition) + bpos.xyz) * weight, 1.0);
	Output.vNormal = SkinRotateVec(q, v.vNormal) * weight;
	Output.vTangent = SkinRotateVec(q, v.vTangent) * weight;
	Output.vBinormal = SkinRotateVec(q, v.vBinormal) * weight;

	return(Output);
}


VSO_SceneCommon SkinAllTransform(VSI_Animation v)
{
	VSO_SceneCommon Output = (VSO_SceneCommon)0;
	VSO_SceneCommon o = (VSO_SceneCommon)0;
	
	//Bone0
    uint iBone = v.vBones.x;
    float fWeight = v.vWeights.x;
	o = SkinBoneTransform(iBone, fWeight, v);
	Output.vPosition += o.vPosition;
	Output.vNormal += o.vNormal;
	Output.vTangent += o.vTangent;
	Output.vBinormal += o.vBinormal;
    
    //Bone1
    iBone = v.vBones.y;
    fWeight = v.vWeights.y;
	o = SkinBoneTransform(iBone, fWeight, v);
	Output.vPosition += o.vPosition;
	Output.vNormal += o.vNormal;
	Output.vTangent += o.vTangent;
	Output.vBinormal += o.vBinormal;
    
    //Bone2
    iBone = v.vBones.z;
    fWeight = v.vWeights.z;
	o = SkinBoneTransform(iBone, fWeight, v);
	Output.vPosition += o.vPosition;
	Output.vNormal += o.vNormal;
	Output.vTangent += o.vTangent;
	Output.vBinormal += o.vBinormal;
    
    //Bone3
    iBone = v.vBones.w;
    // fWeight = v.vWeights.w;
    fWeight = 1.0 - (v.vWeights.x + v.vWeights.y + v.vWeights.z);
	o = SkinBoneTransform(iBone, fWeight, v);
	Output.vPosition += o.vPosition;
	Output.vNormal += o.vNormal;
	Output.vTangent += o.vTangent;
	Output.vBinormal += o.vBinormal;

	Output.vPosition.w = 1.0;
	
	return(Output);
}
