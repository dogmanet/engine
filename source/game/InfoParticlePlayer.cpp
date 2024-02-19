#include "InfoParticlePlayer.h"

/*! \skydocent info_particle_player
Камера
*/

BEGIN_PROPTABLE(CInfoParticlePlayer)
	//! Файл эффекта
	DEFINE_FIELD_STRINGFN(m_szEffect, 0, "file", "Effect file", setEffect, EDITOR_EFFECT)
END_PROPTABLE()

REGISTER_ENTITY(CInfoParticlePlayer, info_particle_player);

CInfoParticlePlayer::~CInfoParticlePlayer()
{
	mem_release(m_pPlayer);
}

void CInfoParticlePlayer::setEffect(const char *szEffectFile)
{
	_setStrVal(&m_szEffect, szEffectFile);

	IXParticleSystem *pPS = GetParticleSystem();

	mem_release(m_pPlayer);
	IXParticleEffect *pEffect;
	if(pPS->getEffect(szEffectFile, &pEffect))
	{
		IXParticlePlayer *pPlayer;
		pPS->newEffectPlayer(pEffect, &m_pPlayer);

		m_pPlayer->setPos(getPos());
		m_pPlayer->setOrient(getOrient());

		m_pPlayer->play();

		mem_release(pEffect);
	}
}

void CInfoParticlePlayer::setPos(const float3 &pos)
{
	BaseClass::setPos(pos);
	SAFE_CALL(m_pPlayer, setPos, pos);
}

void CInfoParticlePlayer::setOrient(const SMQuaternion &q)
{
	BaseClass::setOrient(q);
	SAFE_CALL(m_pPlayer, setOrient, q);
}
