// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// SPDX-FileCopyrightText: © 2018-2020, 2024 Alexandros Theodotou <alex@zrythm.org>

#include <cmath>
#include <cstdlib>

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/audio_track.h"
#include "gui/dsp/automation_tracklist.h"
#include "gui/dsp/engine.h"
#include "gui/dsp/pool.h"
#include "gui/dsp/port.h"
#include "dsp/stretcher.h"
#include "gui/dsp/tracklist.h"

#include "utils/objects.h"

using namespace zrythm;

AudioTrack::AudioTrack (const std::string &name, int pos, unsigned int samplerate)
    : Track (Track::Type::Audio, name, pos, PortType::Audio, PortType::Audio),
      samplerate_ (samplerate)
{
  color_ = Color (QColor ("#2BD700"));
  /* signal-audio also works */
  icon_name_ = "view-media-visualization";
  rt_stretcher_ = dsp::Stretcher::create_rubberband (samplerate_, 2, 1.0, 1.0, true);
  automation_tracklist_->setParent (this);
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
  rt_stretcher_ =
    dsp::Stretcher::create_rubberband (samplerate_, 2, 1.0, 1.0, true);
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
AudioTrack::timestretch_buf (
  const AudioRegion * r,
  AudioClip *         clip,
  unsigned_frame_t    in_frame_offset,
  double              timestretch_ratio,
  float *             lbuf_after_ts,
  float *             rbuf_after_ts,
  unsigned_frame_t    out_frame_offset,
  unsigned_frame_t    frames_to_process)
{
  z_return_if_fail (r && rt_stretcher_);
  rt_stretcher_->set_time_ratio (1.0 / timestretch_ratio);
  auto in_frames_to_process =
    (unsigned_frame_t) (frames_to_process * timestretch_ratio);
  z_debug (
    "in frame offset {}, out frame offset {}, "
    "in frames to process {}, out frames to process {}",
    in_frame_offset, out_frame_offset, in_frames_to_process,
    frames_to_process);
  z_return_if_fail (
    (in_frame_offset + in_frames_to_process)
    <= (unsigned_frame_t) clip->get_num_frames ());
  auto retrieved = rt_stretcher_->stretch (
    &clip->get_samples ().getReadPointer (0)[in_frame_offset],
    clip->get_num_channels () == 1
      ? &clip->get_samples ().getReadPointer (0)[in_frame_offset]
      : &clip->get_samples ().getReadPointer (1)[in_frame_offset],
    in_frames_to_process, &lbuf_after_ts[out_frame_offset],
    &rbuf_after_ts[out_frame_offset], (size_t) frames_to_process);
  z_return_if_fail ((unsigned_frame_t) retrieved == frames_to_process);
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
AudioTrack::append_ports (std::vector<Port *> &ports, bool include_plugins) const
{
  ChannelTrack::append_member_ports (ports, include_plugins);
  ProcessableTrack::append_member_ports (ports, include_plugins);
  RecordableTrack::append_member_ports (ports, include_plugins);
}

void
AudioTrack::init_after_cloning (const AudioTrack &other)
{
  samplerate_ = other.samplerate_;
  rt_stretcher_ =
    dsp::Stretcher::create_rubberband (samplerate_, 2, 1.0, 1.0, true);
  Track::copy_members_from (other);
  ChannelTrack::copy_members_from (other);
  ProcessableTrack::copy_members_from (other);
  AutomatableTrack::copy_members_from (other);
  RecordableTrack::copy_members_from (other);
  LanedTrackImpl::copy_members_from (other);
}
