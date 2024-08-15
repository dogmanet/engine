#ifndef __MATERIALSYSTEM_H
#define __MATERIALSYSTEM_H

#include "IXMaterialSystem.h"
#include <common/string.h>
#include <xcommon/IXTextureProxy.h>
#include <xcommon/IXMaterialProxy.h>
#include <xcommon/IXMaterialLoader.h>
#include <xcommon/resource/IXResourceManager.h>
#include <xcommon/resource/IXResourceTexture.h>
#include <xcommon/IXTextureFilter.h>
#include <xcommon/XEvents.h>
#include <xcommon/IXCore.h>
#include <xcommon/render/IXRender.h>
#include <common/ConcurrentQueue.h>
#include <common/aastring.h>

#include "LogicExpression.h"

class CMaterialSystem;
class CTexture: public IXUnknownImplementation<IXTexture>
{
public:
	CTexture(CMaterialSystem *pMaterialSystem, IXResourceTexture *pResource);
	CTexture(CMaterialSystem *pMaterialSystem, IGXBaseTexture *pGXTexture);
	~CTexture();
	void XMETHODCALLTYPE getAPITexture(IGXBaseTexture **ppTexture, UINT uFrame = 0) override;
	bool XMETHODCALLTYPE isReady() const override;

	UINT XMETHODCALLTYPE getNumFrames() const override;
	float XMETHODCALLTYPE getFrameTime() const override;

	GXTEXTURE_TYPE XMETHODCALLTYPE getType() const override;

	UINT XMETHODCALLTYPE getWidth() const override;
	UINT XMETHODCALLTYPE getHeight() const override;
	UINT XMETHODCALLTYPE getDepth() const override;

	void setName(const char *szName);
	const char* getName() const;

	void replace(IGXBaseTexture *pGXTexture);

	void initGPUresources();
protected:
	CMaterialSystem *m_pMaterialSystem;
	IXResourceTexture *m_pResource = NULL;

	IGXBaseTexture **m_ppGXTexture = NULL;
	const char *m_szName = NULL;
	UINT m_uFrameCount = 0;
	float m_fFrameTime = 0.0f;

	GXTEXTURE_TYPE m_type = GXTEXTURE_TYPE_UNKNOWN;

	UINT m_uWidth = 1;
	UINT m_uHeight = 1;
	UINT m_uDepth = 1;

private:
	void XMETHODCALLTYPE FinalRelease() override;
};

class CMaterial;
class CMaterialProperty: public IMaterialProperty
{
public:
	CMaterialProperty() = default;
	CMaterialProperty(CMaterial *pMaterial, const char *szName, XEventMaterialChanged::TYPE typeNotifier = XEventMaterialChanged::TYPE_PROPERTY);

	const char* XMETHODCALLTYPE getName() const override;

	void XMETHODCALLTYPE set(const float4_t &vValue) override;
	float4_t XMETHODCALLTYPE get() const override;

	void setOffset(UINT uOffset, UINT uSize);
	void setDefault(const float4_t &vValue);

	void preparePass(UINT uPass);

protected:

	void setValue();

private:
	CMaterial *m_pMaterial = NULL;
	const char *m_szName = NULL;
	XEventMaterialChanged::TYPE m_typeNotifier = XEventMaterialChanged::TYPE_PROPERTY;

	float4_t m_vValue = float4_t(0.0f, 0.0f, 0.0f, 0.0f);
	bool m_isSet = false;

	struct MaterialPass
	{
		UINT uOffset = UINT_MAX;
		UINT uSize = 4;
	};

	Array<MaterialPass> m_aPassCache;
	MaterialPass *m_pCurrentPass = NULL;
};

class CMaterialFlag: public IMaterialFlag
{
public:
	CMaterialFlag() = default;
	CMaterialFlag(CMaterial *pMaterial, const char *szName, XEventMaterialChanged::TYPE typeNotifier = XEventMaterialChanged::TYPE_FLAG);

	const char* XMETHODCALLTYPE getName() const override;

	void XMETHODCALLTYPE set(bool bValue) override;
	bool XMETHODCALLTYPE get() const override;

