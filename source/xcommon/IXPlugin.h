#ifndef __IXPLUGIN_H
#define __IXPLUGIN_H

#include "IXCore.h"
#include "IPluginManager.h"

#define X_PLUGIN_ENTRYPOINT XPluginMain
#define X_PLUGIN_API extern "C" __declspec(dllexport)
#define IXPLUGIN_VERSION 2
#define DECLARE_XPLUGIN(cls) IXProfiler *g_pProfiler = NULL;                                              \
class cls##Imp final: public cls{                                                                         \
public:                                                                                                   \
	cls##Imp(ID id):m_id(id){}                                                                            \
	ID XMETHODCALLTYPE getID()override{return(m_id);}                                                     \
	XIMPLEMENT_VERSION(IXPLUGIN_VERSION);                                                                 \
	void XMETHODCALLTYPE startup(IXCore *pCore)override{                                                  \
		INIT_OUTPUT_STREAM(pCore);                                                                        \
		g_pProfiler = (IXProfiler*)pCore->getPluginManager()->getInterface(IXPROFILER_GUID);              \
		cls::startup(pCore);                                                                              \
	}                                                                                                     \
protected:                                                                                                \
	ID m_id;                                                                                              \
}; X_PLUGIN_API IXPlugin *X_PLUGIN_ENTRYPOINT(ID id){return(new cls##Imp(id));}

class IXPlugin: public IXUnknown
{
public:
	virtual ID XMETHODCALLTYPE getID() = 0;

	virtual UINT XMETHODCALLTYPE getInterfaceCount() = 0;
	virtual const XGUID* XMETHODCALLTYPE getInterfaceGUID(UINT id) = 0;
	virtual void XMETHODCALLTYPE getInterface(UINT id, void **ppOut) = 0;

	virtual void XMETHODCALLTYPE startup(IXCore *pCore) = 0;
	virtual void XMETHODCALLTYPE shutdown() = 0;
};

typedef IXPlugin*(*PFNXPLUGINMAIN)(ID id);

#endif
