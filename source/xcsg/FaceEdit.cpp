#include "FaceEdit.h"
#include "resource.h"

#include <windowsx.h>
#include <commctrl.h>
#include <xcommon/IXCamera.h>

#include "Editable.h"
#include "EditorObject.h"
//#include "terrax.h"

#include "CommandFaceEdit.h"

CFaceEdit::CFaceEdit(CEditable *pEditable, IXEditor *pEditor, HINSTANCE hInstance, HWND hMainWnd):
	m_hInstance(hInstance),
	m_hMainWnd(hMainWnd),
	m_pEditable(pEditable),
	m_matBrowserCallback(this),
	m_pEditor(pEditor)
{
	memset(&m_currentSettings, 0, sizeof(m_currentSettings));

	m_currentSettings.fSScale = 1.0f;
	m_currentSettings.fTScale = 1.0f;

	m_pBrowser = pEditor->getMaterialBrowser();

	CreateDialogParamA(m_hInstance, MAKEINTRESOURCE(IDD_FACE_EDIT), hMainWnd, DlgProc, (LPARAM)this);

	setCurrentMaterial(m_pBrowser->getCurrentMaterial());
}

CFaceEdit::~CFaceEdit()
{
	DestroyWindow(m_hDlgWnd);
}

INT_PTR CALLBACK CFaceEdit::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CFaceEdit *pThis = (CFaceEdit*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lParam);
		pThis = (CFaceEdit*)lParam;
		pThis->m_hDlgWnd = hWnd;
	}
	if(pThis)
	{
		return(pThis->dlgProc(hWnd, msg, wParam, lParam));
	}
	return(FALSE);
}

void GetChildRect(HWND hWnd, LPRECT lpRect)
{
	HWND hParentWnd = GetParent(hWnd);
	GetWindowRect(hWnd, lpRect);
	ScreenToClient(hParentWnd, &(((LPPOINT)lpRect)[0]));
	ScreenToClient(hParentWnd, &(((LPPOINT)lpRect)[1]));
}

