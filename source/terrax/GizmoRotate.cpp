#include "GizmoRotate.h"
#include "Editor.h"

CGizmoRotate::CGizmoRotate(CEditor *pEditor):
	m_pEditor(pEditor)
{
}
CGizmoRotate::~CGizmoRotate()
{
	m_pEditor->onGizmoRemoved(this);
}

void XMETHODCALLTYPE CGizmoRotate::enable(bool yesNo)
{
	if(m_isEnabled != yesNo)
	{
		m_isEnabled = yesNo;
		m_pEditor->setDirty();
	}
}

void XMETHODCALLTYPE CGizmoRotate::setPos(const float3_t &vPos)
{
	m_vPos = vPos;
}
const float3_t& XMETHODCALLTYPE CGizmoRotate::getPos()
{
	return(m_vPos);
}

void CGizmoRotate::draw(IXGizmoRenderer *pGRBoth, IXGizmoRenderer *pGR2D, IXGizmoRenderer *pGR3D)
{
	IXCamera *pCamera;
	m_pEditor->getCameraForView(XWP_TOP_LEFT, &pCamera);


	float fDist = SMVector3Distance(m_vPos, pCamera->getPosition()) * 0.20f;
	float fScale = (fDist > 0.25f ? fDist : 0.25f);

	float3 vCamDir = pCamera->getLook();

	float fSize = 0.6f * fScale;

	float4_t vHighlight(1.0f, 1.0f, 0.0f, 1.0f);

	if(m_isTracking)
	{
		pGR3D->setLineWidth(1.0f);
		pGR3D->setColor(float4_t(1.0f, 1.0f, 1.0f, 0.7f));

		float3 vI = m_vBasePoint - m_vPos;
		fSize = SMVector3Length(vI)/* * fScale*/;
		vI = SMVector3Normalize(vI);
		float3 vJ = SMVector3Normalize(SMVector3Cross(m_vCurDir, vI));

		float3 vPos;
		float fArg;
		float fLen;
		
		for(UINT i = 0; i < 360; i += 5)
		{
			fArg = SM_PIDIV180 * i;
			vPos = (sinf(fArg) * vI + cosf(fArg) * vJ) * fSize;

			if(i % 90 == 0)
			{
				fLen = 0.0f;
			}
			else if(i % 45 == 0)
			{
				fLen = 0.5f;
			}
			else if(i % 15 == 0)
			{
				fLen = 0.7f;
			}
			else if(i % 10 == 0)
			{
				fLen = 0.85f;
			}
			else
			{
				fLen = 0.9f;
			}

			pGR3D->jumpTo(vPos + m_vPos);
			pGR3D->lineTo(vPos * fLen + m_vPos);
		}

		for(UINT i = 0; i <= 360; i += 5)
		{
			fArg = SM_PIDIV180 * i;
			vPos = (sinf(fArg) * vI + cosf(fArg) * vJ) * fSize;

			if(i == 0)
			{
				pGR3D->jumpTo(vPos + m_vPos);
			}
			else
			{
				pGR3D->lineTo(vPos + m_vPos);
			}
		}


		pGR3D->setLineWidth(2.0f);
		pGR3D->setColor(float4_t(1.0f, 1.0f, 0.0f, 1.0f));
		pGR3D->jumpTo(m_vBasePoint);
		pGR3D->lineTo(m_vPos);
		pGR3D->lineTo(SMQuaternion(m_vCurDir, m_fCurrentRotation) * (m_vBasePoint - m_vPos) + m_vPos);

		pGR3D->setLineWidth(1.0f);
		pGR3D->setColor(float4_t(1.0f, 1.0f, 0.0f, 0.5f));
		pGR3D->jumpTo(m_vPos);
		pGR3D->lineTo(m_vLastPoint);
		return;
	}

	if(m_isAxisLocked)
	{
		pGR3D->setLineWidth(2.0f);
		pGR3D->setColor(m_currentAxe == HANDLER_AXE_CUSTOM ? vHighlight : float4_t(1.0f, 1.0f, 1.0f, 1.0f));
		drawArc(pGR3D, m_vPos, vCamDir, m_vLockAxis, fSize);

		pGR3D->setLineWidth(1.0f);
		pGR3D->setColor(float4_t(1.0f, 1.0f, 1.0f, 0.4f));
		drawArc(pGR3D, m_vPos, -vCamDir, m_vLockAxis, fSize);
	}
	else
	{
		pGR3D->setLineWidth(2.0f);
		pGR3D->setColor(m_currentAxe == HANDLER_AXE_X ? vHighlight : float4_t(1.0f, 0.0f, 0.0f, 1.0f));
		drawArc(pGR3D, m_vPos, vCamDir, float3(1.0f, 0.0f, 0.0f), fSize);
		pGR3D->setColor(m_currentAxe == HANDLER_AXE_Y ? vHighlight : float4_t(0.0f, 1.0f, 0.0f, 1.0f));
		drawArc(pGR3D, m_vPos, vCamDir, float3(0.0f, 1.0f, 0.0f), fSize);
		pGR3D->setColor(m_currentAxe == HANDLER_AXE_Z ? vHighlight : float4_t(0.0f, 0.0f, 1.0f, 1.0f));
		drawArc(pGR3D, m_vPos, vCamDir, float3(0.0f, 0.0f, 1.0f), fSize);


		pGR3D->setLineWidth(1.0f);
		pGR3D->setColor(float4_t(1.0f, 0.0f, 0.0f, 0.4f));
		drawArc(pGR3D, m_vPos, -vCamDir, float3(1.0f, 0.0f, 0.0f), fSize);
		pGR3D->setColor(float4_t(0.0f, 1.0f, 0.0f, 0.4f));
		drawArc(pGR3D, m_vPos, -vCamDir, float3(0.0f, 1.0f, 0.0f), fSize);
		pGR3D->setColor(float4_t(0.0f, 0.0f, 1.0f, 0.4f));
		drawArc(pGR3D, m_vPos, -vCamDir, float3(0.0f, 0.0f, 1.0f), fSize);
	}
}

