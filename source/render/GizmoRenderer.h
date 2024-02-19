#ifndef __GIZMORENDERER_H
#define __GIZMORENDERER_H

#include <xcommon/render/IXRenderUtils.h>
#include <xcommon/render/IXRender.h>
#include "LineRenderer.h"

class CRenderUtils;

class CGizmoRenderer final: public IXUnknownImplementation<IXGizmoRenderer>
{
public:
	CGizmoRenderer(CRenderUtils *pRenderUtils, IXRender *pRender);
	~CGizmoRenderer();

	void XMETHODCALLTYPE reset() override;
	void XMETHODCALLTYPE render(bool isOrtho, bool useConstantSize = true, bool useDepthTest = true) override;

	void XMETHODCALLTYPE setLineWidth(float fWidth) override;

	void XMETHODCALLTYPE setColor(const float4 &vColor) override;
	void XMETHODCALLTYPE setTexture(IGXBaseTexture *pTexture) override;

	void XMETHODCALLTYPE jumpTo(const float3 &vPos) override;
	void XMETHODCALLTYPE lineTo(const float3 &vPos) override;


	
	void XMETHODCALLTYPE setPointSize(float fWidth) override;
	void XMETHODCALLTYPE setPointMode(XGIZMO_POINT_MODE pointMode) override;
	void XMETHODCALLTYPE drawPoint(const float3 &vPos) override;

	void XMETHODCALLTYPE drawAABB(const SMAABB &aabb) override;

	void XMETHODCALLTYPE drawEllipsoid(const float3 &vPos, const float3 &vSize) override;

	void XMETHODCALLTYPE drawPoly(
		const float3_t &vPosA,
		const float3_t &vPosB,
		const float3_t &vPosC) override;
	void XMETHODCALLTYPE drawPoly(
		const float3_t &vPosA,
		const float3_t &vPosB,
		const float3_t &vPosC,
		const float4_t &vColorA,
		const float4_t &vColorB,
		const float4_t &vColorC) override;

	void XMETHODCALLTYPE setFont(IXFont *pFont) override;

	void XMETHODCALLTYPE setFontParams(const XGizmoFontParams &params) override;

	void XMETHODCALLTYPE drawString(
		const char *szText,
		const float3_t &vRefPoint,
		UINT uAreaWidth = 0
	) override;

private:
	IXRender *m_pRender;
	IGXDevice *m_pDev;
	CRenderUtils *m_pRenderUtils;

	static std::atomic_uint s_uResRefCount;
	static IGXBlendState *s_pBlendAlpha;
	static IGXDepthStencilState *s_pDSState3D;
	static IGXDepthStencilState *s_pDSState2D;
	static IGXDepthStencilState *s_pDSStateNoZ;

	IGXBaseTexture *m_pCurrentTexture = NULL;
	byte m_u8CurrentTexture = 0xFF;
	bool m_isCurrentTextureDirty = false;
	float4_t m_vCurrentColor = float4_t(0.0f, 0.0f, 0.0f, 1.0f);
	float m_fCurrentPointSize = 1.0f;
	XGIZMO_POINT_MODE m_pointMode = XGPM_SQUARE;
	Array<IGXBaseTexture*> m_aTextures;

	CLineRenderer m_lineRenderer;

	bool m_isDirty = false;


	struct PointVertex
	{
		float4_t vPosWidth;
		float4_t vColor;
		float3_t vTexUVMode;
	};

	struct Point
	{
		PointVertex vtx;
		XGIZMO_POINT_MODE mode;
		byte u8Texture;
	};

	struct PointRange
	{
		byte u8Texture;
		UINT uStartVtx;
		UINT uQuadCount;
	};

	Array<Point> m_aPoints;
	Array<PointRange> m_aPointRanges;

	UINT m_uPointCount = 0;

	UINT m_uPointsVBSize = 0;
	static IGXVertexDeclaration *s_pPointsVD;
	IGXRenderBuffer *m_pPointsRB = NULL;
	IGXVertexBuffer *m_pPointsVB = NULL;
	IGXIndexBuffer *m_pPointsIB = NULL;

	UINT m_uTrisVBSize = 0;
	IGXRenderBuffer *m_pTrisRB = NULL;
	IGXVertexBuffer *m_pTrisVB = NULL;

	//IGXConstantBuffer *m_pRightVecCB = NULL;
	static bool s_isShadersLoaded;
	static ID s_idShaders[2][2][2]; // [isTextured][is3D][isFixed]

	Array<PointVertex> m_aPolyVertices;

	//**************************************************

	IXFont *m_pCurrentFont = NULL;
	byte m_u8CurrentFont = 0xFF;
	XGizmoFontParams m_fontParams;

	Array<PointRange> m_aTextRanges;

	struct TextVertex
	{
		float4_t vPosTexUV;
		float4_t vColor;
		float3_t vRefPos;
	};

	Array<TextVertex> m_aTextVertices;

	UINT m_uTextVBSize = 0;
	static IGXVertexDeclaration *s_pTextVD;
	IGXRenderBuffer *m_pTextRB = NULL;
	IGXVertexBuffer *m_pTextVB = NULL;
	IGXIndexBuffer *m_pTextIB = NULL;

	static ID s_idTextShaders[2][2]; // [is3D][isFixed]

private:
	//void prepareLinesIB();
	byte getTextureId();
};

#endif
