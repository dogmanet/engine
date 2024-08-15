
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#ifndef __PROPTABLE_H
#define __PROPTABLE_H

#include <common/Math.h>
#include "EntityPointer.h"
#include "EntityList.h"

class CBaseEntity;

#define ENT_FLAG_0 0x00010000
#define ENT_FLAG_1 0x00020000
#define ENT_FLAG_2 0x00040000
#define ENT_FLAG_3 0x00080000
#define ENT_FLAG_4 0x00100000
#define ENT_FLAG_5 0x00200000
#define ENT_FLAG_6 0x00400000
#define ENT_FLAG_7 0x00800000
#define ENT_FLAG_8 0x01000000
#define ENT_FLAG_9 0x02000000
#define ENT_FLAG_10 0x04000000
#define ENT_FLAG_11 0x08000000
#define ENT_FLAG_12 0x10000000
#define ENT_FLAG_13 0x20000000
#define ENT_FLAG_14 0x40000000
#define ENT_FLAG_15 0x80000000

#define ENT_OUTPUT_PARAM_NONE "<none>"

#define ENT_SPECIAL_NAME_MARKER "!"
#define ENT_IS_NAME_SPECIAL(name) (name[0] == ENT_SPECIAL_NAME_MARKER[0])

#define ENT_SPECIAL_NAME_THIS (ENT_SPECIAL_NAME_MARKER "this")
#define ENT_SPECIAL_NAME_PARENT (ENT_SPECIAL_NAME_MARKER "parent")

static_assert(sizeof(ENT_SPECIAL_NAME_MARKER) == 2, "ENT_SPECIAL_NAME_MARKER must be exactly one character!");

//! типы полей данных
enum PDF_TYPE
{
	PDF_NONE,
	PDF_INT,
	PDF_FLOAT,
	PDF_VECTOR,
	PDF_VECTOR4,
	PDF_BOOL,
	PDF_STRING,
	PDF_ANGLES,
	PDF_ENTITY,
	PDF_FLAGS,

	PDF_FLAG,

	PDF_OUTPUT
};

//! типы редакторов полей
enum PDE_TYPE
{
	PDE_NONE = 0,
	PDE_TEXTFIELD,
	PDE_COMBOBOX,
	PDE_FILEFIELD,
	PDE_FLAGS,

	PDE_TIME,
	PDE_ANGLES,
	PDE_POINTCOORD,
	PDE_RADIUS,
	PDE_COLOR,
	PDE_HDRCOLOR
};

enum PDF_FLAG
{
	PDFF_NONE       = 0x00,
	PDFF_NOEXPORT   = 0x01, //!< Не экспортировать поле в файл
	PDFF_NOEDIT     = 0x02, //!< Не отображать поле в редакторе
	PDFF_INPUT      = 0x04, //!< Поле входа
	PDFF_OUTPUT     = 0x08, //!< Поле выхода
	PDFF_MESSAGE    = 0x10, //!< Поле сообщения
	PDFF_USE_GIZMO  = 0x20, //!< Использовать гизмо для редактирования
};

enum ENT_FLAG
{
	EF_NONE            = 0x0000,
	EF_EXPORT          = 0x0001,
	EF_LEVEL           = 0x0002,
	EF_REMOVED         = 0x0004,
	EF_NO_WORLD_LOOKUP = 0x0008,

	EF_LAST            = 0x8000
};

typedef int CBaseEntity::*fieldtype;

