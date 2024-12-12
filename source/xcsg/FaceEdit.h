#ifndef __FACE_EDIT_H
#define __FACE_EDIT_H

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <gdefines.h>
#include <graphix/graphix.h>
#include <mtrl/IXMaterialSystem.h>
#include <xcommon/editor/IXEditor.h>
#include "BrushMesh.h"

class CEditable;
class CEditorObject;

enum FACE_EDIT_MODE
{
	FEM_PICK = 1,
	FEM_SELECT = 2,
	FEM_PICK_SELECT = FEM_PICK + FEM_SELECT,
	FEM_APPLY_MATERIAL = 4,
	FEM_APPLY_VALUES = 8,
	FEM_APPLY_MATERIAL_VALUES = FEM_APPLY_MATERIAL + FEM_APPLY_VALUES,
	FEM_ALIGN_TO_VIEW = 16
};

enum VALID_VALUES
{
	VV_NONE = 0,
	VV_SCALE_S = 1,
	VV_SCALE_T = 2,
	VV_SHIFT_S = 4,
	VV_SHIFT_T = 8,
	VV_ANGLE = 16,
	VV_MATERIAL = 32,

	VV_ALL = 0xFF
};
XDEFINE_ENUM_FLAG_OPERATORS(VALID_VALUES);

struct FaceDesc
{
	CEditorObject *pObject;
	UINT uFace;
};

//#############################################################################

class CFaceEdit;
class CMaterialBrowserCallback: public IXEditorMaterialBrowserCallback
{
public:
	CMaterialBrowserCallback(CFaceEdit *pEdit):
		m_pEdit(pEdit)
	{
	}
	void XMETHODCALLTYPE onSelected(const char *szName) override;
	void XMETHODCALLTYPE onCancel() override
	{
	}

private:
	CFaceEdit *m_pEdit;
};

//#############################################################################

class IXGizmoRenderer;
class CCommandFaceEdit;
class CFaceEdit final
{
public:
	CFaceEdit(CEditable *pEditable, IXEditor *pEditor, HINSTANCE hInstance, HWND hMainWnd);
	~CFaceEdit();

	void activate();
	void deactivate();

	void render(IXGizmoRenderer *pRenderer);
	//void swapBuffers();

	//void update(float dt);

	void onMouseDown(bool isPrimary, const float3 &vRayStart, const float3 &vRayDir);

	void setCurrentMaterial(const char *szMat);

private:
	HINSTANCE m_hInstance;
	HWND m_hMainWnd;
	HWND m_hDlgWnd = NULL;
	HWND m_hViewportWnd = NULL;
	CEditable *m_pEditable;
	IXEditorMaterialBrowser *m_pBrowser = NULL;

	IXEditor *m_pEditor;

	FACE_EDIT_MODE m_editMode = FEM_PICK_SELECT;

	//IGXIndexBuffer *m_pFrameIB = NULL;
	//IGXRenderBuffer *m_pFrameRB = NULL;

	Array<FaceDesc> m_aSelectedFaces;

	bool m_isMaskHidden = false;

	CMaterialBrowserCallback m_matBrowserCallback;

	BrushFace m_currentSettings;
	float m_fCurrentAngle = 0.0f;

	String m_sCurrentMat;

	bool m_bSkipChanges = false;

	
	VALID_VALUES m_validValues = VV_ALL;

private:
	INT_PTR dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool getWorldFace(const float3 &vRayStart, const float3 &vRayDir, FaceDesc *pOut);

	void clearSelection();
	void initSelection();

	void toggleFaceSelection(const FaceDesc &fd);

	void syncRecentMaterials();

	void pickFace(const FaceDesc &fd);
	void assignMaterial(const FaceDesc &fd, bool useValues, CCommandFaceEdit *pCmd = NULL);

	void syncUI();

	void applyValues();

	void alignFace();
	void alignWorld();

	void justify(int how, bool isAllAsOne);

	void validateStates();

	void alignFaceToView(const FaceDesc &fd);

	bool isFaceSelected(const FaceDesc &fd);
};

#endif
