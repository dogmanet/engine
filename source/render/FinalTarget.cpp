#include "FinalTarget.h"
#include "Render.h"
#include "RenderGraph.h"

CFinalTarget::CFinalTarget(CRender *pRender, IGXDevice *pDevice, SXWINDOW hWnd):
	m_pRender(pRender),
	m_pDevice(pDevice),
	m_hWnd(hWnd)
{
}
CFinalTarget::~CFinalTarget()
{
	mem_release(m_pSwapChain);
	mem_release(m_pScreenTextureRB);
	mem_release(m_pGraph);
	mem_release(m_pGraphData);
}

bool XMETHODCALLTYPE CFinalTarget::getTarget(IGXSurface **ppTarget)
{
	if(m_pSwapChain)
	{
		*ppTarget = m_pSwapChain->getColorTarget();
		return(true);
	}

	return(false);
}

void XMETHODCALLTYPE CFinalTarget::resize(UINT uWidth, UINT uHeight)
{
	m_uWidth = uWidth;
	m_uHeight = uHeight;

	// resize targets and swap chain
	mem_release(m_pSwapChain);
	m_pSwapChain = m_pDevice->createSwapChain(uWidth, uHeight, m_hWnd);

	{
		mem_release(m_pScreenTextureRB);

		struct VERTEX_SCREEN_TEXTURE
		{
			float x, y, z, tx, ty, tz;
		};

		const float fOffsetPixelX = 1.0f / (float)m_uWidth;
		const float fOffsetPixelY = 1.0f / (float)m_uHeight;

		VERTEX_SCREEN_TEXTURE aVertices[] =
		{
			{-1.0f - fOffsetPixelX, -1.0f + fOffsetPixelY, 1.0f, 0.0f, 1.0f, 0},
			{-1.0f - fOffsetPixelX, 1.0f + fOffsetPixelY, 1.0f, 0.0f, 0.0f, 1},
			{1.0f - fOffsetPixelX, 1.0f + fOffsetPixelY, 1.0f, 1.0f, 0.0f, 2},

			{-1.0f - fOffsetPixelX, -1.0f + fOffsetPixelY, 1.0f, 0.0f, 1.0f, 0},
			{1.0f - fOffsetPixelX, 1.0f + fOffsetPixelY, 1.0f, 1.0f, 0.0f, 2},
			{1.0f - fOffsetPixelX, -1.0f + fOffsetPixelY, 1.0f, 1.0f, 1.0f, 3},
		};


		IGXVertexBuffer *pVB = m_pDevice->createVertexBuffer(sizeof(VERTEX_SCREEN_TEXTURE) * 6, GXBUFFER_USAGE_STATIC, aVertices);
		m_pScreenTextureRB = m_pDevice->createRenderBuffer(1, &pVB, m_pRender->getScreenQuadVertexDeclaration());
		mem_release(pVB);
	}

	if(m_pGraph)
	{
		mem_release(m_pGraphData);
		m_pGraph->newGraphData(this, &m_pGraphData);
	}
}

void XMETHODCALLTYPE CFinalTarget::attachGraph(IXRenderGraph *pGraph)
{
	mem_release(m_pGraph);
	m_pGraph = (CRenderGraph*)pGraph;
	add_ref(m_pGraph);
	
	if(m_uWidth != 0 && m_uHeight != 0)
	{
		m_pGraph->newGraphData(this, &m_pGraphData);
	}
}

void XMETHODCALLTYPE CFinalTarget::FinalRelease()
{
	m_pRender->onFinalTargetReleased(this);
}

void XMETHODCALLTYPE CFinalTarget::setCamera(IXCamera *pCamera)
{
	mem_release(m_pCamera);
	add_ref(pCamera);
	m_pCamera = pCamera;
}
void XMETHODCALLTYPE CFinalTarget::getCamera(IXCamera **ppCamera)
{
	add_ref(m_pCamera);
	*ppCamera = m_pCamera;
}

void XMETHODCALLTYPE CFinalTarget::getSize(UINT *puWidth, UINT *puHeight) const
{
	if(puWidth)
	{
		*puWidth = m_uWidth;
	}

	if(puHeight)
	{
		*puHeight = m_uHeight;
	}
}

void CFinalTarget::swapChain()
{
	SAFE_CALL(m_pSwapChain, swapBuffers);
}

IGXRenderBuffer* CFinalTarget::getScreenRB() const
{
	return(m_pScreenTextureRB);
}

IGXSwapChain* CFinalTarget::getSwapChain()
{
	return(m_pSwapChain);
}

void CFinalTarget::render(float fDeltaTime)
{
	SAFE_CALL(m_pGraph, render, fDeltaTime, this, m_pGraphData);
}

void CFinalTarget::updateVisibility()
{
	SAFE_CALL(m_pGraph, updateVisibility, this, m_pGraphData);
}
