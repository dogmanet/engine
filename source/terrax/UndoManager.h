#ifndef _UNDO_MANAGER_H_
#define _UNDO_MANAGER_H_

#include <common/array.h>
#include <xcommon/editor/IXEditorExtension.h>

class CUndoManager
{
public:
	~CUndoManager();

	bool canUndo();
	bool canRedo();

	const char *getUndoText();
	const char *getRedoText();

	bool execCommand(IXEditorCommand *pCommand, bool bSaveForUndo = true);
	void attachCommand(IXEditorCommand *pCommand);
	bool undo();
	bool redo();

	void reset();
	void flushRedo();

	bool isDirty();
	void makeClean();

	bool isInCommandContext();
protected:
	Array<IXEditorCommand*> m_stackUndo;
	Array<IXEditorCommand*> m_stackRedo;

	Array<IXEditorCommand*> *m_pAttachedCommands = NULL;

	int m_iLastSaveIndex = 0;

	int m_isInCommandContext = 0;
};

//#############################################################################

class CCommandContainer: public IXUnknownImplementation<IXEditorCommand>
{
public:
	~CCommandContainer();

	bool XMETHODCALLTYPE exec() override;
	bool XMETHODCALLTYPE undo() override;

	bool XMETHODCALLTYPE isEmpty() override;

	const char* XMETHODCALLTYPE getText() override;

	void addCommand(IXEditorCommand *pCommand);

private:
	Array<IXEditorCommand*> m_aCommands;
};

#endif
