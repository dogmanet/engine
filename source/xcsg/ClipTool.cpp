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

#include "CommandClip.h"

extern HINSTANCE g_hInstance;

//##########################################################################

CClipTool::CClipTool(CEditable *pEditable, IXEditor *pEditor):
	m_pEditable(pEditable),
	m_pEditor(pEditor),
	m_gizmoCallback(this)
{
	m_hBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_AB_CLIP));

	IXRenderUtils *pUtils = (IXRenderUtils*)pEditable->getCore()->getPluginManager()->getInterface(IXRENDERUTILS_GUID);
	pUtils->newGizmoRenderer(&m_pRenderer);

	for(UINT i = 0; i < 3; ++i)
	{
		pEditor->newGizmoHandle(&m_pGizmos[i]);
		m_pGizmos[i]->setCallback(&m_gizmoCallback);
		m_pGizmos[i]->enable(false);
	}

	//m_pFaceEdit = new CFaceEdit(pEditable, pEditor, g_hInstance, (HWND)pEditor->getMainWindow());
	//m_pFaceEdit->initGraphics(SGCore_GetDXDevice());
}
CClipTool::~CClipTool()
{
	//mem_delete(m_pFaceEdit);

	for(UINT i = 0; i < 3; ++i)
	{
		mem_release(m_pGizmos[i]);
	}

	DeleteBitmap(m_hBitmap);
	//mem_release(m_pCurrentCommand);

	mem_release(m_pRenderer);
}

void* XMETHODCALLTYPE CClipTool::getIcon()
{
	return(m_hBitmap);
}
const char* XMETHODCALLTYPE CClipTool::getTitle()
{
	return("Clipping tool");
}
XAccelItem XMETHODCALLTYPE CClipTool::getAccel()
{
	XAccelItem xaf = {XAF_SHIFT | XAF_VIRTKEY, 'X'};
	return(xaf);
}

void XMETHODCALLTYPE CClipTool::activate()
{
	m_isActive = true;
	//m_pFaceEdit->activate();

	
	//for(UINT i = 0; i < 3; ++i)
	//{
	//	m_pGizmos[i]->enable(true);
	//}
}
void XMETHODCALLTYPE CClipTool::deactivate()
{
	m_isActive = false;
	//m_pFaceEdit->deactivate();
	m_isInitiated = false;
	for(UINT i = 0; i < 3; ++i)
	{
		m_pGizmos[i]->enable(false);
	}
	m_pEditable->setClipPlane(CPS_NONE);
}




bool XMETHODCALLTYPE CClipTool::onMouseDown(bool isPrimary)
{
	if(m_pEditor->getViewForWindow(m_pEditor->getState()->activeWindow) == X2D_NONE || m_isInitiated)
	{
		return(false);
	}


	if(isPrimary)
	{
		m_isInitiated = true;
		for(UINT i = 0; i < 3; ++i)
		{
			m_pGizmos[i]->enable(true);
		}

		m_pGizmos[0]->setPos(m_pEditor->getState()->vResolvedWorldMousePos);
		m_pGizmos[1]->setPos((float3)(m_pEditor->getState()->vResolvedWorldMousePos + m_pEditor->getState()->vWorldRayDir));
		m_pGizmos[2]->setPos(m_pEditor->getState()->vResolvedWorldMousePos);

		m_isMouseDown = true;

		onPosChanged();
		return(true);
	}

	// m_pEditor->getState()->vResolvedWorldMousePos
	//m_pFaceEdit->onMouseDown(isPrimary, m_pEditor->getState()->vWorldRayStart, m_pEditor->getState()->vWorldRayDir);

	return(false);
}
bool XMETHODCALLTYPE CClipTool::onMouseMove()
{
	if(m_isMouseDown)
	{
		m_pGizmos[2]->setPos(m_pEditor->getState()->vResolvedWorldMousePos);
		onPosChanged();
		return(true);
	}
	return(false);
}
void XMETHODCALLTYPE CClipTool::onMouseUp(bool isPrimary)
{
	if(isPrimary)
	{
		m_isMouseDown = false;
	}
}