INT_PTR CFaceEdit::dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			m_hViewportWnd = GetDlgItem(hWnd, IDC_CUR_MAT_VP);

			HWND hCombo = GetDlgItem(hWnd, IDC_MODE);
			ComboBox_AddString(hCombo, "Pick+Select");
			ComboBox_AddString(hCombo, "Pick");
			ComboBox_AddString(hCombo, "Select");
			ComboBox_AddString(hCombo, "Apply (material)");
			ComboBox_AddString(hCombo, "Apply (all)");
			ComboBox_AddString(hCombo, "Align to View");
			ComboBox_SetCurSel(hCombo, 0);
			ComboBox_SetItemData(hCombo, 0, FEM_PICK_SELECT);
			ComboBox_SetItemData(hCombo, 1, FEM_PICK);
			ComboBox_SetItemData(hCombo, 2, FEM_SELECT);
			ComboBox_SetItemData(hCombo, 3, FEM_APPLY_MATERIAL);
			ComboBox_SetItemData(hCombo, 4, FEM_APPLY_MATERIAL_VALUES);
			ComboBox_SetItemData(hCombo, 5, FEM_ALIGN_TO_VIEW);

			SendMessage(GetDlgItem(m_hDlgWnd, IDC_ROTATION_SPIN), UDM_SETRANGE, 0, MAKELPARAM(359, 0));

			EnableWindow(GetDlgItem(m_hDlgWnd, IDC_REPLACE), FALSE);

			//SendMessage(GetDlgItem(m_hDlgWnd, IDC_SHIFT_S_SPIN), UDM_SETRANGE, 0, MAKELPARAM(1, 0));
			

			//Button_SetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_OPAQUE), BST_CHECKED);
			//Button_SetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_TRANSPARENT), BST_CHECKED);
			//Button_SetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_SELFILLUM), BST_CHECKED);
			//Button_SetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_MATERIALS), BST_CHECKED);
			//Button_SetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_TEXTURES), BST_CHECKED);
			//Button_SetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_TRANSLATED), BST_CHECKED);
			//Button_SetCheck(GetDlgItem(m_hDlgWnd, IDC_MAT_RAWFILES), BST_CHECKED);

			syncUI();

			break;
		}

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_MODE:
			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				HWND hCombo = (HWND)lParam;
				int iSel = ComboBox_GetCurSel(hCombo);
				if(iSel >= 0)
				{
					int uMode = (int)ComboBox_GetItemData(hCombo, iSel);
					if(uMode)
					{
						m_editMode = (FACE_EDIT_MODE)uMode;
					}
				}
			}
			break;

		case IDC_JUSTIFY_T:
		case IDC_JUSTIFY_B:
		case IDC_JUSTIFY_C:
		case IDC_JUSTIFY_R:
		case IDC_JUSTIFY_L:
		case IDC_JUSTIFY_FIT:
			justify(LOWORD(wParam), IsDlgButtonChecked(m_hDlgWnd, IDC_JUSTIFY_TAO) == BST_CHECKED);
			break;

		case IDC_ALIGN_WORLD:
			alignWorld();
			break;

		case IDC_ALIGN_FACE:
			alignFace();
			break;

		case IDC_CUR_MAT:
			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				HWND hCombo = (HWND)lParam;
				int iSel = ComboBox_GetCurSel(hCombo);
				if(iSel >= 0)
				{
					int iLen = ComboBox_GetLBTextLen(hCombo, iSel);
					char *szMatName = (char*)alloca(sizeof(char)* (iLen + 1));
					ComboBox_GetLBText(hCombo, iSel, szMatName);

					setCurrentMaterial(szMatName);
				}
			}
			break;

		case IDC_BROWSE:
			m_pBrowser->browse(&m_matBrowserCallback);
			break;

		case IDC_REPLACE:
			break;

		case IDC_MAT_EDIT:
			m_pEditor->editMaterial(m_sCurrentMat.c_str());
			break;

		case IDC_APPLY:
			{
				CCommandFaceEdit *pCmd = new CCommandFaceEdit();
				for(UINT i = 0, l = m_aSelectedFaces.size(); i < l; ++i)
				{
					assignMaterial(m_aSelectedFaces[i], true, pCmd);
				}
				m_pEditor->execCommand(pCmd);
			}
			break;

		case IDC_HIDE_MASK:
			m_isMaskHidden = Button_GetCheck((HWND)lParam) == BST_CHECKED;
			break;

		case IDC_SCALE_S:
		case IDC_SCALE_T:
		case IDC_SHIFT_S:
		case IDC_SHIFT_T:
		case IDC_ROTATION:
			if(HIWORD(wParam) == EN_CHANGE && !m_bSkipChanges)
			{
				HWND hEdit = (HWND)lParam;
				int iLen = GetWindowTextLength(hEdit);
				char *szText = (char*)alloca(sizeof(char)* (iLen + 1));
				GetWindowText(hEdit, szText, iLen + 1);
				float fVal = 1.0f;
				sscanf(szText, "%f", &fVal);

				switch(LOWORD(wParam))
				{
				case IDC_SCALE_S:
					m_currentSettings.fSScale = fVal;
					m_validValues |= VV_SCALE_S;
					break;
				case IDC_SCALE_T:
					m_currentSettings.fTScale = fVal;
					m_validValues |= VV_SCALE_T;
					break;
				case IDC_SHIFT_S:
					m_currentSettings.fSShift = fVal;
					m_validValues |= VV_SHIFT_S;
					break;
				case IDC_SHIFT_T:
					m_currentSettings.fTShift = fVal;
					m_validValues |= VV_SHIFT_T;
					break;
				case IDC_ROTATION:
					m_fCurrentAngle = fVal;
					m_validValues |= VV_ANGLE;
					break;
				}

				applyValues();

			}
			break;
		}
		break;

	case WM_CLOSE:
		// ShowWindow(m_hDlgWnd, SW_HIDE);
		m_pEditor->disableCurrentTool();
		break;

	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->code)
		{
		case UDN_DELTAPOS:
			LPNMUPDOWN pUpDown = (LPNMUPDOWN)lParam;

			if(pUpDown->hdr.idFrom != IDC_ROTATION_SPIN)
			{
				HWND hTextEdit = (HWND)SendMessage(pUpDown->hdr.hwndFrom, UDM_GETBUDDY, 0, 0);
				float fVal;
				int iDecimals = 2;
				int iDelta = -pUpDown->iDelta;
				switch(pUpDown->hdr.idFrom)
				{
				case IDC_SCALE_S_SPIN:
					fVal = m_currentSettings.fSScale += (float)iDelta * 0.1f;
					break;
				case IDC_SCALE_T_SPIN:
					fVal = m_currentSettings.fTScale += (float)iDelta * 0.1f;
					break;
				case IDC_SHIFT_S_SPIN:
					m_currentSettings.fSShift += (float)iDelta * 0.005f;
					m_currentSettings.fSShift = fmodf(m_currentSettings.fSShift, 1.0f);
					fVal = m_currentSettings.fSShift;
					iDecimals = 3;
					break;
				case IDC_SHIFT_T_SPIN:
					m_currentSettings.fTShift += (float)iDelta * 0.005f;
					m_currentSettings.fTShift = fmodf(m_currentSettings.fTShift, 1.0f);
					fVal = m_currentSettings.fTShift;
					iDecimals = 3;
					break;
				}

				char tmp[64];
				sprintf(tmp, "%.*f", iDecimals, fVal);
				m_bSkipChanges = true;
				SetWindowText(hTextEdit, tmp);
				m_bSkipChanges = false;

				applyValues();
			}

			//int iDelta = pUpDown->iDelta;
			// pUpDown->hdr.hwndFrom
			//IDC_ROTATION_SPIN
			//printf("%d\n", iDelta);
			break;
		}
		break;

	/*case WM_MOUSEWHEEL:
		{
			m_iScrollPos = (float)m_iScrollPos - (float)GET_WHEEL_DELTA_WPARAM(wParam) * 32.0f / (float)WHEEL_DELTA;

			if(m_iScrollPos < 0)
			{
				m_iScrollPos = 0;
			}
			else if(m_iScrollPos > m_iScrollHeight)
			{
				m_iScrollPos = m_iScrollHeight;
			}
			m_pScrollBar->setScrollPos(m_iScrollPos);

			preload();
			m_isDirty = true;
		}
		break;*/

	default:
		return(FALSE);
	}
	return(TRUE);
}

