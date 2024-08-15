#include "Scene.h"
#include <xcommon/resource/IXModel.h>
#include <xcommon/resource/IXResourceManager.h>
#include <xcommon/resource/IXModelProvider.h>
#include <xcommon/IPluginManager.h>
#include <xcommon/render/IXOcclusionCuller.h>
#include <xcommon/editor/IXEditorExtension.h>
#include <xcommon/editor/IXEditable.h>
#include <xcommon/render/IXRenderUtils.h>
#include <queue>

CSceneObject::CSceneObject(CScene *pScene, const SMAABB &aabb, void *pUserData, NodeType bmType, NodeFeature bmFeatures, UINT uLayer):
	m_pScene(pScene),
	m_aabb(aabb),
	m_pUserData(pUserData),
	m_bmFeatures(bmFeatures),
	m_bmType(bmType),
	m_bmLayer(1 << uLayer)
{
}
CSceneObject::~CSceneObject()
{
	m_pScene->removeObject(this);
}

void XMETHODCALLTYPE CSceneObject::update(const SMAABB &aabb)
{
	m_pScene->enqueueObjectUpdate(this, aabb);
}

void CSceneObject::setNode(CSceneNode *pNode)
{
	m_pNode = pNode;
}
CSceneNode* CSceneObject::getNode()
{
	return(m_pNode);
}

const SMAABB& CSceneObject::getAABB() const
{
	return(m_aabb);
}

void* CSceneObject::getUserData() const
{
	return(m_pUserData);
}

void XMETHODCALLTYPE CSceneObject::FinalRelease()
{
	m_pScene->enqueueObjectDelete(this);
}

void CSceneObject::updateFeatures(NodeFeature bmFeatures)
{
	m_pScene->enqueueObjectUpdateFeatures(this, bmFeatures);
}

void CSceneObject::updateLayer(UINT bmLayer)
{
	m_pScene->enqueueObjectUpdateLayer(this, bmLayer);
}

void XMETHODCALLTYPE CSceneObject::setFeature(IXSceneFeature *pFeat, bool isSet)
{
	CSceneFeature *pFeature = (CSceneFeature*)pFeat;
	NodeFeature bmFeatures = m_bmFeatures;

	if(isSet)
	{
		bmFeatures |= pFeature->getFeature();
	}
	else
	{
		bmFeatures &= ~pFeature->getFeature();
	}

	updateFeatures(bmFeatures);
}

void XMETHODCALLTYPE CSceneObject::setFeatures(IXSceneFeature **ppFeatures)
{
	/*CSceneFeature *pFeature = (CSceneFeature*)pFeat;*/
	NodeFeature bmFeatures = 0;

	while(*ppFeatures)
	{
		bmFeatures |= ((CSceneFeature*)(*ppFeatures))->getFeature();
		++ppFeatures;
	}

	updateFeatures(bmFeatures);
}

void XMETHODCALLTYPE CSceneObject::setLayer(UINT uLayer)
{
	assert(uLayer < sizeof(m_bmLayer) * 8);
	m_bmLayer = 1 << uLayer;

	updateLayer(m_bmLayer);
}

//##########################################################################

#if BVH_CHILD_COUNT == 27
static const UINT gcs_puOrders[][BVH_CHILD_COUNT] = {
	// +x +y +z
	{13, 4, 10, 12, 1, 3, 9, 14, 16, 22, 0, 5, 7, 11, 15, 19, 21, 2, 6, 17, 18, 23, 25, 8, 20, 24, 26},
	// +x +y -z
	{22, 4, 19, 21, 1, 3, 13, 18, 23, 25, 0, 5, 7, 10, 12, 20, 24, 2, 6, 9, 14, 16, 26, 8, 11, 15, 17},
	// +x -y +z
	{16, 7, 10, 15, 1, 6, 9, 13, 17, 25, 0, 4, 8, 11, 12, 19, 24, 2, 3, 14, 18, 22, 26, 5, 20, 21, 23},
	// +x -y -z
	{25, 7, 19, 24, 1, 6, 16, 18, 22, 26, 0, 4, 8, 10, 15, 20, 21, 2, 3, 9, 13, 17, 23, 5, 11, 12, 14},
	// -x +y +z
	{14, 5, 11, 12, 2, 3, 9, 13, 17, 23, 0, 4, 8, 10, 15, 20, 21, 1, 6, 16, 18, 22, 26, 7, 19, 24, 25},
	// -x +y -z
	{23, 5, 20, 21, 2, 3, 14, 18, 22, 26, 0, 4, 8, 11 ,12, 19, 24, 1, 6, 9, 13, 17, 25, 7, 10, 15, 16},
	// -x -y +z
	{17, 8, 11, 15, 2, 6, 9, 14, 16, 26, 0, 5, 7, 10, 12, 20, 24, 1, 3, 13, 18, 23, 25, 4, 19, 21, 22},
	// -x -y -z
	{26, 8, 20, 24, 2, 6, 17, 18, 23, 25, 0, 5, 7, 11, 15, 19, 21, 1, 3, 9, 14, 16, 22, 4, 10, 12, 13}
};
#elif BVH_CHILD_COUNT == 8
static const UINT gcs_puOrders[][BVH_CHILD_COUNT] = {
	// +x +y +z
	{0, 1, 2, 4, 3, 5, 6, 7},
	// +x +y -z
	{4, 0, 5, 6, 1, 2, 7, 3},
	// +x -y +z
	{2, 0, 3, 6, 1, 4, 7, 5},
	// +x -y -z
	{6, 2, 4, 7, 0, 3, 5, 1},
	// -x +y +z
	{1, 0, 3, 5, 2, 4, 7, 6},
	// -x +y -z
	{5, 1, 4, 7, 0, 3, 6, 2},
	// -x -y +z
	{3, 1, 2, 7, 0, 5, 6, 4},
	// -x -y -z
	{7, 3, 5, 6, 1, 2, 4, 0}
};
#else
#error errro
#endif

CSceneQuery::CSceneQuery(CScene *pScene, CSceneObjectType *pObjectType):
	m_pScene(pScene),
	m_bmType(pObjectType->getType())
{
}
CSceneQuery::~CSceneQuery()
{
}

