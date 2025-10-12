// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"
#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/audio_source_object.h"
#include "structure/arrangement/fadeable_object.h"

#include <juce_wrapper.h>

namespace zrythm::structure::arrangement
{

/**
 * A region for playing back audio samples.
 */
class AudioRegion final
    : public ArrangerObject,
      public ArrangerObjectOwner<AudioSourceObject>
{
  Q_OBJECT
  Q_PROPERTY (
    AudioRegion::MusicalMode musicalMode READ musicalMode WRITE setMusicalMode
      NOTIFY musicalModeChanged)
  Q_PROPERTY (float gain READ gain WRITE setGain NOTIFY gainChanged)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    AudioRegion,
    audioSources,
    AudioSourceObject)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  /**
   * Musical mode setting for audio regions.
   */
  enum class MusicalMode : std::uint8_t
  {
    /** Inherit from global musical mode setting. */
    Inherit,
    /** Musical mode off - don't auto-stretch when BPM changes. */
    Off,
    /** Musical mode on - auto-stretch when BPM changes. */
    On,
  };

  using GlobalMusicalModeGetter = std::function<bool ()>;

  /**
   * Number of frames for built-in fade (additional to object fades).
   */
  static constexpr int BUILTIN_FADE_FRAMES = 10;

public:
  AudioRegion (
    const dsp::TempoMap          &tempo_map,
    ArrangerObjectRegistry       &object_registry,
    dsp::FileAudioSourceRegistry &file_audio_source_registry,
    GlobalMusicalModeGetter       musical_mode_getter,
    QObject *                     parent = nullptr) noexcept;

  // ========================================================================
  // QML Interface
  // ========================================================================

  MusicalMode      musicalMode () const { return musical_mode_; }
  Q_INVOKABLE bool effectivelyInMusicalMode () const;
  void             setMusicalMode (MusicalMode musical_mode)
  {
    if (musical_mode_ != musical_mode)
      {
        musical_mode_ = musical_mode;
        Q_EMIT musicalModeChanged (musical_mode);
      }
  }
  Q_SIGNAL void musicalModeChanged (MusicalMode musical_mode);

  float gain () const { return gain_.load (); }
  void  setGain (float gain)
  {
    gain = std::clamp (gain, 0.f, 2.f);
    if (qFuzzyCompare (gain_, gain))
      return;
    gain_.store (gain);
    Q_EMIT gainChanged (gain);
  }
  Q_SIGNAL void gainChanged (float gain);

  // ========================================================================

  /**
   * @brief Set the AudioSourceObject to the region.
   *
   * This must be called before the AudioRegion is used.
   */
  void set_source (const ArrangerObjectUuidReference &source);

  juce::PositionableAudioSource &get_audio_source () const;

  std::string
  get_field_name_for_serialization (const AudioSourceObject *) const override
  {
    return "audioSources";
  }

private:
  friend void init_from (
    AudioRegion           &obj,
    const AudioRegion     &other,
    utils::ObjectCloneType clone_type);

  static constexpr auto kGainKey = "gain"sv;
  static constexpr auto kMusicalModeKey = "musicalMode"sv;
  friend void           to_json (nlohmann::json &j, const AudioRegion &region)
  {
    to_json (j, static_cast<const ArrangerObject &> (region));
    to_json (j, static_cast<const ArrangerObjectOwner &> (region));
    j[kGainKey] = region.gain_.load ();
    j[kMusicalModeKey] = region.musical_mode_;
  }
  friend void from_json (const nlohmann::json &j, AudioRegion &region)
  {
    from_json (j, static_cast<ArrangerObject &> (region));
    from_json (j, static_cast<ArrangerObjectOwner &> (region));
    float gain{};
    j.at (kGainKey).get_to (gain);
    region.gain_.store (gain);
    j.at (kMusicalModeKey).get_to (region.musical_mode_);
  }

private:
  dsp::FileAudioSourceRegistry &file_audio_source_registry_;

  /** Gain to apply to the audio (amplitude 0.0-2.0). */
  std::atomic<float> gain_ = 1.0f;

  /** Musical mode setting. */
  MusicalMode musical_mode_{};

  GlobalMusicalModeGetter global_musical_mode_getter_;

  BOOST_DESCRIBE_CLASS (
    AudioRegion,
    (ArrangerObject),
    (),
    (),
    (gain_, musical_mode_))
};
}

DEFINE_ENUM_FORMATTER (
  zrythm::structure::arrangement::AudioRegion::MusicalMode,
  MusicalMode,
  QT_TRANSLATE_NOOP_UTF8 ("MusicalMode", "Inherit"),
  QT_TRANSLATE_NOOP_UTF8 ("MusicalMode", "Off"),
  QT_TRANSLATE_NOOP_UTF8 ("MusicalMode", "On"));
