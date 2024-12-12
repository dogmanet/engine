#ifndef _GeomDetector_H_
#define _GeomDetector_H_

#include "stdafx.h"

class CGeomDetector
{
public:
	static bool IsBox(float3_t *pMesh, UINT uVertexCount, size_t uStride, float3_t *pCenter, float3_t *pLWH);
	static bool IsSphere(float3_t *pMesh, UINT uVertexCount, size_t uStride, float3_t *pCenter, float3_t *pLWH);
	static bool IsCylinder(float3_t *pMesh, UINT uVertexCount, size_t uStride, float3_t *pCenter, float *pHeight, float *pRadius);
};

#endif
