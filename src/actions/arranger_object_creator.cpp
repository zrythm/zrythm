// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"

#include "actions/arranger_object_creator.h"
#include "commands/add_region_to_clip_slot_command.h"
#include "utils/variant_helpers.h"

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
      using ObjectT = utils::base_type<decltype (obj)>;
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
    utils::convert_to_variant_qobj<
      structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ()));
}

void
ArrangerObjectCreator::add_object_to_clip_slot (
  structure::tracks::Track                           &track,
  structure::scenes::ClipSlot                        &slot,
  structure::arrangement::ArrangerObjectUuidReference obj_ref)
{
  std::visit (
    [&] (auto &&obj) {
      using ObjectT = utils::base_type<decltype (obj)>;
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
    utils::convert_to_variant_qobj<
      structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ()));
}

structure::arrangement::Marker *
ArrangerObjectCreator::addMarker (
  structure::arrangement::Marker::MarkerType markerType,
  structure::tracks::MarkerTrack *           markerTrack,
  const QString                             &name,
  double                                     startTicks)
{
  auto marker_ref =
    arranger_object_factory_.get_builder<structure::arrangement::Marker> ()
      .with_start_ticks (units::ticks (startTicks))
      .with_name (name)
      .with_marker_type (markerType)
      .build_in_registry ();
  undo_stack_.push (
    new commands::AddArrangerObjectCommand<structure::arrangement::Marker> (
      *markerTrack, marker_ref));
  return marker_ref.get_object_as<structure::arrangement::Marker> ();
}

structure::arrangement::TempoObject *
ArrangerObjectCreator::addTempoObject (
  structure::arrangement::TempoObjectManager *   tempoObjectManager,
  double                                         bpm,
  structure::arrangement::TempoObject::CurveType curveType,
  double                                         startTicks)
{
  auto tempo_object_ref =
    arranger_object_factory_.get_builder<structure::arrangement::TempoObject> ()
      .with_start_ticks (units::ticks (startTicks))
      .build_in_registry ();
  undo_stack_.push (
    new commands::AddTempoMapAffectingArrangerObjectCommand<
      structure::arrangement::TempoObject> (
      *tempoObjectManager, tempo_object_ref));
  return tempo_object_ref.get_object_as<structure::arrangement::TempoObject> ();
}

structure::arrangement::TimeSignatureObject *
ArrangerObjectCreator::addTimeSignatureObject (
  structure::arrangement::TempoObjectManager * tempoObjectManager,
  int                                          numerator,
  int                                          denominator,
  double                                       startTicks)
{
  auto time_signature_object_ref =
    arranger_object_factory_
      .get_builder<structure::arrangement::TimeSignatureObject> ()
      .with_start_ticks (units::ticks (startTicks))
      .build_in_registry ();
  undo_stack_.push (
    new commands::AddTempoMapAffectingArrangerObjectCommand<
      structure::arrangement::TimeSignatureObject> (
      *tempoObjectManager, time_signature_object_ref));
  return time_signature_object_ref
    .get_object_as<structure::arrangement::TimeSignatureObject> ();
}

structure::arrangement::MidiRegion *
ArrangerObjectCreator::addEmptyMidiRegion (
  structure::tracks::Track *     track,
  structure::tracks::TrackLane * lane,
  double                         startTicks)
{
  auto mr_ref =
    arranger_object_factory_.get_builder<structure::arrangement::MidiRegion> ()
      .with_start_ticks (units::ticks (startTicks))
      .build_in_registry ();
  add_laned_object (*track, *lane, mr_ref);
  return mr_ref.get_object_as<structure::arrangement::MidiRegion> ();
}

