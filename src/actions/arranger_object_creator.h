// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "commands/add_arranger_object_command.h"
#include "dsp/snap_grid.h"
#include "gui/backend/backend/clip_editor.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/arrangement/arranger_object_owner.h"
#include "structure/tracks/track_all.h"
#include "undo/undo_stack.h"

namespace zrythm::actions
{
/**
 * @brief Actions that require project-wide involvement.
 */
class ArrangerObjectCreator : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("One instance per project")

public:
  struct ArrangerObjectSelectionManagers
  {
    structure::arrangement::ArrangerObjectSelectionManager
      audio_selections_manager_;
    structure::arrangement::ArrangerObjectSelectionManager
      timeline_selections_manager_;
    structure::arrangement::ArrangerObjectSelectionManager
      midi_selections_manager_;
    structure::arrangement::ArrangerObjectSelectionManager
      chord_selections_manager_;
    structure::arrangement::ArrangerObjectSelectionManager
      automation_selections_manager_;
  };

  explicit ArrangerObjectCreator (
    undo::UndoStack                               &undo_stack,
    ClipEditor                                    &clip_editor,
    structure::arrangement::ArrangerObjectFactory &arranger_object_factory,
    ArrangerObjectSelectionManagers arranger_object_selection_managers,
    dsp::SnapGrid                  &snap_grid_timeline,
    dsp::SnapGrid                  &snap_grid_editor,
    QObject *                       parent = nullptr)
      : QObject (parent), clip_editor_ (clip_editor),
        arranger_object_factory_ (arranger_object_factory),
        arranger_object_selection_managers_ (
          std::move (arranger_object_selection_managers)),
        snap_grid_timeline_ (snap_grid_timeline),
        snap_grid_editor_ (snap_grid_editor), undo_stack_ (undo_stack)
  {
  }

  Q_INVOKABLE structure::arrangement::Marker * addMarker (
    structure::arrangement::Marker::MarkerType markerType,
    structure::tracks::MarkerTrack *           markerTrack,
    const QString                             &name,
    double                                     startTicks)
  {
    auto marker_ref =
      arranger_object_factory_.get_builder<structure::arrangement::Marker> ()
        .with_start_ticks (startTicks)
        .with_name (name)
        .with_marker_type (markerType)
        .build_in_registry ();
    undo_stack_.push (
      new commands::AddArrangerObjectCommand<structure::arrangement::Marker> (
        *markerTrack, marker_ref));
    return marker_ref.get_object_as<structure::arrangement::Marker> ();
  }

  Q_INVOKABLE structure::arrangement::MidiRegion * addEmptyMidiRegion (
    structure::tracks::Track *     track,
    structure::tracks::TrackLane * lane,
    double                         startTicks)
  {
    auto mr_ref =
      arranger_object_factory_
        .get_builder<structure::arrangement::MidiRegion> ()
        .with_start_ticks (startTicks)
        .build_in_registry ();
    add_laned_object (*track, *lane, mr_ref);
    return mr_ref.get_object_as<structure::arrangement::MidiRegion> ();
  }

  Q_INVOKABLE structure::arrangement::ChordRegion *
  addEmptyChordRegion (structure::tracks::ChordTrack * track, double startTicks)
  {
    auto cr_ref =
      arranger_object_factory_
        .get_builder<structure::arrangement::ChordRegion> ()
        .with_start_ticks (startTicks)
        .build_in_registry ();
    auto * chord_region =
      cr_ref.get_object_as<structure::arrangement::ChordRegion> ();
    chord_region->regionMixin ()->name ()->setName (
      track->generate_name_for_region (*chord_region));
    track->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::ChordRegion>::add_object (cr_ref);
    undo_stack_.push (
      new commands::AddArrangerObjectCommand<
        structure::arrangement::ChordRegion> (*track, cr_ref));
    return cr_ref.get_object_as<structure::arrangement::ChordRegion> ();
  }

  Q_INVOKABLE structure::arrangement::AutomationRegion *
              addEmptyAutomationRegion (
                structure::tracks::AutomationTrack * automationTrack,
                double                               startTicks)

