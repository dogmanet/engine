#ifndef __GIZMO_HANDLE_H
#define __GIZMO_HANDLE_H

#include <xcommon/editor/IXEditorGizmo.h>
#include <xcommon/render/IXRenderUtils.h>


float MulDpiF(float fUnscaled, UINT uCurrentDpi);

class CEditor;
class CGizmoHandle final: public IXUnknownImplementation<IXEditorGizmoHandle>
{
public:
	CGizmoHandle(CEditor *pEditor);
	~CGizmoHandle();

	void XMETHODCALLTYPE enable(bool yesNo) override;
	bool XMETHODCALLTYPE isEnabled() override
	{
		return(m_isEnabled);
	}

	void XMETHODCALLTYPE setPos(const float3_t &vPos) override;
	const float3_t& XMETHODCALLTYPE getPos() override;

	void XMETHODCALLTYPE lockInPlane(const float3_t &vPlaneNormal) override;
	void XMETHODCALLTYPE lockInDir(const float3_t &vDir) override;
	void XMETHODCALLTYPE unLock() override;

	void draw(IXGizmoRenderer *pGRBoth, IXGizmoRenderer *pGR2D, IXGizmoRenderer *pGR3D);

	float getOnscreenSize() const
	{
		extern UINT g_uWndMainDpi;

		return(MulDpiF(7.0f, g_uWndMainDpi));
	}

	void setBestPlaneNormal(const float3_t &vNormal)
	{
		m_vBestPlaneNormal = vNormal;
	}
	void setWorldRay(const float3 &vRayOrigin, const float3 &vRayDir);
	void setTracking(bool isTracking);

	void XMETHODCALLTYPE setCallback(IXEditorGizmoHandleCallback *pCallback) override
	{
		m_pCallback = pCallback;
	}

	bool wantHandle(const float3 &vRayOrigin, const float3 &vRayDir);

	void setRendered(bool set)
	{
		m_isRendered = set;
	}

	void XMETHODCALLTYPE setColor(const float4_t &vColor) override;

	SX_ALIGNED_OP_MEM();

private:
	CEditor *m_pEditor;
	bool m_isEnabled = true;

	float3_t m_vPos;

	float3_t m_vBestPlaneNormal;

	bool m_isLockedNormal = false;
	bool m_isLockedDir = false;
	float3_t m_vLockNormal;

	SMPLANE m_planeBase;

	bool m_isTracking = false;

	SMMATRIX m_mLocal;
	SMMATRIX m_mInvLocal;

	IXEditorGizmoHandleCallback *m_pCallback = NULL;

	bool m_isRendered = true;

	float4_t m_vColor = float4_t(1.0f, 1.0f, 1.0f, 1.0f);

private:

	void drawGrid(IXGizmoRenderer *pGR3D);
	void drawRay(IXGizmoRenderer *pGR3D);
};

#endif
