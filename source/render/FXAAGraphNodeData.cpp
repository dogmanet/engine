#include "FXAAGraphNodeData.h"
#include <xcommon/IXRenderable.h>

CFXAAGraphNodeData::CFXAAGraphNodeData(IGXDevice *pDevice, IXRender *pRender):
	m_pDevice(pDevice)
{
}
CFXAAGraphNodeData::~CFXAAGraphNodeData()
{
}

void XMETHODCALLTYPE CFXAAGraphNodeData::resize(UINT uWidth, UINT uHeight)
{
	m_uWidth = uWidth;
	m_uHeight = uHeight;
}
