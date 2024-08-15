#ifndef __TONEMAPPINGGRAPHNODEDATA_H
#define __TONEMAPPINGGRAPHNODEDATA_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCamera.h>

class CRender;
class CTonemappingGraphNodeData final: public IXUnknownImplementation<IXRenderGraphNodeData>
{
	//friend class CTonemappingGraphNode;
	friend class CLightSystem;
public:
	CTonemappingGraphNodeData(IGXDevice *pDevice, IXRender *pRender);
	~CTonemappingGraphNodeData();
	
	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;

private:
	UINT m_uWidth = 0;
	UINT m_uHeight = 0;

	IGXDevice *m_pDevice = NULL;

	//###################################

	IGXTexture2D *m_pBloom[2];
	IGXTexture2D *m_pAdaptedLuminance[2];
	UINT m_uCurrAdaptedLuminanceTarget = 0;

	//###################################

	struct
	{
		float2_t vSizeMapInv;
		float fCoefBlur;
		UINT uWidth = 0;
	} m_gaussBlurShaderData;
	IGXConstantBuffer *m_pGaussBlurShaderData = NULL;
};

#endif
