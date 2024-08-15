#include "ScrollBar.h"

#include "terrax.h"

CScrollBar::CScrollBar(IXRender *pRender, IScrollEventListener *pEventListener):
	m_pRender(pRender),
	m_pDev(pRender->getDevice()),
	m_pEventListener(pEventListener)
{
	initHelpers();
}

CScrollBar::~CScrollBar()
{
	mem_release(m_pFrameIB);
	mem_release(m_pFrameRB);

	mem_release(m_pInformCB);
}

void CScrollBar::render()
{
	IGXContext *pCtx = m_pDev->getThreadContext();

	pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);
	pCtx->setRasterizerState(NULL);
	pCtx->setBlendState(g_xRenderStates.pBlendAlpha);

	m_pRender->bindShader(pCtx, m_idFrameShader);
	/*
	float vSizeDiff = (float)(frameSize - 128);

	float fTitleMax = vSizeDiff + 39.0f;

	m_frameState.vSize = float2_t(vSizeDiff, -vSizeDiff);
	m_frameState.fTitleSize = (float)uTitleLen - 60.0f;
	if(m_frameState.fTitleSize > fTitleMax)
	{
		m_frameState.fTitleSize = fTitleMax;
	}

	m_frameState.fHighlight = fSelection;
	m_frameState.vPosition = float2_t((float)iXpos, (float)-iYpos + (float)m_iScrollPos);

	*/
	if(m_isDirty)
	{
		m_pInformCB->update(&m_frameState);
	}
	pCtx->setVSConstant(m_pInformCB, 6);

	pCtx->setIndexBuffer(m_pFrameIB);
	pCtx->setRenderBuffer(m_pFrameRB);
	pCtx->drawIndexed(m_uFrameVC, m_uFramePC);

	m_pRender->unbindShader(pCtx);
}

