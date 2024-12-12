#include <xcommon/IXPlugin.h>
#include <xcommon/XEvents.h>
#include "Renderable.h"
#include "Updatable.h"
#include "AnimatedModelProvider.h"
#include "DynamicModelProvider.h"

class CLevelSizeEventListener final: public IEventListener<XEventLevelSize>
{
public:
	CLevelSizeEventListener(CAnimatedModelProvider *pAnimatedModelProvider, CDynamicModelProvider *pDynamicModelProvider):
		m_pAnimatedModelProvider(pAnimatedModelProvider),
		m_pDynamicModelProvider(pDynamicModelProvider)
	{
	}
	void onEvent(const XEventLevelSize *pData)
	{
		m_pDynamicModelProvider->getLevelSize(pData);
		m_pAnimatedModelProvider->getLevelSize(pData);
	}

protected:
	CAnimatedModelProvider *m_pAnimatedModelProvider;
	CDynamicModelProvider *m_pDynamicModelProvider;
};

class CLoadLevelEventListener final: public IEventListener<XEventLevel>
{
public:
	CLoadLevelEventListener(CRenderable *pRenderable):
		m_pRenderable(pRenderable)
	{}
	void onEvent(const XEventLevel *pData)
	{
		switch(pData->type)
		{
		case XEventLevel::TYPE_LOAD_BEGIN:
			m_pRenderable->setEnabled(false);
			break;
		case XEventLevel::TYPE_LOAD_END:
			m_pRenderable->setEnabled(true);
			break;

		default:
			break;
		}
	}

protected:
	CRenderable *m_pRenderable;
};

class CDSEPlugin: public IXUnknownImplementation<IXPlugin>
{
public:
	void init()
	{
		m_pAnimatedModelProvider = new CAnimatedModelProvider(m_pCore);
		m_pDynamicModelProvider = new CDynamicModelProvider(m_pCore);
		m_pRenderable = new CRenderable(getID(), m_pAnimatedModelProvider, m_pDynamicModelProvider);
		m_pUpdatable = new CUpdatable(m_pAnimatedModelProvider, m_pDynamicModelProvider);
		m_pLevelSizeEventListener = new CLevelSizeEventListener(m_pAnimatedModelProvider, m_pDynamicModelProvider);
		m_pLevelLoadEventListener = new CLoadLevelEventListener(m_pRenderable);

		m_pCore->getEventChannel<XEventLevelSize>(EVENT_LEVEL_GET_SIZE_GUID)->addListener(m_pLevelSizeEventListener);
		m_pCore->getEventChannel<XEventLevel>(EVENT_LEVEL_GUID)->addListener(m_pLevelLoadEventListener);
	}

	void XMETHODCALLTYPE startup(IXCore *pCore) override
	{
		m_pCore = pCore;
	}

	void XMETHODCALLTYPE shutdown() override
	{
		if(m_pLevelSizeEventListener)
		{
			m_pCore->getEventChannel<XEventLevelSize>(EVENT_LEVEL_GET_SIZE_GUID)->removeListener(m_pLevelSizeEventListener);
		}
		mem_delete(m_pLevelSizeEventListener);
		mem_delete(m_pRenderable);
		mem_delete(m_pUpdatable);
		mem_delete(m_pAnimatedModelProvider);
		mem_delete(m_pDynamicModelProvider);
	}

	UINT XMETHODCALLTYPE getInterfaceCount() override
	{
		return(4);
	}
	const XGUID* XMETHODCALLTYPE getInterfaceGUID(UINT id) override
	{
		static XGUID s_guid;
		switch(id)
		{
		case 0:
			s_guid = IXRENDERABLE_GUID;
			break;
		case 1:
			s_guid = IXUPDATABLE_GUID;
			break;
		case 2:
			s_guid = IXANIMATEDMODELPROVIDER_GUID;
			break;
		case 3:
			s_guid = IXDYNAMICMODELPROVIDER_GUID;
			break;
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
			if(!m_pRenderable)
			{
				init();
			}
			add_ref(m_pRenderable);
			*ppOut = m_pRenderable;
			break;

		case 1:
			if(!m_pUpdatable)
			{
				init();
			}
			add_ref(m_pUpdatable);
			*ppOut = m_pUpdatable;
			break;

		case 2:
			if(!m_pAnimatedModelProvider)
			{
				init();
			}
			add_ref(m_pAnimatedModelProvider);
			*ppOut = m_pAnimatedModelProvider;
			break;

		case 3:
			if(!m_pDynamicModelProvider)
			{
				init();
			}
			add_ref(m_pDynamicModelProvider);
			*ppOut = m_pDynamicModelProvider;
			break;
		
		default:
			*ppOut = NULL;
		}
	}

protected:
	CRenderable *m_pRenderable = NULL;
	CUpdatable *m_pUpdatable = NULL;
	IXCore *m_pCore = NULL;
	CAnimatedModelProvider *m_pAnimatedModelProvider = NULL;
	CDynamicModelProvider *m_pDynamicModelProvider = NULL;
	CLevelSizeEventListener *m_pLevelSizeEventListener = NULL;
	CLoadLevelEventListener *m_pLevelLoadEventListener = NULL;
};

DECLARE_XPLUGIN(CDSEPlugin);
