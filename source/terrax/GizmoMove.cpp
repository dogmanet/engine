#include "GizmoMove.h"
#include "Editor.h"
#include "GizmoHandle.h"

#define AXES_HELPER_MOVE_LENGTH 1.0f		/*!< */

CGizmoMove::CGizmoMove(CEditor *pEditor):
	m_pEditor(pEditor),
	m_handleCallback(this),
	m_pHandle(new CGizmoHandle(pEditor))
{
	m_pHandle->setRendered(false);
	m_pHandle->setCallback(&m_handleCallback);
	m_pEditor->setDirty();
}
CGizmoMove::~CGizmoMove()
{
	m_pHandle->setCallback(NULL);
	mem_release(m_pHandle);

	m_pEditor->onGizmoRemoved(this); 

	m_pEditor->setDirty();
}

void XMETHODCALLTYPE CGizmoMove::enable(bool yesNo)
{
	if(m_isEnabled != yesNo)
	{
		m_isEnabled = yesNo;
		m_pEditor->setDirty();
	}
}

void XMETHODCALLTYPE CGizmoMove::setPos(const float3_t &vPos)
{
	m_vPos = vPos;

	if(m_isTracking)
	{
		m_pHandle->setPos((float3)(vPos - m_vMovDelta));
	}
	else
	{
		m_pHandle->setPos(vPos);
	}
}
const float3_t& XMETHODCALLTYPE CGizmoMove::getPos()
{
	return(m_vPos);
}

void CGizmoMove::onHandlerMove(const float3 &vPos)
{
	if(m_isFirstMov)
	{
		m_isFirstMov = false;
		m_vMovDelta = (float3)(m_vPos - vPos);
	}
	else
	{
		float3 vNewPos = m_vMovDelta + vPos;
		SAFE_CALL(m_pCallback, moveTo, vNewPos, this)
		else
		{
			setPos(vNewPos);
		}
	}
}

bool CGizmoMove::wantHandle(const float3 &vRayOrigin, const float3 &vRayDir)
{
	m_lastAxe = m_currentAxe;
	bool res = intersectMove(vRayOrigin, vRayDir);
	if(m_lastAxe != m_currentAxe)
	{
		m_pEditor->setDirty();
	}
	return(res);
}

void CGizmoMove::setWorldRay(const float3 &vRayOrigin, const float3 &vRayDir)
{
	m_pHandle->setWorldRay(vRayOrigin, vRayDir);
}
void CGizmoMove::setTracking(bool isTracking)
{
	if(isTracking)
	{
		switch(m_currentAxe)
		{
		case CGizmoMove::HANDLER_AXE_X:
			m_pHandle->lockInDir(float3_t(1.0f, 0.0f, 0.0));
			break;
		case CGizmoMove::HANDLER_AXE_Y:
			m_pHandle->lockInDir(float3_t(0.0f, 1.0f, 0.0));
			break;
		case CGizmoMove::HANDLER_AXE_XY:
			m_pHandle->lockInPlane(float3_t(0.0f, 0.0f, 1.0));
			break;
		case CGizmoMove::HANDLER_AXE_Z:
			m_pHandle->lockInDir(float3_t(0.0f, 0.0f, 1.0));
			break;
		case CGizmoMove::HANDLER_AXE_XZ:
			m_pHandle->lockInPlane(float3_t(0.0f, 1.0f, 0.0));
			break;
		case CGizmoMove::HANDLER_AXE_YZ:
			m_pHandle->lockInPlane(float3_t(1.0f, 0.0f, 0.0));
			break;
		}

		m_isFirstMov = true;
		m_fFixedScale = -1.0f;
		SAFE_CALL(m_pCallback, onStart, this);
	}
	else
	{
		SAFE_CALL(m_pCallback, onEnd, this);
	}
	m_isTracking = isTracking;
	m_pHandle->setTracking(isTracking);
	if(!isTracking)
	{
		m_pHandle->setPos(m_vPos);
	}
}

