#ifndef _COMMAND_GROUP_H_
#define _COMMAND_GROUP_H_

#include <xcommon/editor/IXEditorExtension.h>
#include "terrax.h"

//#include <common/assotiativearray.h>
//#include <common/string.h>
//#include <xcommon/editor/IXEditable.h>

//#include "CommandCreate.h"
#include "GroupObject.h"

class CCommandGroup final: public IXUnknownImplementation<IXEditorCommand>
{
public:
	CCommandGroup();
	~CCommandGroup();

	bool XMETHODCALLTYPE exec() override;
	bool XMETHODCALLTYPE undo() override;

	const char* XMETHODCALLTYPE getText() override
	{
		return("group");
	}

	bool XMETHODCALLTYPE isEmpty() override
	{
		return(false);
	}

private:
	CGroupObject *m_pGroup = NULL;

	struct ObjLocation
	{
		IXEditorObject *pObj;
		ICompoundObject *pLocation;
	};
	Array<ObjLocation> m_aObjLocations;

	ICompoundObject *m_pCommonParent = NULL;

	float3_t m_vPos;

	bool m_isLocationsSaved = false;
	bool m_isCenterFound = false;

};

#endif
