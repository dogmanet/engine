#ifndef __BASETARGET_H
#define __BASETARGET_H

#include <xcommon/render/IXRender.h>

class CRender;
class CRenderGraph;
class CRenderGraphData;
class CBaseTarget: public IXUnknownImplementation<IXRenderTarget>
{
	DECLARE_CLASS(CBaseTarget, IXUnknownImplementation<IXRenderTarget>);
public:
	CBaseTarget(CRender *pRender, IGXDevice *pDevice);
	~CBaseTarget();

	void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) override;

	void XMETHODCALLTYPE attachGraph(IXRenderGraph *pGraph) override;

	void XMETHODCALLTYPE setCamera(IXCamera *pCamera) override;
	void XMETHODCALLTYPE getCamera(IXCamera **ppCamera) override;

	void XMETHODCALLTYPE getSize(UINT *puWidth, UINT *puHeight) const override;

	IGXRenderBuffer* getScreenRB() const;

	void render(float fDeltaTime);

	void updateVisibility();

	CRenderGraph* getGraph();

	CRenderGraphData* getGraphData();

	virtual bool isEnabled();

private:
	UINT m_uWidth = 0;
	UINT m_uHeight = 0;


	IXCamera *m_pCamera = NULL;

	IGXRenderBuffer *m_pScreenTextureRB = NULL;

	CRenderGraph *m_pGraph = NULL;
	CRenderGraphData *m_pGraphData = NULL;

	bool m_isEnabled = true;

protected:
	CRender *m_pRender;
	IGXDevice *m_pDevice = NULL;
};

#endif
