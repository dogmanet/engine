#ifndef __GAMEUIGRAPHNODEDATA_H
#define __GAMEUIGRAPHNODEDATA_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCamera.h>

class CRender;
class CGameUIGraphNodeData final: public IXUnknownImplementation<IXRenderGraphNodeData>
{
	friend class CGameUIGraphNode;
public:
	CGameUIGraphNodeData(IGXDevice *pDevice, IXRender *pRender);
	~CGameUIGraphNodeData();
	
	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;

private:
	UINT m_uWidth = 0;
	UINT m_uHeight = 0;

	IGXDevice *m_pDevice = NULL;
};

#endif
