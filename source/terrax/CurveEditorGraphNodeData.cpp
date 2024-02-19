#include "CurveEditorGraphNodeData.h"
#include <xcommon/IXRenderable.h>
#include <xcommon/IPluginManager.h>

CCurveEditorGraphNodeData::CCurveEditorGraphNodeData(IGXDevice *pDevice, IXRender *pRender, IXCore *pCore):
	m_pDevice(pDevice)
{
	pRender->getUtils()->newGizmoRenderer(&m_pRenderer);
	pRender->getUtils()->newGizmoRenderer(&m_pLineRenderer);

	IXFontManager *m_pFontManager = (IXFontManager*)pCore->getPluginManager()->getInterface(IXFONTMANAGER_GUID);
	if(m_pFontManager)
	{
		m_pFontManager->getFont(&m_pFont, "gui/fonts/tahoma.ttf", 10);
	}
}
CCurveEditorGraphNodeData::~CCurveEditorGraphNodeData()
{
	mem_release(m_pLineRenderer);
	mem_release(m_pFont);
	mem_release(m_pRenderer);
	mem_release(m_pSurface);
}

void XMETHODCALLTYPE CCurveEditorGraphNodeData::resize(UINT uWidth, UINT uHeight)
{
	m_uWidth = uWidth;
	m_uHeight = uHeight;
	
	mem_release(m_pSurface);
	m_pSurface = m_pDevice->createColorTarget(m_uWidth, m_uHeight, GXFMT_A8B8G8R8_SRGB, GXMULTISAMPLE_4_SAMPLES);

	const float c_fWindowPadding = 5.0f;
	const float2_t c_vWindowSize((float)uWidth, (float)uHeight);

	m_pRenderer->reset();

	m_pRenderer->setLineWidth(1.5f);

	float2_t vMin(c_fWindowPadding, c_fWindowPadding);
	float2_t vMax = c_vWindowSize - float2_t(c_fWindowPadding, c_fWindowPadding);

	m_pRenderer->setPointSize(5.0f);

	//m_pRenderer->drawPoly(float3(vMin), float3(vMin.x, vMax.y, 0.0f), float3(vMax));
	//m_pRenderer->drawPoly(float3(vMin), float3(vMax), float3(vMax.x, vMin.y, 0.0f));

	//m_pRenderer->drawPoint(float3(vMin));
	//m_pRenderer->drawPoint(float3(vMin.x, vMax.y, 0.0f));
	//m_pRenderer->drawPoint(float3(vMax));
	//m_pRenderer->drawPoint(float3(vMax.x, vMin.y, 0.0f));

	float2_t vOx(10.0f, 0.0f);
	float2_t vOy(0.0f, 10.0f);

	// outer frame
	{
		m_pRenderer->setColor(GX_COLOR_COLOR_TO_F4(0xccffbb47));
		float2_t aPoints[] = {
			vMin + vOx,
			float2_t(vMax.x, vMin.y) - vOx,
			float2_t(vMax.x, vMin.y) + vOy,
			//
			float2_t(vMax.x, vMin.y + (vMax.y - vMin.y) * 0.3f),
			float2_t(vMax.x, vMin.y + (vMax.y - vMin.y) * 0.3f) + (-vOx + vOy) * 0.5f,
			float2_t(vMax.x, vMax.y - (vMax.y - vMin.y) * 0.3f) + (-vOx - vOy) * 0.5f,
			float2_t(vMax.x, vMax.y - (vMax.y - vMin.y) * 0.3f),
			//
			vMax - vOy,
			vMax - vOx,
			//
			float2_t(vMin.x, vMax.y) + vOx,
			float2_t(vMin.x, vMax.y) - vOy,
			vMin + vOy
		};

		m_pRenderer->jumpTo(float3(aPoints[ARRAYSIZE(aPoints) - 1], 0.0f));
		for(UINT i = 0, l = ARRAYSIZE(aPoints); i < l; ++i)
		{
			m_pRenderer->lineTo(float3(aPoints[i], 0.0f));
		}
	}

	float2_t vAreaMin = vMin + float2_t(25.0f, 20.0f);
	float2_t vAreaMax = vMax - vOx - vOy;

	m_vAreaMin = vAreaMin;
	m_vAreaMax = vAreaMax;

	// inner frame
	{
		//m_pRenderer->setColor(GX_COLOR_COLOR_TO_F4(0x90c6f0fd));
		m_pRenderer->setColor(GX_COLOR_COLOR_TO_F4(0xffc6f0fd));
		
		float2_t aPoints[] = {
			vAreaMin,
			float2_t(vAreaMin.x, vAreaMax.y),
			vAreaMax,
			float2_t(vAreaMax.x, vAreaMin.y)
		};

		m_pRenderer->jumpTo(float3(aPoints[ARRAYSIZE(aPoints) - 1], 0.0f));
		for(UINT i = 0, l = ARRAYSIZE(aPoints); i < l; ++i)
		{
			m_pRenderer->lineTo(float3(aPoints[i], 0.0f));
		}
	}

	UINT uGridSegsX = 20;
	UINT uGridSegsY = 10;
	float4_t vColor = GX_COLOR_COLOR_TO_F4(0x90c6f0fd);
	m_pRenderer->setLineWidth(1.0f);

	m_pRenderer->setFont(m_pFont);

	XGizmoFontParams fp;
	fp.verticalAlign = XGTVA_MIDDLE;
	fp.textAlign = XFONT_TEXT_ALIGN_RIGHT;
	m_pRenderer->setFontParams(fp);

	//m_pRenderer->drawString("Test\nnewline", float3((vAreaMin + vAreaMax) * 0.5f, 0.0f));

	char tmp[32];

	for(UINT i = 1; i < uGridSegsY; ++i)
	{
		float fVal = ((float)i / (float)uGridSegsY);
		float fY = roundf((vAreaMax.y - vAreaMin.y) * fVal + vAreaMin.y - 0.5f) + 0.5f;

		vColor.w = i == (uGridSegsY / 2) ? 0.5f : 0.1;
		m_pRenderer->setColor(vColor);

		m_pRenderer->jumpTo(float3(vAreaMin.x, fY, 0.0f));
		m_pRenderer->lineTo(float3(vAreaMax.x, fY, 0.0f));

		sprintf(tmp, "%g", fVal);
		m_pRenderer->drawString(tmp, float3(vAreaMin.x - 3.0f, fY, 0.0f));
	}
	vColor.w = 1.0f;
	m_pRenderer->setColor(vColor);
	m_pRenderer->drawString("0.0", float3(vAreaMin.x - 3.0f, vAreaMin.y, 0.0f));
	m_pRenderer->drawString("1.0", float3(vAreaMin.x - 3.0f, vAreaMax.y, 0.0f));


	fp.verticalAlign = XGTVA_TOP;
	fp.textAlign = XFONT_TEXT_ALIGN_CENTER;
	m_pRenderer->setFontParams(fp);

	for(UINT i = 1; i < uGridSegsX; ++i)
	{
		float fVal = ((float)i / (float)uGridSegsX);
		float fX = roundf((vAreaMax.x - vAreaMin.x) * fVal + vAreaMin.x - 0.5f) + 0.5f;

		vColor.w = i == (uGridSegsX / 2) ? 0.5f :
			(i % 2 == 1 ? 0.1 : 0.3);
		m_pRenderer->setColor(vColor);

		m_pRenderer->jumpTo(float3(fX, vAreaMin.y, 0.0f));
		m_pRenderer->lineTo(float3(fX, vAreaMax.y, 0.0f));

		sprintf(tmp, "%g", fVal);
		m_pRenderer->drawString(tmp, float3(fX, vAreaMin.y - 1.0f, 0.0f));
	}
	vColor.w = 1.0f;
	m_pRenderer->setColor(vColor);
	m_pRenderer->drawString("0.0", float3(vAreaMin.x, vAreaMin.y - 1.0f, 0.0f));
	m_pRenderer->drawString("1.0", float3(vAreaMax.x, vAreaMin.y - 1.0f, 0.0f));
}

