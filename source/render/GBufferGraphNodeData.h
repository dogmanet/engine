#ifndef __GBUFFERGRAPHNODEDATA_H
#define __GBUFFERGRAPHNODEDATA_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCamera.h>


class CRender;
class CGBufferGraphNodeData final: public IXUnknownImplementation<IXRenderGraphNodeData>
{
	friend class CGBufferGraphNode;
public:
	CGBufferGraphNodeData(IGXDevice *pDevice, IXRender *pRender);
	~CGBufferGraphNodeData();
	
	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;

private:
	UINT m_uWidth = 0;
	UINT m_uHeight = 0;

	IGXDevice *m_pDevice = NULL;
	
	IGXDepthStencilSurface *m_pDepthStencilSurface = NULL;
};

#endif
