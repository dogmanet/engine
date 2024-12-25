#include "NarrowPassageMovementController.h"
#include "BaseCharacter.h"
#include "FuncNarrowPassage.h"
#include "Player.h"

CNarrowPassageMovementController::CNarrowPassageMovementController(CFuncNarrowPassage *pPassage)
{
	m_vLadderPoint[0] = pPassage->getPos();
	m_vLadderPoint[1] = pPassage->getUpPos();

	m_vLadderDir = SMVector3Normalize(m_vLadderPoint[1] - m_vLadderPoint[0]);

	m_fSpeed = pPassage->getSpeed();
}
CNarrowPassageMovementController::~CNarrowPassageMovementController()
{
	if(m_pCharacter)
	{
		m_pCharacter->getCharacterController()->setGravity(float3(0.0f, -10.0f, 0.0f));
	}
}

static float3 SMProjectPointOnLine(const float3 &vPos, const float3 &vStart, const float3 &vEnd)
{
	float3 vN = SMVector3Normalize(vEnd - vStart);
	float fDot0 = SMVector3Dot(vN, vPos - vStart);
	if(fDot0 <= 0.0f)
	{
		return(vStart);
	}

	float fDot1 = SMVector3Dot(vN, vPos - vEnd);
	if(fDot1 >= 0.0f)
	{
		return(vEnd);
	}

	return(vStart + vN * fDot0);
}

void CNarrowPassageMovementController::setCharacter(CBaseCharacter *pCharacter)
{
	m_pCharacter = pCharacter;

	IXCharacterController *pCharacterController = pCharacter->getCharacterController();

	pCharacterController->setGravity(float3(0.0f, 0.0f, 0.0f));
	pCharacterController->setVelocityForTimeInterval(float3(0.0f, 0.0f, 0.0f), 0.0f);

	m_mounting.is = true;
	m_mounting.fFrac = 0.0f;
	m_mounting.vStartPos = m_pCharacter->getPos();
	m_mounting.vTargetPos = SMProjectPointOnLine(m_pCharacter->getPos(), m_vLadderPoint[0], m_vLadderPoint[1]);
}

void CNarrowPassageMovementController::handleMove(const float3 &vDir)
{
	m_vMoveDir = vDir;
}

void CNarrowPassageMovementController::handleJump()
{
	m_bWillDismount = true;
}

bool CNarrowPassageMovementController::handleUse()
{
	m_bWillDismount = true;
	return(true);
}

void CNarrowPassageMovementController::update(float fDt)
{
	if(m_mounting.is)
	{
		m_mounting.fFrac += 7.0f * fDt;
		if(m_mounting.fFrac > 1.0f)
		{
			m_mounting.fFrac = 1.0f;
			m_mounting.is = false;
		}
		m_pCharacter->setPos(vlerp(m_mounting.vStartPos, m_mounting.vTargetPos, m_mounting.fFrac));
	}
	else if(m_bWillDismount)
	{
		((CPlayer*)m_pCharacter)->m_vCurrentSpeed = m_vMoveDir;
		m_pCharacter->setMovementController(NULL);
	}
	else if(!SMIsZero(SMVector3Length2(m_vMoveDir)))
	{
		float fDot = SMVector3Dot(m_vLadderDir, m_vMoveDir);

		float3 vSpeed = m_vLadderDir * m_fSpeed;
		float3 vNewPos;

		if(fDot > /*-SM_PIDIV4*/ -SMToRadian(10.0f))
		{
			if(SMIsZero(SMVector3Length2(m_vLadderPoint[1] - m_pCharacter->getPos())))
			{
				m_bWillDismount = true;
			}
			else
			{
				vNewPos = m_pCharacter->getPos() + vSpeed * fDt;
				if(SMVector3Length2(vNewPos - m_vLadderPoint[0]) > SMVector3Length2(m_vLadderPoint[1] - m_vLadderPoint[0]))
				{
					vNewPos = m_vLadderPoint[1];
				}
			}
		}
		else
		{
			if(SMIsZero(SMVector3Length2(m_vLadderPoint[0] - m_pCharacter->getPos())))
			{
				m_bWillDismount = true;
			}
			else
			{
				vNewPos = m_pCharacter->getPos() - vSpeed * fDt;
				if(SMVector3Length2(vNewPos - m_vLadderPoint[1]) > SMVector3Length2(m_vLadderPoint[1] - m_vLadderPoint[0]))
				{
					vNewPos = m_vLadderPoint[0];
				}
			}
		}

		if(!m_bWillDismount)
		{
			m_pCharacter->setPos(vNewPos);
		}
	}
}
