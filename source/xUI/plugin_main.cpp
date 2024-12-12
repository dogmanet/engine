#include <xcommon/IXPlugin.h>
#include <xcommon/render/IXRender.h>
#include "XUI.h"
#include "CurvePreviewGraphNode.h"

TODO("Remove direct dependency");
#if defined(_DEBUG)
#pragma comment(lib, "sxgame_d.lib")
#else
#pragma comment(lib, "sxgame.lib")
#endif
#include <game/sxgame.h>

class CXUIPlugin: public IXUnknownImplementation<IXPlugin>
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
			s_guid = IXUI_GUID;
			break;
		case 1:
			s_guid = IXRENDERGRAPHNODE_GUID;
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
			{
				IXRender *pRender = (IXRender*)m_pCore->getPluginManager()->getInterface(IXRENDER_GUID);
				IXWindowSystem *pWindowSystem = (IXWindowSystem*)m_pCore->getPluginManager()->getInterface(IXWINDOWSYSTEM_GUID);
				*ppOut = new CXUI(m_pCore, pRender, pWindowSystem, SGame_GetGUI());
			}
			break;
		case 1:
			*ppOut = new CCurvePreviewGraphNode(m_pCore);
			break;

		default:
			*ppOut = NULL;
		}
	}

protected:
	IXCore *m_pCore;
};

DECLARE_XPLUGIN(CXUIPlugin);
