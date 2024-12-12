#include "BrushCreatorBox.h"
#include "EditorExtension.h"
#include "EditorObject.h"
#include "Editable.h"
#include "CommandCreate.h"

//##########################################################################

CBrushCreatorBox::CBrushCreatorBox(CEditable *pEditable, IXEditor *pEditor, IXGizmoRenderer *pRenderer):
	m_pEditable(pEditable),
	m_pEditor(pEditor),
	m_pRenderer(pRenderer),
	m_handlerCallback(this)
{
	add_ref(pRenderer);

	m_pEditable->getCore()->getConsole()->registerCVar("csg_block_auto_create", false, "Finish block without ENTER");

	for(UINT i = 0, l = ARRAYSIZE(m_apHandlers); i < l; ++i)
	{
		pEditor->newGizmoHandle(&m_apHandlers[i]);
		m_apHandlers[i]->enable(false);
		m_apHandlers[i]->setCallback(&m_handlerCallback);
	}

	m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_Y]->lockInDir(float3(0.0f, 1.0f, 0.0f));
	m_apHandlers[GXCUBEMAP_FACE_POSITIVE_Y]->lockInDir(float3(0.0f, 1.0f, 0.0f));
	m_apHandlers[GXCUBEMAP_FACE_POSITIVE_X]->lockInDir(float3(1.0f, 0.0f, 0.0f));
	m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_X]->lockInDir(float3(1.0f, 0.0f, 0.0f));
	m_apHandlers[GXCUBEMAP_FACE_POSITIVE_Z]->lockInDir(float3(0.0f, 0.0f, 1.0f));
	m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_Z]->lockInDir(float3(0.0f, 0.0f, 1.0f));
}
CBrushCreatorBox::~CBrushCreatorBox()
{
	for(UINT i = 0, l = ARRAYSIZE(m_apHandlers); i < l; ++i)
	{
		mem_release(m_apHandlers[i]);
	}
	mem_release(m_pRenderer);
}

bool CBrushCreatorBox::isInitiated()
{
	return(m_isInitiated);
}
void CBrushCreatorBox::init(const float3 &vNormal)
{
	assert(!m_isInitiated);

	if(m_pEditor->getViewForWindow(m_pEditor->getState()->activeWindow) == X2D_NONE)
	{
		return;
	}

	m_fHeight = m_pEditor->getGridStep();
	if(m_fHeight <= 0.0f)
	{
		m_fHeight = 0.2f;
	}

	m_vUp = SMVector3Normalize(vNormal);
	m_isInitiated = true;
	m_isBoxBuilt = false;

	m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_Y]->lockInDir(m_vUp);
	m_apHandlers[GXCUBEMAP_FACE_POSITIVE_Y]->lockInDir(m_vUp);
	float3 vDir = SMQuaternion(float3(0.0f, 1.0f, 0.0f), m_vUp) * float3(1.0f, 0.0f, 0.0f);
	m_apHandlers[GXCUBEMAP_FACE_POSITIVE_X]->lockInDir(vDir);
	m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_X]->lockInDir(vDir);
	vDir = SMQuaternion(float3(0.0f, 1.0f, 0.0f), m_vUp) * float3(0.0f, 0.0f, 1.0f);
	m_apHandlers[GXCUBEMAP_FACE_POSITIVE_Z]->lockInDir(vDir);
	m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_Z]->lockInDir(vDir);
}

void CBrushCreatorBox::activate()
{
	if(m_isBoxBuilt && m_isInitiated)
	{
		for(UINT i = 0, l = ARRAYSIZE(m_apHandlers); i < l; ++i)
		{
			m_apHandlers[i]->enable(true);
		}
	}
}
void CBrushCreatorBox::deactivate()
{
	if(m_isBoxBuilt)
	{
		for(UINT i = 0, l = ARRAYSIZE(m_apHandlers); i < l; ++i)
		{
			m_apHandlers[i]->enable(false);
		}
	}
}

