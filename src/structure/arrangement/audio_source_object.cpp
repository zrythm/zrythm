// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/audio_source_object.h"
#include "utils/serialization.h"

namespace zrythm::structure::arrangement
{
AudioSourceObject::AudioSourceObject (
  const dsp::TempoMapWrapper       &tempo_map_wrapper,
  utils::IObjectRegistry           &registry,
  dsp::FileAudioSourceUuidReference source,
  QObject *                         parent)
    : ArrangerObject (
        Type::AudioSourceObject,
        tempo_map_wrapper,
        ArrangerObjectFeatures::ClipOwned,
        parent),
      registry_ (registry), source_id_ (std::move (source))
{
}

dsp::FileAudioSource &
AudioSourceObject::file_audio_source () const
{
  return *source_id_.get_object_as<dsp::FileAudioSource> ();
}

dsp::FileAudioSourceUuidReference
AudioSourceObject::audio_source_ref () const
{
  return source_id_;
}

void
init_from (
  AudioSourceObject       &obj,
  const AudioSourceObject &other,
  utils::ObjectCloneType   clone_type)
{
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
  obj.source_id_ = other.source_id_;
}

void
from_json (const nlohmann::json &j, AudioSourceObject &obj)
{
  from_json (j, static_cast<ArrangerObject &> (obj));
  obj.source_id_ = { obj.registry_ };
  j.at (AudioSourceObject::kFileAudioSourceKey).get_to (obj.source_id_);
}

AudioSourceObject::~AudioSourceObject () = default;

void
to_json (nlohmann::json &j, const AudioSourceObject &obj)
{
  to_json (j, static_cast<const ArrangerObject &> (obj));
  j[AudioSourceObject::kFileAudioSourceKey] = obj.source_id_;
}

}
