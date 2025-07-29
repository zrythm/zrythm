// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/audio.h"
#include "utils/audio_file.h"
#include "utils/dsp.h"
#include "utils/exceptions.h"
#include "utils/gtest_wrapper.h"
#include "utils/logger.h"
#include "utils/math.h"

#if defined(__FreeBSD__)
#  include <sys/sysctl.h>
#endif

namespace zrythm::utils::audio
{

/**
 * Returns the number of frames in the given audio
 * file.
 */
unsigned_frame_t
get_num_frames (const fs::path &filepath)
{
  AudioFile  audio_file{ filepath };
  const auto metadata = audio_file.read_metadata ();
  return static_cast<unsigned_frame_t> (metadata.num_frames);
}

bool
frames_equal (
  std::span<const float> src1,
  std::span<const float> src2,
  float                  epsilon)
{
  z_return_val_if_fail (src1.size () == src2.size (), false);
  return std::ranges::equal (src1, src2, [epsilon] (float a, float b) {
    return utils::math::floats_near (a, b, epsilon);
  });
}

/**
 * Returns whether the file contents are equal.
 *
 * @param num_frames Maximum number of frames to check. Passing
 *   0 will check all frames.
 */
bool
audio_files_equal (
  const char * f1,
  const char * f2,
  size_t       num_frames,
  float        epsilon)
{
  using num_frames_t = decltype (num_frames);
  try
    {
      AudioFile c1{ f1 };
      AudioFile c2{ f2 };
      auto      metadata1 = c1.read_metadata ();
      auto      metadata2 = c2.read_metadata ();
      if (metadata1.channels != metadata2.channels)
        return false;

      zrythm::utils::audio::AudioBuffer buf1;
      zrythm::utils::audio::AudioBuffer buf2;
      c1.read_full (buf1, std::nullopt);
      c2.read_full (buf2, std::nullopt);

      if (num_frames == 0)
        {
          if (buf1.getNumSamples () == buf2.getNumSamples ())
            {
              num_frames = static_cast<num_frames_t> (buf1.getNumSamples ());
            }
          else
            {
              return false;
            }
        }
      z_return_val_if_fail (num_frames > 0, false);

      for (int i = 0; i < metadata1.channels; ++i)
        {
          if (
            !frames_equal (
              { buf1.getReadPointer (i), num_frames },
              { buf2.getReadPointer (i), num_frames }, epsilon))
            return false;
        }

      return true;
    }
  catch (const ZrythmException &e)
    {
      e.handle ("An error occurred while comparing files");
      return false;
    }
}

/**
 * Returns whether the frame buffer is empty (zero).
 */
bool
frames_silent (std::span<const float> src)
{
  for (size_t i = 0; i < src.size (); i++)
    {
      if (!utils::math::floats_equal (src[i], 0.f))
        {
          z_debug ("[{}] {:f} != 0", i, static_cast<double> (src[i]));
          return false;
        }
    }
  return true;
}

bool
audio_file_is_silent (const fs::path &filepath)
{
  AudioFile   audio_file{ filepath };
  AudioBuffer buf;
  audio_file.read_full (buf, std::nullopt);
  buf.interleave_samples ();
  return frames_silent (
    { buf.getReadPointer (0), static_cast<size_t> (buf.getNumSamples ()) });
}

float
detect_bpm (
  const float *       src,
  size_t              num_frames,
  unsigned int        samplerate,
  std::vector<float> &candidates)
{
  // unimplemented
  z_return_val_if_reached (140.f);
#if 0
  ZVampPlugin * plugin =
    vamp_get_plugin (Z_VAMP_PLUGIN_FIXED_TEMPO_ESTIMATOR, static_cast<float>(samplerate));
  size_t step_sz = vamp_plugin_get_preferred_step_size (plugin);
  size_t block_sz = vamp_plugin_get_preferred_block_size (plugin);

  /* only works with 1 channel */
  vamp_plugin_initialize (plugin, 1, step_sz, block_sz);

  ZVampOutputList * outputs = vamp_plugin_get_output_descriptors (plugin);
  vamp_plugin_output_list_print (outputs);
  vamp_plugin_output_list_free (outputs);

  long  cur_timestamp = 0;
  float bpm = 0.f;
  while ((cur_timestamp + static_cast<long>(block_sz)) < static_cast<long>(num_frames))
    {
      const float * frames[] = {
        &src[cur_timestamp],
      };
      ZVampFeatureSet * feature_set = vamp_plugin_process (
        plugin, (const float * const *) frames, cur_timestamp, samplerate);
      const ZVampFeatureList * fl =
        vamp_feature_set_get_list_for_output (feature_set, 0);
      if (fl)
        {
          vamp_feature_list_print (fl);
          const ZVampFeature * feature =
            (ZVampFeature *) g_ptr_array_index (fl->list, 0);
          bpm = feature->values[0];

          for (size_t i = 0; i < feature->num_values; i++)
            {
              candidates.push_back (feature->values[i]);
            }
        }
      cur_timestamp += static_cast<long>(step_sz);
      vamp_feature_set_free (feature_set);
    }

  z_info ("getting remaining features");
  ZVampFeatureSet * feature_set =
    vamp_plugin_get_remaining_features (plugin, samplerate);
  const ZVampFeatureList * fl =
    vamp_feature_set_get_list_for_output (feature_set, 0);
  if (fl)
    {
      vamp_feature_list_print (fl);
      const ZVampFeature * feature =
        (ZVampFeature *) g_ptr_array_index (fl->list, 0);
      bpm = feature->values[0];

      for (size_t i = 0; i < feature->num_values; i++)
        {
          candidates.push_back (feature->values[i]);
        }
    }
  vamp_feature_set_free (feature_set);

  return bpm;
#endif
}

/**
 * Returns the number of CPU cores.
 */
int
get_num_cores ()
{
  static int num_cores = 0;

  if (num_cores > 0)
    return num_cores;

  num_cores = static_cast<int> (std::thread::hardware_concurrency ());

  z_info ("Number of CPU cores found: {}", num_cores);

  return num_cores;
}

std::unique_ptr<AudioBuffer>
AudioBuffer::
  from_interleaved (const float * src, size_t num_frames, size_t num_channels)
{
  if (num_channels == 0 || num_frames == 0)
    {
      throw std::invalid_argument ("num_channels and num_frames must be > 0");
    }
  std::unique_ptr<AudioBuffer> buf =
    std::make_unique<AudioBuffer> (1, num_frames * num_channels);
  buf->copyFrom (0, 0, src, static_cast<int> (num_frames * num_channels));
  buf->deinterleave_samples (num_channels);
  return buf;
}

void
AudioBuffer::interleave_samples ()
{
  // Create a temporary buffer to hold the interleaved data
  zrythm::utils::audio::AudioBuffer tempBuffer (
    1, getNumChannels () * getNumSamples ());

  // Interleave the channels
  int writeIndex = 0;
  for (const int sample : std::views::iota (0, getNumSamples ()))
    {
      for (const int channel : std::views::iota (0, getNumChannels ()))
        {
          tempBuffer.setSample (0, writeIndex++, getSample (channel, sample));
        }
    }

  // Copy the interleaved data back to the original buffer
  *this = std::move (tempBuffer);
}

void
AudioBuffer::deinterleave_samples (size_t num_channels)
{
  const auto total_samples = getNumSamples () / (int) num_channels;

  // Create a temporary buffer to hold the deinterleaved data
  zrythm::utils::audio::AudioBuffer tempBuffer (
    (int) num_channels, total_samples);

  // Deinterleave the channels
  int read_index = 0;
  for (const auto sample : std::views::iota (0, total_samples))
    {
      for (const auto channel : std::views::iota (0_zu, num_channels))
        {
          tempBuffer.setSample (
            static_cast<int> (channel), static_cast<int> (sample),
            getSample (0, read_index++));
        }
    }

  // Copy the deinterleaved data back to the original buffer
  *this = std::move (tempBuffer);
}

void
AudioBuffer::invert_phase ()
{
  for (const auto i : std::views::iota (0, getNumChannels ()))
    {
      utils::float_ranges::mul_k2 (
        getWritePointer (i), -1.f, static_cast<size_t> (getNumSamples ()));
    }
}

void
AudioBuffer::normalize_peak ()
{
  for (const auto i : std::views::iota (0, getNumChannels ()))
    {
      auto * write_ptr = getWritePointer (i);
      utils::float_ranges::normalize (
        write_ptr, write_ptr, static_cast<size_t> (getNumSamples ()));
    }
}

} // namespace zrythm::utils::audio
