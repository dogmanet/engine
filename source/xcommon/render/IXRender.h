#ifndef __IXRENDER_H
#define __IXRENDER_H

#include <gdefines.h>
#include <graphix/graphix.h>
//#include <xcommon/IXRenderable.h>
#include <xcommon/IXCamera.h>
#include "IXRenderUtils.h"
#include <mtrl/IXTexture.h>

//! цвет очистки цветового буфера сцены по умолчанию
#define XRENDER_DEFAULT_BACKGROUND_COLOR GX_COLOR_COLOR_TO_F4(GX_COLOR_ARGB(0, 128, 128, 128))

enum SHADER_CONST_REGISTER
{
	SCR_OBSERVER_CAMERA = 5,
	SCR_FRAME = 4,
	//SCR_SCENE = 3,
	SCR_CAMERA = 2,
	SCR_OBJECT = 1,
	SCR_SUBSET = 0,

	SCR_XMATERIAL = 7
};

//! Стадии рендера
enum X_RENDER_STAGE
{
	XRS_PREPARE = 0x00000001, //!< Подготовка, здесь можно обновить необходимые рендертаргеты и прочее
	XRS_GBUFFER = 0x00000002, //!< Рендер в ГБуфер
	XRS_SHADOWS = 0x00000004, //!< Рендер теней
	XRS_GI = 0x00000008, //!< Рендер глобального освещения (?)
	XRS_POSTPROCESS_MAIN = 0x00000010, //!< Основной постпроцесс (рисуется до прозрачных материалов)
	XRS_TRANSPARENT = 0x00000020, //!< Прозрачные материалы
	XRS_POSTPROCESS_FINAL = 0x00000040, //!< Финальный постпроцесс

	XRS_EDITOR_2D = 0x00000080  //!< Рендер для редактора в виде 2D
};

DEFINE_ENUM_FLAG_OPERATORS(X_RENDER_STAGE);

/*! \defgroup sxgcore_ds Render targets для deferred shading
\ingroup sxgcore
@{*/

//! типы render targets
enum DS_RT
{
	//! цвет rgba8
	DS_RT_COLOR = 0,

	//! нормали (включая микрорельеф) rfb10a2
	DS_RT_NORMAL,

	//! параметры освещения rgba8
	DS_RT_PARAM,

	//! глубина r16f, непрозрачные материалы
	DS_RT_DEPTH,

	//! глубина r16f, непрозрачные материалы
	DS_RT_DEPTH0,

	//! глубина r16f, непрозрачные и полупрозрачные пиксели, однако прозрачные (если есть) будут перекрывать непрозрачные, и в этом случае их глубина будет 1 то есть максимальной(для идентификации)
	DS_RT_DEPTH1,


	//! эмбиент цвет (цвет света rgb) и диффузное освещение (a) rgba16f
	DS_RT_AMBIENTDIFF,

	//! яркость
	DS_RT_LUMINANCE,

	//! освещенная сцена rgba16
	DS_RT_SCENELIGHT,

	//! освещенная сцена rgba16
	DS_RT_SCENELIGHT2,

	//! текущий rt адаптации глаза к освещению r16f
	DS_RT_ADAPTEDLUM
};

//!@} sxgcore_ds


//! типы шейдеров (int type_shader)
enum SHADER_TYPE
{
	//! вершинный
	SHADER_TYPE_VERTEX,

	//! пиксельный
	SHADER_TYPE_PIXEL,

	//! геометрический
	SHADER_TYPE_GEOMETRY,

	//! вычислительный
	SHADER_TYPE_COMPUTE
};

class IXRender;
class IXRenderGraphNode;
class IXRenderableVisibility;
class IXMaterial;
class IXRenderGraph;

// {EE63CD9C-7EDD-4E25-8E96-286E54252114}
#define IXRENDER_GUID DEFINE_XGUID(0xee63cd9c, 0x7edd, 0x4e25, 0x8e, 0x96, 0x28, 0x6e, 0x54, 0x25, 0x21, 0x14)
#define IXRENDER_VERSION 1

class IXRenderGraphNodeData: public IXUnknown
{
public:
	virtual void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) = 0;
};

class IXRenderTarget: public IXUnknown
{
public:
	virtual void XMETHODCALLTYPE resize(UINT uWidth, UINT uHeight) = 0;

	virtual void XMETHODCALLTYPE attachGraph(IXRenderGraph *pGraph) = 0;

