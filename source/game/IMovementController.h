#ifndef __IMOVEMENTCONTROLLER_H
#define __IMOVEMENTCONTROLLER_H

#include <gdefines.h>

class CBaseCharacter;
class IMovementController: public IXUnknown
{
public:
	virtual void setCharacter(CBaseCharacter *pCharacter) = 0;

	virtual void handleMove(const float3 &vDir) = 0;
	virtual void handleJump() = 0;
	virtual bool handleUse() = 0;

	virtual void update(float fDt) = 0;
};

#endif
