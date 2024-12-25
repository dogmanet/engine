#ifndef __FUNC_LADDER_H
#define __FUNC_LADDER_H

#include "BaseMover.h"

class CInfoLadderDismount: public CPointEntity
{
	DECLARE_CLASS(CInfoLadderDismount, CPointEntity);
	DECLARE_PROPTABLE();
public:
	DECLARE_TRIVIAL_CONSTRUCTOR();
};

//##########################################################################

class CFuncLadder: public CBaseMover
{
	DECLARE_CLASS(CFuncLadder, CBaseMover);
	DECLARE_PROPTABLE();
public:
	DECLARE_TRIVIAL_CONSTRUCTOR();
	
private:
	void newMovementController(IMovementController **ppOut) override;
};

#endif
