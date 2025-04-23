// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2018 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * ---
 */

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "dsp/stretcher.h"
#include "utils/audio_file.h"
#include "utils/logger.h"
#include "utils/math.h"
#include "utils/mem.h"

#include <rubberband/rubberband-c.h>

namespace zrythm::dsp
{

struct Stretcher::Impl
{
  Backend backend{};

  /** For rubberband API. */
  RubberBandState rubberband_state{};

  unsigned int samplerate{};
  unsigned int channels{};

  bool is_realtime{};

  /**
   * Size of the block to process in each iteration.
   *
   * Somewhere around 6k should be fine.
   */
  unsigned int block_size{};
};

Stretcher::Stretcher () : pimpl_ (std::make_unique<Impl> ()) { }

std::unique_ptr<Stretcher>
Stretcher::create_rubberband (
  unsigned samplerate,
  unsigned channels,
  double   time_ratio,
  double   pitch_ratio,
  bool     realtime)
{
  if (samplerate <= 0)
    {
      throw std::invalid_argument (
        fmt::format ("Invalid samplerate of {}", samplerate));
    }

  auto  stretcher = std::unique_ptr<Stretcher> (new Stretcher ());
  auto &impl = *stretcher->pimpl_;

  impl.backend = Backend::Rubberband;
  impl.samplerate = samplerate;
  impl.channels = channels;
  impl.is_realtime = realtime;

  RubberBandOptions opts = 0;

  if (realtime)
    {
      opts =
        RubberBandOptionProcessRealTime | RubberBandOptionTransientsCrisp
        | RubberBandOptionDetectorCompound | RubberBandOptionPhaseLaminar
        | RubberBandOptionThreadingAlways | RubberBandOptionWindowStandard
        | RubberBandOptionSmoothingOff | RubberBandOptionFormantShifted
        | RubberBandOptionPitchHighSpeed | RubberBandOptionChannelsApart;
      impl.block_size = 16000;
      impl.rubberband_state =
        rubberband_new (samplerate, channels, opts, time_ratio, pitch_ratio);

      /* feed it samples so it is ready to use */
#if 0
      unsigned int samples_required =
        rubberband_get_samples_required (
          pimpl_->rubberband_state);
      float in_samples_l[samples_required];
      float in_samples_r[samples_required];
      for (unsigned int i; i < samples_required; i++)
        {
          in_samples_l[i] = 0.f;
          in_samples_r[i] = 0.f;
        }
      const float * in_samples[channels];
      in_samples[0] = in_samples_l;
      if (channels == 2)
        in_samples[1] = in_samples_r;
      rubberband_process (
        pimpl_->rubberband_state, in_samples,
        samples_required, false);
      std::this_thread::sleep_for (std::chrono::milliseconds (1000));
      int avail =
        rubberband_available (
          pimpl_->rubberband_state);
      float * out_samples[2] = {
        in_samples_l, in_samples_r };
      size_t retrieved_out_samples =
        rubberband_retrieve (
          pimpl_->rubberband_state, out_samples,
          (unsigned int) avail);
      z_debug (
        "required: {}, available {}, retrieved {}",
        samples_required, avail,
        retrieved_out_samples);
      samples_required =
        rubberband_get_samples_required (
          pimpl_->rubberband_state);
      rubberband_process (
        pimpl_->rubberband_state, in_samples,
        samples_required, false);
      std::this_thread::sleep_for (std::chrono::milliseconds (1000));
      avail =
        rubberband_available (
          pimpl_->rubberband_state);
      retrieved_out_samples =
        rubberband_retrieve (
          pimpl_->rubberband_state, out_samples,
          (unsigned int) avail);
      z_debug (
        "required: {}, available {}, "
        "retrieved {}",
        samples_required, avail,
        retrieved_out_samples);
#endif
    }
  else
    {
      opts =
        RubberBandOptionProcessOffline | RubberBandOptionStretchElastic
        | RubberBandOptionTransientsCrisp | RubberBandOptionDetectorCompound
        | RubberBandOptionPhaseLaminar | RubberBandOptionThreadingNever
        | RubberBandOptionWindowStandard | RubberBandOptionSmoothingOff
        | RubberBandOptionFormantShifted | RubberBandOptionPitchHighQuality
        | RubberBandOptionChannelsApart
      /* use finer engine if rubberband v3 */
#if RUBBERBAND_API_MAJOR_VERSION > 2 \
  || (RUBBERBAND_API_MAJOR_VERSION == 2 && RUBBERBAND_API_MINOR_VERSION >= 7)
        | RubberBandOptionEngineFiner
#endif
        ;
      impl.block_size = 6000;
      impl.rubberband_state =
        rubberband_new (samplerate, channels, opts, time_ratio, pitch_ratio);
      rubberband_set_max_process_size (impl.rubberband_state, impl.block_size);
    }
  rubberband_set_default_debug_level (0);

  z_debug (
    "created rubberband stretcher: time ratio: {:f}, latency: {}", time_ratio,
    stretcher->get_latency ());

  return stretcher;
}

signed_frame_t
Stretcher::stretch (
  const float * in_samples_l,
  const float * in_samples_r,
  size_t        in_samples_size,
  float *       out_samples_l,
  float *       out_samples_r,
  size_t        out_samples_wanted)
{
  using rubberband_int_t = unsigned int;
  z_info ("{}: in samples size: {}", __func__, in_samples_size);
  z_return_val_if_fail (in_samples_l, -1);

  /*rubberband_reset (pimpl_->rubberband_state);*/

  /* create the de-interleaved array */
  unsigned int channels = in_samples_r ? 2 : 1;
  z_return_val_if_fail (pimpl_->channels == channels, -1);
  std::array<const float *, 2> in_samples{ nullptr, nullptr };
  in_samples[0] = in_samples_l;
  if (channels == 2)
    in_samples[1] = in_samples_r;
  std::array<float *, 2> out_samples = { out_samples_l, out_samples_r };

  if (pimpl_->is_realtime)
    {
      rubberband_set_max_process_size (
        pimpl_->rubberband_state,
        static_cast<rubberband_int_t> (in_samples_size));
    }
  else
    {
      /* tell rubberband how many input samples it
       * will receive */
      rubberband_set_expected_input_duration (
        pimpl_->rubberband_state,
        static_cast<rubberband_int_t> (in_samples_size));

      rubberband_study (
        pimpl_->rubberband_state, in_samples.data (),
        static_cast<rubberband_int_t> (in_samples_size), 1);
    }
  unsigned int samples_required =
    rubberband_get_samples_required (pimpl_->rubberband_state);
  z_debug (
    "samples required: {}, latency: {}", samples_required,
    rubberband_get_latency (pimpl_->rubberband_state));
  rubberband_process (
    pimpl_->rubberband_state, in_samples.data (),
    static_cast<rubberband_int_t> (in_samples_size), false);

  /* get the output data */
  int avail = rubberband_available (pimpl_->rubberband_state);

  /* if the wanted amount of samples are not ready,
   * fill with silence */
  if (avail < (int) out_samples_wanted)
    {
      z_debug ("not enough samples available");
      return static_cast<signed_frame_t> (out_samples_wanted);
    }

  z_debug ("samples wanted {} (avail {})", out_samples_wanted, avail);
  size_t retrieved_out_samples = rubberband_retrieve (
    pimpl_->rubberband_state, out_samples.data (),
    static_cast<rubberband_int_t> (out_samples_wanted));
  z_warn_if_fail (retrieved_out_samples == out_samples_wanted);

  z_debug ("out samples size: {}", retrieved_out_samples);

  return static_cast<signed_frame_t> (retrieved_out_samples);
}

void
Stretcher::set_time_ratio (double ratio)
{
  rubberband_set_time_ratio (pimpl_->rubberband_state, ratio);
}

unsigned int
Stretcher::get_latency () const
{
  return rubberband_get_latency (pimpl_->rubberband_state);
}

zrythm::utils::audio::AudioBuffer
Stretcher::stretch_interleaved (zrythm::utils::audio::AudioBuffer &in_samples)
{
  using rubberband_int_t = unsigned int;
  assert (
    in_samples.getNumSamples () % static_cast<int> (pimpl_->channels) == 0);
  assert (in_samples.getNumChannels () == 1);
  const auto in_samples_per_channel =
    in_samples.getNumSamples () / static_cast<int> (pimpl_->channels);
  z_debug ("num input samples: {}", in_samples_per_channel);

  /* create the de-interleaved array */
  unsigned int       channels = pimpl_->channels;
  std::vector<float> in_buffers_l (
    static_cast<size_t> (in_samples_per_channel), 0.f);
  std::vector<float> in_buffers_r (
    static_cast<size_t> (in_samples_per_channel), 0.f);
  for (const auto i : std::views::iota (0, in_samples_per_channel))
    {
      in_buffers_l[static_cast<size_t> (i)] = in_samples.getSample (
        0, static_cast<int> (i) * static_cast<int> (channels));
      if (channels == 2)
        in_buffers_r[static_cast<size_t> (i)] = in_samples.getSample (
          0, static_cast<int> (i) * static_cast<int> (channels) + 1);
    }
  const float * in_buffers[2] = { in_buffers_l.data (), in_buffers_r.data () };

  /* tell rubberband how many input samples it will
   * receive */
  rubberband_set_expected_input_duration (
    pimpl_->rubberband_state,
    static_cast<rubberband_int_t> (in_samples_per_channel));

  /* study first */
  auto samples_to_read = static_cast<size_t> (in_samples_per_channel);
  while (samples_to_read > 0)
    {
      /* samples to read now */
      unsigned int read_now =
        (unsigned int) std::min ((size_t) pimpl_->block_size, samples_to_read);

      /* read */
      rubberband_study (
        pimpl_->rubberband_state, in_buffers, read_now,
        read_now == samples_to_read);

      /* remaining samples to read */
      samples_to_read -= read_now;
    }
  z_warn_if_fail (samples_to_read == 0);

  /* create the out sample arrays */
  // float * out_samples[channels];
  auto out_samples_size = static_cast<size_t> (utils::math::round_to_signed_64 (
    rubberband_get_time_ratio (pimpl_->rubberband_state)
    * in_samples_per_channel));
  zrythm::utils::audio::AudioBuffer out_samples (
    static_cast<int> (channels), static_cast<int> (out_samples_size));

  /* process */
  size_t processed = 0;
  size_t total_out_frames = 0;
  while (processed < static_cast<size_t> (in_samples_per_channel))
    {
      size_t in_chunk_size =
        rubberband_get_samples_required (pimpl_->rubberband_state);
      const auto samples_left =
        static_cast<size_t> (in_samples_per_channel) - processed;

      if (samples_left < in_chunk_size)
        {
          in_chunk_size = samples_left;
        }

      /* move the in buffers */
      const float * tmp_in_arrays[2] = {
        in_buffers[0] + processed, in_buffers[1] + processed
      };

      /* process */
      rubberband_process (
        pimpl_->rubberband_state, tmp_in_arrays,
        static_cast<rubberband_int_t> (in_chunk_size),
        samples_left == in_chunk_size);

      processed += in_chunk_size;

      /*z_debug ("processed {}, in samples {}",*/
      /*processed, in_samples_size);*/

      const auto avail =
        (size_t) rubberband_available (pimpl_->rubberband_state);

      /* retrieve the output data in temporary arrays */
      std::vector<float> tmp_out_l (avail);
      std::vector<float> tmp_out_r (avail);
      float * tmp_out_arrays[2] = { tmp_out_l.data (), tmp_out_r.data () };
      size_t             out_chunk_size = rubberband_retrieve (
        pimpl_->rubberband_state, tmp_out_arrays,
        static_cast<rubberband_int_t> (avail));

      /* save the result */
      for (const auto i : std::views::iota (0zu, channels))
        {
          for (const auto j : std::views::iota (0zu, out_chunk_size))
            {
              out_samples.setSample (
                static_cast<int> (i), static_cast<int> (j + total_out_frames),
                tmp_out_arrays[i][j]);
            }
        }

      total_out_frames += out_chunk_size;
    }

  z_debug (
    "retrieved {} samples (expected {})", total_out_frames, out_samples_size);
  z_warn_if_fail (
    /* allow 1 sample earlier */
    total_out_frames <= out_samples_size
    && total_out_frames >= out_samples_size - 1);

  out_samples.interleave_samples ();
  return out_samples;
}

Stretcher::~Stretcher ()
{
  if (pimpl_->rubberband_state)
    {
      rubberband_delete (pimpl_->rubberband_state);
    }
}

} // namespace zrythm::dsp