void CCurveEditorGraphNodeData::updateLine(IXMinMaxCurve *pCurve)
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
void CCurveEditorGraphNodeData::updateLine(int iCurve, IXAnimationCurve *pCurve)
{
	if(m_iHighlightCurve == iCurve)
	{
		m_pLineRenderer->setColor(float4(0.5f, 1.0f, 0.5f, 1.0f));
	}
	else
	{
		m_pLineRenderer->setColor(float4(0.0f, 1.0f, 0.0f, 1.0f));
	}
	m_pLineRenderer->setLineWidth(2.0f);
	m_pLineRenderer->setPointSize(5.0f);
	m_pLineRenderer->setPointMode(XGPM_ROUND);

	UINT uMin = (UINT)m_vAreaMin.x;
	UINT uMax = (UINT)m_vAreaMax.x;
	float2_t vAreaSize = m_vAreaMax - m_vAreaMin;
	for(UINT x = uMin; x <= uMax; ++x)
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
	
	for(UINT i = 0, l = pCurve->getKeyframeCount(); i < l; ++i)
	{
		if((m_iHighlightCurve == iCurve && m_iHighlightKey == i) || (m_iSelectedCurve == iCurve && m_iSelectedKey == i))
		{
			m_pLineRenderer->setColor(float4(1.0f, 0.0f, 0.0f, 1.0f));
		}
		else
		{
			m_pLineRenderer->setColor(float4(1.0f, 1.0f, 0.0f, 1.0f));
		}

		const XKeyframe *pKeyFrame = pCurve->getKeyAt(i);
		float2 vPos = float2(pKeyFrame->fTime, pKeyFrame->fValue) * vAreaSize + m_vAreaMin;
		m_pLineRenderer->drawPoint(vPos);

		if(m_iSelectedCurve == iCurve && m_iSelectedKey == i)
		{
			float2_t vInPos, vOutPos;
			if(getSelectedPointTangents(&vInPos, &vOutPos))
			{
				vInPos.y = (float)m_uHeight - vInPos.y;
				vOutPos.y = (float)m_uHeight - vOutPos.y;
				m_pLineRenderer->setColor(float4(1.0f, 0.7f, 0.0f, 1.0f));
				m_pLineRenderer->drawPoint(float3(vInPos));
				m_pLineRenderer->drawPoint(float3(vOutPos));

				m_pLineRenderer->jumpTo(float3(vInPos));
				m_pLineRenderer->lineTo(vPos);
				m_pLineRenderer->lineTo(float3(vOutPos));
			}
		}
	}
}

