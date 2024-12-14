#include "CommandUngroup.h"

CCommandUngroup::CCommandUngroup(CGroupObject *pObject):
	m_pGroup(pObject)
{
	add_ref(m_pGroup);
	
	IXEditorObject *pObj;
	for(UINT i = 0, l = pObject->getObjectCount(); i < l; ++i)
	{
		pObj = pObject->getObject(i);
		add_ref(pObj);
		m_aObjects.push_back(pObj);
	}
}

CCommandUngroup::~CCommandUngroup()
{
	fora(i, m_aObjects)
	{
		mem_release(m_aObjects[i]);
	}
	mem_release(m_pGroup);
}

bool XMETHODCALLTYPE CCommandUngroup::exec()
{
	ICompoundObject *pParent = XGetObjectParent(m_pGroup);

	fora(i, m_aObjects)
	{
		m_pGroup->removeChildObject(m_aObjects[i]);
		if(pParent)
		{
			pParent->addChildObject(m_aObjects[i]);
		}
		else
		{
			g_pEditor->onObjectAdded(m_aObjects[i]);
		}
	}

	/*
	int idx = g_apGroups.indexOf(m_pGroup);
	assert(idx >= 0);
	if(idx >= 0)
	{
		mem_release(g_apGroups[idx]);
		g_apGroups.erase(idx);
	}

	//m_pProxy->setSelected(false);

	g_pEditor->removeObject(m_pGroup);
	*/
	return(true);
}
bool XMETHODCALLTYPE CCommandUngroup::undo()
{
	ICompoundObject *pParent;
	fora(i, m_aObjects)
	{
		pParent = XGetObjectParent(m_aObjects[i]);
		SAFE_CALL(pParent, removeChildObject, m_aObjects[i]);
		m_pGroup->addChildObject(m_aObjects[i]);
	}

	/*
	fora(i, m_aModels)
	{
		IXEditorModel *pMdl = m_aModels[i];
		assert(!g_apLevelModels.KeyExists(*pMdl->getGUID()));
		g_apLevelModels[*pMdl->getGUID()] = pMdl;
		add_ref(pMdl);
		pMdl->restore();
	}
	fora(i, aObjModels)
	{
		ObjModel &om = aObjModels[i];
		om.pModel->addObject(om.pObj);
	}
	
	fora(i, m_aModels)
	{
		IXEditorModel *pMdl = m_aModels[i];
		m_pProxy->addSrcModel(*pMdl->getGUID());
	}

	bool res = m_pProxy->setDstObject(*m_pEntity->getGUID());
	assert(res);
	m_pProxy->build();

	g_pEditor->addObject(m_pProxy);

	//m_pProxy->setSelected(true);

	add_ref(m_pProxy);
	g_apProxies.push_back(m_pProxy);
	*/
	return(true);
}
