#include "MaterialBrowserGraphNodeData.h"
#include <xcommon/IXRenderable.h>

CMaterialBrowserGraphNodeData::CMaterialBrowserGraphNodeData(IGXDevice *pDevice, IXRender *pRender):
	m_pDevice(pDevice)
{
}
CMaterialBrowserGraphNodeData::~CMaterialBrowserGraphNodeData()
{
}

void XMETHODCALLTYPE CMaterialBrowserGraphNodeData::resize(UINT uWidth, UINT uHeight)
{
	m_uWidth = uWidth;
	m_uHeight = uHeight;
}