typedef void (CBaseEntity::*PFNFIELDSETV3)(const float3&);
typedef void (CBaseEntity::*PFNFIELDSETV4)(const float4&);
typedef void (CBaseEntity::*PFNFIELDSETF)(float);
typedef void (CBaseEntity::*PFNFIELDSETSZ)(const char*);
typedef void (CBaseEntity::*PFNFIELDSETI)(int);
typedef void (CBaseEntity::*PFNFIELDSETB)(bool);
typedef void (CBaseEntity::*PFNFIELDSETQ)(const SMQuaternion&);
typedef void (CBaseEntity::*PFNFIELDSETE)(CBaseEntity*);
union PFNFIELDSET
{
	PFNFIELDSET():
		__(0)
	{
	}
	PFNFIELDSET(PFNFIELDSETV3 arg):
		v3(arg)
	{
	}
	PFNFIELDSET(PFNFIELDSETV4 arg):
		v4(arg)
	{
	}
	PFNFIELDSET(PFNFIELDSETF arg):
		f(arg)
	{
	}
	PFNFIELDSET(PFNFIELDSETSZ arg):
		sz(arg)
	{
	}
	PFNFIELDSET(PFNFIELDSETI arg):
		i(arg)
	{
	}
	PFNFIELDSET(PFNFIELDSETB arg):
		b(arg)
	{
	}
	PFNFIELDSET(PFNFIELDSETQ arg):
		q(arg)
	{
	}
	PFNFIELDSET(PFNFIELDSETE arg):
		e(arg)
	{
	}
	template<typename T>
	PFNFIELDSET(void(CBaseEntity::*arg)(T*)):
		e((PFNFIELDSETE)arg)
	{
		static_assert(std::is_base_of<CBaseEntity, T>::value, "T must be subclass of CBaseEntity");
	}
	PFNFIELDSETV3 v3;
	PFNFIELDSETV4 v4;
	PFNFIELDSETF f;
	PFNFIELDSETSZ sz;
	PFNFIELDSETI i;
	PFNFIELDSETB b;
	PFNFIELDSETQ q;
	PFNFIELDSETE e;
	int __;
};

struct editor_kv
{
	const char * key;
	const char * value;
};

struct prop_editor_t
{
	PDE_TYPE type;
	void * pData;
};


/*! Устанавливает значение строкового свойства
\note только для внутреннего использования
*/
void _setStrVal(const char **to, const char *value);

struct inputdata_t
{
	CBaseEntity *pInflictor; //!< Косвенный активатор (вызвавший эту цепочку активаций)
	CBaseEntity *pActivator; //!< Непосредственный активатор
	PDF_TYPE type; //!< Тип аргумента
	union
	{
		int i;
		bool b;
		float f;
		const char * str;
	}
	parameter;
	float3_t v3Parameter;
	float4_t v4Parameter;

	void setParameter(const inputdata_t &other);
};

typedef void(CBaseEntity::*input_func)(inputdata_t *pInputData);

struct propdata_t
{
	propdata_t():
		pField(NULL),
		type(PDF_NONE),
		flags(0),
		szKey(NULL),
		szEdName(NULL),
		editor({})
	{}
	propdata_t(fieldtype f, PDF_TYPE t, int fl, const char *key, const char *edname, prop_editor_t ed):
		pField(f),
		type(t),
		flags(fl),
		szKey(key),
		szEdName(edname),
		editor(ed)
	{}
	propdata_t(fieldtype f, PDF_TYPE t, int fl, const char *key, const char *edname, PFNFIELDSET _fnSet, prop_editor_t ed):
		pField(f),
		type(t),
		flags(fl),
		szKey(key),
		szEdName(edname),
		editor(ed),
		fnSet(_fnSet)
	{}
	propdata_t(input_func d, PDF_TYPE t, int fl, const char *key, const char *edname, prop_editor_t ed):
		fnInput(d),
		type(t),
		flags(fl),
		szKey(key),
		szEdName(edname),
		editor(ed)
	{}
	union
	{
		fieldtype pField;
		input_func fnInput;
	};
	PDF_TYPE type;
	int flags;
	const char * szKey;
	const char * szEdName;
	prop_editor_t editor;
	PFNFIELDSET fnSet;

	template<typename T>
	static fieldtype ToFieldType(T arg)
	{
		return((fieldtype)arg);
	}

	template<typename T>
	static input_func ToInputFunc(T arg)
	{
		return((input_func)arg);
	}

	template<typename T, typename Targ>
	static PFNFIELDSET ToPFNFieldSet(void(T::*arg)(Targ))
	{
		return((void(CBaseEntity::*)(Targ))arg);
	}
};


struct input_t
{
	input_func fnInput;
	CBaseEntity *pTarget;
	inputdata_t data;
	bool useOverrideData;
};

struct named_output_t
{
	float fDelay = 0.0f;
	float fDelayTo = 0.0f;
	bool useRandomDelay = false;
	const char *szTargetName = NULL;
	const char *szTargetInput = NULL;
	const char *szTargetData = NULL;
	int iFireLimit = -1;

	CEntityList listTargets;
	Array<input_t> aOutputs;
	// int iOutCount = 0;
	// input_t *pOutputs = NULL;