	void clearStatic();
	void setStatic(bool bValue);

private:
	CMaterial *m_pMaterial = NULL;
	const char *m_szName = NULL;
	XEventMaterialChanged::TYPE m_typeNotifier = XEventMaterialChanged::TYPE_FLAG;

	bool m_bValue = false;
	bool m_isStatic = false;
};

class CGlobalFlag: public CMaterialFlag
{
	DECLARE_CLASS(CGlobalFlag, CMaterialFlag);

public:
	CGlobalFlag() = default;
	CGlobalFlag(CMaterialSystem *pMaterialSystem, const char *szName);

	void XMETHODCALLTYPE set(bool bValue) override;

private:
	CMaterialSystem *m_pMaterialSystem = NULL;
};

class CMaterial: public IXUnknownImplementation<IXMaterial>
{
public:
	friend class CMaterialProperty;
	friend class CMaterialFlag;
	// friend class CMaterialSystem;

	CMaterial(CMaterialSystem *pMaterialSystem, const char *szName);
	~CMaterial();

	const char* XMETHODCALLTYPE getName() const override;
	//void XMETHODCALLTYPE getMainTexture(IXTexture **ppTexture) override;
	void XMETHODCALLTYPE setTransparent(bool bValue) override;
	bool XMETHODCALLTYPE isTransparent() const override;

	void XMETHODCALLTYPE setRefractive(bool bValue) override;
	bool XMETHODCALLTYPE isRefractive() const override;

	void XMETHODCALLTYPE setBlurred(bool bValue) override;
	bool XMETHODCALLTYPE isBlurred() const override;

	void XMETHODCALLTYPE setEmissive(bool bValue) override;
	bool XMETHODCALLTYPE isEmissive() const override;

	void XMETHODCALLTYPE setShader(const char *szShader) override;
	const char* XMETHODCALLTYPE getShader() const override;
	XMaterialShaderHandler* XMETHODCALLTYPE getShaderHandler() const override;

	void XMETHODCALLTYPE setFlag(const char *szFlag, bool isSet) override;
	bool XMETHODCALLTYPE getFlag(const char *szFlag) override;
	IMaterialFlag* XMETHODCALLTYPE getFlagHandler(const char *szFlag) override;
	IKeyIterator* XMETHODCALLTYPE getFlagsIterator() override;

	void XMETHODCALLTYPE setParam(const char *szParam, const float4_t &fValue) override;
	float4_t XMETHODCALLTYPE getParam(const char *szParam) override;
	IMaterialProperty* XMETHODCALLTYPE getParamHandler(const char *szParam) override;
	IKeyIterator* XMETHODCALLTYPE getParamsIterator() override;

	void XMETHODCALLTYPE setTexture(const char *szKey, const char *szTexture) override;
	const char* XMETHODCALLTYPE getTextureName(const char *szKey) const override;
	IXTexture* XMETHODCALLTYPE getTexture(const char *szKey) const override;
	IKeyIterator* XMETHODCALLTYPE getTexturesIterator() override;

	void XMETHODCALLTYPE setPhysicsType(MTLTYPE_PHYSIC type) override;
	MTLTYPE_PHYSIC XMETHODCALLTYPE getPhysicsType() const override;

	void XMETHODCALLTYPE setDurablility(float fValue) override;
	float XMETHODCALLTYPE getDurablility() const override;

	void XMETHODCALLTYPE setHitChance(float fValue) override;
	float XMETHODCALLTYPE getHitChance() const override;

	void XMETHODCALLTYPE setDensity(float fValue) override;
	float XMETHODCALLTYPE getDensity() const override;

	bool XMETHODCALLTYPE save() override;
	

	void XMETHODCALLTYPE setEditorial(bool bValue) override;
	bool XMETHODCALLTYPE isEditorial() const override;

	void XMETHODCALLTYPE setTwoSided(bool bValue) override;
	bool XMETHODCALLTYPE isTwoSided() const override;

	bool isDirty() const
	{
		return(m_pCurrentPass->isDirty);
	}

	UINT getCachedVariant() const
	{
		return(m_pCurrentPass->uVariant);
	}