UINT XMETHODCALLTYPE CSceneQuery::execute(const IXFrustum *pFrustum, const float3 &vDir, void ***pppObjects, IXOcclusionCuller *pOcclusionCuller)
{
	pOrder = gcs_puOrders[(vDir.x < 0.0f ? 4 : 0) + (vDir.y < 0.0f ? 2 : 0) + (vDir.z < 0.0f ? 1 : 0)];

	return(execute(pFrustum, pppObjects, pOcclusionCuller));
}

UINT XMETHODCALLTYPE CSceneQuery::execute(const IXFrustum *pFrustum, void ***pppObjects, IXOcclusionCuller *pOcclusionCuller)
{
	XPROFILE_FUNCTION();

	if(!pOrder)
	{
		pOrder = gcs_puOrders[0];
	}
	m_aQueryResponse.clearFast();

	queryObjectsInternal(m_pScene->m_pRootNode, pFrustum, false, pOcclusionCuller);

	if(m_aQueryResponse.size())
	{
		*pppObjects = &m_aQueryResponse[0];
	}
	else
	{
		*pppObjects = NULL;
	}
	return(m_aQueryResponse.size());
}

void CSceneQuery::queryObjectsLeaf(CSceneNode *pNode, const IXFrustum *pFrustum, bool isFullyVisible, IXOcclusionCuller *pOcclusionCuller)
{
	if(!pNode)
	{
		return;
	}

	XPROFILE_FUNCTION();

	auto &aObjects = pNode->getObjects();
	for(UINT i = 0, l = aObjects.size(); i < l; ++i)
	{
		auto &pObj = aObjects[i];

		//if(!(pObj->getType() == m_bmType && testFeatures(pObj->getFeatures())))
		//{
		//	continue;
		//}

		if(
			pObj->getType() == m_bmType && 
			(pObj->getLayer() & m_bmLayerMask) &&
			testFeatures(pObj->getFeatures()) && 
			(isFullyVisible || (pOcclusionCuller ? pOcclusionCuller->isAABBvisible(pObj->getAABB()) : pFrustum->boxInFrustum(pObj->getAABB()))) &&
			testSize(pObj->getAABB())
		)
		{
			m_aQueryResponse.push_back(pObj->getUserData());
		}
	}
}


void CSceneQuery::queryObjectsInternal(CSceneNode *pNode, const IXFrustum *pFrustum, bool isFullyVisible, IXOcclusionCuller *pOcclusionCuller)
{
	if(!pNode)
	{
		return;
	}

	XPROFILE_FUNCTION();

	if((pNode->getTypes() & m_bmType) == 0 || (pNode->getLayerMask() & m_bmLayerMask) == 0 || !testFeatures(pNode->getFeatures(), false))
	{
		return;
	}

	/*if(!isFullyVisible)
	{
		if(!pFrustum->boxInFrustum(pNode->getAABB()))
		{
			return;
		}

		isFullyVisible = pFrustum->boxInFrustum(pNode->getAABB(), true);
	}*/
	if(!isFullyVisible && !(pOcclusionCuller ? pOcclusionCuller->isAABBvisible(pNode->getAABB(), &isFullyVisible) : pFrustum->boxInFrustum(pNode->getAABB(), &isFullyVisible)))
	{
		return;
	}

	if(!testSize(pNode->getAABB()))
	{
		return;
	}

	for(UINT i = 0; i < BVH_CHILD_COUNT; ++i)
	{
		queryObjectsInternal(pNode->getChild(pOrder[i], false), pFrustum, isFullyVisible, pOcclusionCuller);
	}

	queryObjectsLeaf(pNode, pFrustum, isFullyVisible, pOcclusionCuller);
}

void XMETHODCALLTYPE CSceneQuery::setOP(XSCENE_QUERY_OP op)
{
	m_op = op;
}

void XMETHODCALLTYPE CSceneQuery::setFeature(IXSceneFeature *pFeat, XSCENE_QUERY_FEATURE mode)
{
	CSceneFeature *pFeature = (CSceneFeature*)pFeat;

	NodeFeature bmFeature = pFeature->getFeature();

	switch(mode)
	{
	case SQF_SET:
		m_bmSet |= bmFeature;
		m_bmUnset &= ~bmFeature;
		break;
	case SQF_UNSET:
		m_bmSet &= ~bmFeature;
		m_bmUnset |= bmFeature;
		break;
	case SQF_ANY:
		m_bmSet &= ~bmFeature;
		m_bmUnset &= ~bmFeature;
		break;
	}
}

void XMETHODCALLTYPE CSceneQuery::setScreenSizeCulling(const float3 &vCamPos, float fFov, float fScreenHeightPx, float fThresholdPx)
{
	m_useScreenSizeCulling = true;
	m_vCamPos = vCamPos;
	m_fThresholdPx = fThresholdPx;

	float fTan = tanf(fFov * 0.5f);
	m_fScreenSizeCoeff = 0.5f * fScreenHeightPx * fTan;
	m_fScreenSizeCoeff *= m_fScreenSizeCoeff;
}

void XMETHODCALLTYPE CSceneQuery::unsetScreenSizeCulling()
{
	m_useScreenSizeCulling = false;
}

void XMETHODCALLTYPE CSceneQuery::setLayerMask(UINT uLayerMask)
{
	m_bmLayerMask = uLayerMask;
}

bool CSceneQuery::testFeatures(NodeFeature bmFeatures, bool isStrict)
{
	bool isPassed = m_bmSet == 0;
	if(!isPassed)
	{
		NodeFeature bmSet = m_bmSet & bmFeatures;
		switch(m_op)
		{
		case SQO_AND:
			isPassed = bmSet == m_bmSet;
			break;
		case SQO_OR:
			isPassed = bmSet != 0;
			break;
		}
	}

	if((isPassed && m_op == SQO_AND || m_op == SQO_OR) && isStrict && m_bmUnset)
	{
		NodeFeature bmUnset = m_bmUnset & ~bmFeatures;
		switch(m_op)
		{
		case SQO_AND:
			isPassed = bmUnset == m_bmUnset;
			break;
		case SQO_OR:
			isPassed = bmUnset != 0;
			break;
		}
	}

	return(isPassed);
}

