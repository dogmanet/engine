#ifndef __XEVENTS_H
#define __XEVENTS_H

#include <gdefines.h>

template<typename T>
class IEventListener
{
public:
	virtual void onEvent(const T*) = 0;
};

class IBaseEventChannel
{
public:
	virtual ~IBaseEventChannel() = default;
};

template<typename T>
class IEventChannel: public IBaseEventChannel
{
	typedef void(*PFNLISTENER)(const T*);
	friend class CCore;
	IEventChannel()
	{
	}
public:
	void addListener(PFNLISTENER fnListener)
	{
		if(m_vListeners.indexOf(fnListener) < 0)
		{
			m_vListeners.push_back(fnListener);
		}
	}
	void addListener(IEventListener<T> *pListener)
	{
		if(m_vListeners2.indexOf(pListener) < 0)
		{
			m_vListeners2.push_back(pListener);
		}
	}
	void removeListener(PFNLISTENER fnListener)
	{
		int idx = m_vListeners.indexOf(fnListener);
		if(idx >= 0)
		{
			m_vListeners[idx] = m_vListeners[m_vListeners.size() - 1];
			m_vListeners.erase(m_vListeners.size() - 1);
		}
	}
	void removeListener(IEventListener<T> *pListener)
	{
		int idx = m_vListeners2.indexOf(pListener);
		if(idx >= 0)
		{
			m_vListeners2[idx] = m_vListeners2[m_vListeners2.size() - 1];
			m_vListeners2.erase(m_vListeners2.size() - 1);
		}
	}
	void broadcastEvent(const T *pEvent)
	{
		for(UINT i = 0; i < m_vListeners.size(); ++i)
		{
			m_vListeners[i](pEvent);
		}
		for(UINT i = 0; i < m_vListeners2.size(); ++i)
		{
			m_vListeners2[i]->onEvent(pEvent);
		}
	}
	UINT getListenersCount()
	{
		return(m_vListeners.size() + m_vListeners2.size());
	}
protected:
	Array<PFNLISTENER> m_vListeners;
	Array<IEventListener<T>*> m_vListeners2;
};



// {DCC97EFB-5254-48E2-B3C3-D9C03392FFDA}
#define EVENT_LEVEL_GUID DEFINE_XGUID(0xdcc97efb, 0x5254, 0x48e2, 0xb3, 0xc3, 0xd9, 0xc0, 0x33, 0x92, 0xff, 0xda)

struct XEventLevel
{
	enum
	{
		TYPE_LOAD, //!< Запрос на загрузку уровня
		TYPE_SAVE, //!< Запрос на сохранение уровня
		TYPE_UNLOAD, //!< Запрос на выгрузку уровня


		TYPE_LOAD_BEGIN, //!< До начала загрузки уровня
		TYPE_LOAD_END, //!< Уровень полностью загружен
		TYPE_UNLOAD_BEGIN, //!< До начала выгрузки уровня
		TYPE_UNLOAD_END, //!< Уровень полностью выгружен
	} type;
	const char *szLevelName;
};



// {ED67CE1B-C328-45A3-B0E1-288321236F2C}
#define EVENT_LEVEL_PROGRESS_GUID DEFINE_XGUID(0xed67ce1b, 0xc328, 0x45a3, 0xb0, 0xe1, 0x28, 0x83, 0x21, 0x23, 0x6f, 0x2c)

struct XEventLevelProgress
{
	enum
	{
		TYPE_PROGRESS_BEGIN,
		TYPE_PROGRESS_VALUE,
		TYPE_PROGRESS_END,
	} type;
	ID idPlugin;
	float fProgress = 0.0f;
	const char *szLoadingText = NULL;
};


// {E20F61DF-BBFC-4920-A669-3143CB7D3717}
#define EVENT_LEVEL_GET_SIZE_GUID DEFINE_XGUID(0xe20f61df, 0xbbfc, 0x4920, 0xa6, 0x69, 0x31, 0x43, 0xcb, 0x7d, 0x37, 0x17)

struct XEventLevelSize
{
	mutable float3 vMin;
	mutable float3 vMax;
};


// {67822A33-6F92-460F-8AA8-817874CD0F4E}
#define EVENT_ASYNC_TASK_GUID DEFINE_XGUID(0x67822a33, 0x6f92, 0x460f, 0x8a, 0xa8, 0x81, 0x78, 0x74, 0xcd, 0xf, 0x4e)

