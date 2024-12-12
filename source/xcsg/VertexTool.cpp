#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <Commdlg.h>

#include "EditorExtension.h"
#include "resource.h"
//#include "EditorObject.h"
#include "Editable.h"
//#include "CommandCreate.h"

#include <xcommon/IPluginManager.h>

#include "VertexTool.h"

#include "EditorObject.h"

#include "CommandMorph.h"

extern HINSTANCE g_hInstance;

// box selection of vertices

//##########################################################################

CVertexTool::CVertexTool(CEditable *pEditable, IXEditor *pEditor):
	m_pEditable(pEditable),
	m_pEditor(pEditor),
	m_gizmoCallback(this)
{
	m_hBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_AB_VERTEX));

	IXRenderUtils *pUtils = (IXRenderUtils*)pEditable->getCore()->getPluginManager()->getInterface(IXRENDERUTILS_GUID);
	pUtils->newGizmoRenderer(&m_pRenderer);

	//m_pFaceEdit = new CFaceEdit(pEditable, pEditor, g_hInstance, (HWND)pEditor->getMainWindow());
	//m_pFaceEdit->initGraphics(SGCore_GetDXDevice());
}
CVertexTool::~CVertexTool()
{
	DeleteBitmap(m_hBitmap);
	//mem_release(m_pCurrentCommand);

	mem_release(m_pRenderer);
}

void* XMETHODCALLTYPE CVertexTool::getIcon()
{
	return(m_hBitmap);
}
const char* XMETHODCALLTYPE CVertexTool::getTitle()
{
	return("Vertex tool");
}
XAccelItem XMETHODCALLTYPE CVertexTool::getAccel()
{
	XAccelItem xaf = {XAF_SHIFT | XAF_VIRTKEY, 'V'};
	return(xaf);
}

void XMETHODCALLTYPE CVertexTool::activate()
{
	m_isActive = true;
	//m_pFaceEdit->activate();

	const Array<CEditorObject*> &aObjects = m_pEditable->getObjects();

	CEditorObject *pObj;
	for(UINT i = 0, l = aObjects.size(); i < l; ++i)
	{
		pObj = aObjects[i];
		if(pObj->isSelected() && pObj->isVisible())
		{
			addObject(pObj);
		}
	}
}
void XMETHODCALLTYPE CVertexTool::deactivate()
{
	m_isActive = false;

	fora(i, m_aObjects)
	{
		ObjData &od = m_aObjects[i];
		fora(j, od.aHandles)
		{
			mem_release(od.aHandles[j].pHandle);
		}
	}

	m_aObjects.clearFast();
}

bool XMETHODCALLTYPE CVertexTool::onMouseDown(bool isPrimary)
{
	if(m_pEditor->getViewForWindow(m_pEditor->getState()->activeWindow) == X2D_NONE)
	{
		return(false);
	}

	if(isPrimary)
	{
		if(!m_pEditor->isKeyPressed(SIK_CONTROL))
		{
			enumHandles([](UINT, UINT, const ObjData&, HandleData &hd){
				hd.select(false);
			});
		}

		m_pEditor->beginFrameSelect();
		return(true);
	}

	return(false);
}
bool XMETHODCALLTYPE CVertexTool::onMouseMove()
{
	return(false);
}
void XMETHODCALLTYPE CVertexTool::onMouseUp(bool isPrimary)
{
	float2_t vSelectStart, vSelectEnd;
	X_2D_VIEW xCurView;
	if(isPrimary && m_pEditor->endFrameSelect(&xCurView, &vSelectStart, &vSelectEnd))
	{
		enumHandles([&](UINT, UINT, const ObjData&, HandleData &hd){
			if(m_pEditor->isPointInFrame(hd.pHandle->getPos(), vSelectStart, vSelectEnd, xCurView))
			{
				hd.select(true);
			}
		});
	}
}

bool XMETHODCALLTYPE CVertexTool::onKeyDown(UINT uKey)
{
	return(false);
}
void XMETHODCALLTYPE CVertexTool::onKeyUp(UINT uKey)
{
}

void CVertexTool::onSelectionChanged(CEditorObject *pObj)
{
	if(m_isActive)
	{
		if(pObj->isSelected() && pObj->isVisible())
		{
			addObject(pObj);
		}
		else
		{
			removeObject(pObj);
		}
	}
}

void CVertexTool::onObjectTopologyChanged(CEditorObject *pObj)
{
	if(m_isActive && pObj->isSelected() && pObj->isVisible())
	{
		addObject(pObj);
	}
}

