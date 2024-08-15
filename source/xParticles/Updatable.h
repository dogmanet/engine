#ifndef __UPDATABLE_H
#define __UPDATABLE_H

#include <xcommon/IXUpdatable.h>
#include "ParticleSystem.h"

class CUpdatable: public IXUnknownImplementation<IXUpdatable>
{
public:
	CUpdatable(CParticleSystem *pSystem);

	UINT startup() override;
	void shutdown() override;

	ID run(float fDelta) override;
	void sync() override;

protected:
	CParticleSystem *m_pSystem;
};

#endif
