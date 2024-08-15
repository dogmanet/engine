#include "BaseTarget.h"
#include "Render.h"
#include "RenderGraph.h"

CBaseTarget::CBaseTarget(CRender *pRender, IGXDevice *pDevice):
	m_pRender(pRender),
	m_pDevice(pDevice)
{
}
CBaseTarget::~CBaseTarget()
{
	mem_release(m_pScreenTextureRB);
	mem_release(m_pGraph);
	mem_release(m_pGraphData);
}


void XMETHODCALLTYPE CBaseTarget::resize(UINT uWidth, UINT uHeight)
{
	m_uWidth = uWidth;
	m_uHeight = uHeight;

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

void XMETHODCALLTYPE CBaseTarget::attachGraph(IXRenderGraph *pGraph)
{
	mem_release(m_pGraph);
	m_pGraph = (CRenderGraph*)pGraph;
	add_ref(m_pGraph);
	
	if(m_uWidth != 0 && m_uHeight != 0)
	{
		m_pGraph->newGraphData(this, &m_pGraphData);
	}
}

void XMETHODCALLTYPE CBaseTarget::setCamera(IXCamera *pCamera)
{
	mem_release(m_pCamera);
	add_ref(pCamera);
	m_pCamera = pCamera;
}
void XMETHODCALLTYPE CBaseTarget::getCamera(IXCamera **ppCamera)
{
	add_ref(m_pCamera);
	*ppCamera = m_pCamera;
}

void XMETHODCALLTYPE CBaseTarget::getSize(UINT *puWidth, UINT *puHeight) const
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

IGXRenderBuffer* CBaseTarget::getScreenRB() const
{
	return(m_pScreenTextureRB);
}

void CBaseTarget::render(float fDeltaTime)
{
	if(isEnabled() && m_pGraphData)
	{
		SAFE_CALL(m_pGraph, render, fDeltaTime, this, m_pGraphData);
	}
}

void CBaseTarget::updateVisibility()
{
	if(isEnabled() && m_pGraphData)
	{
		SAFE_CALL(m_pGraph, updateVisibility, this, m_pGraphData);
	}
}

CRenderGraph* CBaseTarget::getGraph()
{
	return(m_pGraph);
}

CRenderGraphData* CBaseTarget::getGraphData()
{
	return(m_pGraphData);
}

bool CBaseTarget::isEnabled()
{
	return(m_isEnabled);
}
