#ifndef _COMMAND_UNGROUP_H_
#define _COMMAND_UNGROUP_H_

#include <xcommon/editor/IXEditorExtension.h>
#include "terrax.h"

#include <common/assotiativearray.h>
#include <common/string.h>
#include <xcommon/editor/IXEditable.h>

#include "CommandDelete.h"
#include "ProxyObject.h"

class CCommandUngroup final: public IXUnknownImplementation<IXEditorCommand>
{
public:
	CCommandUngroup(CGroupObject *pObj);
	~CCommandUngroup();

	bool XMETHODCALLTYPE exec() override;
	bool XMETHODCALLTYPE undo() override;

	const char* XMETHODCALLTYPE getText() override
	{
		return("ungroup");
	}

	bool XMETHODCALLTYPE isEmpty() override
	{
		return(false);
	}

private:
	CGroupObject *m_pGroup = NULL;
	
	Array<IXEditorObject*> m_aObjects;
};

#endif
