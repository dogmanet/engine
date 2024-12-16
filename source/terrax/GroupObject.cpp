#include "GroupObject.h"

#include <xcommon/resource/IXResourceManager.h>
#include <xcommon/IXModelWriter.h>
#include "terrax.h"
#include "CommandDelete.h"
#include <common/aastring.h>
#include <core/sxcore.h>

extern AssotiativeArray<AAString, IXEditable*> g_mEditableSystems;

CGroupObject::CGroupObject()
{
	XCreateGUID(&m_guid);
}

CGroupObject::CGroupObject(const XGUID &guid)
{
	m_guid = guid;
}

CGroupObject::~CGroupObject()
{
	fora(i, m_aObjects)
	{
		mem_release(m_aObjects[i].pObj);
	}
}

void XMETHODCALLTYPE CGroupObject::setPos(const float3_t &pos)
{
	m_vPos = pos;

	fora(i, m_aObjects)
	{
		SrcObject &o = m_aObjects[i];
		o.pObj->setPos((float3)(m_vPos + m_qOrient * o.vOffset));
	}
}

void XMETHODCALLTYPE CGroupObject::setSize(const float3_t &vSize)
{
	float3 vMin, vMax, vScale;
	getBound(&vMin, &vMax);
	vScale = vSize / (vMax - vMin);
	fora(i, m_aObjects)
	{
		SrcObject &o = m_aObjects[i];
		o.vOffset = m_qOrient.Conjugate() * (float3)((o.pObj->getPos() - m_vPos) * vScale);
		o.pObj->setPos((float3)(m_vPos + m_qOrient * o.vOffset));
		o.pObj->getBound(&vMin, &vMax);
		//printf("%.2f %.2f %.2f : %.2f %.2f %.2f\n", vMin.x, vMin.y, vMin.z, vMax.x, vMax.y, vMax.z);
		o.pObj->setSize((float3)(vScale * (vMax - vMin)));
	}
}

void XMETHODCALLTYPE CGroupObject::setOrient(const SMQuaternion &orient)
{
	m_qOrient = orient;
	//m_pTargetObject->setOrient(orient);
	fora(i, m_aObjects)
	{
		SrcObject &o = m_aObjects[i];
		o.pObj->setOrient(orient * o.qOffset);
		o.pObj->setPos((float3)(m_vPos + m_qOrient * o.vOffset));
	}
}

float3_t XMETHODCALLTYPE CGroupObject::getPos()
{
	float3 vMin, vMax;
	getBound(&vMin, &vMax);

	m_vPos = (vMax + vMin) * 0.5f;

	fora(i, m_aObjects)
	{
		m_aObjects[i].vOffset = m_qOrient.Conjugate() * (m_aObjects[i].pObj->getPos() - m_vPos);
	}

	return(m_vPos);
}

SMQuaternion XMETHODCALLTYPE CGroupObject::getOrient()
{
	fora(i, m_aObjects)
	{
		m_aObjects[i].qOffset = m_aObjects[i].pObj->getOrient() * m_qOrient.Conjugate();
	}

	return(m_qOrient);
}

void XMETHODCALLTYPE CGroupObject::getBound(float3 *pvMin, float3 *pvMax)
{
	if(!m_aObjects.size())
	{
		*pvMin = 0.0f;
		*pvMax = 0.0f;
		return;
	}

	float3 vMin, vMax;

	m_aObjects[0].pObj->getBound(pvMin, pvMax);
	for(UINT i = 1, l = m_aObjects.size(); i < l; ++i)
	{
		m_aObjects[i].pObj->getBound(&vMin, &vMax);
		*pvMin = SMVectorMin(*pvMin, vMin);
		*pvMax = SMVectorMax(*pvMax, vMax);
	}
}

void XMETHODCALLTYPE CGroupObject::render(bool is3D, bool bRenderSelection, IXGizmoRenderer *pGizmoRenderer)
{
	fora(i, m_aObjects)
	{
		m_aObjects[i].pObj->render(is3D, bRenderSelection, pGizmoRenderer);
	}
}

bool XMETHODCALLTYPE CGroupObject::rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut, float3 *pvNormal, ID *pidMtrl, bool bReturnNearestPoint)
{
	if(bReturnNearestPoint)
	{
		float fBestDist = FLT_MAX;
		float3 vPoint, vNormal;
		ID idMtrl = -1;
		float fDist;
		bool isFound = false;

		fora(i, m_aObjects)
		{
			if(m_aObjects[i].pObj->rayTest(vStart, vEnd, &vPoint, &vNormal, &idMtrl, bReturnNearestPoint))
			{
				fDist = SMVector3Length2(vPoint - vStart);
				if(fDist < fBestDist)
				{
					fBestDist = fBestDist;
					if(pvOut)
					{
						*pvOut = vPoint;
					}
					if(pvNormal)
					{
						*pvNormal = vNormal;
					}
					if(pidMtrl)
					{
						*pidMtrl = idMtrl;
					}
				}

				isFound = true;
			}
		}

		return(isFound);
	}
	else
	{
		fora(i, m_aObjects)
		{
			if(m_aObjects[i].pObj->rayTest(vStart, vEnd, pvOut, pvNormal, pidMtrl, bReturnNearestPoint))
			{
				return(true);
			}
		}
	}

	return(false);
}

