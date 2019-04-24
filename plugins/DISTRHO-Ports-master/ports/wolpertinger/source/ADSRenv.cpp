#include "ADSRenv.h"

ADSRenv::ADSRenv():
	curTime(0.0),
	noteReleased(false),
	curValue(0.0)
{
	setAttack(0.5);
	setDecay(0.5);
	setSustain(0.5);
	setRelease(0.5);
}

ADSRenv::~ADSRenv()
{
}


