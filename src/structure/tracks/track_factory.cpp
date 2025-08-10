// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/actions/tracklist_selections_action.h"
#include "gui/backend/backend/actions/undo_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/tracks/track_factory.h"

namespace zrythm::structure::tracks
{
QVariant
TrackFactory::addEmptyTrackFromType (Track::Type tt)
{
  //   if (zrythm_app->check_and_show_trial_limit_error ())
  //     return;

  // auto track_ref = create_empty_track (tt);

  try
    {
      UNDO_MANAGER->perform (new gui::actions::CreateTracksAction (
        tt, nullptr, nullptr, TRACKLIST->track_count (), nullptr, 1, -1));

      return std::visit (
        [] (auto &&track) -> QVariant {
          using TrackT = base_type<decltype (track)>;
          z_debug (
            "created {} track: {}", typename_to_string<TrackT> (),
            track->get_name ());
          return QVariant::fromValue (track);
        },
        TRACKLIST->get_track_span ().back ());
    }
  catch (const ZrythmException &e)
    {
      e.handle (QObject::tr ("Failed to create track"));
    }
  // TODO
  return {};
}
} // namespace zrythm::structure::tracks
