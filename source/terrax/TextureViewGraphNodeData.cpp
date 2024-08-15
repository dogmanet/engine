#include "TextureViewGraphNodeData.h"
#include <xcommon/IXRenderable.h>

CTextureViewGraphNodeData::CTextureViewGraphNodeData(IGXDevice *pDevice, IXRender *pRender):
	m_pDevice(pDevice)
{
}
CTextureViewGraphNodeData::~CTextureViewGraphNodeData()
{
	mem_release(m_pTexture);
}

void XMETHODCALLTYPE CTextureViewGraphNodeData::resize(UINT uWidth, UINT uHeight)
{
	m_uWidth = uWidth;
	m_uHeight = uHeight;
}

void CTextureViewGraphNodeData::setTexture(IXTexture *pTexture)
{
	mem_release(m_pTexture);
	m_pTexture = pTexture;
	add_ref(m_pTexture);
}
