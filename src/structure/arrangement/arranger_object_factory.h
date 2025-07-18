// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/backend/backend/settings_manager.h"
#include "gui/dsp/snap_grid.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/tracks/track_all.h"

namespace zrythm::structure::arrangement
{
/**
 * @brief Factory for arranger objects.
 *
 * @note API that starts with `add` adds the object to the project and should be
 * used in most cases. API that starts with `create` only creates and registers
 * the object but does not add it to the project (this should only be used
 * internally).
 */
class ArrangerObjectFactory : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  using ArrangerObjectRegistry = structure::arrangement::ArrangerObjectRegistry;
  using ArrangerObjectSelectionManager =
    structure::arrangement::ArrangerObjectSelectionManager;
  using MidiNote = structure::arrangement::MidiNote;
  using ChordObject = structure::arrangement::ChordObject;
  using ScaleObject = structure::arrangement::ScaleObject;
  using AudioRegion = structure::arrangement::AudioRegion;
  using MidiRegion = structure::arrangement::MidiRegion;
  using AutomationRegion = structure::arrangement::AutomationRegion;
  using AutomationPoint = structure::arrangement::AutomationPoint;
  using ChordRegion = structure::arrangement::ChordRegion;
  using Marker = structure::arrangement::Marker;
  using ArrangerObject = structure::arrangement ::ArrangerObject;
  using MidiLane = structure::tracks::MidiLane;
  using AudioLane = structure::tracks::AudioLane;

public:
  ArrangerObjectFactory () = delete;
  ArrangerObjectFactory (
    const dsp::TempoMap            &tempo_map,
    ArrangerObjectRegistry         &registry,
    dsp::FileAudioSourceRegistry   &file_audio_source_registry,
    gui::SettingsManager           &settings_mgr,
    gui::SnapGrid                  &snap_grid_timeline,
    gui::SnapGrid                  &snap_grid_editor,
    std::function<sample_rate_t ()> sample_rate_provider,
    std::function<bpm_t ()>         bpm_provider,
    ArrangerObjectSelectionManager  audio_selections_manager,
    ArrangerObjectSelectionManager  timeline_selections_manager,
    ArrangerObjectSelectionManager  midi_selections_manager,
    ArrangerObjectSelectionManager  chord_selections_manager,
    ArrangerObjectSelectionManager  automation_selections_manager,
    ClipEditor                     &clip_editor,
    QObject *                       parent = nullptr)
      : QObject (parent), tempo_map_ (tempo_map), object_registry_ (registry),
        file_audio_source_registry_ (file_audio_source_registry),
        settings_manager_ (settings_mgr),
        snap_grid_timeline_ (snap_grid_timeline),
        snap_grid_editor_ (snap_grid_editor),
        sample_rate_provider_ (std::move (sample_rate_provider)),
        bpm_provider_ (std::move (bpm_provider)),
        audio_selections_manager_ (audio_selections_manager),
        timeline_selections_manager_ (timeline_selections_manager),
        midi_selections_manager_ (midi_selections_manager),
        chord_selections_manager_ (chord_selections_manager),
        automation_selections_manager_ (automation_selections_manager),
        clip_editor_ (clip_editor)
  {
  }

  static ArrangerObjectFactory * get_instance ();

  template <structure::arrangement::FinalArrangerObjectSubclass ObjT>
  class Builder
  {
    friend class ArrangerObjectFactory;

  private:
    explicit Builder (
      const dsp::TempoMap          &tempo_map,
      ArrangerObjectRegistry       &registry,
      dsp::FileAudioSourceRegistry &file_audio_source_registry)
        : tempo_map_ (tempo_map), registry_ (registry),
          file_audio_source_registry_ (file_audio_source_registry)
    {
    }

    Builder &with_settings_manager (gui::SettingsManager &settings_manager)
    {
      settings_manager_ = settings_manager;
      return *this;
    }

    Builder &with_clip (dsp::FileAudioSourceUuidReference clip_id)
      requires (std::is_same_v<ObjT, AudioRegion>)
    {
      clip_id_.emplace (std::move (clip_id));
      return *this;
    }