bool CBrushCreatorBox::onMouseDown(bool isPrimary)
{
	if(m_pEditor->getViewForWindow(m_pEditor->getState()->activeWindow) == X2D_NONE)
	{
		return(false);
	}
	
	if(m_isBoxBuilt)
	{
		return(false);
	}

	if(isPrimary)
	{
		m_isMouseDown = true;
		m_vPos1 = m_vPos0 = m_pEditor->getState()->vResolvedWorldMousePos;
		return(true);
	}

	return(false);
}
bool CBrushCreatorBox::onMouseMove()
{
	if(m_pEditor->getViewForWindow(m_pEditor->getState()->activeWindow) == X2D_NONE)
	{
		return(false);
	}

	if(m_isMouseDown)
	{
		m_vPos1 = m_pEditor->getState()->vResolvedWorldMousePos;

		return(true);
	}
	return(false);
}
void CBrushCreatorBox::onMouseUp(bool isPrimary)
{
	if(m_isMouseDown)
	{
		m_isMouseDown = false;

		m_vPos1 = m_pEditor->getState()->vResolvedWorldMousePos;

		if(!isValid())
		{
			m_isInitiated = false;
		}
		else
		{
			m_isBoxBuilt = true;

			static const bool *csg_block_auto_create = m_pEditable->getCore()->getConsole()->getPCVarBool("csg_block_auto_create");
			if(*csg_block_auto_create)
			{
				onKeyDown(SIK_ENTER);
			}
			else
			{
				// init resize gizmo
				for(UINT i = 0, l = ARRAYSIZE(m_apHandlers); i < l; ++i)
				{
					m_apHandlers[i]->enable(true);
				}
				placeHandlers();
			}
		}
	}
}

void CBrushCreatorBox::placeHandlers()
{
	float3 vCenter = (m_vPos0 + m_vPos1 + m_vUp * m_fHeight) * 0.5f;
	m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_Y]->setPos(vCenter - m_vUp * m_fHeight * 0.5f);
	m_apHandlers[GXCUBEMAP_FACE_POSITIVE_Y]->setPos(vCenter + m_vUp * m_fHeight * 0.5f);

	SMQuaternion qRot(float3(0.0f, 1.0f, 0.0f), m_vUp);

	float3 vDir = qRot * float3(1.0f, 0.0f, 0.0f);
	float3 vOffset = fabsf(SMVector3Dot(m_vPos1 - m_vPos0, vDir)) * 0.5f * vDir;
	m_apHandlers[GXCUBEMAP_FACE_POSITIVE_X]->setPos(vCenter + vOffset);
	m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_X]->setPos(vCenter - vOffset);
	
	vDir = qRot * float3(0.0f, 0.0f, 1.0f);
	vOffset = fabsf(SMVector3Dot(m_vPos1 - m_vPos0, vDir)) * 0.5f * vDir;
	m_apHandlers[GXCUBEMAP_FACE_POSITIVE_Z]->setPos(vCenter + vOffset);
	m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_Z]->setPos(vCenter - vOffset);
}