void CVertexTool::setSelectedVerticesForObject(CEditorObject *pObj, const Array<UINT> &aSelectedVertices)
{
	if(m_isActive)
	{
		int idx = m_aObjects.indexOf(pObj, [](const ObjData &a, CEditorObject *b){
			return(a.pObj == b);
		});
		assert(idx >= 0);
		if(idx >= 0)
		{
			ObjData &od = m_aObjects[idx];

			fora(i, aSelectedVertices)
			{
				UINT vtx = aSelectedVertices[i];
				assert(vtx < od.aHandles.size());
				if(vtx < od.aHandles.size())
				{
					od.aHandles[vtx].select(true);
				}
			}
		}
	}
}

void CVertexTool::render(bool is3D)
{
	if(m_isActive)
	{
		//if(m_isInitiated)
		//{
		//	//if(m_isDirty)
		//	{
		//		m_pRenderer->reset();

		//		m_pRenderer->setLineWidth(is3D ? 0.02f : 2.0f);
		//		m_pRenderer->setColor(m_isValid ? float4(0.0f, 1.0f, 1.0f, 1.0f) : float4(1.0f, 0.0f, 0.0f, 1.0f));
		//		m_pRenderer->jumpTo(m_pGizmos[0]->getPos());
		//		m_pRenderer->lineTo(m_pGizmos[1]->getPos());
		//		m_pRenderer->lineTo(m_pGizmos[2]->getPos());
		//		m_pRenderer->lineTo(m_pGizmos[0]->getPos());

		//		m_pRenderer->setColor(m_isValid ? float4(0.0f, 1.0f, 1.0f, 0.2f) : float4(1.0f, 0.0f, 0.0f, 0.2f));
		//		m_pRenderer->drawPoly(m_pGizmos[0]->getPos(), m_pGizmos[1]->getPos(), m_pGizmos[2]->getPos());

		//		m_isDirty = false;
		//	}

		//	m_pRenderer->render(!is3D, !is3D);
		//}
	}
}

void XMETHODCALLTYPE CVertexTool::onNextMode()
{
	//int aNextMode[] = {
	//	0, 1, -1
	//};

	//m_iMode = aNextMode[m_iMode + 1];
}

void CVertexTool::addObject(CEditorObject *pObj)
{
	removeObject(pObj);

	UINT uVertices = pObj->getVertexCount();

	ObjData &od = m_aObjects[m_aObjects.size()];
	od.pObj = pObj;
	od.aHandles.resize(uVertices);
	for(UINT i = 0; i < uVertices; ++i)
	{
		m_pEditor->newGizmoHandle(&od.aHandles[i].pHandle);
		od.aHandles[i].pHandle->setPos(pObj->getVertexAt(i));
		od.aHandles[i].pHandle->setCallback(&m_gizmoCallback);
	}
}

void CVertexTool::removeObject(CEditorObject *pObj)
{
	int idx = m_aObjects.indexOf(pObj, [](const ObjData &a, CEditorObject *b){
		return(a.pObj == b);
	});
	if(idx >= 0)
	{
		ObjData &od = m_aObjects[idx];
		fora(i, od.aHandles)
		{
			mem_release(od.aHandles[i].pHandle);
		}
		m_aObjects.erase(idx);
	}
}

