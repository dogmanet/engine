#include "stdafx.h"
#include <process.h>

#include "GeomDetector.h"

#include "Exporter.h"

CActListener::CActListener(CExporter *pExporter):
	m_pExporter(pExporter)
{
}
void CActListener::onCommitted()
{
	m_pExporter->onActsCommitted();
}
void CActListener::onChanged()
{
	m_pExporter->onActsChanged();
}


CLodListener::CLodListener(CExporter *pExporter):
	m_pExporter(pExporter)
{
}
void CLodListener::onCommitted()
{
	m_pExporter->onLodsCommitted();
}


CExporter::CExporter(const char *szFile, bool bSuppressPrompts, IExporterProvider *pCallback):
	m_szFile(szFile),
	m_bSuppressPrompts(bSuppressPrompts),
	m_pCallback(pCallback)
{
	m_pExtPhys = new CExtendedPhys();
	m_pExtAnim = new CExtendedAnim();
	m_pExtSkin = new CExtendedSkin();
	m_pExtActs = new CExtendedActs();
	m_pExtLods = new CExtendedLods();

	m_pExtActsListener = new CActListener(this);
	m_pExtActs->addListener(m_pExtActsListener);

	m_pExtLodListener = new CLodListener(this);
	m_pExtLods->addListener(m_pExtLodListener);

	memset(&m_hdr, 0, sizeof(m_hdr));
	memset(&m_hdr2, 0, sizeof(m_hdr2));
	memset(&m_hdr3, 0, sizeof(m_hdr3));
	tryLoadModel();

	m_hdr.iFlags |= MODEL_FLAG_COMPILED;
	m_hdr.iFlags |= MODEL_FLAG_NEW_STYLE_DEPS;
	m_hdr.iFlags |= MODEL_FLAG_NEW_PACKED_ANIMS;
	m_hdr.iFlags |= MODEL_FLAG_NORMALIZED_NORMALS;
}

CExporter::~CExporter()
{
	if(m_isConsoleAlloced)
	{
		FreeConsole();
	}

	mem_delete(m_pProgress);

	m_pExtActs->removeListener(m_pExtActsListener);
	mem_delete(m_pExtActsListener);

	m_pExtLods->removeListener(m_pExtLodListener);
	mem_delete(m_pExtLodListener);

	mem_delete(m_pExtended);

	clearPhysparts();
	clearHitboxes();

	clearLods();
}

int CExporter::execute()
{
	return((int)DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG1), GetActiveWindow(), DlgProc, (LPARAM)this));
}

INT_PTR CALLBACK CExporter::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CExporter *pThis = (CExporter*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lParam);
		pThis = (CExporter*)lParam;
		pThis->m_hDlgWnd = hWnd;
	}
	if(pThis)
	{
		return(pThis->dlgProc(hWnd, msg, wParam, lParam));
	}
	return(FALSE);
}

INT_PTR CALLBACK CExporter::dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND ed1 = NULL;
	HWND ed2 = NULL;
	switch(msg)
	{
	case WM_INITDIALOG:
		m_pProgress = new CProgress(GetDlgItem(m_hDlgWnd, IDC_PROGRESS1));
		{
			RECT rc;
			GetWindowRect(hWnd, &rc);

			POINT pt;
			pt.x = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
			pt.y = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;

			MoveWindow(hWnd, pt.x, pt.y, rc.right - rc.left, rc.bottom - rc.top, FALSE);

		}

		m_pExtended = new CExtended(m_hDlgWnd);
		m_pExtended->addCustomTab(m_pExtPhys);
		m_pExtended->addCustomTab(m_pExtAnim);
		m_pExtended->addCustomTab(m_pExtSkin);
		m_pExtended->addCustomTab(m_pExtActs);
		m_pExtended->addCustomTab(m_pExtLods);

		loadDefaults();

		if(m_bSuppressPrompts)
		{
			startExport();
		}

		break;

	case WM_COMMAND:
		switch(HIWORD(wParam))
		{
		case CBN_SELCHANGE:
			if(LOWORD(wParam) == IDC_ANIMATION_NAME)
			{
				HWND hCombo = (HWND)lParam;
				int len = ComboBox_GetTextLength(hCombo);
				char *tmp = (char*)alloca(sizeof(char) * (len + 1));
				ComboBox_GetText(hCombo, tmp, len + 1);
				tmp[len] = 0;
				char szTemp[32];
				for(UINT i = 0, l = m_pExtAnim->getSequenceCount(); i < l; ++i)
				{
					const ModelSequence &ms = m_pExtAnim->getSequence(i);
					if(!strcmp(tmp, ms.name))
					{
						CheckDlgButton(m_hDlgWnd, IDC_ANIMATION_LOOP, ms.bLooped ? BST_CHECKED : BST_UNCHECKED);

						sprintf(szTemp, "%d", ms.framerate);
						SetDlgItemText(m_hDlgWnd, IDC_ANIMATION_SPEED, szTemp);

						sprintf(szTemp, "%u", ms.act_chance);
						SetDlgItemText(m_hDlgWnd, IDC_ANIMATION_ACTIVITY_CHANCE, szTemp);

						const char *szAct = "";
						if(ms.activity)
						{
							szAct = m_pExtActs->getList()[ms.activity - 1].c_str();
						}
						HWND hActivity = GetDlgItem(m_hDlgWnd, IDC_ANIMATION_ACTIVITY);
						int idx = ComboBox_FindStringExact(hActivity, -1, szAct);
						ComboBox_SetCurSel(hActivity, idx);
						break;
					}
				}
			}
		}

		switch(LOWORD(wParam))
		{
		case IDOK:
			startExport();
			break;

		case IDC_EXPORT_MESH:
		case IDC_MESH_FULL:
		case IDC_MESH_STATIC:
			updateSectionMesh();
			break;

		case IDC_EXPORT_ANIMATION:
			updateSectionAnim();
			break;

		case IDC_EXPORT_GIBS:
			updateSectionGibs();
			break;

		case IDC_EXTENDED:
			m_pExtended->show();
			break;

		case IDC_ANIMATION_ACTIVITY_ADD:
			m_pExtActs->promptForNew(true);
			break;

		case IDC_ANIMATION_NAME_ADD:
			// prompt for new name
			{
				char tmp[127];
				tmp[0] = 0;
				if(invokeEditor(m_hDlgWnd, tmp, "Animation name") && tmp[0])
				{
					HWND hCombo = GetDlgItem(m_hDlgWnd, IDC_ANIMATION_NAME);

					int idx = ComboBox_AddString(hCombo, tmp);
					ComboBox_SetCurSel(hCombo, idx);
				}
			}
			break;

		case IDCANCEL:
			EndDialog(hWnd, 0);
			break;
		}
		break;

	case WM_CLOSE:
		EndDialog(m_hDlgWnd, 0);
		return(TRUE);
	default:
		return(FALSE);
	}
	return(TRUE);
}

void CExporter::updateSectionMesh()
{
	BOOL isChecked = IsDlgButtonChecked(m_hDlgWnd, IDC_EXPORT_MESH);
	BOOL isFull = IsDlgButtonChecked(m_hDlgWnd, IDC_MESH_FULL);
	BOOL isStatic = FALSE;
	if(m_isLoaded && (m_hdr.iFlags & MODEL_FLAG_STATIC))
	{
		isStatic = TRUE;
	}

	if(!isFull && m_isLoaded)
	{
		CheckDlgButton(m_hDlgWnd, IDC_MESH_TBN, (m_hdr.iFlags & MODEL_FLAG_HAS_TANGENT_BINORM));
	}

	// EnableWindow(GetDlgItem(m_hDlgWnd, IDC_MESH_STATIC), isChecked && !isStatic);
	EnableWindow(GetDlgItem(m_hDlgWnd, IDC_MESH_STATIC), isChecked && !m_isLoaded);
	EnableWindow(GetDlgItem(m_hDlgWnd, IDC_MESH_TBN), isChecked && (isFull || !m_isLoaded));
	EnableWindow(GetDlgItem(m_hDlgWnd, IDC_MESH_FULL), isChecked);
	EnableWindow(GetDlgItem(m_hDlgWnd, IDC_MESH_LOD), isChecked && !isFull);
	EnableWindow(GetDlgItem(m_hDlgWnd, IDC_MESH_LOD_ADD), isChecked && !isFull);

	updateSectionAnim();
}

void CExporter::updateSectionAnim()
{
	BOOL isChecked = IsDlgButtonChecked(m_hDlgWnd, IDC_EXPORT_ANIMATION);
	BOOL isStatic = IsDlgButtonChecked(m_hDlgWnd, IDC_EXPORT_MESH) && IsDlgButtonChecked(m_hDlgWnd, IDC_MESH_STATIC);

	if(m_isLoaded && (m_hdr.iFlags & MODEL_FLAG_STATIC))
	{
		isStatic = TRUE;
	}
	EnableWindow(GetDlgItem(m_hDlgWnd, IDC_EXPORT_ANIMATION), !isStatic);

	if(isStatic)
	{
		isChecked = FALSE;
	}

	EnableWindow(GetDlgItem(m_hDlgWnd, IDC_ANIMATION_FROM), isChecked);
	EnableWindow(GetDlgItem(m_hDlgWnd, IDC_ANIMATION_TO), isChecked);
	EnableWindow(GetDlgItem(m_hDlgWnd, IDC_ANIMATION_SPEED), isChecked);
	EnableWindow(GetDlgItem(m_hDlgWnd, IDC_ANIMATION_LOOP), isChecked);
	EnableWindow(GetDlgItem(m_hDlgWnd, IDC_ANIMATION_NAME), isChecked);
	EnableWindow(GetDlgItem(m_hDlgWnd, IDC_ANIMATION_NAME_ADD), isChecked);
	EnableWindow(GetDlgItem(m_hDlgWnd, IDC_ANIMATION_ACTIVITY), isChecked);
	EnableWindow(GetDlgItem(m_hDlgWnd, IDC_ANIMATION_ACTIVITY_ADD), isChecked);
	EnableWindow(GetDlgItem(m_hDlgWnd, IDC_ANIMATION_ACTIVITY_CHANCE), isChecked);
}

void CExporter::updateSectionGibs()
{
	BOOL isChecked = IsDlgButtonChecked(m_hDlgWnd, IDC_EXPORT_GIBS);

	EnableWindow(GetDlgItem(m_hDlgWnd, IDC_GIB_MARKED), isChecked);
	EnableWindow(GetDlgItem(m_hDlgWnd, IDC_GIB_OBJECT), isChecked);
}

