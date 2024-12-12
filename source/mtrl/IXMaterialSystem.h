#ifndef __IXMATERIALSYSTEM_H
#define __IXMATERIALSYSTEM_H

#include <gdefines.h>
#include <common/math.h>

#include "IXMaterial.h"

// {7EC7E7F7-E0BE-4CB7-ABFE-9FC0C10880A5}
#define IXMATERIALSYSTEM_GUID DEFINE_XGUID(0x7ec7e7f7, 0xe0be, 0x4cb7, 0xab, 0xfe, 0x9f, 0xc0, 0xc1, 0x8, 0x80, 0xa5)
#define IXMATERIALSYSTEM_VERSION 4

struct XVertexOutputElement
{
	//! Имя переменной
	const char *szName;

	//! Тип
	GXDECLTYPE type;

	//! Семантика
	GXDECLUSAGE usage;
};
#define XVERTEX_OUTPUT_DECL_END() {NULL,GXDECLTYPE_UNUSED,(GXDECLUSAGE)0}

struct XRenderPassTexturesElement
{
	//! Имя текстуры
	const char *szName;

	//! Имя переменной
	const char *szKey;

	//! Слот в шейдере
	UINT uSlot;

	//! Тип текстуры
	GXTEXTURE_TYPE type;
};
#define XRENDER_PASS_TEXTURES_LIST_END() {NULL,NULL,~0u}

struct XRenderPassSamplersElement
{
	//! Имя семплера
	const char *szName;

	//! Имя переменной
	const char *szKey;

	//! Слот в шейдере
	UINT uSlot;
};
#define XRENDER_PASS_SAMPLERS_LIST_END() {NULL,NULL,~0u}

struct XRenderPassOutputElement
{
	//! Имя выхода
	const char *szName;

	//! Имя переменной
	const char *szKey;

	//! Тип
	GXDECLTYPE type;

	//! Значение по умолчанию (hlsl)
	const char *szDefault;
};
#define XRENDER_PASS_OUTPUT_LIST_END() {NULL,NULL,GXDECLTYPE_UNUSED,NULL}

struct XRenderPassVariantElement
{
	GXMacro *pMacro;
};

#include "XMaterialProperty.h"


struct XMaterialShaderSampler
{
	const char *szKey;
	GXSamplerDesc samplerDesc;
};
#define XMATERIAL_SHADER_SAMPLER_LIST_END() {0}

struct XMaterialShaderPass
{
	XRenderPassHandler *pRenderPass;
	const char *szShaderFile;
	const char *szEntryPoint;
	GXMacro *pDefines;
	XMaterialShaderSampler *pSamplers;
	XMaterialProperty *pProperties;
};
#define XMATERIAL_SHADER_PASS_LIST_END() {0}

class IXMaterialSystem: public IXUnknown
{
public:
	/*! Загружает материал, если уже загружен - возвращает имеющийся
	    szDefaultShader - дефолтный шейдер, на случай, если материала не существует (есть только текстура)
	*/
	virtual void XMETHODCALLTYPE loadMaterial(const char *szName, IXMaterial **ppMaterial, const char *szDefaultShader = NULL) = 0;

	//! Запрашивает материал, если не существует - вернет false
	virtual bool XMETHODCALLTYPE getMaterial(const char *szName, IXMaterial **ppMaterial) = 0;

	//! Загружает текстуру, если уже загружен - возвращает имеющийся
	virtual bool XMETHODCALLTYPE loadTexture(const char *szName, IXTexture **ppTexture) = 0;

	//! Запрашивает текстуру, если не существует - вернет false
	virtual bool XMETHODCALLTYPE getTexture(const char *szName, IXTexture **ppTexture) = 0;

	//! Установить материал для отрисовки
	virtual bool XMETHODCALLTYPE bindMaterial(IXMaterial *pMaterial) = 0;

	//! Установить мировую матрицу для отрисовки
	virtual void XMETHODCALLTYPE setWorld(const SMMATRIX &mWorld) = 0;

	//! Установить текстуру в слот
	virtual void XMETHODCALLTYPE bindTexture(IXTexture *pTexture, UINT slot = 0) = 0;

	virtual XVertexFormatHandler* XMETHODCALLTYPE registerVertexFormat(const char *szName, XVertexOutputElement *pDecl) = 0;
	virtual void XMETHODCALLTYPE unregisterVertexFormat(const char *szName) = 0;
	virtual XVertexFormatHandler* XMETHODCALLTYPE getVertexFormat(const char *szName) = 0;