  public:
    Builder &with_start_ticks (double start_ticks)
    {
      assert (settings_manager_.has_value ());
      start_ticks_ = start_ticks;

      return *this;
    }

    Builder &with_end_ticks (double end_ticks)
      requires (BoundedObject<ObjT>)
    {
      end_ticks_ = end_ticks;
      return *this;
    }

    Builder &with_name (const QString &name)
      requires (NamedObject<ObjT>)
    {
      name_ = name;
      return *this;
    }

    Builder &with_pitch (const int pitch)
      requires (std::is_same_v<ObjT, MidiNote>)
    {
      pitch_ = pitch;
      return *this;
    }

    Builder &with_velocity (const int vel)
      requires (std::is_same_v<ObjT, MidiNote>)
    {
      velocity_ = vel;
      return *this;
    }

    Builder &with_automatable_value (const double automatable_val)
      requires (std::is_same_v<ObjT, AutomationPoint>)
    {
      automatable_value_ = automatable_val;
      return *this;
    }

    Builder &with_chord_descriptor (const int chord_descriptor_index)
      requires (std::is_same_v<ObjT, ChordObject>)
    {
      chord_descriptor_index_ = chord_descriptor_index;
      return *this;
    }

    Builder &with_scale (utils::QObjectUniquePtr<dsp::MusicalScale> scale)
      requires (std::is_same_v<ObjT, ScaleObject>)
    {
      scale_ = std::move (scale);
      return *this;
    }

    Builder &with_marker_type (Marker::MarkerType marker_type)
      requires (std::is_same_v<ObjT, Marker>)
    {
      marker_type_ = marker_type;
      return *this;
    }

    /**
     * @brief Returns an instantiated object to be used for deserialization.
     */
    std::unique_ptr<ObjT> build_empty () const
    {
      std::unique_ptr<ObjT> ret;
      if constexpr (std::is_same_v<ObjT, AudioRegion>)
        {
          ret = std::make_unique<ObjT> (
            tempo_map_, registry_, file_audio_source_registry_,
            [this] () { return settings_manager_->get_musicalMode (); });
        }
      else if constexpr (RegionObject<ObjT>)
        {
          ret = std::make_unique<ObjT> (
            tempo_map_, registry_, file_audio_source_registry_);
        }
      else if constexpr (std::is_same_v<ObjT, Marker>)
        {
          ret = std::make_unique<ObjT> (tempo_map_, Marker::MarkerType::Custom);
        }
      else if constexpr (std::is_same_v<ObjT, AudioSourceObject>)
        {
          ret = std::make_unique<ObjT> (
            tempo_map_, file_audio_source_registry_,
            dsp::FileAudioSourceUuidReference{ file_audio_source_registry_ });
        }
      else
        {
          ret = std::make_unique<ObjT> (tempo_map_);
        }
      return ret;
    }

