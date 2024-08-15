#include "EditorObject.h"

//#include <render/sxrender.h>
#include <xcommon/IPluginManager.h>
#include <xcommon/resource/IXResourceManager.h>

#include "Editable.h"
#include "EditorModel.h"

CEditorObject::CEditorObject(CEditable *pEditable):
	m_pEditable(pEditable)
{
	XCreateGUID(&m_guid);
	pEditable->onObjectCreated(this);
	//m_szClassName = CEntityFactoryMap::GetInstance()->getClassNamePtr(szClassName);
	//assert(m_szClassName);

	m_vColor.x = 0.0f;
	m_vColor.y = randf(0.1f, 1.0f);
	m_vColor.z = randf(0.1f, 1.0f);
}

CEditorObject::~CEditorObject()
{
	m_pEditable->removeObject(this);
	
	for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
	{
		mem_delete(m_aBrushes[i]);
	}
}


void XMETHODCALLTYPE CEditorObject::setPos(const float3_t &pos)
{
	//SAFE_CALL(m_pEntity, setPos, pos);
	//SAFE_CALL(m_pModel, setPosition, pos);
	float3 vOffset = pos - m_vPos;
	for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
	{
		m_aBrushes[i]->move(vOffset);
	}
	m_vPos = pos;

	SAFE_CALL(m_pModel, onObjectChanged, this);
}

void CEditorObject::fixPos()
{
	float3 vMin, vMax;
	getBound(&vMin, &vMax);
	m_vPos = (float3)((vMin + vMax) * 0.5f);
}

void XMETHODCALLTYPE CEditorObject::setSize(const float3_t &vSize)
{
	float3 vMin, vMax;
	getBound(&vMin, &vMax);

	float3 vRelativeSize = vSize / (vMax - vMin);

	for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
	{
		//const SMAABB &aabb = m_aBrushes[i]->getAABB();

		m_aBrushes[i]->resize(m_vPos, /*(aabb.vMax - aabb.vMin) * */vRelativeSize);
	}

	SAFE_CALL(m_pModel, onObjectChanged, this);
}

void XMETHODCALLTYPE CEditorObject::setOrient(const SMQuaternion &orient)
{
	//SAFE_CALL(m_pEntity, setOrient, orient);
	//SAFE_CALL(m_pModel, setOrientation, orient);

	SMQuaternion qOffset = m_qRot.Conjugate() * orient;
	for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
	{
		m_aBrushes[i]->rotate(m_vPos, qOffset);
	}

	m_qRot = orient;

	SAFE_CALL(m_pModel, onObjectChanged, this);
}

float3_t XMETHODCALLTYPE CEditorObject::getPos()
{
	//if(m_pEntity)
	//{
	//	m_vPos = m_pEntity->getPos();
	//	SAFE_CALL(m_pModel, setPosition, m_vPos);
	//}
	return(m_vPos);
}

SMQuaternion XMETHODCALLTYPE CEditorObject::getOrient()
{
	//if(m_pEntity)
	//{
	//	m_qRot = m_pEntity->getOrient();
	//	SAFE_CALL(m_pModel, setOrientation, m_qRot);
	//}
	return(m_qRot);
}

void XMETHODCALLTYPE CEditorObject::getBound(float3 *pvMin, float3 *pvMax)
{
	*pvMin = *pvMax = float3();

	if(!m_aBrushes.size())
	{
		return;
	}

	SMAABB aabb = m_aBrushes[0]->getAABB();
	for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
	{
		aabb = SMAABBConvex(aabb, m_aBrushes[i]->getAABB());
	}

	*pvMin = aabb.vMin;
	*pvMax = aabb.vMax;
}

void XMETHODCALLTYPE CEditorObject::render(bool is3D, bool bRenderSelection, IXGizmoRenderer *pGizmoRenderer)
{
	if(bRenderSelection)
	{
		for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
		{
			m_aBrushes[i]->renderSelection(is3D, pGizmoRenderer, m_pEditable->getClipPlaneState(), m_pEditable->getClipPlane());
		}
	}
}

