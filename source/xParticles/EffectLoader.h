#ifndef __EFFECTLOADER_H
#define __EFFECTLOADER_H

#include "ParticleEffect.h"
#include <xcommon/IXCore.h>

class CEffectLoader final
{
public:
	CEffectLoader(IXCore *pCore);

	bool loadFromFile(const char *szFile, CParticleEffect *pEffect);
	bool saveToFile(const char *szFile, CParticleEffect *pEffect);

private:
	IXCore *m_pCore;
};

#endif