bool XMETHODCALLTYPE CClipTool::onKeyDown(UINT uKey)
{
	if(m_isInitiated)
	{
		if(uKey == SIK_ESCAPE)
		{
			m_isInitiated = false;
			for(UINT i = 0; i < 3; ++i)
			{
				m_pGizmos[i]->enable(false);
			}
			m_pEditable->setClipPlane(CPS_NONE);
			return(true);
		}
		else if(uKey == SIK_ENTER || uKey == SIK_NUMPADENTER)
		{
			// actually split
			m_pEditor->execCommand(new CCommandClip(m_pEditor, m_pEditable));

			m_isInitiated = false;
			for(UINT i = 0; i < 3; ++i)
			{
				m_pGizmos[i]->enable(false);
			}
			m_pEditable->setClipPlane(CPS_NONE);
			return(true);
		}
	}
	return(false);
}
void XMETHODCALLTYPE CClipTool::onKeyUp(UINT uKey)
{
}

void CClipTool::render(bool is3D)
{
	if(m_isActive)
	{
		if(m_isInitiated)
		{
			//if(m_isDirty)
			{
				m_pRenderer->reset();

				m_pRenderer->setLineWidth(is3D ? 0.02f : 2.0f);
				m_pRenderer->setColor(m_isValid ? float4(0.0f, 1.0f, 1.0f, 1.0f) : float4(1.0f, 0.0f, 0.0f, 1.0f));
				m_pRenderer->jumpTo(m_pGizmos[0]->getPos());
				m_pRenderer->lineTo(m_pGizmos[1]->getPos());
				m_pRenderer->lineTo(m_pGizmos[2]->getPos());
				m_pRenderer->lineTo(m_pGizmos[0]->getPos());

				m_pRenderer->setColor(m_isValid ? float4(0.0f, 1.0f, 1.0f, 0.2f) : float4(1.0f, 0.0f, 0.0f, 0.2f));
				m_pRenderer->drawPoly(m_pGizmos[0]->getPos(), m_pGizmos[1]->getPos(), m_pGizmos[2]->getPos());

				m_isDirty = false;
			}

			m_pRenderer->render(!is3D, !is3D);
		}
		//SAFE_CALL(m_pNewOutline, render, is3D);
		if(is3D)
		{
		//	m_pRenderer->reset();
		//	m_pFaceEdit->render(m_pRenderer);
		//	m_pRenderer->render(false, false);
		}
	}
}

void CClipTool::onPosChanged()
{
	if(!m_isInitiated)
	{
		return;
	}
	m_isDirty = true;

	m_isValid = !SMIsZero(1.0f - fabsf(SMVector3Dot(
		SMVector3Normalize(m_pGizmos[0]->getPos() - m_pGizmos[1]->getPos()),
		SMVector3Normalize(m_pGizmos[0]->getPos() - m_pGizmos[2]->getPos())
		))) 
		&& !SMIsZero(SMVector3Length2(m_pGizmos[0]->getPos() - m_pGizmos[1]->getPos()))
		&& !SMIsZero(SMVector3Length2(m_pGizmos[0]->getPos() - m_pGizmos[2]->getPos()))
		&& !SMIsZero(SMVector3Length2(m_pGizmos[1]->getPos() - m_pGizmos[2]->getPos()));

	if(m_isValid)
	{
		m_clipPlane = SMPlaneNormalize(SMPLANE(m_pGizmos[0]->getPos(), m_pGizmos[1]->getPos(), m_pGizmos[2]->getPos()));
		m_pEditable->setClipPlane(m_iMode ? CPS_ONESIDE : CPS_TWOSIDE, m_iMode >= 0 ? m_clipPlane : -m_clipPlane);
	}
	else
	{
		m_pEditable->setClipPlane(CPS_NONE);
	}
}

void XMETHODCALLTYPE CClipTool::onNextMode()
{
	int aNextMode[] = {
		0, 1, -1
	};

	m_iMode = aNextMode[m_iMode + 1];

	onPosChanged();
}

//##########################################################################

void XMETHODCALLTYPE CGizmoCallback::moveTo(const float3 &vNewPos, IXEditorGizmoHandle *pGizmo)
{
	pGizmo->setPos(vNewPos);
	m_pTool->onPosChanged();
}
