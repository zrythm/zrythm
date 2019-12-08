/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include "utils/audio.h"

#include <sndfile.h>
#include <samplerate.h>

#if defined(__FreeBSD__)
#include <sys/sysctl.h>
#endif

static int num_cores = 0;

/**
 * Writes the buffer as a WAV file to the given
 * path.
 *
 * @param size The number of frames per channel.
 */
void
audio_write_raw_file (
  float *      buff,
  long         nframes,
  uint32_t     samplerate,
  unsigned int channels,
  const char * filename)
{
  SF_INFO info;

  memset (&info, 0, sizeof (info));
  info.frames = nframes;
  info.channels = (int) channels;
  info.samplerate = (int) samplerate;
  info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
  info.seekable = 1;
  info.sections = 1;

  SNDFILE * sndfile =
    sf_open (filename, SFM_WRITE, &info);

  /*sf_count_t frames_written =*/
    sf_writef_float (sndfile, buff, nframes);

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

  return num_cores;
}
