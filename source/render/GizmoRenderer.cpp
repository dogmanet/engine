#include "GizmoRenderer.h"
#include "RenderUtils.h"

std::atomic_uint CGizmoRenderer::s_uResRefCount{0};
IGXVertexDeclaration *CGizmoRenderer::s_pPointsVD = NULL;
IGXBlendState *CGizmoRenderer::s_pBlendAlpha = NULL;
IGXDepthStencilState *CGizmoRenderer::s_pDSState3D = NULL;
IGXDepthStencilState *CGizmoRenderer::s_pDSState2D = NULL;
IGXDepthStencilState *CGizmoRenderer::s_pDSStateNoZ = NULL;
bool CGizmoRenderer::s_isShadersLoaded = false;
ID CGizmoRenderer::s_idShaders[2][2][2]; // [isTextured][is3D][isFixed]
IGXVertexDeclaration *CGizmoRenderer::s_pTextVD;
ID CGizmoRenderer::s_idTextShaders[2][2]; // [is3D][isFixed]

CGizmoRenderer::CGizmoRenderer(CRenderUtils *pRenderUtils, IXRender *pRender):
	m_pRenderUtils(pRenderUtils),
	m_pRender(pRender),
	m_pDev(pRender->getDevice()),
	m_lineRenderer(pRender)
{
	add_ref(m_pRenderUtils);

	if(!s_uResRefCount)
	{
		GXVertexElement oLayout[] =
		{
			{0, 0, GXDECLTYPE_FLOAT4, GXDECLUSAGE_POSITION},
			{0, 16, GXDECLTYPE_FLOAT4, GXDECLUSAGE_TEXCOORD},
			{0, 32, GXDECLTYPE_FLOAT3, GXDECLUSAGE_TEXCOORD2},
			GX_DECL_END()
		};
		s_pTextVD = m_pDev->createVertexDeclaration(oLayout);

		GXVertexElement oLayoutText[] =
		{
			{0, 0, GXDECLTYPE_FLOAT4, GXDECLUSAGE_POSITION},
			{0, 16, GXDECLTYPE_FLOAT4, GXDECLUSAGE_TEXCOORD},
			{0, 32, GXDECLTYPE_FLOAT3, GXDECLUSAGE_TEXCOORD2},
			GX_DECL_END()
		};
		s_pPointsVD = m_pDev->createVertexDeclaration(oLayoutText);

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
		s_idShaders[0][1][0] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_points.vs"), m_pRender->loadShader(SHADER_TYPE_PIXEL, "dev_points.ps"));
		s_idShaders[0][1][1] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_points.vs", aMacro2), m_pRender->loadShader(SHADER_TYPE_PIXEL, "dev_points.ps"));
		s_idShaders[0][0][0] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_points.vs", aMacro1), m_pRender->loadShader(SHADER_TYPE_PIXEL, "dev_points.ps"));
		s_idShaders[0][0][1] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_points.vs", aMacro3), m_pRender->loadShader(SHADER_TYPE_PIXEL, "dev_points.ps"));

		GXMacro aMacro[] = {
			{"USE_TEXTURE", "1"},
			GX_MACRO_END()
		};
		s_idShaders[1][1][0] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_points.vs"), m_pRender->loadShader(SHADER_TYPE_PIXEL, "dev_points.ps", aMacro));
		s_idShaders[1][1][1] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_points.vs", aMacro2), m_pRender->loadShader(SHADER_TYPE_PIXEL, "dev_points.ps", aMacro));
		s_idShaders[1][0][0] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_points.vs", aMacro1), m_pRender->loadShader(SHADER_TYPE_PIXEL, "dev_points.ps", aMacro));
		s_idShaders[1][0][1] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_points.vs", aMacro3), m_pRender->loadShader(SHADER_TYPE_PIXEL, "dev_points.ps", aMacro));

		s_idTextShaders[1][0] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_text.vs"), m_pRender->loadShader(SHADER_TYPE_PIXEL, "dev_text.ps"));
		s_idTextShaders[1][1] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_text.vs", aMacro2), m_pRender->loadShader(SHADER_TYPE_PIXEL, "dev_text.ps"));
		s_idTextShaders[0][0] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_text.vs", aMacro1), m_pRender->loadShader(SHADER_TYPE_PIXEL, "dev_text.ps"));
		s_idTextShaders[0][1] = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "dev_text.vs", aMacro3), m_pRender->loadShader(SHADER_TYPE_PIXEL, "dev_text.ps"));
	}


	//m_pRightVecCB = pDev->createConstantBuffer(sizeof(float3));
}
CGizmoRenderer::~CGizmoRenderer()
{
	mem_release(m_pRenderUtils);
	//mem_release(m_pRightVecCB);

	if(--s_uResRefCount)
	{
		mem_release(s_pBlendAlpha);
		mem_release(s_pDSState2D);
		mem_release(s_pDSState3D);
		mem_release(s_pDSStateNoZ);
		mem_release(s_pPointsVD);
		mem_release(s_pTextVD);
	}

	mem_release(m_pPointsRB);
	mem_release(m_pPointsVB);
	mem_release(m_pPointsIB);

	mem_release(m_pTextRB);
	mem_release(m_pTextVB);
	mem_release(m_pTextIB);

	mem_release(m_pTrisRB);
	mem_release(m_pTrisVB);
}