    auto build_in_registry ()
    {
      auto obj_ref = [&] () {
        auto obj_unique_ptr = build_empty ();
        registry_.register_object (obj_unique_ptr.get ());
        structure::arrangement::ArrangerObjectUuidReference ret_ref{
          obj_unique_ptr->get_uuid (), registry_
        };
        obj_unique_ptr.release ();
        return ret_ref;
      }();

      auto * obj = std::get<ObjT *> (obj_ref.get_object ());

      if constexpr (RegionObject<ObjT>)
        {
          obj->regionMixin ()->loopRange ()->setTrackLength (true);
        }

      if (start_ticks_)
        {
          if (!end_ticks_ && !clip_id_)
            {
              if constexpr (BoundedObject<ObjT>)
                {
                  double len_ticks{};
                  if constexpr (TimelineObject<ObjT>)
                    {
                      len_ticks =
                        settings_manager_
                          ->get_timelineLastCreatedObjectLengthInTicks ();
                    }
                  else
                    {
                      len_ticks =
                        settings_manager_
                          ->get_editorLastCreatedObjectLengthInTicks ();
                    }
                  ArrangerObjectSpan::bounds_projection (obj)
                    ->length ()
                    ->setTicks (len_ticks);
                }
            }
          obj->position ()->setTicks (*start_ticks_);
        }

      if (clip_id_)
        {
          if constexpr (std::is_same_v<ObjT, AudioRegion>)
            {
              auto source_object = registry_.create_object<AudioSourceObject> (
                tempo_map_, file_audio_source_registry_, clip_id_.value ());
              obj->set_source (source_object);
              obj->regionMixin ()->bounds ()->length ()->setSamples (
                clip_id_.value ()
                  .template get_object_as<dsp::FileAudioSource> ()
                  ->get_num_frames ());
            }
        }

      if (end_ticks_)
        {
          if constexpr (BoundedObject<ObjT>)
            {
              ArrangerObjectSpan::bounds_projection (obj)->length ()->setTicks (
                *end_ticks_ - obj->position ()->ticks ());
            }
        }

      if (name_)
        {
          if constexpr (NamedObject<ObjT>)
            {
              if constexpr (RegionObject<ObjT>)
                {
                  obj->regionMixin ()->name ()->setName (*name_);
                }
              else
                {
                  obj->name ()->setName (*name_);
                }
            }
        }

      if (pitch_)
        {
          if constexpr (std::is_same_v<ObjT, MidiNote>)
            {
              obj->setPitch (*pitch_);
            }
        }

      if (velocity_)
        {
          if constexpr (std::is_same_v<ObjT, MidiNote>)
            {
              obj->setVelocity (*velocity_);
            }
        }

      if (automatable_value_)
        {
          if constexpr (std::is_same_v<ObjT, AutomationPoint>)
            {
              obj->setValue (*automatable_value_);
            }
        }

      if (scale_)
        {
          if constexpr (std::is_same_v<ObjT, ScaleObject>)
            {
              obj->setScale (scale_.release ());
            }
        }

      if constexpr (std::is_same_v<ObjT, AutomationPoint>)
        {
          obj->curveOpts ()->setAlgorithm (
            gui::SettingsManager::automationCurveAlgorithm ());
        }

      if (chord_descriptor_index_)
        {
          if constexpr (std::is_same_v<ObjT, ChordObject>)
            {
              obj->setChordDescriptorIndex (*chord_descriptor_index_);
            }
        }

      return obj_ref;
    }

  private:
    const dsp::TempoMap              &tempo_map_;
    ArrangerObjectRegistry           &registry_;
    dsp::FileAudioSourceRegistry     &file_audio_source_registry_;
    OptionalRef<gui::SettingsManager> settings_manager_;
    std::optional<dsp::FileAudioSourceUuidReference> clip_id_;
    std::optional<double>                            start_ticks_;
    std::optional<double>                            end_ticks_;
    std::optional<QString>                           name_;
    std::optional<int>                               pitch_;
    std::optional<double>                            automatable_value_;
    std::optional<int>                               chord_descriptor_index_;
    utils::QObjectUniquePtr<dsp::MusicalScale>       scale_;
    std::optional<int>                               velocity_;
    std::optional<Marker::MarkerType>                marker_type_;
  };

  template <typename ObjT> auto get_builder () const
  {
    return std::move (
      Builder<ObjT> (tempo_map_, object_registry_, file_audio_source_registry_)
        .with_settings_manager (settings_manager_));
  }

private:
  /**
   * @brief Used for MIDI/Audio regions.
   */
  template <structure::tracks::TrackLaneSubclass TrackLaneT>
  void add_laned_object (TrackLaneT &lane, auto obj_ref)
  {
    using RegionT = typename TrackLaneT::RegionT;
    tracks::TrackUuid track_id;
    std::visit (
      [&] (auto &&track) {
        if constexpr (
          std::is_same_v<
            typename base_type<decltype (track)>::TrackLaneType, TrackLaneT>)
          {
            track->template add_region<RegionT> (
              obj_ref, nullptr, lane.get_index_in_track (), true);
            track_id = track->get_uuid ();
          }
      },
      convert_to_variant<structure::tracks::LanedTrackPtrVariant> (
        lane.get_track ()));
    auto * obj = std::get<RegionT *> (obj_ref.get_object ());
    set_selection_handler_to_object (*obj);
    timeline_selections_manager_.append_to_selection (obj->get_uuid ());
    clip_editor_.set_region (obj->get_uuid (), track_id);
  }

