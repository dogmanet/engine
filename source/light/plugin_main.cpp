#include <xcommon/IXPlugin.h>
#include "LightSystem.h"
#include "GIGraphNode.h"
#include "TonemappingGraphNode.h"
#include "Updatable.h"


class CLightSystemPlugin: public IXUnknownImplementation<IXPlugin>
{
public:
	void XMETHODCALLTYPE startup(IXCore *pCore) override
	{
		m_pCore = pCore;
	}
	void XMETHODCALLTYPE shutdown() override
	{
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
			s_guid = IXLIGHTSYSTEM_GUID;
			break;
		case 1:
		case 2:
			s_guid = IXRENDERGRAPHNODE_GUID;
			break;
		case 3:
			s_guid = IXUPDATABLE_GUID;
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
			if(!m_pLightSystem)
			{
				m_pLightSystem = new CLightSystem(m_pCore);
			}
			*ppOut = m_pLightSystem;
			break;

		case 1:
			if(!m_pLightSystem)
			{
				void *ptr;
				getInterface(0, &ptr);
			}
			*ppOut = new CGIGraphNode(m_pCore, m_pLightSystem);
			break;

		case 2:
			if(!m_pLightSystem)
			{
				void *ptr;
				getInterface(0, &ptr);
			}
			*ppOut = new CTonemappingGraphNode(m_pCore, m_pLightSystem);
			break;

		case 3:
			if(!m_pLightSystem)
			{
				void *ptr;
				getInterface(0, &ptr);
			}
			*ppOut = new CUpdatable(m_pLightSystem);
			break;
		
		default:
			*ppOut = NULL;
		}
	}

protected:
	IXCore *m_pCore;
	CLightSystem *m_pLightSystem = NULL;
};

DECLARE_XPLUGIN(CLightSystemPlugin);