void XMETHODCALLTYPE CGizmoRenderer::reset()
{
	m_lineRenderer.reset();
	m_isDirty = true;
	m_aPoints.clearFast();

	mem_release(m_pCurrentTexture);
	m_isCurrentTextureDirty = true;
	for(UINT i = 0, l = m_aTextures.size(); i < l; ++i)
	{
		mem_release(m_aTextures[i]);
	}
	m_aTextures.clearFast();

	m_aPolyVertices.clearFast();

	m_aTextVertices.clearFast();
	m_aTextRanges.clearFast();
}
void XMETHODCALLTYPE CGizmoRenderer::render(bool isOrtho, bool useConstantSize, bool useDepthTest)
{
	m_lineRenderer.render(isOrtho, useConstantSize, useDepthTest);

	if(m_isDirty)
	{
		m_isDirty = false;

		UINT uVertexCount = m_aPoints.size() * 4;
		m_aPointRanges.clearFast();
		if(uVertexCount)
		{
			if(m_uPointsVBSize < uVertexCount)
			{
				m_uPointsVBSize = uVertexCount;
				mem_release(m_pPointsVB);
				mem_release(m_pPointsRB);

				m_pPointsVB = m_pDev->createVertexBuffer(sizeof(PointVertex) * uVertexCount, GXBUFFER_USAGE_DYNAMIC);
				m_pPointsRB = m_pDev->createRenderBuffer(1, &m_pPointsVB, s_pPointsVD);
			}

			UINT uMaxQuadCount = 0;
			UINT uVC = 0;
			PointVertex *pVertices;
			if(m_pPointsVB->lock((void**)&pVertices, GXBL_WRITE))
			{
				byte u8TexIdx;
				m_aPointRanges.reserve(m_aTextures.size() + 1);
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

					PointRange &lr = m_aPointRanges[m_aPointRanges.size()];
					lr.u8Texture = u8TexIdx;

					lr.uStartVtx = uVC;
					for(UINT j = 0, jl = m_aPoints.size(); j < jl; ++j)
					{
						Point &pt = m_aPoints[j];
						if(pt.u8Texture == u8TexIdx)
						{
							float fPM = pt.mode == XGPM_SQUARE ? 0.0f : 1.0f;
							pt.vtx.vTexUVMode = float3_t(0.0f, 1.0f, fPM);
							pVertices[uVC++] = pt.vtx;
							pt.vtx.vTexUVMode = float3_t(0.0f, 0.0f, fPM);
							pVertices[uVC++] = pt.vtx;
							pt.vtx.vTexUVMode = float3_t(1.0f, 1.0f, fPM);
							pVertices[uVC++] = pt.vtx;
							pt.vtx.vTexUVMode = float3_t(1.0f, 0.0f, fPM);
							pVertices[uVC++] = pt.vtx;
						}
					}

					if(uVC > lr.uStartVtx)
					{
						lr.uQuadCount = (uVC - lr.uStartVtx) / 4;
						if(lr.uQuadCount > uMaxQuadCount)
						{
							uMaxQuadCount = lr.uQuadCount;
						}
					}
					else
					{
						m_aPointRanges.erase(m_aPointRanges.size() - 1);
					}
				}

				m_pPointsVB->unlock();
			}

			assert(uVC == uVertexCount);

			mem_release(m_pPointsIB);
			m_pRenderUtils->getQuadIndexBuffer(uMaxQuadCount, &m_pPointsIB);
		}

		//**************************************************

		uVertexCount = m_aPolyVertices.size();
		if(uVertexCount)
		{
			if(m_uTrisVBSize < uVertexCount)
			{
				m_uTrisVBSize = uVertexCount;
				mem_release(m_pTrisRB);
				mem_release(m_pTrisVB);

				m_pTrisVB = m_pDev->createVertexBuffer(sizeof(PointVertex) * uVertexCount, GXBUFFER_USAGE_DYNAMIC);
				m_pTrisRB = m_pDev->createRenderBuffer(1, &m_pTrisVB, s_pPointsVD);
			}

			PointVertex *pVertices;
			if(m_pTrisVB->lock((void**)&pVertices, GXBL_WRITE))
			{
				memcpy(pVertices, m_aPolyVertices, sizeof(PointVertex) * uVertexCount);
				m_pTrisVB->unlock();
			}
		}

		//**************************************************

		uVertexCount = m_aTextVertices.size();
		if(uVertexCount)
		{
			if(m_uTextVBSize < uVertexCount)
			{
				m_uTextVBSize = uVertexCount;
				mem_release(m_pTextRB);
				mem_release(m_pTextVB);

				m_pTextVB = m_pDev->createVertexBuffer(sizeof(TextVertex) * uVertexCount, GXBUFFER_USAGE_DYNAMIC);
				m_pTextRB = m_pDev->createRenderBuffer(1, &m_pTextVB, s_pTextVD);
			}

			PointVertex *pVertices;
			if(m_pTextVB->lock((void**)&pVertices, GXBL_WRITE))
			{
				memcpy(pVertices, m_aTextVertices, sizeof(TextVertex) * uVertexCount);
				m_pTextVB->unlock();
			}

			mem_release(m_pTextIB);
			m_pRenderUtils->getQuadIndexBuffer(uVertexCount / 4, &m_pTextIB);
		}
	}

	IGXContext *pCtx = m_pDev->getThreadContext();
	
	IGXRasterizerState *pRS = pCtx->getRasterizerState();
	pCtx->setRasterizerState(NULL);
	IGXBlendState *pBS = pCtx->getBlendState();
	pCtx->setBlendState(s_pBlendAlpha);
	IGXDepthStencilState *pDS = pCtx->getDepthStencilState();
	pCtx->setDepthStencilState(isOrtho ? s_pDSState2D : s_pDSState3D);
	
	//float3 vRight = gdata::pCamera->getRight();
	//m_pRightVecCB->update(&vRight);
	//pCtx->setVSConstant(m_pRightVecCB, 6);

	pCtx->setRenderBuffer(m_pPointsRB);
	pCtx->setIndexBuffer(m_pPointsIB);

	pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);
	for(UINT i = 0, l = m_aPointRanges.size(); i < l; ++i)
	{
		PointRange &lr = m_aPointRanges[i];
		m_pRender->bindShader(pCtx, s_idShaders[lr.u8Texture != 0xFF ? 1 : 0][isOrtho ? 0 : 1][useConstantSize ? 1 : 0]);

		if(lr.u8Texture != 0xFF)
		{
			pCtx->setPSTexture(m_aTextures[lr.u8Texture]);
		}
		pCtx->drawIndexed(lr.uQuadCount * 4, lr.uQuadCount * 2, 0, lr.uStartVtx);
	}

	if(m_aPolyVertices.size())
	{
		pCtx->setRenderBuffer(m_pTrisRB);
		m_pRender->bindShader(pCtx, s_idShaders[0][isOrtho ? 0 : 1][/*useConstantSize ? 1 : */0]);
		pCtx->drawPrimitive(0, m_aPolyVertices.size() / 3);
	}


	pCtx->setRenderBuffer(m_pTextRB);
	pCtx->setIndexBuffer(m_pTextIB);
	for(UINT i = 0, l = m_aTextRanges.size(); i < l; ++i)
	{
		PointRange &lr = m_aTextRanges[i];
		m_pRender->bindShader(pCtx, s_idTextShaders[isOrtho ? 0 : 1][useConstantSize ? 1 : 0]);

		pCtx->setPSTexture(m_aTextures[lr.u8Texture]);
		pCtx->drawIndexed(lr.uQuadCount * 4, lr.uQuadCount * 2, 0, lr.uStartVtx);
	}

	m_pRender->unbindShader(pCtx);

	pCtx->setRasterizerState(pRS);
	mem_release(pRS);
	pCtx->setBlendState(pBS);
	mem_release(pBS);
	pCtx->setDepthStencilState(pDS);
	mem_release(pDS);
}


