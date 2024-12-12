#include "GameUIGraphNodeData.h"
#include <xcommon/IXRenderable.h>

CGameUIGraphNodeData::CGameUIGraphNodeData(IGXDevice *pDevice, IXRender *pRender):
	m_pDevice(pDevice)
{
}
CGameUIGraphNodeData::~CGameUIGraphNodeData()
{
	//mem_release(m_pGBufferColor);
}

void XMETHODCALLTYPE CGameUIGraphNodeData::resize(UINT uWidth, UINT uHeight)
{
	m_uWidth = uWidth;
	m_uHeight = uHeight;

	// resize targets and swap chain
	//mem_release(m_pGBufferColor);

	//GXFMT_A16B16G16R16F; // 64bpp; GXFMT_A8R8G8B8
	//m_pGBufferColor = m_pDevice->createTexture2D(m_uWidth, m_uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A8B8G8R8);
}