bool CSceneQuery::testSize(const SMAABB &aabb)
{
	if(!m_useScreenSizeCulling)
	{
		return(true);
	}

	float3 vPos = (aabb.vMax + aabb.vMin) * 0.5f; // use any point?
	float fD2 = SMVector3Length2(vPos - m_vCamPos);
	float3 vSize = aabb.vMax - aabb.vMin;
	float fS0 = 0.5f * (vSize.x * vSize.y + vSize.y * vSize.z + vSize.x * vSize.z);
	float fS = fS0 * m_fScreenSizeCoeff / fD2;

	return(fS > m_fThresholdPx);
}

//##########################################################################

CSceneObjectType::CSceneObjectType(CScene *pScene, UINT uId):
	m_pScene(pScene),
	m_bmType(1 << uId)
{
}
CSceneObjectType::~CSceneObjectType()
{
}

IXSceneObject* XMETHODCALLTYPE CSceneObjectType::newObject(const SMAABB &aabb, void *pUserData, IXSceneFeature **ppFeatures, UINT uLayer)
{
	return(m_pScene->newObject(aabb, pUserData, m_bmType, ppFeatures, uLayer));
}
IXSceneQuery* XMETHODCALLTYPE CSceneObjectType::newQuery()
{
	return(new CSceneQuery(m_pScene, this));
}

//##########################################################################

int s_splitval = 42;

CSceneNode::CSceneNode(CScene *pScene, CSceneNode *pParent):
	m_pScene(pScene),
	m_pParent(pParent)
{
	for(UINT i = 0; i < BVH_CHILD_COUNT; ++i)
	{
		m_pChildren[i] = NULL;
	}
}
CSceneNode::~CSceneNode()
{
	for(UINT i = 0; i < BVH_CHILD_COUNT; ++i)
	{
		m_pScene->deleteNode(m_pChildren[i]);
		m_pChildren[i] = NULL;
	}

	for(UINT i = 0, l = m_aObjects.size(); i < l; ++i)
	{
		m_aObjects[i]->setNode(NULL);
	}
}

void CSceneNode::addObject(CSceneObject *pObject, bool force)
{
	assert(force || !pObject->getNode());

	CSceneNode *pNode = findNode(pObject->getAABB());
	assert(pNode);

	pNode->insertObject(pObject);
	SMAABB aabb = pObject->getAABB();
	pNode->growExtents(aabb);

	pNode->updateFeatures();
}
void CSceneNode::removeObject(CSceneObject *pObject)
{
	assert(pObject->getNode() == this);

	removeObject(pObject, pObject->getAABB());
}

bool CSceneNode::removeObject(CSceneObject *pObject, const SMAABB &aabbOld)
{
	shrinkExtents(aabbOld);
	int idx = m_aObjects.indexOf(pObject);
	assert(idx >= 0);
	if(idx < 0)
	{
		// cerr << "Warning: octree " << this
		// 	<< " tried to remove node "
		// 	<< n << " which it doesn’t contain" << endl;
		if(m_pParent)
		{
			// cerr << "Parent says it should be in "
			// 	<< m_parent->FindNode(n) << endl;
			// cerr << "Trying parent" << endl;
			return(m_pParent->removeObject(pObject, pObject->getAABB()));
		}
		return(false);
	}
	m_aObjects.erase(idx);

	updateFeatures();

	// THIS MUST COME LAST
	testSuicide();
	return(true);
}

void CSceneNode::updateFeatures()
{
	CSceneNode *pNode = this;
	NodeFeature bmNewFeatures;
	NodeType bmNewTypes;
	UINT bmNewLayer;

	while(pNode)
	{
		bmNewFeatures = 0;
		bmNewTypes = 0;
		bmNewLayer = 0;

		for(UINT i = 0; i < BVH_CHILD_COUNT; ++i)
		{
			if(pNode->m_pChildren[i])
			{
				bmNewFeatures |= pNode->m_pChildren[i]->m_bmFeatures;
				bmNewTypes |= pNode->m_pChildren[i]->m_bmTypes;
				bmNewLayer |= pNode->m_pChildren[i]->m_bmLayerMask;
			}
		}

		for(UINT i = 0, l = pNode->m_aObjects.size(); i < l; ++i)
		{
			bmNewFeatures |= pNode->m_aObjects[i]->getFeatures();
			bmNewTypes |= pNode->m_aObjects[i]->getType();
			bmNewLayer |= pNode->m_aObjects[i]->getLayer();
		}

		if(pNode->m_bmFeatures == bmNewFeatures && pNode->m_bmTypes == bmNewTypes && pNode->m_bmLayerMask == bmNewLayer)
		{
			break;
		}
		
		pNode->m_bmFeatures = bmNewFeatures;
		pNode->m_bmTypes = bmNewTypes;
		pNode->m_bmLayerMask = bmNewLayer;

		pNode = pNode->m_pParent;
	}
}

bool CSceneNode::updateObject(CSceneObject *pObject, const SMAABB &aabbOld)
{
	SMAABB aabb = pObject->getAABB();
	CSceneNode *pNode = this;
	
	while(pNode->m_pParent && !SMIsAABBInsideAABB(aabb, pNode->getAABB(false)))
	{
		pNode = pNode->m_pParent;
	}

	// Find its ﬁnal reinsertion point
	pNode = pNode->findNode(pObject->getAABB());
	if(pNode != this)
	{
		// Moved to a new node
		pNode->addObject(pObject, true);
		if(!removeObject(pObject, aabbOld))
		{
			return(false);
		}
	}
	else
	{
		// Just update the extents of this node
		growExtents(aabb);
		shrinkExtents(aabbOld);
	}
	return(true);
}

const SMAABB& CSceneNode::getAABB(bool shouldSplit)
{
	if(!m_isExtentsCorrect)
	{
		ScopedSpinLock lock(m_lock);

		if(!m_isExtentsCorrect)
		{
			updateExtents(shouldSplit);
		}
	}

	if(shouldSplit)
	{

		if(!m_isSplit && m_aObjects.size() >= (UINT)s_splitval)
		{
			ScopedSpinLock lock(m_lock);

			if(!m_isSplit && m_aObjects.size() >= (UINT)s_splitval)
			{
				doSplit();
			}
		}
	}
	return(m_aabb);
}