void XMETHODCALLTYPE CGizmoRenderer::setLineWidth(float fWidth)
{
	m_lineRenderer.setWidth(fWidth);
}

void XMETHODCALLTYPE CGizmoRenderer::setColor(const float4 &vColor)
{
	m_vCurrentColor = vColor;
	m_lineRenderer.setColor(vColor);
}

void XMETHODCALLTYPE CGizmoRenderer::setTexture(IGXBaseTexture *pTexture)
{
	m_lineRenderer.setTexture(pTexture);

	if(m_pCurrentTexture == pTexture)
	{
		return;
	}
	add_ref(pTexture);
	mem_release(m_pCurrentTexture);
	m_pCurrentTexture = pTexture;
	m_isCurrentTextureDirty = true;
}


void XMETHODCALLTYPE CGizmoRenderer::jumpTo(const float3 &vPos)
{
	m_lineRenderer.jumpTo(vPos);
}

void XMETHODCALLTYPE CGizmoRenderer::lineTo(const float3 &vPos)
{
	m_lineRenderer.lineTo(vPos);
}


void XMETHODCALLTYPE CGizmoRenderer::setPointSize(float fWidth)
{
	m_fCurrentPointSize = fWidth;
}

void XMETHODCALLTYPE CGizmoRenderer::setPointMode(XGIZMO_POINT_MODE pointMode)
{
	m_pointMode = pointMode;
}

