// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_all.h"
#include "utils/qt.h"
#include "utils/registry_utils.h"

namespace zrythm::structure::arrangement
{
/**
 * @brief Factory for arranger objects.
 */
class ArrangerObjectFactory
{
public:
  using SampleRateProvider = std::function<units::sample_rate_t ()>;
  using BpmProvider = std::function<units::bpm_t ()>;

  struct Dependencies
  {
    using LastTimelineObjectLengthProvider = std::function<double ()>;
    using LastEditorObjectLengthProvider = std::function<double ()>;
    using AutomationCurveAlgorithmProvider =
      std::function<dsp::CurveOptions::Algorithm ()>;

    const dsp::TempoMapWrapper      &tempo_map_;
    utils::IObjectRegistry          &registry_;
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
      requires (std::is_same_v<ObjT, AudioClip>)
    {
      clip_id_.emplace (std::move (clip_id));
      return *this;
    }

  public:
    Builder &with_start_ticks (units::precise_tick_t start_ticks)
    {
      start_ticks_ = start_ticks;

      return *this;
    }

    Builder &with_end_ticks (units::precise_tick_t end_ticks)
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

    Builder &with_midi_channel (const int channel)
      requires (
        std::is_same_v<ObjT, MidiNote> || std::is_same_v<ObjT, MidiControlEvent>)
    {
      midi_channel_ = channel;
      return *this;
    }

    Builder &with_automatable_value (const double automatable_val)
      requires (std::is_same_v<ObjT, AutomationPoint>)
    {
      automatable_value_ = automatable_val;
      return *this;
    }

    Builder &with_chord_descriptor (
      dsp::MusicalNote                root,
      dsp::ChordType                  type,
      dsp::ChordAccent                accent = dsp::ChordAccent::None,
      int                             inversion = 0,
      std::optional<dsp::MusicalNote> bass = std::nullopt)
      requires (std::is_same_v<ObjT, ChordObject>)
    {
      chord_descr_ = utils::make_qobject_unique<dsp::ChordDescriptor> (
        root, type, accent, inversion, bass);
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
      if constexpr (ClipObject<ObjT>)
        {
          ret = std::make_unique<ObjT> (
            dependencies_.tempo_map_, dependencies_.registry_);
        }
      else if constexpr (std::is_same_v<ObjT, Marker>)
        {
          ret = std::make_unique<ObjT> (
            dependencies_.tempo_map_,
            marker_type_.value_or (Marker::MarkerType::Custom));
        }
      else if constexpr (std::is_same_v<ObjT, AudioSourceObject>)
        {
          utils::audio::AudioBuffer dummy_buf (1, 1);
          dummy_buf.clear ();
          auto file_audio_source = utils::create_object<dsp::FileAudioSource> (
            dependencies_.registry_, dummy_buf,
            utils::audio::BitDepth::BIT_DEPTH_32, units::sample_rate (44100),
            units::bpm (120.0), u8"dummy");
          ret = std::make_unique<ObjT> (
            dependencies_.tempo_map_, dependencies_.registry_,
            file_audio_source);
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
        dependencies_.registry_.register_object (*obj_unique_ptr);
        utils::TypedUuidReference<ObjT> ret_ref{
          obj_unique_ptr->get_uuid (), dependencies_.registry_
        };
        obj_unique_ptr.release ();
        return ret_ref;
      }();

      auto * obj = obj_ref.template get_object_as<ObjT> ();

      if constexpr (ClipObject<ObjT>)
        {
          obj->setTrackBounds (true);
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
                  obj->length ()->setTicks (len_ticks);
                }
            }
          obj->position ()->setTicks ((*start_ticks_).in (units::ticks));
        }

      if (clip_id_)
        {
          if constexpr (std::is_same_v<ObjT, AudioClip>)
            {
              auto source_object = utils::create_object<AudioSourceObject> (
                dependencies_.registry_, dependencies_.tempo_map_,
                dependencies_.registry_, clip_id_.value ());
              obj->set_source (source_object);
              // set_source() calls init_length_from_clip(), which sets the
              // clip length to the clip's full duration in the format for the
              // effective musical mode.
            }
        }

      if (end_ticks_)
        {
          if constexpr (BoundedObject<ObjT>)
            {
              obj->length ()->setTicks (
                (*end_ticks_).in (units::ticks) -obj->position ()->ticks ());
            }
        }