	void setCachedVariant(UINT uVariant)
	{
		m_pCurrentPass->uVariant = uVariant;
		m_pCurrentPass->isDirty = false;
	}

	IXTexture* getTextureForSlot(UINT uSlot);
	void clearTextureBindings();
	void bindTextureToSlot(const char *szTexture, UINT uSlot);

	IGXConstantBuffer* getConstants();
	void initConstantsBindings(UINT uSize);
	void bindConstantToOffset(const char *szConstant, UINT uOffset, UINT uSize, const float4_t &vDefault);
	void setConstant(UINT uPass, UINT uOffset, const void *pValue, UINT uSize);

	void preparePass(UINT uPass);

	void clearStaticFlags();
	void setStaticFlag(const char *szName, bool bValue);

	void onGlobalFlagChanged(CGlobalFlag *pFlag);

	void setProxy(IXMaterialProxy *pProxy);
	IXMaterialProxy* getProxy();
	
protected:
	CMaterialSystem *m_pMaterialSystem = NULL;

	IXMaterialLoader *m_pLoader = NULL;
	IXMaterialProxy *m_pProxy = NULL;

	void notifyChanged(XEventMaterialChanged::TYPE type, const char *szReference = NULL);
	
	CMaterialFlag* createFlag(const char *szName, XEventMaterialChanged::TYPE type = XEventMaterialChanged::TYPE_FLAG);
	CMaterialProperty* createProperty(const char *szName, XEventMaterialChanged::TYPE type = XEventMaterialChanged::TYPE_PROPERTY);

	void updateShader();

protected:
	struct MaterialTexture
	{
		String sName;
		IXTexture *pTexture = NULL;
		// UINT uSlot = UINT_MAX;
	};
	AssotiativeArray<AAString, MaterialTexture> m_mapTextures;

private:
	const char *m_szName = NULL;


	String m_sShader;
	XMaterialShaderHandler *m_pShader = NULL;

	AssotiativeArray<AAString, CMaterialProperty> m_mapProperties;
	AssotiativeArray<AAString, CMaterialFlag> m_mapFlags;
	Array<CMaterialProperty*> m_aProperties;

	struct MaterialPass
	{
		Array<MaterialTexture*> aTextureMap;
		bool isDirty = true;
		UINT uVariant = 0;
		IGXConstantBuffer *pConstants = NULL;
		byte *pConstantsBlob = NULL;
		UINT uConstantSize = 0;
		bool isConstantsDirty = true;
	};

	Array<MaterialPass> m_aPassCache;
	MaterialPass *m_pCurrentPass = NULL;
	UINT m_uCurrentPass = UINT_MAX;

	//! тип физического материала
	MTLTYPE_PHYSIC m_typePhysics = MTLTYPE_PHYSIC_DEFAULT;

	//! коэффициент пробиваемости [0, ], чем больше тем сложнее пробить
	CMaterialProperty *m_pDurability = NULL;

	//! шанс пробиваемости [0 - пуля летит насквозь, 1 - пуля ударяется]
	CMaterialProperty *m_pHitChance = NULL;

	//! плотность материала кг/м3
	CMaterialProperty *m_pDensity = NULL;

	//! прозрачный ли материал
	CMaterialFlag *m_pTransparent = NULL;
	CMaterialFlag *m_pRefractive = NULL;
	CMaterialFlag *m_pBlurred = NULL;
	CMaterialFlag *m_pEmissive = NULL;
	CMaterialFlag *m_pEditorial = NULL;
	CMaterialFlag *m_pTwoSided = NULL;
};

class CMaterialInfo: public CMaterial
{
public:
	CMaterialInfo(CMaterialSystem *pMaterialSystem, const char *szName):
		CMaterial(pMaterialSystem, szName)
	{
	}