void CCurveEditorGraphNodeData::setHighlight(int iCurve, int iKeyFrame)
{
	m_iHighlightCurve = iCurve;
	m_iHighlightKey = iKeyFrame;
	m_isDirty = true;

	updateLine(m_pCurve);
}

void CCurveEditorGraphNodeData::setSelect(int iCurve, int iKeyFrame)
{
	m_iSelectedCurve = iCurve;
	m_iSelectedKey = iKeyFrame;
	m_isDirty = true;

	updateLine(m_pCurve);
}

bool CCurveEditorGraphNodeData::screenToGraph(int iX, int iY, float *pfX, float *pfY)
{
	//m_pLineRenderer->drawPoint(float2((float)iX, (float)(m_uHeight - iY)));

	float2_t vPos = (float2_t((float)iX, (float)(m_uHeight - iY)) - m_vAreaMin) / (m_vAreaMax - m_vAreaMin);
	*pfX = clampf(vPos.x, 0.0f, 1.0f);
	*pfY = clampf(vPos.y, 0.0f, 1.0f);
	
	return(vPos.x >= 0.0f && vPos.x <= 1.0f && vPos.y >= 0.0f && vPos.y <= 1.0f);
}

void CCurveEditorGraphNodeData::getScale(float *pfX, float *pfY)
{
	*pfX = 1.0f / (m_vAreaMax.x - m_vAreaMin.x);
	*pfY = 1.0f / (m_vAreaMax.x - m_vAreaMin.x);
}

bool CCurveEditorGraphNodeData::getSelectedPointTangents(float2_t *pvInPos, float2_t *pvOutPos)
{
	if(m_iSelectedCurve < 0 || m_iSelectedKey < 0)
	{
		return(false);
	}

	IXAnimationCurve *pCurve = m_iSelectedCurve == 0 ? m_pCurve->getMinCurve() : m_pCurve->getMaxCurve();
	
	const XKeyframe *pKeyFrame = pCurve->getKeyAt(m_iSelectedKey);
	if(!pKeyFrame)
	{
		return(false);
	}

	float fMaxRadius = 50.0f; // px

	float2_t vOffset;

	float2_t vAreaSize = m_vAreaMax - m_vAreaMin;

	float2 vPos = float2(pKeyFrame->fTime, pKeyFrame->fValue) * vAreaSize + m_vAreaMin;

	float fInWeight = (pKeyFrame->weightedMode & XKWM_IN) ? pKeyFrame->fInWeight : 1.0f / 3.0f;
	if(!std::isfinite(pKeyFrame->fInTangent))
	{
		vOffset.x = 0.0f;
		vOffset.y = fMaxRadius * fInWeight;
	}
	else
	{
		vOffset.x = sqrtf((fMaxRadius * fInWeight) * (fMaxRadius * fInWeight) / (1.0f + pKeyFrame->fInTangent * pKeyFrame->fInTangent));
		vOffset.y = vOffset.x * pKeyFrame->fInTangent;
	}

	*pvInPos = vPos - vOffset;

	pvInPos->y = (float)m_uHeight - pvInPos->y;

	float fOutWeight = (pKeyFrame->weightedMode & XKWM_OUT) ? pKeyFrame->fOutWeight : 1.0f / 3.0f;
	if(!std::isfinite(pKeyFrame->fOutTangent))
	{
		vOffset.x = 0.0f;
		vOffset.y = fMaxRadius * fOutWeight;
	}
	else
	{
		vOffset.x = sqrtf(((fMaxRadius * fOutWeight) * (fMaxRadius * fOutWeight)) / (1.0f + pKeyFrame->fOutTangent * pKeyFrame->fOutTangent));
		vOffset.y = vOffset.x * pKeyFrame->fOutTangent;
	}

	*pvOutPos = vPos + vOffset;
	pvOutPos->y = (float)m_uHeight - pvOutPos->y;

	return(true);
}

