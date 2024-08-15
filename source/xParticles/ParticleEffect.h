#ifndef __PARTICLEEFFECT_H
#define __PARTICLEEFFECT_H

#include <xcommon/particles/IXParticleSystem.h>
#include "ParticleEffectEmitter.h"

class CParticleSystem;
class CParticleEffect final: public IXUnknownImplementation<IXParticleEffect>
{
public:
	CParticleEffect() = default;
	CParticleEffect(CParticleSystem *pSystem);
	~CParticleEffect();

	UINT XMETHODCALLTYPE getEmitterCount() override;
	void XMETHODCALLTYPE setEmitterCount(UINT uCount) override;
	IXParticleEffectEmitter* XMETHODCALLTYPE getEmitterAt(UINT uIdx) override;
	void XMETHODCALLTYPE removeEmitterAt(UINT uIdx) override;

	bool XMETHODCALLTYPE save() override;

	CParticleEffectEmitter* getEmitterAtInternal(UINT uIdx);

	void setName(const char *szName);
	const char* getName();

private:
	void XMETHODCALLTYPE FinalRelease() override;

private:
	CParticleSystem *m_pSystem = NULL;
	const char *m_szName = NULL;
	Array<CParticleEffectEmitter*> m_apEmitters;
};

#endif
