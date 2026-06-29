// SPDX-FileCopyrightText: © 2018-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object_all.h"
#include "utils/logger.h"

#include <fmt/format.h>

namespace zrythm::structure::arrangement
{

AudioClip::AudioClip (
  const dsp::TempoMapWrapper &tempo_map_wrapper,
  utils::IObjectRegistry     &object_registry,
  QObject *                   parent) noexcept
    : Clip (Type::AudioClip, tempo_map_wrapper, parent),
      ArrangerObjectOwner (object_registry, *this)
{
  fade_range_ = utils::make_qobject_unique<ArrangerObjectFadeRange> (
    *position (), tempo_map_wrapper, this);
  QObject::connect (
    fadeRange (), &ArrangerObjectFadeRange::fadePropertiesChanged, this,
    &ArrangerObject::propertiesChanged);

  // Reconfigure warp whenever the effective timebase changes.
  QObject::connect (
    timebaseProvider (), &dsp::TimebaseProvider::effectiveTimebaseChanged, this,
    [this] () { update_warp_configuration (); });
}

void
AudioClip::set_source (const ArrangerObjectUuidReference &source)
{
  add_object (source);
  init_length_from_clip ();
  update_warp_configuration ();
}

void
AudioClip::setStretchAlgorithm (dsp::StretchOptions::Algorithm a)
{
  if (algorithm_ != a)
    {
      algorithm_ = a;
      Q_EMIT stretchAlgorithmChanged (a);
    }
}

void
AudioClip::update_warp_configuration ()
{
  auto * warp = contentWarp ();
  if (warp == nullptr)
    return;

  units::bpm_t bpm{};
  const auto  &children = get_children_view ();
  if (!children.empty ())
    bpm = children.front ()->file_audio_source ().source_bpm ();

  // Musical timebase OR no source BPM: identity warp (clip follows project).
  // Absolute timebase with known BPM: source-anchored warp.
  if (
    timebaseProvider ()->effectiveTimebase () == dsp::Timebase::Musical
    || bpm <= units::bpm (0.))
    warp->configure_as_project (bpm);
  else
    warp->configure_as_source (bpm);
}

double
AudioClip::sourceBpm () const
{
  const auto &children = get_children_view ();
  if (children.empty ())
    return 0.;
  return children.front ()->file_audio_source ().source_bpm ().in (units::bpm);
}

void
AudioClip::init_length_from_clip ()
{
  z_return_if_fail (!get_children_view ().empty ());
  auto      &file_source = get_children_view ().front ()->file_audio_source ();
  const auto num_frames = file_source.get_num_frames ();
  const auto source_bpm = file_source.source_bpm ();
  const auto sr = get_tempo_map ().get_sample_rate ();
  auto *     len = length ();

  const auto bpm =
    source_bpm > units::bpm (0.0)
      ? source_bpm
      : get_tempo_map ().tempo_at_tick (
          units::ticks (static_cast<int64_t> (position ()->ticks ())));
  const auto musical_ticks =
    ((units::samples (static_cast<double> (num_frames)) / sr) * bpm)
      .as (units::ticks);
  len->setTicks (musical_ticks.in (units::ticks));
}

void
init_from (
  AudioClip             &obj,
  const AudioClip       &other,
  utils::ObjectCloneType clone_type)
{
  obj.gain_.store (other.gain_.load ());
  obj.algorithm_ = other.algorithm_;
  init_from (
    static_cast<Clip &> (obj), static_cast<const Clip &> (other), clone_type);
  init_from (
    static_cast<ArrangerObjectOwner<AudioSourceObject> &> (obj),
    static_cast<const ArrangerObjectOwner<AudioSourceObject> &> (other),
    clone_type);
  if (obj.fade_range_ && other.fade_range_)
    init_from (*obj.fade_range_, *other.fade_range_, clone_type);
  obj.update_warp_configuration ();
}

juce::PositionableAudioSource &
AudioClip::get_audio_source () const
{
  assert (get_children_vector ().size () == 1);
  auto * audio_source_obj = get_children_view ().front ();
  return audio_source_obj->get_audio_source ();
}

void
to_json (nlohmann::json &j, const AudioClip &clip)
{
  to_json (j, static_cast<const Clip &> (clip));
  to_json (
    j, static_cast<const ArrangerObjectOwner<AudioSourceObject> &> (clip));
  j[AudioClip::kGainKey] = clip.gain_.load ();
  j[AudioClip::kAlgorithmKey] = clip.algorithm_;
  if (clip.fade_range_)
    to_json (j, *clip.fade_range_);
}

void
from_json (const nlohmann::json &j, AudioClip &clip)
{
  from_json (j, static_cast<Clip &> (clip));
  from_json (j, static_cast<ArrangerObjectOwner<AudioSourceObject> &> (clip));
  float gain{};
  j.at (AudioClip::kGainKey).get_to (gain);
  clip.gain_.store (gain);
  if (j.contains (AudioClip::kAlgorithmKey))
    {
      j.at (AudioClip::kAlgorithmKey).get_to (clip.algorithm_);
    }
  if (clip.fade_range_)
    from_json (j, *clip.fade_range_);
  clip.update_warp_configuration ();
}

} // namespace zrythm::structure::arrangement
