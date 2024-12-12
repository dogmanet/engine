#include "Mesh.h"

IGXVertexDeclaration *CMesh::ms_pVD = 0;
UINT CMesh::ms_uInstanceCount = 0;

CMesh::CMesh(IGXDevice *pDevice, UINT uVertexCount, UINT uIndexCount, float3_t *pVertices, USHORT *pIndices):
	m_pDevice(pDevice),
	m_uVertexCount(uVertexCount),
	m_uIndexCount(uIndexCount)
{
	if(ms_uInstanceCount == 0)
	{
		++ms_uInstanceCount;

		GXVertexElement oLayout[] =
		{
			{0, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_POSITION},
			GX_DECL_END()
		};

		ms_pVD = m_pDevice->createVertexDeclaration(oLayout);
	}

	m_pIB = m_pDevice->createIndexBuffer(sizeof(USHORT)* uIndexCount, GXBUFFER_USAGE_STATIC, GXIT_UINT16, pIndices);
	m_pVB = m_pDevice->createVertexBuffer(sizeof(float)* 3 * uVertexCount, GXBUFFER_USAGE_STATIC, pVertices);
	m_pRB = m_pDevice->createRenderBuffer(1, &m_pVB, ms_pVD);
}
CMesh::~CMesh()
{
	if(--ms_uInstanceCount == 0)
	{
		mem_release(ms_pVD);
	}
	mem_release(m_pIB);
	mem_release(m_pVB);
	mem_release(m_pRB);
}
void XMETHODCALLTYPE CMesh::draw(IGXContext *pCtx)
{
	if(!pCtx)
	{
		pCtx = m_pDevice->getThreadContext();
	}
	pCtx->setRenderBuffer(m_pRB);
	pCtx->setIndexBuffer(m_pIB);
	pCtx->drawIndexed(m_uVertexCount, m_uIndexCount / 3, 0, 0);
}
IGXVertexBuffer* XMETHODCALLTYPE CMesh::getVertexBuffer()
{
	return(m_pVB);
}
IGXIndexBuffer* XMETHODCALLTYPE CMesh::getIndexBuffer()
{
	return(m_pIB);
}
