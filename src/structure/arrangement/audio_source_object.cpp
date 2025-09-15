// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/audio_source_object.h"

namespace zrythm::structure::arrangement
{
AudioSourceObject::AudioSourceObject (
  const dsp::TempoMap              &tempo_map,
  dsp::FileAudioSourceRegistry     &registry,
  dsp::FileAudioSourceUuidReference source,
  QObject *                         parent)
    : ArrangerObject (Type::AudioSourceObject, tempo_map, {}, parent),
      registry_ (registry), source_id_ (std::move (source))
{
  generate_audio_source ();
}

juce::PositionableAudioSource &
AudioSourceObject::get_audio_source () const
{
  return *source_;
}

void
AudioSourceObject::generate_audio_source ()
{
  auto audio_source = source_id_.get_object_as<dsp::FileAudioSource> ();
  // juce API takes non-const reference but it doesn't seem to modify it (and it
  // doesn't make sense to modify it...) so we const-cast
  source_ = std::make_unique<juce::MemoryAudioSource> (
    const_cast<juce::AudioSampleBuffer &> (
      static_cast<const juce::AudioSampleBuffer &> (audio_source->get_samples ())),
    false);
}

void
init_from (
  AudioSourceObject       &obj,
  const AudioSourceObject &other,
  utils::ObjectCloneType   clone_type)
{
  if (clone_type == utils::ObjectCloneType::NewIdentity)
    {
      obj.source_id_ = obj.registry_.clone_object (
        *other.source_id_.get_object_as<dsp::FileAudioSource> ());
    }
  else if (clone_type == utils::ObjectCloneType::Snapshot)
    {
      obj.source_id_ = other.source_id_;
    }
}

void
from_json (const nlohmann::json &j, AudioSourceObject &obj)
{
  from_json (j, static_cast<ArrangerObject &> (obj));
  obj.source_id_ = { obj.registry_ };
  j.at (AudioSourceObject::kFileAudioSourceKey).get_to (obj.source_id_);
  obj.generate_audio_source ();
}
}
