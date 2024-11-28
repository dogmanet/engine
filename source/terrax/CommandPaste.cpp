#include "CommandPaste.h"
#include <common/aastring.h>

extern AssotiativeArray<AAString, IXEditable*> g_mEditableSystems;

bool XMETHODCALLTYPE CCommandPaste::exec()
{
	if(!m_pCommandSelect)
	{
		m_pCommandSelect = new CCommandSelect();
		
		XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, ICompoundObject *pParent){
			if(pObj->isSelected() && (g_xConfig.m_bIgnoreGroups ? !isProxy : !pParent))
			{
				m_pCommandSelect->addDeselected(pObj);
			}
		});
	}

	m_pCommandSelect->exec();

	_paste_obj *pObj;
	fora(i, m_aObjects)
	{
		pObj = &m_aObjects[i];

		pObj->pObject->setSelected(true);
		pObj->pObject->create();

		pObj->pObject->setPos(pObj->vPos);
		pObj->pObject->setSize(pObj->vScale);
		pObj->pObject->setOrient(pObj->qRotate);
		pObj->pObject->preSetup();
		for(auto i = pObj->mKeyValues.begin(); i; ++i)
		{
			pObj->pObject->setKV(i.first->c_str(), i.second->c_str());
		}
		pObj->pObject->postSetup();
		//add_ref(pObj->pObject);
		g_pEditor->addObject(pObj->pObject);
	}

	fora(i, m_aProxies)
	{
		_proxy_obj &po = m_aProxies[i];

		po.pProxy->setDstObject(m_mapGuids[po.guid]);
		fora(j, po.aObjects)
		{
			IXEditorObject *pObj = XFindObjectByGUID(m_mapGuids[po.aObjects[j]]);
			if(pObj)
			{
				po.pProxy->addChildObject(pObj);
			}
		}
		
		po.pProxy->build();
		g_pEditor->addObject(po.pProxy);
		po.pProxy->setSelected(true);
		add_ref(po.pProxy);
		g_apProxies.push_back(po.pProxy);
	}

	XUpdatePropWindow();
	return(m_aObjects.size() != 0);
}
bool XMETHODCALLTYPE CCommandPaste::undo()
{
	forar(i, m_aProxies)
	{
		CProxyObject *pProxy = m_aProxies[i].pProxy;

		int idx = g_apProxies.indexOf(pProxy);
		assert(idx >= 0);
		if(idx >= 0)
		{
			mem_release(g_apProxies[idx]);
			g_apProxies.erase(idx);
		}

		g_pEditor->removeObject(pProxy);

		while(pProxy->getObjectCount())
		{
			pProxy->removeChildObject(pProxy->getObject(0));
		}

		pProxy->reset();
	}


	_paste_obj *pObj;
	forar(i, m_aObjects)
	{
		pObj = &m_aObjects[i];

		pObj->pObject->remove();
		g_pEditor->removeObject(pObj->pObject);
	}

	m_pCommandSelect->undo();

	XUpdatePropWindow();
	return(true);
}

CCommandPaste::~CCommandPaste()
{
	fora(i, m_aObjects)
	{
		mem_release(m_aObjects[i].pObject);
	}
	mem_release(m_pCommandSelect);

	fora(i, m_aProxies)
	{
		mem_release(m_aProxies[i].pProxy);
	}
}

UINT CCommandPaste::addObject(const char *szTypeName, const char *szClassName, const float3 &vPos, const float3 &vScale, const SMQuaternion &qRotate, const XGUID &oldGUID)
{
	const AssotiativeArray<AAString, IXEditable*>::Node *pNode;
	if(!g_mEditableSystems.KeyExists(AAString(szTypeName), &pNode))
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Unknown object type %s, skipping!", szTypeName);
		return(UINT_MAX);
	}

	_paste_obj obj;
	obj.pObject = (*(pNode->Val))->newObject(szClassName);
	if(!obj.pObject)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Cannot create object type %s/%s, skipping!", szTypeName, szClassName);
		return(UINT_MAX);
	}
	obj.vPos = vPos;
	obj.vScale = vScale;
	obj.qRotate = qRotate;

	m_mapGuids[oldGUID] = *obj.pObject->getGUID();

	m_aObjects.push_back(obj);

	return(m_aObjects.size() - 1);
}
void CCommandPaste::setKV(UINT uId, const char *szKey, const char *szValue)
{
	assert(uId < m_aObjects.size());

	m_aObjects[uId].mKeyValues[szKey] = szValue;
}

UINT CCommandPaste::addProxy(const XGUID &guid)
{
	m_aProxies.push_back({guid, new CProxyObject()});

	return(m_aProxies.size() - 1);
}

void CCommandPaste::addProxyObject(UINT uProxy, const XGUID &guid)
{
	assert(uProxy < m_aProxies.size());
	
	m_aProxies[uProxy].aObjects.push_back(guid);
}
