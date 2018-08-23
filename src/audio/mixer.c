/*
 * audio/mixer.c - The mixer
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

/** \file
 * Mixer implementation. */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/mixer.h"

#include <gtk/gtk.h>

static nframes_t   nframes;
static sample_t     * l_buf;
static sample_t     * r_buf;

/**
 * Thread work
 * Pass the L/R to each channel strip and let them handle it
 */
static void *
perform_work (void * argument)
{
  /* prepare empty L & R buffers for the channel strip */
  sample_t * cl_buf, * cr_buf;
  cl_buf = (sample_t *) calloc (nframes, sizeof (sample_t));
  cr_buf = (sample_t *) calloc (nframes, sizeof (sample_t));

  channel_process (MIXER->channels[*((int*) argument)],
                   nframes,
                   cl_buf,
                   cr_buf);

  /* add the channel bufs to the mixer bufs */
  for (int j = 0; j < nframes; j++)
    {
      l_buf[j] += cl_buf[j];
      r_buf[j] += cr_buf[j];
    }

  free (cl_buf);
  free (cr_buf);

  return NULL;
}

/**
 * process callback
 */
void
mixer_process (nframes_t     _nframes,           ///< number of frames to fill in
              sample_t      * _l_buf,            ///< left buffer
              sample_t      * _r_buf)            ///< right buffer
{
  static int i, j;

  nframes = _nframes;
  l_buf = _l_buf;
  r_buf = _r_buf;

  /* set buffers to 0 */
  memset (l_buf, 0, sizeof (sample_t) * nframes);
  memset (r_buf, 0, sizeof (sample_t) * nframes);

  /* prepare threads */
  pthread_t threads[MIXER->num_channels];
  int thread_args[MIXER->num_channels];
  int result_code;
  unsigned index;

  /* loop through each channel strip.
   * the output at each stage should be 2 ports,
   * and they are added to the buffers */
  for (i = 0; i < MIXER->num_channels; i++)
    {
      thread_args[ i ] = i;
      result_code = pthread_create (
                &threads[i], NULL, perform_work, &thread_args[i]);

    }
   // wait for each thread to complete
  for (i = 0; i < MIXER->num_channels; ++i)
    {
      result_code = pthread_join(threads[i], NULL);
    }
}

void
mixer_init ()
{
  g_message ("Initializing mixer...");
  /* allocate size */
  MIXER = malloc (sizeof (Mixer));

  MIXER->num_channels = 0;
  MIXER->num_ports = 0;

  /* init channel strips array and add one of each */
  ADD_CHANNEL (channel_create (CT_MIDI));
  ADD_CHANNEL (channel_create (CT_AUDIO));

  /* create master channel */
  MIXER->master = channel_create (CT_MASTER);
}
