#ifndef __TEXTURETARGET_H
#define __TEXTURETARGET_H

#include <xcommon/render/IXRender.h>
#include <mtrl/IXMaterialSystem.h>
#include "BaseTarget.h"

class CRender;
class CRenderGraph;
class CRenderGraphData;
class CTextureTarget final: public CBaseTarget
{
	DECLARE_CLASS(CTextureTarget, CBaseTarget);
public:
	CTextureTarget(CRender *pRender, IGXDevice *pDevice, IXMaterialSystem *pMaterialSystem);
	~CTextureTarget();

	bool XMETHODCALLTYPE getTarget(IGXSurface **ppTarget) override;

	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;

	void getTextureName(char *szTextureName, size_t sizeTextureNameBuffer);

private:
	void XMETHODCALLTYPE FinalRelease() override;

private:
	//UINT m_uWidth = 0;
	//UINT m_uHeight = 0;

	IXMaterialSystem *m_pMaterialSystem = NULL;

	//IXCamera *m_pCamera = NULL;

	//IGXRenderBuffer *m_pScreenTextureRB = NULL;

	//CRenderGraph *m_pGraph = NULL;
	//CRenderGraphData *m_pGraphData = NULL;

	IGXTexture2D *m_pTexture = NULL;
	IXTexture *m_pTex = NULL;

	XGUID m_guid;
};

#endif