struct XEventAsyncTask
{
	enum
	{
		TYPE_ADDED,
		TYPE_STARTED,
		TYPE_FINISHED,
		TYPE_PROGRESS
	} type;
	void *pTaskId;
	int iProgress;
	const char *szTaskName;
};


// {534A9C0A-52AF-477F-8440-09C4C60A0F05}
#define EVENT_MATERIAL_CHANGED_GUID DEFINE_XGUID(0x534a9c0a, 0x52af, 0x477f, 0x84, 0x40, 0x9, 0xc4, 0xc6, 0xa, 0xf, 0x5)

class IXMaterial;
struct XEventMaterialChanged
{
	enum TYPE
	{
		TYPE_TRANSPARENCY,
		TYPE_REFRACTIVITY,
		TYPE_BLURRED,
		TYPE_PHYSICS_TYPE,
		TYPE_DURABILITY,
		TYPE_HIT_CHANCE,
		TYPE_DENSITY,
		TYPE_PROPERTY,
		TYPE_FLAG,
		TYPE_SHADER,
		TYPE_EMISSIVITY,
		TYPE_EDITORIAL,
		// ...
	} type;
	IXMaterial *pMaterial;
	const char *szReference;
};


// {32D85440-D150-4301-A6E0-7AE89BF34DC6}
#define EVENT_MODEL_CHANGED_GUID DEFINE_XGUID(0x32d85440, 0xd150, 0x4301, 0xa6, 0xe0, 0x7a, 0xe8, 0x9b, 0xf3, 0x4d, 0xc6)

class IXModel;
struct XEventModelChanged
{
	enum TYPE
	{
		TYPE_CREATED,
		TYPE_BEFORE_REMOVED,
		TYPE_MOVED,
		TYPE_VISIBILITY,
		TYPE_SKIN,
		// ...
	} type;
	IXModel *pModel;
};


// {A816DD94-0F92-48EA-A174-03599963BF84}
#define EVENT_CVAR_CHANGED_GUID DEFINE_XGUID(0xa816dd94, 0xf92, 0x48ea, 0xa1, 0x74, 0x3, 0x59, 0x99, 0x63, 0xbf, 0x84)

class IXModel;
struct XEventCvarChanged
{
	enum TYPE
	{
		TYPE_INT,
		TYPE_FLOAT,
		TYPE_STRING,
		TYPE_BOOL
		// ...
	} type;
	const char *szName;
	const void *pCvar;
};


// {5CEC2355-1F1E-4A1D-8E69-5B92850B62D2}
#define EVENT_OBSERVER_CHANGED_GUID DEFINE_XGUID(0x5cec2355, 0x1f1e, 0x4a1d, 0x8e, 0x69, 0x5b, 0x92, 0x85, 0xb, 0x62, 0xd2)

class IXCamera;
struct XEventObserverChanged
{
	IXCamera *pCamera;
};


// {5E77BE25-9855-4E96-ADEA-3F63694DAC82}
#define EVENT_SKYBOX_CHANGED_GUID DEFINE_XGUID(0x5e77be25, 0x9855, 0x4e96, 0xad, 0xea, 0x3f, 0x63, 0x69, 0x4d, 0xac, 0x82)

class IXTexture;
struct XEventSkyboxChanged
{
	IXTexture *pTexture;
};


// {1DB80A19-7DFA-4D61-923B-34906590DBB0}
#define EVENT_PHYSICS_STEP_GUID DEFINE_XGUID(0x1db80a19, 0x7dfa, 0x4d61, 0x92, 0x3b, 0x34, 0x90, 0x65, 0x90, 0xdb, 0xb0)

class IXPhysics;
struct XEventPhysicsStep
{
	IXPhysics *pPhysics;
	float fTimeStep;
};


// @FIXME move that somewhere else
// {2C5F63CB-4CFE-46D1-9AB9-B8B028E96C97}
#define EVENT_EDITOR_XFORM_TYPE_GUID DEFINE_XGUID(0x2c5f63cb, 0x4cfe, 0x46d1, 0x9a, 0xb9, 0xb8, 0xb0, 0x28, 0xe9, 0x6c, 0x97)

enum X_2DXFORM_TYPE
{
	X2DXF_NONE = -1,
	X2DXF_SCALE = 0,
	X2DXF_ROTATE,


	X2DXF__LAST
};
struct XEventEditorXformType
{
	X_2DXFORM_TYPE newXformType;
};

#endif
