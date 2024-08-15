#include "GIGraphNodeData.h"
#include <xcommon/IXRenderable.h>

CGIGraphNodeData::CGIGraphNodeData(IGXDevice *pDevice, IXRender *pRender):
	m_pDevice(pDevice)
{
}
CGIGraphNodeData::~CGIGraphNodeData()
{
	mem_release(m_pLightAmbientDiffuse);

	mem_release(m_pGBufferColor);
	mem_release(m_pGBufferNormals);
	mem_release(m_pGBufferParams);
	mem_release(m_pGBufferDepth);

	mem_release(m_pSSAOTexture);
	mem_release(m_pSSAOTextureBlur);
}

void XMETHODCALLTYPE CGIGraphNodeData::resize(UINT uWidth, UINT uHeight)
{
	m_uWidth = uWidth;
	m_uHeight = uHeight;

	// resize targets and swap chain
	mem_release(m_pLightAmbientDiffuse);
	mem_release(m_pSSAOTexture);
	mem_release(m_pSSAOTextureBlur);

	m_pLightAmbientDiffuse = m_pDevice->createTexture2D(m_uWidth, m_uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A16B16G16R16F);

	m_pSSAOTexture = m_pDevice->createTexture2D(m_uWidth, m_uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_R16F);
	m_pSSAOTextureBlur = m_pDevice->createTexture2D(m_uWidth, m_uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_R16F);
}

void CGIGraphNodeData::setGBuffer(IGXBaseTexture *pColor, IGXBaseTexture *pNormals, IGXBaseTexture *pParams, IGXBaseTexture *pDepth)
{
	mem_release(m_pGBufferColor);
	mem_release(m_pGBufferNormals);
	mem_release(m_pGBufferParams);
	mem_release(m_pGBufferDepth);

	add_ref(pColor);
	add_ref(pNormals);
	add_ref(pParams);
	add_ref(pDepth);

	m_pGBufferColor = pColor;
	m_pGBufferNormals = pNormals;
	m_pGBufferParams = pParams;
	m_pGBufferDepth = pDepth;
}
