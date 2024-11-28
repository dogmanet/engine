#ifndef _COMMAND_MODIFY_MODEL_H_
#define _COMMAND_MODIFY_MODEL_H_

#include <xcommon/editor/IXEditorExtension.h>
#include "terrax.h"

#include <xcommon/editor/IXEditable.h>

#include "ProxyObject.h"

class CCommandModifyModel final: public IXUnknownImplementation<IXEditorCommand>
{
public:
	CCommandModifyModel(CProxyObject *pProxy);
	~CCommandModifyModel();

	bool XMETHODCALLTYPE exec() override;
	bool XMETHODCALLTYPE undo() override;

	const char* XMETHODCALLTYPE getText() override
	{
		return("to object");
	}

	bool XMETHODCALLTYPE isEmpty() override
	{
		return(false);
	}

private:
	CProxyObject *m_pProxy = NULL;

	struct ObjLocation
	{
		IXEditorObject *pObj;
		ICompoundObject *pLocation;
	};
	Array<ObjLocation> m_aObjLocations;
	bool m_isLocationsSaved = false;
	Array<IXEditorObject*> m_aObjects;
};

#endif