void CExporter::loadDefaults()
{
	CheckDlgButton(m_hDlgWnd, IDC_MESH_TBN, !m_isLoaded || (m_hdr.iFlags & MODEL_FLAG_HAS_TANGENT_BINORM));
	CheckDlgButton(m_hDlgWnd, IDC_GIB_OBJECT, BST_CHECKED);
	CheckDlgButton(m_hDlgWnd, IDC_MESH_FULL, BST_CHECKED);
	if(m_isLoaded)
	{
		CheckDlgButton(m_hDlgWnd, IDC_MESH_STATIC, (m_hdr.iFlags & MODEL_FLAG_STATIC));
	}
	else
	{
		CheckDlgButton(m_hDlgWnd, IDC_MESH_STATIC, m_pCallback->hasBones());
	}

	char tmp[32];
	sprintf(tmp, "%u", m_uStartFrame);
	SetDlgItemText(m_hDlgWnd, IDC_ANIMATION_FROM, tmp);
	sprintf(tmp, "%d", m_iFramerate);
	SetDlgItemText(m_hDlgWnd, IDC_ANIMATION_SPEED, tmp);
	sprintf(tmp, "%u", m_uEndFrame);
	SetDlgItemText(m_hDlgWnd, IDC_ANIMATION_TO, tmp);

	updateSectionAnim();

	updateLodList();


	HWND hCombo = GetDlgItem(m_hDlgWnd, IDC_ANIMATION_NAME);
	for(UINT i = 0, l = m_pExtAnim->getSequenceCount(); i < l; ++i)
	{
		ComboBox_AddString(hCombo, m_pExtAnim->getSequence(i).name);
	}

	float fScale = m_pCallback->getConfigFloat(SXPROP_EXP_SCALE, 1.0f);
	sprintf(tmp, "%g", fScale);
	SetDlgItemText(m_hDlgWnd, IDC_SCALE, tmp);
}

void CExporter::tryLoadModel()
{
	FILE *pf = fopen(m_szFile, "rb");
	if(!pf)
	{
		return;
	}

	memset(&m_hdr, 0, sizeof(m_hdr));
	memset(&m_hdr2, 0, sizeof(m_hdr2));
	fread(&m_hdr, sizeof(m_hdr), 1, pf);

	if(m_hdr.Magick != SX_MODEL_MAGICK)
	{
		MessageBox(m_hDlgWnd, "Corrupt model. Invalid magick", "Load error", MB_OK | MB_ICONSTOP);
		fclose(pf);
		return;
	}

	if(m_hdr.iVersion != SX_MODEL_VERSION)
	{
		MessageBox(m_hDlgWnd, "Invalid version", "Load error", MB_OK | MB_ICONSTOP);
		return;
	}

	if(m_hdr.iSecondHeaderOffset)
	{
		_fseeki64(pf, m_hdr.iSecondHeaderOffset, SEEK_SET);
		fread(&m_hdr2, sizeof(m_hdr2), 1, pf);
	}
	if(m_hdr2.iThirdHeaderOffset)
	{
		_fseeki64(pf, m_hdr2.iThirdHeaderOffset, SEEK_SET);
		fread(&m_hdr3, sizeof(m_hdr3), 1, pf);
	}

	loadChunks(pf);
	loadPhysdata(pf);
	loadMaterials(pf);
	loadLoDs(pf);
	loadDeps(pf);
	loadActivities(pf);
	loadBones(pf);
	loadControllers(pf);
	loadAnimations(pf);
	loadHitboxes(pf);

	m_hdr.iFlags &= ~MODEL_FLAG_SOURCED;
	m_hdr.iFlags &= ~MODEL_FLAG_SOURCE_ANIMATION;
	m_hdr.iFlags &= ~MODEL_FLAG_SOURCE_MESH;

	m_isLoaded = true;

	fclose(pf);
}

void CExporter::loadPhysdata(FILE *pf)
{
	if(m_hdr2.iPhysicsDataOffset)
	{
		_fseeki64(pf, m_hdr2.iPhysicsDataOffset, SEEK_SET);

		ModelPhysData physData = {};
		fread(&physData, MODEL_PHYSDATA_STRUCT_SIZE, 1, pf);
		_fseeki64(pf, physData.iPartsOffset, SEEK_SET);

		m_pExtPhys->setWeight(physData.fWeight);

		m_aPhysParts.resize(physData.iPhyspartCount);
		for(UINT i = 0; i < physData.iPhyspartCount; ++i)
		{
			fread(&m_aPhysParts[i], MODEL_PHYSPART_STRUCT_SIZE, 1, pf);
		}

		for(UINT i = 0; i < physData.iPhyspartCount; ++i)
		{
			switch(m_aPhysParts[i].type)
			{
			case HT_CONVEX:
				{
					_fseeki64(pf, m_aPhysParts[i].iDataOffset, SEEK_SET);

					ModelPhyspartDataConvex *pData = new ModelPhyspartDataConvex();
					m_aPhysParts[i].pData = pData;
					fread(m_aPhysParts[i].pData, MODEL_PHYSPART_DATA_CONVEX_STRUCT_SIZE, 1, pf);

					pData->pVerts = new float3_t[pData->iVertCount];
					fread(pData->pVerts, sizeof(float3_t), pData->iVertCount, pf);
				}
				break;
			}
		}
		
		m_hdr.iFlags |= MODEL_FLAG_PHYSIC;
		if(physData.iPhyspartCount)
		{
			m_hdr.iFlags |= MODEL_FLAG_HAS_PHYSBOX;
		}
	}
}
void CExporter::loadMaterials(FILE *pf)
{
	if(m_hdr.iMaterialsOffset)
	{
		_fseeki64(pf, m_hdr.iMaterialsOffset, SEEK_SET);

		char szName[MODEL_MAX_NAME];

		for(UINT j = 0; j < m_hdr.iSkinCount; j++)
		{
			for(uint32_t i = 0; i < m_hdr.iMaterialCount; i++)
			{
				fread(szName, sizeof(char), MODEL_MAX_NAME, pf);

				m_aasMaterials[j][i] = szName;

				if(j == 0)
				{
					m_pExtSkin->addColumn(szName);
				}
				else
				{
					m_pExtSkin->addOverride(j - 1, i, szName);
				}
			}
		}
	}
}
void CExporter::loadLoDs(FILE *pf)
{
	if(m_hdr.iLODcount && m_hdr.iLODoffset)
	{
		_fseeki64(pf, m_hdr.iLODoffset, SEEK_SET);

		for(uint32_t i = 0; i < m_hdr.iLODcount; i++)
		{
			ModelLoD lod;
			fread(&lod, MODEL_LOD_STRUCT_SIZE, 1, pf);

			UINT uTris = 0;

			lod.pSubLODmeshes = new ModelLoDSubset[lod.iSubMeshCount];
			for(uint32_t j = 0; j < lod.iSubMeshCount; j++)
			{
				ModelLoDSubset &lodSubset = lod.pSubLODmeshes[j];
				fread(&lodSubset.iMaterialID, sizeof(uint32_t), 1, pf);
				fread(&lodSubset.iVectexCount, sizeof(uint32_t), 1, pf);
				fread(&lodSubset.iIndexCount, sizeof(uint32_t), 1, pf);

				size_t elSize = 0;
				if(m_hdr.iFlags & MODEL_FLAG_STATIC)
				{
					if(m_hdr.iFlags & MODEL_FLAG_HAS_TANGENT_BINORM)
					{
						elSize = sizeof(vertex_static_ex);
						lodSubset.pVertices = new vertex_static_ex[lodSubset.iVectexCount];
					}
					else
					{
						elSize = sizeof(vertex_static);
						lodSubset.pVertices = new vertex_static[lodSubset.iVectexCount];
					}
				}
				else
				{
					if(m_hdr.iFlags & MODEL_FLAG_HAS_TANGENT_BINORM)
					{
						elSize = sizeof(vertex_animated_ex);
						lodSubset.pVertices = new vertex_animated_ex[lodSubset.iVectexCount];
					}
					else
					{
						elSize = sizeof(vertex_animated);
						lodSubset.pVertices = new vertex_animated[lodSubset.iVectexCount];
					}
				}
				lodSubset.pIndices = new UINT[lodSubset.iIndexCount];
				fread(lodSubset.pVertices, elSize, lodSubset.iVectexCount, pf);
				fread(lodSubset.pIndices, sizeof(UINT), lodSubset.iIndexCount, pf);

				if(!(m_hdr.iFlags & MODEL_FLAG_NORMALIZED_NORMALS))
				{
					for(UINT k = 0; k < lodSubset.iVectexCount; ++k)
					{
						lodSubset.pVertices[k].Norm = SMVector3Normalize(lodSubset.pVertices[k].Norm);
					}
					m_hdr.iFlags |= MODEL_FLAG_NORMALIZED_NORMALS;
				}

				switch(m_hdr2.topology)
				{
				case MDLPT_TRIANGLELIST:
					uTris += lodSubset.iIndexCount / 3;
					break;
				case MDLPT_TRIANGLESTRIP:
					uTris += lodSubset.iIndexCount - 2;
					break;
				}
			}

			m_aLods.push_back(lod);
			m_pExtLods->addLod(uTris, lod.iSubMeshCount);
		}
	}
}
void CExporter::loadDeps(FILE *pf)
{
	if(m_hdr2.iDepsCount && m_hdr2.iDependensiesOffset)
	{
		_fseeki64(pf, m_hdr2.iDependensiesOffset, SEEK_SET);

		if(m_hdr.iFlags & MODEL_FLAG_NEW_STYLE_DEPS)
		{
			m_aParts.resize(m_hdr2.iDepsCount);
			fread(m_aParts, sizeof(ModelPart), m_hdr2.iDepsCount, pf);
		}
		else
		{
			ModelDependensy md;
			for(uint32_t i = 0; i < m_hdr2.iDepsCount; ++i)
			{
				fread(&md, sizeof(ModelDependensy), 1, pf);
				ModelPart pt = {};
				strcpy(pt.name, md.szName);
				strcpy(pt.file, md.szName);
				pt.attachDesc.type = MA_SKIN;
				pt.uFlags = MP_COLLISIONS | MP_RAYTRACE | MP_IMPORTED;
				pt.uImportFlags = MI_ALL;
			}
		}
	}
}
void CExporter::loadActivities(FILE *pf)
{
	if(m_hdr2.iActivitiesTableCount && m_hdr2.iActivitiesTableOffset)
	{
		_fseeki64(pf, m_hdr2.iActivitiesTableOffset, SEEK_SET);

		ModelActivity ma = {};
		for(UINT i = 0, l = m_hdr2.iActivitiesTableCount; i < l; ++i)
		{
			fread(&ma, sizeof(ModelActivity), 1, pf);
			m_pExtActs->addActivity(ma.szName);
		}
	}
}
void CExporter::loadBones(FILE *pf)
{
	if(m_hdr2.iBoneTableOffset)
	{
		_fseeki64(pf, m_hdr2.iBoneTableOffset, SEEK_SET);
		m_aBones.resize(m_hdr2.iBoneTableCount);
		fread(m_aBones, sizeof(ModelBoneName), m_hdr2.iBoneTableCount, pf);
	}
}
void CExporter::loadControllers(FILE *pf)
{
	if(m_hdr2.iControllersCount && m_hdr2.iControllersOffset)
	{
		_fseeki64(pf, m_hdr2.iControllersOffset, SEEK_SET);
		m_aControllers.resize((UINT)m_hdr2.iControllersOffset);
		fread(m_aControllers, sizeof(ModelBoneController), m_hdr2.iControllersCount, pf);
	}
}
void CExporter::loadAnimations(FILE *pf)
{
	if(m_hdr.iAnimationsOffset)
	{
		_fseeki64(pf, m_hdr.iAnimationsOffset, SEEK_SET);

		ModelSequence ms = {0};
		for(UINT i = 0; i < m_hdr.iAnimationCount; ++i)
		{
			if(m_hdr.iFlags & MODEL_FLAG_NEW_PACKED_ANIMS)
			{
				fread(&ms, MODEL_SEQUENCE_STRUCT_SIZE, 1, pf);
			}
			else
			{
				fread(&ms, MODEL_SEQUENCE_STRUCT_SIZE_OLD, 1, pf);
			}

			ms.m_vmAnimData = new ModelBone*[ms.iNumFrames];
			
			ModelBone mb;
			for(UINT j = 0; j < ms.iNumFrames; ++j)
			{
				ms.m_vmAnimData[j] = new ModelBone[m_hdr2.iBoneTableCount];
				fread(ms.m_vmAnimData[j], sizeof(ModelBone), m_hdr2.iBoneTableCount, pf);
			}

			m_pExtAnim->addSequence(ms);
		}
	}
}
void CExporter::loadHitboxes(FILE *pf)
{
	if(m_hdr2.iHitboxCount && m_hdr2.iHitboxesOffset)
	{
		_fseeki64(pf, m_hdr2.iHitboxesOffset, SEEK_SET);
		m_aHitboxes.resize(m_hdr2.iHitboxCount);
		fread(m_aHitboxes, sizeof(ModelHitbox), m_hdr2.iHitboxCount, pf);
	}
}
void CExporter::loadChunks(FILE *pf)
{
	if(m_hdr3.uChunksCount && m_hdr3.uChunksOffset)
	{
		_fseeki64(pf, m_hdr3.uChunksOffset, SEEK_SET);
		m_aChunks.resize(m_hdr3.uChunksCount);
		fora(i, m_aChunks)
		{
			fread(&m_aChunks[i], sizeof(ModelChunkHeader), 1, pf);
		}

		fora(i, m_aChunks)
		{
			ModelChunkRaw &chunk = m_aChunks[i];
			_fseeki64(pf, chunk.hdr.uDataOffset, SEEK_SET);
			if(chunk.hdr.guidType == DSE_CHUNK_HITBOXES_EX_GUID)
			{
				// load ex hitboxes
				ModelHitboxExChunk chunkData = {};
				fread(&chunkData, sizeof(ModelHitboxExChunk), 1, pf);
				if(chunkData.uHitboxExCount)
				{
					m_aHitboxesEx.resize(chunkData.uHitboxExCount);
					for(UINT j = 0; j < chunkData.uHitboxExCount; ++j)
					{
						fread(&m_aHitboxesEx[j], MODEL_HITBOXEX_STRUCT_SIZE, 1, pf);
					}

					fora(j, m_aHitboxesEx)
					{
						if(m_aHitboxesEx[j].hitbox.type == HT_CONVEX)
						{
							_fseeki64(pf, m_aHitboxesEx[j].hitbox.iDataOffset, SEEK_SET);

							ModelPhyspartDataConvex *pData = new ModelPhyspartDataConvex();
							m_aHitboxesEx[j].hitbox.pData = pData;
							fread(m_aHitboxesEx[j].hitbox.pData, MODEL_PHYSPART_DATA_CONVEX_STRUCT_SIZE, 1, pf);

							pData->pVerts = new float3_t[pData->iVertCount];
							fread(pData->pVerts, sizeof(float3_t), pData->iVertCount, pf);
						}
					}
				}
			}
			else
			{
				chunk.aData.resize((UINT)chunk.hdr.uDataSize);
				fread(chunk.aData, sizeof(byte), chunk.hdr.uDataSize, pf);
			}
		}
	}
}