bool XMETHODCALLTYPE CEditorObject::rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut, float3 *pvNormal, ID *pidMtrl, bool bReturnNearestPoint)
{
	float3 vMin, vMax;
	getBound(&vMin, &vMax);
	if(SMRayIntersectAABB(SMAABB(vMin, vMax), vStart, vEnd - vStart))
	{
		if(bReturnNearestPoint)
		{
			float3 vOut, vMinOut, vNormal, *pNormal = NULL;
			if(pNormal)
			{
				pNormal = &vNormal;
			}

			bool isFound = false;
			float fDist, fDist2 = FLT_MAX;
			for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
			{
				if(m_aBrushes[i]->rayTest(vStart, vEnd, &vOut, pNormal, true, bReturnNearestPoint) &&
					(fDist = SMVector3Length2(vStart - vOut)) < fDist2
				)
				{
					vMinOut = vOut;
					fDist2 = fDist;
					isFound = true;
				}
			}

			if(isFound)
			{
				if(pvOut)
				{
					*pvOut = vMinOut;
				}
				if(pvNormal)
				{
					*pvNormal = vNormal;
				}
				return(true);
			}
		}
		else
		{
			for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
			{
				if(m_aBrushes[i]->rayTest(vStart, vEnd, pvOut, pvNormal, true, bReturnNearestPoint))
				{
					return(true);
				}
			}
		}
	}
	return(false);
}

void XMETHODCALLTYPE CEditorObject::remove()
{
	m_isRemoved = true;
	//SAFE_CALL(m_pModel, onObjectRemoved, this);
	for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
	{
		m_aBrushes[i]->enable(false);
	}
	m_isVisible = false;

	m_pEditable->onSelectionChanged(this);
}
void XMETHODCALLTYPE CEditorObject::preSetup()
{
}
void XMETHODCALLTYPE CEditorObject::postSetup()
{
}

void XMETHODCALLTYPE CEditorObject::create()
{
	m_isRemoved = false;
	for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
	{
		m_aBrushes[i]->enable(true);
	}
	m_isVisible = true;
	//SAFE_CALL(m_pModel, onObjectAdded, this);
}

void XMETHODCALLTYPE CEditorObject::setKV(const char *szKey, const char *szValue)
{
	setKV(szKey, szValue, false);
}
void CEditorObject::setKV(const char *szKey, const char *szValue, bool bSkipFixPos)
{
	if(!fstrcmp(szKey, "brush"))
	{
		IXJSON *pJSON = (IXJSON*)m_pEditable->getCore()->getPluginManager()->getInterface(IXJSON_GUID);
		IXJSONItem *pRoot;
		if(pJSON->parse(szValue, &pRoot))
		{
			setKV(szKey, pRoot, bSkipFixPos);
			
			mem_release(pRoot);
		}

		m_pEditable->getVertexTool()->onObjectTopologyChanged(this);

		fixPos();
	}
	else if(!fstrcmp(szKey, "guid"))
	{
		XGUIDFromString(&m_guid, szValue);
	}
	else if(!fstrcmp(szKey, "color"))
	{
		sscanf(szValue, "%f %f %f", &m_vColor.x, &m_vColor.y, &m_vColor.z);
		fora(i, m_aBrushes)
		{
			m_aBrushes[i]->setColor(m_vColor);
		}
	}
	//if(m_pEntity)
	//{
	//	m_pEntity->setKV(szKey, szValue);
	//}
}

void CEditorObject::setKV(const char *szKey, IXJSONItem *pValue, bool bSkipFixPos)
{
	if(!fstrcmp(szKey, "brush"))
	{
		IXJSONArray *pArray = pValue->asArray();
		if(pArray)
		{
			for(UINT i = m_aBrushes.size(), l = pArray->size(); i < l; ++i)
			{
				m_aBrushes[i] = new CBrushMesh(m_pEditable->getCore());
				m_aBrushes[i]->setColor(m_vColor);
			}

			for(UINT i = pArray->size(), l = m_aBrushes.size(); i < l; ++i)
			{
				mem_delete(m_aBrushes[i]);
			}

			m_aBrushes.resize(pArray->size());

			for(UINT i = 0, l = pArray->size(); i < l; ++i)
			{
				if(!m_aBrushes[i]->deserialize(pArray->at(i)->asObject()))
				{
					mem_delete(m_aBrushes[i]);
				}
			}

			for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
			{
				if(!m_aBrushes[i])
				{
					m_aBrushes.erase(i);
					--i; --l;
				}
			}
		}

		float3_t vOldPos = getPos();
		fixPos();
		if(!bSkipFixPos)
		{
			setPos(vOldPos);
		}

		SAFE_CALL(m_pModel, onObjectChanged, this);
	}
	else if(!fstrcmp(szKey, "guid"))
	{
		if(pValue->getType() == XJI_STRING)
		{
			setKV(szKey, pValue->getString());
		}
	}
	else if(!fstrcmp(szKey, "color"))
	{
		if(pValue->getType() == XJI_STRING)
		{
			setKV(szKey, pValue->getString());
		}
	}
}

