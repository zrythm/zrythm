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
  using TrackResolver = structure::tracks::TrackResolver;
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
    ArrangerObjectRegistry              &registry,
    TrackResolver                        track_resolver,
    gui::SettingsManager                &settings_mgr,
    std::function<dsp::FramesPerTick ()> frames_per_tick_getter,
    gui::SnapGrid                       &snap_grid_timeline,
    gui::SnapGrid                       &snap_grid_editor,
    AudioClipResolverFunc                clip_resolver,
    RegisterNewAudioClipFunc             clip_registration_func,
    std::function<sample_rate_t ()>      sample_rate_provider,
    std::function<bpm_t ()>              bpm_provider,
    ArrangerObjectSelectionManager       timeline_selections_manager,
    ArrangerObjectSelectionManager       midi_selections_manager,
    ArrangerObjectSelectionManager       chord_selections_manager,
    ArrangerObjectSelectionManager       automation_selections_manager,
    QObject *                            parent = nullptr)
      : QObject (parent), object_registry_ (registry),
        track_resolver_ (std::move (track_resolver)),
        settings_manager_ (settings_mgr),
        frames_per_tick_getter_ (std::move (frames_per_tick_getter)),
        snap_grid_timeline_ (snap_grid_timeline),
        snap_grid_editor_ (snap_grid_editor),
        clip_resolver_func_ (std::move (clip_resolver)),
        new_clip_registration_func_ (std::move (clip_registration_func)),
        sample_rate_provider_ (std::move (sample_rate_provider)),
        bpm_provider_ (std::move (bpm_provider)),
        timeline_selections_manager_ (timeline_selections_manager),
        midi_selections_manager_ (midi_selections_manager),
        chord_selections_manager_ (chord_selections_manager),
        automation_selections_manager_ (automation_selections_manager)
  {
  }

  static ArrangerObjectFactory * get_instance ();

  template <structure::arrangement::FinalArrangerObjectSubclass ObjT>
  class Builder
  {
    friend class ArrangerObjectFactory;

  private:
    explicit Builder (
      ArrangerObjectRegistry &registry,
      TrackResolver           track_resolver)
        : registry_ (registry), track_resolver_ (std::move (track_resolver))
    {
    }

    Builder &with_settings_manager (gui::SettingsManager &settings_manager)
    {
      settings_manager_ = settings_manager;
      return *this;
    }

    Builder &with_frames_per_tick (dsp::FramesPerTick framesPerTick)
    {
      frames_per_tick_ = framesPerTick;
      return *this;
    }

    Builder &with_clip_resolver (const AudioClipResolverFunc &clip_resolver)
    {
      clip_resolver_ = clip_resolver;
      return *this;
    }

    Builder &with_clip (const AudioClip::Uuid clip_id)
      requires (std::is_same_v<ObjT, AudioRegion>)
    {
      clip_id_.emplace (clip_id);
      return *this;
    }

  public:
    Builder &with_start_ticks (double start_ticks)
    {
      assert (settings_manager_.has_value ());
      assert (frames_per_tick_.has_value ());
      start_ticks_ = start_ticks;

      return *this;
    }

    Builder &with_end_ticks (double end_ticks)
      requires (std::derived_from<ObjT, structure::arrangement::BoundedObject>)
    {
      assert (frames_per_tick_.has_value ());
      end_ticks_ = end_ticks;
      return *this;
    }

    Builder &with_name (
      const QString                                     &name,
      structure::arrangement::NamedObject::NameValidator validator =
        [] (const utils::Utf8String &) { return true; })
      requires (std::derived_from<ObjT, structure::arrangement::NamedObject>)
    {
      name_ = name;
      name_validator_ = validator;
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

    Builder &with_scale (const dsp::MusicalScale scale)
      requires (std::is_same_v<ObjT, ScaleObject>)
    {
      scale_ = scale;
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
            registry_, track_resolver_, *clip_resolver_);
        }
      else if constexpr (std::is_same_v<ObjT, Marker>)
        {
          ret = std::make_unique<ObjT> (
            registry_, track_resolver_, *name_validator_);
        }
      else
        {
          ret = std::make_unique<ObjT> (registry_, track_resolver_);
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
      if (clip_id_)
        {
          if constexpr (std::is_same_v<ObjT, AudioRegion>)
            {
              obj->set_clip_id (*clip_id_);
              obj->set_end_pos_full_size (
                dsp::Position{
                  obj->get_position ().frames_
                    + clip_resolver_.value () (*clip_id_)->get_num_frames (),
                  to_ticks_per_frame (*frames_per_tick_) },
                *frames_per_tick_);
            }
        }

      if (end_ticks_)
        {
          if constexpr (
            std::derived_from<ObjT, structure::arrangement::BoundedObject>)
            {
              obj->set_end_pos_full_size (
                dsp::Position (*end_ticks_, *frames_per_tick_),
                *frames_per_tick_);
            }
        }

      // set start ticks after end ticks to avoid position validation failing
      // due to pos <= end_pos
      if (start_ticks_)
        {
          if (!end_ticks_ && !clip_id_)
            {
              if constexpr (
                std::derived_from<ObjT, structure::arrangement::BoundedObject>)
                {
                  double len_ticks{};
                  if constexpr (
                    std::derived_from<
                      ObjT, structure::arrangement::TimelineObject>)
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
                  obj->set_end_pos_full_size (
                    dsp::Position (*start_ticks_ + len_ticks, *frames_per_tick_),
                    *frames_per_tick_);
                }
            }
          obj->position_setter_validated (
            dsp::Position (*start_ticks_, *frames_per_tick_),
            to_ticks_per_frame (*frames_per_tick_));
        }

      if (name_)
        {
          if constexpr (
            std::derived_from<ObjT, structure::arrangement::NamedObject>)
            {
              obj->set_name (utils::Utf8String::from_qstring (*name_));
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
              obj->set_velocity (*velocity_);
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
              obj->set_scale (*scale_);
            }
        }

      if constexpr (std::is_same_v<ObjT, AutomationPoint>)
        {
          // TODO: add getter/setters on AutomationPoint
          obj->curve_opts_.algo_ = static_cast<dsp::CurveOptions::Algorithm> (
            gui::SettingsManager::automationCurveAlgorithm ());
        }

      if (chord_descriptor_index_)
        {
          if constexpr (std::is_same_v<ObjT, ChordObject>)
            {
              obj->set_chord_descriptor (*chord_descriptor_index_);
            }
        }

      return obj_ref;
    }

  private:
    ArrangerObjectRegistry              &registry_;
    TrackResolver                        track_resolver_;
    OptionalRef<gui::SettingsManager>    settings_manager_;
    std::optional<dsp::FramesPerTick>    frames_per_tick_;
    std::optional<AudioClipResolverFunc> clip_resolver_;
    std::optional<AudioClip::Uuid>       clip_id_;
    std::optional<double>                start_ticks_;
    std::optional<double>                end_ticks_;
    std::optional<QString>               name_;
    std::optional<structure::arrangement::NamedObject::NameValidator>
                                     name_validator_;
    std::optional<int>               pitch_;
    std::optional<double>            automatable_value_;
    std::optional<int>               chord_descriptor_index_;
    std::optional<dsp::MusicalScale> scale_;
    std::optional<int>               velocity_;
  };

  template <typename ObjT> auto get_builder () const
  {
    auto builder =
      Builder<ObjT> (object_registry_, track_resolver_)
        .with_frames_per_tick (frames_per_tick_getter_ ())
        .with_clip_resolver (clip_resolver_func_)
        .with_settings_manager (settings_manager_);
    return builder;
  }

