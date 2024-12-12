#ifndef __2DGRAPHNODEDATA_H
#define __2DGRAPHNODEDATA_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCamera.h>


class CRender;
class C2DGraphNodeData final: public IXUnknownImplementation<IXRenderGraphNodeData>
{
	friend class C2DGraphNode;
public:
	C2DGraphNodeData(IGXDevice *pDevice, IXRender *pRender);
	~C2DGraphNodeData();
	
	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;

private:
	UINT m_uWidth = 0;
	UINT m_uHeight = 0;

	IGXDevice *m_pDevice = NULL;

	IGXDepthStencilSurface *m_pDepthStencilSurface = NULL;
};

#endif
