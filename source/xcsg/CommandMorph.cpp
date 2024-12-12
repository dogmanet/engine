#include "CommandMorph.h"
#include "Editable.h"
#include "VertexTool.h"

CCommandMorph::CCommandMorph(IXEditor *pEditor, CEditable *pEditable):
	m_pEditor(pEditor),
	m_pEditable(pEditable)
{
}

CCommandMorph::~CCommandMorph()
{
	for(UINT i = 0, l = m_aMorphedObjects.size(); i < l; ++i)
	{
		mem_release(m_aMorphedObjects[i].pObj);
	}
}

bool XMETHODCALLTYPE CCommandMorph::exec()
{
	if(!m_wasReverted)
	{
		if(m_shouldFail)
		{
			undo();
			return(false);
		}
		return(true);
	}

	CVertexTool *pVertexTool = m_pEditable->getVertexTool();

	fora(i, m_aMorphedObjects)
	{
		const morph_obj_t &mo = m_aMorphedObjects[i];
		mo.pObj->setKV("brush", mo.sAfter.c_str(), true);
		pVertexTool->setSelectedVerticesForObject(mo.pObj, mo.aSelectedVerticesAfter);
	}
	
	return(true);
}
bool XMETHODCALLTYPE CCommandMorph::undo()
{
	m_wasReverted = true;

	CVertexTool *pVertexTool = m_pEditable->getVertexTool();

	fora(i, m_aMorphedObjects)
	{
		const morph_obj_t &mo = m_aMorphedObjects[i];
		mo.pObj->setKV("brush", mo.sBefore.c_str(), true);
		pVertexTool->setSelectedVerticesForObject(mo.pObj, mo.aSelectedVerticesBefore);
	}
	
	return(true);
}

bool XMETHODCALLTYPE CCommandMorph::isEmpty()
{
	bool isNonEmpty = false;
	fora(i, m_aMorphedObjects)
	{
		if(m_aMorphedObjects[i].sAfter != m_aMorphedObjects[i].sBefore)
		{
			isNonEmpty = true;
			break;
		}
	}
	return(!isNonEmpty);
}

void CCommandMorph::addObject(CEditorObject *pObj, const UINT *puAffectedVertices, UINT uVertexCount)
{
	add_ref(pObj);

	morph_obj_t &mo = m_aMorphedObjects[m_aMorphedObjects.size()];
	mo.pObj = pObj;
	mo.sBefore = pObj->getKV("brush");
	mo.aSelectedVerticesBefore.reserve(uVertexCount);
	for(UINT i = 0; i < uVertexCount; ++i)
	{
		mo.aSelectedVerticesBefore[i] = puAffectedVertices[i];
	}
}
void CCommandMorph::finalizeObject(CEditorObject *pObj, const UINT *puAffectedVertices, UINT uVertexCount)
{
	int idx = m_aMorphedObjects.indexOf(pObj, [](const morph_obj_t &mo, CEditorObject *pObj){
		return(mo.pObj == pObj);
	});
	assert(idx >= 0);
	if(idx >= 0)
	{
		if(pObj->getVertexCount() < 4)
		{
			m_shouldFail = true;
		}

		morph_obj_t &mo = m_aMorphedObjects[idx];

		mo.sAfter = pObj->getKV("brush");
		mo.aSelectedVerticesAfter.reserve(uVertexCount);
		for(UINT i = 0; i < uVertexCount; ++i)
		{
			mo.aSelectedVerticesAfter[i] = puAffectedVertices[i];
		}
	}
}
