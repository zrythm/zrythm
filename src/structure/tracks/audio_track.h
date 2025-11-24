// SPDX-FileCopyrightText: © 2018-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/stretcher.h"
#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{
/**
 * @brief Track containing AudioRegion's.
 */
class AudioTrack : public Track
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  using AudioRegion = arrangement::AudioRegion;

  AudioTrack (FinalTrackDependencies dependencies);

  friend void init_from (
    AudioTrack            &obj,
    const AudioTrack      &other,
    utils::ObjectCloneType clone_type);

  // ========================================================================
  // QML Interface
  // ========================================================================

  // ========================================================================

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
  }
  friend void from_json (const nlohmann::json &j, AudioTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
  }

  bool initialize ();

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