void CFaceEdit::activate()
{
	ShowWindow(m_hDlgWnd, SW_SHOW);
	//SetFocus(m_hDlgWnd);
	initSelection();
	//syncRecentMaterials();
	setCurrentMaterial(m_pBrowser->getCurrentMaterial());
}
void CFaceEdit::deactivate()
{
	ShowWindow(m_hDlgWnd, SW_HIDE);
	clearSelection();
}

void CFaceEdit::onMouseDown(bool isPrimary, const float3 &vRayStart, const float3 &vRayDir)
{
	FaceDesc fd;
	if(!getWorldFace(vRayStart, vRayDir, &fd))
	{
		return;
	}

	if(!m_pEditor->isKeyPressed(SIK_CONTROL))
	{
		clearSelection();
	}

	if(m_editMode & FEM_SELECT)
	{
		toggleFaceSelection(fd);
	}

	if(m_editMode & FEM_PICK)
	{
		pickFace(fd);

		if(m_editMode & FEM_SELECT)
		{
			validateStates();
		}
	}

	if(m_editMode & FEM_APPLY_MATERIAL)
	{
		assignMaterial(fd, (m_editMode & FEM_APPLY_VALUES) != 0);
	}

	if(m_editMode & FEM_ALIGN_TO_VIEW)
	{
		 alignFaceToView(fd);
	}
}

