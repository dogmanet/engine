#include "GBufferGraphNodeData.h"
#include <xcommon/IXRenderable.h>

CGBufferGraphNodeData::CGBufferGraphNodeData(IGXDevice *pDevice, IXRender *pRender):
	m_pDevice(pDevice)
{
}
CGBufferGraphNodeData::~CGBufferGraphNodeData()
{
	mem_release(m_pDepthStencilSurface);
}

void XMETHODCALLTYPE CGBufferGraphNodeData::resize(UINT uWidth, UINT uHeight)
{
	m_uWidth = uWidth;
	m_uHeight = uHeight;

	// resize targets and swap chain
	mem_release(m_pDepthStencilSurface);
	/*
	//GXFMT_A16B16G16R16F; // 64bpp; GXFMT_A8R8G8B8
	m_pGBufferColor = m_pDevice->createTexture2D(m_uWidth, m_uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A8B8G8R8);
	//GXFMT_A16B16G16R16F; // 64bpp; GXFMT_A8R8G8B8
	m_pGBufferNormals = m_pDevice->createTexture2D(m_uWidth, m_uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A8B8G8R8);
	//GXFMT_A16B16G16R16F; // 64bpp; GXFMT_A8R8G8B8
	m_pGBufferParams = m_pDevice->createTexture2D(m_uWidth, m_uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A8B8G8R8);
	//GXFMT_G32R32F; // 64bpp; GXFMT_R32F
	m_pGBufferDepth = m_pDevice->createTexture2D(m_uWidth, m_uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_R32F);
	*/
	m_pDepthStencilSurface = m_pDevice->createDepthStencilSurface(uWidth, uHeight, GXFMT_D24S8, GXMULTISAMPLE_NONE);
}
