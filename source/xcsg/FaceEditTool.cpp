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

extern HINSTANCE g_hInstance;

//##########################################################################

CFaceEditTool::CFaceEditTool(CEditable *pEditable, IXEditor *pEditor):
	m_pEditable(pEditable),
	m_pEditor(pEditor)
{
	m_hBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_AB_FACE_EDIT));

	IXRenderUtils *pUtils = (IXRenderUtils*)pEditable->getCore()->getPluginManager()->getInterface(IXRENDERUTILS_GUID);
	pUtils->newGizmoRenderer(&m_pRenderer);

	m_pFaceEdit = new CFaceEdit(pEditable, pEditor, g_hInstance, (HWND)pEditor->getMainWindow());
}
CFaceEditTool::~CFaceEditTool()
{
	mem_delete(m_pFaceEdit);

	DeleteBitmap(m_hBitmap);
	//mem_release(m_pCurrentCommand);

	mem_release(m_pRenderer);
}

void* XMETHODCALLTYPE CFaceEditTool::getIcon()
{
	return(m_hBitmap);
}
const char* XMETHODCALLTYPE CFaceEditTool::getTitle()
{
	return("Face edit tool");
}
XAccelItem XMETHODCALLTYPE CFaceEditTool::getAccel()
{
	XAccelItem xaf = {XAF_SHIFT | XAF_VIRTKEY, 'A'};
	return(xaf);
}

void XMETHODCALLTYPE CFaceEditTool::activate()
{
	m_isActive = true;
	m_pFaceEdit->activate();
}
void XMETHODCALLTYPE CFaceEditTool::deactivate()
{
	m_isActive = false;
	m_pFaceEdit->deactivate();
}




bool XMETHODCALLTYPE CFaceEditTool::onMouseDown(bool isPrimary)
{
	if(m_pEditor->getViewForWindow(m_pEditor->getState()->activeWindow) != X2D_NONE)
	{
		return(false);
	}

	m_pFaceEdit->onMouseDown(isPrimary, m_pEditor->getState()->vWorldRayStart, m_pEditor->getState()->vWorldRayDir);

	return(true);
}
bool XMETHODCALLTYPE CFaceEditTool::onMouseMove()
{
	return(false);
}
void XMETHODCALLTYPE CFaceEditTool::onMouseUp(bool isPrimary)
{
}

bool XMETHODCALLTYPE CFaceEditTool::onKeyDown(UINT uKey)
{
	return(false);
}
void XMETHODCALLTYPE CFaceEditTool::onKeyUp(UINT uKey)
{
}

void CFaceEditTool::render(bool is3D)
{
	if(m_isActive)
	{
		//SAFE_CALL(m_pNewOutline, render, is3D);
		if(is3D)
		{
			m_pRenderer->reset();
			m_pFaceEdit->render(m_pRenderer);
			m_pRenderer->render(false, false);
		}
	}
}