bool CBrushCreatorBox::onKeyDown(UINT uKey)
{
	if(uKey == SIK_ESCAPE)
	{
		if(m_isInitiated)
		{
			m_isInitiated = false;
			for(UINT i = 0, l = ARRAYSIZE(m_apHandlers); i < l; ++i)
			{
				m_apHandlers[i]->enable(false);
			}
			return(true);
		}
	}
	else if(uKey == SIK_ENTER || uKey == SIK_NUMPADENTER)
	{
		if(m_isInitiated)
		{
			// release resize gizmo
			for(UINT i = 0, l = ARRAYSIZE(m_apHandlers); i < l; ++i)
			{
				m_apHandlers[i]->enable(false);
			}

			m_isInitiated = false;
			const char *szMat = m_pEditor->getMaterialBrowser()->getCurrentMaterial();
			// create brush with default height

			COutline outline(m_pEditor, m_pRenderer);
			outline.setPlane(m_vUp);


			float3 vT = CBrushMesh::GetTangent(m_vUp);
			float3 vS = SMVector3Cross(vT, m_vUp);
			float fDT = SMVector3Dot(m_vPos1, vT) - SMVector3Dot(m_vPos0, vT);
			float fDS = SMVector3Dot(m_vPos1, vS) - SMVector3Dot(m_vPos0, vS);

			if(m_isBoxMode)
			{
				outline.setMouse(m_vPos0);
				outline.addPoint();
				outline.setMouse((float3)(m_vPos0 + vT * fDT));
				outline.addPoint();
				outline.setMouse(m_vPos1);
				outline.addPoint();
				outline.setMouse((float3)(m_vPos0 + vS * fDS));
				outline.addPoint();
			}
			else
			{
				float3 vCenter = (m_vPos1 + m_vPos0) * 0.5f;

				fDT *= 0.5f;
				fDS *= 0.5f;

				UINT uSideCount = 16;

				for(UINT i = 0; i < uSideCount; ++i)
				{
					float A = (float)i / (float)uSideCount * SM_2PI;
					outline.setMouse((float3)(vCenter + vS * fDS * cosf(A) - vT * fDT * sinf(A)));
					outline.addPoint();
				}
			}
			outline.closePath();
			outline.buildContours();

			CEditorObject *pObject = new CEditorObject(m_pEditable);
			for(UINT i = 0, l = outline.getContourCount(); i < l; ++i)
			{
				pObject->addBrush(new CBrushMesh(m_pEditable->getCore(), &outline, i, szMat, m_fHeight));
			}
			
			pObject->fixPos();

			CCommandCreate *pCmd = new CCommandCreate(m_pEditor, pObject);
			m_pEditor->execCommand(pCmd);
			
			return(true);
		}
	}

	return(false);
}
void CBrushCreatorBox::onKeyUp(UINT uKey)
{
}

void CBrushCreatorBox::render(bool is3D)
{
	if(true)
	{
		m_pRenderer->reset();
		if(m_isInitiated)
		{
			m_pRenderer->setColor(isValid() ? float4_t(0.6f, 0.8f, 1.0f, 1.0f) : float4_t(1.0f, 0.2f, 0.2f, 1.0f));
			m_pRenderer->drawAABB(SMAABB(m_vPos0, m_vPos1 + m_vUp * m_fHeight));
		}
	}

	m_pRenderer->render(!is3D);
}

UINT CBrushCreatorBox::getClassCount()
{
	return(2);
}

const char* CBrushCreatorBox::getClassAt(UINT idx)
{
	switch(idx)
	{
	case 0:
		return("Box");
	case 1:
		return("Cylinder");
	}
	assert(!"Invalid class idx");
	return(NULL);
}

bool CBrushCreatorBox::useClass(const char *szClassName)
{
	m_isBoxMode = !strcmp("Box", szClassName);
	return(true);
}

bool CBrushCreatorBox::isValid()
{
	return(!SMIsZero(SMAABBVolume(SMAABB(m_vPos0, m_vPos1 + m_vUp * m_fHeight))));
}

