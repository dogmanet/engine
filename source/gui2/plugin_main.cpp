#include <xcommon/IXPlugin.h>
#include <xcommon/render/IXRender.h>
#include "GUI.h"
#include <gui2/CSSLexer.h>

#include "CSSProperty.h"
#include "FontManager.h"

class CGUIPlugin: public IXUnknownImplementation<IXPlugin>
{
public:
	void XMETHODCALLTYPE startup(IXCore *pCore) override
	{
		m_pCore = pCore;
		CCSSLexer lexer;
		lexer.read("<>");
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
			s_guid = IXFONTMANAGER_GUID;
			break;
		case 1:
			s_guid = IXGUI_GUID;
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
				IGXDevice *pDev = pRender->getDevice();
				*ppOut = new CFontManager(m_pCore, pDev, m_pCore->getFileSystem());
			}
			break;
		case 1:
			*ppOut = new CGUI();
			break;

		default:
			*ppOut = NULL;
		}
	}

protected:
	IXCore *m_pCore;
};

DECLARE_XPLUGIN(CGUIPlugin);