  /**
   * @brief To be used by the backend.
   */
  auto create_audio_region_with_clip (
    AudioLane                        &lane,
    dsp::FileAudioSourceUuidReference clip_id,
    double                            startTicks) const
  {
    auto obj =
      get_builder<AudioRegion> ()
        .with_start_ticks (startTicks)
        .with_clip (std::move (clip_id))
        .build_in_registry ();
    return obj;
  }

  /**
   * @brief Creates and registers a new AudioClip and then creates and returns
   * an AudioRegion from it.
   *
   * Possible use cases: splitting audio regions, audio functions, recording.
   */
  auto create_audio_region_from_audio_buffer (
    AudioLane                       &lane,
    const utils::audio::AudioBuffer &buf,
    utils::audio::BitDepth           bit_depth,
    const utils::Utf8String         &clip_name,
    double                           start_ticks) const
  {
    auto clip = file_audio_source_registry_.create_object<dsp::FileAudioSource> (
      buf, bit_depth, sample_rate_provider_ (), bpm_provider_ (), clip_name);
    auto region =
      create_audio_region_with_clip (lane, std::move (clip), start_ticks);
    return region;
  }

  /**
   * @brief Used to create and add editor objects.
   *
   * @param region_qvar Clip editor region.
   * @param startTicks Start position of the object in ticks.
   * @param value Either pitch (int), automation point value (double) or chord
   * ID.
   */
template <RegionObject RegionT>
auto add_editor_object (
  RegionT                  &region,
  double                    startTicks,
  std::variant<int, double> value)
  -> RegionT::ArrangerObjectChildType * requires (
    !std::is_same_v<RegionT, AudioRegion>) {
    using ChildT = typename RegionT::ArrangerObjectChildType;
    auto builder =
      std::move (get_builder<ChildT> ().with_start_ticks (startTicks));
    if constexpr (std::is_same_v<ChildT, MidiNote>)
      {
        const auto ival = std::get<int> (value);
        assert (ival >= 0 && ival < 128);
        builder.with_pitch (ival);
      }
    else if constexpr (std::is_same_v<ChildT, AutomationPoint>)
      {
        builder.with_automatable_value (std::get<double> (value));
      }
    else if constexpr (std::is_same_v<ChildT, ChordObject>)
      {
        builder.with_chord_descriptor (std::get<int> (value));
      }
    auto obj_ref = builder.build_in_registry ();
    region.add_object (obj_ref);
    auto obj = std::get<ChildT *> (obj_ref.get_object ());
    {
      auto sel_mgr = get_selection_manager_for_object (*obj);
      set_selection_handler_to_object (*obj);
      sel_mgr.append_to_selection (obj->get_uuid ());
    }
    return obj;
  }

public :
    /**
     * @brief
     *
     * @param lane
     * @param clip_id
     * @param start_ticks
     * @return AudioRegion*
     */
    AudioRegion * add_audio_region_with_clip (
      structure::tracks::AudioLane     &lane,
      dsp::FileAudioSourceUuidReference clip_id,
      double                            start_ticks)
  {
    auto obj_ref =
      create_audio_region_with_clip (lane, std::move (clip_id), start_ticks);
    add_laned_object (lane, obj_ref);
    return std::get<AudioRegion *> (obj_ref.get_object ());
  }

  ScaleObject * add_scale_object (
    structure::tracks::ChordTrack             &chord_track,
    utils::QObjectUniquePtr<dsp::MusicalScale> scale,
    double                                     start_ticks)
  {
    auto obj_ref =
      get_builder<ScaleObject> ()
        .with_start_ticks (start_ticks)
        .with_scale (std::move (scale))
        .build_in_registry ();
    chord_track.ArrangerObjectOwner<ScaleObject>::add_object (obj_ref);
    return std::get<ScaleObject *> (obj_ref.get_object ());
  }