void XMETHODCALLTYPE CGizmoRenderer::drawPoint(const float3 &vPos)
{
	Point &pt = m_aPoints[m_aPoints.size()];
	PointVertex &vtx = pt.vtx;
	vtx.vPosWidth = float4(vPos, m_fCurrentPointSize);
	pt.mode = m_pointMode;
	pt.u8Texture = getTextureId();
	vtx.vColor = m_vCurrentColor;

	m_isDirty = true;
}


void XMETHODCALLTYPE CGizmoRenderer::drawAABB(const SMAABB &aabb)
{
	jumpTo(aabb.vMin);
	lineTo(float3(aabb.vMin.x, aabb.vMin.y, aabb.vMax.z));
	lineTo(float3(aabb.vMax.x, aabb.vMin.y, aabb.vMax.z));
	lineTo(aabb.vMax);
	lineTo(float3(aabb.vMax.x, aabb.vMax.y, aabb.vMin.z));
	lineTo(float3(aabb.vMax.x, aabb.vMin.y, aabb.vMin.z));
	lineTo(aabb.vMin);
	lineTo(float3(aabb.vMin.x, aabb.vMax.y, aabb.vMin.z));
	lineTo(float3(aabb.vMin.x, aabb.vMax.y, aabb.vMax.z));
	lineTo(float3(aabb.vMin.x, aabb.vMin.y, aabb.vMax.z));
	jumpTo(float3(aabb.vMin.x, aabb.vMax.y, aabb.vMin.z));
	lineTo(float3(aabb.vMax.x, aabb.vMax.y, aabb.vMin.z));
	jumpTo(float3(aabb.vMin.x, aabb.vMax.y, aabb.vMax.z));
	lineTo(aabb.vMax);
	jumpTo(float3(aabb.vMax.x, aabb.vMin.y, aabb.vMin.z));
	lineTo(float3(aabb.vMax.x, aabb.vMin.y, aabb.vMax.z));
}

