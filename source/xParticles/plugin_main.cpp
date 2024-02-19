#include <xcommon/IXPlugin.h>
#include "ParticleSystem.h"
#include "Updatable.h"
#include "Renderable.h"
#include "Editable.h"


class CEFFPlugin: public IXUnknownImplementation<IXPlugin>
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
			s_guid = IXPARTICLESYSTEM_GUID;
			break;
		case 1:
			s_guid = IXUPDATABLE_GUID;
			break;
		case 2:
			s_guid = IXRENDERABLE_GUID;
			break;
		case 3:
			s_guid = IXEDITABLE_GUID;
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
			if(!m_pParticleSystem)
			{
				m_pParticleSystem = new CParticleSystem(m_pCore);
			}
			*ppOut = m_pParticleSystem;
			break;

		case 1:
			if(!m_pUpdatable)
			{
				if(!m_pParticleSystem)
				{
					void *ptr;
					getInterface(0, &ptr);
				}
				m_pUpdatable = new CUpdatable(m_pParticleSystem);
			}
			*ppOut = m_pUpdatable;
			break;

		case 2:
			if(!m_pRenderable)
			{
				if(!m_pParticleSystem)
				{
					void *ptr;
					getInterface(0, &ptr);
				}
				m_pRenderable = new CRenderable(getID(), m_pParticleSystem);
			}
			*ppOut = m_pRenderable;
			break;

		case 3:
			if(!m_pEditable)
			{
				if(!m_pParticleSystem)
				{
					void *ptr;
					getInterface(0, &ptr);
				}
				m_pEditable = new CEditable(m_pCore, m_pParticleSystem);
			}
			*ppOut = m_pEditable;
			break;
		
		default:
			*ppOut = NULL;
		}
	}

protected:
	IXCore *m_pCore;
	CParticleSystem *m_pParticleSystem = NULL;
	CUpdatable *m_pUpdatable = NULL;
	CRenderable *m_pRenderable = NULL;
	CEditable *m_pEditable = NULL;
};

DECLARE_XPLUGIN(CEFFPlugin);