  Q_INVOKABLE Marker * addMarker (
    Marker::MarkerType               markerType,
    structure::tracks::MarkerTrack * markerTrack,
    const QString                   &name,
    double                           startTicks)

  {
    auto marker_ref =
      get_builder<Marker> ()
        .with_start_ticks (startTicks)
        .with_name (name)
        .build_in_registry ();
    markerTrack->add_object (marker_ref);
    return std::get<Marker *> (marker_ref.get_object ());
  }

  Q_INVOKABLE MidiRegion *
  addEmptyMidiRegion (structure::tracks::MidiLane * lane, double startTicks)
  {
    auto mr_ref =
      get_builder<MidiRegion> ()
        .with_start_ticks (startTicks)
        .build_in_registry ();
    add_laned_object (*lane, mr_ref);
    return std::get<MidiRegion *> (mr_ref.get_object ());
  }

  Q_INVOKABLE ChordRegion *
  addEmptyChordRegion (structure::tracks::ChordTrack * track, double startTicks)
  {
    auto cr_ref =
      get_builder<ChordRegion> ()
        .with_start_ticks (startTicks)
        .build_in_registry ();
    track->Track::add_region<ChordRegion> (cr_ref, nullptr, std::nullopt, true);
    return std::get<ChordRegion *> (cr_ref.get_object ());
  }

  Q_INVOKABLE AutomationRegion * addEmptyAutomationRegion (
    tracks::AutomationTrack * automationTrack,
    double                    startTicks)

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

  AudioRegion * add_empty_audio_region_for_recording (
    AudioLane               &lane,
    int                      num_channels,
    const utils::Utf8String &clip_name,
    double                   start_ticks)
  {
    auto clip = file_audio_source_registry_.create_object<dsp::FileAudioSource> (
      num_channels, 1, sample_rate_provider_ (), bpm_provider_ (), clip_name);
    auto region_ref =
      create_audio_region_with_clip (lane, std::move (clip), start_ticks);
    add_laned_object (lane, region_ref);
    return std::get<AudioRegion *> (region_ref.get_object ());
  }

  Q_INVOKABLE AudioRegion * addAudioRegionFromFile (
    AudioLane *    lane,
    const QString &absPath,
    double         startTicks)
  {
    auto clip = file_audio_source_registry_.create_object<dsp::FileAudioSource> (
      utils::Utf8String::from_qstring (absPath), sample_rate_provider_ (),
      bpm_provider_ ());
    auto ar_ref =
      create_audio_region_with_clip (*lane, std::move (clip), startTicks);
    add_laned_object (*lane, ar_ref);
    return std::get<AudioRegion *> (ar_ref.get_object ());
  }

  /**
   * @brief Creates a MIDI region at @p lane from the given @p descr
   * starting at @p startTicks.
   */
  Q_INVOKABLE MidiRegion * addMidiRegionFromChordDescriptor (
    MidiLane *                  lane,
    const dsp::ChordDescriptor &descr,
    double                      startTicks);

  /**
   * @brief Creates a MIDI region at @p lane from MIDI file path @p abs_path
   * starting at @p startTicks.
   *
   * @param midi_track_idx The index of this track, starting from 0. This
   * will be sequential, ie, if idx 1 is requested and the MIDI file only
   * has tracks 5 and 7, it will use track 7.
   */
  Q_INVOKABLE MidiRegion * addMidiRegionFromMidiFile (
    MidiLane *     lane,
    const QString &absolutePath,
    double         startTicks,
    int            midiTrackIndex);

  Q_INVOKABLE MidiNote *
  addMidiNote (MidiRegion * region, double startTicks, int pitch)
  {
    return add_editor_object (*region, startTicks, pitch);
  }

  Q_INVOKABLE AutomationPoint *
  addAutomationPoint (AutomationRegion * region, double startTicks, double value)

  {
    return add_editor_object (*region, startTicks, value);
  }

  Q_INVOKABLE ChordObject *
  addChordObject (ChordRegion * region, double startTicks, const int chordIndex)

  {
    return add_editor_object (*region, startTicks, chordIndex);
  }

