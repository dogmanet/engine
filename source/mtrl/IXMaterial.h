#ifndef __IXMATERIAL_H
#define __IXMATERIAL_H

#include <gdefines.h>
#include "IXTexture.h"

struct XVertexFormatHandler{};
struct XRenderPassHandler{};
struct XVertexShaderHandler{};
struct XGeometryShaderHandler{};
struct XMaterialShaderHandler{};

//! физический тип материала
enum MTLTYPE_PHYSIC
{
	MTLTYPE_PHYSIC_CONCRETE = 0,	//!< бетон

	//! значение по умолчанию
	MTLTYPE_PHYSIC_DEFAULT = MTLTYPE_PHYSIC_CONCRETE,

	//! металл
	MTLTYPE_PHYSIC_METAL,

	//! стекло
	MTLTYPE_PHYSIC_GLASS,

	//! пластик
	MTLTYPE_PHYSIC_PLASTIC,

	//! дерево
	MTLTYPE_PHYSIC_TREE,

	//! плоть
	MTLTYPE_PHYSIC_FLESH,

	//! земля/песок
	MTLTYPE_PHYSIC_GROUD_SAND,

	//! вода
	MTLTYPE_PHYSIC_WATER,

	//! листва/трава
	MTLTYPE_PHYSIC_LEAF_GRASS,

	//! количество типов
	MPT_COUNT
};

class IMaterialProperty
{
public:
	virtual const char* XMETHODCALLTYPE getName() const = 0;

	virtual void XMETHODCALLTYPE set(const float4_t &vValue) = 0;
	virtual float4_t XMETHODCALLTYPE get() const = 0;
};

class IMaterialFlag
{
public:
	virtual const char* XMETHODCALLTYPE getName() const = 0;

	virtual void XMETHODCALLTYPE set(bool bValue) = 0;
	virtual bool XMETHODCALLTYPE get() const = 0;
};

class IXMaterial: public IXUnknown
{
public:
	// virtual void XMETHODCALLTYPE getMainTexture(IXTexture **ppTexture) = 0;

	virtual void XMETHODCALLTYPE setTransparent(bool bValue) = 0;
	virtual bool XMETHODCALLTYPE isTransparent() const = 0;

	virtual void XMETHODCALLTYPE setRefractive(bool bValue) = 0;
	virtual bool XMETHODCALLTYPE isRefractive() const = 0;

	virtual void XMETHODCALLTYPE setBlurred(bool bValue) = 0;
	virtual bool XMETHODCALLTYPE isBlurred() const = 0;

	virtual void XMETHODCALLTYPE setShader(const char *szShader) = 0;
	virtual const char* XMETHODCALLTYPE getShader() const = 0;
	virtual XMaterialShaderHandler* XMETHODCALLTYPE getShaderHandler() const = 0;

	virtual void XMETHODCALLTYPE setFlag(const char *szFlag, bool isSet) = 0;
	virtual bool XMETHODCALLTYPE getFlag(const char *szFlag) = 0;
	virtual IMaterialFlag* XMETHODCALLTYPE getFlagHandler(const char *szFlag) = 0;
	virtual IKeyIterator* XMETHODCALLTYPE getFlagsIterator() = 0;

	virtual void XMETHODCALLTYPE setParam(const char *szFlag, const float4_t &fValue) = 0;
	virtual float4_t XMETHODCALLTYPE getParam(const char *szFlag) = 0;
	virtual IMaterialProperty* XMETHODCALLTYPE getParamHandler(const char *szFlag) = 0;
	virtual IKeyIterator* XMETHODCALLTYPE getParamsIterator() = 0;

	virtual void XMETHODCALLTYPE setTexture(const char *szKey, const char *szTexture) = 0;
	virtual const char* XMETHODCALLTYPE getTextureName(const char *szKey) const = 0;
	virtual IXTexture* XMETHODCALLTYPE getTexture(const char *szKey) const = 0;
	virtual IKeyIterator* XMETHODCALLTYPE getTexturesIterator() = 0;

	virtual void XMETHODCALLTYPE setPhysicsType(MTLTYPE_PHYSIC type) = 0;
	virtual MTLTYPE_PHYSIC XMETHODCALLTYPE getPhysicsType() const = 0;

	virtual void XMETHODCALLTYPE setDurablility(float fValue) = 0;
	virtual float XMETHODCALLTYPE getDurablility() const = 0;

	virtual void XMETHODCALLTYPE setHitChance(float fValue) = 0;
	virtual float XMETHODCALLTYPE getHitChance() const = 0;

	virtual void XMETHODCALLTYPE setDensity(float fValue) = 0;
	virtual float XMETHODCALLTYPE getDensity() const = 0;

	virtual const char* XMETHODCALLTYPE getName() const = 0;

	virtual bool XMETHODCALLTYPE save() = 0;

	virtual void XMETHODCALLTYPE setEmissive(bool bValue) = 0;
	virtual bool XMETHODCALLTYPE isEmissive() const = 0;

	virtual void XMETHODCALLTYPE setEditorial(bool bValue) = 0;
	virtual bool XMETHODCALLTYPE isEditorial() const = 0;

	virtual void XMETHODCALLTYPE setTwoSided(bool bValue) = 0;
	virtual bool XMETHODCALLTYPE isTwoSided() const = 0;
};

#endif