void CGizmoMove::draw(IXGizmoRenderer *pGRBoth, IXGizmoRenderer *pGR2D, IXGizmoRenderer *pGR3D)
{
	m_pHandle->draw(pGRBoth, pGR2D, pGR3D);

	IXGizmoRenderer *pRenderer = pGR3D;

	IXCamera *pCamera;
	m_pEditor->getCameraForView(XWP_TOP_LEFT, &pCamera);
	float3 vCamDir = pCamera->getPosition() - m_vPos;

	float fDist = SMVector3Length(vCamDir) * 0.20f;
	float3 vScale = (fDist > 0.25f ? fDist : 0.25f);

	if(m_fFixedScale < 0.0f)
	{
		m_fFixedScale = vScale.x;
	}
	else if(m_isTracking)
	{
		vScale = m_fFixedScale;
	}

	DWORD color_act = 0xFFFFFF00;
	DWORD color_act2 = 0x1FFFFF00;


	bool bXNeg = vCamDir.x <= 0.0f;
	bool bYNeg = vCamDir.y <= 0.0f;
	bool bZNeg = vCamDir.z <= 0.0f;

	float len = AXES_HELPER_MOVE_LENGTH;
	vert lst[] = {
		{float3_t(0, 0, 0), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(len, 0, 0), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(0, 0, 0), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(0, len, 0), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(0, 0, 0), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},
		{float3_t(0, 0, len), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},

		{float3_t(len * 0.5f, 0, 0), (m_currentAxe & HANDLER_AXE_XY) == HANDLER_AXE_XY ? color_act : 0xFFFF0000},
		{float3_t(len * 0.5f, len * 0.5f, 0), (m_currentAxe & HANDLER_AXE_XY) == HANDLER_AXE_XY ? color_act : 0x3FFF0000},
		{float3_t(len * 0.5f, 0, 0), (m_currentAxe & HANDLER_AXE_XZ) == HANDLER_AXE_XZ ? color_act : 0xFFFF0000},
		{float3_t(len * 0.5f, 0, len * 0.5f), (m_currentAxe & HANDLER_AXE_XZ) == HANDLER_AXE_XZ ? color_act : 0x3FFF0000},

		{float3_t(0, len * 0.5f, 0), (m_currentAxe & HANDLER_AXE_YZ) == HANDLER_AXE_YZ ? color_act : 0xFF00FF00},
		{float3_t(0, len * 0.5f, len * 0.5f), (m_currentAxe & HANDLER_AXE_YZ) == HANDLER_AXE_YZ ? color_act : 0x3F00FF00},
		{float3_t(0, len * 0.5f, 0), (m_currentAxe & HANDLER_AXE_XY) == HANDLER_AXE_XY ? color_act : 0xFF00FF00},
		{float3_t(len * 0.5f, len * 0.5f, 0), (m_currentAxe & HANDLER_AXE_XY) == HANDLER_AXE_XY ? color_act : 0x3F00FF00},

		{float3_t(0, 0, len * 0.5f), (m_currentAxe & HANDLER_AXE_YZ) == HANDLER_AXE_YZ ? color_act : 0xFF0000FF},
		{float3_t(0, len * 0.5f, len * 0.5f), (m_currentAxe & HANDLER_AXE_YZ) == HANDLER_AXE_YZ ? color_act : 0x3F0000FF},
		{float3_t(0, 0, len * 0.5f), (m_currentAxe & HANDLER_AXE_XZ) == HANDLER_AXE_XZ ? color_act : 0xFF0000FF},
		{float3_t(len * 0.5f, 0, len * 0.5f), (m_currentAxe & HANDLER_AXE_XZ) == HANDLER_AXE_XZ ? color_act : 0x3F0000FF},
	};
	if(bXNeg || bYNeg || bZNeg)
	{
		for(UINT i = 0; i < ARRAYSIZE(lst); ++i)
		{
			if(bXNeg)
			{
				lst[i].pos.x *= -1.0f;
			}
			if(bYNeg)
			{
				lst[i].pos.y *= -1.0f;
			}
			if(bZNeg)
			{
				lst[i].pos.z *= -1.0f;
			}
		}
	}

	for(UINT i = 0, l = ARRAYSIZE(lst) / 2; i < l; ++i)
	{
		pRenderer->setColor(GX_COLOR_COLOR_TO_F4(lst[i * 2 + 0].color));
		pRenderer->jumpTo(lst[i * 2 + 0].pos * vScale + m_vPos);
		pRenderer->setColor(GX_COLOR_COLOR_TO_F4(lst[i * 2 + 1].color));
		pRenderer->lineTo(lst[i * 2 + 1].pos * vScale + m_vPos);
	}

	float asize = 0.1f;
	float a2size = 0.3f;


	vert l2[] = {
		//arrow X
		{float3_t(len, asize * -0.5f, asize * -0.5f), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(len, asize * -0.5f, asize * 0.5f), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(len, asize * 0.5f, asize * -0.5f), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(len, asize * 0.5f, asize * -0.5f), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(len, asize * -0.5f, asize * 0.5f), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(len, asize * 0.5f, asize * 0.5f), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},

		{float3_t(len, asize * -0.5f, asize * 0.5f), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(len, asize * -0.5f, asize * -0.5f), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(len + asize * 2.0f, 0, 0), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(len, asize * 0.5f, asize * 0.5f), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(len, asize * -0.5f, asize * 0.5f), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(len + asize * 2.0f, 0, 0), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(len, asize * 0.5f, asize * -0.5f), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(len, asize * 0.5f, asize * 0.5f), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(len + asize * 2.0f, 0, 0), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(len, asize * -0.5f, asize * -0.5f), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(len, asize * 0.5f, asize * -0.5f), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},
		{float3_t(len + asize * 2.0f, 0, 0), (m_currentAxe & HANDLER_AXE_X) ? color_act : 0xFFFF0000},

		//arrow Y
		{float3_t(asize * -0.5f, len, asize * -0.5f), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(asize * 0.5f, len, asize * -0.5f), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(asize * -0.5f, len, asize * 0.5f), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(asize * 0.5f, len, asize * -0.5f), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(asize * 0.5f, len, asize * 0.5f), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(asize * -0.5f, len, asize * 0.5f), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},

		{float3_t(asize * -0.5f, len, asize * -0.5f), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(asize * -0.5f, len, asize * 0.5f), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(0, len + asize * 2.0f, 0), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(asize * -0.5f, len, asize * 0.5f), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(asize * 0.5f, len, asize * 0.5f), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(0, len + asize * 2.0f, 0), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(asize * 0.5f, len, asize * 0.5f), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(asize * 0.5f, len, asize * -0.5f), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(0, len + asize * 2.0f, 0), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(asize * 0.5f, len, asize * -0.5f), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(asize * -0.5f, len, asize * -0.5f), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},
		{float3_t(0, len + asize * 2.0f, 0), (m_currentAxe & HANDLER_AXE_Y) ? color_act : 0xFF00FF00},

		//arrow Z
		{float3_t(asize * -0.5f, asize * -0.5f, len), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},
		{float3_t(asize * 0.5f, asize * -0.5f, len), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},
		{float3_t(asize * -0.5f, asize * 0.5f, len), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},
		{float3_t(asize * 0.5f, asize * -0.5f, len), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},
		{float3_t(asize * 0.5f, asize * 0.5f, len), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},
		{float3_t(asize * -0.5f, asize * 0.5f, len), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},

		{float3_t(asize * -0.5f, asize * -0.5f, len), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},
		{float3_t(asize * -0.5f, asize * 0.5f, len), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},
		{float3_t(0, 0, len + asize * 2.0f), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},
		{float3_t(asize * -0.5f, asize * 0.5f, len), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},
		{float3_t(asize * 0.5f, asize * 0.5f, len), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},
		{float3_t(0, 0, len + asize * 2.0f), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},
		{float3_t(asize * 0.5f, asize * 0.5f, len), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},
		{float3_t(asize * 0.5f, asize * -0.5f, len), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},
		{float3_t(0, 0, len + asize * 2.0f), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},
		{float3_t(asize * 0.5f, asize * -0.5f, len), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},
		{float3_t(asize * -0.5f, asize * -0.5f, len), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},
		{float3_t(0, 0, len + asize * 2.0f), (m_currentAxe & HANDLER_AXE_Z) ? color_act : 0xFF0000FF},


		//plane XY
		{float3_t(0, 0, 0), (m_currentAxe & HANDLER_AXE_XY) == HANDLER_AXE_XY ? color_act2 : 0x1FFFFF00},
		{float3_t(len * 0.5f, 0, 0), (m_currentAxe & HANDLER_AXE_XY) == HANDLER_AXE_XY ? color_act2 : 0x1FFF0000},
		{float3_t(len * 0.5f, len * 0.5f, 0), (m_currentAxe & HANDLER_AXE_XY) == HANDLER_AXE_XY ? color_act2 : 0x1FFFFF00},
		{float3_t(0, 0, 0), (m_currentAxe & HANDLER_AXE_XY) == HANDLER_AXE_XY ? color_act2 : 0x1FFFFF00},
		{float3_t(len * 0.5f, len * 0.5f, 0), (m_currentAxe & HANDLER_AXE_XY) == HANDLER_AXE_XY ? color_act2 : 0x1FFFFF00},
		{float3_t(0, len * 0.5f, 0), (m_currentAxe & HANDLER_AXE_XY) == HANDLER_AXE_XY ? color_act2 : 0x1F00FF00},
		{float3_t(0, 0, 0), (m_currentAxe & HANDLER_AXE_XY) == HANDLER_AXE_XY ? color_act2 : 0x1FFFFF00},
		{float3_t(len * 0.5f, len * 0.5f, 0), (m_currentAxe & HANDLER_AXE_XY) == HANDLER_AXE_XY ? color_act2 : 0x1FFFFF00},
		{float3_t(len * 0.5f, 0, 0), (m_currentAxe & HANDLER_AXE_XY) == HANDLER_AXE_XY ? color_act2 : 0x1FFF0000},
		{float3_t(len * 0.5f, len * 0.5f, 0), (m_currentAxe & HANDLER_AXE_XY) == HANDLER_AXE_XY ? color_act2 : 0x1FFFFF00},
		{float3_t(0, 0, 0), (m_currentAxe & HANDLER_AXE_XY) == HANDLER_AXE_XY ? color_act2 : 0x1FFFFF00},
		{float3_t(0, len * 0.5f, 0), (m_currentAxe & HANDLER_AXE_XY) == HANDLER_AXE_XY ? color_act2 : 0x1F00FF00},

		//plane XZ
		{float3_t(0, 0, 0), (m_currentAxe & HANDLER_AXE_XZ) == HANDLER_AXE_XZ ? color_act2 : 0x1FFF00FF},
		{float3_t(len * 0.5f, 0, 0), (m_currentAxe & HANDLER_AXE_XZ) == HANDLER_AXE_XZ ? color_act2 : 0x1FFF0000},
		{float3_t(len * 0.5f, 0, len * 0.5f), (m_currentAxe & HANDLER_AXE_XZ) == HANDLER_AXE_XZ ? color_act2 : 0x1FFF00FF},
		{float3_t(0, 0, 0), (m_currentAxe & HANDLER_AXE_XZ) == HANDLER_AXE_XZ ? color_act2 : 0x1FFF00FF},
		{float3_t(len * 0.5f, 0, len * 0.5f), (m_currentAxe & HANDLER_AXE_XZ) == HANDLER_AXE_XZ ? color_act2 : 0x1FFF00FF},
		{float3_t(0, 0, len * 0.5f), (m_currentAxe & HANDLER_AXE_XZ) == HANDLER_AXE_XZ ? color_act2 : 0x1F0000FF},
		{float3_t(0, 0, 0), (m_currentAxe & HANDLER_AXE_XZ) == HANDLER_AXE_XZ ? color_act2 : 0x1FFF00FF},
		{float3_t(len * 0.5f, 0, len * 0.5f), (m_currentAxe & HANDLER_AXE_XZ) == HANDLER_AXE_XZ ? color_act2 : 0x1FFF00FF},
		{float3_t(len * 0.5f, 0, 0), (m_currentAxe & HANDLER_AXE_XZ) == HANDLER_AXE_XZ ? color_act2 : 0x1FFF0000},
		{float3_t(0, 0, 0), (m_currentAxe & HANDLER_AXE_XZ) == HANDLER_AXE_XZ ? color_act2 : 0x1FFF00FF},
		{float3_t(0, 0, len * 0.5f), (m_currentAxe & HANDLER_AXE_XZ) == HANDLER_AXE_XZ ? color_act2 : 0x1F0000FF},
		{float3_t(len * 0.5f, 0, len * 0.5f), (m_currentAxe & HANDLER_AXE_XZ) == HANDLER_AXE_XZ ? color_act2 : 0x1FFF00FF},

		//plane YZ
		{float3_t(0, 0, 0), (m_currentAxe & HANDLER_AXE_YZ) == HANDLER_AXE_YZ ? color_act2 : 0x1F00FFFF},
		{float3_t(0, 0, len * 0.5f), (m_currentAxe & HANDLER_AXE_YZ) == HANDLER_AXE_YZ ? color_act2 : 0x1F0000FF},
		{float3_t(0, len * 0.5f, len * 0.5f), (m_currentAxe & HANDLER_AXE_YZ) == HANDLER_AXE_YZ ? color_act2 : 0x1F00FFFF},
		{float3_t(0, 0, 0), (m_currentAxe & HANDLER_AXE_YZ) == HANDLER_AXE_YZ ? color_act2 : 0x1F00FFFF},
		{float3_t(0, len * 0.5f, len * 0.5f), (m_currentAxe & HANDLER_AXE_YZ) == HANDLER_AXE_YZ ? color_act2 : 0x1F00FFFF},
		{float3_t(0, len * 0.5f, 0), (m_currentAxe & HANDLER_AXE_YZ) == HANDLER_AXE_YZ ? color_act2 : 0x1F00FF00},
		{float3_t(0, 0, 0), (m_currentAxe & HANDLER_AXE_YZ) == HANDLER_AXE_YZ ? color_act2 : 0x1F00FFFF},
		{float3_t(0, len * 0.5f, len * 0.5f), (m_currentAxe & HANDLER_AXE_YZ) == HANDLER_AXE_YZ ? color_act2 : 0x1F00FFFF},
		{float3_t(0, 0, len * 0.5f), (m_currentAxe & HANDLER_AXE_YZ) == HANDLER_AXE_YZ ? color_act2 : 0x1F0000FF},
		{float3_t(0, 0, 0), (m_currentAxe & HANDLER_AXE_YZ) == HANDLER_AXE_YZ ? color_act2 : 0x1F00FFFF},
		{float3_t(0, len * 0.5f, 0), (m_currentAxe & HANDLER_AXE_YZ) == HANDLER_AXE_YZ ? color_act2 : 0x1F00FF00},
		{float3_t(0, len * 0.5f, len * 0.5f), (m_currentAxe & HANDLER_AXE_YZ) == HANDLER_AXE_YZ ? color_act2 : 0x1F00FFFF},
	};
	if(bXNeg || bYNeg || bZNeg)
	{
		for(UINT i = 0; i < ARRAYSIZE(l2); ++i)
		{
			if(bXNeg)
			{
				l2[i].pos.x *= -1.0f;
			}
			if(bYNeg)
			{
				l2[i].pos.y *= -1.0f;
			}
			if(bZNeg)
			{
				l2[i].pos.z *= -1.0f;
			}
		}
	}

	for(UINT i = 0, l = ARRAYSIZE(l2) / 3; i < l; ++i)
	{
		pRenderer->drawPoly(
			(float3)(l2[i * 3 + 0].pos * vScale + m_vPos),
			(float3)(l2[i * 3 + 1].pos * vScale + m_vPos),
			(float3)(l2[i * 3 + 2].pos * vScale + m_vPos),
			GX_COLOR_COLOR_TO_F4(l2[i * 3 + 0].color),
			GX_COLOR_COLOR_TO_F4(l2[i * 3 + 1].color),
			GX_COLOR_COLOR_TO_F4(l2[i * 3 + 2].color)
		);
	}
}

