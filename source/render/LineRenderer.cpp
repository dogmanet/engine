#include "LineRenderer.h"

std::atomic_uint CLineRenderer::s_uResRefCount{0};
IGXBlendState *CLineRenderer::s_pBlendAlpha = NULL;
IGXDepthStencilState *CLineRenderer::s_pDSState3D = NULL;
IGXDepthStencilState *CLineRenderer::s_pDSState2D = NULL;
IGXDepthStencilState *CLineRenderer::s_pDSStateNoZ = NULL;
IGXVertexDeclaration *CLineRenderer::s_pLinesVD = NULL;
bool CLineRenderer::s_isShadersLoaded = false;
ID CLineRenderer::s_idShaders[2][2][2][2]; // [isTextured][is3D][isFixed][isBackward]

CLineRenderer::CLineRenderer(IXRender *pRender):
	m_pRender(pRender),
	m_pDev(pRender->getDevice())
{
	if(!s_uResRefCount)
	{
		GXVertexElement oLayout[] =
		{
			{0, 0, GXDECLTYPE_FLOAT4, GXDECLUSAGE_POSITION},
			{0, 16, GXDECLTYPE_FLOAT4, GXDECLUSAGE_TEXCOORD},
			{0, 32, GXDECLTYPE_FLOAT3, GXDECLUSAGE_TEXCOORD1},
			{0, 44, GXDECLTYPE_FLOAT2, GXDECLUSAGE_TEXCOORD2},
			GX_DECL_END()
		};
		s_pLinesVD = m_pDev->createVertexDeclaration(oLayout);

		GXBlendDesc blendDesc;
		blendDesc.renderTarget[0].useBlend = true;
		blendDesc.renderTarget[0].blendSrcColor = blendDesc.renderTarget[0].blendSrcAlpha = GXBLEND_SRC_ALPHA;
		blendDesc.renderTarget[0].blendDestColor = blendDesc.renderTarget[0].blendDestAlpha = GXBLEND_INV_SRC_ALPHA;
		s_pBlendAlpha = m_pDev->createBlendState(&blendDesc);

		GXDepthStencilDesc dsDesc;
		dsDesc.useDepthWrite = false;
		s_pDSState2D = m_pDev->createDepthStencilState(&dsDesc);

		dsDesc.cmpFuncDepth = GXCMP_GREATER_EQUAL;
		s_pDSState3D = m_pDev->createDepthStencilState(&dsDesc);

		dsDesc.useDepthTest = false;
		s_pDSStateNoZ = m_pDev->createDepthStencilState(&dsDesc);
	}

	++s_uResRefCount;

	if(!s_isShadersLoaded)
	{
		s_isShadersLoaded = true;
		GXMacro aMacro1[] = {
			{"IS_ORTHO", "1"},
			GX_MACRO_END()
		};
		GXMacro aMacro2[] = {
			{"IS_FIXED", "1"},
			GX_MACRO_END()
		};
		GXMacro aMacro3[] = {
			{"IS_ORTHO", "1"},
			{"IS_FIXED", "1"},
			GX_MACRO_END()
		};

		GXMacro aMacro4[] = {
			{"IS_BACKWARD", "1"},
			GX_MACRO_END()
		};
		ID idPixelForward = m_pRender->loadShader(SHADER_TYPE_PIXEL, "dev_lines.ps");
		ID idPixelBackward = m_pRender->loadShader(SHADER_TYPE_PIXEL, "dev_lines.ps", aMacro4);

		s_idShaders[0][1][0][0] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_lines.vs"), idPixelForward);
		s_idShaders[0][1][1][0] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_lines.vs", aMacro2), idPixelForward);
		s_idShaders[0][0][0][0] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_lines.vs", aMacro1), idPixelForward);
		s_idShaders[0][0][1][0] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_lines.vs", aMacro3), idPixelForward);

		s_idShaders[0][1][0][1] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_lines.vs"), idPixelBackward);
		s_idShaders[0][1][1][1] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_lines.vs", aMacro2), idPixelBackward);
		s_idShaders[0][0][0][1] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_lines.vs", aMacro1), idPixelBackward);
		s_idShaders[0][0][1][1] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_lines.vs", aMacro3), idPixelBackward);


		GXMacro aMacro[] = {
			{"USE_TEXTURE", "1"},
			GX_MACRO_END()
		};
		GXMacro aMacro5[] = {
			{"USE_TEXTURE", "1"},
			{"IS_BACKWARD", "1"},
			GX_MACRO_END()
		};

		ID idPixelTexForward = m_pRender->loadShader(SHADER_TYPE_PIXEL, "dev_lines.ps", aMacro);
		ID idPixelTexBackward = m_pRender->loadShader(SHADER_TYPE_PIXEL, "dev_lines.ps", aMacro5);

		s_idShaders[1][1][0][0] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_lines.vs"), idPixelTexForward);
		s_idShaders[1][1][1][0] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_lines.vs", aMacro2), idPixelTexForward);
		s_idShaders[1][0][0][0] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_lines.vs", aMacro1), idPixelTexForward);
		s_idShaders[1][0][1][0] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_lines.vs", aMacro3), idPixelTexForward);

		s_idShaders[1][1][0][1] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_lines.vs"), idPixelTexBackward);
		s_idShaders[1][1][1][1] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_lines.vs", aMacro2), idPixelTexBackward);
		s_idShaders[1][0][0][1] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_lines.vs", aMacro1), idPixelTexBackward);
		s_idShaders[1][0][1][1] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_lines.vs", aMacro3), idPixelTexBackward);
	}
}
CLineRenderer::~CLineRenderer()
{
	if(--s_uResRefCount)
	{
		mem_release(s_pBlendAlpha);
		mem_release(s_pDSState2D);
		mem_release(s_pDSState3D);
		mem_release(s_pDSStateNoZ);
		mem_release(s_pLinesVD);
	}

	mem_release(m_pLinesRB);
	mem_release(m_pLinesVB);
	//mem_release(m_pLinesIB);

}

