#include "ProxyObject.h"

#include <xcommon/resource/IXResourceManager.h>
#include <xcommon/IXModelWriter.h>
#include "terrax.h"
#include "CommandDelete.h"
#include <common/aastring.h>
#include <core/sxcore.h>

extern AssotiativeArray<AAString, IXEditable*> g_mEditableSystems;

CProxyObject::CProxyObject():
	m_modelCallback(this)
{
	XCreateGUID(&m_guid);
}

CProxyObject::CProxyObject(const XGUID &guid):
	m_modelCallback(this)
{
	m_guid = guid;
}

CProxyObject::~CProxyObject()
{
	fora(i, m_aModels)
	{
		m_aModels[i].pModel->setCallback(NULL);
		mem_release(m_aModels[i].pModel);
	}
	fora(i, m_aObjects)
	{
		mem_release(m_aObjects[i].pObj);
	}

	mem_release(m_pTargetObject);

	mem_release(m_pResource);
}

void XMETHODCALLTYPE CProxyObject::setPos(const float3_t &pos)
{
	m_bSkipCallback = true;
	m_vPos = pos;
	SAFE_CALL(m_pTargetObject, setPos, pos);

	fora(i, m_aObjects)
	{
		SrcObject &o = m_aObjects[i];
		o.pObj->setPos((float3)(m_vPos + m_qOrient * o.vOffset));
	}
	m_bSkipCallback = false;
}

void XMETHODCALLTYPE CProxyObject::setSize(const float3_t &vSize)
{
	m_bSkipCallback = true;
	float3 vMin, vMax, vScale;
	m_pTargetObject->getBound(&vMin, &vMax);
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

	m_bSkipCallback = false;
	build();
}

void XMETHODCALLTYPE CProxyObject::setOrient(const SMQuaternion &orient)
{
	m_bSkipCallback = true;
	m_qOrient = orient;
	//m_pTargetObject->setOrient(orient);
	fora(i, m_aObjects)
	{
		SrcObject &o = m_aObjects[i];
		o.pObj->setOrient(orient * o.qOffset);
		o.pObj->setPos((float3)(m_vPos + m_qOrient * o.vOffset));
	}
	m_bSkipCallback = false;
	build();
}

float3_t XMETHODCALLTYPE CProxyObject::getPos()
{
	float3 vPos = m_pTargetObject->getPos();
	if(!SMIsZero(SMVector3Length2(m_vPos - vPos)))
	{
		setPos(vPos);
	}

	SMQuaternion qOrient = m_pTargetObject->getOrient();
	if(!SMIsZero(SMVector4Length2(/*m_qOrient.toFloat4()*/ SMQuaternion().toFloat4() - qOrient.toFloat4())))
	{
		setOrient(qOrient);
	}

	return(m_vPos);
}

SMQuaternion XMETHODCALLTYPE CProxyObject::getOrient()
{
	return(m_qOrient);
}

void XMETHODCALLTYPE CProxyObject::getBound(float3 *pvMin, float3 *pvMax)
{
	m_pTargetObject->getBound(pvMin, pvMax);
}

void XMETHODCALLTYPE CProxyObject::render(bool is3D, bool bRenderSelection, IXGizmoRenderer *pGizmoRenderer)
{
	fora(i, m_aObjects)
	{
		m_aObjects[i].pObj->render(is3D, bRenderSelection, pGizmoRenderer);
	}
}

bool XMETHODCALLTYPE CProxyObject::rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut, float3 *pvNormal, ID *pidMtrl, bool bReturnNearestPoint)
{
	return(m_pTargetObject->rayTest(vStart, vEnd, pvOut, pvNormal, pidMtrl, bReturnNearestPoint));
}

void XMETHODCALLTYPE CProxyObject::remove()
{
	int idx = g_apProxies.indexOf(this);
	assert(idx >= 0);
	if(idx >= 0)
	{
		mem_release(g_apProxies[idx]);
		g_apProxies.erase(idx);
	}

	XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
		g_mObjectsLocation.erase(pObj);
		mem_release(pObj);
	}, this);

	m_isRemoved = true;
	m_pTargetObject->remove();
	fora(i, m_aObjects)
	{
		m_aObjects[i].pObj->remove();
	}
}
void XMETHODCALLTYPE CProxyObject::preSetup()
{
	m_pTargetObject->preSetup();
}
void XMETHODCALLTYPE CProxyObject::postSetup()
{
	m_pTargetObject->preSetup();
}