const char* XMETHODCALLTYPE CEditorObject::getKV(const char *szKey)
{
	return(getKV(szKey, false));
}

const char* CEditorObject::getKV(const char *szKey, bool forJSON)
{
	if(!fstrcmp(szKey, "brush"))
	{
		m_aSerializedState.clearFast();
		m_aSerializedState.push_back('[');
		for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
		{
			if(i != 0)
			{
				m_aSerializedState.push_back(',');
			}

			m_aBrushes[i]->serialize(&m_aSerializedState);
		}
		m_aSerializedState.push_back(']');
		m_aSerializedState.push_back(0);

		return(m_aSerializedState);
	}
	else if(!fstrcmp(szKey, "guid"))
	{
		char tmp[64];
		if(forJSON)
		{
			tmp[0] = '"';
			XGUIDToSting(m_guid, tmp + 1, sizeof(tmp) - 1);
			char *cur = tmp;
			while(*cur)
			{
				++cur;
			}
			*cur = '"';
			++cur;
			*cur = 0;
		}
		else
		{
			XGUIDToSting(m_guid, tmp, sizeof(tmp));
		}
		m_sGUID = tmp;
		return(m_sGUID.c_str());
	}
	else if(!fstrcmp(szKey, "color"))
	{
		if(forJSON)
		{
			sprintf_s(m_szColor, "\"%f %f %f\"", m_vColor.x, m_vColor.y, m_vColor.z);
		}
		else
		{
			sprintf_s(m_szColor, "%f %f %f", m_vColor.x, m_vColor.y, m_vColor.z);
		}
		return(m_szColor);
	}
	//if(!m_pEntity)
	{
		return(NULL);
	}
#if 0
	char tmp[4096];

	if(!fstrcmp(szKey, "guid"))
	{
		XGUIDToSting(*m_pEntity->getGUID(), tmp, sizeof(tmp));
	}
	else
	{
		m_pEntity->getKV(szKey, tmp, sizeof(tmp));
	}

	m_msPropCache[szKey] = tmp;
	return(m_msPropCache[szKey].c_str());
#endif
}
const X_PROP_FIELD* XMETHODCALLTYPE CEditorObject::getPropertyMeta(UINT uKey)
{
	static X_PROP_FIELD s_prop0 = {"brush", "brush", XPET_TEXT, NULL, ""};
	static X_PROP_FIELD s_prop1 = {"guid", "GUID", XPET_TEXT, NULL, "", true};
	static X_PROP_FIELD s_prop2 = {"color", "color", XPET_TEXT, NULL, ""};
	switch(uKey)
	{
	case 0:
		return(&s_prop0);
	case 1:
		return(&s_prop1);
	case 2:
		return(&s_prop2);
	}
	return(NULL);
}
UINT XMETHODCALLTYPE CEditorObject::getProperyCount()
{
	return(3);
}

const char* XMETHODCALLTYPE CEditorObject::getTypeName()
{
	return(m_pEditable->getName());
}
const char* XMETHODCALLTYPE CEditorObject::getClassName()
{
	return("Brush");
}

void XMETHODCALLTYPE CEditorObject::setSelected(bool set)
{
	m_isSelected = set;

	m_pEditable->onSelectionChanged(this);
}

void XMETHODCALLTYPE CEditorObject::setSimulationMode(bool set)
{
	//SAFE_CALL(m_pModel, enable, !set);
}

bool XMETHODCALLTYPE CEditorObject::hasVisualModel()
{
	return(true);
}

void CEditorObject::addBrush(CBrushMesh *pBrushMesh)
{
	pBrushMesh->setColor(m_vColor);
	m_aBrushes.push_back(pBrushMesh);
}

