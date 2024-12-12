#ifndef __INFO_PARTICLE_PLAYER_H
#define __INFO_PARTICLE_PLAYER_H

#include "PointEntity.h"
#include <xcommon/particles/IXParticleSystem.h>

//! Точка респауна игрока
class CInfoParticlePlayer: public CPointEntity
{
	DECLARE_CLASS(CInfoParticlePlayer, CPointEntity);
	DECLARE_PROPTABLE();
public:
	DECLARE_TRIVIAL_CONSTRUCTOR();
	~CInfoParticlePlayer();

	void setEffect(const char *szEffectFile);

	void setPos(const float3 &pos) override;
	void setOrient(const SMQuaternion &q) override;
private:
	const char *m_szEffect = "";
	IXParticlePlayer *m_pPlayer = NULL;
};

#endif