void XMETHODCALLTYPE CProxyObject::create()
{
	int idx = g_apProxies.indexOf(this);
	assert(idx < 0);
	if(idx < 0)
	{
		add_ref(this);
		g_apProxies.push_back(this);
	}

	XEnumerateObjects([&](IXEditorObject *pObj, bool isProxy, CProxyObject *pParent){
		g_mObjectsLocation[pObj] = pParent;
		add_ref(pObj);
	}, this);


	m_isRemoved = false;
	fora(i, m_aObjects)
	{
		m_aObjects[i].pObj->create();
	}
	m_pTargetObject->create();
}

void XMETHODCALLTYPE CProxyObject::setKV(const char *szKey, const char *szValue)
{
	m_pTargetObject->setKV(szKey, szValue);
}
const char* XMETHODCALLTYPE CProxyObject::getKV(const char *szKey)
{
	return(m_pTargetObject->getKV(szKey));
}
const X_PROP_FIELD* XMETHODCALLTYPE CProxyObject::getPropertyMeta(UINT uKey)
{
	return(m_pTargetObject->getPropertyMeta(uKey));
}
UINT XMETHODCALLTYPE CProxyObject::getProperyCount()
{
	return(m_pTargetObject->getProperyCount());
}

const char* XMETHODCALLTYPE CProxyObject::getTypeName()
{
	return(m_pTargetObject->getTypeName());
}
const char* XMETHODCALLTYPE CProxyObject::getClassName()
{
	return(m_pTargetObject->getClassName());
}

void XMETHODCALLTYPE CProxyObject::setSelected(bool set)
{
	m_isSelected = set;

	m_pTargetObject->setSelected(set);
	fora(i, m_aObjects)
	{
		m_aObjects[i].pObj->setSelected(set);
	}
}

void XMETHODCALLTYPE CProxyObject::setSimulationMode(bool set)
{
	m_pTargetObject->setSimulationMode(set);
	fora(i, m_aObjects)
	{
		m_aObjects[i].pObj->setSimulationMode(set);
	}
}

bool CProxyObject::setDstObject(const XGUID &guid)
{
	IXEditorObject *pObj = XFindObjectByGUID(guid);
	assert(pObj);
	if(pObj)
	{
		m_pTargetObject = pObj;
		int idx = g_pLevelObjects.indexOf(pObj);
		assert(idx >= 0);
		if(idx >= 0)
		{
			g_pLevelObjects.erase(idx);
		}

		m_qOrient = pObj->getOrient();
		pObj->setOrient(SMQuaternion());
		m_vPos = pObj->getPos();

		return(true);
	}

	char tmp[64], tmp2[64];
	XGUIDToSting(guid, tmp, sizeof(tmp));
	XGUIDToSting(m_guid, tmp2, sizeof(tmp2));
	LibReport(REPORT_MSG_LEVEL_ERROR, "Cannot set the object %s as the proxy %s target object. The щиоусе is not found\n", tmp, tmp2);
	return(false);
}
void CProxyObject::addSrcModel(const XGUID &guid)
{
	const Map<XGUID, IXEditorModel*>::Node *pNode;
	if(!g_apLevelModels.KeyExists(guid, &pNode))
	{
		char tmp[64], tmp2[64];
		XGUIDToSting(guid, tmp, sizeof(tmp));
		XGUIDToSting(m_guid, tmp2, sizeof(tmp2));
		LibReport(REPORT_MSG_LEVEL_ERROR, "Cannot add the model %s to the proxy %s. The model is not found\n", tmp, tmp2);
		return;
	}

	IXEditorModel *pModel = *pNode->Val;
	g_apLevelModels.erase(guid);

	pModel->setCallback(&m_modelCallback);

	m_aModels.push_back({pModel});

	for(UINT i = 0, l = pModel->getObjectCount(); i < l; ++i)
	{
		IXEditorObject *pObject;
		if(pModel->getObject(i, &pObject))
		{
			CProxyObject *pOldWhere = XTakeObject(pObject, this);
			assert(pOldWhere == NULL);
			m_aObjects.push_back({pObject, m_qOrient.Conjugate() * (pObject->getPos() - m_vPos), pObject->getOrient() * m_qOrient.Conjugate()});
		}
	}
}

