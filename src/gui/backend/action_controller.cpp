#include "common/dsp/track_all.h"

#include "action_controller.h"

ActionController::ActionController (QObject * parent) : QObject (parent) { }

void
ActionController::createEmptyTrack (int type)
{
  Track::Type tt = ENUM_INT_TO_VALUE (Track::Type, type);
  //   if (zrythm_app->check_and_show_trial_limit_error ())
  //     return;

  try
    {
      Track::create_empty_with_action (tt);
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to create track"));
    }
}