void CExporter::startExport()
{
	if(!checkSettings())
	{
		return;
	}

	EnumChildWindows(m_hDlgWnd, [](HWND hWnd, LPARAM lParam)
	{
		EnableWindow(hWnd, FALSE);
		return(TRUE);
	}, NULL);

	_beginthread(ProcessThread, 0, this);
}

void CExporter::ProcessThread(void *p)
{
	CExporter *exp = (CExporter*)p;
	exp->doExport();
}

void CExporter::doExport()
{
	bool isStatic = IsDlgButtonChecked(m_hDlgWnd, IDC_MESH_STATIC) == BST_CHECKED;
	bool bExportMesh = IsDlgButtonChecked(m_hDlgWnd, IDC_EXPORT_MESH) == BST_CHECKED;
	bool includeTB = IsDlgButtonChecked(m_hDlgWnd, IDC_MESH_TBN) == BST_CHECKED;

	if(!isStatic && IsDlgButtonChecked(m_hDlgWnd, IDC_EXPORT_ANIMATION) == BST_CHECKED)
	{
		m_pProgress->setRange(0.0f, 0.8f);
	}
	else
	{
		m_pProgress->setRange(0.0f, 0.9f);
	}

	if(!m_pCallback->prepare(m_pProgress, isStatic, bExportMesh, includeTB))
	{
		if(!m_bSuppressPrompts)
		{
			MessageBox(m_hDlgWnd, "Unable to prepare model to export!", "Fatal error!", MB_OK | MB_ICONSTOP);
		}

		PostMessage(m_hDlgWnd, WM_CLOSE, 0, NULL);
		return;
	}

	if(!isStatic && (bExportMesh || IsDlgButtonChecked(m_hDlgWnd, IDC_EXPORT_ANIMATION) == BST_CHECKED))
	{
		if(!prepareSkeleton(IsDlgButtonChecked(m_hDlgWnd, IDC_EXPORT_MESH) == BST_CHECKED))
		{
			if(!m_bSuppressPrompts)
			{
				MessageBox(m_hDlgWnd, "Skeletons is incompatible! Consider to fix skeleton or perform new export!", "Fatal error!", MB_OK | MB_ICONSTOP);
			}

			PostMessage(m_hDlgWnd, WM_CLOSE, 0, NULL);
			return;
		}
	}

	if(bExportMesh)
	{
		performRetopology();
		m_hdr2.topology = MDLPT_TRIANGLELIST;

		bool isFullExport = IsDlgButtonChecked(m_hDlgWnd, IDC_MESH_FULL) == BST_CHECKED;

		if(isFullExport)
		{
			preparePhysMesh();
			preparePhysMesh(false, true);
			prepareLodMesh();

			if(includeTB)
			{
				m_hdr.iFlags |= MODEL_FLAG_HAS_TANGENT_BINORM;
			}
			else
			{
				m_hdr.iFlags &= ~MODEL_FLAG_HAS_TANGENT_BINORM;
			}
		}
		else
		{
			char tmp[32];
			GetDlgItemText(m_hDlgWnd, IDC_MESH_LOD, tmp, sizeof(tmp));
			if(!strcmp(tmp, "Physbox"))
			{
				preparePhysMesh(true);
			}
			else if(!strcmp(tmp, "Hitbox"))
			{
				preparePhysMesh(true, true);
			}
			else
			{
				int iLod = 0;
				if(!sscanf(tmp, "Lod #%d", &iLod))
				{
					iLod = (int)m_aLods.size();
				}

				prepareLodMesh(iLod);
			}
		}
	}
	if(!isStatic && IsDlgButtonChecked(m_hDlgWnd, IDC_EXPORT_ANIMATION) == BST_CHECKED)
	{
		m_pProgress->setRange(0.9f);
		if(!m_pCallback->preapareAnimationTrack(m_pProgress, m_uStartFrame, m_uEndFrame - m_uStartFrame + 1))
		{
			if(!m_bSuppressPrompts)
			{
				MessageBox(m_hDlgWnd, "Unable to prepare animation to export!", "Fatal error!", MB_OK | MB_ICONSTOP);
			}

			PostMessage(m_hDlgWnd, WM_CLOSE, 0, NULL);
			return;
		}

		prepareAnimation();
	}
		
	if(!m_isLoaded)
	{
		if(isStatic)
		{
			m_hdr.iFlags |= MODEL_FLAG_STATIC;
		}
		else
		{
			m_hdr.iFlags |= MODEL_FLAG_ANIMATED;
		}
	}
	if(!writeFile(m_szFile))
	{
		MessageBox(m_hDlgWnd, "Unable to write file!", "Save error", MB_OK | MB_ICONSTOP);
		PostMessage(m_hDlgWnd, WM_CLOSE, 0, NULL);
		return;
	}

	m_pProgress->setRange(1.0f);

	if(IsDlgButtonChecked(m_hDlgWnd, IDC_EXPORT_GIBS) == BST_CHECKED)
	{
		memset(&m_hdr, 0, sizeof(m_hdr));
		memset(&m_hdr2, 0, sizeof(m_hdr2));
		m_hdr.iFlags = MODEL_FLAG_COMPILED | MODEL_FLAG_STATIC | MODEL_FLAG_HAS_TANGENT_BINORM | MODEL_FLAG_NEW_STYLE_DEPS | MODEL_FLAG_NORMALIZED_NORMALS;
		
		CheckDlgButton(m_hDlgWnd, IDC_MESH_STATIC, BST_CHECKED);
		CheckDlgButton(m_hDlgWnd, IDC_MESH_TBN, BST_CHECKED);

		bool isObjectMode = IsDlgButtonChecked(m_hDlgWnd, IDC_GIB_OBJECT) == BST_CHECKED;
		UINT uTotalGibs = 0;

		UINT uLayerCount = m_pCallback->getLayerCount();
		for(UINT i = 0; i < uLayerCount; ++i)
		{
			if(isObjectMode)
			{
				uTotalGibs += m_pCallback->getLayerObjectCount(i);
			}
			else
			{
				const char *szLayer = m_pCallback->getLayerName(i);

				if(memcmp(szLayer, "#gib", 4) == 0)
				{
					++uTotalGibs;
				}
			}
		}

		UINT uProcessedGibs = 0;

		char szGibFile[MAX_PATH];
		char szBaseFile[MAX_PATH];

		const char *szMask = "%s_gib%02u.dse";
		if(uTotalGibs > 99)
		{
			szMask = "%s_gib%03u.dse";
		}
		strcpy(szBaseFile, m_szFile);
		szBaseFile[strlen(szBaseFile) - 4] = 0;

		clearLods();

		for(UINT i = 0; i < uLayerCount; ++i)
		{
			// prepareLodMesh();

			if(isObjectMode)
			{
				UINT uObjectCount = m_pCallback->getLayerObjectCount(i);
				for(UINT j = 0; j < uObjectCount; ++j)
				{
					// process one gib
					m_iOnlyLayer = i;
					m_iOnlyObject = j;
					prepareLodMesh(0);

					sprintf(szGibFile, szMask, szBaseFile, uProcessedGibs);
					if(!writeFile(szGibFile))
					{
						logWarning("Unable to write file %s!", szGibFile);
					}

					m_pProgress->setProgress((float)++uProcessedGibs / (float)uTotalGibs);
				}
			}
			else
			{
				const char *szLayer = m_pCallback->getLayerName(i);

				if(memcmp(szLayer, "#gib", 4) == 0)
				{
					// process one gib
					m_szOnlyLayer = szLayer;
					prepareLodMesh(0);

					sprintf(szGibFile, szMask, szBaseFile, uProcessedGibs);
					if(!writeFile(szGibFile))
					{
						logWarning("Unable to write file %s!", szGibFile);
					}

					m_pProgress->setProgress((float)++uProcessedGibs / (float)uTotalGibs);
				}
			}

		}
	}

	m_pProgress->setProgress(1.0f);

	if(!m_bSuppressPrompts)
	{
		MessageBox(m_hDlgWnd, "Export done!", "DSE Model export", NULL);
	}

	PostMessage(m_hDlgWnd, WM_CLOSE, 0, NULL);
}