	void XMETHODCALLTYPE setTexture(const char *szKey, const char *szTexture) override
	{
		assert(strlen(szKey) < AAS_MAXLEN);

		const AssotiativeArray<AAString, MaterialTexture>::Node *pNode;
		if(m_mapTextures.KeyExists(szKey, &pNode))
		{
			if(!fstrcmp(pNode->Val->sName.c_str(), szTexture))
			{
				return;
			}
			pNode->Val->sName = szTexture;
		}
		else
		{
			AAString sKey;
			sKey.setName(szKey);
			m_mapTextures[sKey].sName = szTexture;
			m_mapTextures[sKey].pTexture = NULL;
		}
	}
};

class IFileSystem;

class CMaterialSystem final: public IXUnknownImplementation<IXMaterialSystem>
{
public:
	CMaterialSystem(IXCore *pCore);
	~CMaterialSystem();

	void XMETHODCALLTYPE loadMaterial(const char *szName, IXMaterial **ppMaterial, const char *szDefaultShader = NULL) override;
	bool XMETHODCALLTYPE getMaterial(const char *szName, IXMaterial **ppMaterial) override;
	void XMETHODCALLTYPE reloadAll() override;
	bool loadMaterial(const char *szName, CMaterial *pMaterial);

	bool XMETHODCALLTYPE loadTexture(const char *szName, IXTexture **ppTexture) override;
	bool XMETHODCALLTYPE getTexture(const char *szName, IXTexture **ppTexture) override;
	//void XMETHODCALLTYPE addTexture(const char *szName, IGXTexture2D *pTexture) override;

	bool XMETHODCALLTYPE bindMaterial(IXMaterial *pMaterial) override;
	void XMETHODCALLTYPE bindTexture(IXTexture *pTexture, UINT slot = 0) override;
	void XMETHODCALLTYPE setWorld(const SMMATRIX &mWorld) override;

	XVertexFormatHandler* XMETHODCALLTYPE registerVertexFormat(const char *szName, XVertexOutputElement *pDecl) override;
	void XMETHODCALLTYPE unregisterVertexFormat(const char *szName) override;
	XVertexFormatHandler* XMETHODCALLTYPE getVertexFormat(const char *szName) override;

	XVertexShaderHandler* XMETHODCALLTYPE registerVertexShader(XVertexFormatHandler *pVertexFormat, const char *szShaderFile, GXMacro *pDefines = NULL) override;
	void XMETHODCALLTYPE bindVS(XVertexShaderHandler *pVertexShader);

	XGeometryShaderHandler* XMETHODCALLTYPE registerGeometryShader(const char *szShaderFile, const char **aszRequiredParameters, GXMacro *pDefines = NULL) override;
	void XMETHODCALLTYPE bindGS(XGeometryShaderHandler *pGeometryShader) override;

	XRenderPassHandler* XMETHODCALLTYPE registerRenderPass(const char *szName, const char *szShaderFile, XRenderPassTexturesElement *pTextures, XRenderPassSamplersElement *pSamplers, XRenderPassOutputElement *pOutput, XRenderPassVariantElement *pVariants = NULL, bool bSkipMaterialShader = false) override;
	XRenderPassHandler* XMETHODCALLTYPE getRenderPass(const char *szName) override;
	void XMETHODCALLTYPE bindRenderPass(XRenderPassHandler *pRenderPass, UINT uVariant = 0) override;

	XMaterialShaderHandler* XMETHODCALLTYPE registerMaterialShader(const char *szName, XVertexFormatHandler *pVertexFormat, XMaterialShaderPass *pPasses, XMaterialProperty *pGenericProperties) override;
	XMaterialShaderHandler* XMETHODCALLTYPE getMaterialShader(const char *szName) override;

	void XMETHODCALLTYPE setFlag(const char *szFlag, bool isSet) override;
	bool XMETHODCALLTYPE getFlag(const char *szFlag) override;
	IMaterialFlag* XMETHODCALLTYPE getFlagHandler(const char *szFlag) override;

	void queueTextureUpload(CTexture *pTexture);
	void onTextureRelease(CTexture *pTexture);
	void onMaterialRelease(CMaterial *pMaterial);
	void update(float fDT);
	void onMaterialShaderChange(CMaterial *pMaterial);

	void onGlobalFlagChanged(CGlobalFlag *pFlag);

	void notifyChanged(XEventMaterialChanged *pEvent);

