#include "CommandMove.h"
#include "UndoManager.h"

CCommandMove::CCommandMove(bool bClone)
{
	if(bClone)
	{
		extern CUndoManager *g_pUndoManager;
		m_pDuplicateCommand = new CCommandDuplicate();
		g_pUndoManager->execCommand(m_pDuplicateCommand, false);
	}
}

CCommandMove::~CCommandMove()
{
	mem_release(m_pDuplicateCommand);
	for(UINT i = 0, l = m_aObjects.size(); i < l; ++i)
	{
		mem_release(m_aObjects[i].pObj);
	}
}

bool XMETHODCALLTYPE CCommandMove::exec()
{
	if(m_pDuplicateCommand && !m_pDuplicateCommand->exec())
	{
		return(false);
	}
	_move_obj *pObj;
	bool moved = false;
	for(UINT i = 0, l = m_aObjects.size(); i < l; ++i)
	{
		pObj = &m_aObjects[i];
		pObj->pObj->setPos(pObj->vEndPos);
		moved = moved || memcmp(&pObj->vEndPos, &pObj->vStartPos, sizeof(pObj->vStartPos));
	}
	XUpdatePropWindow();
	return(moved);
}
bool XMETHODCALLTYPE CCommandMove::undo()
{
	_move_obj *pObj;
	for(UINT i = 0, l = m_aObjects.size(); i < l; ++i)
	{
		pObj = &m_aObjects[i];
		pObj->pObj->setPos(pObj->vStartPos);
	}
	if(m_pDuplicateCommand && !m_pDuplicateCommand->undo())
	{
		return(false);
	}
	XUpdatePropWindow();
	return(true);
}

const char *CCommandMove::getText()
{
	return("move");
}

void CCommandMove::addObject(IXEditorObject *pObj)
{
	add_ref(pObj);
	float3_t vPos = pObj->getPos();
	m_aObjects.push_back({pObj, vPos, vPos});
}
void CCommandMove::setStartPos(const float3 &vPos)
{
	m_vStartPos = vPos;
}

void CCommandMove::setCurrentPos(const float3 &vPos)
{
	float3 vDelta = vPos - m_vStartPos;
	_move_obj *pObj;
	for(UINT i = 0, l = m_aObjects.size(); i < l; ++i)
	{
		pObj = &m_aObjects[i];
		pObj->vEndPos = (float3)(pObj->vStartPos + vDelta);
		pObj->pObj->setPos(pObj->vEndPos);
	}
	//XUpdatePropWindow();
}

void CCommandMove::setReferenceBound(const float3_t &vBoundMin, const float3_t &vBoundMax)
{
	m_vRefBoundMin = vBoundMin;
	m_vRefBoundMax = vBoundMax;
}
void CCommandMove::getReferenceBound(float3_t *pvBoundMin, float3_t *pvBoundMax)
{
	*pvBoundMin = m_vRefBoundMin;
	*pvBoundMax = m_vRefBoundMax;
}
