// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <math.h>

#include "dsp/engine.h"
#include "project.h"
#include "utils/audio.h"
#include "utils/error.h"
#include "utils/math.h"
#include "utils/string.h"
#include "utils/vamp.h"

#include <glib/gi18n.h>

#include <sndfile.h>
#include <unistd.h>

#if defined(__FreeBSD__)
#  include <sys/sysctl.h>
#endif

typedef enum
{
  Z_UTILS_AUDIO_ERROR_FAILED,
} ZUtilsAudioError;

#define Z_UTILS_AUDIO_ERROR z_utils_audio_error_quark ()
GQuark
z_utils_audio_error_quark (void);
G_DEFINE_QUARK (
  z - utils - audio - error - quark,
  z_utils_audio_error)

static int num_cores = 0;

static const char * bit_depth_pretty_strings[] = {
  __ ("16 bit"),
  __ ("24 bit"),
  __ ("32 bit"),
};

BitDepth
audio_bit_depth_from_pretty_str (const char * str)
{
  for (BitDepth i = BIT_DEPTH_16; i <= BIT_DEPTH_32; i++)
    {
      if (string_is_equal (
            str, _ (bit_depth_pretty_strings[i])))
        return i;
    }

  g_return_val_if_reached (BIT_DEPTH_16);
}

const char *
audio_bit_depth_to_pretty_str (BitDepth depth)
{
  return _ (bit_depth_pretty_strings[depth]);
}

/**
 * Writes the buffer as a raw file to the given path.
 *
 * @param size The number of frames per channel.
 * @param samplerate The samplerate of \ref buff.
 * @param frames_already_written Frames (per channel) already
 *   written. If this is non-zero and the file exists, it will
 *   append to the existing file.
 *
 * @return Whether successful.
 */
