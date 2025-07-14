// SPDX-FileCopyrightText: © 2018-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/stretcher.h"
#include "structure/arrangement/audio_region.h"
#include "structure/tracks/automatable_track.h"
#include "structure/tracks/channel_track.h"
#include "structure/tracks/laned_track.h"
#include "structure/tracks/recordable_track.h"

namespace zrythm::structure::tracks
{
/**
 * The AudioTrack class represents an audio track in the project. It
 * inherits from ChannelTrack, LanedTrack, and AutomatableTrack, providing
 * functionality for managing audio channels, lanes, and automation.
 */
class AudioTrack final
    : public QObject,
      public ChannelTrack,
      public LanedTrackImpl<AudioLane>,
      public RecordableTrack,
      public utils::InitializableObject<AudioTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (AudioTrack)
  DEFINE_LANED_TRACK_QML_PROPERTIES (AudioTrack)
  DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES (AudioTrack)
  DEFINE_CHANNEL_TRACK_QML_PROPERTIES (AudioTrack)
  DEFINE_RECORDABLE_TRACK_QML_PROPERTIES (AudioTrack)

  friend class InitializableObject;

public:
  using AudioRegion = arrangement::AudioRegion;

private:
  AudioTrack (
    dsp::FileAudioSourceRegistry    &file_audio_source_registry,
    TrackRegistry                   &track_registry,
    PluginRegistry                  &plugin_registry,
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry,
    ArrangerObjectRegistry          &obj_registry,
    bool                             new_identity);

public:
  void init_loaded (
    PluginRegistry                  &plugin_registry,
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry) override;

  friend void init_from (
    AudioTrack            &obj,
    const AudioTrack      &other,
    utils::ObjectCloneType clone_type);

  void temporary_virtual_method_hack () const override { }

  /**
   * Wrapper for audio tracks to fill in StereoPorts from the timeline data.
   *
   * @note The engine splits the cycle so transport loop related logic is not
   * needed.
   *
   * @param stereo_ports StereoPorts to fill.
   */
  void fill_events (
    const EngineProcessTimeInfo                  &time_nfo,
    std::pair<std::span<float>, std::span<float>> stereo_ports);

  void clear_objects () override;

  void get_regions_in_range (
    std::vector<arrangement::ArrangerObjectUuidReference> &regions,
    std::optional<signed_frame_t>                          p1,
    std::optional<signed_frame_t>                          p2) override;

  void timestretch_buf (
    const AudioRegion *    r,
    dsp::FileAudioSource * clip,
    unsigned_frame_t       in_frame_offset,
    double                 timestretch_ratio,
    float *                lbuf_after_ts,
    float *                rbuf_after_ts,
    unsigned_frame_t       out_frame_offset,
    unsigned_frame_t       frames_to_process);

private:
  friend void to_json (nlohmann::json &j, const AudioTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
    to_json (j, static_cast<const ProcessableTrack &> (track));
    to_json (j, static_cast<const AutomatableTrack &> (track));
    to_json (j, static_cast<const RecordableTrack &> (track));
    to_json (j, static_cast<const ChannelTrack &> (track));
    to_json (j, static_cast<const LanedTrackImpl &> (track));
  }
  friend void from_json (const nlohmann::json &j, AudioTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
    from_json (j, static_cast<ProcessableTrack &> (track));
    from_json (j, static_cast<AutomatableTrack &> (track));
    from_json (j, static_cast<RecordableTrack &> (track));
    from_json (j, static_cast<ChannelTrack &> (track));
    from_json (j, static_cast<LanedTrackImpl &> (track));
  }

  bool initialize ();

  void set_playback_caches () override;

private:
  /** Real-time time stretcher. */
  std::unique_ptr<zrythm::dsp::Stretcher> rt_stretcher_;

  /**
   * The samplerate @ref rt_stretcher_ is working with.
   *
   * Should be initialized with the samplerate of the audio engine.
   *
   * Not to be serialized.
   */
  unsigned int samplerate_ = 0;
};
}