void CVertexTool::onHandleMoved(const float3 &vNewPos, IXEditorGizmoHandle *pHandle)
{
	UINT uMaxVertices = 0, uCurVertex;
	fora(i, m_aObjects)
	{
		ObjData &od = m_aObjects[i];
		uCurVertex = od.aHandles.size();
		if(uCurVertex > uMaxVertices)
		{
			uMaxVertices = uCurVertex;
		}
	}
	assert(uMaxVertices);

	UINT *puAffectedVertices = (UINT*)alloca(sizeof(UINT) * uMaxVertices);

	float3 vDeltaPos = vNewPos - pHandle->getPos();

	bool bCaptureState = false;
	if(!m_pCommand)
	{
		m_pCommand = new CCommandMorph(m_pEditor, m_pEditable);
		bCaptureState = true;
	}

	fora(i, m_aObjects)
	{
		ObjData &od = m_aObjects[i];
		uCurVertex = 0;
		fora(j, od.aHandles)
		{
			HandleData &hd = od.aHandles[j];

			if(hd.isSelected)
			{
				puAffectedVertices[uCurVertex++] = j;
			}
		}

		if(uCurVertex)
		{
			if(bCaptureState)
			{
				m_pCommand->addObject(od.pObj, puAffectedVertices, uCurVertex);
			}

			UINT uRemovedVertexCount = od.pObj->moveVertices(puAffectedVertices, uCurVertex, vDeltaPos);

			for(int iVtx = (int)uRemovedVertexCount - 1; iVtx >= 0; --iVtx)
			{
				mem_release(od.aHandles[puAffectedVertices[iVtx]].pHandle);
				od.aHandles.erase(puAffectedVertices[iVtx]);
			}

			fora(j, od.aHandles)
			{
				HandleData &hd = od.aHandles[j];

				if(hd.isSelected)
				{
					hd.pHandle->setPos(od.pObj->getVertexAt(j));
				}
			}
		}
	}
}
void CVertexTool::onHandleClick(IXEditorGizmoHandle *pHandle)
{
	int idHandle = -1;
	int idObject = m_aObjects.indexOf(pHandle, [&idHandle](const ObjData &od, IXEditorGizmoHandle *h){
		idHandle = od.aHandles.indexOf(h, [](const HandleData &hd, IXEditorGizmoHandle *h){
			return(hd.pHandle == h);
		});

		return(idHandle >= 0);
	});
	assert(idObject >= 0);
	assert(idHandle >= 0);

	X_2D_VIEW x2dView = m_pEditor->getViewForWindow(m_pEditor->getState()->activeWindow);

	//if (3d and has selection in handle's position) or (2d and has selection in projected handle's position)
	{
		if(m_aObjects[idObject].aHandles[idHandle].isSelected)
		{
			return;
		}

		bool isSelected = false;

		float2 v2Pos1;	
		const float3_t &vPos1 = pHandle->getPos();

		UINT idSelectedObject;
		UINT idSelectedHandle;

		switch(x2dView)
		{
		case X2D_NONE:
			enumHandles([pHandle, &isSelected, &idSelectedObject, &idSelectedHandle](UINT idObject, UINT idHandle, const ObjData &od, HandleData &hd){
				if(hd.isSelected && SMIsZero(SMVector3Length2(hd.pHandle->getPos() - pHandle->getPos())))
				{
					idSelectedObject = idObject;
					idSelectedHandle = idHandle;
					isSelected = true;
				}
			});
			break;

		case X2D_TOP: // x/z
			v2Pos1 = float2(vPos1.x, vPos1.z);
			enumHandles([pHandle, &isSelected, &v2Pos1, &idSelectedObject, &idSelectedHandle](UINT idObject, UINT idHandle, const ObjData &od, HandleData &hd){
				const float3_t &vPos0 = hd.pHandle->getPos();
				if(hd.isSelected && SMIsZero(SMVector2Length(float2(vPos0.x, vPos0.z) - v2Pos1)))
				{
					idSelectedObject = idObject;
					idSelectedHandle = idHandle;
					isSelected = true;
				}
			});
			break;

		case X2D_FRONT: // x/y
			v2Pos1 = float2(vPos1.x, vPos1.y);
			enumHandles([pHandle, &isSelected, &v2Pos1, &idSelectedObject, &idSelectedHandle](UINT idObject, UINT idHandle, const ObjData &od, HandleData &hd){
				const float3_t &vPos0 = hd.pHandle->getPos();
				if(hd.isSelected && SMIsZero(SMVector2Length(float2(vPos0.x, vPos0.y) - v2Pos1)))
				{
					idSelectedObject = idObject;
					idSelectedHandle = idHandle;
					isSelected = true;
				}
			});
			break;

		case X2D_SIDE: // z/y
			v2Pos1 = float2(vPos1.z, vPos1.y);
			enumHandles([pHandle, &isSelected, &v2Pos1, &idSelectedObject, &idSelectedHandle](UINT idObject, UINT idHandle, const ObjData &od, HandleData &hd){
				const float3_t &vPos0 = hd.pHandle->getPos();
				if(hd.isSelected && SMIsZero(SMVector2Length(float2(vPos0.z, vPos0.y) - v2Pos1)))
				{
					idSelectedObject = idObject;
					idSelectedHandle = idHandle;
					isSelected = true;
				}
			});
			break;
		}

		if(isSelected)
		{
			// swap two handles

			IXEditorGizmoHandle *pHandle0 = m_aObjects[idObject].aHandles[idHandle].pHandle;
			IXEditorGizmoHandle *pHandle1 = m_aObjects[idSelectedObject].aHandles[idSelectedHandle].pHandle;

			float3 vPos = pHandle0->getPos();
			pHandle0->setPos(pHandle1->getPos());
			pHandle1->setPos(vPos);

			std::swap(m_aObjects[idObject].aHandles[idHandle].pHandle, m_aObjects[idSelectedObject].aHandles[idSelectedHandle].pHandle);
			m_aObjects[idObject].aHandles[idHandle].select(false);
			m_aObjects[idSelectedObject].aHandles[idSelectedHandle].select(true);
			return;
		}
	}

	bool isCtrlPressed = m_pEditor->isKeyPressed(SIK_CONTROL);

	if(!isCtrlPressed)
	{
		enumHandles([](UINT, UINT, const ObjData&, HandleData &hd){
			hd.select(false);
		});
	}

	ObjData &od = m_aObjects[idObject];
	float2 v2Pos1;
	const float3_t &vPos1 = pHandle->getPos();
	switch(x2dView)
	{
	case X2D_NONE:
		fora(j, od.aHandles)
		{
			HandleData &hd = od.aHandles[j];
			if(SMIsZero(SMVector3Length2(hd.pHandle->getPos() - pHandle->getPos())))
			{
				hd.select(true);
			}
		}
		break;

	case X2D_TOP: // x/z
		v2Pos1 = float2(vPos1.x, vPos1.z);
		fora(j, od.aHandles)
		{
			HandleData &hd = od.aHandles[j];
			const float3_t &vPos0 = hd.pHandle->getPos();
			if(SMIsZero(SMVector2Length(float2(vPos0.x, vPos0.z) - v2Pos1)))
			{
				hd.select(true);
			}
		}
		break;

	case X2D_FRONT: // x/y
		v2Pos1 = float2(vPos1.x, vPos1.y);
		fora(j, od.aHandles)
		{
			HandleData &hd = od.aHandles[j];
			const float3_t &vPos0 = hd.pHandle->getPos();
			if(SMIsZero(SMVector2Length(float2(vPos0.x, vPos0.y) - v2Pos1)))
			{
				hd.select(true);
			}
		}
		break;

	case X2D_SIDE: // z/y
		v2Pos1 = float2(vPos1.z, vPos1.y);
		fora(j, od.aHandles)
		{
			HandleData &hd = od.aHandles[j];
			const float3_t &vPos0 = hd.pHandle->getPos();
			if(SMIsZero(SMVector2Length(float2(vPos0.z, vPos0.y) - v2Pos1)))
			{
				hd.select(true);
			}
		}
		break;
	}
}

