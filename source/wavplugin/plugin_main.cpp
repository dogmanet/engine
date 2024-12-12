/*****************************************************
Copyright © DogmaNet Team, 2020
Site: dogmanet.ru
See the license in LICENSE
*****************************************************/

#include <xcommon/IXPlugin.h>
#include "AudioCodecWave.h"

//##########################################################################

class CAudioCodecWav: public IXUnknownImplementation<IXPlugin>
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
	const XGUID * XMETHODCALLTYPE getInterfaceGUID(UINT id) override
	{
		static XGUID s_guid;
		switch(id)
		{
		case 0:
			s_guid = IXAUDIOCODEC_GUID;
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
			*ppOut = new CAudioCodecWave(m_pCore->getFileSystem());
			break;
		
		default:
			*ppOut = NULL;
		}
	}

protected:
	IXCore *m_pCore = NULL;
};

DECLARE_XPLUGIN(CAudioCodecWav);
