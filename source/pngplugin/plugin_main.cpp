#include <xcommon/IXPlugin.h>
#include "TextureLoader.h"

#ifdef _DEBUG
#	pragma comment(lib, "libpng16_d.lib")
#	pragma comment(lib, "zlib_d.lib")
#else
#	pragma comment(lib, "libpng16.lib")
#	pragma comment(lib, "zlib.lib")
#endif

class CDDSPlugin: public IXUnknownImplementation<IXPlugin>
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
		return(1);
	}
	const XGUID* XMETHODCALLTYPE getInterfaceGUID(UINT id) override
	{
		static XGUID s_guid;
		switch(id)
		{
		case 0:
			s_guid = IXTEXTURELOADER_GUID;
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
			*ppOut = new CTextureLoader(m_pCore->getFileSystem());
			break;
		
		default:
			*ppOut = NULL;
		}
	}

protected:
	IXCore *m_pCore;
};

DECLARE_XPLUGIN(CDDSPlugin);
