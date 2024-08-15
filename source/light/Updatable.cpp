#include "Updatable.h"

CUpdatable::CUpdatable(CLightSystem *pSystem):
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
	//m_pSystem->update(fDelta);

	return(-1);
}

void CUpdatable::sync()
{
	m_pSystem->sync();
}
