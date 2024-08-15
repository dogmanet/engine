#ifndef __RENDERUTILS_H
#define __RENDERUTILS_H

#include <xcommon/render/IXRenderUtils.h>

class IXRender;
class CRenderUtils final: public IXUnknownImplementation<IXRenderUtils>
{
public:
	CRenderUtils(IXRender *pRender);
	~CRenderUtils();

	void XMETHODCALLTYPE newGizmoRenderer(IXGizmoRenderer **ppOut) override;
	void XMETHODCALLTYPE newLineRenderer(IXLineRenderer **ppOut) override;
	void XMETHODCALLTYPE getQuadIndexBuffer(UINT uQuads, IGXIndexBuffer **ppIndexBuffer) override;

	void XMETHODCALLTYPE createConeMesh(float fTopRadius, float fBottomRadius, float fHeight, IXMesh **ppMesh, UINT uSideCount) override;

	void XMETHODCALLTYPE createSphereMesh(float fRadius, UINT uSideCount, UINT uStackCount, IXMesh **ppMesh) override;

	void XMETHODCALLTYPE createBoundingBoxMesh(const float3 &vMin, const float3 &vMax, IXMesh **ppMesh) override;

private:
	UINT m_uQuadIBSize = 0;
	IGXIndexBuffer *m_pQuadIB = NULL;
	IXRender *m_pRender = NULL;
	IGXDevice *m_pDevice = NULL;

	Array<float3_t> m_aVCache;
	Array<USHORT> m_aICache;
};

#endif