bool CExporter::writeFile(const char *szFile, bool isForGib)
{
	FILE *pf = fopen(szFile, "wb");
	if(!pf)
	{
		return(false);
	}
	m_hdr.Magick = SX_MODEL_MAGICK;
	m_hdr.iVersion = SX_MODEL_VERSION;
	m_hdr2.iBboxInfoOffset = 0;
	fwrite(&m_hdr, sizeof(m_hdr), 1, pf);

	m_hdr.iSecondHeaderOffset = _ftelli64(pf);
	fwrite(&m_hdr2, sizeof(m_hdr2), 1, pf);

	m_hdr2.iThirdHeaderOffset = _ftelli64(pf);
	fwrite(&m_hdr3, sizeof(m_hdr3), 1, pf);

	if(isForGib)
	{
		//writePhysdata(pf); //?
		writeMaterials(pf);
		writeLoDs(pf);
	}
	else
	{
		writePhysdata(pf);
		writeMaterials(pf);
		writeLoDs(pf);
		writeDeps(pf);
		writeActivities(pf);
		writeBones(pf);
		writeControllers(pf);
		writeAnimations(pf);
		writeHitboxes(pf);
		writeChunks(pf);
	}

	_fseeki64(pf, 0, SEEK_SET);
	fwrite(&m_hdr, sizeof(m_hdr), 1, pf);
	fwrite(&m_hdr2, sizeof(m_hdr2), 1, pf);
	fwrite(&m_hdr3, sizeof(m_hdr3), 1, pf);

	fclose(pf);

	return(true);
}

void CExporter::writePhysdata(FILE *pf)
{
	m_hdr2.iPhysicsDataOffset = _ftelli64(pf);

	ModelPhysData physData = {};
	physData.fWeight = m_pExtPhys->getWeight();
	physData.iPhyspartCount = m_aPhysParts.size();
	physData.iPartsOffset = m_hdr2.iPhysicsDataOffset + MODEL_PHYSDATA_STRUCT_SIZE;
	fwrite(&physData, MODEL_PHYSDATA_STRUCT_SIZE, 1, pf);

	uint64_t uBaseOffset = physData.iPartsOffset + MODEL_PHYSPART_STRUCT_SIZE * physData.iPhyspartCount;

	for(UINT i = 0; i < physData.iPhyspartCount; ++i)
	{
		switch(m_aPhysParts[i].type)
		{
		case HT_CONVEX:
			{
				m_aPhysParts[i].iDataOffset = uBaseOffset;
				uBaseOffset += MODEL_PHYSPART_DATA_CONVEX_STRUCT_SIZE;
				ModelPhyspartDataConvex *pData = (ModelPhyspartDataConvex*)m_aPhysParts[i].pData;
				uBaseOffset += sizeof(float3_t) * pData->iVertCount;
			}
			break;
		}
	}

	for(UINT i = 0; i < physData.iPhyspartCount; ++i)
	{
		fwrite(&m_aPhysParts[i], MODEL_PHYSPART_STRUCT_SIZE, 1, pf);
	}

	for(UINT i = 0; i < physData.iPhyspartCount; ++i)
	{
		switch(m_aPhysParts[i].type)
		{
		case HT_CONVEX:
			{
				fwrite(m_aPhysParts[i].pData, MODEL_PHYSPART_DATA_CONVEX_STRUCT_SIZE, 1, pf);

				ModelPhyspartDataConvex *pData = (ModelPhyspartDataConvex*)m_aPhysParts[i].pData;

				fwrite(pData->pVerts, sizeof(float3_t), pData->iVertCount, pf);
			}
			break;
		}
	}

	m_hdr.iFlags |= MODEL_FLAG_PHYSIC;
	if(physData.iPhyspartCount)
	{
		m_hdr.iFlags |= MODEL_FLAG_HAS_PHYSBOX;
	}
}
void CExporter::writeMaterials(FILE *pf)
{
	m_hdr.iMaterialsOffset = _ftelli64(pf);
	char szName[MODEL_MAX_NAME];

	m_hdr.iSkinCount = m_pExtSkin->getRowCount() + 1;

	for(UINT j = 0; j < m_hdr.iSkinCount; j++)
	{
		for(uint32_t i = 0; i < m_hdr.iMaterialCount; i++)
		{
			strncpy(szName, j == 0 ? m_aasMaterials[j][i].c_str() : m_pExtSkin->getCell(j - 1, i), MODEL_MAX_NAME);
			szName[MODEL_MAX_NAME - 1] = 0;
			fwrite(szName, sizeof(char), MODEL_MAX_NAME, pf);
		}
	}
}
void CExporter::writeLoDs(FILE *pf)
{
	m_hdr.iLODoffset = _ftelli64(pf);

	m_hdr.iLODcount = m_aLods.size();

	for(uint32_t i = 0; i < m_hdr.iLODcount; i++)
	{
		ModelLoD &lod = m_aLods[i];
		fwrite(&lod, MODEL_LOD_STRUCT_SIZE, 1, pf);

		for(uint32_t j = 0; j < lod.iSubMeshCount; j++)
		{
			ModelLoDSubset &lodSubset = lod.pSubLODmeshes[j];
			fwrite(&lodSubset.iMaterialID, sizeof(uint32_t), 1, pf);
			fwrite(&lodSubset.iVectexCount, sizeof(uint32_t), 1, pf);
			fwrite(&lodSubset.iIndexCount, sizeof(uint32_t), 1, pf);

			size_t elSize = 0;
			if(m_hdr.iFlags & MODEL_FLAG_STATIC)
			{
				if(m_hdr.iFlags & MODEL_FLAG_HAS_TANGENT_BINORM)
				{
					elSize = sizeof(vertex_static_ex);
				}
				else
				{
					elSize = sizeof(vertex_static);
				}
			}
			else
			{
				if(m_hdr.iFlags & MODEL_FLAG_HAS_TANGENT_BINORM)
				{
					elSize = sizeof(vertex_animated_ex);
				}
				else
				{
					elSize = sizeof(vertex_animated);
				}
			}
			fwrite(lodSubset.pVertices, elSize, lodSubset.iVectexCount, pf);
			fwrite(lodSubset.pIndices, sizeof(UINT), lodSubset.iIndexCount, pf);
		}

	}
}
void CExporter::writeDeps(FILE *pf)
{
#if 0
	ModelPart &p = m_aParts[0];
	p.uImportFlags = MI_ANIMATIONS | MI_HITBOXES | MI_CONTROLLERS;
	p.uFlags = MP_IMPORTED;
	p.attachDesc.type = MA_SKIN;
	p.attachDesc.v3Offs = float3(0.0f);
	p.attachDesc.szBone[0] = 0;
	strcpy(p.name, "AnimationsLibrary");
	strcpy(p.file, "models/animlib/human.dse");
#endif
	m_hdr2.iDependensiesOffset = _ftelli64(pf);
	m_hdr2.iDepsCount = m_aParts.size();

	fwrite(m_aParts, sizeof(ModelPart), m_hdr2.iDepsCount, pf);
}
void CExporter::writeActivities(FILE *pf)
{
	m_hdr2.iActivitiesTableOffset = _ftelli64(pf);
	m_hdr2.iActivitiesTableCount = m_pExtActs->getList().size();

	auto &list = m_pExtActs->getList();
	ModelActivity ma;
	for(UINT i = 0; i < m_hdr2.iActivitiesTableCount; ++i)
	{
		strncpy(ma.szName, list[i].c_str(), MODEL_MAX_NAME);
		ma.szName[MODEL_MAX_NAME - 1] = 0;
		fwrite(&ma, sizeof(ModelActivity), 1, pf);
	}
}
void CExporter::writeBones(FILE *pf)
{
	m_hdr.iBonesOffset = _ftelli64(pf);
	m_hdr2.iBoneTableOffset = _ftelli64(pf);
	m_hdr.iBoneCount = m_aBones.size();
	m_hdr2.iBoneTableCount = m_aBones.size();

	fwrite(m_aBones, sizeof(ModelBoneName), m_hdr2.iBoneTableCount, pf);
}
void CExporter::writeControllers(FILE *pf)
{
	m_hdr2.iControllersOffset = _ftelli64(pf);
	m_hdr2.iControllersCount = m_aControllers.size();
	fwrite(m_aControllers, sizeof(ModelBoneController), m_hdr2.iControllersCount, pf);
}
void CExporter::writeAnimations(FILE *pf)
{
	m_hdr.iAnimationsOffset = _ftelli64(pf);
	m_hdr.iAnimationCount = m_pExtAnim->getSequenceCount();

	for(UINT i = 0; i < m_hdr.iAnimationCount; ++i)
	{
		const ModelSequence &ms = m_pExtAnim->getSequence(i);
		fwrite(&ms, MODEL_SEQUENCE_STRUCT_SIZE, 1, pf);

		ModelBone mb;
		for(UINT j = 0; j < ms.iNumFrames; ++j)
		{
			fwrite(ms.m_vmAnimData[j], sizeof(ModelBone), m_hdr2.iBoneTableCount, pf);
		}
	}
}
void CExporter::writeHitboxes(FILE *pf)
{
	m_hdr2.iHitboxesOffset = _ftelli64(pf);
	m_hdr2.iHitboxCount = m_aHitboxes.size();
	fwrite(m_aHitboxes, sizeof(ModelHitbox), m_hdr2.iHitboxCount, pf);
}
void CExporter::writeChunks(FILE *pf)
{
	enableChunk(DSE_CHUNK_HITBOXES_EX_GUID, m_aHitboxesEx.size() != 0);

	m_hdr3.uChunksOffset = _ftelli64(pf);
	m_hdr3.uChunksCount = m_aChunks.size();

	fora(i, m_aChunks)
	{
		fwrite(&m_aChunks[i], sizeof(ModelChunkHeader), 1, pf);
	}

	fora(i, m_aChunks)
	{
		ModelChunkRaw &chunk = m_aChunks[i];
		chunk.hdr.uDataOffset = _ftelli64(pf);
		if(chunk.hdr.guidType == DSE_CHUNK_HITBOXES_EX_GUID)
		{
			// write ex hitboxes
			ModelHitboxExChunk chunkData = {};
			chunkData.uHitboxExCount = m_aHitboxesEx.size();
			fwrite(&chunkData, sizeof(ModelHitboxExChunk), 1, pf);
			uint64_t uBaseOffset = _ftelli64(pf) + MODEL_HITBOXEX_STRUCT_SIZE * chunkData.uHitboxExCount;

			fora(j, m_aHitboxesEx)
			{
				if(m_aHitboxesEx[j].hitbox.type == HT_CONVEX)
				{
					m_aHitboxesEx[j].hitbox.iDataOffset = uBaseOffset;
					uBaseOffset += MODEL_PHYSPART_DATA_CONVEX_STRUCT_SIZE;
					ModelPhyspartDataConvex *pData = (ModelPhyspartDataConvex*)m_aHitboxesEx[j].hitbox.pData;
					uBaseOffset += sizeof(float3_t) * pData->iVertCount;
				}
			}

			fora(j, m_aHitboxesEx)
			{
				fwrite(&m_aHitboxesEx[j], MODEL_HITBOXEX_STRUCT_SIZE, 1, pf);
			}

			fora(j, m_aHitboxesEx)
			{
				if(m_aHitboxesEx[j].hitbox.type == HT_CONVEX)
				{
					fwrite(m_aHitboxesEx[j].hitbox.pData, MODEL_PHYSPART_DATA_CONVEX_STRUCT_SIZE, 1, pf);

					ModelPhyspartDataConvex *pData = (ModelPhyspartDataConvex*)m_aHitboxesEx[j].hitbox.pData;

					fwrite(pData->pVerts, sizeof(float3_t), pData->iVertCount, pf);
				}
			}
		}
		else
		{
			fwrite(chunk.aData, sizeof(byte), chunk.aData.size(), pf);
		}

		chunk.hdr.uDataSize = _ftelli64(pf) - chunk.hdr.uDataOffset;
	}

	uint64_t uLastOffset = _ftelli64(pf);
	_fseeki64(pf, m_hdr3.uChunksOffset, SEEK_SET);
	fora(i, m_aChunks)
	{
		fwrite(&m_aChunks[i], sizeof(ModelChunkHeader), 1, pf);
	}

	_fseeki64(pf, uLastOffset, SEEK_SET);
}