CSceneNode* CSceneNode::getChild(int idx, bool shouldCreate)
{
	assert(idx < BVH_CHILD_COUNT);

	if(idx < 0)
	{
		return(this);
	}

	if(!m_pChildren[idx] && shouldCreate)
	{
		m_pChildren[idx] = m_pScene->newNode(this);
	}

	return(m_pChildren[idx]);
}

CSceneNode* CSceneNode::findNode(const SMAABB &aabb)
{
	CSceneNode *pNode = this;
	int c = selectChild(aabb);
	while(c >= 0)
	{
		pNode = pNode->getChild(c);
		c = pNode->selectChild(aabb);
	}
	return(pNode);
}

int CSceneNode::selectChild(const SMAABB &aabb)
{
	if(!m_isSplit)
	{
		return(-1);
	}

	int c = 0;

	// If this object’s bounding volume is at least 1/8
	// the last known tree bounding volume, just keep it
	// here
	SMAABB aabbIntersection;
	if(SMAABBIntersectAABB(aabb, m_aabb, &aabbIntersection)
		&& (SMAABBVolume(aabbIntersection) > SMAABBVolume(m_aabb) * 0.125f))
	{
		return(-1);
	}

#if BVH_CHILD_COUNT == 27
	if(aabb.vMax.x < m_vSplit.x)
	{
		c += 1;
	}
	else if(aabb.vMin.x > m_vSplit.x)
	{
		c += 2;
	}

	if(aabb.vMax.y < m_vSplit.y)
	{
		c += 3;
	}
	else if(aabb.vMin.y > m_vSplit.y)
	{
		c += 6;
	}

	if(aabb.vMax.z < m_vSplit.z)
	{
		c += 9;
	}
	else if(aabb.vMin.z > m_vSplit.z)
	{
		c += 18;
	}
#elif BVH_CHILD_COUNT == 8
	float3 vCenter = (aabb.vMax + aabb.vMin) * 0.5f;
	if(aabb.vMax.x < m_vSplit.x)
	{
		c += 0;
	}
	else if(aabb.vMin.x > m_vSplit.x || vCenter.x > m_vSplit.x)
	{
		c += 1;
	}

	if(aabb.vMax.y < m_vSplit.y)
	{
		c += 0;
	}
	else if(aabb.vMin.y > m_vSplit.y || vCenter.y > m_vSplit.y)
	{
		c += 2;
	}

	if(aabb.vMax.z < m_vSplit.z)
	{
		c += 0;
	}
	else if(aabb.vMin.z > m_vSplit.z || vCenter.z > m_vSplit.z)
	{
		c += 4;
	}
#else
#error errro
#endif

	assert(c < BVH_CHILD_COUNT);
	//{
	//	cerr << this << "s == " << m_issplit
	//		<< ":" << m_split.x << ","
	//		<< m_split.y << ","
	//		<< m_split.z
	//		<< " c == " << c
	//		<< " sz == " << m_children.size() << "/"
	//		<< TREETYPE << endl;
	//	abort();
	//}
	return(c);
}

void CSceneNode::insertObject(CSceneObject *pObject)
{
	m_aObjects.push_back(pObject);
	pObject->setNode(this);
}

void CSceneNode::growExtents(const SMAABB &aabb)
{
	CSceneNode *pNode = this;
	while(pNode)
	{
		if(pNode->m_isExtentsCorrect)
		{
			pNode->m_aabb = SMAABBConvex(pNode->m_aabb, aabb);
		}
		pNode = pNode->m_pParent;
	}
}

void CSceneNode::shrinkExtents(const SMAABB &aabb)
{
	CSceneNode *pNode = this;
	while(pNode)
	{
		if(
			aabb.vMin.x <= pNode->m_aabb.vMin.x
		 || aabb.vMin.y <= pNode->m_aabb.vMin.y
		 || aabb.vMin.z <= pNode->m_aabb.vMin.z

		 || aabb.vMax.x >= pNode->m_aabb.vMax.x
		 || aabb.vMax.y >= pNode->m_aabb.vMax.y
		 || aabb.vMax.z >= pNode->m_aabb.vMax.z
		)
		{
			pNode->m_isExtentsCorrect = false;
		}

		pNode = pNode->m_pParent;
	}
}

void CSceneNode::testSuicide()
{
	if(m_pParent)
	{
		bool killme = !m_aObjects.size();

		for(UINT i = 0; killme && i < BVH_CHILD_COUNT; ++i)
		{
			killme = !m_pChildren[i];
		}

		if(killme)
		{
			m_pParent->removeChild(this);
		}
	}
}

void CSceneNode::removeChild(CSceneNode *pNode)
{
	bool shouldUnsplit = true;

	for(UINT i = 0; i < BVH_CHILD_COUNT; ++i)
	{
		CSceneNode *pTmp = m_pChildren[i];

		if(m_pChildren[i] == pNode)
		{
			m_pScene->deleteNode(m_pChildren[i]);
			m_pChildren[i] = NULL;
		}
		else if(m_pChildren[i])
		{
			shouldUnsplit = false;
		}
	}
	if(shouldUnsplit)
	{
		unsplit();
	}

	testSuicide();
}

void CSceneNode::unsplit()
{
	m_isSplit = false;
	for(UINT i = 0; i < BVH_CHILD_COUNT; ++i)
	{
		assert(!m_pChildren[i]);
	}
}

static bool IsPointInsideAABB(const float3 &vPoint, const SMAABB &aabb)
{
	return(
		vPoint.x > aabb.vMin.x &&
		vPoint.y > aabb.vMin.y &&
		vPoint.z > aabb.vMin.z &&
		vPoint.x < aabb.vMax.x &&
		vPoint.y < aabb.vMax.y &&
		vPoint.z < aabb.vMax.z
	);
}

void CSceneNode::takeChildren(CSceneNode *pNode)
{
	for(UINT i = 0; i < BVH_CHILD_COUNT; ++i)
	{
		if(pNode->m_pChildren[i])
		{
			takeChildren(pNode->m_pChildren[i]);
		}
	}

	forar(i, pNode->m_aObjects)
	{
		CSceneObject *n = pNode->m_aObjects[i];
		insertObject(n);
		pNode->removeObject(n, n->getAABB());
	}
}

