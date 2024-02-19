#ifndef __FINALTARGET_H
#define __FINALTARGET_H

#include <xcommon/render/IXRender.h>

class CRender;
class CRenderGraph;
class CRenderGraphData;
class CFinalTarget final: public IXUnknownImplementation<IXRenderTarget>
{
public:
	CFinalTarget(CRender *pRender, IGXDevice *pDevice, SXWINDOW hWnd);
	~CFinalTarget();

	bool XMETHODCALLTYPE getTarget(IGXSurface **ppTarget) override;

	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;

	void XMETHODCALLTYPE attachGraph(IXRenderGraph *pGraph) override;

	void XMETHODCALLTYPE setCamera(IXCamera *pCamera) override;
	void XMETHODCALLTYPE getCamera(IXCamera **ppCamera) override;

	void XMETHODCALLTYPE getSize(UINT *puWidth, UINT *puHeight) const override;

	void swapChain();

	IGXRenderBuffer* getScreenRB() const;

	IGXSwapChain* getSwapChain();

	void render(float fDeltaTime);

	void updateVisibility();

	CRenderGraph* getGraph()
	{
		return(m_pGraph);
	}

	CRenderGraphData* getGraphData()
	{
		return(m_pGraphData);
	}

private:
	void XMETHODCALLTYPE FinalRelease() override;

private:
	UINT m_uWidth = 0;
	UINT m_uHeight = 0;

	CRender *m_pRender;
	IGXDevice *m_pDevice = NULL;
	SXWINDOW m_hWnd = NULL;

	IXCamera *m_pCamera = NULL;

	IGXSwapChain *m_pSwapChain = NULL;

	IGXRenderBuffer *m_pScreenTextureRB = NULL;

	CRenderGraph *m_pGraph = NULL;
	CRenderGraphData *m_pGraphData = NULL;
};

#endif