static TCHAR* FixName(const TCHAR* name)
{
	static TCHAR buffer[256];
	TCHAR* cPtr;

	strcpy(buffer, name);
	cPtr = buffer;

	while(*cPtr)
	{
		if(*cPtr == '"')
			*cPtr = 39;
		else if(*cPtr <= 31)
			*cPtr = '_';
		cPtr++;
	}

	return buffer;
}


UINT CExporter::addTexture(const char *szName)
{
	char *mapName = FixName(szName);
	int iDotPos = -1;
	char *tmp = mapName;
	while(*tmp)
	{
		if(*tmp == '.')
		{
			iDotPos = (int)(tmp - mapName);
		}
		++tmp;
	}

	if(iDotPos >= 0)
	{
		mapName[iDotPos] = 0;
	}

	const char *szBaseName = basename(mapName);

	if(!m_hdr.iSkinCount)
	{
		m_hdr.iSkinCount = 1;
	}

	for(uint32_t i = 0; i < m_hdr.iMaterialCount; i++)
	{
		if(!fstrcmp(m_aasMaterials[0][i].c_str(), szBaseName))
		{
			return(i);
		}
	}

	for(UINT j = 0; j < m_hdr.iSkinCount; j++)
	{
		m_aasMaterials[j][m_hdr.iMaterialCount] = j == 0 ? szBaseName : "";
	}
	m_pExtSkin->addColumn(szBaseName);
	++m_hdr.iMaterialCount;

	return(m_hdr.iMaterialCount - 1);
}

void CExporter::onActsCommitted()
{
	HWND hCombo = GetDlgItem(m_hDlgWnd, IDC_ANIMATION_ACTIVITY);

	ComboBox_ResetContent(hCombo);

	ComboBox_AddString(hCombo, "");
	auto &list = m_pExtActs->getList();
	for(UINT i = 0, l = list.size(); i < l; ++i)
	{
		ComboBox_AddString(hCombo, list[i].c_str());
	}

	m_pExtAnim->setActivityList(m_pExtActs->getList());
}

void CExporter::onActsChanged()
{
	m_pExtAnim->setActivityList(m_pExtActs->getUncommittedList());
}

void CExporter::onLodsCommitted()
{
	UINT uCount = m_pExtLods->getCount();

	Array<ModelLoD> aNewLods(uCount);
	for(UINT i = 0; i < uCount; ++i)
	{
		int idx = m_pExtLods->getItem(i);
		aNewLods.push_back(m_aLods[idx]);
		m_aLods[idx].pSubLODmeshes = NULL;
	}

	for(UINT i = 0, l = m_aLods.size(); i < l; ++i)
	{
		ModelLoD &lod = m_aLods[i];
		if(lod.pSubLODmeshes)
		{
			for(UINT j = 0; j < lod.iSubMeshCount; ++j)
			{
				mem_delete_a(lod.pSubLODmeshes[j].pIndices);
				mem_delete_a(lod.pSubLODmeshes[j].pVertices);
			}
			mem_delete_a(lod.pSubLODmeshes);
		}
	}

	m_aLods = aNewLods;

	updateLodList();
}

void CExporter::updateLodList()
{
	HWND hCombo = GetDlgItem(m_hDlgWnd, IDC_MESH_LOD);

	ComboBox_ResetContent(hCombo);

	ComboBox_AddString(hCombo, "Physbox");
	ComboBox_AddString(hCombo, "Hitbox");
	char tmp[32];
	for(UINT i = 0, l = m_aLods.size(); i < l; ++i)
	{
		sprintf(tmp, "Lod #%u", i);
		ComboBox_AddString(hCombo, tmp);
	}
	ComboBox_AddString(hCombo, "New lod");
}

void CExporter::performRetopology()
{
	if(m_hdr2.topology == MDLPT_TRIANGLESTRIP)
	{
		m_hdr2.topology = MDLPT_TRIANGLELIST;

		for(UINT i = 0, l = m_aLods.size(); i < l; ++i)
		{
			ModelLoD &lod = m_aLods[i];
			for(UINT j = 0; j < lod.iSubMeshCount; ++j)
			{
				UINT uNewIndexCount = (lod.pSubLODmeshes[j].iIndexCount - 2) * 3;
				UINT *pNewIndices = new UINT[uNewIndexCount];

				UINT uIdx = 0;
				for(UINT k = 0, kl = lod.pSubLODmeshes[j].iIndexCount - 2; k < kl;  ++k)
				{
					pNewIndices[uIdx++] = lod.pSubLODmeshes[j].pIndices[k];
					pNewIndices[uIdx++] = lod.pSubLODmeshes[j].pIndices[k + 1];
					pNewIndices[uIdx++] = lod.pSubLODmeshes[j].pIndices[k + 2];
				}

				assert(uIdx == uNewIndexCount);

				mem_delete_a(lod.pSubLODmeshes[j].pIndices);
				lod.pSubLODmeshes[j].pIndices = pNewIndices;
				lod.pSubLODmeshes[j].iIndexCount = uNewIndexCount;
			}
		}
	}
}

void CExporter::clearLods(int iTargetLod)
{
	if(iTargetLod < 0)
	{
		for(UINT i = 0, l = m_aLods.size(); i < l; ++i)
		{
			ModelLoD &lod = m_aLods[i];
			for(UINT j = 0; j < lod.iSubMeshCount; ++j)
			{
				mem_delete_a(lod.pSubLODmeshes[j].pIndices);
				mem_delete_a(lod.pSubLODmeshes[j].pVertices);
			}
			mem_delete_a(lod.pSubLODmeshes);
		}
		m_aLods.clearFast();
	}
	else if(m_aLods.size() > (UINT)iTargetLod)
	{
		ModelLoD &lod = m_aLods[iTargetLod];
		for(UINT j = 0; j < lod.iSubMeshCount; ++j)
		{
			mem_delete_a(lod.pSubLODmeshes[j].pIndices);
			mem_delete_a(lod.pSubLODmeshes[j].pVertices);
		}
		mem_delete_a(lod.pSubLODmeshes);
		lod.iSubMeshCount = 0;
	}
}

void CExporter::clearPhysparts()
{
	for(UINT i = 0, l = m_aPhysParts.size(); i < l; ++i)
	{
		switch(m_aPhysParts[i].type)
		{
		case HT_CONVEX:
			{
				ModelPhyspartDataConvex *pData = (ModelPhyspartDataConvex*)m_aPhysParts[i].pData;
				mem_delete_a(pData->pVerts);
				mem_delete(pData);
			}
			break;
		}
	}
	m_aPhysParts.clearFast();
}

void CExporter::clearHitboxes()
{
	fora(i, m_aHitboxesEx)
	{
		switch(m_aHitboxesEx[i].hitbox.type)
		{
		case HT_CONVEX:
		{
			ModelPhyspartDataConvex *pData = (ModelPhyspartDataConvex*)m_aHitboxesEx[i].hitbox.pData;
			mem_delete_a(pData->pVerts);
			mem_delete(pData);
		}
		break;
		}
	}
	m_aHitboxesEx.clearFast();
	//m_aHitboxes.clearFast();
}

void CExporter::enableChunk(const XGUID &guid, bool yesNo)
{
	int idx = m_aChunks.indexOf(guid, [](const ModelChunkRaw &chunk, const XGUID &guid){
		return(chunk.hdr.guidType == guid);
	});

	if(yesNo && idx < 0)
	{
		m_aChunks[m_aChunks.size()].hdr.guidType = guid;
	}
	else if(!yesNo && idx >= 0)
	{
		m_aChunks.erase(idx);
	}
}

