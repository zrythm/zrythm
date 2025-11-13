// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_all.h"

namespace zrythm::structure::arrangement
{
/**
 * @brief Factory for arranger objects.
 */
class ArrangerObjectFactory
{
public:
  using SampleRateProvider = std::function<sample_rate_t ()>;
  using BpmProvider = std::function<bpm_t ()>;

  struct Dependencies
  {
    using MusicalModeGetter = std::function<bool ()>;
    using LastTimelineObjectLengthProvider = std::function<double ()>;
    using LastEditorObjectLengthProvider = std::function<double ()>;
    using AutomationCurveAlgorithmProvider =
      std::function<dsp::CurveOptions::Algorithm ()>;

    const dsp::TempoMap             &tempo_map_;
    ArrangerObjectRegistry          &object_registry_;
    dsp::FileAudioSourceRegistry    &file_audio_source_registry_;
    MusicalModeGetter                musical_mode_getter_;
    LastTimelineObjectLengthProvider last_timeline_obj_len_provider_;
    LastEditorObjectLengthProvider   last_editor_obj_len_provider_;
    AutomationCurveAlgorithmProvider automation_curve_algorithm_provider_;
  };

  ArrangerObjectFactory (
    Dependencies       dependencies,
    SampleRateProvider sample_rate_provider,
    BpmProvider        bpm_provider)
      : dependencies_ (std::move (dependencies)),
        sample_rate_provider_ (std::move (sample_rate_provider)),
        bpm_provider_ (std::move (bpm_provider))
  {
  }

  template <structure::arrangement::FinalArrangerObjectSubclass ObjT>
  class Builder
  {
    friend class ArrangerObjectFactory;

  private:
    explicit Builder (Dependencies dependencies)
        : dependencies_ (std::move (dependencies))
    {
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
            dependencies_.tempo_map_, dependencies_.object_registry_,
            dependencies_.file_audio_source_registry_,
            dependencies_.musical_mode_getter_);
        }
      else if constexpr (RegionObject<ObjT>)
        {
          ret = std::make_unique<ObjT> (
            dependencies_.tempo_map_, dependencies_.object_registry_,
            dependencies_.file_audio_source_registry_);
        }
      else if constexpr (std::is_same_v<ObjT, Marker>)
        {
          ret = std::make_unique<ObjT> (
            dependencies_.tempo_map_, Marker::MarkerType::Custom);
        }
      else if constexpr (std::is_same_v<ObjT, AudioSourceObject>)
        {
          ret = std::make_unique<ObjT> (
            dependencies_.tempo_map_, dependencies_.file_audio_source_registry_,
            dsp::FileAudioSourceUuidReference{
              dependencies_.file_audio_source_registry_ });
        }
      else
        {
          ret = std::make_unique<ObjT> (dependencies_.tempo_map_);
        }
      return ret;
    }