bool CEditorObject::findFace(const float3 &vRayStart, const float3 &vRayDir, UINT *puFace, float3 *pvFacePoint)
{
	float3 vOut, vMinOut;
	
	bool isFound = false;
	float fDist, fDist2 = FLT_MAX;
	UINT uFaceId, uMinFaceId, uFaceOffset = 0;
	for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
	{
		if(m_aBrushes[i]->findFace(vRayStart, vRayDir, &uFaceId, &vOut) &&
			(fDist = SMVector3Length2(vRayStart - vOut)) < fDist2
			)
		{
			uMinFaceId = uFaceId + uFaceOffset;
			vMinOut = vOut;
			fDist2 = fDist;
			isFound = true;
		}

		uFaceOffset += m_aBrushes[i]->getFaceCount();
	}

	if(isFound)
	{
		if(puFace)
		{
			*puFace = uMinFaceId;
		}
		if(pvFacePoint)
		{
			*pvFacePoint = vMinOut;
		}
		return(true);
	}

	return(false);
}

UINT CEditorObject::getFaceCount() const
{
	UINT uFaceCount = 0;
	for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
	{
		uFaceCount += m_aBrushes[i]->getFaceCount();
	}
	return(uFaceCount);
}

void CEditorObject::renderFace(IXGizmoRenderer *pRenderer, UINT uFace)
{
	UINT uFaceCount;
	CBrushMesh *pBrush;
	for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
	{
		pBrush = m_aBrushes[i];
		uFaceCount = pBrush->getFaceCount();
		if(uFace >= uFaceCount)
		{
			uFace -= uFaceCount;
			continue;
		}

		pBrush->renderFace(pRenderer, uFace);
		break;
	}
}

void CEditorObject::getFaceInfo(UINT uFace, BrushFace *pOut)
{
	UINT uFaceCount;
	CBrushMesh *pBrush;
	for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
	{
		pBrush = m_aBrushes[i];
		uFaceCount = pBrush->getFaceCount();
		if(uFace >= uFaceCount)
		{
			uFace -= uFaceCount;
			continue;
		}

		pBrush->getFaceInfo(uFace, pOut);
		break;
	}
}

void CEditorObject::setFaceInfo(UINT uFace, const BrushFace &brushFace)
{
	UINT uFaceCount;
	CBrushMesh *pBrush;
	for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
	{
		pBrush = m_aBrushes[i];
		uFaceCount = pBrush->getFaceCount();
		if(uFace >= uFaceCount)
		{
			uFace -= uFaceCount;
			continue;
		}

		pBrush->setFaceInfo(uFace, brushFace);
		break;
	}

	SAFE_CALL(m_pModel, onObjectChanged, this);
}

void CEditorObject::getFaceExtents(UINT uFace, Extents extents)
{
	UINT uFaceCount;
	CBrushMesh *pBrush;
	for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
	{
		pBrush = m_aBrushes[i];
		uFaceCount = pBrush->getFaceCount();
		if(uFace >= uFaceCount)
		{
			uFace -= uFaceCount;
			continue;
		}

		pBrush->getFaceExtents(uFace, extents);
		break;
	}
}

int CEditorObject::classify(const SMPLANE &plane)
{
	int res, cls;
	CBrushMesh *pBrush;
	for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
	{
		pBrush = m_aBrushes[i];
		cls = pBrush->classify(plane);

		if(cls == 0)
		{
			return(0);
		}

		if(i == 0)
		{
			res = cls;
		}
		else if(res != cls)
		{
			return(0);
		}
	}

	return(res);
}

bool CEditorObject::clip(const SMPLANE &plane)
{
	int cls;
	CBrushMesh *pBrush;
	fora(i, m_aBrushes)
	{
		pBrush = m_aBrushes[i];
		cls = pBrush->classify(plane);

		if(cls < 0)
		{
			removeBrush(i);
			--i;
			--il;
		}
	}

	fora(i, m_aBrushes)
	{
		pBrush = m_aBrushes[i];
		cls = pBrush->classify(plane);

		if(cls == 0)
		{
			pBrush->clip(plane);
		}
	}

	fixPos();

	if(m_aBrushes.size() != 0)
	{
		SAFE_CALL(m_pModel, onObjectChanged, this);
	}

	return(m_aBrushes.size() != 0);
}

void CEditorObject::removeBrush(UINT idx)
{
	Array<float3_t> aVertices;

	CBrushMesh *pBrush = m_aBrushes[idx];

	while(pBrush->findInternalFace(aVertices))
	{
		fora(i, m_aBrushes)
		{
			if(i != idx && m_aBrushes[i]->fillInternalFace(aVertices))
			{
				break;
			}
		}
		aVertices.clearFast();
	}

	mem_delete(m_aBrushes[idx]);
	m_aBrushes.erase(idx);
}

