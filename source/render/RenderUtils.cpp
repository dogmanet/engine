#include "RenderUtils.h"
#include "GizmoRenderer.h"
#include "LineRenderer.h"
//#include <gcore/sxgcore.h>
#include "Mesh.h"

CRenderUtils::CRenderUtils(IXRender *pRender):
	m_pRender(pRender),
	m_pDevice(pRender->getDevice())
{
}

CRenderUtils::~CRenderUtils()
{
	mem_release(m_pQuadIB);
}

void XMETHODCALLTYPE CRenderUtils::newGizmoRenderer(IXGizmoRenderer **ppOut)
{
	*ppOut = new CGizmoRenderer(this, m_pRender);
}

void XMETHODCALLTYPE CRenderUtils::newLineRenderer(IXLineRenderer **ppOut)
{
	*ppOut = new CLineRenderer(m_pRender);
}

void XMETHODCALLTYPE CRenderUtils::getQuadIndexBuffer(UINT uQuads, IGXIndexBuffer **ppIndexBuffer)
{
	assert(uQuads < USHRT_MAX / 4);

	if(uQuads > m_uQuadIBSize)
	{
		UINT m_uFontIBChars = 1;
		while(m_uFontIBChars < uQuads)
		{
			m_uFontIBChars *= 2;
		}

		USHORT *pIB = new USHORT[m_uFontIBChars * 6];

		UINT CI = 0;
		for(UINT i = 0; i < m_uFontIBChars; ++i)
		{
			pIB[CI++] = i * 4 + 0;
			pIB[CI++] = i * 4 + 1;
			pIB[CI++] = i * 4 + 2;
			pIB[CI++] = i * 4 + 2;
			pIB[CI++] = i * 4 + 1;
			pIB[CI++] = i * 4 + 3;
		}

		mem_release(m_pQuadIB);
		m_pQuadIB = m_pDevice->createIndexBuffer(sizeof(USHORT)* m_uFontIBChars * 6, GXBUFFER_USAGE_STATIC, GXIT_UINT16, pIB);

		mem_delete_a(pIB);

		m_uQuadIBSize = m_uFontIBChars;
	}

	add_ref(m_pQuadIB);
	*ppIndexBuffer = m_pQuadIB;
}

void XMETHODCALLTYPE CRenderUtils::createConeMesh(float fTopRadius, float fBottomRadius, float fHeight, IXMesh **ppMesh, UINT uSideCount)
{
	UINT uVC = uSideCount * 2;
	UINT uIC = (uSideCount - 2) * 6 + uSideCount * 6;
	m_aVCache.resizeFast(uVC);
	m_aICache.resizeFast(uIC);
	float3_t *pVertices = m_aVCache;
	USHORT *pIndices = m_aICache;

	//top
	UINT iCurV = 0;
	for(UINT i = 0; i < uSideCount; i++)
	{
		float A = (float)i / (float)uSideCount * SM_2PI;
		pVertices[iCurV] = float3_t(fTopRadius * cosf(A), 0.0f, -fTopRadius * sinf(A));
		iCurV++;
	}

	//bottom
	for(UINT i = 0; i < uSideCount; i++)
	{
		float A = (float)i / (float)uSideCount * SM_2PI;
		pVertices[iCurV] = float3_t(fBottomRadius * cosf(A), -fHeight, fBottomRadius * sinf(A));
		iCurV++;
	}

	//top
	UINT uCurI = 0;
	for(UINT i = 0; i < uSideCount - 2; i++)
	{
		pIndices[uCurI] = 0;
		uCurI++;
		pIndices[uCurI] = 1 + i;
		uCurI++;
		pIndices[uCurI] = 2 + i;
		uCurI++;
	}

	//bottom
	for(UINT i = 0; i < uSideCount - 2; i++)
	{
		pIndices[uCurI] = uSideCount;
		uCurI++;
		pIndices[uCurI] = uSideCount + 1 + i;
		uCurI++;
		pIndices[uCurI] = uSideCount + 2 + i;
		uCurI++;
	}

	//side
	for(UINT i = 0; i < uSideCount; i++)
	{
		pIndices[uCurI] = (i + 1) % uSideCount;
		uCurI++;
		pIndices[uCurI] = i;
		uCurI++;
		pIndices[uCurI] = (uSideCount - i) % uSideCount + uSideCount;
		uCurI++;

		pIndices[uCurI] = (uSideCount - i) % uSideCount + uSideCount;
		uCurI++;
		pIndices[uCurI] = (2 * uSideCount - i - 1) % uSideCount + uSideCount;
		uCurI++;
		pIndices[uCurI] = (i + 1) % uSideCount;
		uCurI++;
	}

	IXMesh *pMesh = new CMesh(m_pDevice, uVC, uIC, pVertices, pIndices);

	(*ppMesh) = pMesh;
}

