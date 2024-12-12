#include <xcommon/IXPlugin.h>
#include "Scene.h"
#include "Updatable.h"
#include "RenderUtils.h"
#include "Render.h"
#include "GBufferGraphNode.h"
#include "GameUIGraphNode.h"
#include "FXAAGraphNode.h"
#include "TransparentGraphNode.h"
#include "SceneFinalGraphNode.h"

#if defined(_DEBUG)
#pragma comment(lib, "sxgame_d.lib")
#else
#pragma comment(lib, "sxgame.lib")
#endif

class CRenderPlugin: public IXUnknownImplementation<IXPlugin>
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
		return(9);
	}
	const XGUID* XMETHODCALLTYPE getInterfaceGUID(UINT id) override
	{
		static XGUID s_guid;
		switch(id)
		{
		case 0:
			s_guid = IXSCENE_GUID;
			break;
		case 1:
			s_guid = IXUPDATABLE_GUID;
			break;
		case 2:
			s_guid = IXRENDER_GUID;
			break;
		case 3:
			s_guid = IXRENDERUTILS_GUID;
			break;
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
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
			if(!m_pScene)
			{
				m_pScene = new CScene(m_pCore);
			}
			*ppOut = m_pScene;
			break;

		case 1:
			if(!m_pUpdatable)
			{
				if(!m_pScene)
				{
					void *ptr;
					getInterface(0, &ptr);
				}
				m_pUpdatable = new CUpdatable(m_pScene);
			}
			*ppOut = m_pUpdatable;
			break;

		case 2:
			if(!m_pRender)
			{
				m_pRender = new CRender(m_pCore);
				*ppOut = m_pRender;

				if(!m_pUpdatable)
				{
					void *ptr;
					getInterface(1, &ptr);
				}

				m_pUpdatable->setRender(m_pRender);
			}
			else
			{
				*ppOut = m_pRender;
			}
			break;

		case 3:
			if(!m_pRender)
			{
				void *ptr;
				getInterface(2, &ptr);
			}
			*ppOut = m_pRender->getUtils();
			break;

		case 4:
			*ppOut = new CGBufferGraphNode(m_pCore);
			break;

		case 5:
			*ppOut = new CSceneFinalGraphNode(m_pCore);
			break;

		case 6:
			*ppOut = new CGameUIGraphNode(m_pCore);
			break;

		case 7:
			*ppOut = new CFXAAGraphNode(m_pCore);
			break;

		case 8:
			*ppOut = new CTransparentGraphNode(m_pCore);
			break;

		default:
			*ppOut = NULL;
		}
	}

protected:
	IXCore *m_pCore;
	CScene *m_pScene = NULL;
	CUpdatable *m_pUpdatable = NULL;
	CRender *m_pRender = NULL;
};

DECLARE_XPLUGIN(CRenderPlugin);