bool CFaceEdit::getWorldFace(const float3 &vRayStart, const float3 &vRayDir, FaceDesc *pOut)
{
	const Array<CEditorObject*> &aObjects = m_pEditable->getObjects();

	static Array<CEditorObject*> s_aPotentialSet;
	s_aPotentialSet.clearFast();

	CEditorObject *pObj;
	float3 vMin, vMax;
	for(UINT i = 0, l = aObjects.size(); i < l; ++i)
	{
		pObj = aObjects[i];
		if(pObj->isVisible())
		{
			pObj->getBound(&vMin, &vMax);
			if(SMRayIntersectAABB(SMAABB(vMin, vMax), vRayStart, vRayDir))
			{
				s_aPotentialSet.push_back(pObj);
			}
		}
	}

	if(!s_aPotentialSet.size())
	{
		return(false);
	}

	struct FoundFace
	{
		FaceDesc fd;
		float fDist2;
	};
	static Array<FoundFace> s_aFoundSet;
	s_aFoundSet.clearFast();

	UINT uFace;
	float3 vFacePoint;
	for(UINT i = 0, l = s_aPotentialSet.size(); i < l; ++i)
	{
		pObj = s_aPotentialSet[i];

		if(pObj->findFace(vRayStart, vRayDir, &uFace, &vFacePoint))
		{
			s_aFoundSet.push_back({{pObj, uFace}, SMVector3Length2(vRayStart - vFacePoint)});
		}
	}

	if(!s_aFoundSet.size())
	{
		return(false);
	}

	s_aFoundSet.quickSort([](const FoundFace &a, const FoundFace &b){
		return(a.fDist2 < b.fDist2);
	});

	if(pOut)
	{
		*pOut = s_aFoundSet[0].fd;
	}

	return(true);
}

void CFaceEdit::clearSelection()
{
	m_aSelectedFaces.clearFast();
}

void CFaceEdit::initSelection()
{
	const Array<CEditorObject*> &aObjects = m_pEditable->getObjects();

	CEditorObject *pObj;
	for(UINT i = 0, l = aObjects.size(); i < l; ++i)
	{
		pObj = aObjects[i];
		if(pObj->isSelected() && pObj->isVisible())
		{
			UINT uFaceCount = pObj->getFaceCount();

			m_aSelectedFaces.reserve(m_aSelectedFaces.size() + uFaceCount);

			for(UINT j = 0; j < uFaceCount; ++j)
			{
				m_aSelectedFaces.push_back({pObj, j});
			}
		}
	}

	validateStates();
}

void CFaceEdit::render(IXGizmoRenderer *pRenderer)
{
	if(m_isMaskHidden)
	{
		return;
	}
	pRenderer->setColor(float4(1.0f, 0.0f, 0.0f, 0.15f));

	for(UINT i = 0, l = m_aSelectedFaces.size(); i < l; ++i)
	{
		FaceDesc &fd = m_aSelectedFaces[i];
		fd.pObject->renderFace(pRenderer, fd.uFace);
	}
}

void CFaceEdit::toggleFaceSelection(const FaceDesc &newFD)
{
	for(UINT i = 0, l = m_aSelectedFaces.size(); i < l; ++i)
	{
		FaceDesc &fd = m_aSelectedFaces[i];
		if(!memcmp(&fd, &newFD, sizeof(FaceDesc)))
		{
			m_aSelectedFaces.erase(i);
			return;
		}
	}

	m_aSelectedFaces.push_back(newFD);

	//validateStates();
}

void CFaceEdit::setCurrentMaterial(const char *szMat)
{
	m_sCurrentMat = szMat;
	m_pBrowser->setCurrentMaterial(szMat);

	IXMaterial *pMat = NULL;
	IXTexture *pTex = NULL;

	m_pBrowser->getCurrentMaterialInfo(&pMat, &pTex);

	if(pTex)
	{
		char tmp[64];
		sprintf(tmp, "%ux%u", pTex->getWidth(), pTex->getHeight());
		SetDlgItemText(m_hDlgWnd, IDC_TEX_SIZE, tmp);
	}

	IKeyIterator *pIter = NULL;
	const char *szTexture = pMat->getTextureName("txBase");
	if(!szTexture)
	{
		pIter = pMat->getTexturesIterator();
		if(pIter)
		{
			szTexture = pIter->getCurrent();
		}
	}
	SetWindowTextW(m_hViewportWnd, szTexture ? CMB2WC(szTexture) : L"");
	mem_release(pIter);

	mem_release(pMat);
	mem_release(pTex);

	syncRecentMaterials();
}

