#include "FuncLadder.h"
#include "BaseCharacter.h"
#include "LadderMovementController.h"


BEGIN_PROPTABLE(CInfoLadderDismount)
	// empty
END_PROPTABLE()

REGISTER_ENTITY(CInfoLadderDismount, info_ladder_dismount);

//##########################################################################

BEGIN_PROPTABLE(CFuncLadder)
	// empty
END_PROPTABLE()

REGISTER_ENTITY(CFuncLadder, func_ladder);

void CFuncLadder::newMovementController(IMovementController **ppOut)
{
	*ppOut = new CLadderMovementController(this);
}
