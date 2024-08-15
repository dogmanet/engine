#include "GradientPreviewGraphNodeData.h"
#include <xcommon/IXRenderable.h>
#include <xcommon/IPluginManager.h>

CGradientPreviewGraphNodeData::CGradientPreviewGraphNodeData(IGXDevice *pDevice, IXRender *pRender, IXCore *pCore):
	m_pDevice(pDevice)
{
	pRender->getUtils()->newGizmoRenderer(&m_pRenderer);
}
CGradientPreviewGraphNodeData::~CGradientPreviewGraphNodeData()
{
	mem_release(m_pRenderer);
	mem_release(m_pSurface);
}

void XMETHODCALLTYPE CGradientPreviewGraphNodeData::resize(UINT uWidth, UINT uHeight)
{
	m_uWidth = uWidth;
	m_uHeight = uHeight;
	
	mem_release(m_pSurface);
	m_pSurface = m_pDevice->createColorTarget(m_uWidth, m_uHeight, GXFMT_A8B8G8R8_SRGB, GXMULTISAMPLE_4_SAMPLES);
}

void CGradientPreviewGraphNodeData::update(IX2ColorGradients *pGradient)
{
	m_pRenderer->reset();

	for(UINT y = 0; y < m_uHeight; y += 8)
	{
		for(UINT x = 0; x < m_uWidth; x += 8)
		{
			float fX = (float)x;
			float fX2 = (float)(x + 8);
			float fY = (float)y;
			float fY2 = (float)(y + 8);

			float3_t vPt0(fX, fY, 0.0f);
			float3_t vPt1(fX, fY2, 0.0f);
			float3_t vPt2(fX2, fY2, 0.0f);
			float3_t vPt3(fX2, fY, 0.0f);

			float4_t vColor = ((x + y) % 16) ? float4_t(1.0f, 1.0f, 1.0f, 1.0f) : float4_t(0.7f, 0.7f, 0.7f, 1.0f);

			m_pRenderer->drawPoly(vPt0, vPt1, vPt2, vColor, vColor, vColor);
			m_pRenderer->drawPoly(vPt0, vPt2, vPt3, vColor, vColor, vColor);
		}
	}

	if(pGradient->getMode() == X2CGM_GRADIENT)
	{
		drawArea(pGradient->getGradient0(), 0, m_uHeight);
	}
	else
	{
		UINT h = m_uHeight / 2;
		drawArea(pGradient->getGradient1(), 0, h);
		drawArea(pGradient->getGradient0(), m_uHeight - h, h);
	}
}

void CGradientPreviewGraphNodeData::drawArea(IXColorGradient *pGradient, UINT uStartY, UINT uHeight)
{
	m_pRenderer->setColor(float4(0.0f, 1.0f, 0.0f, 0.5f));

	UINT uMin = 0;
	UINT uMax = m_uWidth;
	float2_t vAreaSize((float)m_uWidth, (float)(uHeight + 1));
	float fStartY = (float)uStartY;

	float4_t vColorPrev;
	for(UINT x = uMin; x <= uMax; x += 1)
	{
		float fX = (float)x / vAreaSize.x;
		float4_t vColor = pGradient->evaluate(fX);

		if(x != uMin)
		{
			float3_t vPt0((float)(x - 1), fStartY, 0.0f);
			float3_t vPt1((float)(x - 1), fStartY + vAreaSize.y, 0.0f);
			float3_t vPt2((float)x, fStartY + vAreaSize.y, 0.0f);
			float3_t vPt3((float)x, fStartY, 0.0f);

			m_pRenderer->drawPoly(vPt0, vPt1, vPt2, vColorPrev, vColorPrev, vColor);
			m_pRenderer->drawPoly(vPt0, vPt2, vPt3, vColorPrev, vColor, vColor);
		}

		vColorPrev = vColor;
	}
}