void CFaceEdit::syncRecentMaterials()
{
	HWND hCombo = GetDlgItem(m_hDlgWnd, IDC_CUR_MAT);
	ComboBox_ResetContent(hCombo);
	for(UINT i = 0, l = m_pBrowser->getRecentMaterialCount(); i < l; ++i)
	{
		ComboBox_AddString(hCombo, m_pBrowser->getRecentMaterial(i));
	}
	ComboBox_SetCurSel(hCombo, 0);
}

static float CalcAngle(const BrushFace &bf)
{
	float3 vNearestAxis = CBrushMesh::GetNearestAxis(bf.vN);

	float3 vRotationAxis = SMVector3Cross(bf.vT, bf.vS);

	float3 vBaseTangent = SMVector3Normalize(CBrushMesh::ProjectToFace(vRotationAxis, CBrushMesh::GetTangent(vNearestAxis)));

	float fAngle = safe_acosf(SMVector3Dot(bf.vT, vBaseTangent));

	if(SMVector3Dot(SMVector3Normalize(SMVector3Cross(bf.vT, vBaseTangent)), vRotationAxis) < 0.0f)
	{
		fAngle = SM_2PI - fAngle;
	}

	fAngle = SMToAngle(fAngle);
	if(fAngle < 0.0f)
	{
		fAngle += 360.0f;
	}

	return(fAngle);
}

void CFaceEdit::pickFace(const FaceDesc &fd)
{
	m_validValues = VV_ALL;
	fd.pObject->getFaceInfo(fd.uFace, &m_currentSettings);
	setCurrentMaterial(m_currentSettings.szMaterial);

	m_fCurrentAngle = CalcAngle(m_currentSettings);

	syncUI();
}

void CFaceEdit::syncUI()
{
	char tmp[64];

	m_bSkipChanges = true;
	if(m_validValues & VV_SCALE_S)
	{
		sprintf(tmp, "%.2f", m_currentSettings.fSScale);
		SetDlgItemText(m_hDlgWnd, IDC_SCALE_S, tmp);
	}
	else
	{
		SetDlgItemText(m_hDlgWnd, IDC_SCALE_S, "");
	}

	if(m_validValues & VV_SCALE_T)
	{
		sprintf(tmp, "%.2f", m_currentSettings.fTScale);
		SetDlgItemText(m_hDlgWnd, IDC_SCALE_T, tmp);
	}
	else
	{
		SetDlgItemText(m_hDlgWnd, IDC_SCALE_T, "");
	}

	if(m_validValues & VV_SHIFT_S)
	{
		sprintf(tmp, "%.3f", m_currentSettings.fSShift);
		SetDlgItemText(m_hDlgWnd, IDC_SHIFT_S, tmp);
	}
	else
	{
		SetDlgItemText(m_hDlgWnd, IDC_SHIFT_S, "");
	}

	if(m_validValues & VV_SHIFT_T)
	{
		sprintf(tmp, "%.3f", m_currentSettings.fTShift);
		SetDlgItemText(m_hDlgWnd, IDC_SHIFT_T, tmp);
	}
	else
	{
		SetDlgItemText(m_hDlgWnd, IDC_SHIFT_T, "");
	}

	if(m_validValues & VV_ANGLE)
	{
		sprintf(tmp, "%.0f", m_fCurrentAngle);
		SetDlgItemText(m_hDlgWnd, IDC_ROTATION, tmp);
	}
	else
	{
		SetDlgItemText(m_hDlgWnd, IDC_ROTATION, "");
	}	

	bool isFaceAligned = SMIsZero(SMVector3Dot(m_currentSettings.vN, m_currentSettings.vS)) && 
		SMIsZero(SMVector3Dot(m_currentSettings.vN, m_currentSettings.vT)) && 
		SMIsZero(SMVector3Dot(m_currentSettings.vS, m_currentSettings.vT));

	float3 vNearestAxis = CBrushMesh::GetNearestAxis(m_currentSettings.vN);

	bool isWorldAligned = SMIsZero(SMVector3Dot(vNearestAxis, m_currentSettings.vS)) &&
		SMIsZero(SMVector3Dot(vNearestAxis, m_currentSettings.vT)) &&
		SMIsZero(SMVector3Dot(m_currentSettings.vS, m_currentSettings.vT));

	CheckDlgButton(m_hDlgWnd, IDC_ALIGN_FACE, isFaceAligned ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hDlgWnd, IDC_ALIGN_WORLD, isWorldAligned ? BST_CHECKED : BST_UNCHECKED);

	m_bSkipChanges = false;
}

