#ifndef __COMMAND_MORPH_H
#define __COMMAND_MORPH_H

#include <xcommon/editor/IXEditorExtension.h>

//#include <common/assotiativearray.h>
//#include <common/string.h>
#include <xcommon/editor/IXEditable.h>
#include "EditorObject.h"

class CCommandMorph final: public IXUnknownImplementation<IXEditorCommand>
{
public:
	CCommandMorph(IXEditor *pEditor, CEditable *pEditable);
	~CCommandMorph();

	bool XMETHODCALLTYPE exec() override;
	bool XMETHODCALLTYPE undo() override;

	const char* XMETHODCALLTYPE getText() override
	{
		return("morphing");
	}

	bool XMETHODCALLTYPE isEmpty() override;

	void addObject(CEditorObject *pObj, const UINT *puAffectedVertices, UINT uVertexCount);
	void finalizeObject(CEditorObject *pObj, const UINT *puAffectedVertices, UINT uVertexCount);

private:
	IXEditor *m_pEditor = NULL;
	CEditable *m_pEditable = NULL;

	bool m_wasReverted = false;
	bool m_shouldFail = false;

	struct morph_obj_t
	{
		CEditorObject *pObj;
		String sBefore;
		String sAfter;
		Array<UINT> aSelectedVerticesBefore;
		Array<UINT> aSelectedVerticesAfter;
	};

	Array<morph_obj_t> m_aMorphedObjects;
};

#endif
