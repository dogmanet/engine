#include "Updatable.h"

CUpdatable::CUpdatable(CAnimatedModelProvider *pAnimatedModelProvider, CDynamicModelProvider *pDynamicModelProvider, CDecalProvider *pDecalProvider):
	m_pAnimatedModelProvider(pAnimatedModelProvider),
	m_pDynamicModelProvider(pDynamicModelProvider),
	m_pDecalProvider(pDecalProvider)
{

}

UINT CUpdatable::startup()
{
	return(20);
}

void CUpdatable::shutdown()
{
}


ID CUpdatable::run(float fDelta)
{
	m_pAnimatedModelProvider->update(fDelta);

	m_pDynamicModelProvider->update();

	m_pDecalProvider->update();

	return(-1);
}

void CUpdatable::sync()
{
	m_pAnimatedModelProvider->sync();
	m_pDynamicModelProvider->sync();
	//m_pDecalProvider->update();
}
