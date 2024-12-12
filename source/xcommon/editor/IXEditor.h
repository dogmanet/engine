#ifndef __XEDITOR_H
#define __XEDITOR_H

#include <gdefines.h>
#include "IXEditorGizmo.h"
#include "IXEditorMaterialBrowser.h"

enum X_WINDOW_POS
{
	XWP_TOP_LEFT,
	XWP_TOP_RIGHT,
	XWP_BOTTOM_LEFT,
	XWP_BOTTOM_RIGHT
};

struct TerraXState
{
	X_WINDOW_POS activeWindow = XWP_TOP_LEFT;
	float2 vWinSize;
	float2_t vMousePos;
	float2_t vWorldMousePos;
	float3_t vResolvedWorldMousePos;

	float3 vWorldRayStart;
	float3 vWorldRayDir;

	float3 vBestPlaneNormal;
};

enum X_2D_VIEW
{
	X2D_NONE = -1, // 3d view
	X2D_TOP,   // x/z
	X2D_FRONT, // x/y
	X2D_SIDE   // z/y
};

//##########################################################################

// {84ECF1FC-4C03-4EB9-BC39-D991B83F73BA}
#define IXEDITOR_GUID DEFINE_XGUID(0x84ecf1fc, 0x4c03, 0x4eb9, 0xbc, 0x39, 0xd9, 0x91, 0xb8, 0x3f, 0x73, 0xba)
#define IXEDITOR_VERSION 1

class IXCamera;
class IXEditorObject;
class IXEditorCommand;
class IXEditor: public IXUnknown
{
public:
	virtual void XMETHODCALLTYPE getCameraForView(X_WINDOW_POS winPos, IXCamera **ppCamera) = 0;

	virtual void XMETHODCALLTYPE newGizmoHandle(IXEditorGizmoHandle **ppOut) = 0;
	virtual void XMETHODCALLTYPE newGizmoRadius(IXEditorGizmoRadius **ppOut) = 0;
	virtual void XMETHODCALLTYPE newGizmoMove(IXEditorGizmoMove **ppOut) = 0;
	virtual void XMETHODCALLTYPE newGizmoRotate(IXEditorGizmoRotate **ppOut) = 0;
	virtual void XMETHODCALLTYPE newGizmoScale(IXEditorGizmoScale **ppOut) = 0;

	virtual const TerraXState* XMETHODCALLTYPE getState() = 0;

	virtual X_2D_VIEW XMETHODCALLTYPE getViewForWindow(X_WINDOW_POS winPos) = 0;
	virtual float XMETHODCALLTYPE getViewScale(X_WINDOW_POS winPos) = 0;
	virtual bool XMETHODCALLTYPE getGridSnapState() = 0;
	virtual float XMETHODCALLTYPE getGridStep() = 0;

	//! Available only in undo/redo context!
	virtual void XMETHODCALLTYPE addObject(IXEditorObject *pObject) = 0;
	virtual void XMETHODCALLTYPE removeObject(IXEditorObject *pObject) = 0;

	virtual bool XMETHODCALLTYPE execCommand(IXEditorCommand *pCmd) = 0;

	virtual IXEditorMaterialBrowser* XMETHODCALLTYPE getMaterialBrowser() = 0;

	virtual bool XMETHODCALLTYPE isKeyPressed(UINT uKey) = 0;

	// HWND for now
	virtual void* XMETHODCALLTYPE getMainWindow() = 0;

	virtual void XMETHODCALLTYPE disableCurrentTool() = 0;

	virtual void XMETHODCALLTYPE beginFrameSelect() = 0;
	virtual bool XMETHODCALLTYPE endFrameSelect(X_2D_VIEW *pxCurView, float2_t *pvStartPos, float2_t *pvEndPos) = 0;
	virtual bool XMETHODCALLTYPE isPointInFrame(const float3 &vPos, const float2_t &vFrameStart, const float2_t &vFrameEnd, X_2D_VIEW xCurView) = 0;

	virtual void XMETHODCALLTYPE editMaterial(const char *szMatName) = 0;
};


#endif
