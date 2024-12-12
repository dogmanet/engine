#ifndef _COMMAND_DUPLICATE_H_
#define _COMMAND_DUPLICATE_H_

#include <xcommon/editor/IXEditorExtension.h>
#include "terrax.h"

#include <xcommon/editor/IXEditable.h>
#include "CommandPaste.h"

class CCommandDuplicate final: public IXUnknownImplementation<IXEditorCommand>
{
public:
	CCommandDuplicate();

	bool XMETHODCALLTYPE exec() override;

	bool XMETHODCALLTYPE undo() override;

	const char* XMETHODCALLTYPE getText() override
	{
		return("duplicate");
	}

	bool XMETHODCALLTYPE isEmpty() override
	{
		return(m_commandPaste.isEmpty());
	}

	
private:
	CCommandPaste m_commandPaste;
	bool m_isExecuted = false;

private:
	void initialize();

	void processObject(IXEditorObject *pObj);
};

#endif