	bool saveMaterial(CMaterial *pMaterial);

	void XMETHODCALLTYPE scanMaterials() override;

	UINT XMETHODCALLTYPE getScannedMaterialsCount() override;
	const char* XMETHODCALLTYPE getScannedMaterial(UINT uIdx, IXMaterial **ppOut, bool *pisTexture = NULL, bool *pisTranslated = NULL, bool *pisMaterial = NULL) override;

	bool XMETHODCALLTYPE isMaterialLoaded(const char *szName) override;

	void XMETHODCALLTYPE addTexture(const char *szName, IGXBaseTexture *pGXTexture, IXTexture **ppTexture) override;

	void XMETHODCALLTYPE setFillMode(GXFILL_MODE fillMode) override
	{
		m_fillMode = fillMode;
	}
	GXFILL_MODE XMETHODCALLTYPE getFillMode() const override
	{
		return(m_fillMode);
	}
	void XMETHODCALLTYPE setCullMode(GXCULL_MODE cullMode = GXCULL_BACK) override
	{
		m_cullMode = cullMode;
	}
	GXCULL_MODE XMETHODCALLTYPE getCullMode() const override
	{
		return(m_cullMode);
	}

	UINT XMETHODCALLTYPE getMaterialPropertyCount(IXMaterial *pMat) const override;
	UINT XMETHODCALLTYPE getMaterialProperties(XMaterialProperty *pOut, IXMaterial *pMat, bool bSkipInactive = false) const override;

	void XMETHODCALLTYPE getMaterialShadersIterator(IKeyIterator **ppOut) override;

	bool XMETHODCALLTYPE testMaterialName(const char *szName) override;

	IGXDevice* getDevice()
	{
		return(m_pDevice);
	}

protected:
	struct CObjectData
	{
		SMMATRIX m_mW;
		//	SMMATRIX m_mWV;
		//	SMMATRIX m_mWVP;
	} m_objectData;
	IGXConstantBuffer *m_pObjectConstantBuffer = NULL;

	float m_fCurrentTime = 0.0f;

	MemAlloc<CTexture> m_poolTextures;
	Array<IXTextureProxy*> m_aTextureProxies;
	AssotiativeArray<String, IXTextureFilter*> m_mapTextureFilters;
	Map<String, CTexture*> m_mpTextures;
	CConcurrentQueue<CTexture*> m_queueTextureToLoad;
	IXTexture *m_pDefaultTexture = NULL;

	struct MaterialLoader
	{
		IXMaterialLoader *pLoader;
		bool canSave;
	};

	Array<IXMaterialProxy*> m_aMaterialProxies;
	AssotiativeArray<AAString, Array<MaterialLoader>> m_mapMaterialLoaders;
	Array<XFormatName> m_aMaterialExts;
	Map<String, CMaterial*> m_mapMaterials;
	SpinLock m_slMaterials;

	IEventChannel<XEventMaterialChanged> *m_pNotifyChannel = NULL;

	void updateReferences();
	void cleanData();

	struct VertexFormatData;
	struct VertexShaderData: public XVertexShaderHandler
	{
		const char *szShaderFile; //!< @fixme: нужно ли это хранить?
		ID idShader;
		VertexFormatData *pVertexFormat;
		Array<GXMacro> aDefines; //!< @fixme: нужно ли это хранить?
		UINT uID;
	};
	struct GeometryShader: public XGeometryShaderHandler
	{
		const char *szShaderFile;
		//VertexFormatData *pVertexFormat;
		Array<GXMacro> aDefines;
		Array<const char*> aszRequiredParameters;
		UINT uID;
	};
	struct GeometryShaderData
	{
		bool isSkipped;
		Array<GXMacro> aDefines;
		int iCommonDefines;
		ID idShader;
	};

	struct VertexFormatData: public XVertexFormatHandler
	{
		UINT uID;
		Array<XVertexOutputElement> aDecl;
		Array<VertexShaderData*> aVS;
		Array<GeometryShaderData*> aGS;
	};

	struct MaterialVariantVS
	{
		ID idSet;

		Array<ID> aGeometryShaders;
	};