void CEditorObject::buildMesh(CMeshBuilder *pBuilder)
{
	fora(i, m_aBrushes)
	{
		m_aBrushes[i]->buildMesh(pBuilder);
	}
}

void CEditorObject::setModel(CEditorModel *pModel)
{
	m_pModel = pModel;
	// destroy model and physics

	fora(i, m_aBrushes)
	{
		m_aBrushes[i]->setFinalized(pModel != NULL);
	}
}

void CEditorObject::buildPhysbox(IXResourceModel *pResource)
{
	fora(i, m_aBrushes)
	{
		m_aBrushes[i]->buildPhysbox(pResource);
	}
}

UINT CEditorObject::getVertexCount()
{
	UINT uVertexCount = 0;
	for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
	{
		uVertexCount += m_aBrushes[i]->getVertexCount();
	}
	return(uVertexCount);
}

const float3_t& CEditorObject::getVertexAt(UINT uVertex)
{
	UINT uVertexCount;
	CBrushMesh *pBrush;
	for(UINT i = 0, l = m_aBrushes.size(); i < l; ++i)
	{
		pBrush = m_aBrushes[i];
		uVertexCount = pBrush->getVertexCount();
		if(uVertex >= uVertexCount)
		{
			uVertex -= uVertexCount;
			continue;
		}

		return(pBrush->getVertexAt(uVertex));
	}

	assert(!"Invalid index!");
	static float3_t a;
	return(a);
}

UINT CEditorObject::moveVertices(UINT *puAffectedVertices, UINT uVertexCount, const float3 &vDeltaPos)
{
	if(uVertexCount == getVertexCount())
	{
		setPos((float3)(getPos() + vDeltaPos));
		return(0);
	}

	bool isSorted = true;
	for(UINT i = 1; i < uVertexCount; ++i)
	{
		if(puAffectedVertices[i - 1] >= puAffectedVertices[i])
		{
			isSorted = false;
		}
	}
	assert(isSorted);
	if(!isSorted)
	{
		return(0);
	}

	{
		UINT uStart = 0;
		UINT uBrush = 0;
		UINT uBrushVC;
		UINT uCount;
		UINT uOffset = 0;
		while(uStart < uVertexCount && uBrush < m_aBrushes.size())
		{
			uBrushVC = m_aBrushes[uBrush]->getVertexCount();

			uCount = 0;
			for(UINT i = uStart; i < uVertexCount; ++i)
			{
				if(puAffectedVertices[i] < uBrushVC + uOffset)
				{
					++uCount;
				}
				else
				{
					break;
				}
			}

			if(uCount)
			{
				if(!m_aBrushes[uBrush]->couldMoveVertices(puAffectedVertices + uStart, uCount, uOffset, vDeltaPos))
				{
					// couldn't move
					return(0);
				}
				uStart += uCount;
			}

			++uBrush;
			uOffset += uBrushVC;
		}
	}

	UINT uTotalRemoved = 0;
	{
		UINT uStart = 0;
		UINT uCount;
		UINT uBrush = 0;
		UINT uBrushVC;
		UINT uWriteStart = 0;
		UINT uRemoved;
		UINT uOffset = 0;
		while(uStart < uVertexCount && uBrush < m_aBrushes.size())
		{
			uBrushVC = m_aBrushes[uBrush]->getVertexCount();

			uCount = 0;
			for(UINT i = uStart; i < uVertexCount; ++i)
			{
				if(puAffectedVertices[i] < uBrushVC)
				{
					++uCount;
				}
				else
				{
					puAffectedVertices[i] -= uBrushVC;
				}
			}

			if(uCount)
			{
				uRemoved = m_aBrushes[uBrush]->moveVertices(puAffectedVertices + uStart, uCount, vDeltaPos, puAffectedVertices + uWriteStart);
				if(uRemoved)
				{
					uTotalRemoved += uRemoved;
					for(UINT i = uWriteStart, l = uWriteStart + uRemoved; i < l; ++i)
					{
						puAffectedVertices[i] += uOffset;
					}
					uWriteStart += uRemoved;
				}
				uStart += uCount;
			}

			uOffset += uBrushVC;
			++uBrush;
		}
	}
	
	fixPos();

	SAFE_CALL(m_pModel, onObjectChanged, this);

	return(uTotalRemoved);
}