private:
  /**
   * @brief Used for MIDI/Audio regions.
   */
  template <structure::tracks::TrackLaneSubclass TrackLaneT>
  void add_laned_object (TrackLaneT &lane, auto obj_ref)
  {
    using RegionT = typename TrackLaneT::RegionT;
    std::visit (
      [&] (auto &&track) {
        if constexpr (
          std::is_same_v<
            typename base_type<decltype (track)>::TrackLaneType, TrackLaneT>)
          {
            track->template add_region<RegionT> (
              obj_ref, nullptr, lane.get_index_in_track (), true);
          }
      },
      convert_to_variant<structure::tracks::LanedTrackPtrVariant> (
        lane.get_track ()));
    auto * obj = std::get<RegionT *> (obj_ref.get_object ());
    timeline_selections_manager_.append_to_selection (obj->get_uuid ());
    set_selection_handler_to_object (*obj);
  }

  /**
   * @brief To be used by the backend.
   */
  auto create_audio_region_with_clip (
    AudioLane             &lane,
    const AudioClip::Uuid &clip_id,
    double                 startTicks) const
  {
    auto obj =
      get_builder<AudioRegion> ()
        .with_start_ticks (startTicks)
        .with_clip (clip_id)
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
    auto clip = std::make_shared<AudioClip> (
      buf, bit_depth, sample_rate_provider_ (), bpm_provider_ (), clip_name);
    new_clip_registration_func_ (clip);
    auto region =
      create_audio_region_with_clip (lane, clip->get_uuid (), start_ticks);
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
  template <structure::arrangement::RegionWithChildren RegionT>
  auto add_editor_object (
    RegionT                  &region,
    double                    startTicks,
    std::variant<int, double> value) -> RegionT::ChildT *
  {
    using ChildT = typename RegionT::ChildT;
    auto builder = get_builder<ChildT> ().with_start_ticks (startTicks);
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
      sel_mgr.append_to_selection (obj->get_uuid ());
      set_selection_handler_to_object (*obj);
    }
    return obj;
  }

