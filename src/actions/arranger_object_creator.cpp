// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_object_creator.h"
#include "commands/add_region_to_clip_slot_command.h"

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
          obj->name ()->setName (
            track.generate_name_for_region (*obj).to_qstring ());

          undo_stack_.push (
            new commands::AddArrangerObjectCommand<ObjectT> (lane, obj_ref));
        }
    },
    obj_ref.get_object ());
}

void
ArrangerObjectCreator::add_object_to_clip_slot (
  structure::tracks::Track                           &track,
  structure::scenes::ClipSlot                        &slot,
  structure::arrangement::ArrangerObjectUuidReference obj_ref)
{
  std::visit (
    [&] (auto &&obj) {
      using ObjectT = base_type<decltype (obj)>;
      if constexpr (
        std::is_same_v<ObjectT, structure::arrangement::MidiRegion>
        || std::is_same_v<ObjectT, structure::arrangement::AudioRegion>)
        {
          obj->name ()->setName (
            track.generate_name_for_region (*obj).to_qstring ());

          undo_stack_.push (
            new commands::AddRegionToClipSlotCommand (slot, obj_ref));
        }
    },
    obj_ref.get_object ());
}

structure::arrangement::ChordRegion *
ArrangerObjectCreator::addEmptyChordRegion (
  structure::tracks::ChordTrack * track,
  double                          startTicks)
{
  auto cr_ref =
    arranger_object_factory_.get_builder<structure::arrangement::ChordRegion> ()
      .with_start_ticks (startTicks)
      .build_in_registry ();
  auto * chord_region =
    cr_ref.get_object_as<structure::arrangement::ChordRegion> ();
  chord_region->name ()->setName (
    track->generate_name_for_region (*chord_region));
  undo_stack_.push (
    new commands::AddArrangerObjectCommand<structure::arrangement::ChordRegion> (
      *track, cr_ref));
  return chord_region;
}

structure::arrangement::AutomationRegion *
ArrangerObjectCreator::addEmptyAutomationRegion (
  structure::tracks::Track *           track,
  structure::tracks::AutomationTrack * automationTrack,
  double                               startTicks)
{
  auto ar_ref =
    arranger_object_factory_
      .get_builder<structure::arrangement::AutomationRegion> ()
      .with_start_ticks (startTicks)
      .build_in_registry ();
  auto * ar = ar_ref.get_object_as<structure::arrangement::AutomationRegion> ();
  ar->name ()->setName (track->generate_name_for_region (*ar, automationTrack));
  undo_stack_.push (
    new commands::AddArrangerObjectCommand<
      structure::arrangement::AutomationRegion> (*automationTrack, ar_ref));
  return ar;
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
  mr->bounds ()->length ()->setTicks (static_cast<double> (r_len_ticks));
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

#if 0
  try
    {
      MidiFile mf{ utils::Utf8String::from_qstring (abs_path) };
      mf.into_region (*mr, midi_track_idx);
    }
  catch (const ZrythmException &e)
    {
      z_warning ("Failed to create region from MIDI file: {}", e.what ());
    }
#endif

  return mr;
}

structure::arrangement::MidiNote *
ArrangerObjectCreator::addMidiNote (
  structure::arrangement::MidiRegion * region,
  double                               startTicks,
  int                                  pitch)
{
  return add_editor_object (*region, startTicks, pitch);
}

structure::arrangement::AudioRegion *
ArrangerObjectCreator::addAudioRegionToClipSlotFromFile (
  structure::tracks::Track *    track,
  structure::scenes::ClipSlot * clipSlot,
  const QString                &absPath)
{
  auto ar_ref =
    arranger_object_factory_.create_audio_region_from_file (absPath, 0.0);
  add_object_to_clip_slot (*track, *clipSlot, ar_ref);
  return ar_ref.get_object_as<structure::arrangement::AudioRegion> ();
}

structure::arrangement::AudioRegion *
ArrangerObjectCreator::addMidiRegionToClipSlotFromFile (
  structure::tracks::Track *    track,
  structure::scenes::ClipSlot * clipSlot,
  const QString                &absPath)
{
  // TODO
  return nullptr;
}

} // namespace zrythm::actions