void XMETHODCALLTYPE CRenderUtils::createSphereMesh(float fRadius, UINT uSideCount, UINT uStackCount, IXMesh **ppMesh)
{
	UINT uVC = (uStackCount - 1) * uSideCount + 2;
	UINT uIC = (uStackCount - 1) * uSideCount * 6;
	m_aVCache.resizeFast(uVC);
	m_aICache.resizeFast(uIC);
	float3_t *pVertices = m_aVCache;
	USHORT *pIndices = m_aICache;

	UINT uCurV = 0;
	UINT uCurI = 0;
	pVertices[uCurV++] = float3_t(0.0f, fRadius, 0.0f);
	UINT uPrevBaseVtx;
	for(UINT i = 0; i < uStackCount; ++i)
	{
		float A = (float)(i + 1) / (float)uStackCount * SM_PI;
		float fCurrentRadius = sinf(A) * fRadius;
		float fCurrentY = cosf(A) * fRadius;
		UINT uBaseVtx = uCurV;
		for(UINT j = 0; j < uSideCount; ++j)
		{
			A = (float)j / (float)uSideCount * SM_2PI;
			float x = fCurrentRadius * cosf(A);
			float z = -fCurrentRadius * sinf(A);
			// north pole
			if(i == 0)
			{
				pVertices[uCurV++] = float3_t(x, fCurrentY, z);
				pIndices[uCurI++] = 0;
				pIndices[uCurI++] = uBaseVtx + j;
				pIndices[uCurI++] = uBaseVtx + (j + 1) % uSideCount;
			}
			// south pole
			else if(i == uStackCount - 1)
			{
				pIndices[uCurI++] = uPrevBaseVtx + (j + 1) % uSideCount;
				pIndices[uCurI++] = uPrevBaseVtx + j;
				pIndices[uCurI++] = uBaseVtx;
			}
			else
			{
				pVertices[uCurV++] = float3_t(x, fCurrentY, z);
				pIndices[uCurI++] = uPrevBaseVtx + j;
				pIndices[uCurI++] = uBaseVtx + (j + 1) % uSideCount;
				pIndices[uCurI++] = uPrevBaseVtx + (j + 1) % uSideCount;

				pIndices[uCurI++] = uPrevBaseVtx + j;
				pIndices[uCurI++] = uBaseVtx + j;
				pIndices[uCurI++] = uBaseVtx + (j + 1) % uSideCount;
			}
		}
		uPrevBaseVtx = uBaseVtx;
	}
	pVertices[uCurV++] = float3_t(0.0f, -fRadius, 0.0f);

	assert(uCurI == uIC);
	assert(uCurV == uVC);

	IXMesh *pMesh = new CMesh(m_pDevice, uVC, uIC, pVertices, pIndices);

	(*ppMesh) = pMesh;
}

void XMETHODCALLTYPE CRenderUtils::createBoundingBoxMesh(const float3 &vMin, const float3 &vMax, IXMesh **ppMesh)
{
	float dist_x = abs(vMax.x - vMin.x);
	float dist_y = abs(vMax.y - vMin.y);
	float dist_z = abs(vMax.z - vMin.z);

	UINT uVC = 8;
	UINT uIC = 36;
	float3_t pVertices[] = {
		float3_t(vMin.x, vMin.y, vMin.z),
		float3_t(vMin.x, vMin.y, vMax.z),
		float3_t(vMin.x, vMax.y, vMin.z),
		float3_t(vMin.x, vMax.y, vMax.z),
		float3_t(vMax.x, vMin.y, vMin.z),
		float3_t(vMax.x, vMin.y, vMax.z),
		float3_t(vMax.x, vMax.y, vMin.z),
		float3_t(vMax.x, vMax.y, vMax.z),
	};
	USHORT pIndices[] = {
		0, 1, 3, 0, 3, 2,
		0, 6, 4, 0, 2, 6,
		0, 5, 1, 0, 4, 5,
		4, 7, 5, 4, 6, 7,
		6, 3, 7, 6, 2, 3,
		3, 5, 7, 3, 1, 5
	};

	IXMesh *pMesh = new CMesh(m_pDevice, uVC, uIC, pVertices, pIndices);

	(*ppMesh) = pMesh;
}
