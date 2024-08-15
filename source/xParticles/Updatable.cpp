#include "Updatable.h"

CUpdatable::CUpdatable(CParticleSystem *pSystem):
	m_pSystem(pSystem)
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
	return(m_pSystem->update(fDelta));
}

void CUpdatable::sync()
{
	m_pSystem->sync();
}
