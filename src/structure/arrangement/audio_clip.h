// SPDX-FileCopyrightText: © 2019-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/timestretch_engine.h"
#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/audio_source_object.h"
#include "structure/arrangement/clip.h"
#include "structure/arrangement/fadeable_object.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace zrythm::structure::arrangement
{

/**
 * @brief A clip for playing back audio samples.
 *
 * The effective timebase is resolved via @ref timebaseProvider() (inherited from
 * ArrangerObject). The timestretch algorithm is stored directly on the clip.
 */
class AudioClip final : public Clip, public ArrangerObjectOwner<AudioSourceObject>
{
  Q_OBJECT
  Q_PROPERTY (double sourceBpm READ sourceBpm CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::StretchOptions::Algorithm stretchAlgorithm READ
      stretchAlgorithm WRITE setStretchAlgorithm NOTIFY stretchAlgorithmChanged)
  Q_PROPERTY (float gain READ gain WRITE setGain NOTIFY gainChanged)
  Q_PROPERTY (
    zrythm::structure::arrangement::ArrangerObjectFadeRange * fadeRange READ
      fadeRange CONSTANT)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    AudioClip,
    audioSources,
    AudioSourceObject)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  static constexpr int BUILTIN_FADE_FRAMES = 10;

  AudioClip (
    const dsp::TempoMapWrapper &tempo_map_wrapper,
    utils::IObjectRegistry     &object_registry,
    QObject *                   parent = nullptr) noexcept;

  ArrangerObjectFadeRange * fadeRange () const { return fade_range_.get (); }

  [[nodiscard]] dsp::StretchOptions::Algorithm stretchAlgorithm () const
  {
    return algorithm_;
  }
  void          setStretchAlgorithm (dsp::StretchOptions::Algorithm a);
  Q_SIGNAL void stretchAlgorithmChanged (dsp::StretchOptions::Algorithm a);

  [[nodiscard]] dsp::StretchOptions::Algorithm
  effectiveStretchAlgorithm () const
  {
    return algorithm_;
  }

  double sourceBpm () const;

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

  void set_source (const ArrangerObjectUuidReference &source);

  juce::PositionableAudioSource &get_audio_source () const;

  std::string
  get_field_name_for_serialization (const AudioSourceObject *) const override
  {
    return "audioSources";
  }

  std::vector<ArrangerObjectListModel *> get_child_list_models () const override
  {
    return { ArrangerObjectOwner<AudioSourceObject>::get_model () };
  }

private:
  friend void init_from (
    AudioClip             &obj,
    const AudioClip       &other,
    utils::ObjectCloneType clone_type);

  void init_length_from_clip ();
  void update_warp_configuration ();

  static constexpr auto kGainKey = "gain"sv;
  static constexpr auto kAlgorithmKey = "stretchAlgorithm"sv;
  static constexpr auto kFadeRangeKey = "fadeRange"sv;
  friend void           to_json (nlohmann::json &j, const AudioClip &clip);
  friend void           from_json (const nlohmann::json &j, AudioClip &clip);

  std::atomic<float>             gain_ = 1.0f;
  dsp::StretchOptions::Algorithm algorithm_{
    dsp::StretchOptions::Algorithm::Polyphonic
  };
  utils::QObjectUniquePtr<ArrangerObjectFadeRange> fade_range_;

  BOOST_DESCRIBE_CLASS (AudioClip, (Clip), (), (), (gain_, algorithm_))
};

} // namespace zrythm::structure::arrangement
