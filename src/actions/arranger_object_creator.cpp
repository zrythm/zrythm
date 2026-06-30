// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>

#include "utils/format_qt.h"

#include "actions/arranger_object_creator.h"
#include "commands/add_clip_to_clip_slot_command.h"
#include "commands/edit_chord_object_command.h"
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
        std::is_same_v<ObjectT, structure::arrangement::MidiClip>
        || std::is_same_v<ObjectT, structure::arrangement::AudioClip>)
        {
          obj->name ()->setName (
            track.generate_name_for_clip (*obj).to_qstring ());

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
        std::is_same_v<ObjectT, structure::arrangement::MidiClip>
        || std::is_same_v<ObjectT, structure::arrangement::AudioClip>)
        {
          obj->name ()->setName (
            track.generate_name_for_clip (*obj).to_qstring ());

          undo_stack_.push (
            new commands::AddClipToClipSlotCommand (slot, obj_ref));
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
    new commands::AddArrangerObjectCommand<structure::arrangement::TempoObject> (
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
    new commands::AddArrangerObjectCommand<
      structure::arrangement::TimeSignatureObject> (
      *tempoObjectManager, time_signature_object_ref));
  return time_signature_object_ref
    .get_object_as<structure::arrangement::TimeSignatureObject> ();
}

structure::arrangement::MidiClip *
ArrangerObjectCreator::addEmptyMidiClip (
  structure::tracks::Track *     track,
  structure::tracks::TrackLane * lane,
  double                         startTicks)
{
  auto mr_ref =
    arranger_object_factory_.get_builder<structure::arrangement::MidiClip> ()
      .with_start_ticks (units::ticks (startTicks))
      .build_in_registry ();
  add_laned_object (*track, *lane, mr_ref);
  return mr_ref.get_object_as<structure::arrangement::MidiClip> ();
}

structure::arrangement::ChordClip *
ArrangerObjectCreator::addEmptyChordClip (
  structure::tracks::ChordTrack * track,
  double                          startTicks)
{
  auto cr_ref =
    arranger_object_factory_.get_builder<structure::arrangement::ChordClip> ()
      .with_start_ticks (units::ticks (startTicks))
      .build_in_registry ();
  auto * chord_clip = cr_ref.get_object_as<structure::arrangement::ChordClip> ();
  chord_clip->name ()->setName (track->generate_name_for_clip (*chord_clip));
  undo_stack_.push (
    new commands::AddArrangerObjectCommand<structure::arrangement::ChordClip> (
      *track, cr_ref));
  return chord_clip;
}

structure::arrangement::AutomationClip *
ArrangerObjectCreator::addEmptyAutomationClip (
  structure::tracks::Track *           track,
  structure::tracks::AutomationTrack * automationTrack,
  double                               startTicks)
{
  auto ar_ref =
    arranger_object_factory_
      .get_builder<structure::arrangement::AutomationClip> ()
      .with_start_ticks (units::ticks (startTicks))
      .build_in_registry ();
  auto * ar = ar_ref.get_object_as<structure::arrangement::AutomationClip> ();
  ar->name ()->setName (track->generate_name_for_clip (*ar, automationTrack));
  undo_stack_.push (
    new commands::AddArrangerObjectCommand<
      structure::arrangement::AutomationClip> (*automationTrack, ar_ref));
  return ar;
}

structure::arrangement::MidiClip *
ArrangerObjectCreator::addMidiClipFromChordDescriptor (
  structure::tracks::Track *     track,
  structure::tracks::TrackLane * lane,
  const dsp::ChordDescriptor    &descr,
  double                         startTicks)
{
  auto *     mr = addEmptyMidiClip (track, lane, startTicks);
  const auto r_len_ticks =
    snap_grid_timeline_.defaultTicks (static_cast<int64_t> (startTicks));
  mr->length ()->setTicks (static_cast<double> (r_len_ticks));
  const auto mn_len_ticks =
    snap_grid_editor_.defaultTicks (static_cast<int64_t> (startTicks));

  /* create midi notes */
  for (auto pitch : descr.getMidiPitches ())
    {
      auto mn =
        arranger_object_factory_
          .get_builder<structure::arrangement::MidiNote> ()
          .with_pitch (static_cast<int> (pitch))
          .with_velocity (structure::arrangement::MidiNote::DEFAULT_VELOCITY)
          .with_start_ticks (units::ticks (0))
          .with_end_ticks (units::ticks (mn_len_ticks))
          .build_in_registry ();
      mr->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiNote>::add_object (mn);
    }

  return mr;
}

