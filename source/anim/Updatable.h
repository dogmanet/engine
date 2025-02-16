#ifndef __UPDATABLE_H
#define __UPDATABLE_H

#include <xcommon/IXUpdatable.h>
#include "AnimatedModelProvider.h"
#include "DynamicModelProvider.h"
#include "DecalProvider.h"

class CUpdatable: public IXUnknownImplementation<IXUpdatable>
{
public:
	CUpdatable(CAnimatedModelProvider *pAnimatedModelProvider, CDynamicModelProvider *pDynamicModelProvider, CDecalProvider *pDecalProvider);

	UINT startup() override;
	void shutdown() override;

	ID run(float fDelta) override;
	void sync() override;

protected:
	CAnimatedModelProvider *m_pAnimatedModelProvider;
	CDynamicModelProvider *m_pDynamicModelProvider;
	CDecalProvider *m_pDecalProvider;
};

#endif
