// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object_all.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/logger.h"
#include "utils/math.h"

#include <fmt/format.h>

namespace zrythm::structure::arrangement
{

AudioRegion::AudioRegion (
  const dsp::TempoMap          &tempo_map,
  ArrangerObjectRegistry       &object_registry,
  dsp::FileAudioSourceRegistry &file_audio_source_registry,
  GlobalMusicalModeGetter       musical_mode_getter,
  QObject *                     parent) noexcept
    : ArrangerObject (
        Type::AudioRegion,
        tempo_map,
        ArrangerObjectFeatures::Region | ArrangerObjectFeatures::Fading,
        parent),
      ArrangerObjectOwner (object_registry, file_audio_source_registry, *this),
      file_audio_source_registry_ (file_audio_source_registry),
      global_musical_mode_getter_ (std::move (musical_mode_getter))
{
}

void
AudioRegion::set_source (const ArrangerObjectUuidReference &source)
{
  add_object (source);
}

bool
AudioRegion::effectivelyInMusicalMode () const
{
  switch (musical_mode_)
    {
    case MusicalMode::Inherit:
      return global_musical_mode_getter_ ();
    case MusicalMode::Off:
      return false;
    case MusicalMode::On:
      return true;
    }
  z_return_val_if_reached (false);
}

void
init_from (
  AudioRegion           &obj,
  const AudioRegion     &other,
  utils::ObjectCloneType clone_type)
{
  obj.gain_.store (other.gain_.load ());
  obj.musical_mode_ = other.musical_mode_;
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
  init_from (
    static_cast<ArrangerObjectOwner<AudioSourceObject> &> (obj),
    static_cast<const ArrangerObjectOwner<AudioSourceObject> &> (other),
    clone_type);
}

juce::PositionableAudioSource &
AudioRegion::get_audio_source () const
{
  assert (get_children_vector ().size () == 1);
  auto * audio_source_obj = get_children_view ().front ();
  return audio_source_obj->get_audio_source ();
}

void
AudioRegion::prepare_to_play (size_t max_expected_samples, double sample_rate)
{
  tmp_buf_ = std::make_unique<juce::AudioSampleBuffer> (2, max_expected_samples);
  audio_source_buffer_ =
    std::make_unique<juce::AudioSampleBuffer> (2, max_expected_samples);
  get_audio_source ().prepareToPlay (
    static_cast<int> (max_expected_samples), sample_rate);
}

void
AudioRegion::release_resources ()
{
  tmp_buf_.reset ();
  audio_source_buffer_.reset ();
  get_audio_source ().releaseResources ();
}
}