structure::arrangement::ChordRegion *
ArrangerObjectCreator::addEmptyChordRegion (
  structure::tracks::ChordTrack * track,
  double                          startTicks)
{
  auto cr_ref =
    arranger_object_factory_.get_builder<structure::arrangement::ChordRegion> ()
      .with_start_ticks (units::ticks (startTicks))
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
      .with_start_ticks (units::ticks (startTicks))
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
  for (
    const auto i : std::views::iota (
      static_cast<decltype (dsp::ChordDescriptor::MAX_NOTES)> (0),
      dsp::ChordDescriptor::MAX_NOTES))
    {
      if (descr.notes_.at (i))
        {
          auto mn =
            arranger_object_factory_
              .get_builder<structure::arrangement::MidiNote> ()
              .with_pitch (static_cast<int> (i) + 36)
              .with_velocity (structure::arrangement::MidiNote::DEFAULT_VELOCITY)
              .with_start_ticks (units::ticks (0))
              .with_end_ticks (units::ticks (mn_len_ticks))
              .build_in_registry ();
          mr->ArrangerObjectOwner<structure::arrangement::MidiNote>::add_object (
            mn);
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
  return add_editor_object<structure::arrangement::MidiNote> (
    *region, units::ticks (startTicks), pitch);
}

structure::arrangement::AutomationPoint *
ArrangerObjectCreator::addAutomationPoint (
  structure::arrangement::AutomationRegion * region,
  double                                     startTicks,
  double                                     value)
{
  return add_editor_object<structure::arrangement::AutomationPoint> (
    *region, units::ticks (startTicks), value);
}

structure::arrangement::ChordObject *
ArrangerObjectCreator::addChordObject (
  structure::arrangement::ChordRegion * region,
  double                                startTicks,
  const int                             chordIndex)
{
  return add_editor_object<structure::arrangement::ChordObject> (
    *region, units::ticks (startTicks), chordIndex);
}

structure::arrangement::MidiRegion *
ArrangerObjectCreator::addEmptyMidiRegionToClip (
  structure::tracks::Track *    track,
  structure::scenes::ClipSlot * clipSlot)
{
  auto mr_ref =
    arranger_object_factory_.get_builder<structure::arrangement::MidiRegion> ()
      .with_start_ticks (units::ticks (0))
      .build_in_registry ();
  clipSlot->setRegion (
    mr_ref.get_object_as<structure::arrangement::MidiRegion> ());
  return mr_ref.get_object_as<structure::arrangement::MidiRegion> ();
}

structure::arrangement::AudioRegion *
ArrangerObjectCreator::add_audio_region_with_clip (
  structure::tracks::Track         &track,
  structure::tracks::TrackLane     &lane,
  dsp::FileAudioSourceUuidReference clip_id,
  units::precise_tick_t             start_ticks)
{
  auto obj_ref = arranger_object_factory_.create_audio_region_with_clip (
    std::move (clip_id), start_ticks);
  add_laned_object (track, lane, obj_ref);
  return obj_ref.get_object_as<structure::arrangement::AudioRegion> ();
}

structure::arrangement::ScaleObject *
ArrangerObjectCreator::add_scale_object (
  structure::tracks::ChordTrack             &chord_track,
  utils::QObjectUniquePtr<dsp::MusicalScale> scale,
  units::precise_tick_t                      start_ticks)
{
  auto obj_ref =
    arranger_object_factory_.get_builder<structure::arrangement::ScaleObject> ()
      .with_start_ticks (start_ticks)
      .with_scale (std::move (scale))
      .build_in_registry ();
  undo_stack_.push (
    new commands::AddArrangerObjectCommand<structure::arrangement::ScaleObject> (
      chord_track, obj_ref));
  return obj_ref.get_object_as<structure::arrangement::ScaleObject> ();
}

structure::arrangement::ArrangerObjectUuidReference
ArrangerObjectCreator::add_audio_region_for_recording (
  structure::tracks::Track        &track,
  structure::tracks::TrackLane    &lane,
  const utils::audio::AudioBuffer &initial_frames,
  const utils::Utf8String         &clip_name,
  units::precise_tick_t            start_ticks)
{
  auto region_ref =
    arranger_object_factory_.create_audio_region_from_audio_buffer (
      initial_frames, utils::audio::BitDepth::BIT_DEPTH_32, clip_name,
      start_ticks);
  add_laned_object (track, lane, region_ref);
  return region_ref;
}

structure::arrangement::ArrangerObjectUuidReference
ArrangerObjectCreator::add_midi_region_for_recording (
  structure::tracks::Track     &track,
  structure::tracks::TrackLane &lane,
  units::precise_tick_t         start_ticks)
{
  auto region_ref =
    arranger_object_factory_.get_builder<structure::arrangement::MidiRegion> ()
      .with_start_ticks (start_ticks)
      .build_in_registry ();
  add_laned_object (track, lane, region_ref);
  return region_ref;
}

structure::arrangement::MidiControlEvent *
ArrangerObjectCreator::add_midi_control_event (
  structure::arrangement::MidiRegion                 &region,
  units::precise_tick_t                               startTicks,
  structure::arrangement::MidiControlEvent::EventType type,
  int                                                 channel,
  int                                                 controller,
  int                                                 value)
{
  auto obj_ref =
    arranger_object_factory_
      .get_builder<structure::arrangement::MidiControlEvent> ()
      .with_start_ticks (startTicks)
      .build_in_registry ();
  auto * ev = obj_ref.get_object_as<structure::arrangement::MidiControlEvent> ();
  ev->setControlType (type);
  ev->setChannel (channel);
  ev->setController (controller);
  ev->setValue (value);
  undo_stack_.push (
    new commands::AddArrangerObjectCommand<
      structure::arrangement::MidiControlEvent> (region, obj_ref));
  return ev;
}

structure::arrangement::AudioRegion *
ArrangerObjectCreator::addAudioRegionFromFile (
  structure::tracks::Track *     track,
  structure::tracks::TrackLane * lane,
  const QString                 &absPath,
  double                         startTicks)
{
  auto ar_ref = arranger_object_factory_.create_audio_region_from_file (
    absPath, units::ticks (startTicks));
  add_laned_object (*track, *lane, ar_ref);
  return ar_ref.get_object_as<structure::arrangement::AudioRegion> ();
}

structure::arrangement::AudioRegion *
ArrangerObjectCreator::addAudioRegionToClipSlotFromFile (
  structure::tracks::Track *    track,
  structure::scenes::ClipSlot * clipSlot,
  const QString                &absPath)
{
  auto ar_ref = arranger_object_factory_.create_audio_region_from_file (
    absPath, units::ticks (0.0));
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
