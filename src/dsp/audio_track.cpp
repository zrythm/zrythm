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

void
AudioTrack::init_loaded ()
{
  AutomatableTrack::init_loaded ();
  LanedTrackImpl::init_loaded ();
  ChannelTrack::init_loaded ();
  auto         tracklist = get_tracklist ();
  unsigned int samplerate =
    (tracklist && tracklist->project_)
      ? tracklist->project_->audio_engine_->sample_rate_
      : AUDIO_ENGINE->sample_rate_;
  rt_stretcher_ = stretcher_new_rubberband (samplerate, 2, 1.0, 1.0, true);
}

AudioTrack::AudioTrack (const std::string &name, int pos, unsigned int samplerate)
    : Track (Track::Type::Audio, name, pos, PortType::Audio, PortType::Audio)
{
  color_ = Color ("#19664c");
  /* signal-audio also works */
  icon_name_ = "view-media-visualization";
  rt_stretcher_ = stretcher_new_rubberband (samplerate, 2, 1.0, 1.0, true);

  generate_automation_tracks ();
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