void XMETHODCALLTYPE CGizmoRenderer::drawEllipsoid(const float3 &vPos, const float3 &vSize)
{
	const int nSides = 16;
	const int nSlices = 8;

	float fUpStep = SM_PIDIV2 / (float)nSlices;
	float fRoundStep = SM_2PI / (float)nSides;

	for(int i = 0; i < nSlices; ++i)
	{
		float3 vec = SMQuaternion(fUpStep * (float)i, 'z') * float3(1.0f, 0.0f, 0.0f);

		jumpTo(vec * vSize + vPos);
		for(int j = 0; j <= nSides; ++j)
		{
			lineTo(SMQuaternion(fRoundStep * (float)j, 'y') * vec * vSize + vPos);
		}

		if(i)
		{
			vec.y *= -1.0f;
			jumpTo(vec * vSize + vPos);
			for(int j = 0; j <= nSides; ++j)
			{
				lineTo(SMQuaternion(fRoundStep * (float)j, 'y') * vec * vSize + vPos);
			}
		}
	}

	for(int j = 0; j < nSides; ++j)
	{
		
		for(int i = -nSlices; i <= nSlices; ++i)
		{
			float3 vec = SMQuaternion(fUpStep * (float)i, 'z') * SMQuaternion(fRoundStep * (float)j, 'y') * float3(1.0f, 0.0f, 0.0f) * vSize + vPos;
			if(i == -nSlices)
			{
				jumpTo(vec);
			}
			else
			{
				lineTo(vec);
			}
		}
	}
}

byte CGizmoRenderer::getTextureId()
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
				add_ref(m_pCurrentTexture);
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

void XMETHODCALLTYPE CGizmoRenderer::drawPoly(
	const float3_t &vPosA,
	const float3_t &vPosB,
	const float3_t &vPosC)
{
	drawPoly(vPosA, vPosB, vPosC, m_vCurrentColor, m_vCurrentColor, m_vCurrentColor);
}
void XMETHODCALLTYPE CGizmoRenderer::drawPoly(
	const float3_t &vPosA,
	const float3_t &vPosB,
	const float3_t &vPosC,
	const float4_t &vColorA,
	const float4_t &vColorB,
	const float4_t &vColorC)
{
	m_aPolyVertices.push_back({float4(vPosA, 1.0f), vColorA, float3(float2_t(0.5f, 0.5f), 0.0f)});
	m_aPolyVertices.push_back({float4(vPosB, 1.0f), vColorB, float3(float2_t(0.5f, 0.5f), 0.0f)});
	m_aPolyVertices.push_back({float4(vPosC, 1.0f), vColorC, float3(float2_t(0.5f, 0.5f), 0.0f)});

	m_aPolyVertices.push_back({float4(vPosA, 1.0f), vColorA, float3(float2_t(0.5f, 0.5f), 0.0f)});
	m_aPolyVertices.push_back({float4(vPosC, 1.0f), vColorC, float3(float2_t(0.5f, 0.5f), 0.0f)});
	m_aPolyVertices.push_back({float4(vPosB, 1.0f), vColorB, float3(float2_t(0.5f, 0.5f), 0.0f)});

	m_isDirty = true;
}

void XMETHODCALLTYPE CGizmoRenderer::setFont(IXFont *pFont)
{
	if(m_pCurrentFont == pFont)
	{
		return;
	}
	add_ref(pFont);
	mem_release(m_pCurrentFont);
	m_pCurrentFont = pFont;
}

void XMETHODCALLTYPE CGizmoRenderer::setFontParams(const XGizmoFontParams &params)
{
	m_fontParams = params;
}

