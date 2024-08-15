#ifndef __FINALTARGET_H
#define __FINALTARGET_H

#include <xcommon/render/IXRender.h>
#include "BaseTarget.h"

// {6CFA187C-5714-4016-B105-E53D1700F0D9}
#define X_IS_FINAL_TARGET_GUID DEFINE_XGUID(0x6cfa187c, 0x5714, 0x4016, 0xb1, 0x5, 0xe5, 0x3d, 0x17, 0x0, 0xf0, 0xd9)

class CRender;
class CRenderGraph;
class CRenderGraphData;
class CFinalTarget final: public CBaseTarget
{
	DECLARE_CLASS(CFinalTarget, CBaseTarget);
public:
	CFinalTarget(CRender *pRender, IGXDevice *pDevice, SXWINDOW hWnd);
	~CFinalTarget();

	bool XMETHODCALLTYPE getTarget(IGXSurface **ppTarget) override;

	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;

	void swapChain();

	IGXSwapChain* getSwapChain();

	void XMETHODCALLTYPE getInternalData(const XGUID *pGUID, void **ppOut) override;

	bool isEnabled() override;

private:
	void XMETHODCALLTYPE FinalRelease() override;

private:
	//UINT m_uWidth = 0;
	//UINT m_uHeight = 0;

	SXWINDOW m_hWnd = NULL;

	//IXCamera *m_pCamera = NULL;

	IGXSwapChain *m_pSwapChain = NULL;
};

#endif
