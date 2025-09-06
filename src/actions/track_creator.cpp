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

  // FIXME: need to pause the graph
  auto track_ref = track_factory_.create_empty_track (trackType);

  {
    auto * track = track_ref.get_object_base ();
    track->setName (
      get_unique_name_for_track (track->get_uuid (), track->get_name ())
        .to_qstring ());
  }

  undo_stack_.beginMacro (QObject::tr ("Add Track"));
  undo_stack_.push (
    new commands::AddEmptyTrackCommand (track_collection_, track_ref));

  // if audio output route to master
  std::visit (
    [&] (auto * track) {
      if (track->get_output_signal_type () == dsp::PortType::Audio)
        {
          undo_stack_.push (new commands::RouteTrackCommand (
            track_routing_, track_ref.id (),
            singleton_tracks_.masterTrack ()->get_uuid ()));
        }
    },
    track_ref.get_object ());
  undo_stack_.endMacro ();

  return QVariant::fromStdVariant (track_ref.get_object ());
}

void
TrackCreator::importFiles (
  const QStringList         &filePaths,
  double                     startTicks,
  structure::tracks::Track * track) const
{
  z_debug ("Importing {} files: {}", filePaths.size (), filePaths);

  if (filePaths.empty ())
    {
      return;
    }
}

void
TrackCreator::importPlugin (const plugins::PluginDescriptor * config) const
{
}

utils::Utf8String
TrackCreator::get_unique_name_for_track (
  const structure::tracks::Track::Uuid &track_to_skip,
  const utils::Utf8String              &name) const
{
  const auto name_is_unique = [&] (const utils::Utf8String &name_to_check) {
    auto track_ids_to_check = std::ranges::to<std::vector> (
      std::views::filter (track_collection_.tracks (), [&] (const auto &ref) {
        return ref.id () != track_to_skip;
      }));
    return !structure::tracks::TrackSpan{ track_ids_to_check }.contains_track_name (
      name_to_check);
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
