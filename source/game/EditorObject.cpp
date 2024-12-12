#include "EditorObject.h"

//#include <render/sxrender.h>
#include <xcommon/resource/IXResourceManager.h>

#include "Editable.h"

CEditorObject::CEditorObject(CEditable *pEditable, CBaseEntity *pEntity):
	m_pEditable(pEditable),
	m_pEntity(pEntity),
	m_guidEnt(*pEntity->getGUID())
{
	assert(pEntity);

	m_szClassName = pEntity->getClassName();

	_iniFieldList();

	m_vPos = pEntity->getPos();
	m_qRot = pEntity->getOrient();
}

CEditorObject::CEditorObject(CEditable *pEditable, const char *szClassName):
	m_pEditable(pEditable)
{
	m_szClassName = CEntityFactoryMap::GetInstance()->getClassNamePtr(szClassName);
	assert(m_szClassName);

	XCreateGUID(&m_guidEnt);

	_iniFieldList();
}

CEditorObject::~CEditorObject()
{
	m_pEditable->removeObject(this);
	mem_release(m_pIcon);
	mem_release(m_pModel);
}

void CEditorObject::_iniFieldList()
{
	proptable_t *pTable = CEntityFactoryMap::GetInstance()->getPropTable(m_szClassName);
	propdata_t *pField = NULL;
	X_PROP_FIELD xField = {};

	for(UINT i = 0; i < 16; ++i)
	{
		m_aszFlags[i] = NULL;
	}

	{
		xField.editorType = XPET_TEXT; 
		xField.szHelp = "";
		xField.szKey = "guid";
		xField.szName = "GUID";
		xField.isGeneric = true;

		m_aFields.push_back(xField);
	}

	while(pTable)
	{
		for(int i = 0; i < pTable->numFields; ++i)
		{
			pField = &pTable->pData[i];
			if(
				!(pField->flags & (PDFF_INPUT | PDFF_MESSAGE | PDFF_NOEDIT | PDFF_OUTPUT))
				&& pField->szKey 
				&& pField->szEdName
				&& pField->editor.type != PDE_NONE
				)
			{
				switch(pField->editor.type)
				{
				case PDE_COMBOBOX:
					xField.editorType = XPET_COMBO;
					xField.pEditorData = pField->editor.pData;
					break;
				case PDE_FILEFIELD:
					xField.editorType = XPET_FILE;
					xField.pEditorData = NULL;
					{
						editor_kv *pKV = (editor_kv*)pField->editor.pData;
						if(pKV && pKV[0].value)
						{
							if(!fstrcmp(pKV[0].value, "dse"))
							{
								xField.pEditorData = "model";
							}
							else if(!fstrcmp(pKV[0].value, "ogg"))
							{
								xField.pEditorData = "sound";
							}
							else if(!fstrcmp(pKV[0].value, "dds"))
							{
								xField.pEditorData = "texture";
							}
							else if(!fstrcmp(pKV[0].value, "eff"))
							{
								xField.pEditorData = "effect";
							}
						}
						//xField.pEditorData
					}
					break;
				case PDE_FLAGS:
					xField.editorType = XPET_FLAGS;
					xField.pEditorData = m_aszFlags;
					break;
				case PDE_TEXTFIELD:
					xField.editorType = XPET_TEXT; 
					break;
				case PDE_TIME:
					xField.editorType = XPET_TIME;
					break;
				case PDE_ANGLES:
					xField.editorType = XPET_ANGLES;
					break;
				case PDE_POINTCOORD:
					xField.editorType = XPET_POINTCOORD;
					break;
				case PDE_RADIUS:
					xField.editorType = XPET_RADIUS;
					break;
				case PDE_COLOR:
					xField.editorType = XPET_COLOR;
					break;
				case PDE_HDRCOLOR:
					xField.editorType = XPET_HDRCOLOR;
					break;
				}
				xField.szHelp = "";
				xField.szKey = pField->szKey;
				xField.szName = pField->szEdName;
				xField.isGeneric = !fstrcmp(pField->szKey, "origin") || !fstrcmp(pField->szKey, "rotation")/* || !fstrcmp(pField->szKey, "scale")*/;
				xField.useGizmo = pField->flags & PDFF_USE_GIZMO;

				m_aFields.push_back(xField);
			}

			if(pField->type == PDF_FLAG && pField->flags)
			{
				UINT uFlag = (UINT)pField->flags >> 16;
				uFlag = (UINT)lroundf(log2f((float)uFlag));
				assert(uFlag < 16);

				if(!m_aszFlags[uFlag])
				{
					m_aszFlags[uFlag] = pField->szEdName;
				}
			}
		}

		pTable = pTable->pBaseProptable;
	}


	IEntityFactory *pFactory = CEntityFactoryMap::GetInstance()->getFactory(m_szClassName);
	if(pFactory)
	{
		const char *szIcon = pFactory->getKV("icon");
		if(szIcon)
		{
			IXMaterialSystem *pMatSys = (IXMaterialSystem*)Core_GetIXCore()->getPluginManager()->getInterface(IXMATERIALSYSTEM_GUID);
			if(pMatSys)
			{
				pMatSys->loadTexture(szIcon, &m_pIcon);
			}
		}

		const char *szModel = pFactory->getKV("model");
		if(szModel)
		{
			IXResourceManager *pResourceManager = Core_GetIXCore()->getResourceManager();
			IXModelProvider *pProvider = (IXModelProvider*)Core_GetIXCore()->getPluginManager()->getInterface(IXMODELPROVIDER_GUID);

			IXResourceModel *pResource;
			if(pResourceManager->getModel(szModel, &pResource))
			{
				IXDynamicModel *pModel;
				if(pProvider->createDynamicModel(pResource, &pModel))
				{
					m_pModel = pModel;
					m_pModel->setPosition(getPos());
					m_pModel->setOrientation(getOrient());
					m_pModel->setColor(float4(0.71647654f, 0.013f, 0.71647654f, 1.0f));
				}
				mem_release(pResource);
			}
		}

		//m_pModel
	}
}

