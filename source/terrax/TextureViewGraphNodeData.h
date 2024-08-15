#ifndef __TEXTUREVIEWGRAPHNODEDATA_H
#define __TEXTUREVIEWGRAPHNODEDATA_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCamera.h>
#include <mtrl/IXTexture.h>

// {6C878F36-1ACD-4DFB-818C-BB6B55C9B92A}
#define X_TEXTURE_VIEW_INTERNALS_GUID DEFINE_XGUID(0x6c878f36, 0x1acd, 0x4dfb, 0x81, 0x8c, 0xbb, 0x6b, 0x55, 0xc9, 0xb9, 0x2a)

class CRender;
class CTextureViewGraphNodeData final: public IXUnknownImplementation<IXRenderGraphNodeData>
{
	friend class CTextureViewGraphNode;
public:
	CTextureViewGraphNodeData(IGXDevice *pDevice, IXRender *pRender);
	~CTextureViewGraphNodeData();
	
	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;

	void setTexture(IXTexture *pTexture);

private:
	UINT m_uWidth = 0;
	UINT m_uHeight = 0;

	IGXDevice *m_pDevice = NULL;

	IXTexture *m_pTexture = NULL;
};

#endif