structure::arrangement::MidiClip *
ArrangerObjectCreator::addMidiClipFromMidiFile (
  structure::tracks::Track *     track,
  structure::tracks::TrackLane * lane,
  const QString                 &abs_path,
  double                         startTicks,
  int                            midi_track_idx)
{
  auto * mr = addEmptyMidiClip (track, lane, startTicks);

#if 0
  try
    {
      MidiFile mf{ utils::Utf8String::from_qstring (abs_path) };
      mf.into_clip (*mr, midi_track_idx);
    }
  catch (const ZrythmException &e)
    {
      z_warning ("Failed to create clip from MIDI file: {}", e.what ());
    }
#endif

  return mr;
}

structure::arrangement::MidiNote *
ArrangerObjectCreator::addMidiNote (
  structure::arrangement::MidiClip * clip,
  double                             startTicks,
  int                                pitch)
{
  return add_editor_object<structure::arrangement::MidiNote> (
    *clip, units::ticks (startTicks), pitch);
}

structure::arrangement::AutomationPoint *
ArrangerObjectCreator::addAutomationPoint (
  structure::arrangement::AutomationClip * clip,
  double                                   startTicks,
  double                                   value)
{
  return add_editor_object<structure::arrangement::AutomationPoint> (
    *clip, units::ticks (startTicks), value);
}

structure::arrangement::ChordObject *
ArrangerObjectCreator::addChordObject (
  structure::arrangement::ChordClip * clip,
  double                              startTicks,
  const int                           chordIndex)
{
  return add_editor_object<structure::arrangement::ChordObject> (
    *clip, units::ticks (startTicks), chordIndex);
}

structure::arrangement::ChordObject *
ArrangerObjectCreator::addChordObjectFromFields (
  structure::arrangement::ChordClip * clip,
  double                              startTicks,
  dsp::MusicalNote                    rootNote,
  dsp::ChordType                      chordType,
  dsp::ChordAccent                    chordAccent,
  bool                                hasBass,
  dsp::MusicalNote                    bassNote,
  int                                 inversion)
{
  assert (clip != nullptr);
  auto builder = std::move (
    arranger_object_factory_.get_builder<structure::arrangement::ChordObject> ()
      .with_start_ticks (units::ticks (startTicks)));
  builder.with_chord_descriptor (
    rootNote, chordType, chordAccent, inversion,
    hasBass ? std::make_optional (bassNote) : std::nullopt);
  auto obj_ref = builder.build_in_registry ();
  undo_stack_.push (
    new commands::AddArrangerObjectCommand<structure::arrangement::ChordObject> (
      *clip, obj_ref));
  return obj_ref.get_object_as<structure::arrangement::ChordObject> ();
}

structure::arrangement::ChordObject *
ArrangerObjectCreator::addChordObjectFromDescriptor (
  structure::arrangement::ChordClip * clip,
  double                              startTicks,
  dsp::ChordDescriptor *              descriptor)
{
  assert (clip != nullptr);
  if (descriptor == nullptr)
    return nullptr;
  return addChordObjectFromFields (
    clip, startTicks, descriptor->rootNote (), descriptor->chordType (),
    descriptor->chordAccent (), descriptor->hasBass (),
    descriptor->hasBass () ? descriptor->bassNote () : dsp::MusicalNote::C,
    descriptor->inversion ());
}

void
ArrangerObjectCreator::editChordObjectsDescriptor (
  QVariantList     chordObjects,
  dsp::MusicalNote rootNote,
  dsp::ChordType   chordType,
  dsp::ChordAccent chordAccent,
  bool             hasBass,
  dsp::MusicalNote bassNote,
  int              inversion)
{
  // Build a target descriptor from the given fields.
  dsp::ChordDescriptor target;
  target.setRootNote (rootNote);
  target.setChordType (chordType);
  target.setChordAccent (chordAccent);
  target.setInversion (inversion);
  if (hasBass)
    target.setBassNote (bassNote);
  else
    target.setHasBass (false);

  // Count valid (non-null) targets so a single edit can be pushed without a
  // macro, allowing it to merge with the previous top-level edit (one undo
  // step for repeated tweaks of the same object).
  int valid_count = 0;
  for (const auto &variant : chordObjects)
    {
      if (variant.value<structure::arrangement::ChordObject *> () != nullptr)
        ++valid_count;
    }
  if (valid_count == 0)
    return;

  const bool single = (valid_count == 1);
  if (!single)
    undo_stack_.beginMacro (QObject::tr ("Edit chord"));
  for (const auto &variant : chordObjects)
    {
      auto * co = variant.value<structure::arrangement::ChordObject *> ();
      if (co != nullptr)
        undo_stack_.push (new commands::EditChordObjectCommand (co, target));
    }
  if (!single)
    undo_stack_.endMacro ();
}

