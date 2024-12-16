#include "CommandBuildModel.h"
#include <common/aastring.h>

extern AssotiativeArray<AAString, IXEditable*> g_mEditableSystems;

CCommandBuildModel::CCommandBuildModel(const char *szTypeName, const char *szClassName)
{
	m_pCommandCreate = new CCommandCreate(float3_t(), szTypeName, szClassName);
}

CCommandBuildModel::~CCommandBuildModel()
{
	fora(i, aObjModels)
	{
		mem_release(aObjModels[i].pObj);
	}
	fora(i, m_aModels)
	{
		mem_release(m_aModels[i]);
	}
	mem_release(m_pCommandCreate);
	mem_release(m_pProxy);
}

bool XMETHODCALLTYPE CCommandBuildModel::exec()
{
	// build entity
	if(!m_pCommandCreate->exec())
	{
		return(false);
	}


	if(!m_isLocationsSaved)
	{
		IXEditorObject *pObj;
		const Map<IXEditorObject*, ICompoundObject*>::Node *pNode;
		for(auto i = g_mObjectsLocation.begin(); i; ++i)
		{
			if((*i.first)->isSelected())
			{
				void *pData = NULL;
				(*i.first)->getInternalData(&X_IS_COMPOUND_GUID, &pData);
				if(!pData)
				{
					m_aObjLocations.push_back({*i.first, *i.second});
				}
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

	// build models
	if(m_aModels.size() == 0)
	{
		for(auto i = g_mEditableSystems.begin(); i; ++i)
		{
			IXEditable *pEditable = *(i.second);
			IXEditorModel *pMdl;
			if(pEditable->canProduceModel() && pEditable->buildModelFromSelection(&pMdl))
			{
				add_ref(pMdl);
				m_aModels.push_back(pMdl);
				g_apLevelModels[*pMdl->getGUID()] = pMdl;
			}
		}
		if(m_aModels.size() == 0)
		{
			m_pCommandCreate->undo();
			return(false);
		}
	}
	else
	{
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
	}
	if(!m_isCenterFound)
	{
		SMAABB aabb, aabb2;
		IXEditorObject *pObj;
		fora(i, m_aModels)
		{
			IXEditorModel *pModel = m_aModels[i];
			for(UINT j = 0, jl = pModel->getObjectCount(); j < jl; ++j)
			{
				if(pModel->getObject(j, &pObj))
				{
					if(!m_isCenterFound)
					{
						pObj->getBound(&aabb.vMin, &aabb.vMax);
						m_isCenterFound = true;
					}
					else
					{
						pObj->getBound(&aabb2.vMin, &aabb2.vMax);
						aabb = SMAABBConvex(aabb, aabb2);
					}
					aObjModels.push_back({pObj, pModel});
					//mem_release(pObj);
				}
			}
		}

		m_vPos = (float3)((aabb.vMin + aabb.vMax) * 0.5f);
	}

	// build proxy
	if(!m_pProxy)
	{
		m_pProxy = new CProxyObject();
		m_pProxy->setPos(m_vPos);
	}
	fora(i, m_aModels)
	{
		IXEditorModel *pMdl = m_aModels[i];
		m_pProxy->addSrcModel(*pMdl->getGUID());
	}
	m_pCommandCreate->getObject()->setPos(m_vPos);
	bool res = m_pProxy->setDstObject(*m_pCommandCreate->getObject()->getGUID());
	assert(res);
	m_pProxy->build();

	g_pEditor->addObject(m_pProxy);

	m_pProxy->setSelected(true);

	add_ref(m_pProxy);
	g_apProxies.push_back(m_pProxy);

	return(true);
}
bool XMETHODCALLTYPE CCommandBuildModel::undo()
{
	int idx = g_apProxies.indexOf(m_pProxy);
	assert(idx >= 0);
	if(idx >= 0)
	{
		mem_release(g_apProxies[idx]);
		g_apProxies.erase(idx);
	}

	//m_pProxy->setSelected(false);

	g_pEditor->removeObject(m_pProxy);

	// destroy proxy
	m_pProxy->reset();

	// destroy models
	fora(i, m_aModels)
	{
		IXEditorModel *pMdl = m_aModels[i];
		assert(g_apLevelModels.KeyExists(*pMdl->getGUID()));
		g_apLevelModels.erase(*pMdl->getGUID());
		pMdl->destroy();
		mem_release(pMdl);
		//mem_release(m_aModels[i]);
	}
	//m_aModels.clearFast();


	// restore object locations
	fora(i, m_aObjLocations)
	{
		ObjLocation &loc = m_aObjLocations[i];
		loc.pLocation->addChildObject(loc.pObj);
	}

	//destroy entity
	m_pCommandCreate->undo();

	return(true);
}
