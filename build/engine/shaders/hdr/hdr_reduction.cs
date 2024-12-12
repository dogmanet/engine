
#define THREAD_GROUP_DIM 16
#define TOTAL_THREADS (THREAD_GROUP_DIM * THREAD_GROUP_DIM)

Texture2D<float> g_txInput: register(t0);

RWTexture2D<float> g_txOutput: register(u0);

groupshared float4 g_vShared[TOTAL_THREADS];

[numthreads(THREAD_GROUP_DIM, THREAD_GROUP_DIM, 1)]
void main(uint3 vGroupID: SV_GroupID, uint3 vGroupThreadID: SV_GroupThreadID)
{
	const uint uThreadID = vGroupThreadID.y * THREAD_GROUP_DIM + vGroupThreadID.x;

	const uint2 vSampleID = (vGroupID.xy * THREAD_GROUP_DIM + vGroupThreadID.xy) * 2;
	float4 vVal = 0.0f;
	vVal.x = g_txInput[vSampleID + uint2(0, 0)];
	vVal.y = g_txInput[vSampleID + uint2(1, 0)];
	vVal.z = g_txInput[vSampleID + uint2(0, 1)];
	vVal.w = g_txInput[vSampleID + uint2(1, 1)];

#ifdef APPLY_MASK
	float2 vCenter = float2(THREAD_GROUP_DIM - 0.5f, THREAD_GROUP_DIM - 0.5f);
	float fMaxRadius = length(vCenter);
	float fRadius = length(float2(vSampleID) - float2(THREAD_GROUP_DIM - 0.5f, THREAD_GROUP_DIM - 0.5f));
	float fWeight = smoothstep(0.0f, 1.0f, 1.0f - (fRadius / fMaxRadius)) * 0.009740113406211618f;
	
	vVal *= fWeight;
#endif

	g_vShared[uThreadID] = vVal;
	GroupMemoryBarrierWithGroupSync();

	[unroll(TOTAL_THREADS)]
	for(uint s = TOTAL_THREADS / 2; s > 0; s >>= 1)
	{
		if(uThreadID < s)
			g_vShared[uThreadID] += g_vShared[uThreadID + s];

		GroupMemoryBarrierWithGroupSync();
	}

	if(uThreadID == 0)
	{
#ifdef APPLY_MASK
		g_txOutput[vGroupID.xy] = dot(g_vShared[0], 0.25f);
#else
		g_txOutput[vGroupID.xy] = dot(g_vShared[0], 0.25f) / (float)(TOTAL_THREADS);
#endif
		// g_txOutput[vGroupID.xy] = -10.0f;
	}
}