structure::arrangement::MidiClip *
ArrangerObjectCreator::addEmptyMidiClipToClip (
  structure::tracks::Track *    track,
  structure::scenes::ClipSlot * clipSlot)
{
  auto mr_ref =
    arranger_object_factory_.get_builder<structure::arrangement::MidiClip> ()
      .with_start_ticks (units::ticks (0))
      .build_in_registry ();
  clipSlot->setClip (mr_ref.get_object_as<structure::arrangement::MidiClip> ());
  return mr_ref.get_object_as<structure::arrangement::MidiClip> ();
}

structure::arrangement::AudioClip *
ArrangerObjectCreator::add_audio_clip_with_clip (
  structure::tracks::Track         &track,
  structure::tracks::TrackLane     &lane,
  dsp::FileAudioSourceUuidReference clip_id,
  units::precise_tick_t             start_ticks)
{
  auto obj_ref = arranger_object_factory_.create_audio_clip_with_clip (
    std::move (clip_id), start_ticks);
  add_laned_object (track, lane, obj_ref);
  return obj_ref.get_object_as<structure::arrangement::AudioClip> ();
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
ArrangerObjectCreator::add_audio_clip_for_recording (
  structure::tracks::Track        &track,
  structure::tracks::TrackLane    &lane,
  const utils::audio::AudioBuffer &initial_frames,
  const utils::Utf8String         &clip_name,
  units::precise_tick_t            start_ticks)
{
  auto clip_ref = arranger_object_factory_.create_audio_clip_from_audio_buffer (
    initial_frames, utils::audio::BitDepth::BIT_DEPTH_32, clip_name,
    start_ticks);
  add_laned_object (track, lane, clip_ref);
  return clip_ref;
}

structure::arrangement::ArrangerObjectUuidReference
ArrangerObjectCreator::add_midi_clip_for_recording (
  structure::tracks::Track     &track,
  structure::tracks::TrackLane &lane,
  units::precise_tick_t         start_ticks)
{
  auto clip_ref =
    arranger_object_factory_.get_builder<structure::arrangement::MidiClip> ()
      .with_start_ticks (start_ticks)
      .build_in_registry ();
  add_laned_object (track, lane, clip_ref);
  return clip_ref;
}

structure::arrangement::MidiControlEvent *
ArrangerObjectCreator::add_midi_control_event (
  structure::arrangement::MidiClip                   &clip,
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
      structure::arrangement::MidiControlEvent> (clip, obj_ref));
  return ev;
}

structure::arrangement::AudioClip *
ArrangerObjectCreator::addAudioClipFromFile (
  structure::tracks::Track *     track,
  structure::tracks::TrackLane * lane,
  const QString                 &absPath,
  double                         startTicks)
{
  auto ar_ref = arranger_object_factory_.create_audio_clip_from_file (
    absPath, units::ticks (startTicks));
  add_laned_object (*track, *lane, ar_ref);
  return ar_ref.get_object_as<structure::arrangement::AudioClip> ();
}

structure::arrangement::AudioClip *
ArrangerObjectCreator::addAudioClipToClipSlotFromFile (
  structure::tracks::Track *    track,
  structure::scenes::ClipSlot * clipSlot,
  const QString                &absPath)
{
  auto ar_ref = arranger_object_factory_.create_audio_clip_from_file (
    absPath, units::ticks (0.0));
  add_object_to_clip_slot (*track, *clipSlot, ar_ref);
  return ar_ref.get_object_as<structure::arrangement::AudioClip> ();
}

structure::arrangement::MidiClip *
ArrangerObjectCreator::addMidiClipToClipSlotFromFile (
  structure::tracks::Track *    track,
  structure::scenes::ClipSlot * clipSlot,
  const QString                &absPath)
{
  // FIXME: MIDI file parsing is not yet implemented. This stub creates a
  // placeholder MidiClip with two dummy notes so import does not crash.
  // Real MIDI import (parsing tempo meta-events into source_bpm_, notes,
  // and control events) is deferred.
  z_warning (
    "MIDI file import not implemented: creating placeholder clip for '{}'",
    absPath.toUtf8 ().constData ());

  auto mr_ref =
    arranger_object_factory_.get_builder<structure::arrangement::MidiClip> ()
      .with_start_ticks (units::ticks (0.0))
      .build_in_registry ();
  add_object_to_clip_slot (*track, *clipSlot, mr_ref);

  auto * clip = mr_ref.get_object_as<structure::arrangement::MidiClip> ();

  addMidiNote (clip, 0.0, 60);
  addMidiNote (clip, 480.0, 64);

  return clip;
}

} // namespace zrythm::actions