void CScrollBar::initHelpers()
{
	struct FrameVertex
	{
		float2_t vPos;
		float2_t vSizeFactor; // x: bar, y: handler
		float4_t vMarker; // x: top, y: bottom, z: handler, w: line
	};

	FrameVertex aVertices[] = {
		// object button_top
		{float2_t(9.0016f,   -10.7656f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(5.5417f,   -12.7089f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(5.5587f,   -10.3548f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(9.0037f,    -8.3985f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(12.4610f,  -10.3541f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(12.4495f,  -12.7073f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(9.0016f,    -6.7466f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(5.5417f,    -8.6899f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(5.5587f,    -6.3358f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(9.0037f,    -4.3795f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(12.4610f,   -6.3351f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(12.4495f,   -8.6883f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(2.0197f,    -6.3273f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(2.0199f,   -14.2784f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(0.0000f,   -15.5885f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(18.0000f,   -5.1960f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(18.0000f,  -15.5885f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(15.9814f,  -14.2784f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(0.0000f,    -5.1962f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(15.9624f,   -6.3273f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(8.9980f,    -2.3671f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},
		{float2_t(9.0000f,        0.0f), float2_t(0.0f, 0.0f), float4_t(1.0f, 0.0f, 0.0f, 0.0f)},

		// object line
		{float2_t(10.5387f,  -97.3515f), float2_t(1.0f, 0.0f), float4_t(0.0f, 0.0f, 0.0f, 1.0f)},
		{float2_t(10.5387f,  -99.1307f), float2_t(1.0f, 0.0f), float4_t(0.0f, 0.0f, 0.0f, 1.0f)},
		{float2_t(8.9978f,  -100.0203f), float2_t(1.0f, 0.0f), float4_t(0.0f, 0.0f, 0.0f, 1.0f)},
		{float2_t(7.4570f,   -99.1308f), float2_t(1.0f, 0.0f), float4_t(0.0f, 0.0f, 0.0f, 1.0f)},
		{float2_t(7.4570f,   -97.3515f), float2_t(1.0f, 0.0f), float4_t(0.0f, 0.0f, 0.0f, 1.0f)},
		{float2_t(8.1397f,   -18.7041f), float2_t(0.0f, 0.0f), float4_t(0.0f, 0.0f, 0.0f, 1.0f)},
		{float2_t(7.4571f,   -18.2722f), float2_t(0.0f, 0.0f), float4_t(0.0f, 0.0f, 0.0f, 1.0f)},
		{float2_t(7.4571f,   -16.4930f), float2_t(0.0f, 0.0f), float4_t(0.0f, 0.0f, 0.0f, 1.0f)},
		{float2_t(8.9979f,   -15.6034f), float2_t(0.0f, 0.0f), float4_t(0.0f, 0.0f, 0.0f, 1.0f)},
		{float2_t(10.5388f,  -16.4930f), float2_t(0.0f, 0.0f), float4_t(0.0f, 0.0f, 0.0f, 1.0f)},
		{float2_t(10.5388f,  -18.2722f), float2_t(0.0f, 0.0f), float4_t(0.0f, 0.0f, 0.0f, 1.0f)},
		{float2_t(9.8560f,   -18.7041f), float2_t(0.0f, 0.0f), float4_t(0.0f, 0.0f, 0.0f, 1.0f)},
		{float2_t(9.8561f,   -96.9197f), float2_t(1.0f, 0.0f), float4_t(0.0f, 0.0f, 0.0f, 1.0f)},
		{float2_t(8.1398f,   -96.9197f), float2_t(1.0f, 0.0f), float4_t(0.0f, 0.0f, 0.0f, 1.0f)},

		// object button_bottom
		{float2_t(8.9984f,  -104.8256f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(12.4583f, -102.8823f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(12.4413f, -105.2364f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(8.9963f,  -107.1927f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(5.5390f,  -105.2371f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(5.5505f,  -102.8839f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(8.9984f,  -108.8446f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(12.4583f, -106.9013f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(12.4413f, -109.2554f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(8.9963f,  -111.2117f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(5.5390f,  -109.2561f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(5.5505f,  -106.9029f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(15.9803f, -109.2639f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(15.9801f, -101.3128f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(18.0000f, -100.0028f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(0.0000f,  -110.3952f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(-0.0000f, -100.0028f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(2.0186f,  -101.3128f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(18.0000f, -110.3951f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(2.0376f,  -109.2639f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(9.0020f,  -113.2241f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},
		{float2_t(9.0000f,  -115.5912f), float2_t(1.0f, 0.0f), float4_t(0.0f, 1.0f, 0.0f, 0.0f)},

		// object handler
		{float2_t(9.0000f,  -100.0204f), float2_t(0.0f, 1.0f), float4_t(0.0f, 0.0f, 1.0f, 0.0f)},
		{float2_t(3.7845f,   -97.0305f), float2_t(0.0f, 1.0f), float4_t(0.0f, 0.0f, 1.0f, 0.0f)},
		{float2_t(3.7845f,   -18.6209f), float2_t(0.0f, 0.0f), float4_t(0.0f, 0.0f, 1.0f, 0.0f)},
		{float2_t(9.0000f,   -15.6036f), float2_t(0.0f, 0.0f), float4_t(0.0f, 0.0f, 1.0f, 0.0f)},
		{float2_t(14.2155f,  -18.6208f), float2_t(0.0f, 0.0f), float4_t(0.0f, 0.0f, 1.0f, 0.0f)},
		{float2_t(14.2155f,  -97.0305f), float2_t(0.0f, 1.0f), float4_t(0.0f, 0.0f, 1.0f, 0.0f)},
	};

	m_uFrameVC = ARRAYSIZE(aVertices);

	USHORT aIndices[] = {
		1, 2, 3,
		3, 4, 5,
		5, 6, 1,
		3, 5, 1,
		7, 8, 9,
		9, 10, 11,
		11, 12, 7,
		9, 11, 7,
		13, 14, 15,
		16, 17, 18,
		13, 15, 19,
		16, 18, 20,
		21, 13, 19,
		22, 16, 20,
		21, 19, 22,
		22, 20, 21,
		23, 24, 25,
		25, 26, 27,
		28, 29, 30,
		30, 31, 32,
		32, 33, 34,
		35, 23, 25,
		25, 27, 36,
		28, 30, 32,
		34, 35, 25,
		25, 36, 28,
		28, 32, 34,
		25, 28, 34,
		37, 38, 39,
		39, 40, 41,
		41, 42, 37,
		39, 41, 37,
		43, 44, 45,
		45, 46, 47,
		47, 48, 43,
		45, 47, 43,
		49, 50, 51,
		52, 53, 54,
		49, 51, 55,
		52, 54, 56,
		57, 49, 55,
		58, 52, 56,
		57, 55, 58,
		58, 56, 57,
		59, 60, 61,
		59, 61, 62,
		59, 62, 63,
		59, 63, 64
	};

	for(UINT i = 0, l = ARRAYSIZE(aIndices); i < l; ++i)
	{
		--aIndices[i];
	}

	m_uFramePC = ARRAYSIZE(aIndices) / 3;

	GXVertexElement oLayout[] =
	{
		{0, 0, GXDECLTYPE_FLOAT2, GXDECLUSAGE_POSITION},
		{0, 8, GXDECLTYPE_FLOAT2, GXDECLUSAGE_TEXCOORD},
		{0, 16, GXDECLTYPE_FLOAT4, GXDECLUSAGE_TEXCOORD1},
		GX_DECL_END()
	};
	IGXVertexDeclaration *pVD = m_pDev->createVertexDeclaration(oLayout);

	m_pFrameIB = m_pDev->createIndexBuffer(sizeof(USHORT) * ARRAYSIZE(aIndices), GXBUFFER_USAGE_STATIC, GXIT_UINT16, aIndices);

	IGXVertexBuffer *pVB = m_pDev->createVertexBuffer(sizeof(FrameVertex) * ARRAYSIZE(aVertices), GXBUFFER_USAGE_STATIC, aVertices);
	
	m_pFrameRB = m_pDev->createRenderBuffer(1, &pVB, pVD);

	mem_release(pVB);
	mem_release(pVD);


	m_idFrameShader = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "terrax_scrollbar.vs"), m_pRender->loadShader(SHADER_TYPE_PIXEL, "terrax_colored2.ps"));

	m_pInformCB = m_pDev->createConstantBuffer(sizeof(m_frameState));
}

void CScrollBar::setPos(int iX, int iY)
{
	m_frameState.vOffset = float2_t((float)iX, (float)iY);
	m_isDirty = true;
}

void CScrollBar::setSize(UINT uSize)
{
	m_uSize = uSize;

	m_frameState.fSize = (float)uSize - 115.5912f;

	updateScroll();
}

void CScrollBar::setScrollMax(int iMax)
{
	m_iScrollMax = iMax;
	updateScroll();
}

void CScrollBar::setScrollPos(int iPos)
{
	m_iScrollPos = iPos;
	updateScroll();
}

void CScrollBar::setPageSize(UINT uSize)
{
	m_uPageSize = uSize;
	updateScroll();
}

void CScrollBar::updateScroll()
{
	//printf("%f\n", ((float)m_uPageSize / (float)(m_iScrollMax + m_uPageSize)));
	m_frameState.fHandlerSize = ((float)m_uPageSize / (float)(m_iScrollMax + m_uPageSize)) * ((float)m_uSize - 37.2416f);
	m_uHandlerSize = (UINT)m_frameState.fHandlerSize;
	m_frameState.fHandlerSize -= 78.4096f;

	m_frameState.fHandlerPos = ((float)m_iScrollPos / (float)(m_iScrollMax + m_uPageSize)) * ((float)m_uSize - 37.2416f);
	m_uHandlerPos = (UINT)m_frameState.fHandlerPos + 18;

	const float c_fDisabled = 0.5f;
	const float c_fNormal = 0.75f;
	const float c_fHover = 0.9f;
	const float c_fActive = 1.0f;

	if(m_iScrollMax == 0)
	{
		m_frameState.vHighlight = float4_t(c_fDisabled, c_fDisabled, 0.0f, c_fDisabled);
	}
	else
	{
		m_frameState.vHighlight = (float4)c_fNormal;
		if(m_iScrollPos == 0)
		{
			m_frameState.vHighlight.x = c_fDisabled;
		}
		else if(m_activeEl == HEL_UP)
		{
			m_frameState.vHighlight.x = c_fActive;
		}
		else if(m_hoverEl == HEL_UP)
		{
			m_frameState.vHighlight.x = c_fHover;
		}

		if(m_iScrollPos == m_iScrollMax)
		{
			m_frameState.vHighlight.y = c_fDisabled;
		}
		else if(m_activeEl == HEL_DOWN)
		{
			m_frameState.vHighlight.y = c_fActive;
		}
		else if(m_hoverEl == HEL_DOWN)
		{
			m_frameState.vHighlight.y = c_fHover;
		}

		if(m_activeEl == HEL_HANDLER)
		{
			m_frameState.vHighlight.z = c_fActive;
		}
		else if(m_hoverEl == HEL_HANDLER)
		{
			m_frameState.vHighlight.z = c_fHover;
		}
		
	}
	m_isDirty = true;
}

void CScrollBar::onMouseDown(int y)
{
	onMouseMove(y);

	m_activeEl = m_hoverEl;

	updateScroll();

	switch(m_activeEl)
	{
	case HEL_UP:
		m_pEventListener->onLineUp();
		break;
	case HEL_DOWN:
		m_pEventListener->onLineDown();
		break;
	case HEL_HANDLER:
		m_iDragStartY = y;
		m_iDragStartPos = m_uHandlerPos;
		break;
	}

	m_fTimer = -0.5f;
}

void CScrollBar::onMouseUp()
{
	m_activeEl = HEL_NONE;

	updateScroll();
}

void CScrollBar::onMouseMove(int y)
{
	if(m_activeEl == HEL_NONE)
	{
		if(y < 16)
		{
			m_hoverEl = HEL_UP;
		}
		else if(y > m_uSize - 18)
		{
			m_hoverEl = HEL_DOWN;
		}
		else if(y > m_uHandlerPos && y < m_uHandlerPos + max(m_uHandlerSize, 16))
		{
			m_hoverEl = HEL_HANDLER;
		}
		else
		{
			m_hoverEl = HEL_NONE;
		}
	}
	else if(m_activeEl == HEL_HANDLER)
	{
		int iNewPos = m_iDragStartPos + (y - m_iDragStartY) - 18;

		iNewPos = (float)iNewPos / ((float)m_uSize - 37.2416f) * (float)(m_iScrollMax + m_uPageSize);

		if(iNewPos < 0)
		{
			iNewPos = 0;
		}
		if(iNewPos > m_iScrollMax)
		{
			iNewPos = m_iScrollMax;
		}
		m_iScrollPos = iNewPos;
		m_pEventListener->onScroll(iNewPos);
	}
	updateScroll();
}

void CScrollBar::onMouseHover(bool isHover)
{
	if(!isHover)
	{
		m_hoverEl = HEL_NONE;
		updateScroll();
	}
}

void CScrollBar::update(float fDT)
{
	if(m_activeEl != HEL_NONE)
	{
		m_fTimer += fDT;

		if(m_fTimer > 0.2f)
		{
			m_fTimer -= 0.2f;
			switch(m_activeEl)
			{
			case HEL_UP:
				m_pEventListener->onLineUp();
				break;
			case HEL_DOWN:
				m_pEventListener->onLineDown();
				break;
			}
		}
		
	}
}