void CGizmoRotate::drawArc(IXGizmoRenderer *pGR3D, const float3 &vCenter, const float3 &vCamDir, const float3 &vDir, float fRadius)
{
	// @FIXME There could be a problem in case vCamDir collinear with vDir
	float3 vJ = SMVector3Normalize(SMVector3Cross(vDir, vCamDir)) * fRadius;
	float3 vI = SMVector3Normalize(SMVector3Cross(vDir, vJ)) * fRadius;

	const int c_iSteps = 16;
	const float c_fStep = SM_PI / (float)c_iSteps;
	
	float3 vPos;
	float fArg;
	for(int i = 0; i <= c_iSteps; ++i)
	{
		fArg = i * c_fStep;
		vPos = sinf(fArg) * vI + cosf(fArg) * vJ + vCenter;
		if(i == 0)
		{
			pGR3D->jumpTo(vPos);
		}
		else
		{
			pGR3D->lineTo(vPos);
		}
	}
}

bool CGizmoRotate::wantHandle(const float3 &vRayOrigin, const float3 &vRayDir)
{
	m_lastAxe = m_currentAxe;
	bool res = intersectMove(vRayOrigin, vRayDir);
	if(m_lastAxe != m_currentAxe)
	{
		m_pEditor->setDirty();
	}
	return(res);
}