public:
  /**
   * @brief
   *
   * @param lane
   * @param clip_id
   * @param start_ticks
   * @return AudioRegion*
   */
  AudioRegion * add_audio_region_with_clip (
    structure::tracks::AudioLane &lane,
    const AudioClip::Uuid        &clip_id,
    double                        start_ticks)
  {
    // clip must already be registered before calling this method
    assert (clip_resolver_func_ (clip_id) != nullptr);
    auto obj_ref = create_audio_region_with_clip (lane, clip_id, start_ticks);
    add_laned_object (lane, obj_ref);
    return std::get<AudioRegion *> (obj_ref.get_object ());
  }

  ScaleObject * add_scale_object (
    structure::tracks::ChordTrack &chord_track,
    const dsp::MusicalScale       &scale,
    double                         start_ticks)
  {
    auto obj_ref =
      get_builder<ScaleObject> ()
        .with_start_ticks (start_ticks)
        .with_scale (scale)
        .build_in_registry ();
    chord_track.ArrangerObjectOwner<ScaleObject>::add_object (obj_ref);
    return std::get<ScaleObject *> (obj_ref.get_object ());
  }

  Q_INVOKABLE Marker * addMarker (
    structure::tracks::MarkerTrack * markerTrack,
    const QString                   &name,
    double                           startTicks)

  {
    auto marker_ref =
      get_builder<Marker> ()
        .with_start_ticks (startTicks)
        .with_name (
          name,
          [markerTrack] (const utils::Utf8String &inner_name) {
            return markerTrack->validate_marker_name (inner_name);
          })
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
  }

  AudioRegion * add_empty_audio_region_for_recording (
    AudioLane               &lane,
    int                      num_channels,
    const utils::Utf8String &clip_name,
    double                   start_ticks)
  {
    auto clip = std::make_shared<AudioClip> (
      num_channels, 1, sample_rate_provider_ (), bpm_provider_ (), clip_name);
    new_clip_registration_func_ (clip);
    auto region_ref =
      create_audio_region_with_clip (lane, clip->get_uuid (), start_ticks);
    add_laned_object (lane, region_ref);
    return std::get<AudioRegion *> (region_ref.get_object ());
  }

  Q_INVOKABLE AudioRegion * addAudioRegionFromFile (
    AudioLane *    lane,
    const QString &absPath,
    double         startTicks)
  {
    auto clip = std::make_shared<AudioClip> (
      utils::Utf8String::from_qstring (absPath), sample_rate_provider_ (),
      bpm_provider_ ());
    new_clip_registration_func_ (clip);
    auto ar_ref =
      create_audio_region_with_clip (*lane, clip->get_uuid (), startTicks);
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
          other, object_registry_, track_resolver_, clip_resolver_func_);
      }
    else
      {
        return object_registry_.clone_object (
          other, object_registry_, track_resolver_);
      }
  }

  template <structure::arrangement::FinalArrangerObjectSubclass ObjT>
  auto clone_object_snapshot (const ObjT &other, QObject &owner) const
  {
    ObjT * new_obj{};
    if constexpr (std::is_same_v<ObjT, AudioRegion>)
      {
        // TODO
        new_obj = utils::clone_qobject (
          other, &owner, utils::ObjectCloneType::Snapshot, object_registry_,
          track_resolver_, clip_resolver_func_);
      }
    else if constexpr (std::is_same_v<ObjT, Marker>)
      {
        new_obj = utils::clone_qobject (
          other, &owner, utils::ObjectCloneType::Snapshot, object_registry_,
          track_resolver_, [] (const auto &name) { return true; });
      }
    else
      {
        new_obj = utils::clone_qobject (
          other, &owner, utils::ObjectCloneType::Snapshot, object_registry_,
          track_resolver_);
      }
    return new_obj;
  }

  template <structure::arrangement::FinalArrangerObjectSubclass ObjT>
  auto get_selection_manager_for_object (const ObjT &obj) const
  {
    if constexpr (
      std::derived_from<ObjT, structure::arrangement::TimelineObject>)
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
  ArrangerObjectRegistry              &object_registry_;
  TrackResolver                        track_resolver_;
  gui::SettingsManager                &settings_manager_;
  std::function<dsp::FramesPerTick ()> frames_per_tick_getter_;
  gui::SnapGrid                       &snap_grid_timeline_;
  gui::SnapGrid                       &snap_grid_editor_;
  AudioClipResolverFunc                clip_resolver_func_;
  RegisterNewAudioClipFunc             new_clip_registration_func_;
  std::function<sample_rate_t ()>      sample_rate_provider_;
  std::function<bpm_t ()>              bpm_provider_;
  ArrangerObjectSelectionManager       timeline_selections_manager_;
  ArrangerObjectSelectionManager       midi_selections_manager_;
  ArrangerObjectSelectionManager       chord_selections_manager_;
  ArrangerObjectSelectionManager       automation_selections_manager_;
};
}
