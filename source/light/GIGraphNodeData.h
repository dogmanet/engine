#ifndef __GIGRAPHNODEDATA_H
#define __GIGRAPHNODEDATA_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCamera.h>

class CRender;
class CGIGraphNodeData final: public IXUnknownImplementation<IXRenderGraphNodeData>
{
	friend class CGIGraphNode;
	friend class CLightSystem;
public:
	CGIGraphNodeData(IGXDevice *pDevice, IXRender *pRender);
	~CGIGraphNodeData();
	
	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;

	void setGBuffer(IGXBaseTexture *pColor, IGXBaseTexture *pNormals, IGXBaseTexture *pParams, IGXBaseTexture *pDepth);

private:
	UINT m_uWidth = 0;
	UINT m_uHeight = 0;

	IGXDevice *m_pDevice = NULL;

	//! Буфер освещения (GXFMT_A16B16G16R16F, full size)
	IGXTexture2D *m_pLightAmbientDiffuse = NULL;

	//###################################

	//! G-Buffer
	IGXBaseTexture *m_pGBufferColor = NULL;
	IGXBaseTexture *m_pGBufferNormals = NULL;
	IGXBaseTexture *m_pGBufferParams = NULL;
	IGXBaseTexture *m_pGBufferDepth = NULL;

	//###################################

	IGXTexture2D *m_pSSAOTexture = NULL;
	IGXTexture2D *m_pSSAOTextureBlur = NULL;
};

#endif
