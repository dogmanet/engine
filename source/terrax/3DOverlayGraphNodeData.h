#ifndef __3DOVERLAYGRAPHNODEDATA_H
#define __3DOVERLAYGRAPHNODEDATA_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCamera.h>

class CRender;
class C3DOverlayGraphNodeData final: public IXUnknownImplementation<IXRenderGraphNodeData>
{
	friend class C3DOverlayGraphNode;
public:
	C3DOverlayGraphNodeData(IGXDevice *pDevice, IXRender *pRender);
	~C3DOverlayGraphNodeData();
	
	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;

private:
	UINT m_uWidth = 0;
	UINT m_uHeight = 0;

	IGXDevice *m_pDevice = NULL;
};

#endif
