#ifndef __MESH_H
#define __MESH_H

#include <xcommon/render/IXRenderUtils.h>

class CMesh final: public IXUnknownImplementation<IXMesh>
{
public:
	CMesh(IGXDevice *pDevice, UINT uVertexCount, UINT uIndexCount, float3_t *pVertices, USHORT *pIndices);
	~CMesh();
	void XMETHODCALLTYPE draw(IGXContext *pCtx = NULL) override;
	IGXVertexBuffer* XMETHODCALLTYPE getVertexBuffer() override;
	IGXIndexBuffer* XMETHODCALLTYPE getIndexBuffer() override;

private:
	IGXDevice *m_pDevice;
	IGXVertexBuffer *m_pVB;
	IGXRenderBuffer *m_pRB;
	IGXIndexBuffer *m_pIB;
	UINT m_uVertexCount;
	UINT m_uIndexCount;

	static IGXVertexDeclaration *ms_pVD;
	static UINT ms_uInstanceCount;
};

#endif
