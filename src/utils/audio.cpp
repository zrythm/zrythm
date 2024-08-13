// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>

#include "dsp/clip.h"
#include "utils/audio.h"
#include "utils/exceptions.h"
#include "utils/logger.h"
#include "utils/math.h"
#include "utils/vamp.h"

#include <glib/gi18n.h>

#include <giomm.h>
#include <sndfile.h>
#include <unistd.h>

#if defined(__FreeBSD__)
#  include <sys/sysctl.h>
#endif

void
audio_write_raw_file (
  const float *      buff,
  size_t             frames_already_written,
  size_t             nframes,
  uint32_t           samplerate,
  bool               flac,
  BitDepth           bit_depth,
  channels_t         channels,
  const std::string &filename)
{
  z_return_if_fail (samplerate < 10000000);

  z_debug (
    "writing raw file: already written %zu, "
    "nframes %zu, samplerate {}, channels {}, "
    "filename %s, flac? %d",
    frames_already_written, nframes, samplerate, channels, filename, flac);

  SF_INFO info;

  memset (&info, 0, sizeof (info));
  info.channels = (int) channels;
  info.samplerate = (int) samplerate;
  int type_major = flac ? SF_FORMAT_FLAC : SF_FORMAT_WAV;
  int type_minor = 0;
  switch (bit_depth)
    {
    case BitDepth::BIT_DEPTH_16:
      type_minor = SF_FORMAT_PCM_16;
      break;
    case BitDepth::BIT_DEPTH_24:
      type_minor = SF_FORMAT_PCM_24;
      break;
    case BitDepth::BIT_DEPTH_32:
      z_return_if_fail (!flac);
      type_minor = SF_FORMAT_PCM_32;
      break;
    }
  info.format = type_major | type_minor;

  if (!flac)
    {
      info.seekable = 1;
      info.sections = 1;
    }

  bool write_chunk =
    (frames_already_written > 0)
    && Glib::file_test (filename, Glib::FileTest::IS_REGULAR);

  if (flac && write_chunk)
    {
      z_error ("cannot write chunks for flac");
      return;
    }

  if (!sf_format_check (&info))
    {
      z_error ("Invalid SFINFO: {}", sf_strerror (nullptr));
      return;
    }

  SNDFILE * sndfile =
    sf_open (filename.c_str (), flac ? SFM_WRITE : SFM_RDWR, &info);
  if (!sndfile)
    {
      throw ZrythmException (
        fmt::format ("Error opening sndfile: {}", sf_strerror (nullptr)));
    }

  if (info.format != (type_major | type_minor))
    {
      z_error (
        "Invalid SNDFILE format: 0x%08X != 0x%08X", info.format,
        type_major | type_minor);
      return;
    }

  sf_set_string (sndfile, SF_STR_SOFTWARE, PROGRAM_NAME);

  if (!flac)
    {
      size_t seek_to = write_chunk ? frames_already_written : 0;
      z_debug ("seeking to {}", seek_to);
      int ret = sf_seek (sndfile, (sf_count_t) seek_to, SEEK_SET | SFM_WRITE);
      if (ret == -1 || ret != (int) seek_to)
        {
          throw ZrythmException (
            fmt::format ("Seek error {}: {}", ret, sf_strerror (nullptr)));
        }
    }

  sf_count_t _nframes = (sf_count_t) nframes;
  z_debug ("nframes = {}", _nframes);
  sf_count_t count = sf_writef_float (sndfile, buff, (sf_count_t) _nframes);
  if (count != (sf_count_t) nframes)
    {
      throw ZrythmException (fmt::format (
        "Mismatch: expected {} frames, got {}: {}", _nframes, count,
        sf_strerror (sndfile)));
    }

  sf_write_sync (sndfile);

  if (sf_close (sndfile) != 0)
    {
      throw ZrythmException (
        fmt::format ("Failed to close sndfile: {}", sf_strerror (nullptr)));
    }

  z_debug ("wrote {} frames to '{}'", count, filename);
}

/**
 * Returns the number of frames in the given audio
 * file.
 */
