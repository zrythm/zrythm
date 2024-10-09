// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// SPDX-FileCopyrightText: © 2018-2020, 2024 Alexandros Theodotou <alex@zrythm.org>

#include <cmath>
#include <cstdlib>

#include "common/dsp/audio_track.h"
#include "common/dsp/automation_tracklist.h"
#include "common/dsp/engine.h"
#include "common/dsp/pool.h"
#include "common/dsp/port.h"
#include "common/dsp/stretcher.h"
#include "common/dsp/tracklist.h"
#include "common/utils/objects.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

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
AudioTrack::clear_objects ()
{
  LanedTrackImpl::clear_objects ();
  AutomatableTrack::clear_objects ();
}

void
AudioTrack::get_regions_in_range (
  std::vector<Region *> &regions,
  const Position *       p1,
  const Position *       p2)
{
  LanedTrackImpl::get_regions_in_range (regions, p1, p2);
  AutomatableTrack::get_regions_in_range (regions, p1, p2);
}

bool
AudioTrack::validate () const
{
  return Track::validate_base () && LanedTrackImpl::validate_base ()
         && AutomatableTrack::validate_base () && ChannelTrack::validate_base ();
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