void XMETHODCALLTYPE CLineRenderer::reset()
{
	mem_release(m_pCurrentTexture);
	m_isCurrentTextureDirty = true;

	for(UINT i = 0, l = m_aTextures.size(); i < l; ++i)
	{
		mem_release(m_aTextures[i]);
	}

	m_aVertices.clearFast();
	m_aTextures.clearFast();
	m_isLinesDirty = true;

	m_uLineCount = 0;
	m_uLineSegmentCount = 0;
}
void XMETHODCALLTYPE CLineRenderer::render(bool isOrtho, bool useConstantSize, bool useDepthTest)
{
	if(m_isLinesDirty)
	{
		m_isLinesDirty = false;

		if(!m_uLineCount)
		{
			m_aLineRanges.clearFast();
			return;
		}

		// update buffers
		UINT uVertexCount = m_uLineSegmentCount * 4 + (m_uLineCount - 1) * 2;
		if(!uVertexCount)
		{
			m_aLineRanges.clearFast();
			return;
		}
		if(m_uLinesVBSize < uVertexCount)
		{
			m_uLinesVBSize = uVertexCount;
			printf("Resizing to %u\n", m_uLinesVBSize);
			mem_release(m_pLinesVB);
			mem_release(m_pLinesRB);

			m_pLinesVB = m_pDev->createVertexBuffer(sizeof(LineVertex) * uVertexCount, GXBUFFER_USAGE_DYNAMIC);
			m_pLinesRB = m_pDev->createRenderBuffer(1, &m_pLinesVB, s_pLinesVD);
		}

		m_aLineRanges.clearFast();

		UINT uVC = 0;
		LineVertex *pVertices;
		if(m_pLinesVB->lock((void**)&pVertices, GXBL_WRITE))
		{
			byte u8TexIdx;
			m_aLineRanges.reserve(m_aTextures.size() + 1);
			for(int i = -1, l = m_aTextures.size(); i < l; ++i)
			{
				if(i == -1)
				{
					u8TexIdx = 0xFF;
				}
				else
				{
					u8TexIdx = i;
				}

				LineRange &lr = m_aLineRanges[m_aLineRanges.size()];
				lr.u8Texture = u8TexIdx;

				lr.uStartVtx = uVC;
				bool isFirst = true;
				// 0123|34|4567|78|891011
				for(UINT j = 0, jl = m_aVertices.size(); j < jl; ++j)
				{
					LinePoint &pt = m_aVertices[j];
					if(pt.u8Texture == u8TexIdx)
					{
						if(pt.isStart)
						{
							if(!isFirst)
							{
								pVertices[uVC] = pVertices[uVC - 1];
								++uVC;


								pt.vtx.vTexUV = float2_t(0.0f, 0.0f);
								pVertices[uVC++] = pt.vtx;
							}
						}
						else
						{
							LinePoint &ptPrev = m_aVertices[j - 1];
							
							// add 4 vertices
							ptPrev.vtx.vTexUV = float2_t(0.0f, 0.0f);
							pVertices[uVC] = ptPrev.vtx;
							pVertices[uVC].vDir = pt.vtx.vDir;
							++uVC;
							ptPrev.vtx.vTexUV = float2_t(0.0f, 1.0f);
							pVertices[uVC] = ptPrev.vtx;
							pVertices[uVC].vDir = pt.vtx.vDir;
							++uVC;

							pt.vtx.vTexUV = float2_t(1.0f, 0.0f);
							pVertices[uVC++] = pt.vtx;
							pt.vtx.vTexUV = float2_t(1.0f, 1.0f);
							pVertices[uVC++] = pt.vtx;
						}

						isFirst = false;
					}
				}

				if(uVC > lr.uStartVtx + 2)
				{
					lr.uTriangleCount = uVC - lr.uStartVtx - 2;
				}
				else
				{
					m_aLineRanges.erase(m_aLineRanges.size() - 1);
				}
			}

			m_pLinesVB->unlock();
		}

		assert(uVC == uVertexCount);
	}


	IGXContext *pCtx = m_pDev->getThreadContext();
	
	IGXRasterizerState *pRS = pCtx->getRasterizerState();
	pCtx->setRasterizerState(NULL);
	IGXBlendState *pBS = pCtx->getBlendState();
	pCtx->setBlendState(s_pBlendAlpha);
	IGXDepthStencilState *pDS = pCtx->getDepthStencilState();
	pCtx->setDepthStencilState(useDepthTest ? (isOrtho ? s_pDSState2D : s_pDSState3D) : s_pDSStateNoZ);

	pCtx->setRenderBuffer(m_pLinesRB);

	pCtx->setPrimitiveTopology(GXPT_TRIANGLESTRIP);
	for(UINT i = 0, l = m_aLineRanges.size(); i < l; ++i)
	{
		LineRange &lr = m_aLineRanges[i];
		m_pRender->bindShader(pCtx, s_idShaders[lr.u8Texture != 0xFF ? 1 : 0][isOrtho ? 0 : 1][useConstantSize ? 1 : 0][0]);
		
		if(lr.u8Texture != 0xFF)
		{
			pCtx->setPSTexture(m_aTextures[lr.u8Texture]);
		}
		if(useDepthTest)
		{
			m_pRender->bindShader(pCtx, s_idShaders[lr.u8Texture != 0xFF ? 1 : 0][isOrtho ? 0 : 1][useConstantSize ? 1 : 0][0]);
			pCtx->setDepthStencilState(isOrtho ? s_pDSState2D : s_pDSState3D);
			pCtx->drawPrimitive(lr.uStartVtx, lr.uTriangleCount);
			m_pRender->bindShader(pCtx, s_idShaders[lr.u8Texture != 0xFF ? 1 : 0][isOrtho ? 0 : 1][useConstantSize ? 1 : 0][1]);
			pCtx->setDepthStencilState(isOrtho ? s_pDSState3D : s_pDSState2D);
			pCtx->drawPrimitive(lr.uStartVtx, lr.uTriangleCount);
		}
		else
		{
			pCtx->drawPrimitive(lr.uStartVtx, lr.uTriangleCount);
		}
	}
	m_pRender->unbindShader(pCtx);
	pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);

	pCtx->setRasterizerState(pRS);
	mem_release(pRS);
	pCtx->setBlendState(pBS);
	mem_release(pBS);
	pCtx->setDepthStencilState(pDS);
	mem_release(pDS);
}


