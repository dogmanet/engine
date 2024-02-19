#ifndef __EDITOR_H
#define __EDITOR_H

#include <xcommon/editor/IXEditor.h>
#include <xcommon/editor/IXEditorExtension.h>
#include <xcommon/IXCore.h>
//#include "terrax.h"
#include "GizmoHandle.h"
#include "GizmoRadius.h"
#include "GizmoMove.h"
#include "GizmoRotate.h"
#include "GizmoScale.h"
#include "EditorMaterialBrowser.h"
#include "CurveEditorDialog.h"

#define GIZMO_TYPES() \
	GTO(Handle)\
	GTO(Radius)\
	GTO(Move)\
	GTO(Rotate)\
	GTO(Scale)

class CEditor: public IXUnknownImplementation<IXEditor>
{
public:
	CEditor(IXCore *pCore);
	~CEditor();

	void XMETHODCALLTYPE getCameraForView(X_WINDOW_POS winPos, IXCamera **ppCamera) override;

#define GTO(gt) \
	void XMETHODCALLTYPE newGizmo##gt(IXEditorGizmo##gt **ppOut) override;\
	void onGizmoRemoved(CGizmo##gt *pGizmo);
	GIZMO_TYPES();
#undef GTO

	void render(bool is3D);

	void setDirty()
	{
		m_isDirty = true;
	}

	void onMouseMove();
	bool onMouseDown();
	void onMouseUp();

	const TerraXState* XMETHODCALLTYPE getState() override;

	X_2D_VIEW XMETHODCALLTYPE getViewForWindow(X_WINDOW_POS winPos) override;
	float XMETHODCALLTYPE getViewScale(X_WINDOW_POS winPos) override;
	bool XMETHODCALLTYPE getGridSnapState() override;
	float XMETHODCALLTYPE getGridStep() override;

	void XMETHODCALLTYPE addObject(IXEditorObject *pObject) override;
	void XMETHODCALLTYPE removeObject(IXEditorObject *pObject) override;

	bool XMETHODCALLTYPE execCommand(IXEditorCommand *pCmd) override;

	void* XMETHODCALLTYPE getMainWindow() override;

	void XMETHODCALLTYPE disableCurrentTool() override;

	IXEditorMaterialBrowser* XMETHODCALLTYPE getMaterialBrowser() override
	{
		return(&m_matBrowser);
	}

	bool XMETHODCALLTYPE isKeyPressed(UINT uKey) override;

	void XMETHODCALLTYPE beginFrameSelect() override;
	bool XMETHODCALLTYPE endFrameSelect(X_2D_VIEW *pxCurView, float2_t *pvStartPos, float2_t *pvEndPos) override;
	bool XMETHODCALLTYPE isPointInFrame(const float3 &vPos, const float2_t &vFrameStart, const float2_t &vFrameEnd, X_2D_VIEW xCurView) override;

	void registerResourceBrowser(IXEditorResourceBrowser *pResourceBrowser);
	bool getResourceBrowserForType(const char *szType, IXEditorResourceBrowser **ppResourceBrowser);

	void XMETHODCALLTYPE editMaterial(const char *szMatName) override;

private:
#define GTO(gt) Array<CGizmo##gt*> m_aGizmos##gt; CGizmo##gt *m_pSelected##gt = NULL;
	GIZMO_TYPES();
#undef GTO

	IXGizmoRenderer *m_pGizmoRendererBoth = NULL;
	IXGizmoRenderer *m_pGizmoRenderer2D = NULL;
	IXGizmoRenderer *m_pGizmoRenderer3D = NULL;

	bool m_isCapturing = false;

	bool m_isDirty = false;

	float3_t m_vOldCamPos;

	CEditorMaterialBrowser m_matBrowser;

	CCurveEditorDialog m_curveEditor;

	Map<AAString, IXEditorResourceBrowser*> m_mapResourceBrowsers;
};

#endif