bool CGizmoMove::intersectMove(const float3 &vStart, const float3 &vRayDir)
{
	IXCamera *pCamera;
	m_pEditor->getCameraForView(XWP_TOP_LEFT, &pCamera);
	float3 vCamDir = pCamera->getPosition() - m_vPos;

	float fDist = SMVector3Length(vCamDir) * 0.20f;
	float fScale = (fDist > 0.25f ? fDist : 0.25f);
	
	float3 vRayOrigin = (vStart - m_vPos) / fScale;

	float3 end = vRayOrigin + vRayDir * 1000.0f;
	float len = AXES_HELPER_MOVE_LENGTH;
	float asize = 0.1f;
	float a2size = 0.3f;

	m_currentAxe = HANDLER_AXE_NONE;

	float3 p;

	float mind = FLOAT_INF;

	bool bXNeg = vCamDir.x <= 0.0f;
	bool bYNeg = vCamDir.y <= 0.0f;
	bool bZNeg = vCamDir.z <= 0.0f;

	float3 vNeg(bXNeg ? -1.0f : 1.0f, bYNeg ? -1.0f : 1.0f, bZNeg ? -1.0f : 1.0f);

	if(SMTriangleIntersectLine(float3(0, 0, 0), float3(len * 0.5f, 0, 0) * vNeg, float3(len * 0.5f, len * 0.5f, 0) * vNeg, vRayOrigin, end, &p)
		|| SMTriangleIntersectLine(float3(0, 0, 0), float3(len * 0.5f, len * 0.5f, 0) * vNeg, float3(0, len * 0.5f, 0) * vNeg, vRayOrigin, end, &p))
	{
		float d = SMVector3Length2(p - vRayOrigin);
		if(d < mind)
		{
			mind = d;
			m_currentAxe = HANDLER_AXE_XY;
		}
	}

	if(SMTriangleIntersectLine(float3(0, 0, 0), float3(len * 0.5f, 0, 0) * vNeg, float3_t(len * 0.5f, 0, len * 0.5f) * vNeg, vRayOrigin, end, &p)
		|| SMTriangleIntersectLine(float3(0, 0, 0), float3(len * 0.5f, 0, len * 0.5f) * vNeg, float3_t(0, 0, len * 0.5f) * vNeg, vRayOrigin, end, &p))
	{
		float d = SMVector3Length2(p - vRayOrigin);
		if(d < mind)
		{
			mind = d;
			m_currentAxe = HANDLER_AXE_XZ;
		}
	}

	if(SMTriangleIntersectLine(float3(0, 0, 0), float3(0, 0, len * 0.5f) * vNeg, float3_t(0, len * 0.5f, len * 0.5f) * vNeg, vRayOrigin, end, &p)
		|| SMTriangleIntersectLine(float3(0, 0, 0), float3(0, len * 0.5f, len * 0.5f) * vNeg, float3_t(0, len * 0.5f, 0) * vNeg, vRayOrigin, end, &p))
	{
		float d = SMVector3Length2(p - vRayOrigin);
		if(d < mind)
		{
			mind = d;
			m_currentAxe = HANDLER_AXE_YZ;
		}
	}


	float3 asX[] = {
		float3(a2size * 0.5f, -a2size * 0.5f, -a2size * 0.5f),
		float3(a2size * 0.5f, a2size * 0.5f, -a2size * 0.5f),
		float3(len + asize * 2.0f, a2size * 0.5f, -a2size * 0.5f),
		float3(a2size * 0.5f, -a2size * 0.5f, -a2size * 0.5f),
		float3(len + asize * 2.0f, a2size * 0.5f, -a2size * 0.5f),
		float3(len + asize * 2.0f, -a2size * 0.5f, -a2size * 0.5f),

		float3(a2size * 0.5f, -a2size * 0.5f, a2size * 0.5f),
		float3(len + asize * 2.0f, a2size * 0.5f, a2size * 0.5f),
		float3(a2size * 0.5f, a2size * 0.5f, a2size * 0.5f),
		float3(a2size * 0.5f, -a2size * 0.5f, a2size * 0.5f),
		float3(len + asize * 2.0f, -a2size * 0.5f, a2size * 0.5f),
		float3(len + asize * 2.0f, a2size * 0.5f, a2size * 0.5f),

		float3(a2size * 0.5f, a2size * 0.5f, -a2size * 0.5f),
		float3(a2size * 0.5f, a2size * 0.5f, a2size * 0.5f),
		float3(len + asize * 2.0f, a2size * 0.5f, a2size * 0.5f),
		float3(a2size * 0.5f, a2size * 0.5f, -a2size * 0.5f),
		float3(len + asize * 2.0f, a2size * 0.5f, a2size * 0.5f),
		float3(len + asize * 2.0f, a2size * 0.5f, -a2size * 0.5f),

		float3(a2size * 0.5f, -a2size * 0.5f, -a2size * 0.5f),
		float3(len + asize * 2.0f, -a2size * 0.5f, a2size * 0.5f),
		float3(a2size * 0.5f, -a2size * 0.5f, a2size * 0.5f),
		float3(a2size * 0.5f, -a2size * 0.5f, -a2size * 0.5f),
		float3(len + asize * 2.0f, -a2size * 0.5f, -a2size * 0.5f),
		float3(len + asize * 2.0f, -a2size * 0.5f, a2size * 0.5f),
	};

	float3 asY[] = {
		float3(-a2size * 0.5f, a2size * 0.5f, -a2size * 0.5f),
		float3(a2size * 0.5f, len + asize * 2.0f, -a2size * 0.5f),
		float3(a2size * 0.5f, a2size * 0.5f, -a2size * 0.5f),
		float3(-a2size * 0.5f, a2size * 0.5f, -a2size * 0.5f),
		float3(-a2size * 0.5f, len + asize * 2.0f, -a2size * 0.5f),
		float3(a2size * 0.5f, len + asize * 2.0f, -a2size * 0.5f),

		float3(-a2size * 0.5f, a2size * 0.5f, a2size * 0.5f),
		float3(a2size * 0.5f, a2size * 0.5f, a2size * 0.5f),
		float3(a2size * 0.5f, len + asize * 2.0f, a2size * 0.5f),
		float3(-a2size * 0.5f, a2size * 0.5f, a2size * 0.5f),
		float3(a2size * 0.5f, len + asize * 2.0f, a2size * 0.5f),
		float3(-a2size * 0.5f, len + asize * 2.0f, a2size * 0.5f),

		float3(a2size * 0.5f, a2size * 0.5f, -a2size * 0.5f),
		float3(a2size * 0.5f, len + asize * 2.0f, a2size * 0.5f),
		float3(a2size * 0.5f, a2size * 0.5f, a2size * 0.5f),
		float3(a2size * 0.5f, a2size * 0.5f, -a2size * 0.5f),
		float3(a2size * 0.5f, len + asize * 2.0f, -a2size * 0.5f),
		float3(a2size * 0.5f, len + asize * 2.0f, a2size * 0.5f),

		float3(-a2size * 0.5f, a2size * 0.5f, -a2size * 0.5f),
		float3(-a2size * 0.5f, a2size * 0.5f, a2size * 0.5f),
		float3(-a2size * 0.5f, len + asize * 2.0f, a2size * 0.5f),
		float3(-a2size * 0.5f, a2size * 0.5f, -a2size * 0.5f),
		float3(-a2size * 0.5f, len + asize * 2.0f, a2size * 0.5f),
		float3(-a2size * 0.5f, len + asize * 2.0f, -a2size * 0.5f),
	};

	float3 asZ[] = {
		float3(-a2size * 0.5f, -a2size * 0.5f, a2size * 0.5f),
		float3(a2size * 0.5f, -a2size * 0.5f, a2size * 0.5f),
		float3(a2size * 0.5f, -a2size * 0.5f, len + asize * 2.0f),
		float3(-a2size * 0.5f, -a2size * 0.5f, a2size * 0.5f),
		float3(a2size * 0.5f, -a2size * 0.5f, len + asize * 2.0f),
		float3(-a2size * 0.5f, -a2size * 0.5f, len + asize * 2.0f),

		float3(-a2size * 0.5f, a2size * 0.5f, a2size * 0.5f),
		float3(a2size * 0.5f, a2size * 0.5f, len + asize * 2.0f),
		float3(a2size * 0.5f, a2size * 0.5f, a2size * 0.5f),
		float3(-a2size * 0.5f, a2size * 0.5f, a2size * 0.5f),
		float3(-a2size * 0.5f, a2size * 0.5f, len + asize * 2.0f),
		float3(a2size * 0.5f, a2size * 0.5f, len + asize * 2.0f),

		float3(a2size * 0.5f, -a2size * 0.5f, a2size * 0.5f),
		float3(a2size * 0.5f, a2size * 0.5f, a2size * 0.5f),
		float3(a2size * 0.5f, a2size * 0.5f, len + asize * 2.0f),
		float3(a2size * 0.5f, -a2size * 0.5f, a2size * 0.5f),
		float3(a2size * 0.5f, a2size * 0.5f, len + asize * 2.0f),
		float3(a2size * 0.5f, -a2size * 0.5f, len + asize * 2.0f),

		float3(-a2size * 0.5f, -a2size * 0.5f, a2size * 0.5f),
		float3(-a2size * 0.5f, a2size * 0.5f, len + asize * 2.0f),
		float3(-a2size * 0.5f, a2size * 0.5f, a2size * 0.5f),
		float3(-a2size * 0.5f, -a2size * 0.5f, a2size * 0.5f),
		float3(-a2size * 0.5f, -a2size * 0.5f, len + asize * 2.0f),
		float3(-a2size * 0.5f, a2size * 0.5f, len + asize * 2.0f),
	};

	if(bXNeg || bYNeg || bZNeg)
	{
		for(UINT i = 0; i < ARRAYSIZE(asX); ++i)
		{
			if(bXNeg)
			{
				asX[i].x *= -1.0f;
			}
			if(bYNeg)
			{
				asX[i].y *= -1.0f;
			}
			if(bZNeg)
			{
				asX[i].z *= -1.0f;
			}
		}
	}
	if(bXNeg || bYNeg || bZNeg)
	{
		for(UINT i = 0; i < ARRAYSIZE(asY); ++i)
		{
			if(bXNeg)
			{
				asY[i].x *= -1.0f;
			}
			if(bYNeg)
			{
				asY[i].y *= -1.0f;
			}
			if(bZNeg)
			{
				asY[i].z *= -1.0f;
			}
		}
	}
	if(bXNeg || bYNeg || bZNeg)
	{
		for(UINT i = 0; i < ARRAYSIZE(asZ); ++i)
		{
			if(bXNeg)
			{
				asZ[i].x *= -1.0f;
			}
			if(bYNeg)
			{
				asZ[i].y *= -1.0f;
			}
			if(bZNeg)
			{
				asZ[i].z *= -1.0f;
			}
		}
	}

	for(int i = 0, l = sizeof(asX) / sizeof(asX[0]); i < l; i += 3)
	{
		if(SMTriangleIntersectLine(asX[i], asX[i + 1], asX[i + 2], vRayOrigin, end, &p))
		{
			float d = SMVector3Length2(p - vRayOrigin);
			if(d < mind)
			{
				mind = d;
				m_currentAxe = HANDLER_AXE_X;
				break;
			}
		}
	}
	for(int i = 0, l = sizeof(asY) / sizeof(asY[0]); i < l; i += 3)
	{
		if(SMTriangleIntersectLine(asY[i], asY[i + 1], asY[i + 2], vRayOrigin, end, &p))
		{
			float d = SMVector3Length2(p - vRayOrigin);
			if(d < mind)
			{
				mind = d;
				m_currentAxe = HANDLER_AXE_Y;
				break;
			}
		}
	}
	for(int i = 0, l = sizeof(asZ) / sizeof(asZ[0]); i < l; i += 3)
	{
		if(SMTriangleIntersectLine(asZ[i], asZ[i + 1], asZ[i + 2], vRayOrigin, end, &p))
		{
			float d = SMVector3Length2(p - vRayOrigin);
			if(d < mind)
			{
				mind = d;
				m_currentAxe = HANDLER_AXE_Z;
				break;
			}
		}
	}

	return(m_currentAxe != HANDLER_AXE_NONE);
}

//##########################################################################

CMoveHandleCallback::CMoveHandleCallback(CGizmoMove *pMove):
	m_pGizmo(pMove)
{
}
void XMETHODCALLTYPE CMoveHandleCallback::moveTo(const float3 &vNewPos, IXEditorGizmoHandle *pGizmo)
{
	m_pGizmo->onHandlerMove(vNewPos);
}