void XMETHODCALLTYPE CEditorObject::setPos(const float3_t &pos)
{
	SAFE_CALL(m_pEntity, setPos, pos);
	SAFE_CALL(m_pModel, setPosition, pos);
	m_vPos = pos;
}

void CEditorObject::setPos(const float3_t &pos, bool isSeparate)
{
	if(isSeparate)
	{
		SAFE_CALL(m_pEntity, setSeparateMovement, true);
	}

	setPos(pos);

	if(isSeparate)
	{
		SAFE_CALL(m_pEntity, setSeparateMovement, false);
	}
}

void XMETHODCALLTYPE CEditorObject::setSize(const float3_t &vSize)
{
	// TODO Implement me
	//m_vScale = vScale;
}

/*void CEditorObject::setScale(const float3_t &vScale, bool isSeparate)
{
	if(isSeparate)
	{
		SAFE_CALL(m_pEntity, setSeparateMovement, true);
	}

	setScale(vScale);

	if(isSeparate)
	{
		SAFE_CALL(m_pEntity, setSeparateMovement, false);
	}
}*/

void XMETHODCALLTYPE CEditorObject::setOrient(const SMQuaternion &orient)
{
	SAFE_CALL(m_pEntity, setOrient, orient);
	SAFE_CALL(m_pModel, setOrientation, orient);

	m_qRot = orient;
}

void CEditorObject::setOrient(const SMQuaternion &orient, bool isSeparate)
{
	if(isSeparate)
	{
		SAFE_CALL(m_pEntity, setSeparateMovement, true);
	}

	setOrient(orient);

	if(isSeparate)
	{
		SAFE_CALL(m_pEntity, setSeparateMovement, false);
	}
}

float3_t XMETHODCALLTYPE CEditorObject::getPos()
{
	if(m_pEntity)
	{
		m_vPos = m_pEntity->getPos();
		SAFE_CALL(m_pModel, setPosition, m_vPos);
	}
	return(m_vPos);
}

SMQuaternion XMETHODCALLTYPE CEditorObject::getOrient()
{
	if(m_pEntity)
	{
		m_qRot = m_pEntity->getOrient();
		SAFE_CALL(m_pModel, setOrientation, m_qRot);
	}
	return(m_qRot);
}

void XMETHODCALLTYPE CEditorObject::getBound(float3 *pvMin, float3 *pvMax)
{
	*pvMin = *pvMax = float3();

	SAFE_CALL(m_pEntity, getMinMax, pvMin, pvMax);

	if(SMVector3Length2(*pvMax - *pvMin) < 0.0001f)
	{
		if(m_pModel)
		{
			*pvMin = m_pModel->getLocalBoundMin();
			*pvMax = m_pModel->getLocalBoundMax();
		}
		else
		{
			*pvMin = -(*pvMax = float3(0.1f));
		}
	}

	*pvMin += m_vPos;
	*pvMax += m_vPos;
}

void XMETHODCALLTYPE CEditorObject::render(bool is3D, bool bRenderSelection, IXGizmoRenderer *pGizmoRenderer)
{
	if(!m_pEntity)
	{
		return;
	}

	if(bRenderSelection)
	{
		IGXDevice *pDevice = m_pEditable->getDevice();
		IGXContext *pCtx = pDevice->getThreadContext();

		IGXBlendState *pOldBlendState = pCtx->getBlendState();
//		IGXRasterizerState *pOldRS = pCtx->getRasterizerState();

		m_pEditable->m_pMaterialSystem->bindTexture(m_pEditable->m_pWhiteTexture);
		//pDevice->setTexture(m_pEditable->m_pWhiteTexture);
		pCtx->setBlendState(m_pEditable->m_pBlendColorFactor);

		if(is3D)
		{
			pCtx->setBlendFactor(GX_COLOR_ARGB(70, 255, 0, 0));
			m_pEntity->renderEditor(is3D, bRenderSelection, NULL);

			SAFE_CALL(m_pModel, render, 0, MF_OPAQUE | MF_TRANSPARENT);
		}

		GXFILL_MODE oldFillMode = m_pEditable->m_pMaterialSystem->getFillMode();
		m_pEditable->m_pMaterialSystem->setFillMode(GXFILL_WIREFRAME);

//		pCtx->setRasterizerState(m_pEditable->m_pRSWireframe);
		pCtx->setBlendFactor(GX_COLOR_ARGB(255, 255, 255, 0));
		m_pEntity->renderEditor(is3D, bRenderSelection, pGizmoRenderer);
		SAFE_CALL(m_pModel, render, 0, MF_OPAQUE | MF_TRANSPARENT);

		m_pEditable->m_pMaterialSystem->setFillMode(oldFillMode);
		pCtx->setBlendState(pOldBlendState);
//		pCtx->setRasterizerState(pOldRS);
		mem_release(pOldBlendState);
//		mem_release(pOldRS);
	}
	else
	{
		m_pEntity->renderEditor(is3D, bRenderSelection, pGizmoRenderer);
	}
}