void CSceneNode::updateExtents(bool shouldRebalance)
{
	bool set = false;

	float3 vNewSplit;
	int ct = 0;

	for(UINT i = 0, l = m_aObjects.size(); i < l; ++i)
	{
		const SMAABB &aabb = m_aObjects[i]->getAABB();

		if(!set)
		{
			m_aabb = aabb;
			set = true;
		}
		else
		{
			m_aabb = SMAABBConvex(m_aabb, aabb);
		}
		vNewSplit += (aabb.vMin + aabb.vMax) * 0.5f;
		ct++;
	}

	for(UINT i = 0; i < BVH_CHILD_COUNT; ++i)
	{
		if(m_pChildren[i])
		{
			const SMAABB &aabb = m_pChildren[i]->getAABB(false);

			if(!set)
			{
				m_aabb = aabb;
				set = true;
			}
			else
			{
				m_aabb = SMAABBConvex(m_aabb, aabb);
			}

			vNewSplit += m_pChildren[i]->m_vSplit * (float)s_splitval;
			ct += s_splitval;
		}
	}

	m_vSplit = vNewSplit / (float)ct;

	if(shouldRebalance)
	{
		for(UINT i = 0; i < BVH_CHILD_COUNT; ++i)
		{
			if(m_pChildren[i])
			{
				if(IsPointInsideAABB(m_vSplit, m_pChildren[i]->getAABB(false)))
				{
					// unsplit all
					for(i = 0; i < BVH_CHILD_COUNT; ++i)
					{
						if(m_pChildren[i])
						{
							takeChildren(m_pChildren[i]);
						}
					}
					break;
				}
			}
		}
	}

	m_isExtentsCorrect = true;
}

void CSceneNode::doSplit()
{
	assert(!m_isSplit);
	assert(m_aObjects.size());

	// Find the split point
	m_vSplit = float3();
	float ttl = 0.0f;

	float3 vMinSum;
	for(UINT i = 0, l = m_aObjects.size(); i < l; ++i)
	// for (Nodes::iterator iter = m_nodes.begin(); iter != m_nodes.end(); ++iter)
	{
		vMinSum += m_aObjects[i]->getAABB().vMin;
		m_vSplit += m_aObjects[i]->getAABB().vMax;
		ttl += 1.0f;
	}

	m_vSplit += vMinSum;
	m_vSplit /= ttl * 2.0f;
	m_isSplit = true;

	bool safe = false, first = true;
	int lc = 0;
	//Array<CSceneObject*> here;
	int iHere = 0;
	for(UINT i = 0, l = m_aObjects.size(); i < l; ++i)
	// for (Nodes::iterator iter = m_nodes.begin(); iter != m_nodes.end(); ++iter)
	{
		int c = selectChild(m_aObjects[i]->getAABB());
		// Make sure at least one object doesn’t go into the
		// same child as everyone else
		if(!safe)
		{
			if(c < 0)
			{
				safe = true;
			}
			else
			{
				if(first)
				{
					lc = c;
					first = false;
				}
				else
				{
					safe = (c != lc);
				}
			}
		}
		if(c < 0)
		{
			m_aObjects[iHere++] = m_aObjects[i];
			//here.push_back(m_aObjects[i]);
		}
		else
		{
			getChild(c)->insertObject(m_aObjects[i]);
		}
	}
	m_aObjects.resizeFast(iHere);
	//m_aObjects.swap(here);


	if(!safe)
	{
		// Oops, all objects went into the same child node!
		// Take them back.
		CSceneNode *cn = getChild(lc);

		
		forar(i, cn->m_aObjects)
		{
			CSceneObject *n = cn->m_aObjects[i];
			cn->removeObject(n);
			insertObject(n);
		}

		// The last removal marked this node as unsplit, so
		// we’ll just have to do this all over again next time
		// this AABB is queried, unless we simply keep this
		// node split
		m_isSplit = true;
	}
	for(UINT i = 0; i < BVH_CHILD_COUNT; ++i)
	// for (Children::iterator iter = m_children.begin(); iter != m_children.end(); ++iter)
	{
		if(m_pChildren[i])
		{
			m_pChildren[i]->updateExtents();
			m_pChildren[i]->updateFeatures();

			if(m_pChildren[i]->m_bmTypes == 0)
			{
				int a = 0;
			}
		}
	}

	if(!safe)
	{
		updateFeatures();
	}
}

Array<CSceneObject*>& CSceneNode::getObjects()
{
	return(m_aObjects);
}

bool CSceneNode::validate()
{
	const SMAABB &aabb = getAABB();
	fora(i, m_aObjects)
	{
		if(!SMIsAABBInsideAABB(m_aObjects[i]->getAABB(), aabb))
		{
			assert(!"Object's bound outside node's bound!");
			return(false);
		}
	}

	for(UINT i = 0; i < BVH_CHILD_COUNT; ++i)
	{
		if(m_pChildren[i])
		{
			if(!SMIsAABBInsideAABB(m_pChildren[i]->getAABB(), aabb))
			{
				assert(!"Child node's bound outside node's bound!");
				return(false);
			}

			if(!m_pChildren[i]->validate())
			{
				return(false);
			}
		}
	}

	return(true);
}

void CSceneNode::print(UINT uLvl, UINT *puNodesCount, UINT *puObjectsCount, UINT *puMaxDepth)
{
	for(UINT i = 0; i < uLvl; ++i)
	{
		//	printf("-");
	}

	//printf("O: %u\n", m_aObjects.size());

	if(puNodesCount)
	{
		*puNodesCount += 1;
	}

	if(puObjectsCount)
	{
		*puObjectsCount += m_aObjects.size();
	}

	if(puMaxDepth && *puMaxDepth < uLvl)
	{
		*puMaxDepth = uLvl;
	}

	for(UINT i = 0; i < BVH_CHILD_COUNT; ++i)
	{
		if(m_pChildren[i])
		{
			m_pChildren[i]->print(uLvl + 1, puNodesCount, puObjectsCount, puMaxDepth);
		}
	}
}

//##########################################################################

class CDevBVHrenderInc final: public IXConsoleCommand
{
public:
	CDevBVHrenderInc(IXCore *pCore):
		m_pCore(pCore)
	{
	}
	void XMETHODCALLTYPE execute(int argc, const char **argv) override
	{
		auto pConsole = m_pCore->getConsole();
		static const int *dev_bvh_render_inc = pConsole->getPCVarInt("dev_bvh_render_level");
		pConsole->execCommand2("dev_bvh_render_level %d", *dev_bvh_render_inc + 1);
	}

private:
	IXCore *m_pCore;
};

