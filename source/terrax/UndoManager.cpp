#include "UndoManager.h"

void XUpdateWindowTitle();

CUndoManager::~CUndoManager()
{
	reset();
}

bool CUndoManager::canUndo()
{
	return(m_stackUndo.size() != 0);
}

bool CUndoManager::canRedo()
{
	return(m_stackRedo.size() != 0);
}

const char *CUndoManager::getUndoText()
{
	if(!canUndo())
	{
		return(NULL);
	}

	return(m_stackUndo[m_stackUndo.size() - 1]->getText());
}
const char *CUndoManager::getRedoText()
{
	if(!canRedo())
	{
		return(NULL);
	}

	return(m_stackRedo[m_stackRedo.size() - 1]->getText());
}

void CUndoManager::reset()
{
	for(UINT i = 0, l = m_stackRedo.size(); i < l; ++i)
	{
		mem_release(m_stackRedo[i]);
	}
	m_stackRedo.clearFast();

	for(UINT i = 0, l = m_stackUndo.size(); i < l; ++i)
	{
		mem_release(m_stackUndo[i]);
	}
	m_stackUndo.clearFast();

	m_iLastSaveIndex = 0;

	XUpdateWindowTitle();
}
void CUndoManager::flushRedo()
{
	for(UINT i = 0, l = m_stackRedo.size(); i < l; ++i)
	{
		mem_release(m_stackRedo[i]);
	}
	m_stackRedo.clearFast();

	if(m_iLastSaveIndex > m_stackUndo.size()) 
	{
		m_iLastSaveIndex = -1;
	}
}

bool CUndoManager::execCommand(IXEditorCommand *pCommand, bool bSaveForUndo)
{
	if(!bSaveForUndo)
	{
		++m_isInCommandContext;
		bool isSuccessful = !pCommand->isEmpty() && pCommand->exec();
		--m_isInCommandContext;
		return(isSuccessful);
	}

	Array<IXEditorCommand*> *pOldAttachedCommands = m_pAttachedCommands;
	Array<IXEditorCommand*> aAttachedCommands;
	m_pAttachedCommands = &aAttachedCommands;
	++m_isInCommandContext;
	if(!pCommand->isEmpty() && pCommand->exec())
	{

		if(aAttachedCommands.size())
		{
			// create new command container
			CCommandContainer *pContainer = new CCommandContainer();
			pContainer->addCommand(pCommand);
			fora(i, aAttachedCommands)
			{
				aAttachedCommands[i]->exec();
				pContainer->addCommand(aAttachedCommands[i]);
			}
			m_stackUndo.push_back(pContainer);
		}
		else
		{
			m_stackUndo.push_back(pCommand);
		}

		--m_isInCommandContext;
		flushRedo();

		XUpdateWindowTitle();
		m_pAttachedCommands = pOldAttachedCommands;
		return(true);
	}
	--m_isInCommandContext;
	fora(i, aAttachedCommands)
	{
		mem_release(aAttachedCommands[i]);
	}
	mem_release(pCommand);
	m_pAttachedCommands = pOldAttachedCommands;
	return(false);
}
void CUndoManager::attachCommand(IXEditorCommand *pCommand)
{
	if(!m_pAttachedCommands)
	{
		if(isInCommandContext())
		{
			mem_release(pCommand);
			return;
		}
		LibReport(REPORT_MSG_LEVEL_FATAL, "CUndoManager::attachCommand() is only available in exec context!");
	}

	add_ref(pCommand);
	m_pAttachedCommands->push_back(pCommand);
}
bool CUndoManager::undo()
{
	assert(!isInCommandContext());

	if(!canUndo() || isInCommandContext())
	{
		return(false);
	}

	IXEditorCommand *pCmd = m_stackUndo[m_stackUndo.size() - 1];
	m_isInCommandContext = true;
	if(pCmd->undo())
	{
		m_isInCommandContext = false;
		m_stackUndo.erase(m_stackUndo.size() - 1);
		m_stackRedo.push_back(pCmd);
		XUpdateWindowTitle();
		return(true);
	}
	m_isInCommandContext = false;
	return(false);
}
bool CUndoManager::redo()
{
	assert(!isInCommandContext());

	if(!canRedo() || isInCommandContext())
	{
		return(false);
	}

	IXEditorCommand *pCmd = m_stackRedo[m_stackRedo.size() - 1];
	m_isInCommandContext = true;
	if(pCmd->exec())
	{
		m_isInCommandContext = false;
		m_stackRedo.erase(m_stackRedo.size() - 1);
		m_stackUndo.push_back(pCmd);
		XUpdateWindowTitle();
		return(true);
	}
	m_isInCommandContext = false;
	return(false);
}

bool CUndoManager::isDirty()
{
	return(m_stackUndo.size() != m_iLastSaveIndex);
}

void CUndoManager::makeClean()
{
	m_iLastSaveIndex = m_stackUndo.size();
	XUpdateWindowTitle();
}

bool CUndoManager::isInCommandContext()
{
	return(m_isInCommandContext);
}

//#############################################################################

CCommandContainer::~CCommandContainer()
{
	fora(i, m_aCommands)
	{
		mem_release(m_aCommands[i]);
	}
}

bool XMETHODCALLTYPE CCommandContainer::exec()
{
	bool isSuccess = true;
	fora(i, m_aCommands)
	{
		isSuccess &= m_aCommands[i]->exec();
	}
	return(isSuccess);
}
bool XMETHODCALLTYPE CCommandContainer::undo()
{
	bool isSuccess = true;
	for(int i = m_aCommands.size() - 1; i >= 0; --i)
	{
		isSuccess &= m_aCommands[i]->undo();
	}
	return(isSuccess);
}

bool XMETHODCALLTYPE CCommandContainer::isEmpty()
{
	return(false);
}

const char* XMETHODCALLTYPE CCommandContainer::getText()
{
	return(m_aCommands[0]->getText());
}

void CCommandContainer::addCommand(IXEditorCommand *pCommand)
{
	m_aCommands.push_back(pCommand);
}
