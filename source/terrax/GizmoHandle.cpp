#include "GizmoHandle.h"
#include "Editor.h"
#include "terrax.h"

CGizmoHandle::CGizmoHandle(CEditor *pEditor):
	m_pEditor(pEditor)
{
	m_pEditor->setDirty();
}
CGizmoHandle::~CGizmoHandle()
{
	m_pEditor->onGizmoRemoved(this);

	m_pEditor->setDirty();
}

void XMETHODCALLTYPE CGizmoHandle::enable(bool yesNo)
{
	if(m_isEnabled != yesNo)
	{
		m_isEnabled = yesNo;
		m_pEditor->setDirty();
	}
}

void XMETHODCALLTYPE CGizmoHandle::setPos(const float3_t &vPos)
{
	m_vPos = vPos;

	m_pEditor->setDirty();

	if(m_isTracking)
	{
		if(m_isLockedNormal)
		{
			m_planeBase = SMPLANE(m_vLockNormal, m_vPos);
		}
		else
		{
			m_planeBase = SMPLANE(m_vBestPlaneNormal, m_vPos);
		}
	}
}
const float3_t& XMETHODCALLTYPE CGizmoHandle::getPos()
{
	return(m_vPos);
}

void XMETHODCALLTYPE CGizmoHandle::lockInPlane(const float3_t &vPlaneNormal)
{
	unLock();

	m_vLockNormal = SMVector3Normalize(vPlaneNormal);
	m_isLockedNormal = true;
}
void XMETHODCALLTYPE CGizmoHandle::lockInDir(const float3_t &vDir)
{
	unLock();

	m_vLockNormal = SMVector3Normalize(vDir);
	m_isLockedDir = true;
}
void XMETHODCALLTYPE CGizmoHandle::unLock()
{
	m_isLockedNormal = false;
	m_isLockedDir = false;
}

void CGizmoHandle::draw(IXGizmoRenderer *pGRBoth, IXGizmoRenderer *pGR2D, IXGizmoRenderer *pGR3D)
{
	if(m_isRendered)
	{
		pGRBoth->setPointMode(XGPM_SQUARE);
		pGRBoth->setColor(m_vColor);
		pGRBoth->setPointSize(getOnscreenSize());
		pGRBoth->drawPoint(m_vPos);
	}

	if(m_isLockedDir)
	{
		drawRay(pGR3D);
	}
	else
	{
		drawGrid(pGR3D);
	}
}

bool CGizmoHandle::wantHandle(const float3 &vRayOrigin, const float3 &vRayDir)
{
	if(m_isLockedDir)
	{
		// Skip parallel lines
		if(fabsf(SMVector3Dot(vRayDir, m_vLockNormal)) > 0.999f)
		{
			return(false);
		}
	}
	else if(m_isLockedNormal)
	{
		if(fabsf(SMVector3Dot(vRayDir, m_vLockNormal)) < 0.001f)
		{
			return(false);
		}
	}
	return(true);
}