void XMETHODCALLTYPE CLineRenderer::setWidth(float fWidth)
{
	m_fCurrentLineWidth = fWidth;
}

void XMETHODCALLTYPE CLineRenderer::setColor(const float4 &vColor)
{
	m_vCurrentColor = vColor;
}

void XMETHODCALLTYPE CLineRenderer::setTexture(IGXBaseTexture *pTexture)
{
	if(m_pCurrentTexture == pTexture)
	{
		return;
	}
	add_ref(pTexture);
	mem_release(m_pCurrentTexture);
	m_pCurrentTexture = pTexture;
	m_isCurrentTextureDirty = true;
}


void XMETHODCALLTYPE CLineRenderer::jumpTo(const float3 &vPos)
{
	m_isLineStart = true;
	m_vNextLineStart = vPos;
	m_vLastPointColor = m_vCurrentColor;
}

void XMETHODCALLTYPE CLineRenderer::lineTo(const float3 &vPos)
{
	LineVertex vtx;
	vtx.vDir = (float3)(vPos - m_vNextLineStart);
	if(m_isLineStart)
	{
		vtx.vPosWidth = float4(m_vNextLineStart, m_fCurrentLineWidth);
		vtx.vColor = m_vLastPointColor;
		m_aVertices.push_back({vtx, getTextureId(), true});
		m_isLineStart = false;
		++m_uLineCount;
	}

	vtx.vPosWidth = float4(vPos, m_fCurrentLineWidth);
	vtx.vColor = m_vCurrentColor;
	m_aVertices.push_back({vtx, getTextureId(), false});

	++m_uLineSegmentCount;

	m_vNextLineStart = vPos;
	m_vLastPointColor = m_vCurrentColor;

	m_isLinesDirty = true;
}

byte CLineRenderer::getTextureId()
{
	if(m_isCurrentTextureDirty)
	{
		m_isCurrentTextureDirty = false;

		if(m_pCurrentTexture)
		{
			int idx = m_aTextures.indexOf(m_pCurrentTexture);
			if(idx < 0)
			{
				idx = m_aTextures.size();
				m_aTextures.push_back(m_pCurrentTexture);
				m_pCurrentTexture->AddRef();
			}
			m_u8CurrentTexture = idx;
		}
		else
		{
			m_u8CurrentTexture = 0xFF;
		}
	}

	return(m_u8CurrentTexture);
}