bool CExporter::checkSettings()
{
	if(IsDlgButtonChecked(m_hDlgWnd, IDC_EXPORT_MESH) == BST_CHECKED && IsDlgButtonChecked(m_hDlgWnd, IDC_MESH_FULL) != BST_CHECKED)
	{
		char tmp[32];
		GetDlgItemText(m_hDlgWnd, IDC_MESH_LOD, tmp, sizeof(tmp));

		if(!tmp[0])
		{
			MessageBox(m_hDlgWnd, "Please select LOD", "Error", MB_OK | MB_ICONSTOP);
			return(false);
		}
	}

	if(IsDlgButtonChecked(m_hDlgWnd, IDC_EXPORT_ANIMATION) == BST_CHECKED && IsWindowEnabled(GetDlgItem(m_hDlgWnd, IDC_EXPORT_ANIMATION)))
	{
		BOOL isFound = FALSE;
		m_uStartFrame = GetDlgItemInt(m_hDlgWnd, IDC_ANIMATION_FROM, &isFound, FALSE);
		if(!isFound)
		{
			MessageBox(m_hDlgWnd, "Invalid start frame", "Error", MB_OK | MB_ICONSTOP);
			return(false);
		}
		m_uEndFrame = GetDlgItemInt(m_hDlgWnd, IDC_ANIMATION_TO, &isFound, FALSE);
		if(!isFound || m_uEndFrame < m_uStartFrame)
		{
			MessageBox(m_hDlgWnd, "Invalid end frame", "Error", MB_OK | MB_ICONSTOP);
			return(false);
		}
		m_iFramerate = (int)GetDlgItemInt(m_hDlgWnd, IDC_ANIMATION_SPEED, &isFound, TRUE);
		if(!isFound)
		{
			MessageBox(m_hDlgWnd, "Invalid animation speed", "Error", MB_OK | MB_ICONSTOP);
			return(false);
		}
		m_isLooped = IsDlgButtonChecked(m_hDlgWnd, IDC_ANIMATION_LOOP) == BST_CHECKED;

		m_szAnimationName[0] = 0;
		GetDlgItemText(m_hDlgWnd, IDC_ANIMATION_NAME, m_szAnimationName, MODEL_MAX_NAME);
		m_szAnimationName[MODEL_MAX_NAME - 1] = 0;
		if(!m_szAnimationName[0])
		{
			MessageBox(m_hDlgWnd, "Invalid animation name", "Error", MB_OK | MB_ICONSTOP);
			return(false);
		}

		m_szAnimationActivity[0] = 0;
		GetDlgItemText(m_hDlgWnd, IDC_ANIMATION_ACTIVITY, m_szAnimationActivity, MODEL_MAX_NAME);
		m_szAnimationActivity[MODEL_MAX_NAME - 1] = 0;

		m_uActivityChance = GetDlgItemInt(m_hDlgWnd, IDC_ANIMATION_ACTIVITY_CHANCE, &isFound, FALSE);
		if(!isFound && m_szAnimationActivity[0])
		{
			MessageBox(m_hDlgWnd, "Invalid activity chance", "Error", MB_OK | MB_ICONSTOP);
			return(false);
		}
	}

	char tmp[32];
	GetDlgItemText(m_hDlgWnd, IDC_SCALE, tmp, sizeof(tmp));
	tmp[31] = 0;

	if(sscanf(tmp, "%g", &m_fScale) != 1)
	{
		MessageBox(m_hDlgWnd, "Invalid scale", "Error", MB_OK | MB_ICONSTOP);
		return(false);
	}
	m_pCallback->setConfigFloat(SXPROP_EXP_SCALE, m_fScale);

	return(true);
}

static void CalcTangentBasis(const float3 &p1, const float3 &p2, const float3 &p3,
	const float2 &t1, const float2 &t2, const float2 &t3,
	float3 *pTangent, float3 *pBinormal)
{
	float3 e1 = p2 - p1;
	float3 e2 = p3 - p1;
	float2 et1 = t2 - t1;
	float2 et2 = t3 - t1;
	float3 tangent = 0.0f;
	float3 binormal = 0.0f;
	float tmp = 0.0;
	if(fabsf(et1.x*et2.y - et1.y*et2.x)<0.0001f)
		tmp = 1.0f;
	else
		tmp = 1.0f / (et1.x*et2.y - et1.y*et2.x);
	tangent = (e1 * et2.y - e2 * et1.y) * tmp;
	binormal = (e2 * et1.x - e1 * et2.x) * tmp;

	*pTangent = SMVector3Normalize(tangent);
	*pBinormal = SMVector3Normalize(binormal);
}

template<typename T>
static Array<T> GenerateTBN(T *pVertices, UINT *pIndices, UINT uVertexCount, UINT uFaceCount)
{
	Array<T> aNewVertices;

	for(UINT i = 0; i < uVertexCount; ++i)
	{
		pVertices[i].Tangent = float3(0.0f);
	}

	float3 vBaseNormal;
	float3 vBaseTangent;
	float3 vBaseBinormal;
	T vtx;
	for(UINT uFace = 0; uFace < uFaceCount; ++uFace)
	{
		UINT p[] = {
			pIndices[uFace * 3 + 0],
			pIndices[uFace * 3 + 1],
			pIndices[uFace * 3 + 2]
		};

		vBaseNormal = SMVector3Normalize(SMVector3Cross(pVertices[p[0]].Pos - pVertices[p[1]].Pos, pVertices[p[0]].Pos - pVertices[p[2]].Pos));

		CalcTangentBasis(pVertices[p[0]].Pos, pVertices[p[1]].Pos, pVertices[p[2]].Pos, pVertices[p[0]].Tex, pVertices[p[1]].Tex, pVertices[p[2]].Tex, &vBaseTangent, &vBaseBinormal);

		for(UINT pt = 0; pt < 3; ++pt)
		{
			vtx = pVertices[p[pt]];

			SMQuaternion q(vBaseNormal, vtx.Norm);
			vtx.Tangent = q * vBaseTangent;
			vtx.Binorm = q * vBaseBinormal;
			if(SMVector3Length(pVertices[p[pt]].Tangent) < 0.1f)
			{
				pVertices[p[pt]] = vtx;
			}
			else if(memcmp(&pVertices[p[pt]], &vtx, sizeof(vtx)))
			{
				pIndices[uFace * 3 + pt] = aNewVertices.size() + uVertexCount;
				aNewVertices.push_back(vtx);
			}
		}
	}

	return(aNewVertices);
}

template<typename T>
static bool GetBoneId(T *pVertices, UINT uVertexCount, int *pOut)
{
	byte u8Bone = 255;
	for(UINT s = 0; s < uVertexCount; ++s)
	{
		for(UINT t = 0; t < 4; ++t)
		{
			if(pVertices[s].BoneIndices[t] != 255)
			{
				if(u8Bone == 255)
				{
					u8Bone = pVertices[s].BoneIndices[t];
				}
				else if(u8Bone != pVertices[s].BoneIndices[t])
				{
					return(false);
				}
			}
		}
	}

	if(pOut)
	{
		*pOut = u8Bone == 255 ? -1 : (int)u8Bone;
	}

	return(true);
}

void CExporter::preparePhysMesh(bool bIgnoreLayers, bool bHitbox)
{
	if(bHitbox)
	{
		clearHitboxes();
	}
	else
	{
		clearPhysparts();
	}

	bool isStatic = IsDlgButtonChecked(m_hDlgWnd, IDC_MESH_STATIC) == BST_CHECKED;
	bool includeTB = IsDlgButtonChecked(m_hDlgWnd, IDC_MESH_TBN) == BST_CHECKED;
	bool isTBSupported = m_pCallback->canExportTB();

	UINT uLayerCount = m_pCallback->getLayerCount();

	for(UINT i = 0; i < uLayerCount; ++i)
	{
		const char *szLayer = m_pCallback->getLayerName(i);
		UINT uObjectCount = m_pCallback->getLayerObjectCount(i);

		if(bIgnoreLayers || !strcmp(szLayer, bHitbox ? "#hitbox" : "#physbox"))
		{
			// process physbox
			for(UINT j = 0; j < uObjectCount; ++j)
			{
				// merge all subsets of object

				UINT uVertexCount = 0;

				UINT uSubsetCount = m_pCallback->getObjectSubsetCount(i, j);
				for(UINT k = 0; k < uSubsetCount; ++k)
				{
					uVertexCount += m_pCallback->getObjectSubsetVertexCount(i, j, k);
				}
				Array<float3_t> aVertices(uVertexCount);

				for(UINT k = 0; k < uSubsetCount; ++k)
				{
					model_vertex *pVertices = NULL;
					size_t uStride = 0;
					if(isStatic)
					{
						if(includeTB && isTBSupported)
						{
							pVertices = m_pCallback->getObjectSubsetStaticExVertices(i, j, k);
							uStride = sizeof(vertex_static_ex);
						}
						else
						{
							pVertices = m_pCallback->getObjectSubsetStaticVertices(i, j, k);
							uStride = sizeof(vertex_static);
						}
					}
					else
					{
						if(includeTB && isTBSupported)
						{
							pVertices = m_pCallback->getObjectSubsetAnimatedExVertices(i, j, k);
							uStride = sizeof(vertex_animated_ex);
						}
						else
						{
							pVertices = m_pCallback->getObjectSubsetAnimatedVertices(i, j, k);
							uStride = sizeof(vertex_animated);
						}
					}

					for(UINT s = 0, sl = m_pCallback->getObjectSubsetVertexCount(i, j, k); s < sl; ++s)
					{
						float3 pt = pVertices->Pos * m_fScale;
						if(aVertices.indexOf(pt, [](const float3 &a, const float3 &b){
							return(fabsf(SMVector3Length2(a - b)) < 0.0001f);
						}) < 0)
						{
							aVertices.push_back(pt);
						}
						
						MOVE_PTR(pVertices, uStride);
					}
				}

				{
					float3_t vCenter;
					float3_t vLWH;

					const char *szName = m_pCallback->getObjectName(i, j);

					ModelPhyspart physPart = {};

					for(UINT k = 0; k < MODEL_MAX_NAME; ++k)
					{
						if(szName[k] && szName[k] != ':')
						{
							physPart.name[k] = szName[k];
						}
						else
						{
							physPart.name[k] = 0;

							if(szName[k] == ':')
							{
								szName += k + 1;
							}

							break;
						}
					}
					physPart.name[MODEL_MAX_NAME - 1] = 0;

					if(CGeomDetector::IsBox(aVertices, aVertices.size(), sizeof(float3_t), &vCenter, &vLWH))
					{ // HT_BOX
						physPart.type = HT_BOX;
						physPart.lwh = vLWH;
						physPart.pos = vCenter; // + world mat
					}
					else if(CGeomDetector::IsSphere(aVertices, aVertices.size(), sizeof(float3_t), &vCenter, &vLWH))
					{ // HT_ELIPSOID
						physPart.type = HT_ELIPSOID;
						physPart.lwh = vLWH;
						physPart.pos = vCenter; // + world mat
					}
					else // HT_CONVEX
					{
						physPart.type = HT_CONVEX;
						ModelPhyspartDataConvex *pDataConvex = new ModelPhyspartDataConvex();
						pDataConvex->iVertCount = aVertices.size();
						pDataConvex->pVerts = new float3_t[pDataConvex->iVertCount];

						for(UINT k = 0; k < pDataConvex->iVertCount; ++k)
						{
							pDataConvex->pVerts[k] = aVertices[k];
						}
						physPart.rot.w = 1.0f;
						physPart.pData = pDataConvex;
					}

					if(physPart.type != HT_CONVEX)
					{
						SMQuaternion q = m_pCallback->getObjectRotation(i, j);
						physPart.rot = float4(q.x, q.y, q.z, q.w);
						//TODO Check that
						physPart.pos = physPart.pos + m_pCallback->getObjectPosition(i, j);
					}

					if(isStatic)
					{
						physPart.bone_id = -1;
					}
					else
					{
						physPart.bone_id = -2;
						for(UINT k = 0; k < uSubsetCount; ++k)
						{
							bool isSuccess = false;
							int iBone = -1;
							if(isTBSupported && includeTB)
							{
								isSuccess = GetBoneId(m_pCallback->getObjectSubsetAnimatedExVertices(i, j, k), m_pCallback->getObjectSubsetVertexCount(i, j, k), &iBone);
							}
							else
							{
								isSuccess = GetBoneId(m_pCallback->getObjectSubsetAnimatedVertices(i, j, k), m_pCallback->getObjectSubsetVertexCount(i, j, k), &iBone);
							}

							if(physPart.bone_id == -2)
							{
								physPart.bone_id = iBone;
							}
							else if(physPart.bone_id != iBone)
							{
								isSuccess = false;
							}

							if(!isSuccess)
							{
								if(!m_bSuppressPrompts)
								{
									logWarning("Object: %s. All vertices of physbox should be skinned to the same bone", m_pCallback->getObjectName(i, j));
								}
								break;
							}
						}
						physPart.bone_id = getNewBoneId(physPart.bone_id);
					}

					if(physPart.bone_id >= 0)
					{
						strcpy(physPart.bone, m_aBones[physPart.bone_id].szName);
					}
					else
					{
						physPart.bone[0] = 0;
					}

					if(bHitbox)
					{
						ModelHitboxEx &hbex = m_aHitboxesEx[m_aHitboxesEx.size()];
						hbex.hitbox = physPart;
						hbex.part = HBP_DEFAULT;
						if(szName)
						{
							const auto &reflector = EnumReflector::Get<HITBOX_BODYPART>();
							auto val = reflector.find(szName);
							if(val.isValid())
							{
								hbex.part = (HITBOX_BODYPART)val.getValue();
							}
						}
					}
					else
					{
						m_aPhysParts.push_back(physPart);
					}
				}
			}
		}
	}
}