void CBrushCreatorBox::onHandleMove(const float3 &vNewPos, IXEditorGizmoHandle *pHandle)
{
	if(pHandle == m_apHandlers[GXCUBEMAP_FACE_POSITIVE_Y] || pHandle == m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_Y])
	{
		float fDist = SMVector4Dot(SMPLANE(m_vUp, m_vPos0), float4(vNewPos, 1.0f));

		if(pHandle == m_apHandlers[GXCUBEMAP_FACE_POSITIVE_Y])
		{
			m_fHeight = fDist;
		}
		else
		{
			m_fHeight -= fDist;
			float3 vOffset = m_vUp * fDist;
			m_vPos0 += vOffset;
			m_vPos1 += vOffset;
		}

		if(SMIsZero(m_fHeight))
		{
			m_fHeight = 0.001f;
		}

		if(m_fHeight < 0.0f)
		{
			float3 vOffset = m_vUp * m_fHeight;
			m_vPos0 += vOffset;
			m_vPos1 += vOffset;
			m_fHeight = -m_fHeight;

			std::swap(m_apHandlers[GXCUBEMAP_FACE_POSITIVE_Y], m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_Y]);
		}
	}
	else if(pHandle == m_apHandlers[GXCUBEMAP_FACE_POSITIVE_X] || pHandle == m_apHandlers[GXCUBEMAP_FACE_POSITIVE_Z])
	{
		float3 vDir = SMQuaternion(float3(0.0f, 1.0f, 0.0f), m_vUp) * (pHandle == m_apHandlers[GXCUBEMAP_FACE_POSITIVE_X] ? 
			float3(1.0f, 0.0f, 0.0f) : 
			float3(0.0f, 0.0f, 1.0f)
			);

		float3 &vPos = SMVector3Dot(m_vPos1 - m_vPos0, vDir) > 0.0f ? m_vPos1 : m_vPos0;
		float fDist = SMVector4Dot(SMPLANE(vDir, vPos), float4(vNewPos, 1.0f));

		vPos += vDir * fDist;

		float fProj = SMVector3Dot(vPos - (&vPos == &m_vPos0 ? m_vPos1 : m_vPos0), vDir);

		if(SMIsZero(fProj))
		{
			vPos += 0.001f * vDir;
		}

		if(fProj < 0.0f)
		{
			if(pHandle == m_apHandlers[GXCUBEMAP_FACE_POSITIVE_X])
			{
				std::swap(m_apHandlers[GXCUBEMAP_FACE_POSITIVE_X], m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_X]);
			}
			else
			{
				std::swap(m_apHandlers[GXCUBEMAP_FACE_POSITIVE_Z], m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_Z]);
			}
		}
	}
	else if(pHandle == m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_X] || pHandle == m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_Z])
	{
		float3 vDir = SMQuaternion(float3(0.0f, 1.0f, 0.0f), m_vUp) * (pHandle == m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_X] ?
			float3(1.0f, 0.0f, 0.0f) :
			float3(0.0f, 0.0f, 1.0f)
			);

		float3 &vPos = SMVector3Dot(m_vPos1 - m_vPos0, vDir) < 0.0f ? m_vPos1 : m_vPos0;
		float fDist = SMVector4Dot(SMPLANE(vDir, vPos), float4(vNewPos, 1.0f));

		vPos += vDir * fDist;

		float fProj = SMVector3Dot((&vPos == &m_vPos0 ? m_vPos1 : m_vPos0) - vPos, vDir);

		if(SMIsZero(fProj))
		{
			vPos -= 0.001f * vDir;
		}

		if(fProj < 0.0f)
		{
			if(pHandle == m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_X])
			{
				std::swap(m_apHandlers[GXCUBEMAP_FACE_POSITIVE_X], m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_X]);
			}
			else
			{
				std::swap(m_apHandlers[GXCUBEMAP_FACE_POSITIVE_Z], m_apHandlers[GXCUBEMAP_FACE_NEGATIVE_Z]);
			}
		}
	}

	placeHandlers();
}

//##########################################################################

void XMETHODCALLTYPE CBrushCreatorHandleCallback::moveTo(const float3 &vNewPos, IXEditorGizmoHandle *pGizmo)
{
	m_pBrushCreator->onHandleMove(vNewPos, pGizmo);
}
void XMETHODCALLTYPE CBrushCreatorHandleCallback::onStart(IXEditorGizmoHandle *pGizmo)
{
}
void XMETHODCALLTYPE CBrushCreatorHandleCallback::onEnd(IXEditorGizmoHandle *pGizmo)
{
}
