/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Audio utils.
 */

#include <math.h>

#include "audio/engine.h"
#include "ext/audio_decoder/ad.h"
#include "project.h"
#include "utils/audio.h"

#include <sndfile.h>
#include <samplerate.h>

static int num_cores = 2;

/**
 * Decodes the given filename (absolute path).
 */
void
audio_decode (
  struct adinfo * nfo,
  SRC_DATA *      src_data,
  float **        out_buff,
  long *          out_buff_size,
  const char *    filename)
{
  void * handle =
    ad_open (filename,
             nfo);
  long in_buff_size = nfo->frames * nfo->channels;

  /* add some extra buffer for some reason */
  float * in_buff =
    malloc (in_buff_size * sizeof (float));
  long samples_read =
    ad_read (handle,
             in_buff,
             in_buff_size);
  g_message ("in buff size: %ld, %ld samples read",
             in_buff_size,
             samples_read);

  /* resample with libsamplerate */
  double src_ratio =
    (1.0 * AUDIO_ENGINE->sample_rate) /
    nfo->sample_rate ;
  if (fabs (src_ratio - 1.0) < 1e-20)
    {
      g_message ("Target samplerate and input "
                 "samplerate are the same.");
    }
  if (src_is_valid_ratio (src_ratio) == 0)
    {
      g_warning ("Sample rate change out of valid "
                 "range.");
    }
  *out_buff_size =
    in_buff_size * src_ratio;
  /* add some extra buffer for some reason */
  *out_buff =
    malloc (*out_buff_size * sizeof (float));
  g_message ("out_buff_size %ld, sizeof float %ld, "
             "sizeof long %ld, src ratio %f",
             *out_buff_size,
             sizeof (float),
             sizeof (long),
             src_ratio);
  src_data->data_in = &in_buff[0];
  src_data->data_out = *out_buff;
  src_data->input_frames = nfo->frames;
  src_data->output_frames = nfo->frames * src_ratio;
  src_data->src_ratio = src_ratio;

  int err =
    src_simple (src_data,
                SRC_SINC_BEST_QUALITY,
                nfo->channels);
  g_message ("output frames gen %ld, out buff size %ld, "
             "input frames used %ld, err %d",
             src_data->output_frames_gen,
             *out_buff_size,
             src_data->input_frames_used,
             err);

  /* cleanup */
  ad_close (handle);
  free (in_buff);

  return;
}

/**
 * Writes the buffer as a raw file to the given
 * path.
 */
void
audio_write_raw_file (
  float * buff,
  long    size,
  int     samplerate,
  int     channels,
  const char * filename)
{
  SF_INFO info;

  memset (&info, 0, sizeof (info));
  info.frames = size;
  info.channels = channels;
  info.samplerate = samplerate;
  info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
  info.seekable = 1;
  info.sections = 1;

  SNDFILE * sndfile =
    sf_open (filename, SFM_WRITE, &info);

  sf_write_float (sndfile, buff, size);

  sf_close (sndfile);

  g_message ("wrote %s", filename);
}

/**
 * Returns the number of CPU cores.
 */
int
audio_get_num_cores ()
{
  if (num_cores > 0)
    return num_cores;

#ifdef _WIN32
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  num_cores = sysinfo.dwNumberOfProcessors;
#endif

#if defined(__linux__) || defined(__APPLE__)
  num_cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif

#ifdef __FreeBSD__
  int mib[4];
  std::size_t len = sizeof(num_cores);

  /* set the mib for hw.ncpu */
  mib[0] = CTL_HW;
  mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

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

  return num_cores;
}