void CGizmoHandle::setWorldRay(const float3 &vRayOrigin, const float3 &vRayDir)
{
	if(m_isLockedDir)
	{
		// Skip parallel lines
		if(fabsf(SMVector3Dot(vRayDir, m_vLockNormal)) > 0.999f)
		{
			return;
		}

		float3 vNewPos;
		SMCrossLines(vRayOrigin, vRayDir, m_vPos, m_vLockNormal, NULL, &vNewPos);

		float fGridStep = XGetGridStep();
		if(fGridStep > 0.0f && g_xConfig.m_bSnapGrid)
		{
			// world center in projection on the line
			float3 o = vNewPos + m_vLockNormal * SMVector3Dot(-vNewPos, m_vLockNormal);
			float fOrigin = SMVector3Length(vNewPos - o);
			if(SMVector3Dot(m_vLockNormal, vNewPos - o) < 0.0f)
			{
				fOrigin *= -1;
			}
			vNewPos = o + m_vLockNormal * round_step(fOrigin, fGridStep);
		}

		SAFE_CALL(m_pCallback, moveTo, vNewPos, this)
		else
		{
			setPos(vNewPos);
		}

	}
	else
	{

		float3 vDir = vRayDir * 1000.0f;
		float3 vPoint;
		if(m_planeBase.intersectLine(&vPoint, vRayOrigin - vDir, vRayOrigin + vDir))
		{
			float3 vT, vB, vN = SMVector3Normalize(m_planeBase);
			if(fabsf(SMVector3Dot(vN, float3(0.0f, 1.0f, 0.0f))) > 0.9f)
			{
				vT = float3(1.0f, 0.0f, 0.0f);
			}
			else
			{
				vT = float3(0.0f, 1.0f, 0.0f);
			}
			vB = SMVector3Normalize(SMVector3Cross(vT, vN));
			vT = SMVector3Cross(vN, vB);

			float fGridStep = XGetGridStep();
			if(fGridStep > 0.0f)
			{
				m_mLocal = SMMATRIX(SMMatrixTranspose(SMMATRIX(
					float4(vT, 0.0f),
					float4(vB, 0.0f),
					float4(vN, 0.0f),
					float4(0.0f, 0.0f, 0.0f, 1.0f)
					)));
				m_mInvLocal = SMMatrixInverse(NULL, m_mLocal);

				if(g_xConfig.m_bSnapGrid)
				{
					vPoint = vPoint * m_mLocal;
					vPoint.x = round_step(vPoint.x, fGridStep);
					vPoint.y = round_step(vPoint.y, fGridStep);
					vPoint = vPoint * m_mInvLocal;
				}
			}

			
			SAFE_CALL(m_pCallback, moveTo, vPoint, this)
			else
			{
				setPos(vPoint);
			}
		}
	}
}
void CGizmoHandle::setTracking(bool isTracking)
{
	m_isTracking = isTracking;

	if(isTracking)
	{
		if(m_isLockedNormal)
		{
			m_planeBase = SMPLANE(m_vLockNormal, m_vPos);
		}
		else
		{
			m_planeBase = SMPLANE(m_vBestPlaneNormal, m_vPos);
		}

		SAFE_CALL(m_pCallback, onStart, this);
	}
	else
	{
		SAFE_CALL(m_pCallback, onEnd, this);
	}

	m_pEditor->setDirty();
}

