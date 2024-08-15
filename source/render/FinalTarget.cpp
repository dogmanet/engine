#include "FinalTarget.h"
#include "Render.h"
#include "RenderGraph.h"

CFinalTarget::CFinalTarget(CRender *pRender, IGXDevice *pDevice, SXWINDOW hWnd):
	BaseClass(pRender, pDevice),
	m_hWnd(hWnd)
{
}
CFinalTarget::~CFinalTarget()
{
	mem_release(m_pSwapChain);
}

bool XMETHODCALLTYPE CFinalTarget::getTarget(IGXSurface **ppTarget)
{
	if(m_pSwapChain)
	{
		*ppTarget = m_pSwapChain->getColorTarget();
		return(true);
	}

	return(false);
}

void XMETHODCALLTYPE CFinalTarget::resize(UINT uWidth, UINT uHeight)
{
	// resize targets and swap chain
	mem_release(m_pSwapChain);
	m_pSwapChain = m_pDevice->createSwapChain(uWidth, uHeight, m_hWnd);

	BaseClass::resize(uWidth, uHeight);
}


void XMETHODCALLTYPE CFinalTarget::FinalRelease()
{
	m_pRender->onFinalTargetReleased(this);
}

void CFinalTarget::swapChain()
{
	SAFE_CALL(m_pSwapChain, swapBuffers);
}

IGXSwapChain* CFinalTarget::getSwapChain()
{
	return(m_pSwapChain);
}

void XMETHODCALLTYPE CFinalTarget::getInternalData(const XGUID *pGUID, void **ppOut)
{
	if(*pGUID == X_IS_FINAL_TARGET_GUID)
	{
		*ppOut = (void*)1;
	}
	else
	{
		BaseClass::getInternalData(pGUID, ppOut);
	}
}

bool CFinalTarget::isEnabled()
{
	if(!BaseClass::isEnabled())
	{
		return(false);
	}

	return(IsWindowVisible((HWND)m_hWnd));
}
