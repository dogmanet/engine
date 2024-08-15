#ifndef __PLUGINMANAGER_H
#define __PLUGINMANAGER_H

#include <xcommon/IPluginManager.h>
#include <common/assotiativearray.h>

// Интерфейс для работы с плагинами движка


class CPluginManager: public IXUnknownImplementation<IPluginManager>
{
public:
	~CPluginManager();

	IXPlugin *loadPlugin(const char *szPath);

	IXUnknown *getInterface(const XGUID &guid, UINT uStartFrom = 0);

	void registerInterface(const XGUID &guid, IXUnknown *pIFace);

	void invokeStartup(IXCore *pCore);

protected:

	struct _iface
	{
		IXUnknown *pIFace = NULL;
		IXPlugin *pPlugin = NULL;
		UINT uPluginVersion = 0;
		UINT uIFaceIndex = 0;
	};

	Array<IXPlugin*> m_aPlugins;

	AssotiativeArray<XGUID, Array<_iface>> m_maIFaces;
};

#endif