void XMETHODCALLTYPE CGizmoRenderer::drawString(
	const char *szText,
	const float3_t &vRefPoint,
	UINT uAreaWidth
)
{
	if(!m_pCurrentFont)
	{
		return;
	}

	XFontBuildParams xfbp;
	xfbp.decoration = m_fontParams.decoration;
	xfbp.textAlign = m_fontParams.textAlign;
	xfbp.uAreaWidth = uAreaWidth;
	xfbp.uFirstLineShift = m_fontParams.uFirstLineShift;
	xfbp.pVertices = NULL;
	xfbp.pCharRects = NULL;

	XFontStringMetrics xfsm = {};
	m_pCurrentFont->buildString(szText, xfbp, &xfsm);
	if(!xfsm.uVertexCount)
	{
		return;
	}

	xfbp.pVertices = (XFontVertex*)alloca(sizeof(XFontVertex) * xfsm.uVertexCount);
	m_pCurrentFont->buildString(szText, xfbp, &xfsm);

	float3 vMin = xfbp.pVertices[0].vPos;
	float3 vMax = xfbp.pVertices[0].vPos;
	UINT uMaxTex = xfbp.pVertices[0].uTexIdx;

	for(UINT i = 1; i < xfsm.uVertexCount; ++i)
	{
		vMin = SMVectorMin(vMin, xfbp.pVertices[i].vPos);
		vMax = SMVectorMax(vMax, xfbp.pVertices[i].vPos);
		uMaxTex = max(uMaxTex, xfbp.pVertices[i].uTexIdx);
	}

	//drawAABB(SMAABB(vMin, vMax));

	float2_t vOffset;
	switch(m_fontParams.verticalAlign)
	{
	case XGTVA_TOP:
		break;
	case XGTVA_BOTTOM:
		vOffset.y = vMax.y;
		break;
	case XGTVA_MIDDLE:
		vOffset.y = (vMax.y - vMin.y) * 0.5f + vMin.y;
		break;
	}

	switch(m_fontParams.textAlign)
	{
	case XFONT_TEXT_ALIGN_LEFT:
		break;
	case XFONT_TEXT_ALIGN_RIGHT:
		vOffset.x = -vMax.x;
		break;
	case XFONT_TEXT_ALIGN_CENTER:
		vOffset.x = -vMax.x * 0.5f;
		break;
	}

	vOffset.x = roundf(vOffset.x);
	vOffset.y = roundf(vOffset.y);

	IGXBaseTexture *pOldTexture = m_pCurrentTexture;
	add_ref(pOldTexture);

	TextVertex vtx;
	vtx.vColor = m_vCurrentColor;

	for(UINT uCurTex = 0; uCurTex <= uMaxTex; ++uCurTex)
	{
		PointRange *pRange = NULL;
		byte u8TexId;
		for(UINT i = 0; i < xfsm.uVertexCount; ++i)
		{
			XFontVertex &fv = xfbp.pVertices[i];
			if(fv.uTexIdx == uCurTex)
			{
				if(!pRange)
				{
					assert(i % 4 == 0);

					IGXTexture2D *pTex = NULL;
					m_pCurrentFont->getTexture(uCurTex, &pTex);
					setTexture(pTex);

					u8TexId = getTextureId();
					if(!m_aTextRanges.size() || m_aTextRanges[m_aTextRanges.size() - 1].u8Texture != u8TexId)
					{
						pRange = &m_aTextRanges[m_aTextRanges.size()];
						pRange->u8Texture = u8TexId;
						pRange->uQuadCount = 0;
						pRange->uStartVtx = m_aTextVertices.size();
					}
					else
					{
						pRange = &m_aTextRanges[m_aTextRanges.size() - 1];
					}
				}

				vtx.vPosTexUV = float4(fv.vPos.x + vOffset.x, fv.vPos.y - vOffset.y, fv.vTex.x, fv.vTex.y);
				vtx.vRefPos = vRefPoint;

				m_aTextVertices.push_back(vtx);

				if(i % 4 == 3)
				{
					++pRange->uQuadCount;
				}
			}
		}
	}

	setTexture(pOldTexture);
	mem_release(pOldTexture);

	m_isDirty = true;
}