void CFaceEdit::assignMaterial(const FaceDesc &fd, bool useValues, CCommandFaceEdit *pCmd_)
{
	BrushFace oldFace;
	fd.pObject->getFaceInfo(fd.uFace, &oldFace);

	CCommandFaceEdit *pCmd = pCmd_;
	if(!pCmd)
	{
		pCmd = new CCommandFaceEdit();
	}

	if(useValues)
	{
		if(m_validValues & VV_SCALE_S)
		{
			oldFace.fSScale = m_currentSettings.fSScale;
		}
		if(m_validValues & VV_SCALE_T)
		{
			oldFace.fTScale = m_currentSettings.fTScale;
		}
		if(m_validValues & VV_SHIFT_S)
		{
			oldFace.fSShift = m_currentSettings.fSShift;
		}
		if(m_validValues & VV_SHIFT_T)
		{
			oldFace.fTShift = m_currentSettings.fTShift;
		}

		if(m_validValues & VV_ANGLE)
		{
			float3 vNearestAxis = CBrushMesh::GetNearestAxis(oldFace.vN);
			float3 vRotationAxis = SMVector3Cross(oldFace.vT, oldFace.vS);
			float3 vBaseTangent = SMVector3Normalize(CBrushMesh::ProjectToFace(vRotationAxis, CBrushMesh::GetTangent(vNearestAxis)));

			SMQuaternion q(vRotationAxis, SMToRadian(m_fCurrentAngle));
			oldFace.vT = SMVector3Normalize(q * vBaseTangent);
			oldFace.vS = SMVector3Normalize(SMVector3Cross(vRotationAxis, oldFace.vT));
		}
	}

	if(m_validValues & VV_MATERIAL)
	{
		oldFace.szMaterial = m_sCurrentMat.c_str();
	}

	pCmd->addFace(fd, oldFace);

	if(!pCmd_)
	{
		m_pEditor->execCommand(pCmd);
	}
}

void CFaceEdit::applyValues()
{
	BrushFace oldFace;

	CCommandFaceEdit *pCmd = new CCommandFaceEdit();

	for(UINT i = 0, l = m_aSelectedFaces.size(); i < l; ++i)
	{
		FaceDesc &fd = m_aSelectedFaces[i];

		fd.pObject->getFaceInfo(fd.uFace, &oldFace);

		if(m_validValues & VV_SCALE_S)
		{
			oldFace.fSScale = m_currentSettings.fSScale;
		}
		if(m_validValues & VV_SCALE_T)
		{
			oldFace.fTScale = m_currentSettings.fTScale;
		}
		if(m_validValues & VV_SHIFT_S)
		{
			oldFace.fSShift = m_currentSettings.fSShift;
		}
		if(m_validValues & VV_SHIFT_T)
		{
			oldFace.fTShift = m_currentSettings.fTShift;
		}

		if(m_validValues & VV_ANGLE)
		{
			float3 vNearestAxis = CBrushMesh::GetNearestAxis(oldFace.vN);
			float3 vRotationAxis = SMVector3Cross(oldFace.vT, oldFace.vS);
			float3 vBaseTangent = SMVector3Normalize(CBrushMesh::ProjectToFace(vRotationAxis, CBrushMesh::GetTangent(vNearestAxis)));

			SMQuaternion q(vRotationAxis, SMToRadian(m_fCurrentAngle));
			oldFace.vT = SMVector3Normalize(q * vBaseTangent);
			oldFace.vS = SMVector3Normalize(SMVector3Cross(vRotationAxis, oldFace.vT));
		}
		// m_fCurrentAngle



		//oldFace.szMaterial = m_sCurrentMat.c_str();

		pCmd->addFace(fd, oldFace);
	}

	m_pEditor->execCommand(pCmd);
}

