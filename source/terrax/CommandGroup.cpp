#include "CommandGroup.h"
#include <common/aastring.h>

extern AssotiativeArray<AAString, IXEditable*> g_mEditableSystems;

CCommandGroup::CCommandGroup()
{
}

CCommandGroup::~CCommandGroup()
{
	mem_release(m_pGroup);
}

bool XMETHODCALLTYPE CCommandGroup::exec()
{
	if(!m_isLocationsSaved)
	{
		IXEditorObject *pObj;
		const Map<IXEditorObject*, ICompoundObject*>::Node *pNode;
		for(auto i = g_mObjectsLocation.begin(); i; ++i)
		{
			if((*i.first)->isSelected() && !(*i.second)->isSelected())
			{
				m_aObjLocations.push_back({*i.first, *i.second});
			}
		}

		bool canHaveCommonParent = true;
		fora(i, g_pLevelObjects)
		{
			if(g_pLevelObjects[i]->isSelected())
			{
				canHaveCommonParent = false;
				break;
			}
		}

		if(canHaveCommonParent)
		{
			Array<ICompoundObject*> aPath;
			ICompoundObject *pParent;
			fora(i, m_aObjLocations)
			{
				pParent = m_aObjLocations[i].pLocation;
				while(pParent)
				{
					if(i == 0)
					{
						aPath.push_back(pParent);
					}
					else
					{
						int idx = aPath.indexOf(pParent);
						if(idx >= 0)
						{
							while(idx--)
							{
								aPath.erase(0);
							}
							break;
						}
					}
					pParent = XGetObjectParent(pParent);
				}

				if(i != 0 && !pParent)
				{
					aPath.clear();
					break;
				}
			}

			if(aPath.size())
			{
				fora(i, aPath)
				{
					// Groups cannot be inside proxies
					void *pData = NULL;
					aPath[i]->getInternalData(&X_IS_PROXY_GUID, &pData);
					if(!pData)
					{
						m_pCommonParent = aPath[i];
						break;
					}
				}
			}
		}

		m_isLocationsSaved = true;
	}
	
	if(!m_pGroup)
	{
		m_pGroup = new CGroupObject();
		m_pGroup->setPos(m_vPos);
	}

	g_pEditor->addObject(m_pGroup);

	if(m_pCommonParent)
	{
		m_pCommonParent->addChildObject(m_pGroup);
	}

	fora(i, m_aObjLocations)
	{
		ObjLocation &loc = m_aObjLocations[i];
		loc.pLocation->removeChildObject(loc.pObj);
	}

	IXEditorObject *pObject;
	forar(i, g_pLevelObjects)
	{
		pObject = g_pLevelObjects[i];
		if(pObject->isSelected())
		{
			m_pGroup->addChildObject(pObject);
		}
	}

	//g_pEditor->addObject(m_pGroup);

	m_pGroup->setSelected(true);

	add_ref(m_pGroup);
	g_apGroups.push_back(m_pGroup);

	//TODO("Find deepest common parent to place group into");

	return(true);
}
bool XMETHODCALLTYPE CCommandGroup::undo()
{
	int idx = g_apGroups.indexOf(m_pGroup);
	assert(idx >= 0);
	if(idx >= 0)
	{
		mem_release(g_apGroups[idx]);
		g_apGroups.erase(idx);
	}

	//m_pProxy->setSelected(false);


	// destroy proxy
	//m_pProxy->reset();

	IXEditorObject *pObj;
	for(int i = (int)m_pGroup->getObjectCount() - 1; i >= 0; --i)
	{
		pObj = m_pGroup->getObject(i);
		m_pGroup->removeChildObject(pObj);

		if(m_aObjLocations.indexOf(pObj, [](const ObjLocation &a, IXEditorObject *pB){
			return(a.pObj == pB);
		}) < 0)
		{
			g_pEditor->onObjectAdded(pObj);
		}
	}
	// restore object locations
	fora(i, m_aObjLocations)
	{
		ObjLocation &loc = m_aObjLocations[i];
		loc.pLocation->addChildObject(loc.pObj);
	}

	m_pGroup->setSelected(false);

	if(m_pCommonParent)
	{
		m_pCommonParent->removeChildObject(m_pGroup);
	}
	g_pEditor->removeObject(m_pGroup);
		
	return(true);
}