void CCurveEditorGraphNodeData::calcNewTangent(int iX, int iY, bool isIn)
{
	if(m_iSelectedCurve < 0 || m_iSelectedKey < 0)
	{
		return;
	}

	IXAnimationCurve *pCurve = m_iSelectedCurve == 0 ? m_pCurve->getMinCurve() : m_pCurve->getMaxCurve();

	const XKeyframe *pKeyFrame = pCurve->getKeyAt(m_iSelectedKey);
	if(!pKeyFrame)
	{
		return;
	}

	float fMaxRadius = 50.0f; // px

	iY = m_uHeight - iY;

	float2_t vAreaSize = m_vAreaMax - m_vAreaMin;
	float2 vPos = float2(pKeyFrame->fTime, pKeyFrame->fValue) * vAreaSize + m_vAreaMin;

	XKeyframe kf = *pKeyFrame;

	if(isIn)
	{
		float2_t vOffset = vPos - float2_t(iX, iY);
		
		if(vOffset.x <= 0.0f)
		{
			kf.fInTangent = INFINITY;
			kf.fInWeight = 1.0f;
		}
		else
		{
			kf.fInWeight = clampf(SMVector2Length(vOffset) / fMaxRadius, 0.0f, 1.0f);
			kf.fInTangent = vOffset.y / vOffset.x;
		}

		kf.weightedMode |= XKWM_IN;
	}
	else
	{
		float2_t vOffset = float2_t(iX, iY) - vPos;

		if(vOffset.x <= 0.0f)
		{
			kf.fOutTangent = INFINITY;
			kf.fOutWeight = 1.0f;
		}
		else
		{
			kf.fOutWeight = clampf(SMVector2Length(vOffset) / fMaxRadius, 0.0f, 1.0f);
			kf.fOutTangent = vOffset.y / vOffset.x;
		}

		kf.weightedMode |= XKWM_OUT;
	}

	pCurve->setKey(m_iSelectedKey, kf);

	m_isDirty = true;
	updateLine(m_pCurve);
}

void CCurveEditorGraphNodeData::drawArea(IXMinMaxCurve *pCurve)
{
	m_pLineRenderer->setColor(float4(0.0f, 1.0f, 0.0f, 0.2f));

	IXAnimationCurve *pMin = pCurve->getMinCurve();
	IXAnimationCurve *pMax = pCurve->getMaxCurve();

	UINT uMin = (UINT)m_vAreaMin.x;
	UINT uMax = (UINT)m_vAreaMax.x;
	float2_t vAreaSize = m_vAreaMax - m_vAreaMin;

	float fYMinPrev, fYMaxPrev;
	for(UINT x = uMin; x <= uMax; ++x)
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
			float3_t vPt0((float)(x - 1), fYMinPrev * vAreaSize.y + m_vAreaMin.y, 0.0f);
			float3_t vPt1((float)(x - 1), fYMaxPrev * vAreaSize.y + m_vAreaMin.y, 0.0f);
			float3_t vPt2((float)x, fYMax * vAreaSize.y + m_vAreaMin.y, 0.0f);
			float3_t vPt3((float)x, fYMin * vAreaSize.y + m_vAreaMin.y, 0.0f);

			m_pLineRenderer->drawPoly(vPt0, vPt1, vPt2);
			m_pLineRenderer->drawPoly(vPt0, vPt2, vPt3);
		}

		fYMinPrev = fYMin;
		fYMaxPrev = fYMax;
	}
}