	void init(CBaseEntity *pThisEntity);
	void onEntityAdded(CBaseEntity *pEnt);
	void onEntityRemoved(CBaseEntity *pEnt);

	class OnLinkEstablished final: public IEntityPointerEvent
	{
	public:
		OnLinkEstablished(named_output_t *pOut):
			m_pOut(pOut)
		{
		}
		void onEvent(CBaseEntity *pEnt) override
		{
			m_pOut->onEntityAdded(pEnt);
		}
	private:
		named_output_t *m_pOut;
	};
	class OnLinkBroken final: public IEntityPointerEvent
	{
	public:
		OnLinkBroken(named_output_t *pOut):
			m_pOut(pOut)
		{
		}
		void onEvent(CBaseEntity *pEnt) override
		{
			m_pOut->onEntityRemoved(pEnt);
		}
	private:
		named_output_t *m_pOut;
	};

	OnLinkEstablished onLinkEstablished;
	OnLinkBroken onLinkBroken;

	named_output_t():
		onLinkEstablished(this),
		onLinkBroken(this)
	{
		listTargets.setLinkEstablishedListener(&onLinkEstablished);
		listTargets.setLinkBrokenListener(&onLinkBroken);
	}
};

struct output_t
{
	void fire(CBaseEntity *pInflictor, CBaseEntity *pActivator, inputdata_t *pInputData = NULL);
	
	// bool bDirty = false;
	// int iOutCount = 0;
	Array<named_output_t> aOutputs;
	void *pData = NULL;
};

#define FIRE_OUTPUT(output, inflictor) (output).fire((inflictor), this)
#define FIRE_OUTPUT_PARAM(output, inflictor, param) (output).fire((inflictor), this, (param))

struct proptable_t
{
	propdata_t * pData;
	int numFields;
	proptable_t * pBaseProptable;
};

#define ED_COMBO_MAXELS 256

prop_editor_t _GetEditorCombobox(int start, ...);
prop_editor_t _GetEditorFilefield(int start, ...);

#define DECLARE_PROPTABLE() \
	\
	friend class CEntityFactoryMap; \
	friend class CEntityFactory<ThisClass>; \
	static proptable_t m_pPropTable; \
	\
protected:\
	static void InitPropData(); \
	virtual proptable_t * getPropTable(); \
	static proptable_t * SGetPropTable(); \
	static void ReleasePropData();\
public:\
	virtual propdata_t * getField(const char * name);\
private:

#define _BEGIN_PROPTABLE(cls, bclpt) \
proptable_t cls::m_pPropTable = {0,0,0}; \
void cls::ReleasePropData()\
{\
	for(int i = 0; i < m_pPropTable.numFields; ++i)\
	{\
		char *szData = (char*)m_pPropTable.pData[i].editor.pData; mem_delete_a(szData); m_pPropTable.pData[i].editor.pData = NULL; \
	}\
}\
proptable_t * cls::SGetPropTable()\
{\
	if(!m_pPropTable.numFields)\
		InitPropData();\
	return(&m_pPropTable);\
}\
\
proptable_t * cls::getPropTable()\
{\
	if(!m_pPropTable.numFields)\
		InitPropData(); \
	return(&m_pPropTable); \
}\
propdata_t * cls::getField(const char * name)\
{\
	proptable_t * pt = getPropTable();\
	while(pt)\
	{\
		for(int i = 0; i < pt->numFields; ++i)\
		{\
			if(pt->pData[i].szKey && !strcmp(pt->pData[i].szKey, name))\
			{\
				return(&pt->pData[i]);\
			}\
		}\
		pt = pt->pBaseProptable;\
	}\
	return(NULL);\
}\
void cls::InitPropData() \
{ \
	bclpt \
	typedef cls DataClass; \
	static propdata_t pData[] = {propdata_t()

#define BEGIN_PROPTABLE(cls) \
	_BEGIN_PROPTABLE(cls, m_pPropTable.pBaseProptable = BaseClass::SGetPropTable();)

#define BEGIN_PROPTABLE_NOBASE(cls) \
	_BEGIN_PROPTABLE(cls, m_pPropTable.pBaseProptable = NULL;)

#define END_PROPTABLE() \
	}; \
	if(ARRAYSIZE(pData) > 1) \
	{\
		m_pPropTable.pData = &pData[1]; \
		m_pPropTable.numFields = ARRAYSIZE(pData) - 1; \
	} \
	else \
	{ \
		m_pPropTable.pData = pData; \
		m_pPropTable.numFields = 1; \
	} \
} 