template<class T>
static void MoveLods(T *pResource, const float3 &vOffset, const SMQuaternion &qOffset)
{
	for(UINT uLod = 0, uLodCount = pResource->getLodCount(); uLod < uLodCount; ++uLod)
	{
		for(UINT uSubset = 0, uSubsetCount = pResource->getSubsetCount(uLod); uSubset < uSubsetCount; ++uSubset)
		{
			auto *pLod = pResource->getSubset(uLod, uSubset);
			for(UINT i = 0, l = pLod->iVertexCount; i < l; ++i)
			{
				pLod->pVertices[i].vPos = /*qOffset * */(float3)(pLod->pVertices[i].vPos - vOffset);
				//pLod->pVertices[i].vNorm = qOffset * pLod->pVertices[i].vNorm;
				//pLod->pVertices[i].vTangent = qOffset * pLod->pVertices[i].vTangent;
				//pLod->pVertices[i].vBinorm = qOffset * pLod->pVertices[i].vBinorm;
			}
		}
	}
}

void CProxyObject::build()
{
	mem_release(m_pResource);

	// TODO add support for multiple models!
	// collect resources and build model
	IXResourceModel *pResource;
	m_aModels[0].pModel->getResource(&pResource);


	// translate relative to m_vPos
	if(pResource->getType() == XMT_STATIC)
	{
		MoveLods(pResource->asStatic(), m_vPos, m_qOrient.Conjugate());
	}
	else
	{
		MoveLods(pResource->asAnimated(), m_vPos, m_qOrient.Conjugate());
	}
	
	for(UINT j = 0, jl = pResource->getPhysboxCount(); j < jl;  ++j)
	{
		IModelPhysbox *pBox = pResource->getPhysbox(j);
		pBox->setPosition(pBox->getPosition() - m_vPos);
	}

	IXResourceManager *pRM = Core_GetIXCore()->getResourceManager();

	extern String g_sLevelName;

	char tmp[64];
	char szModel[256];
	XGUIDToSting(m_guid, tmp, sizeof(tmp));
	sprintf(szModel, "levels/%s/models/%s.dse", g_sLevelName.c_str(), tmp);

	pRM->addModel(szModel, pResource);

	m_pResource = pResource;

	//! TODO make universal way to set model!
	m_pTargetObject->setKV("model", szModel);
}
void CProxyObject::reset()
{
	fora(i, m_aModels)
	{
		m_aModels[i].pModel->setCallback(NULL);
		g_apLevelModels[*m_aModels[i].pModel->getGUID()] = m_aModels[i].pModel;
	}
	m_aModels.clearFast();

	fora(i, m_aObjects)
	{
		CProxyObject *pOldWhere = XTakeObject(m_aObjects[i].pObj, NULL);
		assert(pOldWhere == this);
		mem_release(m_aObjects[i].pObj);
	}
	m_aObjects.clearFast();

	if(m_pTargetObject)
	{
		g_pLevelObjects.push_back(m_pTargetObject);
		m_pTargetObject = NULL;
	}

	mem_release(m_pResource);
}

void CProxyObject::onModelChanged(IXEditorModel *pModel, IXEditorObject *pObject)
{
	if(pObject)
	{
		int idx = m_aObjects.indexOf(pObject, [](const SrcObject &a, IXEditorObject *b){
			return(a.pObj == b);
		});

		if(idx >= 0)
		{
			SrcObject &o = m_aObjects[idx];
			o.vOffset = m_qOrient.Conjugate() * (float3)(o.pObj->getPos() - m_vPos);
			o.qOffset = pObject->getOrient() * m_qOrient.Conjugate();
		}
	}

	if(m_bSkipCallback)
	{
		return;
	}

	bool hasObjects = m_aObjects.size() != 0;
	if(hasObjects)
	{
		build();
	}
	else
	{
		CCommandDelete *pCmd = new CCommandDelete();
		pCmd->addObject(this);
		XAttachCommand(pCmd);
	}
}

