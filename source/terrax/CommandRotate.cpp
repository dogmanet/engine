#include "CommandRotate.h"
#include "UndoManager.h"

CCommandRotate::CCommandRotate(bool bClone)
{
	if(bClone)
	{
		extern CUndoManager *g_pUndoManager;
		m_pDuplicateCommand = new CCommandDuplicate();
		g_pUndoManager->execCommand(m_pDuplicateCommand, false);
	}
}

CCommandRotate::~CCommandRotate()
{
	mem_release(m_pDuplicateCommand);
	for(UINT i = 0, l = m_aObjects.size(); i < l; ++i)
	{
		mem_release(m_aObjects[i].pObj);
	}
}

bool XMETHODCALLTYPE CCommandRotate::exec()
{
	if(m_pDuplicateCommand && !m_pDuplicateCommand->exec())
	{
		return(false);
	}
	_rot_obj *pObj;
	bool moved = false;
	for(UINT i = 0, l = m_aObjects.size(); i < l; ++i)
	{
		pObj = &m_aObjects[i];
		pObj->pObj->setPos(pObj->vEndPos);
		pObj->pObj->setOrient(pObj->vEndOrient);
		moved = moved || memcmp(&pObj->vEndPos, &pObj->vStartPos, sizeof(pObj->vStartPos)) || memcmp(&pObj->vEndOrient, &pObj->vStartOrient, sizeof(pObj->vStartOrient));
	}
	XUpdatePropWindow();
	return(moved);
}
bool XMETHODCALLTYPE CCommandRotate::undo()
{
	_rot_obj *pObj;
	for(UINT i = 0, l = m_aObjects.size(); i < l; ++i)
	{
		pObj = &m_aObjects[i];
		pObj->pObj->setPos(pObj->vStartPos);
		pObj->pObj->setOrient(pObj->vStartOrient);
	}
	if(m_pDuplicateCommand && !m_pDuplicateCommand->undo())
	{
		return(false);
	}
	XUpdatePropWindow();
	return(true);
}

void CCommandRotate::addObject(IXEditorObject *pObj)
{
	add_ref(pObj);
	float3_t vPos = pObj->getPos();
	SMQuaternion qOrient = pObj->getOrient();
	m_aObjects.push_back({pObj, vPos, vPos, qOrient, qOrient});
}
void CCommandRotate::setStartOrigin(const float3 &vOrigin, const float3 &vAxis)
{
	m_vOrigin = vOrigin;
	m_vAxis = vAxis;
}
void CCommandRotate::setStartPos(const float3 &vPos)
{
	m_vStartPos = vPos;
}

void CCommandRotate::setCurrentPos(const float3 &vPos, bool useSnap)
{
	float3 v1 = SMVector3Normalize(m_vStartPos - m_vOrigin);
	float3 v2 = SMVector3Normalize(vPos - m_vOrigin);
	SMQuaternion q;
	if(v1 == -v2)
	{
		q = SMQuaternion(m_vAxis, SM_PI);
	}
	else
	{
		q = SMQuaternion(v1, v2);
	}


	_rot_obj *pObj;
	for(UINT i = 0, l = m_aObjects.size(); i < l; ++i)
	{
		pObj = &m_aObjects[i];
		pObj->vEndPos = (float3)(m_vOrigin + q * (pObj->vStartPos - m_vOrigin));

		pObj->vEndOrient = pObj->vStartOrient * q;

		pObj->pObj->setPos(pObj->vEndPos);
		pObj->pObj->setOrient(pObj->vEndOrient);
	}
}
