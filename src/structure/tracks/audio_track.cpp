// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// SPDX-FileCopyrightText: © 2018-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>

#include <cmath>
#include <cstdlib>

#include "dsp/stretcher.h"
#include "engine/device_io/engine.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/port.h"
#include "structure/tracks/audio_track.h"
#include "structure/tracks/automation_tracklist.h"
#include "structure/tracks/tracklist.h"
#include "utils/objects.h"

namespace zrythm::structure::tracks
{

AudioTrack::AudioTrack (
  dsp::FileAudioSourceRegistry &file_audio_source_registry,
  TrackRegistry                &track_registry,
  PluginRegistry               &plugin_registry,
  PortRegistry                 &port_registry,
  ArrangerObjectRegistry       &obj_registry,
  bool                          new_identity)
    : Track (
        Track::Type::Audio,
        PortType::Audio,
        PortType::Audio,
        plugin_registry,
        port_registry,
        obj_registry),
      AutomatableTrack (file_audio_source_registry, port_registry, new_identity),
      ProcessableTrack (port_registry, new_identity),
      ChannelTrack (track_registry, plugin_registry, port_registry, new_identity),
      RecordableTrack (port_registry, new_identity)

{
  if (new_identity)
    {
      color_ = Color (QColor ("#2BD700"));
      /* signal-audio also works */
      icon_name_ = u8"view-media-visualization";
    }
  automation_tracklist_->setParent (this);
  // TODO
  // samplerate_ = samplerate;
  // rt_stretcher_ = dsp::Stretcher::create_rubberband (samplerate_,
  // 2, 1.0, 1.0, true);
}

void
AudioTrack::init_loaded (
  PluginRegistry &plugin_registry,
  PortRegistry   &port_registry)
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded (plugin_registry, port_registry);
  AutomatableTrack::init_loaded (plugin_registry, port_registry);
  ProcessableTrack::init_loaded (plugin_registry, port_registry);
  LanedTrackImpl::init_loaded (plugin_registry, port_registry);
// TODO
#if 0
  auto tracklist = get_tracklist ();
  samplerate_ =
    (tracklist && tracklist->project_)
      ? tracklist->project_->audio_engine_->sample_rate_
      : AUDIO_ENGINE->sample_rate_;
#endif
  rt_stretcher_ =
    dsp::Stretcher::create_rubberband (samplerate_, 2, 1.0, 1.0, true);
}

bool
AudioTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();
  init_recordable_track ([] () {
    return ZRYTHM_HAVE_UI
           && zrythm::gui::SettingsManager::get_instance ()->get_trackAutoArm ();
  });

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
AudioTrack::get_regions_in_range (
  std::vector<arrangement::ArrangerObjectUuidReference> &regions,
  std::optional<signed_frame_t>                          p1,
  std::optional<signed_frame_t>                          p2)
{
  LanedTrackImpl::get_regions_in_range (regions, p1, p2);
  AutomatableTrack::get_regions_in_range (regions, p1, p2);
}

void
AudioTrack::set_playback_caches ()
{
  LanedTrackImpl::set_playback_caches ();
  AutomatableTrack::set_playback_caches ();
}

void
AudioTrack::fill_events (
  const EngineProcessTimeInfo                  &time_nfo,
  std::pair<std::span<float>, std::span<float>> stereo_ports)
{
  fill_events_common (time_nfo, nullptr, stereo_ports);
}

void
AudioTrack::append_ports (std::vector<Port *> &ports, bool include_plugins) const
{
  ChannelTrack::append_member_ports (ports, include_plugins);
  ProcessableTrack::append_member_ports (ports, include_plugins);
  RecordableTrack::append_member_ports (ports, include_plugins);
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
  init_from (
    static_cast<ChannelTrack &> (obj),
    static_cast<const ChannelTrack &> (other), clone_type);
  init_from (
    static_cast<ProcessableTrack &> (obj),
    static_cast<const ProcessableTrack &> (other), clone_type);
  init_from (
    static_cast<AutomatableTrack &> (obj),
    static_cast<const AutomatableTrack &> (other), clone_type);
  init_from (
    static_cast<RecordableTrack &> (obj),
    static_cast<const RecordableTrack &> (other), clone_type);
  init_from (
    static_cast<AudioTrack::LanedTrackImpl &> (obj),
    static_cast<const AudioTrack::LanedTrackImpl &> (other), clone_type);
}
}