class CDevBVHrenderDec final: public IXConsoleCommand
{
public:
	CDevBVHrenderDec(IXCore *pCore):
		m_pCore(pCore)
	{
	}
	void XMETHODCALLTYPE execute(int argc, const char **argv) override
	{
		auto pConsole = m_pCore->getConsole();
		static const int *dev_bvh_render_inc = pConsole->getPCVarInt("dev_bvh_render_level");
		if(*dev_bvh_render_inc >= 0)
		{
			pConsole->execCommand2("dev_bvh_render_level %d", *dev_bvh_render_inc - 1);
		}
	}

private:
	IXCore *m_pCore;
};

class CDevBVHValidate final: public IXConsoleCommand
{
public:
	CDevBVHValidate(CScene *pScene):
		m_pScene(pScene)
	{
	}
	void XMETHODCALLTYPE execute(int argc, const char **argv) override
	{
		bool isValid = m_pScene->validate();
		LogInfo("Scene is %s" COLOR_RESET "\n", isValid ? COLOR_GREEN "valid" : COLOR_LRED "invalid");
		m_pScene->print();
	}

private:
	CScene *m_pScene;
};

class CCvarListener final: public IEventListener<XEventCvarChanged>
{
public:
	CCvarListener(IXCore *pCore, CScene *pScene):
		m_pCore(pCore),
		m_pScene(pScene)
	{
	}
	void onEvent(const XEventCvarChanged *pData) override
	{
		static const int *dev_bvh_render_inc = m_pCore->getConsole()->getPCVarInt("dev_bvh_render_level");
		
		if(pData->pCvar == dev_bvh_render_inc)
		{
			m_pScene->drawLevel(*dev_bvh_render_inc);
		}
	}

private:
	IXCore *m_pCore;
	CScene *m_pScene;
};

class CEditorExt final: public IXUnknownImplementation<IXEditorExtension>
{
public:
	CEditorExt(CScene *pScene, IXCore *pCore)
	{
		m_pCore = pCore;
		m_pScene = pScene;
	}
	~CEditorExt()
	{
		mem_release(m_pRenderer);
	}
	UINT XMETHODCALLTYPE getPropertyTabCount() override
	{
		return(0);
	}
	IXEditorPropertyTab* XMETHODCALLTYPE getPropertyTab(UINT uId) override
	{
		return(NULL);
	}

	UINT XMETHODCALLTYPE getToolCount() override
	{
		return(0);
	}
	bool XMETHODCALLTYPE getTool(UINT uId, IXEditorTool **ppOut) override
	{
		return(false);
	}

	void XMETHODCALLTYPE render(bool is3D) override
	{
		if(!m_pRenderer)
		{
			IXRenderUtils *pUtils = (IXRenderUtils*)m_pCore->getPluginManager()->getInterface(IXRENDERUTILS_GUID);
			pUtils->newGizmoRenderer(&m_pRenderer);
		}

		if(is3D)
		{
			static const int *dev_bvh_render_inc = m_pCore->getConsole()->getPCVarInt("dev_bvh_render_level");

			m_pScene->drawLevel(*dev_bvh_render_inc);
		}

		m_pRenderer->render(!is3D, true);
	}

	UINT XMETHODCALLTYPE getResourceBrowserCount() override
	{
		return(0);
	}
	bool XMETHODCALLTYPE getResourceBrowser(UINT uId, IXEditorResourceBrowser **ppOut) override
	{
		return(false);
	}

	IXGizmoRenderer* getRenderer()
	{
		return(m_pRenderer);
	}

private:
	IXGizmoRenderer *m_pRenderer = NULL;
	IXCore *m_pCore = NULL;
	CScene *m_pScene = NULL;
};

class CEditable final: public IXUnknownImplementation<IXEditable>
{
public:
	CEditable(CScene *pScene, IXCore *pCore)
	{
		m_pEditorExt = new CEditorExt(pScene, pCore);
	}

	XIMPLEMENT_VERSION(IXEDITABLE_VERSION);

	~CEditable()
	{
		mem_release(m_pEditorExt);
	}

	UINT XMETHODCALLTYPE getObjectCount() override
	{
		return(0);
	}
	IXEditorObject* XMETHODCALLTYPE getObject(UINT id) override
	{
		return(NULL);
	}
	IXEditorObject* XMETHODCALLTYPE newObject(const char *szClassName) override
	{
		return(NULL);
	}

	const char* XMETHODCALLTYPE getName() override
	{
		return("Scene");
	}
	UINT XMETHODCALLTYPE getClassCount() override
	{
		return(0);
	}
	const char* XMETHODCALLTYPE getClass(UINT id) override
	{
		return(NULL);
	}

	void XMETHODCALLTYPE startup(IGXDevice *pDevice) override
	{}
	void XMETHODCALLTYPE shutdown() override
	{}

	IXEditorExtension* XMETHODCALLTYPE getEditorExtension() override
	{
		return(m_pEditorExt);
	}

	bool XMETHODCALLTYPE canProduceModel() override
	{
		return(false);
	}
	bool XMETHODCALLTYPE buildModelFromSelection(IXEditorModel **ppOut) override
	{
		return(false);
	}
	bool XMETHODCALLTYPE newModel(IXEditorModel **ppOut) override
	{
		return(false);
	}
	UINT XMETHODCALLTYPE getModelCount() override
	{
		return(0);
	}
	bool XMETHODCALLTYPE getModel(UINT id, IXEditorModel **ppOut) override
	{
		return(false);
	}

	bool XMETHODCALLTYPE canUseModel(const char *szClass) override
	{
		return(false);
	}

	IXGizmoRenderer* getRenderer()
	{
		return(m_pEditorExt->getRenderer());
	}

private:
	CEditorExt *m_pEditorExt = NULL;
};

//##########################################################################