	virtual XVertexShaderHandler* XMETHODCALLTYPE registerVertexShader(XVertexFormatHandler *pVertexFormat, const char *szShaderFile, GXMacro *pDefines = NULL) = 0;
	virtual void XMETHODCALLTYPE bindVS(XVertexShaderHandler *pVertexShader) = 0;

	virtual XGeometryShaderHandler* XMETHODCALLTYPE registerGeometryShader(const char *szShaderFile, const char **aszRequiredParameters, GXMacro *pDefines = NULL) = 0;
	virtual void XMETHODCALLTYPE bindGS(XGeometryShaderHandler *pGeometryShader) = 0;

	// ["GBuffer color(rgb) light(a)", "g_txGBufferC3L1", 0],
	// ["Scene default", "g_sScene", 0]
	// ["Base color", "vBaseColor", 'GXDECLTYPE_FLOAT4', "float4(1.0f, 0.0f, 0.0f, 0.5f)"],
	virtual XRenderPassHandler* XMETHODCALLTYPE registerRenderPass(const char *szName, const char *szShaderFile, XRenderPassTexturesElement *pTextures, XRenderPassSamplersElement *pSamplers, XRenderPassOutputElement *pOutput, XRenderPassVariantElement *pVariants = NULL, bool bSkipMaterialShader = false) = 0;
	virtual XRenderPassHandler* XMETHODCALLTYPE getRenderPass(const char *szName) = 0;
	virtual void XMETHODCALLTYPE bindRenderPass(XRenderPassHandler *pRenderPass, UINT uVariant = 0) = 0;

	virtual XMaterialShaderHandler* XMETHODCALLTYPE registerMaterialShader(const char *szName, XVertexFormatHandler *pVertexFormat, XMaterialShaderPass *pPasses, XMaterialProperty *pGenericProperties) = 0;
	virtual XMaterialShaderHandler* XMETHODCALLTYPE getMaterialShader(const char *szName) = 0;

	virtual void XMETHODCALLTYPE setFlag(const char *szFlag, bool isSet) = 0;
	virtual bool XMETHODCALLTYPE getFlag(const char *szFlag) = 0;
	virtual IMaterialFlag* XMETHODCALLTYPE getFlagHandler(const char *szFlag) = 0;

	//! Перезагружает все материалы
	virtual void XMETHODCALLTYPE reloadAll() = 0;

	//! Сканирует директории на наличие материалов и формирует кэш
	virtual void XMETHODCALLTYPE scanMaterials() = 0;

	virtual UINT XMETHODCALLTYPE getScannedMaterialsCount() = 0;
	virtual const char* XMETHODCALLTYPE getScannedMaterial(UINT uIdx, IXMaterial **ppOut, bool *pisTexture = NULL, bool *pisTranslated = NULL, bool *pisMaterial = NULL) = 0;

	virtual bool XMETHODCALLTYPE isMaterialLoaded(const char *szName) = 0;

	//! Добавить текстуру
	virtual void XMETHODCALLTYPE addTexture(const char *szName, IGXBaseTexture *pGXTexture, IXTexture **ppTexture) = 0;

	//! Установить режим заполнения
	virtual void XMETHODCALLTYPE setFillMode(GXFILL_MODE fillMode) = 0;
	virtual GXFILL_MODE XMETHODCALLTYPE getFillMode() const = 0;

	//! Установить режим отсечения
	virtual void XMETHODCALLTYPE setCullMode(GXCULL_MODE cullMode = GXCULL_BACK) = 0;
	virtual GXCULL_MODE XMETHODCALLTYPE getCullMode() const = 0;

	//! Получить количество свойств шейдера материала
	virtual UINT XMETHODCALLTYPE getMaterialPropertyCount(IXMaterial *pMat) const = 0;
	//! Получить свойства шейдера материала. Буфер pOut должен быть достаточного размера, получить размер можно с помощью getMaterialPropertyCount
	virtual UINT XMETHODCALLTYPE getMaterialProperties(XMaterialProperty *pOut, IXMaterial *pMat, bool bSkipInactive = false) const = 0;

	//! Получить итератор материал-шейдеров
	virtual void XMETHODCALLTYPE getMaterialShadersIterator(IKeyIterator **ppOut) = 0;

	//! Проверка возможности использования имени материала
	virtual bool XMETHODCALLTYPE testMaterialName(const char *szName) = 0;
};

#endif
