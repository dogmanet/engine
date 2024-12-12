#include "stdafx.h"
#include "GeomDetector.h"

bool CGeomDetector::IsBox(float3_t *pMesh, UINT uVertexCount, size_t uStride, float3_t *pCenter, float3_t *pLWH)
{
	if(uVertexCount != 8)
	{
		return(false);
	}
	return(false);

	float3 pt;
	float3 ptMin, ptMax;
	float3_t *pCur = pMesh;
	for(UINT i = 0; i < uVertexCount; ++i)
	{
		pt = *pCur;

		ptMin = SMVectorMin(ptMin, pt);
		ptMax = SMVectorMax(ptMax, pt);

		pCur = (float3_t*)((byte*)pCur + uStride);
	}

	pCur = pMesh;
	for(UINT i = 0; i < uVertexCount; ++i)
	{
		pt = *pCur;

		if(!((pt.x == ptMin.x || pt.x == ptMax.x)
			&& (pt.y == ptMin.y || pt.y == ptMax.y)
			&& (pt.z == ptMin.z || pt.z == ptMax.z)))
		{
			return(false);
		}

		pCur = (float3_t*)((byte*)pCur + uStride);
	}

	*pCenter = (float3)((ptMin + ptMax) * 0.5f);
	*pLWH = (float3)((ptMax - ptMin) * 0.5f);

	return(true);
}

bool CGeomDetector::IsSphere(float3_t *pMesh, UINT uVertexCount, size_t uStride, float3_t *pCenter, float3_t *pLWH)
{
	float3 pt;
	float3 ptCenter;
	float fRadius = 0.0f;


	float3 ptMin, ptMax;
	float3_t *pCur = pMesh;
	for(UINT i = 0; i < uVertexCount; ++i)
	{
		pt = *pCur;

		ptMin = SMVectorMin(ptMin, pt);
		ptMax = SMVectorMax(ptMax, pt);

		pCur = (float3_t*)((byte*)pCur + uStride);
	}

	float fx = ptMax.x - ptMin.x;
	float fy = ptMax.y - ptMin.y;
	float fz = ptMax.z - ptMin.z;

	if(fx != fy || fy != fz)
	{
		return(false);
	}

	fRadius = fx * 0.5f;
	ptCenter.x = (ptMin.x + ptMax.x) * 0.5f;
	ptCenter.y = (ptMin.y + ptMax.y) * 0.5f;
	ptCenter.z = (ptMin.z + ptMax.z) * 0.5f;

	float r21 = fRadius * fRadius, r22;
	pCur = pMesh;
	for(UINT i = 0; i < uVertexCount; ++i)
	{
		pt = *pCur - ptCenter;

		r22 = pt.x * pt.x + pt.y * pt.y + pt.z * pt.z;

		if(r21 - r22 > 0.0256f)
		{
			return(false);
		}
		pCur = (float3_t*)((byte*)pCur + uStride);
	}

	*pCenter = ptCenter;

	pLWH->x = pLWH->y = pLWH->z = fRadius;

	return(true);
}

bool CGeomDetector::IsCylinder(float3_t *pMesh, UINT uVertexCount, size_t uStride, float3_t *pCenter, float *pHeight, float *pRadius)
{
	float3 pt;
	float3 ptCenter;
	float fRadius = 0.0f;

	float3 ptMin, ptMax;
	float3_t *pCur = pMesh;
	for(UINT i = 0; i < uVertexCount; ++i)
	{
		pt = *pCur;

		ptMin = SMVectorMin(ptMin, pt);
		ptMax = SMVectorMax(ptMax, pt);

		pCur = (float3_t*)((byte*)pCur + uStride);
	}

	float fx = ptMax.x - ptMin.x;
	float fy = ptMax.y - ptMin.y;
	float fz = ptMax.z - ptMin.z;

	bool isX = fy == fz && fx > fz;
	bool isY = fx == fz && fz > fx;
	bool isZ = fy == fz && fx != fz;

	if((!isX && !isY && !isZ) || (isX && isY) || (isX && isZ) || (isY && isZ))
	{
		return(false);
	}

	return(false);
}