void XMETHODCALLTYPE CGroupObject::remove()
{
	int idx = g_apGroups.indexOf(this);
	assert(idx >= 0);
	if(idx >= 0)
	{
		mem_release(g_apGroups[idx]);
		g_apGroups.erase(idx);
	}

	XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, ICompoundObject *pParent){
		g_mObjectsLocation.erase(pObj);
		mem_release(pObj);
		return(XEOR_SKIP_CHILDREN);
	}, this);

	m_isRemoved = true;
	fora(i, m_aObjects)
	{
		m_aObjects[i].pObj->remove();
	}
}
void XMETHODCALLTYPE CGroupObject::preSetup()
{
	//m_pTargetObject->preSetup();
}
void XMETHODCALLTYPE CGroupObject::postSetup()
{
	//m_pTargetObject->preSetup();
}

void XMETHODCALLTYPE CGroupObject::create()
{
	int idx = g_apGroups.indexOf(this);
	assert(idx < 0);
	if(idx < 0)
	{
		add_ref(this);
		g_apGroups.push_back(this);
	}

	XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, ICompoundObject *pParent){
		g_mObjectsLocation[pObj] = pParent;
		add_ref(pObj);
		return(XEOR_SKIP_CHILDREN);
	}, this);


	m_isRemoved = false;
	fora(i, m_aObjects)
	{
		m_aObjects[i].pObj->create();
	}
}

void XMETHODCALLTYPE CGroupObject::setKV(const char *szKey, const char *szValue)
{
	if(!fstrcmp(szKey, "guid"))
	{
		XGUIDFromString(&m_guid, szValue);
	}
	else if(!fstrcmp(szKey, "name"))
	{
		m_sName = szValue;
	}
}
const char* XMETHODCALLTYPE CGroupObject::getKV(const char *szKey)
{
	if(!fstrcmp(szKey, "guid"))
	{
		char tmp[64];
		XGUIDToSting(m_guid, tmp, sizeof(tmp));
		m_sGUID = tmp;
		return(m_sGUID.c_str());
	}
	else if(!fstrcmp(szKey, "name"))
	{
		return(m_sName.c_str());
	}
	return(NULL);
}
const X_PROP_FIELD* XMETHODCALLTYPE CGroupObject::getPropertyMeta(UINT uKey)
{
	static X_PROP_FIELD s_prop0 = {"guid", "GUID", XPET_TEXT, NULL, "", true};
	static X_PROP_FIELD s_prop1 = {"name", "Name", XPET_TEXT, NULL, ""};
	switch(uKey)
	{
	case 0:
		return(&s_prop0);
	case 1:
		return(&s_prop1);
	}
	return(NULL);
}
UINT XMETHODCALLTYPE CGroupObject::getProperyCount()
{
	return(2);
}

const char* XMETHODCALLTYPE CGroupObject::getTypeName()
{
	return("TerraX");
}
const char* XMETHODCALLTYPE CGroupObject::getClassName()
{
	return("Group");
}

void XMETHODCALLTYPE CGroupObject::setSelected(bool set)
{
	m_isSelected = set;

	fora(i, m_aObjects)
	{
		m_aObjects[i].pObj->setSelected(set);
	}
}

void XMETHODCALLTYPE CGroupObject::setSimulationMode(bool set)
{
	fora(i, m_aObjects)
	{
		m_aObjects[i].pObj->setSimulationMode(set);
	}
}

void CGroupObject::addChildObject(IXEditorObject *pObject)
{
	assert(pObject != this);

	ICompoundObject *pOldContainer = XTakeObject(pObject, this);
	assert(pOldContainer == NULL);
	//add_ref(pObject);
	m_aObjects.push_back({pObject, m_qOrient.Conjugate() * (pObject->getPos() - m_vPos), pObject->getOrient() * m_qOrient.Conjugate()});
	
	g_pEditor->onObjectAdded(pObject);
}
void CGroupObject::removeChildObject(IXEditorObject *pObject)
{
	ICompoundObject *pOldContainer = XTakeObject(pObject, NULL);
	assert(pOldContainer == this);

	int idx = m_aObjects.indexOf(pObject, [](const SrcObject &a, IXEditorObject *b){
		return(a.pObj == b);
	});
	assert(idx >= 0);
	if(idx >= 0)
	{
		m_aObjects.erase(idx);

		g_pEditor->onObjectRemoved(pObject);

		//mem_release(pObject);

		if(!m_aObjects.size())
		{
			CCommandDelete *pCmd = new CCommandDelete();
			pCmd->addObject(this);
			XAttachCommand(pCmd);
		}
	}
}

UINT CGroupObject::getObjectCount()
{
	return(m_aObjects.size());
}
IXEditorObject* CGroupObject::getObject(UINT id)
{
	assert(id < m_aObjects.size());
	if(id < m_aObjects.size())
	{
		return(m_aObjects[id].pObj);
	}
	return(NULL);
}

void XMETHODCALLTYPE CGroupObject::getInternalData(const XGUID *pGUID, void **ppOut)
{
	if(*pGUID == X_IS_GROUP_GUID || *pGUID == X_IS_COMPOUND_GUID)
	{
		*ppOut = (void*)1;
	}
	else
	{
		BaseClass::getInternalData(pGUID, ppOut);
	}
}
