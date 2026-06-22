// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object_all.h"
#include "utils/debug.h"
#include "utils/float_ranges.h"
#include "utils/logger.h"
#include "utils/math_utils.h"

#include <fmt/format.h>

namespace zrythm::structure::arrangement
{

AudioRegion::AudioRegion (
  const dsp::TempoMapWrapper &tempo_map_wrapper,
  utils::IObjectRegistry     &object_registry,
  utils::AppSettings         &app_settings,
  QObject *                   parent) noexcept
    : ArrangerObject (
        Type::AudioRegion,
        tempo_map_wrapper,
        ArrangerObjectFeatures::Region | ArrangerObjectFeatures::Fading,
        parent),
      ArrangerObjectOwner (object_registry, *this), app_settings_ (app_settings),
      last_effective_musical_mode_ (effectivelyInMusicalMode ())
{
  connect (
    &app_settings_, &utils::AppSettings::musicalModeChanged, this,
    [this] () { reevaluate_effective_musical_mode (); });
}

void
AudioRegion::set_source (const ArrangerObjectUuidReference &source)
{
  add_object (source);
  init_length_from_clip ();
}

void
AudioRegion::setMusicalMode (MusicalMode musical_mode)
{
  if (musical_mode_ != musical_mode)
    {
      musical_mode_ = musical_mode;
      reevaluate_effective_musical_mode ();
      Q_EMIT musicalModeChanged (musical_mode);
    }
}

bool
AudioRegion::effectivelyInMusicalMode () const
{
  switch (musical_mode_)
    {
    case MusicalMode::Inherit:
      return app_settings_.musicalMode ();
    case MusicalMode::Off:
      return false;
    case MusicalMode::On:
      return true;
    }
  z_return_val_if_reached (false);
}

void
AudioRegion::reevaluate_effective_musical_mode ()
{
  const bool now_effective = effectivelyInMusicalMode ();
  if (now_effective != last_effective_musical_mode_)
    {
      last_effective_musical_mode_ = now_effective;
      update_time_format ();
      Q_EMIT effectivelyInMusicalModeChanged ();
    }
}

float
AudioRegion::sourceBpm () const
{
  const auto &children = get_children_view ();
  if (children.empty ())
    return 0.f;
  return children.front ()->file_audio_source ().source_bpm ();
}

void
AudioRegion::init_length_from_clip ()
{
  z_return_if_fail (!get_children_view ().empty ());
  auto      &file_source = get_children_view ().front ()->file_audio_source ();
  const auto num_frames = file_source.get_num_frames ();
  const auto source_bpm = file_source.source_bpm ();
  const auto sr = get_tempo_map ().get_sample_rate ();
  auto *     len = bounds ()->length ();

  if (effectivelyInMusicalMode () && source_bpm > 0.f)
    {
      const double musical_ticks =
        (static_cast<double> (num_frames) / sr) * (source_bpm / 60.0)
        * dsp::TempoMap::get_ppq ();
      len->setMode (dsp::AtomicPosition::TimeFormat::Musical);
      len->setTicks (musical_ticks);
    }
  else
    {
      const double seconds = static_cast<double> (num_frames) / sr;
      len->setMode (dsp::AtomicPosition::TimeFormat::Absolute);
      len->setSeconds (seconds);
    }
}

void
AudioRegion::update_time_format ()
{
  z_return_if_fail (!get_children_view ().empty ());
  auto      &file_source = get_children_view ().front ()->file_audio_source ();
  const auto source_bpm = file_source.source_bpm ();
  const bool want_musical = effectivelyInMusicalMode () && source_bpm > 0.f;
  const auto target =
    want_musical
      ? dsp::AtomicPosition::TimeFormat::Musical
      : dsp::AtomicPosition::TimeFormat::Absolute;
  // Ticks-per-second at the clip's source tempo. Stable, unlike the project
  // tempo that AtomicPosition::setMode would convert through.
  const double factor =
    (source_bpm / 60.0) * static_cast<double> (dsp::TempoMap::get_ppq ());

  const auto convert = [&] (dsp::AtomicPositionQmlAdapter * pos) {
    if (pos->mode () == target)
      return;
    if (want_musical)
      {
        // Currently Absolute: seconds -> ticks at the source tempo.
        const double secs = pos->seconds ();
        pos->setMode (dsp::AtomicPosition::TimeFormat::Musical);
        pos->setTicks (secs * factor);
      }
    else
      {
        // Currently Musical: ticks -> seconds at the source tempo.
        const double ticks = pos->ticks ();
        pos->setMode (dsp::AtomicPosition::TimeFormat::Absolute);
        pos->setSeconds (ticks / factor);
      }
  };

  // Convert the loop points BEFORE the length: the trackBounds connection
  // (loopable_object.cpp) sets loop_end = length via setTicks when the length
  // changes, so loop_end must already be in the target format by then, or that
  // setTick would convert through the project tempo and corrupt it.
  convert (loopRange ()->loopEndPosition ());
  if (!loopRange ()->trackBounds ())
    {
      convert (loopRange ()->clipStartPosition ());
      convert (loopRange ()->loopStartPosition ());
    }
  convert (bounds ()->length ());
}

void
init_from (
  AudioRegion           &obj,
  const AudioRegion     &other,
  utils::ObjectCloneType clone_type)
{
  obj.gain_.store (other.gain_.load ());
  obj.musical_mode_ = other.musical_mode_;
  obj.last_effective_musical_mode_ = obj.effectivelyInMusicalMode ();
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
to_json (nlohmann::json &j, const AudioRegion &region)
{
  to_json (j, static_cast<const ArrangerObject &> (region));
  to_json (
    j, static_cast<const ArrangerObjectOwner<AudioSourceObject> &> (region));
  j[AudioRegion::kGainKey] = region.gain_.load ();
  j[AudioRegion::kMusicalModeKey] = region.musical_mode_;
}

void
from_json (const nlohmann::json &j, AudioRegion &region)
{
  from_json (j, static_cast<ArrangerObject &> (region));
  from_json (j, static_cast<ArrangerObjectOwner<AudioSourceObject> &> (region));
  float gain{};
  j.at (AudioRegion::kGainKey).get_to (gain);
  region.gain_.store (gain);
  j.at (AudioRegion::kMusicalModeKey).get_to (region.musical_mode_);
}

}
