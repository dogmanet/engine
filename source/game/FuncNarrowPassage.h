#ifndef __FUNC_NARROW_PASSAGE_H
#define __FUNC_NARROW_PASSAGE_H

#include "BaseMover.h"

class CFuncNarrowPassage: public CBaseMover
{
	DECLARE_CLASS(CFuncNarrowPassage, CBaseMover);
	DECLARE_PROPTABLE();
public:
	DECLARE_TRIVIAL_CONSTRUCTOR();
	
private:
	void newMovementController(IMovementController **ppOut) override;
};

#endif
