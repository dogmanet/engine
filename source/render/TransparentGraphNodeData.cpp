#include "TransparentGraphNodeData.h"
#include <xcommon/IXRenderable.h>

CTransparentGraphNodeData::CTransparentGraphNodeData(IGXDevice *pDevice, IXRender *pRender):
	m_pDevice(pDevice)
{
	memset(m_pRefractiveTextures, 0, sizeof(m_pRefractiveTextures));
}
CTransparentGraphNodeData::~CTransparentGraphNodeData()
{
	mem_release(m_pRefractiveTextures[0]);
	mem_release(m_pRefractiveTextures[1]);
}

void XMETHODCALLTYPE CTransparentGraphNodeData::resize(UINT uWidth, UINT uHeight)
{
	m_uWidth = uWidth;
	m_uHeight = uHeight;

	// resize targets and swap chain
	
	mem_release(m_pRefractiveTextures[0]);
	mem_release(m_pRefractiveTextures[1]);

	m_pRefractiveTextures[0] = m_pDevice->createTexture2D(m_uWidth, m_uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A16B16G16R16F);
	m_pRefractiveTextures[1] = m_pDevice->createTexture2D(m_uWidth, m_uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A16B16G16R16F);
}