void CFaceEdit::alignFace()
{
	BrushFace oldFace;

	CCommandFaceEdit *pCmd = new CCommandFaceEdit();

	for(UINT i = 0, l = m_aSelectedFaces.size(); i < l; ++i)
	{
		FaceDesc &fd = m_aSelectedFaces[i];

		fd.pObject->getFaceInfo(fd.uFace, &oldFace);

		oldFace.vT = SMVector3Normalize(CBrushMesh::ProjectToFace(oldFace.vN, CBrushMesh::GetTangent(oldFace.vN)));
		oldFace.vS = SMVector3Cross(oldFace.vT, oldFace.vN);

		//! TODO save/restore angle

		if(i == 0)
		{
			m_currentSettings = oldFace;
		}

		pCmd->addFace(fd, oldFace);
	}

	m_pEditor->execCommand(pCmd);

	validateStates();
}
void CFaceEdit::alignWorld()
{
	BrushFace oldFace;

	CCommandFaceEdit *pCmd = new CCommandFaceEdit();

	for(UINT i = 0, l = m_aSelectedFaces.size(); i < l; ++i)
	{
		FaceDesc &fd = m_aSelectedFaces[i];

		fd.pObject->getFaceInfo(fd.uFace, &oldFace);
		
		float3 vAxis = CBrushMesh::GetNearestAxis(oldFace.vN);
		oldFace.vT = CBrushMesh::GetTangent(vAxis);
		oldFace.vS = SMVector3Cross(oldFace.vT, vAxis);

		//! TODO save/restore angle

		if(i == 0)
		{
			m_currentSettings = oldFace;
		}

		pCmd->addFace(fd, oldFace);
	}

	m_pEditor->execCommand(pCmd);

	validateStates();
}

void CFaceEdit::justify(int how, bool isAllAsOne)
{
	Extents extents;
	BrushFace oldFace;

	if(isAllAsOne)
	{
		Extents faceExtents;
		for(UINT i = 0, l = m_aSelectedFaces.size(); i < l; ++i)
		{
			FaceDesc &fd = m_aSelectedFaces[i];

			if(i)
			{
				fd.pObject->getFaceExtents(fd.uFace, faceExtents);
				
				for(UINT j = 0; j < 6; ++j)
				{
					CBrushMesh::BuildExtents(extents, faceExtents[j]);
				}
			}
			else
			{
				fd.pObject->getFaceExtents(fd.uFace, extents);
			}
		}
	}

	CCommandFaceEdit *pCmd = new CCommandFaceEdit();

	for(UINT i = 0, l = m_aSelectedFaces.size(); i < l; ++i)
	{
		FaceDesc &fd = m_aSelectedFaces[i];

		if(!isAllAsOne)
		{
			fd.pObject->getFaceExtents(fd.uFace, extents);
		}

		fd.pObject->getFaceInfo(fd.uFace, &oldFace);

		if(how == IDC_JUSTIFY_FIT)
		{
			oldFace.fSScale = 1.0f;
			oldFace.fTScale = 1.0f;
		}

		float2 vTopLeft, vBottomRight;
		bool isFirst = true;
		for(int j = 0; j < 6; ++j)
		{
			float2_t vTest;
			vTest.x = SMVector3Dot(extents[j], oldFace.vS) / oldFace.fSScale;
			vTest.y = SMVector3Dot(extents[j], oldFace.vT) / oldFace.fTScale;

			if(vTest.x < vTopLeft.x || isFirst)
			{
				vTopLeft.x = vTest.x;
			}

			if(vTest.y < vTopLeft.y || isFirst)
			{
				vTopLeft.y = vTest.y;
			}

			if(vTest.x > vBottomRight.x || isFirst)
			{
				vBottomRight.x = vTest.x;
			}

			if(vTest.y > vBottomRight.y || isFirst)
			{
				vBottomRight.y = vTest.y;
			}

			isFirst = false;
		}

		switch(how)
		{
		case IDC_JUSTIFY_T:
			oldFace.fTShift = -vTopLeft.y;
			break;

		case IDC_JUSTIFY_B:
			oldFace.fTShift = -vBottomRight.y;
			break;

		case IDC_JUSTIFY_C:
			oldFace.fSShift = -(vTopLeft.x + vBottomRight.x) * 0.5f - 0.5f;
			oldFace.fTShift = -(vTopLeft.y + vBottomRight.y) * 0.5f - 0.5f;
			break;

		case IDC_JUSTIFY_R:
			oldFace.fSShift = -vBottomRight.x;
			break;

		case IDC_JUSTIFY_L:
			oldFace.fSShift = -vTopLeft.x;
			break;

		case IDC_JUSTIFY_FIT:
			oldFace.fSScale = vBottomRight.x - vTopLeft.x;
			oldFace.fTScale = vBottomRight.y - vTopLeft.y;

			oldFace.fSShift = -vTopLeft.x / oldFace.fSScale;
			oldFace.fTShift = -vTopLeft.y / oldFace.fTScale;
			break;
		}

		if(i == 0)
		{
			m_currentSettings = oldFace;
			syncUI();
		}

		pCmd->addFace(fd, oldFace);
	}

	m_pEditor->execCommand(pCmd);

	validateStates();
}