#define DECLARE_TRIVIAL_CONSTRUCTOR() ThisClass():BaseClass(){}
#define DECLARE_CONSTRUCTOR() ThisClass();

const char * GetEmptyString();

#define EDITOR_NONE {PDE_NONE, NULL}}
#define EDITOR_TEXTFIELD {PDE_TEXTFIELD, NULL}}
#define EDITOR_TIMEFIELD {PDE_TIME, NULL}}
#define EDITOR_ANGLES {PDE_ANGLES, NULL}}
#define EDITOR_POINTCOORD {PDE_POINTCOORD, NULL}}
#define EDITOR_RADIUS {PDE_RADIUS, NULL}}
#define EDITOR_FLAGS {PDE_FLAGS, NULL}}
#define EDITOR_COLOR {PDE_COLOR, NULL}}
#define EDITOR_HDRCOLOR {PDE_HDRCOLOR, NULL}}

#define EDITOR_COMBOBOX _GetEditorCombobox(0
#define COMBO_OPTION(name, value) , name, value
#define EDITOR_COMBO_END() , NULL)}

#define EDITOR_FILEFIELD _GetEditorFilefield(0
#define FILE_OPTION(name, value) , name, value
#define EDITOR_FILE_END() , NULL)}

#define EDITOR_YESNO EDITOR_COMBOBOX COMBO_OPTION("Yes", "1") COMBO_OPTION("No", "0") EDITOR_COMBO_END()
#define EDITOR_MODEL EDITOR_FILEFIELD FILE_OPTION("Select model", "dse") EDITOR_FILE_END()
#define EDITOR_SOUND EDITOR_FILEFIELD FILE_OPTION("Select sound", "ogg") EDITOR_FILE_END()
#define EDITOR_TEXTURE EDITOR_FILEFIELD FILE_OPTION("Select texture", "dds") EDITOR_FILE_END()
#define EDITOR_EFFECT EDITOR_FILEFIELD FILE_OPTION("Select effect", "eff") EDITOR_FILE_END()

#define DEFINE_FIELD_STRING(field, flags, keyname, edname, editor)              , {propdata_t::ToFieldType<const char* DataClass::*>(&DataClass::field),          PDF_STRING,  flags, keyname, edname, editor
#define DEFINE_FIELD_VECTOR(field, flags, keyname, edname, editor)              , {propdata_t::ToFieldType<float3_t DataClass::*>(&DataClass::field),             PDF_VECTOR,  flags, keyname, edname, editor
#define DEFINE_FIELD_VECTOR4(field, flags, keyname, edname, editor)             , {propdata_t::ToFieldType<float4_t DataClass::*>(&DataClass::field),             PDF_VECTOR4, flags, keyname, edname, editor
#define DEFINE_FIELD_ANGLES(field, flags, keyname, edname, editor)              , {propdata_t::ToFieldType<SMQuaternion DataClass::*>(&DataClass::field),         PDF_ANGLES,  flags, keyname, edname, editor
#define DEFINE_FIELD_INT(field, flags, keyname, edname, editor)                 , {propdata_t::ToFieldType<int DataClass::*>(&DataClass::field),                  PDF_INT,     flags, keyname, edname, editor
#define DEFINE_FIELD_ENUM(type, field, flags, keyname, edname, editor)          , {propdata_t::ToFieldType<type DataClass::*>(&DataClass::field),                 PDF_INT,     flags, keyname, edname, editor
#define DEFINE_FIELD_FLOAT(field, flags, keyname, edname, editor)               , {propdata_t::ToFieldType<float DataClass::*>(&DataClass::field),                PDF_FLOAT,   flags, keyname, edname, editor
#define DEFINE_FIELD_BOOL(field, flags, keyname, edname, editor)                , {propdata_t::ToFieldType<bool DataClass::*>(&DataClass::field),                 PDF_BOOL,    flags, keyname, edname, editor
#define DEFINE_FIELD_ENTITY(type, field, flags, keyname, edname, editor)        , {propdata_t::ToFieldType<CEntityPointer<type> DataClass::*>(&DataClass::field), PDF_ENTITY,  flags, keyname, edname, editor
#define DEFINE_FIELD_FLAGS(field, flags, keyname, edname, editor)               , {propdata_t::ToFieldType<UINT DataClass::*>(&DataClass::field),                 PDF_FLAGS,   flags, keyname, edname, editor