CScene::CScene(IXCore *pCore):
	m_pCore(pCore)
{
	m_pDevBVHrenderInc = new CDevBVHrenderInc(pCore);
	m_pDevBVHrenderDec = new CDevBVHrenderDec(pCore);
	m_pDevBVHValidate = new CDevBVHValidate(this);
	m_pCvarListener = new CCvarListener(pCore, this);

	auto pConsole = m_pCore->getConsole();

	pConsole->registerCommand("dev_bvh_render_inc", m_pDevBVHrenderInc, "Увеличивает квар dev_bvh_render_level на 1");
	pConsole->registerCommand("dev_bvh_render_dec", m_pDevBVHrenderDec, "Уменьшает квар dev_bvh_render_level на 1");
	pConsole->registerCommand("dev_bvh_validate", m_pDevBVHValidate, "Осуществляет валидацию дерева сцены");
	pConsole->registerCVar("dev_bvh_render_level", -1, "", FCVAR_NOTIFY);

	m_pCore->getEventChannel<XEventCvarChanged>(EVENT_CVAR_CHANGED_GUID)->addListener(m_pCvarListener);

	m_pCore->getPluginManager()->registerInterface(IXEDITABLE_GUID, m_pEditable = new CEditable(this, m_pCore));

	m_pRootNode = newNode(NULL);
}
CScene::~CScene()
{
	deleteNode(m_pRootNode);

	auto pConsole = m_pCore->getConsole();
	pConsole->removeCommand("dev_bvh_render_inc");
	pConsole->removeCommand("dev_bvh_render_dec");
	pConsole->removeCommand("dev_bvh_validate");

	m_pCore->getEventChannel<XEventCvarChanged>(EVENT_CVAR_CHANGED_GUID)->removeListener(m_pCvarListener);
	mem_delete(m_pCvarListener);
	mem_delete(m_pDevBVHrenderInc);
	mem_delete(m_pDevBVHrenderDec);
	mem_delete(m_pDevBVHValidate);
}

IXSceneObjectType* XMETHODCALLTYPE CScene::registerObjectType(const char *szName)
{
	assert(strlen(szName) < AAS_MAXLEN);

	CSceneObjectType *pOut;

	const AssotiativeArray<AAString, CSceneObjectType*>::Node *pNode;
	if(m_mapTypes.KeyExists(szName, &pNode))
	{
		pOut = *pNode->Val;
	}
	else
	{
		if(m_mapFeatures.Size() >= sizeof(NodeType) * 8)
		{
			LibReport(REPORT_MSG_LEVEL_ERROR, "Scene object type limit exceeded");
			return(NULL);
		}

		pOut = new CSceneObjectType(this, m_mapTypes.Size());

		AAString sKey;
		sKey.setName(szName);
		m_mapTypes[sKey] = pOut;
	}

	pOut->AddRef();
	return(pOut);
}
IXSceneObjectType* XMETHODCALLTYPE CScene::getObjectType(const char *szName)
{
	assert(strlen(szName) < AAS_MAXLEN);

	const AssotiativeArray<AAString, CSceneObjectType*>::Node *pNode;
	if(m_mapTypes.KeyExists(szName, &pNode))
	{
		CSceneObjectType *pOut = *pNode->Val;
		pOut->AddRef();
		return(pOut);
	}

	return(NULL);
}

IXSceneFeature* XMETHODCALLTYPE CScene::registerObjectFeature(const char *szName)
{
	assert(strlen(szName) < AAS_MAXLEN);

	CSceneFeature *pOut;

	const AssotiativeArray<AAString, CSceneFeature*>::Node *pNode;
	if(m_mapFeatures.KeyExists(szName, &pNode))
	{
		pOut = *pNode->Val;
	}
	else
	{
		if(m_mapFeatures.Size() >= sizeof(NodeFeature) * 8)
		{
			LibReport(REPORT_MSG_LEVEL_ERROR, "Scene object feature limit exceeded");
			return(NULL);
		}
		pOut = new CSceneFeature(m_mapFeatures.Size());

		AAString sKey;
		sKey.setName(szName);
		m_mapFeatures[sKey] = pOut;
	}

	//pOut->AddRef();
	return(pOut);
}
IXSceneFeature* XMETHODCALLTYPE CScene::getObjectFeature(const char *szName)
{
	assert(strlen(szName) < AAS_MAXLEN);

	const AssotiativeArray<AAString, CSceneFeature*>::Node *pNode;
	if(m_mapFeatures.KeyExists(szName, &pNode))
	{
		CSceneFeature *pOut = *pNode->Val;
		//pOut->AddRef();
		return(pOut);
	}

	return(NULL);
}

IXSceneObject* CScene::newObject(const SMAABB &aabb, void *pUserData, NodeType bmType, IXSceneFeature **ppFeatures, UINT uLayer)
{
	NodeFeature bmFeatures = 0;
	while(ppFeatures && *ppFeatures)
	{
		bmFeatures |= ((CSceneFeature*)(*ppFeatures))->getFeature();
		++ppFeatures;
	}

	CSceneObject *pObject;
	{
		ScopedSpinLock lock(m_lockPoolObjects);
		pObject = m_poolObjects.Alloc(this, aabb, pUserData, bmType, bmFeatures, uLayer);
	}
	m_qUpdate.emplace({SMAABB(), pObject, 0, UpdateItem::ADD});
	return(pObject);
}

void CScene::enqueueObjectUpdate(CSceneObject* pObject, const SMAABB &aabb)
{
	m_qUpdate.emplace({aabb, pObject, 0, UpdateItem::UPDATE});
}
void CScene::enqueueObjectUpdateFeatures(CSceneObject* pObject, NodeFeature bmFeatures)
{
	m_qUpdate.emplace({SMAABB(), pObject, bmFeatures, UpdateItem::UPDATE_FEATURES});
}
void CScene::enqueueObjectUpdateLayer(CSceneObject* pObject, UINT bmLayer)
{
	m_qUpdate.emplace({SMAABB(), pObject, bmLayer, UpdateItem::UPDATE_LAYER});
}
void CScene::enqueueObjectDelete(CSceneObject* pObject)
{
	m_qUpdate.emplace({SMAABB(), pObject, 0, UpdateItem::REMOVE});
}

static UINT GetMaxDepth(CSceneNode *pNode)
{
	if(!pNode)
	{
		return(0);
	}

	UINT uCur = 0;
	for(UINT i = 0; i < BVH_CHILD_COUNT; ++i)
	{
		UINT uTmp = GetMaxDepth(pNode->getChild(i, false));
		if(uCur < uTmp)
		{
			uCur = uTmp;
		}
	}

	return(1 + uCur);
}

