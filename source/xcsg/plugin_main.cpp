#include <xcommon/IXPlugin.h>
#include "Editable.h"
#include <game/sxgame.h>

#if defined(_DEBUG)
#pragma comment(lib, "sxgame_d.lib")
#else
#pragma comment(lib, "sxgame.lib")
#endif

class CCSGPlugin;
class CLevelLoadListener: public IEventListener<XEventLevel>
{
public:
	CLevelLoadListener(CCSGPlugin *pPlugin):
		m_pPlugin(pPlugin)
	{
	}

	void onEvent(const XEventLevel *pData) override;

protected:
	CCSGPlugin *m_pPlugin;
};


class CCSGPlugin: public IXUnknownImplementation<IXPlugin>
{
public:
	void XMETHODCALLTYPE startup(IXCore *pCore) override
	{
		m_pCore = pCore;

		m_pEventListener = new CLevelLoadListener(this);

		m_pCore->getEventChannel<XEventLevel>(EVENT_LEVEL_GUID)->addListener(m_pEventListener);
	}

	void XMETHODCALLTYPE shutdown() override
	{
		m_pCore->getEventChannel<XEventLevel>(EVENT_LEVEL_GUID)->removeListener(m_pEventListener);
		mem_delete(m_pEventListener);
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
			s_guid = IXEDITABLE_GUID;
			break;
		//case 1:
		//	s_guid = IXMATERIALPROXY_GUID;
		//	break;
		//case 2:
		//	s_guid = IXMATERIALLOADER_GUID;
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
			if(!m_pEditable)
			{
				m_pEditable = new CEditable(m_pCore);
			}
			*ppOut = m_pEditable;
			break;
		
		default:
			*ppOut = NULL;
		}
	}

	void onLevelEvent(const XEventLevel *pData)
	{
		switch(pData->type)
		{
		case XEventLevel::TYPE_LOAD:
			SAFE_CALL(m_pEditable, load, pData->szLevelName, getID())
			else
			{
				char szFile[1024];
				sprintf(szFile, "levels/%s/xcsg/", pData->szLevelName);
				IFileIterator *pIter = m_pCore->getFileSystem()->getFileList(szFile, "dse");
				if(pIter)
				{
					const char *szModel;
					while((szModel = pIter->next()))
					{
						// TODO Use more clean way to insert object!
						CBaseEntity *pEntity = SGame_CreateEntity("prop_static");
						pEntity->setKV("auto_physbox", "0");
						pEntity->setKV("model", szModel);

						pEntity->setFlags(pEntity->getFlags() | EF_LEVEL);
					}

					mem_release(pIter);
				}
			}

			break;
		case XEventLevel::TYPE_UNLOAD:
			SAFE_CALL(m_pEditable, unload);
			break;
		case XEventLevel::TYPE_SAVE:
			SAFE_CALL(m_pEditable, save, pData->szLevelName);
			break;
		}
	}


protected:
	IXCore *m_pCore;
	CLevelLoadListener *m_pEventListener = NULL;
	CEditable *m_pEditable = NULL;
};

DECLARE_XPLUGIN(CCSGPlugin);


void CLevelLoadListener::onEvent(const XEventLevel *pData)
{
	m_pPlugin->onLevelEvent(pData);
}