#define DEFINE_FIELD_STRINGFN(field, flags, keyname, edname, fn, editor)        , {propdata_t::ToFieldType<const char* DataClass::*>(&DataClass::field),          PDF_STRING,  flags, keyname, edname, propdata_t::ToPFNFieldSet<DataClass, const char*>(&DataClass::fn),         editor
#define DEFINE_FIELD_VECTORFN(field, flags, keyname, edname, fn, editor)        , {propdata_t::ToFieldType<float3_t DataClass::*>(&DataClass::field),             PDF_VECTOR,  flags, keyname, edname, propdata_t::ToPFNFieldSet<DataClass, const float3&>(&DataClass::fn),       editor
#define DEFINE_FIELD_VECTOR4FN(field, flags, keyname, edname, fn, editor)       , {propdata_t::ToFieldType<float4_t DataClass::*>(&DataClass::field),             PDF_VECTOR4, flags, keyname, edname, propdata_t::ToPFNFieldSet<DataClass, const float4&>(&DataClass::fn),       editor
#define DEFINE_FIELD_ANGLESFN(field, flags, keyname, edname, fn, editor)        , {propdata_t::ToFieldType<SMQuaternion DataClass::*>(&DataClass::field),         PDF_ANGLES,  flags, keyname, edname, propdata_t::ToPFNFieldSet<DataClass, const SMQuaternion&>(&DataClass::fn), editor
#define DEFINE_FIELD_INTFN(field, flags, keyname, edname, fn, editor)           , {propdata_t::ToFieldType<int DataClass::*>(&DataClass::field),                  PDF_INT,     flags, keyname, edname, propdata_t::ToPFNFieldSet<DataClass, int>(&DataClass::fn),                 editor
#define DEFINE_FIELD_ENUMFN(type, field, flags, keyname, edname, fn, editor)    , {propdata_t::ToFieldType<type DataClass::*>(&DataClass::field),                 PDF_INT,     flags, keyname, edname, propdata_t::ToPFNFieldSet<DataClass, int>(&DataClass::fn),                 editor
#define DEFINE_FIELD_FLOATFN(field, flags, keyname, edname, fn, editor)         , {propdata_t::ToFieldType<float DataClass::*>(&DataClass::field),                PDF_FLOAT,   flags, keyname, edname, propdata_t::ToPFNFieldSet<DataClass, float>(&DataClass::fn),               editor
#define DEFINE_FIELD_BOOLFN(field, flags, keyname, edname, fn, editor)          , {propdata_t::ToFieldType<bool DataClass::*>(&DataClass::field),                 PDF_BOOL,    flags, keyname, edname, propdata_t::ToPFNFieldSet<DataClass, bool>(&DataClass::fn),                editor
//#define DEFINE_FIELD_FLAGSFN(field, flags, keyname, edname, fn, editor)  , {(fieldtype)&DataClass::field, PDF_FLAGS,  flags, keyname, edname, fn, editor

#define DEFINE_INPUT(method, keyname, edname, argtype)   , {propdata_t::ToInputFunc<void(DataClass::*)(inputdata_t*)>(&DataClass::method), argtype, PDFF_NOEDIT | PDFF_INPUT, keyname, edname, EDITOR_NONE
#define DEFINE_OUTPUT(field, keyname, edname)            , {propdata_t::ToFieldType<output_t DataClass::*>(&DataClass::field), PDF_OUTPUT, PDFF_NOEDIT | PDFF_OUTPUT, keyname, edname, EDITOR_NONE
#define DEFINE_MESSAGE(method, keyname, edname, argtype) , {propdata_t::ToInputFunc<void(DataClass::*)(inputdata_t*)>(&DataClass::method), argtype, PDFF_NOEDIT | PDFF_MESSAGE, keyname, edname, EDITOR_NONE

#define DEFINE_FLAG(value, edname)                       , {(fieldtype)NULL, PDF_FLAG, value, NULL, edname, EDITOR_FLAGS

#endif
