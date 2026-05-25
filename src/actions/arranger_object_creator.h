// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "commands/add_arranger_object_command.h"
#include "dsp/snap_grid.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/tempo_object_manager.h"
#include "structure/scenes/clip_slot.h"
#include "structure/tracks/track_all.h"
#include "undo/undo_stack.h"

namespace zrythm::actions
{
class ArrangerObjectCreator : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("One instance per project")

public:
  explicit ArrangerObjectCreator (
    undo::UndoStack                               &undo_stack,
    structure::arrangement::ArrangerObjectFactory &arranger_object_factory,
    dsp::SnapGrid                                 &snap_grid_timeline,
    dsp::SnapGrid                                 &snap_grid_editor,
    QObject *                                      parent = nullptr)
      : QObject (parent), arranger_object_factory_ (arranger_object_factory),
        snap_grid_timeline_ (snap_grid_timeline),
        snap_grid_editor_ (snap_grid_editor), undo_stack_ (undo_stack)
  {
  }

  Q_INVOKABLE structure::arrangement::Marker * addMarker (
    structure::arrangement::Marker::MarkerType markerType,
    structure::tracks::MarkerTrack *           markerTrack,
    const QString                             &name,
    double                                     startTicks);

  Q_INVOKABLE structure::arrangement::TempoObject * addTempoObject (
    structure::arrangement::TempoObjectManager *   tempoObjectManager,
    double                                         bpm,
    structure::arrangement::TempoObject::CurveType curveType,
    double                                         startTicks);

  Q_INVOKABLE structure::arrangement::TimeSignatureObject *
              addTimeSignatureObject (
                structure::arrangement::TempoObjectManager * tempoObjectManager,
                int                                          numerator,
                int                                          denominator,
                double                                       startTicks);

  Q_INVOKABLE structure::arrangement::MidiRegion * addEmptyMidiRegion (
    structure::tracks::Track *     track,
    structure::tracks::TrackLane * lane,
    double                         startTicks);

  Q_INVOKABLE structure::arrangement::MidiRegion * addEmptyMidiRegionToClip (
    structure::tracks::Track *    track,
    structure::scenes::ClipSlot * clipSlot);

  Q_INVOKABLE structure::arrangement::ChordRegion *
  addEmptyChordRegion (structure::tracks::ChordTrack * track, double startTicks);

  Q_INVOKABLE structure::arrangement::AutomationRegion *
              addEmptyAutomationRegion (
                structure::tracks::Track *           track,
                structure::tracks::AutomationTrack * automationTrack,
                double                               startTicks);

  /**
   * @brief
   *
   * @param lane
   * @param clip_id
   * @param start_ticks
   * @return AudioRegion*
   */
  structure::arrangement::AudioRegion * add_audio_region_with_clip (
    structure::tracks::Track         &track,
    structure::tracks::TrackLane     &lane,
    dsp::FileAudioSourceUuidReference clip_id,
    units::precise_tick_t             start_ticks);

  structure::arrangement::ScaleObject * add_scale_object (
    structure::tracks::ChordTrack             &chord_track,
    utils::QObjectUniquePtr<dsp::MusicalScale> scale,
    units::precise_tick_t                      start_ticks);

  structure::arrangement::ArrangerObjectUuidReference
  add_audio_region_for_recording (
    structure::tracks::Track        &track,
    structure::tracks::TrackLane    &lane,
    const utils::audio::AudioBuffer &initial_frames,
    const utils::Utf8String         &clip_name,
    units::precise_tick_t            start_ticks);

  structure::arrangement::ArrangerObjectUuidReference
  add_midi_region_for_recording (
    structure::tracks::Track     &track,
    structure::tracks::TrackLane &lane,
    units::precise_tick_t         start_ticks);

  structure::arrangement::MidiControlEvent * add_midi_control_event (
    structure::arrangement::MidiRegion                 &region,
    units::precise_tick_t                               startTicks,
    structure::arrangement::MidiControlEvent::EventType type,
    int                                                 channel,
    int                                                 controller,
    int                                                 value);

