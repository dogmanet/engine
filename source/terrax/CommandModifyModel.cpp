#include "CommandModifyModel.h"
#include <common/aastring.h>

extern AssotiativeArray<AAString, IXEditable*> g_mEditableSystems;

CCommandModifyModel::CCommandModifyModel(CProxyObject *pProxy):
	m_pProxy(pProxy)
{
	add_ref(pProxy);
}

CCommandModifyModel::~CCommandModifyModel()
{
	mem_release(m_pProxy);
	fora(i, m_aObjects)
	{
		mem_release(m_aObjects[i]);
	}
}

bool XMETHODCALLTYPE CCommandModifyModel::exec()
{
	if(!m_isLocationsSaved)
	{
		IXEditorObject *pObj;
		const Map<IXEditorObject*, ICompoundObject*>::Node *pNode;
		for(auto i = g_mObjectsLocation.begin(); i; ++i)
		{
			if((*i.first)->isSelected() && *i.second != m_pProxy)
			{
				m_aObjLocations.push_back({*i.first, *i.second});
			}
		}

		m_isLocationsSaved = true;
	}
	
	fora(i, m_aObjLocations)
	{
		ObjLocation &loc = m_aObjLocations[i];
		loc.pLocation->removeChildObject(loc.pObj);
	}
	// all objects should be taken from containers at that point
	
	forar(i, g_pLevelObjects)
	{
		IXEditorObject *pObj = g_pLevelObjects[i];
		if(pObj != m_pProxy && pObj->isSelected())
		{
			add_ref(pObj);
			m_aObjects.push_back(pObj);
			m_pProxy->addChildObject(pObj);
		}
	}

	//m_pProxy->build();

	return(true);
}
bool XMETHODCALLTYPE CCommandModifyModel::undo()
{
	forar(i, m_aObjects)
	{
		m_pProxy->removeChildObject(m_aObjects[i]);
		mem_release(m_aObjects[i]);
		m_aObjects.erase(i);
	}

	// restore object locations
	fora(i, m_aObjLocations)
	{
		ObjLocation &loc = m_aObjLocations[i];
		loc.pLocation->addChildObject(loc.pObj);
	}

	return(true);
}