bool XMETHODCALLTYPE CEditorObject::rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut, float3 *pvNormal, ID *pidMtrl, bool bReturnNearestPoint)
{
	//SMAABBIN
	float3 vMin, vMax;
	getBound(&vMin, &vMax);
	if(SMRayIntersectAABB(SMAABB(vMin, vMax), vStart, vEnd - vStart))
	{
		if(m_pModel)
		{
			return(m_pModel->rayTest(vStart, vEnd, pvOut, pvNormal, true, bReturnNearestPoint));
		}
		else if(m_pEntity)
		{
			return(m_pEntity->rayTest(vStart, vEnd, pvOut, pvNormal, true, bReturnNearestPoint));
		}
	}
	
	return(false);
}

void XMETHODCALLTYPE CEditorObject::remove()
{
	if(!m_pEntity)
	{
		return;
	}
	REMOVE_ENTITY(m_pEntity);
	m_pEntity = NULL;
	SAFE_CALL(m_pModel, enable, false);
	//m_guidEnt = XGUID();
}
void XMETHODCALLTYPE CEditorObject::preSetup()
{
}
void XMETHODCALLTYPE CEditorObject::postSetup()
{
	
}

void XMETHODCALLTYPE CEditorObject::create()
{
	assert(!m_pEntity);

	if(m_guidEnt == XGUID())
	{
		m_pEntity = CREATE_ENTITY(m_szClassName, GameData::m_pMgr);
		m_guidEnt = *m_pEntity->getGUID();
	}
	else
	{
		m_pEntity = CEntityFactoryMap::GetInstance()->create(m_szClassName, GameData::m_pMgr, false, &m_guidEnt);
	}

	m_pEntity->setFlags(m_pEntity->getFlags() | EF_LEVEL | EF_EXPORT);

	setPos(m_vPos, true);
	setOrient(m_qRot, true);
	//setScale(m_vScale, true);

	SAFE_CALL(m_pModel, enable, true);
}

void CEditorObject::resync()
{
	if(m_pEntity)
	{
		m_pEntity = GameData::m_pMgr->getByGUID(m_guidEnt);
	}
}

void XMETHODCALLTYPE CEditorObject::setKV(const char *szKey, const char *szValue)
{
	if(m_pEntity)
	{
		m_pEntity->setKV(szKey, szValue);
	}
}
const char* XMETHODCALLTYPE CEditorObject::getKV(const char *szKey)
{
	if(!m_pEntity)
	{
		return(NULL);
	}

	char tmp[4096];

	if(!fstrcmp(szKey, "guid"))
	{
		XGUIDToSting(*m_pEntity->getGUID(), tmp, sizeof(tmp));
	}
	else
	{
		m_pEntity->getKV(szKey, tmp, sizeof(tmp));
	}

	m_msPropCache[szKey] = tmp;
	return(m_msPropCache[szKey].c_str());
}
const X_PROP_FIELD* XMETHODCALLTYPE CEditorObject::getPropertyMeta(UINT uKey)
{
	assert(uKey < m_aFields.size());

	return(&m_aFields[uKey]);
}
UINT XMETHODCALLTYPE CEditorObject::getProperyCount()
{
	return(m_aFields.size());
}

const char* XMETHODCALLTYPE CEditorObject::getTypeName()
{
	return(m_pEditable->getName());
}
const char* XMETHODCALLTYPE CEditorObject::getClassName()
{
	return(m_szClassName);
}

void XMETHODCALLTYPE CEditorObject::setSelected(bool set)
{
	m_isSelected = set;

	m_pEditable->onSelectionChanged(this);
}

void XMETHODCALLTYPE CEditorObject::setSimulationMode(bool set)
{
	SAFE_CALL(m_pModel, enable, !set);
}

bool XMETHODCALLTYPE CEditorObject::hasVisualModel()
{
	float3 vMin, vMax;

	SAFE_CALL(m_pEntity, getMinMax, &vMin, &vMax);

	return(SMVector3Length2(vMax - vMin) >= 0.0001f || m_pModel);
}