UINT XMETHODCALLTYPE CScene::getTreeHeight()
{
	if(!m_pRootNode)
	{
		return(0);
	}
	return(GetMaxDepth(m_pRootNode));
	/*
	Create a queue.
	Push root into the queue.
	height = 0
	Loop
		nodeCount = size of queue
        
			// If the number of nodes at this level is 0, return height
		if nodeCount is 0
			return Height;
		else
			increase Height

			// Remove nodes of this level and add nodes of 
			// next level
		while (nodeCount > 0)
			pop node from front
			push its children to queue
			decrease nodeCount
		   // At this point, queue has nodes of next level
	*/

	std::queue<CSceneNode*> q;
	q.push(m_pRootNode);
	UINT uHeight = 0;
	while(true)
	{
		UINT uNodeCount = (UINT)q.size();

		if(!uNodeCount)
		{
			return(uHeight);
		}
		
		++uHeight;
		while(uNodeCount)
		{
			CSceneNode* pNode = q.front();
			q.pop();
			if(pNode->getChild(true))
			{
				q.push(pNode->getChild(true));
			}
			if(pNode->getChild(false))
			{
				q.push(pNode->getChild(false));
			}
			--uNodeCount;
		}
	}

	//return(GetMaxDepth(m_pRootNode));
}

void CScene::addObject(CSceneObject *pObject)
{
	m_pRootNode->addObject(pObject);
}
void CScene::removeObject(CSceneObject *pObject)
{
	CSceneNode *pNode = pObject->getNode();
	pNode->removeObject(pObject);
}

static UINT CountChildren(CSceneNode *pNode)
{
	UINT uCount = 1;
	for(UINT i = 0; i < BVH_CHILD_COUNT; ++i)
	{
		CSceneNode *pChildNode = pNode->getChild(i, false);

		if(pChildNode)
		{
			uCount += CountChildren(pChildNode);
		}
	}
	return(uCount);
}

void CScene::drawLevel(int iLvl)
{
	if(!m_pRenderer)
	{
		m_pRenderer = m_pEditable->getRenderer();
		if(!m_pRenderer)
		{
			return;
		}
	}
	m_pRenderer->reset();
	m_pRenderer->setLineWidth(2.0f);
	m_pRenderer->setColor(float4(1.0, 0.7f, 0.0f, 1.0f));

	if(iLvl >= 0)
	{
		drawLevelInternal(m_pRootNode, NULL, iLvl);
	}
}

void CScene::drawLevelInternal(CSceneNode *pNode, CSceneNode *pParent, int iLvl, int iCurLvl)
{
	if(!pNode)
	{
		return;
	}

	/*if(iLvl == iCurLvl)
	{
		m_pRenderer->setColor(float4(pParent ? (((UINT)pParent >> 4 & 0xf) << 4) / 255.0f : 1.0f, 0.7f, (((UINT)pParent >> 8 & 0xf) << 4) / 255.0f, 1.0f));

		m_pRenderer->drawAABB(pNode->getAABB());
	}
	else*/
	{
		if(iLvl == iCurLvl)
		{
			m_pRenderer->setPointSize(10.0f);
			m_pRenderer->setColor(float3(0.0f, 0.0f, 1.0f));
			m_pRenderer->drawPoint(pNode->m_vSplit);
			m_pRenderer->drawAABB(pNode->getAABB());

			m_pRenderer->setLineWidth(1.0f);
			fora(i, pNode->m_aObjects)
			{
				m_pRenderer->setColor(float3(1.0f, 0.0f, 0.0f));

				m_pRenderer->drawAABB(pNode->m_aObjects[i]->getAABB());
			}

			m_pRenderer->setLineWidth(2.0f);
		}
		for(UINT i = 0; i < BVH_CHILD_COUNT; ++i)
		{
			if(iLvl == iCurLvl && pNode->getChild(i, false))
			{
				m_pRenderer->setColor(float4(1.0f, 0.5f, CountChildren(pNode->getChild(i, false)) == 1 ? 0.5f : 0.0f, 1.0f));

				m_pRenderer->drawAABB(pNode->getChild(i, false)->getAABB());
			}

			drawLevelInternal(pNode->getChild(i, false), pNode, iLvl, iCurLvl + 1);
		}
	}
}

CSceneNode* CScene::newNode(CSceneNode *pParent)
{
	return(m_poolNodes.Alloc(this, pParent));
}
void CScene::deleteNode(CSceneNode *pNode)
{
	if(pNode)
	{
		m_poolNodes.Delete(pNode);
	}
}

void CScene::sync()
{
	UpdateItem item;
	SMAABB aabbOld;
	while(m_qUpdate.pop(&item))
	{
		switch(item.action)
		{
		case UpdateItem::ADD:
			addObject(item.pObject);
			break;

		case UpdateItem::REMOVE:
			{
				ScopedSpinLock lock(m_lockPoolObjects);
				m_poolObjects.Delete(item.pObject);
			}
			break;

		case UpdateItem::UPDATE:
			aabbOld = item.pObject->m_aabb;
			item.pObject->m_aabb = item.aabbNew;
			if(item.pObject->m_pNode)
			{
				item.pObject->m_pNode->updateObject(item.pObject, aabbOld);
			}
			break;

		case UpdateItem::UPDATE_FEATURES:
			item.pObject->m_bmFeatures = item.bmFeatures;
			if(item.pObject->m_pNode)
			{
				item.pObject->m_pNode->updateFeatures();
			}
			break;

		case UpdateItem::UPDATE_LAYER:
			item.pObject->m_bmLayer = item.bmLayer;
			if(item.pObject->m_pNode)
			{
				item.pObject->m_pNode->updateFeatures();
			}
			break;
		}
	}
}

bool CScene::validate()
{
	return(m_pRootNode->validate());
}

void CScene::print()
{
	UINT uNodesCount = 0;
	UINT uObjectsCount = 0;
	UINT uMaxDepth = 0;
	m_pRootNode->print(0, &uNodesCount, &uObjectsCount, &uMaxDepth);
	printf("Total objects: %u, Total nodes: %u, Max depth: %u, avg objects per node: %f\n", uObjectsCount, uNodesCount, uMaxDepth, (float)uObjectsCount / (float)uNodesCount);
}
