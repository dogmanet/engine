#include "FuncNarrowPassage.h"
#include "BaseCharacter.h"
#include "NarrowPassageMovementController.h"


BEGIN_PROPTABLE(CFuncNarrowPassage)
	// empty
END_PROPTABLE()

REGISTER_ENTITY(CFuncNarrowPassage, func_narrow_passage);

void CFuncNarrowPassage::newMovementController(IMovementController **ppOut)
{
	*ppOut = new CNarrowPassageMovementController(this);
}
