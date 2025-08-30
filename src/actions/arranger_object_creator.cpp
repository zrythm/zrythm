// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_object_creator.h"
#include "gui/backend/io/midi_file.h"

namespace zrythm::actions
{
void
ArrangerObjectCreator::add_laned_object (
  structure::tracks::Track                           &track,
  structure::tracks::TrackLane                       &lane,
  structure::arrangement::ArrangerObjectUuidReference obj_ref)
{
  std::visit (
    [&] (auto &&obj) {
      using ObjectT = base_type<decltype (obj)>;
      if constexpr (
        std::is_same_v<ObjectT, structure::arrangement::MidiRegion>
        || std::is_same_v<ObjectT, structure::arrangement::AudioRegion>)
        {
          obj->regionMixin ()->name ()->setName (
            track.generate_name_for_region (*obj).to_qstring ());

          undo_stack_.push (
            new commands::AddArrangerObjectCommand<ObjectT> (lane, obj_ref));
          clip_editor_.set_region (obj->get_uuid (), track.get_uuid ());
        }
    },
    obj_ref.get_object ());
}

structure::arrangement::MidiRegion *
ArrangerObjectCreator::addMidiRegionFromChordDescriptor (
  structure::tracks::Track *     track,
  structure::tracks::TrackLane * lane,
  const dsp::ChordDescriptor    &descr,
  double                         startTicks)
{
  auto *     mr = addEmptyMidiRegion (track, lane, startTicks);
  const auto r_len_ticks =
    snap_grid_timeline_.defaultTicks (static_cast<int64_t> (startTicks));
  mr->regionMixin ()->bounds ()->length ()->setTicks (
    static_cast<double> (r_len_ticks));
  const auto mn_len_ticks =
    snap_grid_editor_.defaultTicks (static_cast<int64_t> (startTicks));

  /* create midi notes */
  for (const auto i : std::views::iota (0_zu, dsp::ChordDescriptor::MAX_NOTES))
    {
      if (descr.notes_.at (i))
        {
          auto mn =
            arranger_object_factory_
              .get_builder<structure::arrangement::MidiNote> ()
              .with_pitch (static_cast<int> (i) + 36)
              .with_velocity (structure::arrangement::MidiNote::DEFAULT_VELOCITY)
              .with_start_ticks (0)
              .with_end_ticks (mn_len_ticks)
              .build_in_registry ();
          mr->add_object (mn);
        }
    }

  return mr;
}

structure::arrangement::MidiRegion *
ArrangerObjectCreator::addMidiRegionFromMidiFile (
  structure::tracks::Track *     track,
  structure::tracks::TrackLane * lane,
  const QString                 &abs_path,
  double                         startTicks,
  int                            midi_track_idx)
{
  auto * mr = addEmptyMidiRegion (track, lane, startTicks);

  try
    {
      MidiFile mf{ utils::Utf8String::from_qstring (abs_path) };
      mf.into_region (*mr, midi_track_idx);
    }
  catch (const ZrythmException &e)
    {
      z_warning ("Failed to create region from MIDI file: {}", e.what ());
    }

  return mr;
}
} // namespace zrythm::actions
