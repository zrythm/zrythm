// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// SPDX-FileCopyrightText: © 2018-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>

#include "dsp/stretcher.h"
#include "structure/tracks/audio_track.h"

namespace zrythm::structure::tracks
{

AudioTrack::AudioTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::Audio,
        PortType::Audio,
        PortType::Audio,
        TrackFeatures::Automation | TrackFeatures::Lanes | TrackFeatures::Recording,
        dependencies.to_base_dependencies ())
{
  color_ = Color (QColor ("#2BD700"));
  /* signal-audio also works */
  icon_name_ = u8"view-media-visualization";

  processor_ = make_track_processor ();

  // TODO
  // samplerate_ = samplerate;
  // rt_stretcher_ = dsp::Stretcher::create_rubberband (samplerate_,
  // 2, 1.0, 1.0, true);
}

void
AudioTrack::timestretch_buf (
  const AudioRegion *    r,
  dsp::FileAudioSource * clip,
  unsigned_frame_t       in_frame_offset,
  double                 timestretch_ratio,
  float *                lbuf_after_ts,
  float *                rbuf_after_ts,
  unsigned_frame_t       out_frame_offset,
  unsigned_frame_t       frames_to_process)
{
// TODO
#if 0
  z_return_if_fail (r && rt_stretcher_);
  rt_stretcher_->set_time_ratio (1.0 / timestretch_ratio);
  auto in_frames_to_process =
    (unsigned_frame_t) (frames_to_process * timestretch_ratio);
  z_debug (
    "in frame offset {}, out frame offset {}, "
    "in frames to process {}, out frames to process {}",
    in_frame_offset, out_frame_offset, in_frames_to_process, frames_to_process);
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
#endif
}

void
init_from (
  AudioTrack            &obj,
  const AudioTrack      &other,
  utils::ObjectCloneType clone_type)
{
  obj.samplerate_ = other.samplerate_;
  obj.rt_stretcher_ =
    dsp::Stretcher::create_rubberband (obj.samplerate_, 2, 1.0, 1.0, true);
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
}
}