	virtual bool XMETHODCALLTYPE getTarget(IGXSurface **ppTarget) = 0;

	virtual void XMETHODCALLTYPE setCamera(IXCamera *pCamera) = 0;
	virtual void XMETHODCALLTYPE getCamera(IXCamera **ppCamera) = 0;

	virtual void XMETHODCALLTYPE getSize(UINT *puWidth, UINT *puHeight) const = 0;
};

class IXRenderGraphNodeInstance: public IXUnknown
{
public:
	virtual IXRenderGraphNode* XMETHODCALLTYPE getNode() = 0;
	virtual bool XMETHODCALLTYPE getNodeData(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData **ppOut) = 0;
	
	virtual void XMETHODCALLTYPE setTexture(IXRenderTarget *pFinalTarget, UINT uIdx, IXTexture *pTexture) = 0;
	virtual void XMETHODCALLTYPE setProp(IXRenderTarget *pFinalTarget, UINT uIdx, const float4_t &vProp) = 0;
	virtual void XMETHODCALLTYPE setFlag(IXRenderTarget *pFinalTarget, UINT uIdx, bool bVal) = 0;
};

class IXRenderGraph: public IXUnknown
{
public:
	virtual bool XMETHODCALLTYPE getNodeInstance(const char *szNodeInstance, IXRenderGraphNodeInstance **ppNodeInstance) = 0;
};

// {3F2FD273-F043-4395-882F-C68EDF4B86E9}
#define IXRENDERGRAPHNODE_GUID DEFINE_XGUID(0x3f2fd273, 0xf043, 0x4395, 0x88, 0x2f, 0xc6, 0x8e, 0xdf, 0x4b, 0x86, 0xe9)
#define IXRENDERGRAPHNODE_VERSION 1

enum XRENDER_PIPELINE_EXTENSION_TYPE
{
	XRPET_PRE,
	XRPET_POST
};

struct XRenderPipelineExtensionDesc
{
	XRENDER_PIPELINE_EXTENSION_TYPE type;
	UINT uPriority;
	const char *szTargetNodeName;
	void *pReserved = NULL; // must be NULL
};

struct XRenderGraphNodeInputDesc
{
	const char *szName;
	GXTEXTURE_TYPE type;
	GXFORMAT format;
	bool isPassthrough;
	bool isRequired;
	bool isOptional;
	void *pReserved; // must be NULL
};

struct XRenderGraphNodeOutputDesc
{
	const char *szName;
	GXTEXTURE_TYPE type;
	GXFORMAT format;
	void *pReserved; // must be NULL
};

struct XRenderGraphNodeTextureDesc
{
	const char *szName;
	GXTEXTURE_TYPE type;
	void *pReserved; // must be NULL
};

struct XRenderGraphNodePropDesc
{
	const char *szName;
	GXDECLTYPE type;
	void *pReserved = NULL; // must be NULL
};

struct XRenderGraphNodeFlagDesc
{
	const char *szName;
	void *pReserved = NULL; // must be NULL
};

class IXRenderGraphNode: public IXUnknown
{
public:
	virtual const char* XMETHODCALLTYPE getName() const = 0;
	
	virtual bool XMETHODCALLTYPE isExtension() const = 0;
	virtual XRenderPipelineExtensionDesc* XMETHODCALLTYPE getExtensionDesc() = 0;

	virtual void XMETHODCALLTYPE startup(IXRender *pRender) = 0;
	//virtual void XMETHODCALLTYPE shutdown() = 0;

	virtual void XMETHODCALLTYPE newData(IXRenderGraphNodeData **ppData) = 0;
	virtual void XMETHODCALLTYPE newOutputTarget(UINT uOutput, UINT uWidth, UINT uHeight, IGXBaseTexture **ppTarget) = 0;

	virtual void XMETHODCALLTYPE updateVisibility(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData *pData) = 0;

	virtual void XMETHODCALLTYPE process(float fDeltaTime, IXRenderTarget *pFinalTarget, IGXBaseTexture **ppInputs, UINT uInputCount, IXRenderGraphNodeData *pData, IGXSurface **ppOutputs, UINT uOutputCount) = 0;

	virtual const XRenderGraphNodeInputDesc* XMETHODCALLTYPE getInputsDesc(UINT *puCount) = 0;
	virtual const XRenderGraphNodeOutputDesc* XMETHODCALLTYPE getOutputsDesc(UINT *puCount) = 0;

