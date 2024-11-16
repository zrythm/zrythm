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
      auto * track_base = Track::create_empty_with_action (tt);
      std::visit (
        [&] (auto &&track) {
          using TrackT = base_type<decltype (track)>;
          z_debug (
            "created {} track: {}", typename_to_string<TrackT> (),
            track->get_name ());
        },
        convert_to_variant<TrackPtrVariant> (track_base));
    }
  catch (const ZrythmException &e)
    {
      e.handle (QObject::tr ("Failed to create track"));
    }
}