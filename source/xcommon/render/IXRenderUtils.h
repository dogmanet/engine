#ifndef __IXRENDERUTILS_H
#define __IXRENDERUTILS_H

#include <gdefines.h>
#include <graphix/graphix.h>
#include <xcommon/gui/IXFontManager.h>

enum XGIZMO_POINT_MODE
{
	XGPM_SQUARE,
	XGPM_ROUND
};

enum XGIZMO_TEXT_VERTICAL_ALIGN
{
	XGTVA_TOP,
	XGTVA_BOTTOM,
	XGTVA_MIDDLE
};

struct XGizmoFontParams
{
	XFONT_DECORATION decoration = XFONT_DECORATION_NONE;
	XFONT_TEXT_ALIGN textAlign = XFONT_TEXT_ALIGN_LEFT;
	XGIZMO_TEXT_VERTICAL_ALIGN verticalAlign = XGTVA_TOP;
	UINT uFirstLineShift = 0;
};

//! 
class IXGizmoRenderer: public IXUnknown
{
public:
	//! 
	virtual void XMETHODCALLTYPE reset() = 0;
	//! 
	virtual void XMETHODCALLTYPE render(bool isOrtho, bool useConstantSize = true, bool useDepthTest = true) = 0;

	//! 
	virtual void XMETHODCALLTYPE setLineWidth(float fWidth) = 0;

	//! 
	virtual void XMETHODCALLTYPE setColor(const float4 &vColor) = 0;

	//! 
	virtual void XMETHODCALLTYPE setTexture(IGXBaseTexture *pTexture) = 0;

	//! 
	virtual void XMETHODCALLTYPE jumpTo(const float3 &vPos) = 0;
	//! 
	virtual void XMETHODCALLTYPE lineTo(const float3 &vPos) = 0;


	//! 
	virtual void XMETHODCALLTYPE setPointSize(float fWidth) = 0;
	//! 
	virtual void XMETHODCALLTYPE setPointMode(XGIZMO_POINT_MODE pointMode) = 0;
	//! 
	virtual void XMETHODCALLTYPE drawPoint(const float3 &vPos) = 0;

	//! 
	virtual void XMETHODCALLTYPE drawAABB(const SMAABB &aabb) = 0;

	//! 
	virtual void XMETHODCALLTYPE drawEllipsoid(const float3 &vPos, const float3 &vSize) = 0;


	//! 
	virtual void XMETHODCALLTYPE drawPoly(
		const float3_t &vPosA,
		const float3_t &vPosB,
		const float3_t &vPosC) = 0;
	virtual void XMETHODCALLTYPE drawPoly(
		const float3_t &vPosA,
		const float3_t &vPosB,
		const float3_t &vPosC,
		const float4_t &vColorA,
		const float4_t &vColorB,
		const float4_t &vColorC) = 0;

	//! 
	virtual void XMETHODCALLTYPE setFont(IXFont *pFont) = 0;

	//! 
	virtual void XMETHODCALLTYPE setFontParams(const XGizmoFontParams &params) = 0;

	//! 
	virtual void XMETHODCALLTYPE drawString(
		const char *szText,
		const float3_t &vRefPoint,
		UINT uAreaWidth = 0
	) = 0;
};

//#############################################################################

//! 
class IXLineRenderer: public IXUnknown
{
public:
	//! 
	virtual void XMETHODCALLTYPE reset() = 0;
	//! 
	virtual void XMETHODCALLTYPE render(bool isOrtho, bool useConstantSize = true, bool useDepthTest = true) = 0;

	//! 
	virtual void XMETHODCALLTYPE setWidth(float fWidth) = 0;

	//! 
	virtual void XMETHODCALLTYPE setColor(const float4 &vColor) = 0;

	//! 
	virtual void XMETHODCALLTYPE setTexture(IGXBaseTexture *pTexture) = 0;

	//! 
	virtual void XMETHODCALLTYPE jumpTo(const float3 &vPos) = 0;
	//! 
	virtual void XMETHODCALLTYPE lineTo(const float3 &vPos) = 0;
};

//#############################################################################

/*! ћеш
*/
class IXMesh: public IXUnknown
{
public:
	virtual void XMETHODCALLTYPE draw(IGXContext *pCtx = NULL) = 0;
	virtual IGXVertexBuffer* XMETHODCALLTYPE getVertexBuffer() = 0;
	virtual IGXIndexBuffer* XMETHODCALLTYPE getIndexBuffer() = 0;
};

//#############################################################################

// {D4025713-43EC-4A55-8989-0639379F981F}
#define IXRENDERUTILS_GUID DEFINE_XGUID(0xd4025713, 0x43ec, 0x4a55, 0x89, 0x89, 0x6, 0x39, 0x37, 0x9f, 0x98, 0x1f)
#define IXRENDERUTILS_VERSION 1

//! 
class IXRenderUtils: public IXUnknown
{
public:
	//! 
	virtual void XMETHODCALLTYPE newGizmoRenderer(IXGizmoRenderer **ppOut) = 0;
	virtual void XMETHODCALLTYPE newLineRenderer(IXLineRenderer **ppOut) = 0;

	virtual void XMETHODCALLTYPE getQuadIndexBuffer(UINT uQuads, IGXIndexBuffer **ppIndexBuffer) = 0;

	//! создание меша (ID3DXMesh) конуса 
	virtual void XMETHODCALLTYPE createConeMesh(
		float fTopRadius,       //!< верхний радиус
		float fBottomRadius,    //!< нижний радиус
		float fHeight,          //!< длина
		IXMesh **ppMesh,        //!< выходной меш
		UINT uSideCount         //!< количество боковых сторон
		) = 0;

	//! создание меша (ID3DXMesh) сферы 
	virtual void XMETHODCALLTYPE createSphereMesh(
		float fRadius,      //!< радиус
		UINT uSideCount,    //!< количество боковых сторон
		UINT uStackCount,   //!< количество горизонтальных слоев
		IXMesh **ppMesh     //!< выходной меш
		) = 0;

	//! создание меша (ID3DXMesh) ограничивающего объема
	virtual void XMETHODCALLTYPE createBoundingBoxMesh(const float3 &vMin, const float3 &vMax, IXMesh **ppMesh) = 0;
};


#endif
