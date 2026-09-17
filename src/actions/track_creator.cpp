// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/track_creator.h"
#include "commands/route_track_command.h"

namespace zrythm::actions
{
QVariant
TrackCreator::addEmptyTrackFromType (
  structure::tracks::Track::Type trackType) const
{
  // throw if attempted to re-add a singleton track
  if (!structure::tracks::Track::type_is_deletable (trackType))
    {
      throw std::invalid_argument (
        fmt::format (
          "cannot re-add track of type {} when it already exists", trackType));
    }

  auto track_ref = track_factory_.create_empty_track (trackType);

  {
    auto * track = track_ref.get ();
    track->setName (
      track_collection_
        .get_unique_name_for_track (track->get_uuid (), track->get_name ())
        .to_qstring ());
  }

  undo::UndoStack::ScopedMacro macro (undo_stack_, QObject::tr ("Add Track"));
  undo_stack_.push (
    new commands::AddEmptyTrackCommand (track_collection_, track_ref));

  // if audio output route to master
  {
    auto * track = track_ref.get ();
    if (track->output_signal_type () == dsp::PortType::Audio)
      {
        undo_stack_.push (new commands::RouteTrackCommand (
          track_routing_, track_ref.id (),
          singleton_tracks_.masterTrack ()->get_uuid ()));
      }
  }

  return QVariant::fromValue (track_ref.get ());
}
} // namespace zrythm::actions