unsigned_frame_t
audio_get_num_frames (const char * filepath)
{
  SF_INFO sfinfo;
  memset (&sfinfo, 0, sizeof (sfinfo));
  sfinfo.format = sfinfo.format | SF_FORMAT_PCM_16;
  SNDFILE * sndfile = sf_open (filepath, SFM_READ, &sfinfo);
  if (!sndfile)
    {
      const char * err_str = sf_strerror (sndfile);
      z_error ("sndfile null: {}", err_str);
      return 0;
    }
  z_return_val_if_fail (sfinfo.frames > 0, 0);
  unsigned_frame_t frames = (unsigned_frame_t) sfinfo.frames;

  int ret = sf_close (sndfile);
  z_return_val_if_fail (ret == 0, 0);

  return frames;
}

/**
 * Returns whether the frame buffers are equal.
 */
bool
audio_frames_equal (
  const float * src1,
  const float * src2,
  size_t        num_frames,
  float         epsilon)
{
  for (size_t i = 0; i < num_frames; i++)
    {
      if (!math_floats_equal_epsilon (src1[i], src2[i], epsilon))
        {
          z_debug ("[{}] {:f} != {:f}", i, (double) src1[i], (double) src2[i]);
          return false;
        }
    }
  return true;
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
  try
    {
      AudioClip c1{ f1 };
      AudioClip c2{ f2 };

      if (num_frames == 0)
        {
          if (c1.num_frames_ == c2.num_frames_)
            {
              num_frames = c1.num_frames_;
            }
          else
            {
              return false;
            }
        }
      z_return_val_if_fail (num_frames > 0, false);

      if (c1.channels_ != c2.channels_)
        return false;

      for (size_t i = 0; i < c1.channels_; i++)
        {
          if (
            !audio_frames_equal (
              c1.ch_frames_.getReadPointer (i),
              c2.ch_frames_.getReadPointer (i), num_frames, epsilon))
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
audio_frames_empty (float * src, size_t num_frames)
{
  for (size_t i = 0; i < num_frames; i++)
    {
      if (!math_floats_equal (src[i], 0.f))
        {
          z_debug ("[{}] {:f} != 0", i, (double) src[i]);
          return false;
        }
    }
  return true;
}

bool
audio_file_is_silent (const char * filepath)
{
  SF_INFO sfinfo;
  memset (&sfinfo, 0, sizeof (sfinfo));
  sfinfo.format = sfinfo.format | SF_FORMAT_PCM_16;
  SNDFILE * sndfile = sf_open (filepath, SFM_READ, &sfinfo);
  z_return_val_if_fail (sndfile && sfinfo.frames > 0, true);

  long    buf_size = sfinfo.frames * sfinfo.channels;
  float * data =
    static_cast<float *> (calloc ((size_t) buf_size, sizeof (float)));
  sf_count_t frames_read = sf_readf_float (sndfile, data, sfinfo.frames);
  g_assert_cmpint (frames_read, ==, sfinfo.frames);
  z_return_val_if_fail (frames_read == sfinfo.frames, true);
  z_debug ("read {} frames for {}", frames_read, filepath);

  bool is_empty = audio_frames_empty (data, (size_t) buf_size);
  free (data);

  int ret = sf_close (sndfile);
  z_return_val_if_fail (ret == 0, true);

  return is_empty;
}

float
audio_detect_bpm (
  const float *       src,
  size_t              num_frames,
  unsigned int        samplerate,
  std::vector<float> &candidates)
{
  ZVampPlugin * plugin =
    vamp_get_plugin (Z_VAMP_PLUGIN_FIXED_TEMPO_ESTIMATOR, (float) samplerate);
  size_t step_sz = vamp_plugin_get_preferred_step_size (plugin);
  size_t block_sz = vamp_plugin_get_preferred_block_size (plugin);

  /* only works with 1 channel */
  vamp_plugin_initialize (plugin, 1, step_sz, block_sz);

  ZVampOutputList * outputs = vamp_plugin_get_output_descriptors (plugin);
  vamp_plugin_output_list_print (outputs);
  vamp_plugin_output_list_free (outputs);

  long  cur_timestamp = 0;
  float bpm = 0.f;
  while ((cur_timestamp + (long) block_sz) < (long) num_frames)
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
      cur_timestamp += (long) step_sz;
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
}

/**
 * Returns the number of CPU cores.
 */
int
audio_get_num_cores ()
{
  static int num_cores = 0;

  if (num_cores > 0)
    return num_cores;

  num_cores = std::thread::hardware_concurrency ();

  z_info ("Number of CPU cores found: {}", num_cores);

  return num_cores;
}
