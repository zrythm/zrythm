#include "global_state.h"

Zrythm *
GlobalState::getZrythm ()
{
  return Zrythm::getInstance ();
}