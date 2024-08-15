#include "CurvePreviewGraphNodeData.h"
#include <xcommon/IXRenderable.h>
#include <xcommon/IPluginManager.h>

CCurvePreviewGraphNodeData::CCurvePreviewGraphNodeData(IGXDevice *pDevice, IXRender *pRender, IXCore *pCore):
	m_pDevice(pDevice)
{
	pRender->getUtils()->newGizmoRenderer(&m_pLineRenderer);

	IXFontManager *m_pFontManager = (IXFontManager*)pCore->getPluginManager()->getInterface(IXFONTMANAGER_GUID);
	if(m_pFontManager)
	{
		m_pFontManager->getFont(&m_pFont, "gui/fonts/tahoma.ttf", 10);
	}
}
CCurvePreviewGraphNodeData::~CCurvePreviewGraphNodeData()
{
	mem_release(m_pLineRenderer);
	mem_release(m_pFont);
	mem_release(m_pSurface);
}

void XMETHODCALLTYPE CCurvePreviewGraphNodeData::resize(UINT uWidth, UINT uHeight)
{
	m_uWidth = uWidth;
	m_uHeight = uHeight;
	
	mem_release(m_pSurface);
	m_pSurface = m_pDevice->createColorTarget(m_uWidth, m_uHeight, GXFMT_A8B8G8R8_SRGB, GXMULTISAMPLE_4_SAMPLES);

	const float c_fWindowPadding = 0.0f;
	const float2_t c_vWindowSize((float)uWidth, (float)uHeight);

	float2_t vMin(c_fWindowPadding, c_fWindowPadding);
	float2_t vMax = c_vWindowSize - float2_t(c_fWindowPadding, c_fWindowPadding);

	m_vAreaMin = vMin;
	m_vAreaMax = vMax;
}

void CCurvePreviewGraphNodeData::updateLine(IXMinMaxCurve *pCurve)
{
	m_pCurve = pCurve;

	m_pLineRenderer->reset();

	if(pCurve->getMode() == XMCM_TWO_CURVES)
	{
		drawArea(pCurve);
		updateLine(0, m_pCurve->getMinCurve());
	}
	
	updateLine(1, m_pCurve->getMaxCurve());
}
void CCurvePreviewGraphNodeData::updateLine(int iCurve, IXAnimationCurve *pCurve)
{
	m_pLineRenderer->setColor(float4(0.0f, 1.0f, 0.0f, 1.0f));

	m_pLineRenderer->setLineWidth(2.0f);
	m_pLineRenderer->setPointSize(3.0f);
	m_pLineRenderer->setPointMode(XGPM_ROUND);

	UINT uMin = (UINT)m_vAreaMin.x;
	UINT uMax = (UINT)m_vAreaMax.x;
	float2_t vAreaSize = m_vAreaMax - m_vAreaMin;
	for(UINT x = uMin; x <= uMax; x += 2)
	{
		float fX = ((float)x - m_vAreaMin.x) / vAreaSize.x;
		float fY = pCurve->evaluate(fX);

		float3_t vPt((float)x, fY * vAreaSize.y + m_vAreaMin.y, 0.0f);

		if(x == uMin)
		{
			m_pLineRenderer->jumpTo(vPt);
		}
		else
		{
			m_pLineRenderer->lineTo(vPt);
		}
	}
	
	/*for(UINT i = 0, l = pCurve->getKeyframeCount(); i < l; ++i)
	{
		m_pLineRenderer->setColor(float4(1.0f, 1.0f, 0.0f, 1.0f));

		const XKeyframe *pKeyFrame = pCurve->getKeyAt(i);
		float2 vPos = float2(pKeyFrame->fTime, pKeyFrame->fValue) * vAreaSize + m_vAreaMin;
		m_pLineRenderer->drawPoint(vPos);
	}*/
}

void CCurvePreviewGraphNodeData::drawArea(IXMinMaxCurve *pCurve)
{
	m_pLineRenderer->setColor(float4(0.0f, 1.0f, 0.0f, 0.5f));

	IXAnimationCurve *pMin = pCurve->getMinCurve();
	IXAnimationCurve *pMax = pCurve->getMaxCurve();

	UINT uMin = (UINT)m_vAreaMin.x;
	UINT uMax = (UINT)m_vAreaMax.x;
	float2_t vAreaSize = m_vAreaMax - m_vAreaMin;

	float fYMinPrev, fYMaxPrev;
	for(UINT x = uMin; x <= uMax; x += 2)
	{
		float fX = ((float)x - m_vAreaMin.x) / vAreaSize.x;
		float fYMin = pMin->evaluate(fX);
		float fYMax = pMax->evaluate(fX);

		if(fYMax < fYMin)
		{
			std::swap(fYMin, fYMax);
		}

		if(x != uMin)
		{
			float3_t vPt0((float)(x - 2), fYMinPrev * vAreaSize.y + m_vAreaMin.y, 0.0f);
			float3_t vPt1((float)(x - 2), fYMaxPrev * vAreaSize.y + m_vAreaMin.y, 0.0f);
			float3_t vPt2((float)x, fYMax * vAreaSize.y + m_vAreaMin.y, 0.0f);
			float3_t vPt3((float)x, fYMin * vAreaSize.y + m_vAreaMin.y, 0.0f);

			m_pLineRenderer->drawPoly(vPt0, vPt1, vPt2);
			m_pLineRenderer->drawPoly(vPt0, vPt2, vPt3);
		}

		fYMinPrev = fYMin;
		fYMaxPrev = fYMax;
	}
}