  Q_INVOKABLE structure::arrangement::AudioRegion * addAudioRegionFromFile (
    structure::tracks::Track *     track,
    structure::tracks::TrackLane * lane,
    const QString                 &absPath,
    double                         startTicks);

  Q_INVOKABLE structure::arrangement::AudioRegion *
              addAudioRegionToClipSlotFromFile (
                structure::tracks::Track *    track,
                structure::scenes::ClipSlot * clipSlot,
                const QString                &absPath);
  Q_INVOKABLE structure::arrangement::AudioRegion *
              addMidiRegionToClipSlotFromFile (
                structure::tracks::Track *    track,
                structure::scenes::ClipSlot * clipSlot,
                const QString                &absPath);

  /**
   * @brief Creates a MIDI region at @p lane from the given @p descr
   * starting at @p startTicks.
   */
  Q_INVOKABLE structure::arrangement::MidiRegion *
              addMidiRegionFromChordDescriptor (
                structure::tracks::Track *     track,
                structure::tracks::TrackLane * lane,
                const dsp::ChordDescriptor    &descr,
                double                         startTicks);

  /**
   * @brief Creates a MIDI region at @p lane from MIDI file path @p abs_path
   * starting at @p startTicks.
   *
   * @param midi_track_idx The index of this track, starting from 0. This
   * will be sequential, ie, if idx 1 is requested and the MIDI file only
   * has tracks 5 and 7, it will use track 7.
   */
  Q_INVOKABLE structure::arrangement::MidiRegion * addMidiRegionFromMidiFile (
    structure::tracks::Track *     track,
    structure::tracks::TrackLane * lane,
    const QString                 &absolutePath,
    double                         startTicks,
    int                            midiTrackIndex);

  Q_INVOKABLE structure::arrangement::MidiNote * addMidiNote (
    structure::arrangement::MidiRegion * region,
    double                               startTicks,
    int                                  pitch);

  Q_INVOKABLE structure::arrangement::AutomationPoint * addAutomationPoint (
    structure::arrangement::AutomationRegion * region,
    double                                     startTicks,
    double                                     value);

  Q_INVOKABLE structure::arrangement::ChordObject * addChordObject (
    structure::arrangement::ChordRegion * region,
    double                                startTicks,
    const int                             chordIndex);

private:
  /**
   * @brief Used for MIDI/Audio regions.
   */
  void add_laned_object (
    structure::tracks::Track                           &track,
    structure::tracks::TrackLane                       &lane,
    structure::arrangement::ArrangerObjectUuidReference obj_ref);

  void add_object_to_clip_slot (
    structure::tracks::Track                           &track,
    structure::scenes::ClipSlot                        &clip_slot,
    structure::arrangement::ArrangerObjectUuidReference obj_ref);

  /**
   * @brief Used to create and add editor objects.
   *
   * @param region Clip editor region.
   * @param startTicks Start position of the object in ticks.
   * @param value Either pitch (int), automation point value (double) or chord
   * ID.
   */
  template <structure::arrangement::EditorObject ChildT>
  auto add_editor_object (
    structure::arrangement::RegionObject auto &region,
    units::precise_tick_t                      startTicks,
    std::variant<int, double>                  value) -> ChildT *
  {
    auto obj_ref =
      arranger_object_factory_.create_editor_object<ChildT> (startTicks, value);
    undo_stack_.push (
      new commands::AddArrangerObjectCommand<ChildT> (region, obj_ref));
    auto obj = obj_ref.template get_object_as<ChildT> ();
    return obj;
  }

private:
  structure::arrangement::ArrangerObjectFactory &arranger_object_factory_;
  dsp::SnapGrid                                 &snap_grid_timeline_;
  dsp::SnapGrid                                 &snap_grid_editor_;
  undo::UndoStack                               &undo_stack_;
};
} // namespace zrythm::actions
