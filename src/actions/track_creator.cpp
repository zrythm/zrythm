// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
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
      get_unique_name_for_track (track->get_uuid (), track->get_name ())
        .to_qstring ());
  }

  undo_stack_.beginMacro (QObject::tr ("Add Track"));
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
  undo_stack_.endMacro ();

  return QVariant::fromValue (track_ref.get ());
}

utils::Utf8String
TrackCreator::get_unique_name_for_track (
  const structure::tracks::Track::Uuid &track_to_skip,
  const utils::Utf8String              &name) const
{
  const auto name_is_unique = [&] (const utils::Utf8String &name_to_check) {
    return !std::ranges::any_of (
      track_collection_.tracks (), [&] (const auto &ref) {
        return ref.id () != track_to_skip
               && ref.get ()->get_name () == name_to_check;
      });
  };

  auto new_name = name;
  while (!name_is_unique (new_name))
    {
      auto [ending_num, name_without_num] = new_name.get_int_after_last_space ();
      if (ending_num == -1)
        {
          new_name += u8" 1";
        }
      else
        {
          new_name = utils::Utf8String::from_utf8_encoded_string (
            fmt::format ("{} {}", name_without_num, ending_num + 1));
        }
    }
  return new_name;
}
} // namespace zrythm::actions