  {
    // TODO
    return nullptr;
#if 0
    auto ar_ref =
      get_builder<AutomationRegion> ()
        .with_start_ticks (startTicks)
        .build_in_registry ();
    auto track_var = automationTrack->get_track ();
    std::visit (
      [&] (auto &&track) {
        track->structure::tracks::Track::template add_region<AutomationRegion> (
          ar_ref, automationTrack, std::nullopt, true);
      },
      track_var);
    return std::get<AutomationRegion *> (ar_ref.get_object ());
#endif
  }

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
    double                            start_ticks)
  {
    auto obj_ref = arranger_object_factory_.create_audio_region_with_clip (
      std::move (clip_id), start_ticks);
    add_laned_object (track, lane, obj_ref);
    return obj_ref.get_object_as<structure::arrangement::AudioRegion> ();
  }

  structure::arrangement::ScaleObject * add_scale_object (
    structure::tracks::ChordTrack             &chord_track,
    utils::QObjectUniquePtr<dsp::MusicalScale> scale,
    double                                     start_ticks)
  {
    auto obj_ref =
      arranger_object_factory_
        .get_builder<structure::arrangement::ScaleObject> ()
        .with_start_ticks (start_ticks)
        .with_scale (std::move (scale))
        .build_in_registry ();
    undo_stack_.push (
      new commands::AddArrangerObjectCommand<
        structure::arrangement::ScaleObject> (chord_track, obj_ref));
    return obj_ref.get_object_as<structure::arrangement::ScaleObject> ();
  }

  structure::arrangement::AudioRegion * add_empty_audio_region_for_recording (
    structure::tracks::Track     &track,
    structure::tracks::TrackLane &lane,
    int                           num_channels,
    const utils::Utf8String      &clip_name,
    double                        start_ticks)
  {
    auto region_ref =
      arranger_object_factory_.create_empty_audio_region_for_recording (
        num_channels, clip_name, start_ticks);
    add_laned_object (track, lane, region_ref);
    return region_ref.get_object_as<structure::arrangement::AudioRegion> ();
  }

  Q_INVOKABLE structure::arrangement::AudioRegion * addAudioRegionFromFile (
    structure::tracks::Track *     track,
    structure::tracks::TrackLane * lane,
    const QString                 &absPath,
    double                         startTicks)
  {
    auto ar_ref = arranger_object_factory_.create_audio_region_from_file (
      absPath, startTicks);
    add_laned_object (*track, *lane, ar_ref);
    return ar_ref.get_object_as<structure::arrangement::AudioRegion> ();
  }

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
    int                                  pitch)
  {
    return add_editor_object (*region, startTicks, pitch);
  }

  Q_INVOKABLE structure::arrangement::AutomationPoint * addAutomationPoint (
    structure::arrangement::AutomationRegion * region,
    double                                     startTicks,
    double                                     value)

  {
    return add_editor_object (*region, startTicks, value);
  }

  Q_INVOKABLE structure::arrangement::ChordObject * addChordObject (
    structure::arrangement::ChordRegion * region,
    double                                startTicks,
    const int                             chordIndex)

  {
    return add_editor_object (*region, startTicks, chordIndex);
  }

private:
  /**
   * @brief Used for MIDI/Audio regions.
   */
  void add_laned_object (
    structure::tracks::Track                           &track,
    structure::tracks::TrackLane                       &lane,
    structure::arrangement::ArrangerObjectUuidReference obj_ref);

  template <structure::arrangement::FinalArrangerObjectSubclass ObjT>
  auto get_selection_manager_for_object (const ObjT &obj) const
  {
    if constexpr (structure ::arrangement::TimelineObject<ObjT>)
      {
        return arranger_object_selection_managers_.timeline_selections_manager_;
      }
    else if constexpr (std::is_same_v<ObjT, structure ::arrangement::MidiNote>)
      {
        return arranger_object_selection_managers_.midi_selections_manager_;
      }
    else if constexpr (
      std::is_same_v<ObjT, structure ::arrangement::AutomationPoint>)
      {
        return arranger_object_selection_managers_.automation_selections_manager_;
      }
    else if constexpr (
      std::is_same_v<ObjT, structure ::arrangement::ChordObject>)
      {
        return arranger_object_selection_managers_.chord_selections_manager_;
      }
    else if constexpr (
      std::is_same_v<ObjT, structure ::arrangement::AudioSourceObject>)
      {
        return arranger_object_selection_managers_.audio_selections_manager_;
      }
    else
      {
        static_assert (false);
      }
  }

  template <structure::arrangement::FinalArrangerObjectSubclass ObjT>
  void set_selection_handler_to_object (ObjT &obj)
  {
    auto sel_mgr = get_selection_manager_for_object (obj);
    obj.set_selection_status_getter (
      [sel_mgr] (const structure::arrangement::ArrangerObject::Uuid &id) {
        return sel_mgr.is_selected (id);
      });
  }

  /**
   * @brief Used to create and add editor objects.
   *
   * @param region Clip editor region.
   * @param startTicks Start position of the object in ticks.
   * @param value Either pitch (int), automation point value (double) or chord
   * ID.
   */
template <structure::arrangement::RegionObject RegionT>
auto add_editor_object (
  RegionT                  &region,
  double                    startTicks,
  std::variant<int, double> value)
  -> RegionT::ArrangerObjectChildType * requires (
    !std::is_same_v<RegionT, structure::arrangement::AudioRegion>) {
    using ChildT = typename RegionT::ArrangerObjectChildType;
    auto obj_ref = arranger_object_factory_.create_editor_object<RegionT> (
      startTicks, value);
    undo_stack_.push (
      new commands::AddArrangerObjectCommand<ChildT> (region, obj_ref));
    auto obj = obj_ref.template get_object_as<ChildT> ();
    {
      auto sel_mgr = get_selection_manager_for_object (*obj);
      set_selection_handler_to_object (*obj);
      sel_mgr.append_to_selection (obj->get_uuid ());
    }
    return obj;
  }

private : ClipEditor &clip_editor_;
  structure::arrangement::ArrangerObjectFactory &arranger_object_factory_;
  ArrangerObjectSelectionManagers arranger_object_selection_managers_;
  dsp::SnapGrid                  &snap_grid_timeline_;
  dsp::SnapGrid                  &snap_grid_editor_;
  undo::UndoStack                &undo_stack_;
};
} // namespace zrythm::actions