void CExporter::prepareLodMesh(int iTargetLod)
{
	clearLods(iTargetLod);

	bool isStatic = IsDlgButtonChecked(m_hDlgWnd, IDC_MESH_STATIC) == BST_CHECKED;
	bool includeTB = IsDlgButtonChecked(m_hDlgWnd, IDC_MESH_TBN) == BST_CHECKED;
	bool isTBSupported = m_pCallback->canExportTB();

	UINT uLayerCount = m_pCallback->getLayerCount();

	struct SubsetItem
	{
		UINT uVertexCount;
		UINT uIndexCount;
		model_vertex *pVertices;
		UINT *pIndices;
	};

	struct Subset
	{
		const char *szTexture;
		Array<SubsetItem> aItems;
	};

	Array<Array<Subset>> aLods; // [iLod][iSubset].aItems[]
	
	for(UINT i = 0; i < uLayerCount; ++i)
	{
		if(m_iOnlyLayer >= 0 && i != m_iOnlyLayer)
		{
			continue;
		}
		const char *szLayer = m_pCallback->getLayerName(i);

		if(m_szOnlyLayer && fstrcmp(szLayer, m_szOnlyLayer))
		{
			continue;
		}

		UINT uObjectCount = m_pCallback->getLayerObjectCount(i);

		int iLod = 0;
		if(iTargetLod < 0 && szLayer[0] == '#' && !sscanf(szLayer, "#lod%d", &iLod))
		{
			//logWarning("Unknown layer type '%s', ignoring!\n", szLayer);
			continue;
		}

		if(iLod < 0)
		{
			logWarning("Invalid LOD index '%d' < 0\n", iLod);
			continue;
		}

		// process lod
		for(UINT j = 0; j < uObjectCount; ++j)
		{
			if(m_iOnlyObject >= 0 && j != m_iOnlyObject)
			{
				continue;
			}

			UINT uSubsets = m_pCallback->getObjectSubsetCount(i, j);
			for(UINT k = 0; k < uSubsets; ++k)
			{
				const char *szTexture = m_pCallback->getObjectSubsetTexture(i, j, k);
				Array<Subset> &aSubsets = aLods[iLod];

				UINT uSubsetIdx = aSubsets.size();
				for(UINT s = 0; s < uSubsetIdx; ++s)
				{
					if(!fstrcmp(aSubsets[s].szTexture, szTexture))
					{
						uSubsetIdx = s;
					}
				}
				Subset &ss = aSubsets[uSubsetIdx];
				ss.szTexture = szTexture;

				SubsetItem ssi = {};
				ssi.uVertexCount = m_pCallback->getObjectSubsetVertexCount(i, j, k);
				ssi.uIndexCount = m_pCallback->getObjectSubsetIndexCount(i, j, k);
				if(isStatic)
				{
					if(includeTB && isTBSupported)
					{
						ssi.pVertices = m_pCallback->getObjectSubsetStaticExVertices(i, j, k);
					}
					else
					{
						ssi.pVertices = m_pCallback->getObjectSubsetStaticVertices(i, j, k);
					}
				}
				else
				{
					if(includeTB && isTBSupported)
					{
						ssi.pVertices = m_pCallback->getObjectSubsetAnimatedExVertices(i, j, k);
					}
					else
					{
						ssi.pVertices = m_pCallback->getObjectSubsetAnimatedVertices(i, j, k);
					}
				}
				ssi.pIndices = m_pCallback->getObjectSubsetIndices(i, j, k);
				ss.aItems.push_back(ssi);
			}
		}
	}

	for(UINT i = 0, l = aLods.size(); i < l; ++i)
	{
		auto &aSubsets = aLods[i];
		ModelLoD lod;
		lod.iSubMeshCount = aSubsets.size();
		lod.pSubLODmeshes = new ModelLoDSubset[lod.iSubMeshCount];

		for(UINT j = 0, jl = aSubsets.size(); j < jl; ++j)
		{
			Subset &ss = aSubsets[j];

			UINT uVertexCount = 0;
			UINT uIndexCount = 0;
			for(UINT k = 0, kl = ss.aItems.size(); k < kl; ++k)
			{
				uVertexCount += ss.aItems[k].uVertexCount;
				uIndexCount += ss.aItems[k].uIndexCount;
			}

			UINT *pIndices = new UINT[uIndexCount];
			model_vertex *pVertices = NULL;

			size_t sizeSrcEl;
			size_t sizeDstEl;
			if(isStatic)
			{
				if(includeTB)
				{
					pVertices = new vertex_static_ex[uVertexCount];
					sizeDstEl = sizeof(vertex_static_ex);
					sizeSrcEl = isTBSupported ? sizeof(vertex_static_ex) : sizeof(vertex_static);
				}
				else
				{
					pVertices = new vertex_static[uVertexCount];
					sizeDstEl = sizeof(vertex_static);
					sizeSrcEl = sizeof(vertex_static);
				}
			}
			else
			{
				if(includeTB)
				{
					pVertices = new vertex_animated_ex[uVertexCount];
					sizeDstEl = sizeof(vertex_animated_ex);
					sizeSrcEl = isTBSupported ? sizeof(vertex_animated_ex) : sizeof(vertex_animated);
				}
				else
				{
					pVertices = new vertex_animated[uVertexCount];
					sizeDstEl = sizeof(vertex_animated);
					sizeSrcEl = sizeof(vertex_animated);
				}
			}

			UINT *pIdx = pIndices;
			model_vertex *pDstVtx = pVertices;
			UINT uVtxOffset = 0;
			for(UINT k = 0, kl = ss.aItems.size(); k < kl; ++k)
			{
				auto &item = ss.aItems[k];

				for(UINT s = 0; s < item.uIndexCount; ++s)
				{
					pIdx[s] = item.pIndices[s] + uVtxOffset;
				}

				model_vertex *pSrcVtx = item.pVertices;

				for(UINT s = 0; s < item.uVertexCount; ++s)
				{
					pSrcVtx->Pos = (float3)(pSrcVtx->Pos * m_fScale);
					pSrcVtx->Norm = SMVector3Normalize(pSrcVtx->Norm);

					memcpy(pDstVtx, pSrcVtx, sizeSrcEl);
					memset(MovePtr(pDstVtx, sizeSrcEl), 0, sizeDstEl - sizeSrcEl);

					if(!isStatic && includeTB && !isTBSupported)
					{
						for(UINT t = 0; t < 4; ++t)
						{
							((vertex_animated_ex*)pDstVtx)->BoneIndices[t] = ((vertex_animated*)pSrcVtx)->BoneIndices[t];
						}
						((vertex_animated_ex*)pDstVtx)->BoneWeights = ((vertex_animated*)pSrcVtx)->BoneWeights;
					}

					MOVE_PTR(pSrcVtx, sizeSrcEl);
					MOVE_PTR(pDstVtx, sizeDstEl);
				}

				pIdx += item.uIndexCount;
				uVtxOffset += item.uVertexCount;
			}

			if(includeTB && !isTBSupported)
			{
				// generate TBN
				if(isStatic)
				{
					Array<vertex_static_ex> aNewVtx = GenerateTBN((vertex_static_ex*)pVertices, pIndices, uVertexCount, uIndexCount / 3);
					if(aNewVtx.size())
					{
						vertex_static_ex *pNewVtx = new vertex_static_ex[uVertexCount + aNewVtx.size()];
						memcpy(pNewVtx, pVertices, sizeof(vertex_static_ex) * uVertexCount);
						memcpy(pNewVtx + uVertexCount, aNewVtx, sizeof(vertex_static_ex) * aNewVtx.size());
						mem_delete_a(pVertices);
						pVertices = pNewVtx;
						uVertexCount += aNewVtx.size();
					}
				}
				else
				{
					Array<vertex_animated_ex> aNewVtx = GenerateTBN((vertex_animated_ex*)pVertices, pIndices, uVertexCount, uIndexCount / 3);
					if(aNewVtx.size())
					{
						vertex_animated_ex *pNewVtx = new vertex_animated_ex[uVertexCount + aNewVtx.size()];
						memcpy(pNewVtx, pVertices, sizeof(vertex_animated_ex) * uVertexCount);
						memcpy(pNewVtx + uVertexCount, aNewVtx, sizeof(vertex_animated_ex) * aNewVtx.size());
						mem_delete_a(pVertices);
						pVertices = pNewVtx;
						uVertexCount += aNewVtx.size();
					}
				}
			}

			if(!isStatic)
			{
				if(includeTB)
				{
					fixSubsetBoneIds((vertex_animated_ex*)pVertices, uVertexCount);
				}
				else
				{
					fixSubsetBoneIds((vertex_animated*)pVertices, uVertexCount);
				}
			}

			lod.pSubLODmeshes[j].iVectexCount = uVertexCount;
			lod.pSubLODmeshes[j].iIndexCount = uIndexCount;
			lod.pSubLODmeshes[j].iMaterialID = addTexture(ss.szTexture);
			// TODO check this values!
			lod.pSubLODmeshes[j].iStartIndex = 0;
			lod.pSubLODmeshes[j].iStartVertex = 0;
			lod.pSubLODmeshes[j].pVertices = pVertices;
			lod.pSubLODmeshes[j].pIndices = pIndices;
		}

		if(iTargetLod < 0)
		{
			m_aLods.push_back(lod);
		}
		else
		{
			m_aLods[iTargetLod] = lod;
			break;
		}
	}
}

