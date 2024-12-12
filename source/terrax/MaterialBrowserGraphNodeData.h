#ifndef __MATERIALBROWSERGRAPHNODEDATA_H
#define __MATERIALBROWSERGRAPHNODEDATA_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCamera.h>

class CRender;
class CMaterialBrowserGraphNodeData final: public IXUnknownImplementation<IXRenderGraphNodeData>
{
	//friend class C3DOverlayGraphNode;
public:
	CMaterialBrowserGraphNodeData(IGXDevice *pDevice, IXRender *pRender);
	~CMaterialBrowserGraphNodeData();
	
	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;

private:
	UINT m_uWidth = 0;
	UINT m_uHeight = 0;

	IGXDevice *m_pDevice = NULL;
};

#endif
