#include "Updatable.h"

CUpdatable::CUpdatable(CMaterialSystem *pSystem):
	m_pSystem(pSystem)
{
}

UINT CUpdatable::startup()
{
	return(90);
}

void CUpdatable::shutdown()
{
}

ID CUpdatable::run(float fDelta)
{
	m_pSystem->update(fDelta);

	return(-1);
}

void CUpdatable::sync()
{
}
