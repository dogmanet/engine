#include "PluginManager.h"

#if defined(_LINUX) || defined(_MAC)
#	include <dlfcn.h>
#endif

typedef IXUnknown* (XMETHODCALLTYPE IXPlugin::*IXPLUGIN_GET_INTERFACE)(const XGUID &guid);

CPluginManager::~CPluginManager()
{
	for(UINT i = 0, l = m_aPlugins.size(); i < l; ++i)
	{
		m_aPlugins[i]->shutdown();
		mem_release(m_aPlugins[i]);
	}
}

void CPluginManager::invokeStartup(IXCore *pCore)
{
	for(UINT i = 0, l = m_aPlugins.size(); i < l; ++i)
	{
		m_aPlugins[i]->startup(pCore);
	}
}

IXPlugin *CPluginManager::loadPlugin(const char *szPath)
{
	PFNXPLUGINMAIN pfnXPluginMain = NULL;
#if defined(_WINDOWS)
	HMODULE dmod = LoadLibraryA(szPath);
	if(!dmod)
	{
		return(NULL);	
	}
	pfnXPluginMain = (PFNXPLUGINMAIN)GetProcAddress(dmod, MACRO_TEXT(X_PLUGIN_ENTRYPOINT));
#endif
#if defined(_LINUX) || defined(_MAC)
	void *dmod = dlopen(szPath, RTLD_LAZY);
	if(!dmod)
	{
		return(NULL);
	}
	pfnXPluginMain = (PFNXPLUGINMAIN)dlsym(mod, MACRO_TEXT(X_PLUGIN_ENTRYPOINT));
#endif

	if(pfnXPluginMain)
	{
		IXPlugin *pPlugin = pfnXPluginMain(m_aPlugins.size());
		if(pPlugin)
		{
			UINT uVersion = pPlugin->getVersion();
			if(uVersion == IXPLUGIN_VERSION || uVersion == 1)
			{
				//char tmp[128];
				UINT uIFaceCount = pPlugin->getInterfaceCount();
				_iface iface;
				iface.pPlugin = pPlugin;
				iface.uPluginVersion = uVersion;
				for(UINT i = 0; i < uIFaceCount; ++i)
				{
					iface.uIFaceIndex = i;
					m_maIFaces[*pPlugin->getInterfaceGUID(i)].push_back(iface);
					//XGUIDToSting(*pPlugin->getInterfaceGUID(i), tmp, sizeof(tmp));
					//LogInfo("Registered iface %s\n", tmp);
				}

				m_aPlugins.push_back(pPlugin);

				return(pPlugin);
			}
			else
			{
				printf(COLOR_LRED "Invalid version! " COLOR_RESET);
				mem_release(pPlugin);
			}
		}
	}

#if defined(_WINDOWS)
	FreeLibrary(dmod);
#endif
#if defined(_LINUX) || defined(_MAC)
	dlclose(dmod);
#endif
	return(NULL);
}

IXUnknown *CPluginManager::getInterface(const XGUID &guid, UINT uStartFrom)
{
	const AssotiativeArray<XGUID, Array<_iface>>::Node *pNode;
	if(m_maIFaces.KeyExists(guid, &pNode))
	{
		Array<_iface> &arr = *pNode->Val;
		if(arr.size() <= uStartFrom)
		{
			return(NULL);
		}
		_iface &iface = arr[uStartFrom];
		if(!iface.pIFace)
		{
			if(iface.uPluginVersion == 1)
			{
				IXPLUGIN_GET_INTERFACE pfnGetInterface = (IXPLUGIN_GET_INTERFACE)&IXPlugin::getInterface;

				iface.pIFace = (iface.pPlugin->*pfnGetInterface)(guid);
			}
			else
			{
				iface.pPlugin->getInterface(iface.uIFaceIndex, (void**)&iface.pIFace);
			}
		}
		return(iface.pIFace);
	}
	return(NULL);
}

void CPluginManager::registerInterface(const XGUID &guid, IXUnknown *pIFace)
{
	_iface iface;
	iface.pIFace = pIFace;
	m_maIFaces[guid].push_back(iface);
}