	struct MaterialVariantPass
	{
		ID idShader;
		Array<MaterialVariantVS> aVertexShaders;
	};

	struct RenderPassVertexFormat
	{
		Array<MaterialVariantPass> aPassVariants;
	};

	struct RenderPass: public XRenderPassHandler
	{
		const char *szName;
		const char *szShaderFile;
		Array<XRenderPassTexturesElement> aTextures;
		Array<XRenderPassSamplersElement> aSamplers;
		Array<XRenderPassOutputElement> aOutput;
		Array<GXMacro> aDefines;

		bool bSkipMaterialShader;
		Array<RenderPassVertexFormat> aPassFormats; // used only if bSkipMaterialShader

		Array<Array<GXMacro>> aVariants;
	};

	struct MaterialShaderSamplerData
	{
		const char *szKey;
		IGXSamplerState *pSampler;
	};

	struct MaterialProperty
	{
		XMaterialProperty prop;
		CLogicExpression *pCondition;
	};

	struct MaterialDefine
	{
		const char *szName;
		CLogicExpression *pCondition;

		// XMaterialProperty *pProp;
	};

	struct MaterialShaderConstants
	{
		const char *szKey;
		GXDECLTYPE type;
		float4_t vDefault;
		UINT uGroup;
		UINT uOffset;
	};

	struct MaterialStaticFlag
	{
		const char *szName;
		bool bValue;
	};

	struct MaterialTexture
	{
		const char *szName;
		GXTEXTURE_TYPE type;
	};

	struct MaterialVariant
	{
		bool isReady;

		Array<MaterialVariantPass> aPassVariants;
		Array<MaterialTexture> aTextureMap;
		UINT uConstantsSize;
		Array<MaterialShaderConstants> aConstants;
	};
	
	struct MaterialShaderPassData
	{
		RenderPass *pRenderPass;
		const char *szShaderFile;
		const char *szEntryPoint;
		Array<GXMacro> aDefines;
		Array<MaterialShaderSamplerData> aSamplers;
		Array<MaterialProperty> aProperties;
		bool isDirty;

		Array<MaterialDefine> aAllDefines;
		Array<MaterialProperty*> aAllProperties;

		Array<MaterialShaderSamplerData> aTotalSamplers;

		Array<MaterialVariant> aVariants;
	};

	struct MaterialShader: public XMaterialShaderHandler
	{
		const char *szName;
		VertexFormatData *pVertexFormat;
		Array<MaterialShaderPassData> aPasses;
		Array<MaterialProperty> aProperties;

		Array<MaterialStaticFlag> aStaticFlags;
	};

	AssotiativeArray<String, VertexFormatData> m_mVertexFormats;
	MemAlloc<VertexShaderData> m_poolVSdata;
	MemAlloc<GeometryShader> m_poolGSdata;
	Array<GeometryShader*> m_aGeometryShaders;
	MemAlloc<GeometryShaderData> m_aGeometryShadersData;

	bool m_isMateriallesRenderPassDirty = false;
	AssotiativeArray<String, RenderPass> m_mRenderPasses;
	AssotiativeArray<String, MaterialShader> m_mMaterialShaders;

	VertexShaderData *m_pCurrentVS = NULL;
	GeometryShader *m_pCurrentGS = NULL;
	RenderPass *m_pCurrentRP = NULL;
	UINT m_uCurrentRPvariant = 0;

	AssotiativeArray<AAString, CGlobalFlag> m_mapFlags;

	friend void CopyProps(XMaterialProperty *pProperties, Array<CMaterialSystem::MaterialProperty> &aTarget, const char *szShaderName);
	friend void GetAllDefines(Array<MaterialDefine> &aAllDefines, Array<MaterialProperty> &aProperties, Array<MaterialProperty*> &aAllProps);
	friend void ParseTexturesConstants(Array<MaterialProperty> &aProperties, Array<MaterialDefine*> &aStaticList,
		Array<MaterialTexture> &aTextures, Array<MaterialShaderConstants> &aConstants);
	friend bool EvalCondition(CLogicExpression *pExpr, Array<MaterialDefine*> &aStaticList);

