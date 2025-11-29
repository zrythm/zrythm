// SPDX-FileCopyrightText: © 2019-2020, 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstddef>

#include "utils/audio.h"
#include "utils/units.h"

namespace zrythm::dsp
{

/**
 * @brief Time and pitch stretching API.
 */
class Stretcher
{
public:
  enum class Backend
  {
    /** Rubberband. */
    Rubberband,

    /** Paulstretch. */
    Paulstretch,

    /** SBSMS - Subband Sinusoidal Modeling Synthesis. */
    SBSMS,
  };

  /**
   * Create a new Stretcher using the rubberband backend.
   *
   * @param samplerate The new samplerate.
   * @param time_ratio The ratio to multiply time by (eg if the
   *   BPM is doubled, this will be 0.5).
   * @param pitch_ratio The ratio to pitch by. This will normally
   *   be 1.0 when time-stretching).
   * @param realtime Whether to perform realtime stretching
   *   (lower quality but fast enough to be used real-time).
   *
   * @throw std::runtime_error If parameters are invalid.
   */
  static std::unique_ptr<Stretcher> create_rubberband (
    units::sample_rate_t samplerate,
    unsigned             channels,
    double               time_ratio,
    double               pitch_ratio,
    bool                 realtime);

public:
  ~Stretcher ();

  // Delete copy/move until properly implemented
  Stretcher (const Stretcher &) = delete;
  Stretcher &operator= (const Stretcher &) = delete;
  Stretcher (Stretcher &&) = delete;
  Stretcher &operator= (Stretcher &&) = delete;

  /**
   * Perform stretching.
   *
   * @param in_samples_l The left samples.
   * @param in_samples_r The right channel samples. If
   *   this is nullptr, the audio is assumed to be mono.
   * @param in_samples_size The number of input samples
   *   per channel.
   *
   * @return The number of output samples generated per
   *   channel.
   */
  signed_frame_t stretch (
    const float * in_samples_l,
    const float * in_samples_r,
    size_t        in_samples_size,
    float *       out_samples_l,
    float *       out_samples_r,
    size_t        out_samples_wanted);

  /**
   * Get latency in number of samples.
   */
  unsigned int get_latency () const;

  void set_time_ratio (double ratio);

  /**
   * Perform stretching.
   *
   * @warning Not real-time safe, does allocations.
   *
   * @param in_samples Input samples (interleaved).
   * @return The output samples (interleaved).
   */
  zrythm::utils::audio::AudioBuffer
  stretch_interleaved (zrythm::utils::audio::AudioBuffer &in_samples);

private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;

  // Private constructor used by factory method
  Stretcher ();
};

} // namespace zrythm::dsp