void CVertexTool::onHandleReleased()
{
	if(m_pCommand)
	{
		UINT uMaxVertices = 0, uCurVertex;
		fora(i, m_aObjects)
		{
			ObjData &od = m_aObjects[i];
			uCurVertex = od.aHandles.size();
			if(uCurVertex > uMaxVertices)
			{
				uMaxVertices = uCurVertex;
			}
		}
		assert(uMaxVertices);

		UINT *puAffectedVertices = (UINT*)alloca(sizeof(UINT) * uMaxVertices);
		
		fora(i, m_aObjects)
		{
			ObjData &od = m_aObjects[i];
			uCurVertex = 0;
			fora(j, od.aHandles)
			{
				HandleData &hd = od.aHandles[j];

				if(hd.isSelected)
				{
					puAffectedVertices[uCurVertex++] = j;
				}
			}

			if(uCurVertex)
			{
				m_pCommand->finalizeObject(od.pObj, puAffectedVertices, uCurVertex);
			}
		}

		m_pEditor->execCommand(m_pCommand);
		m_pCommand = NULL;
	}
}

//##########################################################################

void XMETHODCALLTYPE CVertexGizmoCallback::moveTo(const float3 &vNewPos, IXEditorGizmoHandle *pGizmo)
{
	if(m_isFirstMov)
	{
		m_isFirstMov = false;
	}
	else
	{
		m_isMoved = true;
		m_pTool->onHandleMoved(vNewPos, pGizmo);
	}
}
void XMETHODCALLTYPE CVertexGizmoCallback::onStart(IXEditorGizmoHandle *pGizmo)
{
	m_pTool->onHandleClick(pGizmo);
	m_isMoved = false;
	m_isFirstMov = true;
}
void XMETHODCALLTYPE CVertexGizmoCallback::onEnd(IXEditorGizmoHandle *pGizmo)
{
	m_pTool->onHandleReleased();
	if(!m_isMoved)
	{
		//m_pTool->onHandleClick(pGizmo);
	}
}