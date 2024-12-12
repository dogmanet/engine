
#include <xcommon/IXPlugin.h>
#include <xcommon/IXUpdatable.h>
#include "SoundSystem.h"
#include <Audio.h>
//#include <gcore/sxgcore.h>
#include <core/ITask.h>
#include <xcommon/IXCamera.h>

#if defined(_DEBUG)
#pragma comment(lib, "mital_d.lib")
#else
#pragma comment(lib, "mital.lib")
#endif

//##########################################################################

class CObserverChangedEventListener: public IEventListener<XEventObserverChanged>
{
public:
	SX_ALIGNED_OP_MEM();

	CObserverChangedEventListener(IXCore *pCore)
	{
		m_pObserverChangedEventChannel = pCore->getEventChannel<XEventObserverChanged>(EVENT_OBSERVER_CHANGED_GUID);
		m_pObserverChangedEventChannel->addListener(this);
	}

	~CObserverChangedEventListener()
	{
		m_pObserverChangedEventChannel->removeListener(this);
	}

	void onEvent(const XEventObserverChanged *pData) override
	{
		ScopedSpinLock guard(m_spLock);
		m_vPos = pData->pCamera->getPosition();
		m_vUp = pData->pCamera->getUp();
		m_vLook = pData->pCamera->getLook();
	}

	void getObserverParam(float3 *pPos, float3 *pLook, float3 *pUp)
	{
		ScopedSpinLock guard(m_spLock);
		*pPos = m_vPos; *pLook = m_vLook; *pUp = m_vUp;
	}

protected:
	IEventChannel<XEventObserverChanged> *m_pObserverChangedEventChannel = NULL;
	float3 m_vPos, m_vLook, m_vUp;
	SpinLock m_spLock;
};

//##########################################################################

class CUpdateTask: public ITaskImpl<ITask>
{
public:
	CUpdateTask(CSoundSystem *pSoundSystem, CObserverChangedEventListener *pObserverListener):
		m_pSoundSystem(pSoundSystem),
		m_pObserverListener(pObserverListener),
		ITaskImpl(CORE_TASK_FLAG_BACKGROUND_REPEATING)
	{
	}

	~CUpdateTask()
	{
		mem_release(m_pSoundSystem);
	}

	void run() override
	{
		float3 vPos, vLook, vUp;

		if(m_pObserverListener)
			m_pObserverListener->getObserverParam(&vPos, &vLook, &vUp);

		if(m_pSoundSystem)
			m_pSoundSystem->update(vPos, vLook, vUp);
	}

	uint64_t getRepeatInterval() override
	{
		return(50000); // 50ms
	}

protected:
	CSoundSystem *m_pSoundSystem = NULL;
	CObserverChangedEventListener *m_pObserverListener = NULL;
};

#if 0
class CUpdatableSoundSystem: public IXUnknownImplementation<IXUpdatable>
{
public:
	CUpdatableSoundSystem(CSoundSystem *pSoundSystem, CObserverChangedEventListener *pObserverListener)
	{
		m_pSoundSystem = pSoundSystem;
		m_pObserverListener = pObserverListener;
	}

	UINT startup() override
	{
		return(1);
	}
	void shutdown() override
	{
		m_pSoundSystem->update(float3(), float3(), float3());
		mem_release(m_pSoundSystem);
	}

	ID run(float fDelta) override
	{
		float3 vPos, vLook, vUp;

		if(m_pObserverListener)
			m_pObserverListener->getObserverParam(&vPos, &vLook, &vUp);

		if(m_pSoundSystem)
			m_pSoundSystem->update(vPos, vLook, vUp);

		/*if (!m_pEmitter)
		{
			IXSoundLayer *pMasterLayer = m_pSoundSystem->findLayer("master");
			if (pMasterLayer)
			{
				m_pEmitter = pMasterLayer->newSoundEmitter("sounds/ak74_shoot.ogg", SOUND_DTYPE_3D);
				m_pEmitter->setWorldPos(float3(-11.084, 0.435, -18.707));
				m_pEmitter->setDistance(50);
				//pEmitter->play();
			}
		}
		else
		{
			if (GetAsyncKeyState('I'))
			{
				m_pEmitter->play();
				//Sleep(100);
			}
		}*/

		return(-1);
	}

	void sync() override
	{

	}

protected:
	CSoundSystem *m_pSoundSystem = NULL;
	CObserverChangedEventListener *m_pObserverListener = NULL;
	IXSoundEmitter *m_pEmitter = NULL;
};
#endif

//##########################################################################

class CSoundSystemPlugin: public IXUnknownImplementation<IXPlugin>
{
public:

	void XMETHODCALLTYPE startup(IXCore *pCore) override
	{
		m_pCore = pCore;
		m_pSoundSystem = new CSoundSystem(m_pCore);
		m_pObserverListener = new CObserverChangedEventListener(m_pCore);
		m_pCore->addTask(new CUpdateTask(m_pSoundSystem, m_pObserverListener));
		//m_pUpdatableSoundSystem = new CUpdatableSoundSystem(m_pSoundSystem, m_pObserverListener);
	}

	void XMETHODCALLTYPE shutdown() override
	{
	}

	UINT XMETHODCALLTYPE getInterfaceCount() override
	{
		return(1);
	}
	const XGUID* XMETHODCALLTYPE getInterfaceGUID(UINT id) override
	{
		static XGUID s_guid;
		switch(id)
		{
		case 0:
			s_guid = IXSOUNDSYSTEM_GUID;
			break;
		//case 1:
		//	s_guid = IXUPDATABLE_GUID;
		//	break;
		default:
			return(NULL);
		}
		return(&s_guid);
	}
	void XMETHODCALLTYPE getInterface(UINT id, void **ppOut) override
	{
		switch(id)
		{
		case 0:
			*ppOut = m_pSoundSystem;
			break;

		//case 1:
		//	*ppOut = m_pUpdatableSoundSystem;
		//	break;

		default:
			*ppOut = NULL;
		}
	}

protected:
	IXCore *m_pCore;
	CSoundSystem *m_pSoundSystem = NULL;
	//CUpdatableSoundSystem *m_pUpdatableSoundSystem = NULL;
	CObserverChangedEventListener *m_pObserverListener = NULL;
};

DECLARE_XPLUGIN(CSoundSystemPlugin);
