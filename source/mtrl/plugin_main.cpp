#include <xcommon/IXPlugin.h>
#include "MaterialSystem.h"
#include "Updatable.h"

#if defined(_DEBUG)
#pragma comment(lib, "sxcore_d.lib")
#else
#pragma comment(lib, "sxcore.lib")
#endif

class CMtrlPlugin: public IXUnknownImplementation<IXPlugin>
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
		return(2);
	}
	const XGUID* XMETHODCALLTYPE getInterfaceGUID(UINT id) override
	{
		static XGUID s_guid;
		switch(id)
		{
		case 0:
			s_guid = IXMATERIALSYSTEM_GUID;
			break;
		case 1:
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
			if(!m_pMaterialSystem)
			{
				m_pMaterialSystem = new CMaterialSystem(m_pCore);
			}
			*ppOut = m_pMaterialSystem;
			break;

		case 1:
			if(!m_pUpdatable)
			{
				if(!m_pMaterialSystem)
				{
					void *ptr;
					getInterface(0, &ptr);
				}
				m_pUpdatable = new CUpdatable(m_pMaterialSystem);
			}
			*ppOut = m_pUpdatable;
			break;
		
		default:
			*ppOut = NULL;
		}
	}

protected:
	IXCore *m_pCore;
	CMaterialSystem *m_pMaterialSystem = NULL;
	CUpdatable *m_pUpdatable = NULL;
};

DECLARE_XPLUGIN(CMtrlPlugin);