void CFaceEdit::validateStates()
{
	m_validValues = VV_ALL;

	BrushFace oldFace;
	for(UINT i = 0, l = m_aSelectedFaces.size(); i < l; ++i)
	{
		FaceDesc &fd = m_aSelectedFaces[i];

		fd.pObject->getFaceInfo(fd.uFace, &oldFace);

		if(i == 0)
		{
			m_currentSettings = oldFace;
			m_fCurrentAngle = CalcAngle(m_currentSettings);
			setCurrentMaterial(oldFace.szMaterial);
		}
		else
		{
			if(m_validValues & VV_SCALE_S && m_currentSettings.fSScale != oldFace.fSScale)
			{
				m_validValues &= ~VV_SCALE_S;
			}
			if(m_validValues & VV_SCALE_T && m_currentSettings.fTScale != oldFace.fTScale)
			{
				m_validValues &= ~VV_SCALE_T;
			}
			if(m_validValues & VV_SHIFT_S && m_currentSettings.fSShift != oldFace.fSShift)
			{
				m_validValues &= ~VV_SHIFT_S;
			}
			if(m_validValues & VV_SHIFT_T && m_currentSettings.fTShift != oldFace.fTShift)
			{
				m_validValues &= ~VV_SHIFT_T;
			}
			if(m_validValues & VV_ANGLE && m_fCurrentAngle != CalcAngle(oldFace))
			{
				m_validValues &= ~VV_ANGLE;
			}
			/*if(m_validValues & VV_MATERIAL && !strcmp(m_sCurrentMat.c_str(), oldFace.szMaterial))
			{
				m_validValues &= ~VV_MATERIAL;
			}*/
		}
	}

	syncUI();
}

void CFaceEdit::alignFaceToView(const FaceDesc &fd)
{
	IXCamera *pCamera;
	m_pEditor->getCameraForView(XWP_TOP_LEFT, &pCamera);
	
	BrushFace oldFace;
	fd.pObject->getFaceInfo(fd.uFace, &oldFace);

	const float3 &vUp = pCamera->getUp();
	const float3 &vRight = pCamera->getRight();
	const float3 &vPos = pCamera->getPosition();

	oldFace.vS = vRight;
	oldFace.vT = (float3)-vUp;
	oldFace.fSShift = SMVector3Dot(oldFace.vS, vPos);
	oldFace.fTShift = SMVector3Dot(oldFace.vT, vPos);

	CCommandFaceEdit *pCmd = new CCommandFaceEdit();
	pCmd->addFace(fd, oldFace);
	m_pEditor->execCommand(pCmd);

	if(isFaceSelected(fd))
	{
		validateStates();
	}
}

bool CFaceEdit::isFaceSelected(const FaceDesc &newFD)
{
	for(UINT i = 0, l = m_aSelectedFaces.size(); i < l; ++i)
	{
		FaceDesc &fd = m_aSelectedFaces[i];
		if(!memcmp(&fd, &newFD, sizeof(FaceDesc)))
		{
			return(true);
		}
	}
	return(false);
}

//#############################################################################

void XMETHODCALLTYPE CMaterialBrowserCallback::onSelected(const char *szName)
{
	m_pEdit->setCurrentMaterial(szName);
}
