// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// SPDX-FileCopyrightText: © 2018-2020, 2024 Alexandros Theodotou <alex@zrythm.org>

#include <cmath>
#include <cstdlib>

#include "dsp/audio_track.h"
#include "dsp/automation_tracklist.h"
#include "dsp/engine.h"
#include "dsp/pool.h"
#include "dsp/port.h"
#include "dsp/stretcher.h"
#include "dsp/tracklist.h"
#include "project.h"
#include "utils/objects.h"
#include "zrythm.h"
#include "zrythm_app.h"

AudioTrack::AudioTrack (const std::string &name, int pos, unsigned int samplerate)
    : Track (Track::Type::Audio, name, pos, PortType::Audio, PortType::Audio),
      samplerate_ (samplerate)
{
  color_ = Color ("#19664c");
  /* signal-audio also works */
  icon_name_ = "view-media-visualization";
  rt_stretcher_ = stretcher_new_rubberband (samplerate_, 2, 1.0, 1.0, true);
}

void
AudioTrack::init_loaded ()
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded ();
  AutomatableTrack::init_loaded ();
  ProcessableTrack::init_loaded ();
  LanedTrackImpl::init_loaded ();
  auto tracklist = get_tracklist ();
  samplerate_ =
    (tracklist && tracklist->project_)
      ? tracklist->project_->audio_engine_->sample_rate_
      : AUDIO_ENGINE->sample_rate_;
  rt_stretcher_ = stretcher_new_rubberband (samplerate_, 2, 1.0, 1.0, true);
}

bool
AudioTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();

  return true;
}

void
AudioTrack::set_playback_caches ()
{
  LanedTrackImpl::set_playback_caches ();
  AutomatableTrack::set_playback_caches ();
}

void
AudioTrack::update_name_hash (NameHashT new_name_hash)
{
  LanedTrackImpl::update_name_hash (new_name_hash);
  AutomatableTrack::update_name_hash (new_name_hash);
}

void
AudioTrack::fill_events (
  const EngineProcessTimeInfo &time_nfo,
  StereoPorts                 &stereo_ports)
{
  fill_events_common (time_nfo, nullptr, &stereo_ports);
}

void
AudioTrack::init_after_cloning (const AudioTrack &other)
{
  samplerate_ = other.samplerate_;
  rt_stretcher_ = stretcher_new_rubberband (samplerate_, 2, 1.0, 1.0, true);
  Track::copy_members_from (other);
  ChannelTrack::copy_members_from (other);
  ProcessableTrack::copy_members_from (other);
  AutomatableTrack::copy_members_from (other);
  RecordableTrack::copy_members_from (other);
  LanedTrackImpl::copy_members_from (other);
}

AudioTrack::~AudioTrack ()
{
  object_free_w_func_and_null (stretcher_free, rt_stretcher_);
}