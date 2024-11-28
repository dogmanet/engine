#include "CommandDuplicate.h"

CCommandDuplicate::CCommandDuplicate()
{
	initialize();
}

bool XMETHODCALLTYPE CCommandDuplicate::exec()
{
	if(!m_isExecuted)
	{
		m_isExecuted = m_commandPaste.exec();
	}

	return(m_isExecuted);
}

bool XMETHODCALLTYPE CCommandDuplicate::undo()
{
	if(m_isExecuted)
	{
		m_isExecuted = !m_commandPaste.undo();
	}

	return(!m_isExecuted);
}

void CCommandDuplicate::initialize()
{
	for(UINT i = 0, l = g_pLevelObjects.size(); i < l; ++i)
	{
		IXEditorObject *pObj = g_pLevelObjects[i];
		if(pObj->isSelected())
		{
			processObject(pObj);
		}
	}

	for(UINT i = 0, l = g_apProxies.size(); i < l; ++i)
	{
		CProxyObject *pObj = g_apProxies[i];
		if(pObj->isSelected())
		{
			UINT uProxy = m_commandPaste.addProxy(*pObj->getGUID());
			for(UINT j = 0, jl = pObj->getObjectCount(); j < jl; ++j)
			{
				m_commandPaste.addProxyObject(uProxy, *pObj->getObject(j)->getGUID());
			}
		}
	}
}

void CCommandDuplicate::processObject(IXEditorObject *pObj)
{
	float3 vMin, vMax;
	pObj->getBound(&vMin, &vMax);

	UINT uObj = m_commandPaste.addObject(pObj->getTypeName(), pObj->getClassName(), pObj->getPos(), vMax - vMin, pObj->getOrient(), *pObj->getGUID());
	if(uObj != UINT_MAX)
	{
		for(UINT i = 0, l = pObj->getProperyCount(); i < l; ++i)
		{
			const X_PROP_FIELD *pField = pObj->getPropertyMeta(i);

			if(!pField->isGeneric)
			{
				m_commandPaste.setKV(uObj, pField->szKey, pObj->getKV(pField->szKey));
			}
		}
	}

	void *isProxy = NULL;
	pObj->getInternalData(&X_IS_COMPOUND_GUID, &isProxy);
	if(isProxy)
	{
		// export nested objects
		ICompoundObject *pProxy = (ICompoundObject*)pObj;
		for(UINT i = 0, l = pProxy->getObjectCount(); i < l; ++i)
		{
			processObject(pProxy->getObject(i));
		}
	}
}