WARN_UNUSED_RESULT bool
audio_write_raw_file (
  float *      buff,
  size_t       frames_already_written,
  size_t       nframes,
  uint32_t     samplerate,
  bool         flac,
  BitDepth     bit_depth,
  channels_t   channels,
  const char * filename,
  GError **    error)
{
  g_return_val_if_fail (samplerate < 10000000, false);

  g_debug (
    "writing raw file: already written %zu, "
    "nframes %zu, samplerate %u, channels %u, "
    "filename %s, flac? %d",
    frames_already_written, nframes, samplerate, channels,
    filename, flac);

  SF_INFO info;

  memset (&info, 0, sizeof (info));
  info.channels = (int) channels;
  info.samplerate = (int) samplerate;
  int type_major = flac ? SF_FORMAT_FLAC : SF_FORMAT_WAV;
  int type_minor = 0;
  switch (bit_depth)
    {
    case BIT_DEPTH_16:
      type_minor = SF_FORMAT_PCM_16;
      break;
    case BIT_DEPTH_24:
      type_minor = SF_FORMAT_PCM_24;
      break;
    case BIT_DEPTH_32:
      g_return_val_if_fail (!flac, false);
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
    && g_file_test (filename, G_FILE_TEST_IS_REGULAR);

  if (flac && write_chunk)
    {
      g_critical ("cannot write chunks for flac");
      return false;
    }

  if (!sf_format_check (&info))
    {
      g_critical ("Invalid SFINFO: %s", sf_strerror (NULL));
      return false;
    }

  SNDFILE * sndfile =
    sf_open (filename, flac ? SFM_WRITE : SFM_RDWR, &info);
  if (!sndfile)
    {
      g_set_error (
        error, Z_UTILS_AUDIO_ERROR, Z_UTILS_AUDIO_ERROR_FAILED,
        _ ("Error opening sndfile: %s"), sf_strerror (NULL));
      return false;
    }

  if (info.format != (type_major | type_minor))
    {
      g_critical (
        "Invalid SNDFILE format: 0x%08X != 0x%08X",
        info.format, type_major | type_minor);
      return false;
    }

  sf_set_string (sndfile, SF_STR_SOFTWARE, PROGRAM_NAME);

  if (!flac)
    {
      size_t seek_to =
        write_chunk ? frames_already_written : 0;
      g_debug ("seeking to %zu", seek_to);
      int ret = sf_seek (
        sndfile, (sf_count_t) seek_to, SEEK_SET | SFM_WRITE);
      if (ret == -1 || ret != (int) seek_to)
        {
          g_set_error (
            error, Z_UTILS_AUDIO_ERROR,
            Z_UTILS_AUDIO_ERROR_FAILED,
            _ ("Seek error %d: %s"), ret, sf_strerror (NULL));
          return false;
        }
    }

  sf_count_t _nframes = (sf_count_t) nframes;
  g_debug ("nframes = %ld", _nframes);
  sf_count_t count =
    sf_writef_float (sndfile, buff, (sf_count_t) _nframes);
  if (count != (sf_count_t) nframes)
    {
      g_set_error (
        error, Z_UTILS_AUDIO_ERROR, Z_UTILS_AUDIO_ERROR_FAILED,
        "Mismatch: expected %ld frames, got %ld: %s",
        _nframes, count, sf_strerror (sndfile));
      return false;
    }

  sf_write_sync (sndfile);

  if (sf_close (sndfile) != 0)
    {
      g_set_error (
        error, Z_UTILS_AUDIO_ERROR, Z_UTILS_AUDIO_ERROR_FAILED,
        "Failed to close sndfile: %s", sf_strerror (NULL));
      return false;
    }

  g_message ("wrote %zu frames to '%s'", count, filename);

  return true;
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
      g_critical ("sndfile null: %s", err_str);
      return 0;
    }
  g_return_val_if_fail (sfinfo.frames > 0, 0);
  unsigned_frame_t frames = (unsigned_frame_t) sfinfo.frames;

  int ret = sf_close (sndfile);
  g_return_val_if_fail (ret == 0, 0);

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
      if (!math_floats_equal_epsilon (
            src1[i], src2[i], epsilon))
        {
          g_debug (
            "[%zu] %f != %f", i, (double) src1[i],
            (double) src2[i]);
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
  GError *    err = NULL;
  AudioClip * c1 = audio_clip_new_from_file (f1, &err);
  if (!c1)
    {
      HANDLE_ERROR_LITERAL (err, "Failed to create clip 1");
      return false;
    }
  err = NULL;
  AudioClip * c2 = audio_clip_new_from_file (f2, &err);
  if (!c2)
    {
      HANDLE_ERROR_LITERAL (err, "Failed to create clip 2");
      return false;
    }

  bool ret = false;
  if (num_frames == 0)
    {
      if (c1->num_frames == c2->num_frames)
        {
          num_frames = c1->num_frames;
        }
      else
        {
          goto cleanup_clips;
        }
    }
  g_return_val_if_fail (num_frames > 0, false);

  ret = c1->channels == c2->channels;
  if (!ret)
    goto cleanup_clips;

  for (size_t i = 0; i < c1->channels; i++)
    {
      ret = audio_frames_equal (
        c1->ch_frames[i], c2->ch_frames[i], num_frames,
        epsilon);
      if (!ret)
        {
          goto cleanup_clips;
        }
    }

cleanup_clips:
  audio_clip_free (c1);
  audio_clip_free (c2);

  return ret;
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
          g_debug ("[%zu] %f != 0", i, (double) src[i]);
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
  g_return_val_if_fail (sndfile && sfinfo.frames > 0, true);

  long    buf_size = sfinfo.frames * sfinfo.channels;
  float * data = calloc ((size_t) buf_size, sizeof (float));
  sf_count_t frames_read =
    sf_readf_float (sndfile, data, sfinfo.frames);
  g_assert_cmpint (frames_read, ==, sfinfo.frames);
  g_return_val_if_fail (frames_read == sfinfo.frames, true);
  g_debug ("read %ld frames for %s", frames_read, filepath);

  bool is_empty = audio_frames_empty (data, (size_t) buf_size);
  free (data);

  int ret = sf_close (sndfile);
  g_return_val_if_fail (ret == 0, true);

  return is_empty;
}

/**
 * Detect BPM.
 *
 * @return The BPM, or 0 if not found.
 */
float
audio_detect_bpm (
  float *      src,
  size_t       num_frames,
  unsigned int samplerate,
  GArray *     candidates)
{
  ZVampPlugin * plugin = vamp_get_plugin (
    Z_VAMP_PLUGIN_FIXED_TEMPO_ESTIMATOR, (float) samplerate);
  size_t step_sz =
    vamp_plugin_get_preferred_step_size (plugin);
  size_t block_sz =
    vamp_plugin_get_preferred_block_size (plugin);

  /* only works with 1 channel */
  vamp_plugin_initialize (plugin, 1, step_sz, block_sz);

  ZVampOutputList * outputs =
    vamp_plugin_get_output_descriptors (plugin);
  vamp_plugin_output_list_print (outputs);
  vamp_plugin_output_list_free (outputs);

  long  cur_timestamp = 0;
  float bpm = 0.f;
  while ((cur_timestamp + (long) block_sz) < (long) num_frames)
    {
      float * frames[] = {
        &src[cur_timestamp],
      };
      ZVampFeatureSet * feature_set = vamp_plugin_process (
        plugin, (const float * const *) frames, cur_timestamp,
        samplerate);
      const ZVampFeatureList * fl =
        vamp_feature_set_get_list_for_output (feature_set, 0);
      if (fl)
        {
          vamp_feature_list_print (fl);
          const ZVampFeature * feature =
            g_ptr_array_index (fl->list, 0);
          bpm = feature->values[0];

          if (candidates)
            {
              for (size_t i = 0; i < feature->num_values; i++)
                {
                  g_array_append_val (
                    candidates, feature->values[i]);
                }
            }
        }
      cur_timestamp += (long) step_sz;
      vamp_feature_set_free (feature_set);
    }

  g_message ("getting remaining features");
  ZVampFeatureSet * feature_set =
    vamp_plugin_get_remaining_features (plugin, samplerate);
  const ZVampFeatureList * fl =
    vamp_feature_set_get_list_for_output (feature_set, 0);
  if (fl)
    {
      vamp_feature_list_print (fl);
      const ZVampFeature * feature =
        g_ptr_array_index (fl->list, 0);
      bpm = feature->values[0];

      if (candidates)
        {
          for (size_t i = 0; i < feature->num_values; i++)
            {
              g_array_append_val (
                candidates, feature->values[i]);
            }
        }
    }
  vamp_feature_set_free (feature_set);

  return bpm;
}

/**
 * Returns the number of CPU cores.
 */
int
audio_get_num_cores (void)
{
  if (num_cores > 0)
    return num_cores;

#ifdef _WOE32
  SYSTEM_INFO sysinfo;
  GetSystemInfo (&sysinfo);
  num_cores = (int) sysinfo.dwNumberOfProcessors;
#endif

#if defined(__linux__) || defined(__APPLE__)
  num_cores = (int) sysconf (_SC_NPROCESSORS_ONLN);
#endif

#ifdef __FreeBSD__
  int    mib[4];
  size_t len = sizeof (num_cores);

  /* set the mib for hw.ncpu */
  mib[0] = CTL_HW;
  mib[1] = HW_NCPU;

  /* get the number of CPUs from the system */
  sysctl (mib, 2, &num_cores, &len, NULL, 0);

  if (num_cores < 1)
    {
      mib[1] = HW_NCPU;
      sysctl (mib, 2, &num_cores, &len, NULL, 0);
      if (num_cores < 1)
        num_cores = 1;
    }
#endif

  g_message ("Number of CPU cores found: %d", num_cores);

  return num_cores;
}