	virtual const XRenderGraphNodeTextureDesc* XMETHODCALLTYPE getTexturesDesc(UINT *puCount) = 0;
	virtual const XRenderGraphNodePropDesc* XMETHODCALLTYPE getPropsDesc(UINT *puCount) = 0;
	virtual const XRenderGraphNodeFlagDesc* XMETHODCALLTYPE getFlagsDesc(UINT *puCount) = 0;

	virtual void XMETHODCALLTYPE setTexture(IXRenderGraphNodeData *pNodeData, UINT uIdx, IXTexture *pTexture) = 0;
	virtual void XMETHODCALLTYPE setProp(IXRenderGraphNodeData *pNodeData, UINT uIdx, const float4_t &vProp) = 0;
	virtual void XMETHODCALLTYPE setFlag(IXRenderGraphNodeData *pNodeData, UINT uIdx, bool bVal) = 0;
};

struct XTransparentObject
{
	bool hasPSP;
	SMPLANE psp;
	float3 vMin;
	float3 vMax;
	IXMaterial *pMaterial;
};

struct XRenderTransparentObject
{
	XTransparentObject obj;
	UINT uRenderable;
	UINT uObjectID;
};

//! 
class IXRender: public IXUnknown
{
public:
	//! 
	virtual bool XMETHODCALLTYPE newFinalTarget(SXWINDOW hWnd, const char *szName, IXRenderTarget **ppOut) = 0;
	
	virtual bool XMETHODCALLTYPE getFinalTarget(const char *szName, IXRenderTarget **ppOut) = 0;

	virtual bool XMETHODCALLTYPE getGraphNode(const char *szName, IXRenderGraphNode **ppOut) = 0;

	virtual bool XMETHODCALLTYPE getGraph(const char *szName, IXRenderGraph **ppOut) = 0;

	virtual void XMETHODCALLTYPE newVisData(IXRenderableVisibility **ppVisibility) = 0;

	virtual IGXDevice* XMETHODCALLTYPE getDevice() = 0;

	virtual void XMETHODCALLTYPE renderStage(X_RENDER_STAGE stage, IXRenderableVisibility *pVisibility = NULL) = 0;
	
	virtual void XMETHODCALLTYPE setSceneFilter(IGXContext *pCtx, UINT uSlot = 0) = 0;

	virtual void XMETHODCALLTYPE drawScreenQuad(IGXContext *pCtx, IXRenderTarget *pFinalTarget = NULL) = 0;

	virtual void XMETHODCALLTYPE bindShader(IGXContext *pCtx, ID idShaderKit) = 0;
	virtual void XMETHODCALLTYPE unbindShader(IGXContext *pCtx) = 0;

	virtual ID XMETHODCALLTYPE loadShader(SHADER_TYPE type_shader, const char *szPath, GXMacro *pMacro = NULL) = 0;
	virtual ID XMETHODCALLTYPE createShaderKit(ID idVertexShader, ID idPixelShader, ID idGeometryShader = -1, ID idComputeShader = -1) = 0;

	virtual void XMETHODCALLTYPE newCamera(IXCamera **ppCamera) = 0;
	virtual void XMETHODCALLTYPE newFrustum(IXFrustum **ppFrustum) = 0;

	virtual UINT XMETHODCALLTYPE getTransparentObjectsCount(IXRenderableVisibility *pVisibility) = 0;
	virtual void XMETHODCALLTYPE getTransparentObjects(IXRenderableVisibility *pVisibility, XRenderTransparentObject *pObjects) = 0;

	virtual void XMETHODCALLTYPE renderTransparentObject(IXRenderableVisibility *pVisibility, XRenderTransparentObject *pObject, UINT uSplitPlanes) = 0;

	virtual void XMETHODCALLTYPE renderFrame(float fDeltaTime) = 0;
	virtual void XMETHODCALLTYPE endFrame() = 0;
	virtual void XMETHODCALLTYPE updateVisibility() = 0;

	virtual IXRenderUtils* XMETHODCALLTYPE getUtils() = 0;

	virtual const GXModeDesc* XMETHODCALLTYPE getModes(UINT *puCount, IXRenderTarget *pTarget = NULL) = 0;

	virtual void XMETHODCALLTYPE initGraphics(void *pReserved = NULL) = 0;
};

#endif