bool CGizmoRotate::intersectMove(const float3 &vStart, const float3 &vRayDir)
{
	IXCamera *pCamera;
	m_pEditor->getCameraForView(XWP_TOP_LEFT, &pCamera);

	float fDist = SMVector3Distance(m_vPos, pCamera->getPosition()) * 0.20f;
	float fScale = (fDist > 0.25f ? fDist : 0.25f);

	float3 vRayOrigin = (vStart - m_vPos) / fScale;

	float3 end = vRayOrigin + vRayDir * 1000.0f;

	m_currentAxe = HANDLER_AXE_NONE;

	float fSize = 0.6f;
	float fDelta = 0.1f;
	float vCamDir2, vCamDirMax = FLT_MAX;
	float3 vPt;

	float3 avDirs[] = {
		float3(1.0f, 0.0f, 0.0f),
		float3(0.0f, 1.0f, 0.0f),
		float3(0.0f, 0.0f, 1.0f)
	};

	float3 avDirAny[] = {
		m_vLockAxis
	};

	float3 *pvDirs = NULL;
	int iDirsCount = 0;
	int iAxeBasis = 0;
	if(m_isAxisLocked)
	{
		pvDirs = avDirAny;
		iDirsCount = ARRAYSIZE(avDirAny);
		iAxeBasis = HANDLER_AXE_CUSTOM;
	}
	else
	{
		pvDirs = avDirs;
		iDirsCount = ARRAYSIZE(avDirs);
		iAxeBasis = HANDLER_AXE_X;
	}

	for(int i = 0; i < iDirsCount; ++i)
	{
		SMPLANE p(pvDirs[i], float3());
		if(p.intersectLine(&vPt, vRayOrigin, end) && SMVector3Dot(vRayOrigin, vPt) >= 0.0f)
		{
			fDist = fabsf(SMVector3Length(vPt) - fSize);
			vCamDir2 = SMVector3Length2(vRayOrigin - vPt);
			if(fDist < fDelta && vCamDir2 < vCamDirMax)
			{
				m_vLastPoint = m_vBasePoint = (float3)(vPt * fScale + m_vPos);
				vCamDirMax = vCamDir2;
				m_currentAxe = (HANDLER_AXE)(iAxeBasis + i);
			}
		}
	}
	
	return(m_currentAxe != HANDLER_AXE_NONE);
}

void CGizmoRotate::setWorldRay(const float3 &vRayOrigin, const float3 &vRayDir)
{
	SMPLANE p(m_vCurDir, m_vPos);
	float3 vPos;

	if(p.intersectLine(&vPos, vRayOrigin, vRayOrigin + vRayDir * 1000.0f))
	{
		m_vLastPoint = vPos;
		float3 vA = SMVector3Normalize(vPos - m_vPos);
		float3 vB = SMVector3Normalize(m_vBasePoint - m_vPos);
		float fAngle = SMVector3Dot(vA, vB);
		fAngle = safe_acosf(fAngle);

		if(SMVector3Dot(SMVector3Normalize(SMVector3Cross(vA, vB)), m_vCurDir) < 0.0f)
		{
			fAngle = SM_2PI - fAngle;
		}

		fAngle = round_step(fAngle, 5.0f * SM_PIDIV180);

		SAFE_CALL(m_pCallback, onRotate, m_vCurDir, fAngle, this)
		else
		{
			setDeltaAngle(fAngle);
		}
	}
}
void CGizmoRotate::setTracking(bool isTracking)
{
	if(isTracking)
	{
		float3_t avDirs[] = {
			float3_t(),
			float3_t(1.0f, 0.0f, 0.0f),
			float3_t(0.0f, 1.0f, 0.0f),
			float3_t(0.0f, 0.0f, 1.0f),
			m_vLockAxis
		};
		m_vCurDir = avDirs[m_currentAxe];
		SAFE_CALL(m_pCallback, onStart, m_vCurDir, this);
	}
	else
	{
		setOrient(getOrient());
		SAFE_CALL(m_pCallback, onEnd, this);
	}
	m_isTracking = isTracking;
	m_pEditor->setDirty();
}

void XMETHODCALLTYPE CGizmoRotate::setDeltaAngle(float fAngle)
{
	m_fCurrentRotation = fAngle;
	m_pEditor->setDirty();
}

SMQuaternion XMETHODCALLTYPE CGizmoRotate::getOrient()
{
	if(!m_isTracking)
	{
		return(m_qBaseRotation);
	}
	return(m_qBaseRotation * SMQuaternion(m_vCurDir, m_fCurrentRotation));
}
