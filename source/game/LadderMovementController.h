#ifndef __LADDERMOVEMENTCONTROLLER_H
#define __LADDERMOVEMENTCONTROLLER_H

#include "IMovementController.h"

class CFuncLadder;
class CLadderMovementController: public IXUnknownImplementation<IMovementController>
{
public:
	CLadderMovementController(CFuncLadder *pLadder);
	~CLadderMovementController();

	void setCharacter(CBaseCharacter *pCharacter) override;

	void handleMove(const float3 &vDir) override;
	void handleJump() override;
	bool handleUse() override;

	void update(float fDt) override;

private:
	CBaseCharacter *m_pCharacter;

	float3_t m_vLadderPoint[2];
	float3_t m_vLadderDir;

	float3_t m_vMoveDir;

	struct
	{
		bool is = false;
		float fFrac = 0.0f;
		float3_t vStartPos;
		float3_t vTargetPos;
	}
	m_mounting;

	bool m_bWillDismount = false;
};

#endif