bool CExporter::prepareSkeleton(bool bOverwriteBindPose)
{
	if(m_isSkeletonReady)
	{
		return(true);
	}

	struct MetaBone
	{
		int idParent;
		UINT uProviderBoneId;
	};
	UINT uBonesCount = m_pCallback->getBonesCount();
	Array<MetaBone> aBones(uBonesCount);

	for(UINT i = 0; i < uBonesCount; ++i)
	{
		int idParent = m_pCallback->getBoneParent(i);
		aBones.push_back({idParent, i});
	}

	aBones.quickSort([](const MetaBone &a, const MetaBone &b){
		return(a.idParent < b.idParent);
	});
	
	for(UINT i = 0; i < uBonesCount; ++i)
	{
		if(!addBone(aBones[i].idParent, aBones[i].uProviderBoneId, bOverwriteBindPose))
		{
			return(false);
		}
	}

	m_isSkeletonReady = true;
	return(true);
}

bool CExporter::addBone(int iProviderParent, UINT uProviderId, bool bOverwriteBindPose)
{
	for(UINT i = m_aSkeletonRemap.size(); i <= uProviderId; ++i)
	{
		m_aSkeletonRemap[i] = -1;
	}

	int iParent = -1;
	if(iProviderParent >= 0)
	{
		assert(m_aSkeletonRemap.size() > (UINT)iProviderParent && m_aSkeletonRemap[iProviderParent] >= 0);
		iParent = m_aSkeletonRemap[iProviderParent];
	}

	const char *szBoneName = m_pCallback->getBoneName(uProviderId);
	{
		int iLastColonPos = -1;
		for(int i = 0; szBoneName[i]; ++i)
		{
			if(szBoneName[i] == ':' && szBoneName[i + 1])
			{
				iLastColonPos = i;
			}
		}
		if(iLastColonPos >= 0)
		{
			szBoneName += iLastColonPos + 1;
		}
	}

	for(UINT i = 0, l = m_aBones.size(); i < l; ++i)
	{
		if(!strcmp(m_aBones[i].szName, szBoneName))
		{
			if(m_aBones[i].bone.pid == iParent)
			{
				m_aSkeletonRemap[uProviderId] = i;
				for(UINT j = m_aSkeletonRemapInv.size(); j < i; ++j)
				{
					m_aSkeletonRemapInv[j] = -1;
				}
				m_aSkeletonRemapInv[i] = uProviderId;

				if(bOverwriteBindPose)
				{
					m_aBones[i].bone.position = (float3)(m_pCallback->getBoneLocalPos(uProviderId) * m_fScale);
					m_aBones[i].bone.orient = m_pCallback->getBoneLocalRot(uProviderId);
				}
				return(true);
			}
			return(false);
		}
	}

	ModelBoneName newBone = {};
	strncpy(newBone.szName, szBoneName, MODEL_MAX_NAME);
	newBone.szName[MODEL_MAX_NAME - 1] = 0;
	newBone.bone.id = m_aSkeletonRemap[uProviderId] = m_aBones.size();
	m_aSkeletonRemapInv[newBone.bone.id] = uProviderId;
	newBone.bone.pid = iParent;
	newBone.bone.position = (float3)(m_pCallback->getBoneLocalPos(uProviderId) * m_fScale);
	newBone.bone.orient = m_pCallback->getBoneLocalRot(uProviderId);
	m_aBones.push_back(newBone);

	return(true);
}

int CExporter::getNewBoneId(int iProviderId)
{
	if(iProviderId >= 0)
	{
		assert(m_aSkeletonRemap.size() > (UINT)iProviderId && m_aSkeletonRemap[iProviderId] >= 0);
		return(m_aSkeletonRemap[iProviderId]);
	}
	return(-1);
}

int CExporter::getOldBoneId(int iId)
{
	if(iId >= 0)
	{
		assert(m_aSkeletonRemapInv.size() > (UINT)iId/* && m_aSkeletonRemapInv[iId] >= 0*/);
		return(m_aSkeletonRemapInv[iId]);
	}
	return(-1);
}

bool CExporter::invokeEditor(HWND hWnd, LPCTSTR szDefault, LPCTSTR szTitle)
{
	m_szEditDefault = szDefault;
	m_szEditTitle = szTitle;
	return(DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG3), hWnd, EditDlgProc, (LPARAM)this) != 0);
}

INT_PTR CALLBACK CExporter::EditDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//CSkinEditor *exp = DLGetWindowLongPtr<CSkinEditor*>(hWnd);
	CExporter *exp = (CExporter*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	switch(msg)
	{
	case WM_INITDIALOG:
		exp = (CExporter*)lParam;
		//CenterWindow(hWnd, GetParent(hWnd));
		//DLSetWindowLongPtr(hWnd, lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
		//IDC_CUSTOM1
		//SetWindowText(GetDlgItem(hWnd, IDC_EDIT1), exp->m_szEditDefault);
		SetWindowText(GetDlgItem(hWnd, IDC_PROMPT2), "");
		SetWindowText(GetDlgItem(hWnd, IDC_EDIT_PROMPT), exp->m_szEditTitle);
		{
			HWND ed1 = GetDlgItem(hWnd, IDC_CUSTOM1);
			if(ed1)
			{
				SetWindowText(ed1, exp->m_szEditDefault);
				//ed1->SetText(exp->m_szEditDefault);
				//ed1->GiveFocus();
				//ReleaseICustEdit(ed1);
				SetFocus(ed1);
			}
		}
		//SetFocus(GetDlgItem(hWnd, IDC_CUSTOM1));
		SendMessage(GetDlgItem(hWnd, IDC_CUSTOM1), EM_SETSEL, 0, -1);
		return(FALSE);

	case WM_COMMAND:
		//SendMessage(exp->hProgressBar, PBM_SETPOS, 429, 0);
		switch(LOWORD(wParam))
		{
		case IDOK:
		{
			// commit changes
			HWND ed1 = GetDlgItem(hWnd, IDC_CUSTOM1);
			if(ed1)
			{
				GetWindowText(ed1, (LPTSTR)exp->m_szEditDefault, 127);
				//ed1->GetText((LPTSTR)exp->m_szEditDefault, 127);
				//ReleaseICustEdit(ed1);
			}
			//GetWindowText(GetDlgItem(hWnd, IDC_EDIT1), (LPWSTR)exp->m_szEditDefault, 127);
			EndDialog(hWnd, 1);
		}
		break;
		case IDCANCEL:
			EndDialog(hWnd, 0);
			break;
		}
		break;
	default:
		return(FALSE);
	}
	return(TRUE);
}

void CExporter::prepareAnimation()
{
	ModelSequence ms;

	ms.iNumFrames = m_uEndFrame - m_uStartFrame + 1;
	ms.m_vmAnimData = new ModelBone*[ms.iNumFrames];
	for(UINT j = 0; j < ms.iNumFrames; ++j)
	{
		ms.m_vmAnimData[j] = new ModelBone[m_aBones.size()];

		for(UINT i = 0, l = m_aBones.size(); i < l; ++i)
		{
			ms.m_vmAnimData[j][i] = m_aBones[i].bone;
			
			int iOldBoneId = getOldBoneId(i);

			if(iOldBoneId >= 0)
			{
				ms.m_vmAnimData[j][i].position = (float3)(m_pCallback->getBonePositionAtFrame(iOldBoneId, j) * m_fScale);
				ms.m_vmAnimData[j][i].orient = m_pCallback->getBoneRotationAtFrame(iOldBoneId, j);
			}
			else
			{
				ms.m_vmAnimData[j][i].position = m_aBones[i].bone.position;
				ms.m_vmAnimData[j][i].orient = m_aBones[i].bone.orient;
			}
		}
	}
	ms.act_chance = m_uActivityChance;
	ms.bLooped = m_isLooped;
	ms.framerate = m_iFramerate;
	strcpy(ms.name, m_szAnimationName);

	auto &aActivities = m_pExtActs->getList();
	ms.activity = aActivities.indexOf(m_szAnimationActivity, [](const String &a, const char *b){
		return(!strcmp(a.c_str(), b));
	}) + 1;

	m_pExtAnim->addSequence(ms);
}

//##########################################################################

CExporter::CProgress::CProgress(HWND hProgressBar):
	m_hProgressBar(hProgressBar)
{
	SendMessage(m_hProgressBar, PBM_SETRANGE, 0, (LPARAM)MAKELONG(0, mc_iTopRange));
	SendMessage(m_hProgressBar, PBM_SETSTEP, (WPARAM)1, 0);
}

void CExporter::CProgress::setProgress(float fProgress)
{
	SendMessage(m_hProgressBar, PBM_SETPOS, (WPARAM)((float)mc_iTopRange * lerpf(m_fMinRange, m_fMaxRange, fProgress)), 0);
}

void CExporter::CProgress::setRange(float fMin, float fMax)
{
	assert(fMin < fMax);

	m_fMinRange = fMin;
	m_fMaxRange = fMax;
}

void CExporter::CProgress::setRange(float fMax)
{
	setRange(m_fMaxRange, fMax);
}
