
/*
const.h
Файл включения в шейдеры (вершинные и пиксельные)
Определены все структуры которые используются
*/

/* struct CDataScene
{
	float4x4 mP;
	float4 vTimeDelta;
	float4 vWinSize;
	float4 vNearFarLayers;
}; */
cbuffer CDataObserverCamera: register(b5)
{
	// float4x4 mV;
	float4x4 g_mObserverVP;
	float4 g_vObserverPosCam;
	float4x4 g_mObserverInvVP;
	float4x4 g_mObserverInvV;
	
	float4 g_vObserverNearFar; // invWinSize
	float3 g_vObserverParamProj;
};
cbuffer CDataFrame: register(b4)
{
	float g_fFrameTime;
};
/* cbuffer CDataScene: register(b3)
{
	float4 g_vNearFarInvWinSize;
}; */
cbuffer CDataCamera: register(b2)
{
	// float4x4 mV;
	float4x4 g_mVP;
	float4 g_vPosCam;
	float4x4 g_mInvVP;
	float4x4 g_mInvV;
	
	float4 g_vNearFar;
	float3 g_vParamProj;
};
#ifdef USE_INSTANCING
cbuffer CInstanceData: register(b1)
{
	float4 g_instanceData[MAX_INSTANCES * 2];
};
#else
cbuffer CDataObject: register(b1)
{
	float4x4 g_mW;
	// float4x4 g_mWV;
	// float4x4 g_mWVP;
};
#endif
/* cbuffer CDataMaterial: register(b0)
{
	float4 g_vUserVS;
	float4 g_vUserPS;
	float4 g_vDestColor;
	float4 g_vNearFarLayers;
}; */



#ifdef XMATERIAL_HAS_CONSTANTS
cbuffer XMaterialConstants: register(b7)
{
    struct
	{
        XMAT_MS_CONST_STRUCT();
    } g_xMaterialConstants;
};
#else
cbuffer CSkyBoxLight: register(b7)
{
    float4 g_vSkyBoxLights[16][4][16][3];
};	
#endif

cbuffer CObjectColors: register(b8)
{
	float4 g_avColors[128];
};






