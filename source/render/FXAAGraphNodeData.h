#ifndef __FXAAGRAPHNODEDATA_H
#define __FXAAGRAPHNODEDATA_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCamera.h>

class CRender;
class CFXAAGraphNodeData final: public IXUnknownImplementation<IXRenderGraphNodeData>
{
	//friend class CFXAAGraphNode;
public:
	CFXAAGraphNodeData(IGXDevice *pDevice, IXRender *pRender);
	~CFXAAGraphNodeData();
	
	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;


private:
	UINT m_uWidth = 0;
	UINT m_uHeight = 0;

	IGXDevice *m_pDevice = NULL;
};

#endif
