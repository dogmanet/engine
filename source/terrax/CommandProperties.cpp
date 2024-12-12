#include "CommandProperties.h"

CCommandProperties::~CCommandProperties()
{
	for(UINT i = 0, l = m_aCustomTabCommands.size(); i < l; ++i)
	{
		mem_release(m_aCustomTabCommands[i]);
	}

	for(UINT j = 0, jl = m_aObjects.size(); j < jl; ++j)
	{
		mem_release(m_aObjects[j].pObject);
	}
}

bool XMETHODCALLTYPE CCommandProperties::isEmpty()
{
	if(!m_isEmpty)
	{
		return(false);
	}

	for(UINT i = 0, l = m_aCustomTabCommands.size(); i < l; ++i)
	{
		if(!m_aCustomTabCommands[i]->isEmpty())
		{
			return(false);
		}
	}

	return(true);
}

bool XMETHODCALLTYPE CCommandProperties::exec()
{
	_prop_obj *pObj;
	bool isNameChanged = false;

	for(UINT j = 0, jl = m_aObjects.size(); j < jl; ++j)
	{
		pObj = &m_aObjects[j];

		pObj->pObject->preSetup();
		for(auto i = pObj->mKeyValues.begin(); i; ++i)
		{
			if(i.second->isChanged)
			{
				pObj->pObject->setKV(i.first->c_str(), i.second->sNew.c_str());
				if(!strcmp(i.first->c_str(), "name"))
				{
					g_pEditor->onObjectNameChanged(pObj->pObject);
					isNameChanged = true;
				}
			}
		}
		pObj->pObject->postSetup();
	}

	if(isNameChanged)
	{
		g_pEditor->onObjectNameChanged();
	}

	bool isSucceeded = true;

	for(UINT i = 0, l = m_aCustomTabCommands.size(); i < l; ++i)
	{
		isSucceeded &= m_aCustomTabCommands[i]->exec();
	}

	XUpdatePropWindow();
	return(isSucceeded);
}
bool XMETHODCALLTYPE CCommandProperties::undo()
{
	_prop_obj *pObj;
	bool isNameChanged = false;

	for(UINT j = 0, jl = m_aObjects.size(); j < jl; ++j)
	{
		pObj = &m_aObjects[j];

		pObj->pObject->preSetup();
		for(auto i = pObj->mKeyValues.begin(); i; ++i)
		{
			if(i.second->isChanged)
			{
				pObj->pObject->setKV(i.first->c_str(), i.second->sOld.c_str());
				if(!strcmp(i.first->c_str(), "name"))
				{
					g_pEditor->onObjectNameChanged(pObj->pObject);
					isNameChanged = true;
				}
			}
		}
		pObj->pObject->postSetup();
	}

	if(isNameChanged)
	{
		g_pEditor->onObjectNameChanged();
	}

	bool isSucceeded = true;

	for(UINT i = 0, l = m_aCustomTabCommands.size(); i < l; ++i)
	{
		isSucceeded &= m_aCustomTabCommands[i]->undo();
	}

	XUpdatePropWindow();
	return(isSucceeded);
}

void CCommandProperties::addObject(IXEditorObject *pObj)
{
	_prop_obj obj;
	add_ref(pObj);
	obj.pObject = pObj;

	UINT uKeyCount = obj.pObject->getProperyCount();
	for(UINT i = 0; i < uKeyCount; ++i)
	{
		const char *szKey = obj.pObject->getPropertyMeta(i)->szKey;
		obj.mKeyValues[szKey].sOld = obj.pObject->getKV(szKey);
	}

	m_aObjects.push_back(obj);
}

void CCommandProperties::setKV(const char *szKey, const char *szValue)
{
	for(UINT i = 0, l = m_aObjects.size(); i < l; ++i)
	{
		_prop_kv *pKV = &m_aObjects[i].mKeyValues[szKey];
		pKV->sNew = szValue;
		pKV->isChanged = pKV->sOld != pKV->sNew;

		if(pKV->isChanged || !m_isEmpty)
		{
			m_aObjects[i].pObject->setKV(szKey, szValue);
			m_isEmpty = false;
		}
	}

	/*if(m_aObjects.size())
	{
		m_isEmpty = false;
	}*/
}