  /**
   * @brief Temporary solution for splitting regions.
   *
   * Eventually need a public method here that not only creates the region but
   * also adds it to the project like the rest of the public API here.
   */
  auto create_audio_region_from_audio_buffer_FIXME (
    AudioLane                       &lane,
    const utils::audio::AudioBuffer &buf,
    utils::audio::BitDepth           bit_depth,
    const utils::Utf8String         &clip_name,
    double                           start_ticks) const
  {
    return create_audio_region_from_audio_buffer (
      lane, buf, bit_depth, clip_name, start_ticks);
  }

  template <structure::arrangement::FinalArrangerObjectSubclass ObjT>
  auto clone_new_object_identity (const ObjT &other) const
  {
    if constexpr (std::is_same_v<ObjT, AudioRegion>)
      {
        return object_registry_.clone_object (
          other, tempo_map_, object_registry_, file_audio_source_registry_,
          [this] () { return settings_manager_.get_musicalMode (); });
      }
    else if constexpr (RegionObject<ObjT>)
      {
        return object_registry_.clone_object (
          other, tempo_map_, object_registry_, file_audio_source_registry_);
      }
    else if constexpr (std::is_same_v<ObjT, Marker>)
      {
        return object_registry_.clone_object (
          other, tempo_map_, other.markerType ());
      }
    else
      {
        return object_registry_.clone_object (other, tempo_map_);
      }
  }

// deprecated - no use case for snapshots, just serialize straight to/from json
#if 0
  template <structure::arrangement::FinalArrangerObjectSubclass ObjT>
  auto clone_object_snapshot (const ObjT &other, QObject &owner) const
  {
    ObjT * new_obj{};
    if constexpr (std::is_same_v<ObjT, AudioRegion>)
      {
        // TODO
        new_obj = utils::clone_qobject (
          other, &owner, utils::ObjectCloneType::Snapshot, object_registry_,
          file_audio_source_registry_);
      }
    else if constexpr (std::is_same_v<ObjT, Marker>)
      {
        new_obj = utils::clone_qobject (
          other, &owner, utils::ObjectCloneType::Snapshot, object_registry_,
          [] (const auto &name) { return true; });
      }
    else
      {
        new_obj = utils::clone_qobject (
          other, &owner, utils::ObjectCloneType::Snapshot, object_registry_);
      }
    return new_obj;
  }
#endif

  template <structure::arrangement::FinalArrangerObjectSubclass ObjT>
  auto get_selection_manager_for_object (const ObjT &obj) const
  {
    if constexpr (TimelineObject<ObjT>)
      {
        return timeline_selections_manager_;
      }
    else if constexpr (std::is_same_v<ObjT, MidiNote>)
      {
        return midi_selections_manager_;
      }
    else if constexpr (std::is_same_v<ObjT, AutomationPoint>)
      {
        return automation_selections_manager_;
      }
    else if constexpr (std::is_same_v<ObjT, ChordObject>)
      {
        return chord_selections_manager_;
      }
    else if constexpr (std::is_same_v<ObjT, AudioSourceObject>)
      {
        return audio_selections_manager_;
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
    obj.set_selection_status_getter ([sel_mgr] (const ArrangerObject::Uuid &id) {
      return sel_mgr.is_selected (id);
    });
  }

private:
  const dsp::TempoMap            &tempo_map_;
  ArrangerObjectRegistry         &object_registry_;
  dsp::FileAudioSourceRegistry   &file_audio_source_registry_;
  gui::SettingsManager           &settings_manager_;
  gui::SnapGrid                  &snap_grid_timeline_;
  gui::SnapGrid                  &snap_grid_editor_;
  std::function<sample_rate_t ()> sample_rate_provider_;
  std::function<bpm_t ()>         bpm_provider_;
  ArrangerObjectSelectionManager  audio_selections_manager_;
  ArrangerObjectSelectionManager  timeline_selections_manager_;
  ArrangerObjectSelectionManager  midi_selections_manager_;
  ArrangerObjectSelectionManager  chord_selections_manager_;
  ArrangerObjectSelectionManager  automation_selections_manager_;
  ClipEditor                     &clip_editor_;
};
}
