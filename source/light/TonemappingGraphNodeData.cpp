#include "TonemappingGraphNodeData.h"
#include <xcommon/IXRenderable.h>

CTonemappingGraphNodeData::CTonemappingGraphNodeData(IGXDevice *pDevice, IXRender *pRender):
	m_pDevice(pDevice)
{
	m_pAdaptedLuminance[0] = m_pDevice->createTexture2D(1, 1, 1, GX_TEXFLAG_RENDERTARGET | GX_TEXFLAG_UNORDERED_ACCESS, GXFMT_R16F);
	m_pAdaptedLuminance[1] = m_pDevice->createTexture2D(1, 1, 1, GX_TEXFLAG_RENDERTARGET | GX_TEXFLAG_UNORDERED_ACCESS, GXFMT_R16F);

	m_pGaussBlurShaderData = m_pDevice->createConstantBuffer(sizeof(m_gaussBlurShaderData));

	m_pBloom[0] = 0;
	m_pBloom[1] = 0;
}
CTonemappingGraphNodeData::~CTonemappingGraphNodeData()
{
	mem_release(m_pBloom[0]);
	mem_release(m_pBloom[1]);

	mem_release(m_pGaussBlurShaderData);

	mem_release(m_pAdaptedLuminance[0]);
	mem_release(m_pAdaptedLuminance[1]);
}


void XMETHODCALLTYPE CTonemappingGraphNodeData::resize(UINT uWidth, UINT uHeight)
{
	m_uWidth = uWidth;
	m_uHeight = uHeight;

	mem_release(m_pBloom[0]);
	mem_release(m_pBloom[1]);

	// resize targets and swap chain
	
	m_pBloom[0] = m_pDevice->createTexture2D(m_uWidth / 2, m_uHeight / 2, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A16B16G16R16F); // GXFMT_A8B8G8R8
	m_pBloom[1] = m_pDevice->createTexture2D(m_uWidth / 2, m_uHeight / 2, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A16B16G16R16F); // GXFMT_A8B8G8R8

	m_gaussBlurShaderData.uWidth = m_uWidth / 2;
	m_gaussBlurShaderData.vSizeMapInv.x = 1.0f / (float)(m_uWidth / 2);
	m_gaussBlurShaderData.vSizeMapInv.y = 1.0f / (float)(m_uHeight / 2);
	m_gaussBlurShaderData.fCoefBlur = 1.0f;
	m_pGaussBlurShaderData->update(&m_gaussBlurShaderData);
}
