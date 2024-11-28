#ifndef _COMMAND_BUILD_MODEL_H_
#define _COMMAND_BUILD_MODEL_H_

#include <xcommon/editor/IXEditorExtension.h>
#include "terrax.h"

#include <common/assotiativearray.h>
#include <common/string.h>
#include <xcommon/editor/IXEditable.h>

#include "CommandCreate.h"
#include "ProxyObject.h"

class CCommandBuildModel final: public IXUnknownImplementation<IXEditorCommand>
{
public:
	CCommandBuildModel(const char *szTypeName, const char *szClassName);
	~CCommandBuildModel();

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
	CCommandCreate *m_pCommandCreate;

	Array<IXEditorModel*> m_aModels;

	CProxyObject *m_pProxy = NULL;

	struct ObjLocation
	{
		IXEditorObject *pObj;
		ICompoundObject *pLocation;
	};
	Array<ObjLocation> m_aObjLocations;
	bool m_isLocationsSaved = false;
	bool m_isCenterFound = false;

	float3_t m_vPos;

	struct ObjModel
	{
		IXEditorObject *pObj;
		IXEditorModel *pModel;
	};
	Array<ObjModel> aObjModels;
	//SMQuaternion m_qOrient;
	//float m_fScale = 1.0f;
	//String m_sClassName;
	//IXEditable *m_pEditable = NULL;
	//IXEditorObject *m_pObject = NULL;
};

#endif