      if (name_)
        {
          if constexpr (NamedObject<ObjT>)
            {
              if constexpr (ClipObject<ObjT>)
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

      if (midi_channel_)
        {
          if constexpr (std::is_same_v<ObjT, MidiNote>)
            {
              obj->setMidiChannel (*midi_channel_);
            }
          else if constexpr (std::is_same_v<ObjT, MidiControlEvent>)
            {
              obj->setChannel (*midi_channel_);
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

      if (chord_descr_)
        {
          if constexpr (std::is_same_v<ObjT, ChordObject>)
            {
              auto * dest = obj->chordDescriptor ();
              dest->setRootNote (chord_descr_->rootNote ());
              dest->setChordType (chord_descr_->chordType ());
              dest->setChordAccent (chord_descr_->chordAccent ());
              dest->setInversion (chord_descr_->inversion ());
              if (chord_descr_->hasBass ())
                dest->setBassNote (chord_descr_->bassNote ());
            }
        }

      return obj_ref;
    }

  private:
    Dependencies                                     dependencies_;
    std::optional<dsp::FileAudioSourceUuidReference> clip_id_;
    std::optional<units::precise_tick_t>             start_ticks_;
    std::optional<units::precise_tick_t>             end_ticks_;
    std::optional<QString>                           name_;
    std::optional<int>                               pitch_;
    std::optional<double>                            automatable_value_;
    utils::QObjectUniquePtr<dsp::ChordDescriptor>    chord_descr_;
    utils::QObjectUniquePtr<dsp::MusicalScale>       scale_;
    std::optional<int>                               velocity_;
    std::optional<int>                               midi_channel_;
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
  auto create_audio_clip_with_clip (
    dsp::FileAudioSourceUuidReference clip_id,
    units::precise_tick_t             startTicks) const
  {
    auto obj =
      get_builder<structure::arrangement::AudioClip> ()
        .with_start_ticks (startTicks)
        .with_clip (std::move (clip_id))
        .build_in_registry ();
    return obj;
  }

  auto create_audio_clip_from_file (
    const QString        &absPath,
    units::precise_tick_t startTicks)
  {
    auto clip = utils::create_object<dsp::FileAudioSource> (
      dependencies_.registry_, utils::Utf8String::from_qstring (absPath),
      sample_rate_provider_ ());
    return create_audio_clip_with_clip (std::move (clip), startTicks);
  }

  /**
   * @brief Creates and registers a new AudioClip and then creates and returns
   * an AudioClip from it.
   *
   * Possible use cases: splitting audio clips, audio functions, recording.
   */
  auto create_audio_clip_from_audio_buffer (
    const utils::audio::AudioBuffer &buf,
    utils::audio::BitDepth           bit_depth,
    const utils::Utf8String         &clip_name,
    units::precise_tick_t            start_ticks) const
  {
    auto clip = utils::create_object<dsp::FileAudioSource> (
      dependencies_.registry_, buf, bit_depth, sample_rate_provider_ (),
      bpm_provider_ (), clip_name);
    return create_audio_clip_with_clip (std::move (clip), start_ticks);
  }

  /**
   * @brief Used to create and add editor objects.
   *
   * @param startTicks Start position of the object in ticks.
   * @param value Either pitch (int), automation point value (double) or chord
   * ID.
   */
  template <structure::arrangement::EditorObject ChildT>
  auto create_editor_object (
    units::precise_tick_t     startTicks,
    std::variant<int, double> value)
  {
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
        // Value was formerly a chord index into ChordEditor; now interpreted
        // as root note for the inline ChordDescriptor.
        const auto root = static_cast<dsp::MusicalNote> (std::get<int> (value));
        builder.with_chord_descriptor (root, dsp::ChordType::Major);
      }
    return builder.build_in_registry ();
  }

  template <structure::arrangement::FinalArrangerObjectSubclass ObjT>
  auto clone_new_object_identity (const ObjT &other) const
  {
    if constexpr (ClipObject<ObjT>)
      {
        return utils::clone_object (
          other, dependencies_.registry_, utils::ObjectCloneType::NewIdentity,
          dependencies_.tempo_map_, dependencies_.registry_);
      }
    else if constexpr (std::is_same_v<ObjT, Marker>)
      {
        return utils::clone_object (
          other, dependencies_.registry_, utils::ObjectCloneType::NewIdentity,
          dependencies_.tempo_map_, other.markerType ());
      }
    else if constexpr (std::is_same_v<ObjT, AudioSourceObject>)
      {
        return utils::clone_object (
          other, dependencies_.registry_, utils::ObjectCloneType::NewIdentity,
          dependencies_.tempo_map_, dependencies_.registry_,
          other.audio_source_ref ());
      }
    else
      {
        return utils::clone_object (
          other, dependencies_.registry_, utils::ObjectCloneType::NewIdentity,
          dependencies_.tempo_map_);
      }
  }

// deprecated - no use case for snapshots, just serialize straight to/from json
#if 0
  template <structure::arrangement::FinalArrangerObjectSubclass ObjT>
  auto clone_object_snapshot (const ObjT &other, QObject &owner) const
  {
    ObjT * new_obj{};
    if constexpr (std::is_same_v<ObjT, AudioClip>)
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