	const char* getHLSLType(GXDECLTYPE type)
	{
		switch(type)
		{
		case GXDECLTYPE_FLOAT1:
			return("float");
		case GXDECLTYPE_FLOAT2:
			return("float2");
		case GXDECLTYPE_FLOAT3:
			return("float3");
		case GXDECLTYPE_FLOAT4:
			return("float4");
		case GXDECLTYPE_UBYTE4:
			return("uint4");
		}
		assert(!"Unknown type");
		return("");
	}
	int getTypeSize(GXDECLTYPE type)
	{
		switch(type)
		{
		case GXDECLTYPE_FLOAT1:
			return(1);
		case GXDECLTYPE_FLOAT2:
			return(2);
		case GXDECLTYPE_FLOAT3:
			return(3);
		case GXDECLTYPE_FLOAT4:
			return(4);
		case GXDECLTYPE_UBYTE4:
			return(4);
		}
		assert(!"Unknown type");
		return(0);
	}
	const char* getHLSLSemantic(GXDECLUSAGE usage)
	{
		switch(usage)
		{
		case GXDECLUSAGE_POSITION:
			return("POSITION0");
		case GXDECLUSAGE_TEXCOORD:
			return("TEXCOORD0");
		case GXDECLUSAGE_TEXCOORD1:
			return("TEXCOORD1");
		case GXDECLUSAGE_TEXCOORD2:
			return("TEXCOORD2");
		case GXDECLUSAGE_TEXCOORD3:
			return("TEXCOORD3");
		case GXDECLUSAGE_TEXCOORD4:
			return("TEXCOORD4");
		case GXDECLUSAGE_TEXCOORD5:
			return("TEXCOORD5");
		case GXDECLUSAGE_TEXCOORD6:
			return("TEXCOORD6");
		case GXDECLUSAGE_TEXCOORD7:
			return("TEXCOORD7");
		case GXDECLUSAGE_BINORMAL:
			return("BINORMAL0");
		case GXDECLUSAGE_BLENDINDICES:
			return("BLENDINDICES0");
		case GXDECLUSAGE_BLENDWEIGHT:
			return("BLENDWEIGHT0");
		case GXDECLUSAGE_COLOR:
			return("COLOR0");
		case GXDECLUSAGE_NORMAL:
			return("NORMAL0");
		case GXDECLUSAGE_TANGENT:
			return("TANGENT0");
		case GXDECLUSAGE_TESSFACTOR:
			return("TESSFACTOR0");
		}
		assert(!"Unknown usage");
		return("");
	}

	bool loadMaterialFromFile(const char *szName, CMaterial *pMaterial);
	CGlobalFlag* createFlag(const char *szName);

private:

	struct ScanItem
	{
		String sName;
		bool isTranslated;
		bool isTexture;
		bool isMaterial;
		CMaterialInfo *pMatInfo;

		ScanItem() = default;

		ScanItem(ScanItem &&other):
			sName(std::move(other.sName)),
			isTranslated(other.isTranslated),
			isTexture(other.isTexture),
			isMaterial(other.isMaterial),
			pMatInfo(other.pMatInfo)
		{
		}

		ScanItem& operator=(ScanItem &&other)
		{
			sName = std::move(other.sName);
			isTranslated = other.isTranslated;
			isTexture = other.isTexture;
			isMaterial = other.isMaterial;
			pMatInfo = other.pMatInfo;
			return(*this);
		}
	};

	Array<ScanItem> m_aScanCache;

	void scanForExtension(IFileSystem *pFS, const char *szDir, const char *szExt, Map<String, bool> &mapFiles, bool isTexture);
	void clearScanCache();

private:
	GXFILL_MODE m_fillMode = GXFILL_SOLID;
	GXCULL_MODE m_cullMode = GXCULL_BACK;
	// m_apRasterizerStates[GXFILL_MODE][GXCULL_MODE];
	IGXRasterizerState *m_aapRasterizerStates[2][3];

	IGXDevice *m_pDevice = NULL;
	IXRender *m_pRender = NULL;
};

#endif
