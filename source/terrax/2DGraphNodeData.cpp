#include "2DGraphNodeData.h"
#include <xcommon/IXRenderable.h>

C2DGraphNodeData::C2DGraphNodeData(IGXDevice *pDevice, IXRender *pRender):
	m_pDevice(pDevice)
{
}
C2DGraphNodeData::~C2DGraphNodeData()
{
	mem_release(m_pDepthStencilSurface);
}

void XMETHODCALLTYPE C2DGraphNodeData::resize(UINT uWidth, UINT uHeight)
{
	m_uWidth = uWidth;
	m_uHeight = uHeight;

	m_pDepthStencilSurface = m_pDevice->createDepthStencilSurface(uWidth, uHeight, GXFMT_D24S8, GXMULTISAMPLE_NONE);
}