void CGizmoHandle::drawGrid(IXGizmoRenderer *pGR3D)
{
	if(!m_isTracking)
	{
		return;
	}

	float3 vN = m_planeBase;

	float3 vOrigin;
	if(m_planeBase.intersectLine(&vOrigin, vN * -1000.0f, vN * 1000.0f))
	{
		float fGridStep = XGetGridStep();
		if(fGridStep > 0.0f)
		{
			float3 vGridCenter = m_vPos * m_mLocal;
			vGridCenter.x -= fmodf(vGridCenter.x, fGridStep);
			vGridCenter.y -= fmodf(vGridCenter.y, fGridStep);

			//vGridCenter = vGridCenter * mInv;

			pGR3D->setLineWidth(1.0f);

			const int iSize = 10;

			float fInvMax = 1.0f / SMVector3Length2(float3((float)iSize, (float)iSize, 0.0f) * fGridStep);

			float3 vPt;
			for(int i = -iSize; i <= iSize; ++i)
			{
				vPt = float3(vGridCenter + float3((float)i, (float)-iSize, 0.0f) * fGridStep) * m_mInvLocal;
				pGR3D->setColor(float4(1.0f, 1.0f, 1.0f, 1.0f - SMVector3Length2(vPt - m_vPos) * fInvMax));
				pGR3D->jumpTo(vPt);

				vPt = float3(vGridCenter + float3((float)i, 0.0f, 0.0f) * fGridStep) * m_mInvLocal;
				pGR3D->setColor(float4(1.0f, 1.0f, 1.0f, 1.0f - SMVector3Length2(vPt - m_vPos) * fInvMax));
				pGR3D->lineTo(vPt);

				vPt = float3(vGridCenter + float3((float)i, (float)iSize, 0.0f) * fGridStep) * m_mInvLocal;
				pGR3D->setColor(float4(1.0f, 1.0f, 1.0f, 1.0f - SMVector3Length2(vPt - m_vPos) * fInvMax));
				pGR3D->lineTo(vPt);
			}
			for(int j = -iSize; j <= iSize; ++j)
			{
				vPt = float3(vGridCenter + float3((float)-iSize, (float)j, 0.0f) * fGridStep) * m_mInvLocal;
				pGR3D->setColor(float4(1.0f, 1.0f, 1.0f, 1.0f - SMVector3Length2(vPt - m_vPos) * fInvMax));
				pGR3D->jumpTo(vPt);

				vPt = float3(vGridCenter + float3(0.0f, (float)j, 0.0f) * fGridStep) * m_mInvLocal;
				pGR3D->setColor(float4(1.0f, 1.0f, 1.0f, 1.0f - SMVector3Length2(vPt - m_vPos) * fInvMax));
				pGR3D->lineTo(vPt);

				vPt = float3(vGridCenter + float3((float)iSize, (float)j, 0.0f) * fGridStep) * m_mInvLocal;
				pGR3D->setColor(float4(1.0f, 1.0f, 1.0f, 1.0f - SMVector3Length2(vPt - m_vPos) * fInvMax));
				pGR3D->lineTo(vPt);
			}
		}
	}
}

void CGizmoHandle::drawRay(IXGizmoRenderer *pGR3D)
{
	if(!m_isTracking)
	{
		return;
	}

	// m_vLockNormal

	float3 vLen = m_vLockNormal;

	const int iSize = 10;

	float fGridStep = XGetGridStep();
	if(fGridStep > 0.0f)
	{
		vLen *= fGridStep * (float)iSize;
	}
	else
	{
		vLen *= (float)iSize;
	}

	// world center in projection on the line
	float3 o = m_vPos + m_vLockNormal * SMVector3Dot(-m_vPos, m_vLockNormal);
	float fOrigin = SMVector3Length(m_vPos - o);
	if(SMVector3Dot(m_vLockNormal, m_vPos - o) < 0.0f)
	{
		fOrigin *= -1;
	}
	float3 vOrigin = m_vPos;

	if(fGridStep)
	{
		vOrigin = o + m_vLockNormal * round_step(fOrigin, fGridStep);
	}

	pGR3D->setLineWidth(1.0f);
	pGR3D->setColor(float4(1.0f, 1.0f, 1.0f, 0.0f));
	pGR3D->jumpTo(vOrigin - vLen);
	pGR3D->setColor(float4(1.0f, 1.0f, 1.0f, 1.0f));
	pGR3D->lineTo(vOrigin);
	pGR3D->setColor(float4(1.0f, 1.0f, 1.0f, 0.0f));
	pGR3D->lineTo(vOrigin + vLen);


	if(fGridStep > 0.0f)
	{

		float fInvMax = 1.0f / SMVector3Length2(float3((float)iSize, (float)iSize, 0.0f) * fGridStep);

		pGR3D->setPointSize(5.0f);
		pGR3D->setPointMode(XGPM_ROUND);

		for(int i = -iSize; i <= iSize; ++i)
		{
			pGR3D->setColor(float4(1.0f, 1.0f, 1.0f, 1.0f - fabsf(i) / (float)iSize));
			pGR3D->drawPoint(vOrigin + m_vLockNormal * (float)i * fGridStep);
		}
	}
}

void XMETHODCALLTYPE CGizmoHandle::setColor(const float4_t &vColor)
{
	m_vColor = vColor;

	if(m_isEnabled)
	{
		m_pEditor->setDirty();
	}
}
