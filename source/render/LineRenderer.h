#ifndef __LINERENDERER_H
#define __LINERENDERER_H

#include <xcommon/render/IXRenderUtils.h>
#include <xcommon/render/IXRender.h>

class CLineRenderer final: public IXUnknownImplementation<IXLineRenderer>
{
public:
	CLineRenderer(IXRender *pRender);
	~CLineRenderer();

	void XMETHODCALLTYPE reset() override;
	void XMETHODCALLTYPE render(bool isOrtho, bool useConstantSize = true, bool useDepthTest = true) override;

	void XMETHODCALLTYPE setWidth(float fWidth) override;

	void XMETHODCALLTYPE setColor(const float4 &vColor) override;
	void XMETHODCALLTYPE setTexture(IGXBaseTexture *pTexture) override;

	void XMETHODCALLTYPE jumpTo(const float3 &vPos) override;
	void XMETHODCALLTYPE lineTo(const float3 &vPos) override;

private:
	IXRender *m_pRender;
	IGXDevice *m_pDev;

	static std::atomic_uint s_uResRefCount;
	static IGXBlendState *s_pBlendAlpha;
	static IGXDepthStencilState *s_pDSState3D;
	static IGXDepthStencilState *s_pDSState2D;
	static IGXDepthStencilState *s_pDSStateNoZ;

	IGXBaseTexture *m_pCurrentTexture = NULL;
	byte m_u8CurrentTexture = 0xFF;
	bool m_isCurrentTextureDirty = false;
	float4_t m_vCurrentColor = float4_t(0.0f, 0.0f, 0.0f, 1.0f);
	float4_t m_vLastPointColor = float4_t(0.0f, 0.0f, 0.0f, 1.0f);
	float m_fCurrentLineWidth = 1.0f;

	struct LineVertex
	{
		float4_t vPosWidth;
		float4_t vColor;
		float3_t vDir;
		float2_t vTexUV;
	};

	struct LinePoint
	{
		LineVertex vtx;
		byte u8Texture;
		byte isStart;
	};

	struct LineRange
	{
		byte u8Texture;
		UINT uStartVtx;
		UINT uTriangleCount;
	};

	Array<LineRange> m_aLineRanges;
	Array<IGXBaseTexture*> m_aTextures;
	Array<LinePoint> m_aVertices;
	float3_t m_vNextLineStart;
	bool m_isLineStart = true;

	UINT m_uLineCount = 0;
	UINT m_uLineSegmentCount = 0;

	UINT m_uLinesVBSize = 0;
	static IGXVertexDeclaration *s_pLinesVD;
	IGXRenderBuffer *m_pLinesRB = NULL;
	IGXVertexBuffer *m_pLinesVB = NULL;
	bool m_isLinesDirty = false;

	static bool s_isShadersLoaded;
	static ID s_idShaders[2][2][2][2]; // [isTextured][is3D][isFixed][isBackward]

private:
	byte getTextureId();
};

#endif