    auto build_in_registry ()
    {
      auto obj_ref = [&] () {
        auto obj_unique_ptr = build_empty ();
        dependencies_.object_registry_.register_object (obj_unique_ptr.get ());
        structure::arrangement::ArrangerObjectUuidReference ret_ref{
          obj_unique_ptr->get_uuid (), dependencies_.object_registry_
        };
        obj_unique_ptr.release ();
        return ret_ref;
      }();

      auto * obj = std::get<ObjT *> (obj_ref.get_object ());

      if constexpr (RegionObject<ObjT>)
        {
          obj->loopRange ()->setTrackBounds (true);
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
                        dependencies_.last_timeline_obj_len_provider_ ();
                    }
                  else
                    {
                      len_ticks = dependencies_.last_editor_obj_len_provider_ ();
                    }
                  get_object_bounds (*obj)->length ()->setTicks (len_ticks);
                }
            }
          obj->position ()->setTicks (*start_ticks_);
        }

      if (clip_id_)
        {
          if constexpr (std::is_same_v<ObjT, AudioRegion>)
            {
              auto source_object = dependencies_.object_registry_.create_object<
                AudioSourceObject> (
                dependencies_.tempo_map_,
                dependencies_.file_audio_source_registry_, clip_id_.value ());
              obj->set_source (source_object);
              obj->bounds ()->length ()->setSamples (
                clip_id_.value ()
                  .template get_object_as<dsp::FileAudioSource> ()
                  ->get_num_frames ());
            }
        }

      if (end_ticks_)
        {
          if constexpr (BoundedObject<ObjT>)
            {
              get_object_bounds (*obj)->length ()->setTicks (
                *end_ticks_ - obj->position ()->ticks ());
            }
        }

      if (name_)
        {
          if constexpr (NamedObject<ObjT>)
            {
              if constexpr (RegionObject<ObjT>)
                {
                  obj->name ()->setName (*name_);
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
              obj->setValue (static_cast<float> (*automatable_value_));
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
            dependencies_.automation_curve_algorithm_provider_ ());
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
    Dependencies                                     dependencies_;
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
    return Builder<ObjT> (dependencies_);
  }

public:
  /**
   * @brief To be used by the backend.
   */
  auto create_audio_region_with_clip (
    dsp::FileAudioSourceUuidReference clip_id,
    double                            startTicks) const
  {
    auto obj =
      get_builder<structure::arrangement::AudioRegion> ()
        .with_start_ticks (startTicks)
        .with_clip (std::move (clip_id))
        .build_in_registry ();
    return obj;
  }

  auto create_empty_audio_region_for_recording (
    int                      num_channels,
    const utils::Utf8String &clip_name,
    double                   start_ticks)
  {
    auto clip = dependencies_.file_audio_source_registry_.create_object<
      dsp::FileAudioSource> (
      num_channels, 1, sample_rate_provider_ (), bpm_provider_ (), clip_name);
    return create_audio_region_with_clip (std::move (clip), start_ticks);
  }

  auto create_audio_region_from_file (const QString &absPath, double startTicks)
  {
    auto clip = dependencies_.file_audio_source_registry_.create_object<
      dsp::FileAudioSource> (
      utils::Utf8String::from_qstring (absPath), sample_rate_provider_ (),
      bpm_provider_ ());
    return create_audio_region_with_clip (std::move (clip), startTicks);
  }

  /**
   * @brief Creates and registers a new AudioClip and then creates and returns
   * an AudioRegion from it.
   *
   * Possible use cases: splitting audio regions, audio functions, recording.
   */
  auto create_audio_region_from_audio_buffer (
    const utils::audio::AudioBuffer &buf,
    utils::audio::BitDepth           bit_depth,
    const utils::Utf8String         &clip_name,
    double                           start_ticks) const
  {
    auto clip = dependencies_.file_audio_source_registry_.create_object<
      dsp::FileAudioSource> (
      buf, bit_depth, sample_rate_provider_ (), bpm_provider_ (), clip_name);
    return create_audio_region_with_clip (std::move (clip), start_ticks);
  }

  /**
   * @brief Used to create and add editor objects.
   *
   * @param startTicks Start position of the object in ticks.
   * @param value Either pitch (int), automation point value (double) or chord
   * ID.
   */
  template <structure::arrangement::RegionObject RegionT>
  auto create_editor_object (double startTicks, std::variant<int, double> value)
    requires (!std::is_same_v<RegionT, structure::arrangement::AudioRegion>)
  {
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
    return builder.build_in_registry ();
  }

  template <structure::arrangement::FinalArrangerObjectSubclass ObjT>
  auto clone_new_object_identity (const ObjT &other) const
  {
    if constexpr (std::is_same_v<ObjT, AudioRegion>)
      {
        return dependencies_.object_registry_.clone_object (
          other, dependencies_.tempo_map_, dependencies_.object_registry_,
          dependencies_.file_audio_source_registry_,
          dependencies_.musical_mode_getter_);
      }
    else if constexpr (RegionObject<ObjT>)
      {
        return dependencies_.object_registry_.clone_object (
          other, dependencies_.tempo_map_, dependencies_.object_registry_,
          dependencies_.file_audio_source_registry_);
      }
    else if constexpr (std::is_same_v<ObjT, Marker>)
      {
        return dependencies_.object_registry_.clone_object (
          other, dependencies_.tempo_map_, other.markerType ());
      }
    else
      {
        return dependencies_.object_registry_.clone_object (
          other, dependencies_.tempo_map_);
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

private:
  Dependencies       dependencies_;
  SampleRateProvider sample_rate_provider_;
  BpmProvider        bpm_provider_;
};
}
