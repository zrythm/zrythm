/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <unistd.h>

#include "audio/engine.h"
#include "project.h"
#include "utils/math.h"
#include "utils/audio.h"

#include <audec/audec.h>
#include <sndfile.h>
#include <samplerate.h>

#if defined(__FreeBSD__)
#include <sys/sysctl.h>
#endif

static int num_cores = 0;

void
audio_audec_log_func (
  AudecLogLevel level,
  const char *  fmt,
  va_list       args)
{
  GLogLevelFlags g_level = G_LOG_LEVEL_MESSAGE;
  switch (level)
    {
    case AUDEC_LOG_LEVEL_ERROR:
      g_level = G_LOG_LEVEL_CRITICAL;
      break;
    default:
      break;
    }

  /*char format[9000];*/
  /*strcpy (format, fmt);*/
  /*format[strlen (format) - 1] = '\0';*/

  if (ZRYTHM_TESTING)
    {
      g_message (fmt, args);
    }
  else
    {
      g_logv ("audec", g_level, fmt, args);
    }
}

/**
 * Writes the buffer as a raw file to the given
 * path.
 *
 * @param size The number of frames per channel.
 * @param samplerate The samplerate of \ref buff.
 * @param frames_already_written Frames (per
 *   channel)already written. If this is non-zero
 *   and the file exists, it will append to the
 *   existing file.
 *
 * @return Non-zero if fail.
 */
int
audio_write_raw_file (
  float *      buff,
  long         frames_already_written,
  long         nframes,
  uint32_t     samplerate,
  unsigned int channels,
  const char * filename)
{
  g_return_val_if_fail (
    samplerate > 0 &&
    channels > 0 &&
    samplerate < 10000000, -1);

  g_debug (
    "writing raw file: already written %ld, "
    "nframes %ld, samplerate %u, channels %u, "
    "filename %s",
    frames_already_written, nframes, samplerate,
    channels, filename);

  SF_INFO info;

  memset (&info, 0, sizeof (info));
  info.frames = nframes;
  info.channels = (int) channels;
  info.samplerate = (int) samplerate;
  info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
  info.seekable = 1;
  info.sections = 1;

  bool write_chunk =
    (frames_already_written > 0) &&
    g_file_test (filename, G_FILE_TEST_IS_REGULAR);

  SNDFILE * sndfile =
    sf_open (filename, SFM_RDWR, &info);

  long seek_to =
    write_chunk ? frames_already_written : 0;
  g_debug ("seeking to %ld", seek_to);
  int ret =
    sf_seek (
      sndfile, seek_to, SEEK_SET | SFM_WRITE);
  if (ret == -1 || ret != seek_to)
    {
      g_warning ("seek error %d", ret);
    }

  sf_count_t count =
    sf_writef_float (sndfile, buff, nframes);
  g_warn_if_fail (count == nframes);

  sf_write_sync (sndfile);

  sf_close (sndfile);

  g_message (
    "wrote %zu frames to '%s'", count, filename);

  return 0;
}

/**
 * Returns whether the frame buffers are equal.
 */
bool
audio_frames_equal (
  float * src1,
  float * src2,
  size_t  num_frames)
{
  for (size_t i = 0; i < num_frames; i++)
    {
      if (!math_floats_equal (src1[i], src2[i]))
        {
          g_debug (
            "[%zu] %f != %f",
            i, (double) src1[i], (double) src2[i]);
          return false;
        }
    }
  return true;
}

/**
 * Returns whether the frame buffer is empty (zero).
 */
bool
audio_frames_empty (
  float * src,
  size_t  num_frames)
{
  for (size_t i = 0; i < num_frames; i++)
    {
      if (!math_floats_equal (src[i], 0.f))
        {
          g_debug (
            "[%zu] %f != 0", i, (double) src[i]);
          return false;
        }
    }
  return true;
}

bool
audio_file_is_silent (
  const char * filepath)
{
  SF_INFO sfinfo;
  memset (&sfinfo, 0, sizeof (sfinfo));
  sfinfo.format =
    sfinfo.format | SF_FORMAT_PCM_16;
  SNDFILE * sndfile =
    sf_open (filepath, SFM_READ, &sfinfo);
  g_return_val_if_fail (
    sndfile && sfinfo.frames > 0, true);

  long buf_size = sfinfo.frames * sfinfo.channels;
  float * data =
    calloc ((size_t) buf_size, sizeof (float));
  sf_count_t frames_read =
    sf_readf_float (sndfile, data, sfinfo.frames);
  g_assert_cmpint (frames_read, ==, sfinfo.frames);
  g_return_val_if_fail (
    frames_read == sfinfo.frames, true);
  g_debug (
    "read %ld frames for %s", frames_read,
    filepath);

  bool is_empty =
    audio_frames_empty (data, (size_t) buf_size);
  free (data);

  int ret = sf_close (sndfile);
  g_return_val_if_fail (ret == 0, true);

  return is_empty;
}

/**
 * Returns the number of CPU cores.
 */
int
audio_get_num_cores ()
{
  if (num_cores > 0)
    return num_cores;

#ifdef _WOE32
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  num_cores = (int) sysinfo.dwNumberOfProcessors;
#endif

#if defined(__linux__) || defined(__APPLE__)
  num_cores =
    (int) sysconf (_SC_NPROCESSORS_ONLN);
#endif

#ifdef __FreeBSD__
  int mib[4];
  size_t len = sizeof(num_cores);

  /* set the mib for hw.ncpu */
  mib[0] = CTL_HW;
  mib[1] = HW_NCPU;

  /* get the number of CPUs from the system */
  sysctl(mib, 2, &num_cores, &len, NULL, 0);

  if (num_cores < 1)
  {
      mib[1] = HW_NCPU;
      sysctl(mib, 2, &num_cores, &len, NULL, 0);
      if (num_cores < 1)
          num_cores = 1;
  }
#endif

  g_message (
      "Number of CPU cores found: %d",
      num_cores);

  return num_cores;
}
