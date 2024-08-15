#ifndef __TRANSPARENTGRAPHNODEDATA_H
#define __TRANSPARENTGRAPHNODEDATA_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCamera.h>

class CRender;
class CTransparentGraphNodeData final: public IXUnknownImplementation<IXRenderGraphNodeData>
{
	friend class CTransparentGraphNode;
public:
	CTransparentGraphNodeData(IGXDevice *pDevice, IXRender *pRender);
	~CTransparentGraphNodeData();

	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;

private:
	UINT m_uWidth = 0;
	UINT m_uHeight = 0;

	IGXDevice *m_pDevice = NULL;

	IGXBaseTexture *m_pRefractiveTextures[2];
	int m_iRefractiveSource = -1;
	IGXBaseTexture *m_pRefractiveTextureRead = NULL;
	IGXBaseTexture *m_pRefractiveTextureWrite = NULL;
};

#endif
