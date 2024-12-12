#include "Tracer.h"

CTracer::CTracer(int iTracePoolSize):
	m_isTracing(false),
	m_iCurTrace(0),
	m_iPoolSize(iTracePoolSize),
	m_iLineCount(0)
{
	m_vpPoints = new Array<Point>[iTracePoolSize];
}

CTracer::~CTracer()
{
	mem_delete_a(m_vpPoints);
}

//! Начинает новый трейс из точки vPoint с цветом fFracColor (0-1)
void CTracer::begin(const float3 &vPoint, float fFracColor)
{
	if(m_isTracing)
	{
		end();
	}
	m_isTracing = true;

	if(m_iLineCount < m_iCurTrace + 1)
	{
		m_iLineCount = m_iCurTrace + 1;
	}

	m_vpPoints[m_iCurTrace].clearFast();

	putPoint(vPoint, fFracColor);
}

//! Заканчивает активный трейс
void CTracer::end()
{
	if(m_isTracing)
	{
		m_isTracing = false;

		++m_iCurTrace;
		m_iCurTrace %= m_iPoolSize;
	}
}

//! Очищает пул трейсов
void CTracer::clear()
{
	end();
	m_iCurTrace = 0;
	m_iLineCount = 0;

	for(int i = 0; i < m_iPoolSize; ++i)
	{
		m_vpPoints[i].clearFast();
	}
}

//! Проводит линию до указанной точки
void CTracer::lineTo(const float3 &vPoint, float fFracColor)
{
	putPoint(vPoint, fFracColor);
}

void CTracer::render()
{
	if(!m_iLineCount)
	{
		return;
	}

#ifdef _GRAPHIX_API
	SGCore_ShaderUnBind();

	SMMATRIX mView, mProj;
	Core_RMatrixGet(G_RI_MATRIX_VIEW, &mView);
	Core_RMatrixGet(G_RI_MATRIX_PROJECTION, &mProj);

	SGCore_GetDXDevice()->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&SMMatrixIdentity());
	SGCore_GetDXDevice()->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&mView);
	SGCore_GetDXDevice()->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&mProj);

	SGCore_GetDXDevice()->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	SGCore_GetDXDevice()->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	SGCore_GetDXDevice()->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

	SGCore_GetDXDevice()->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);

	DX_CALL(SGCore_GetDXDevice()->SetTexture(0, 0));

	// render m_iLineCount traces
	for(int i = 0; i < m_iLineCount; ++i)
	{
		DX_CALL(SGCore_GetDXDevice()->DrawPrimitiveUP(D3DPT_LINESTRIP, m_vpPoints[i].size() - 1, &(m_vpPoints[i][0]), sizeof(Point)));
	}
#endif
}

void CTracer::putPoint(const float3 &vPoint, float fFracColor)
{
	m_vpPoints[m_iCurTrace].push_back({vPoint, getColor(fFracColor)});
}

unsigned int CTracer::getColor(float fFrac)
{
	bool isSecond = fFrac > 0.5f;
	unsigned int iStartColor = isSecond ? 0xFFFFFF00 : 0xFFFF0000;
	unsigned int iEndColor = isSecond ? 0xFF00FF00 : 0xFFFFFF00;

	unsigned int iResult = 0;

	if(isSecond)
	{
		fFrac -= 0.5f;
	}
	fFrac *= 2.0f;

	iResult = (int)lerpf((float)((iStartColor >> 24) & 0xFF), (float)((iEndColor >> 24) & 0xFF), fFrac);
	iResult <<= 8;
	iResult |= (int)lerpf((float)((iStartColor >> 16) & 0xFF), (float)((iEndColor >> 16) & 0xFF), fFrac);
	iResult <<= 8;
	iResult |= (int)lerpf((float)((iStartColor >> 8) & 0xFF), (float)((iEndColor >> 8) & 0xFF), fFrac);
	iResult <<= 8;
	iResult |= (int)lerpf((float)(iStartColor & 0xFF), (float)(iEndColor & 0xFF), fFrac);

	return(iResult);
}