void CProxyObject::addChildObject(IXEditorObject *pObject)
{
	assert(pObject != this);

	// add to model
	int idx = m_aModels.indexOf(pObject->getTypeName(), [](const SrcModel &a, const char *szType){
		return(!fstrcmp(a.pModel->getTypeName(), szType));
	});

	if(idx < 0)
	{
		// create empty model
		const AssotiativeArray<AAString, IXEditable*>::Node *pNode;
		if(!g_mEditableSystems.KeyExists(AAString(pObject->getTypeName()), &pNode))
		{
			assert(!"Invalid type name!");
			return;
		}

		IXEditorModel *pModel = NULL;

		if(!(*pNode->Val)->canProduceModel() || !(*pNode->Val)->newModel(&pModel))
		{
			LibReport(REPORT_MSG_LEVEL_ERROR, "Cannot use object type %s/%s as model source, skipping!", pObject->getTypeName(), pObject->getClassName());
			return;
		}
		assert(pModel);
		if(pModel)
		{
			pModel->setCallback(&m_modelCallback);
			m_aModels.push_back({pModel});
		}
		idx = m_aModels.size() - 1;
	}

	assert(idx >= 0);
	if(idx >= 0)
	{
		CProxyObject *pOldContainer = XTakeObject(pObject, this);
		assert(pOldContainer == NULL);
		add_ref(pObject);
		m_aObjects.push_back({pObject, m_qOrient.Conjugate() * (pObject->getPos() - m_vPos), pObject->getOrient() * m_qOrient.Conjugate()});

		m_aModels[idx].pModel->addObject(pObject);
	}
}
void CProxyObject::removeChildObject(IXEditorObject *pObject)
{
	CProxyObject *pOldContainer = XTakeObject(pObject, NULL);
	assert(pOldContainer == this);

	int idx = m_aObjects.indexOf(pObject, [](const SrcObject &a, IXEditorObject *b){
		return(a.pObj == b);
	});
	assert(idx >= 0);
	if(idx >= 0)
	{
		m_aObjects.erase(idx);

		// remove from model
		idx = m_aModels.indexOf(pObject->getTypeName(), [](const SrcModel &a, const char *szType){
			return(!fstrcmp(a.pModel->getTypeName(), szType));
		});
		assert(idx >= 0);
		if(idx >= 0)
		{
			m_aModels[idx].pModel->removeObject(pObject);
		}

		mem_release(pObject);
	}
}

IXEditorObject* CProxyObject::getTargetObject()
{
	return(m_pTargetObject);
}
UINT CProxyObject::getModelCount()
{
	return(m_aModels.size());
}
IXEditorModel* CProxyObject::getModel(UINT id)
{
	assert(id < m_aModels.size());
	if(id < m_aModels.size())
	{
		return(m_aModels[id].pModel);
	}
	return(NULL);
}

UINT CProxyObject::getObjectCount()
{
	return(m_aObjects.size());
}
IXEditorObject* CProxyObject::getObject(UINT id)
{
	assert(id < m_aObjects.size());
	if(id < m_aObjects.size())
	{
		return(m_aObjects[id].pObj);
	}
	return(NULL);
}

void CProxyObject::saveModel()
{
	if(!m_pResource)
	{
		return;
	}
	extern String g_sLevelName;
	char tmp[64];
	char szModel[256];
	XGUIDToSting(m_guid, tmp, sizeof(tmp));
	sprintf(szModel, "levels/%s/models/%s.dse", g_sLevelName.c_str(), tmp);

	IFile *pFile = Core_GetIXCore()->getFileSystem()->openFile(szModel, FILE_MODE_WRITE);
	if(!pFile)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Unable to open file '%s' for writing!\n", szModel);
		return;
	}

	IXModelWriter *pWriter = (IXModelWriter*)Core_GetIXCore()->getPluginManager()->getInterface(IXMODELWRITER_GUID);
	if(pWriter)
	{
		pWriter->writeModel(m_pResource, pFile);
	}

	mem_release(pFile);
}

void XMETHODCALLTYPE CProxyObject::getInternalData(const XGUID *pGUID, void **ppOut)
{
	if(*pGUID == X_IS_PROXY_GUID)
	{
		*ppOut = (void*)1;
	}
	else
	{
		BaseClass::getInternalData(pGUID, ppOut);
	}
}

//#############################################################################

void XMETHODCALLTYPE CModelCallback::onChanged(IXEditorModel *pModel, IXEditorObject *pObject)
{
	m_pObject->onModelChanged(pModel, pObject);
}

void XMETHODCALLTYPE CModelCallback::onObjectAdded(IXEditorModel *pModel, IXEditorObject *pObject)
{
	m_pObject->addChildObject(pObject);
}
void XMETHODCALLTYPE CModelCallback::onObjectRemoved(IXEditorModel *pModel, IXEditorObject *pObject)
{
	m_pObject->removeChildObject(pObject);
}
