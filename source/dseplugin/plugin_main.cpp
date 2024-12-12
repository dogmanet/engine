#include <xcommon/IXPlugin.h>
#include "ModelLoader.h"
#include "ModelWriter.h"


class CDSEPlugin: public IXUnknownImplementation<IXPlugin>
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
			s_guid = IXMODELLOADER_GUID;
			break;
		case 1:
			s_guid = IXMODELWRITER_GUID;
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
			*ppOut = new CModelLoader(m_pCore->getFileSystem());
			break;
		case 1:
			*ppOut = new CModelWriter();
			break;
		
		default:
			*ppOut = NULL;
		}
	}

private:
	IXCore *m_pCore = NULL;
};

DECLARE_XPLUGIN(CDSEPlugin);
