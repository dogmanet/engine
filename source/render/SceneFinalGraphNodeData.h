#ifndef __SCENEFINALGRAPHNODEDATA_H
#define __SCENEFINALGRAPHNODEDATA_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCamera.h>

class CRender;
class CSceneFinalGraphNodeData final: public IXUnknownImplementation<IXRenderGraphNodeData>
{
	friend class CSceneFinalGraphNode;
public:
	CSceneFinalGraphNodeData(IGXDevice *pDevice, IXRender *pRender);
	~CSceneFinalGraphNodeData();

	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;

private:
	UINT m_uWidth = 0;
	UINT m_uHeight = 0;

	IGXDevice *m_pDevice = NULL;
};

#endif
