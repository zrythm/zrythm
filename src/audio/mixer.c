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

static nframes_t      nframes;

/**
 * Thread work
 * Pass the L/R to each channel strip and let them handle it
 */
static void *
process_channel_work (void * argument)
{
  channel_process (MIXER->channels[*((int*) argument)],
                   nframes);
  return 0;
}

/**
 * process callback
 */
void
mixer_process (nframes_t     _nframes)           ///< number of frames to fill in
{
  static int i, j;

  nframes = _nframes;

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
                &threads[i], NULL, process_channel_work, &thread_args[i]);

    }
   // wait for each thread to complete
  for (i = 0; i < MIXER->num_channels; ++i)
    {
      result_code = pthread_join(threads[i], NULL);
    }

  /* process master channel */
  channel_process (MIXER->master,
                   nframes);
}

void
mixer_init ()
{
  g_message ("Initializing mixer...");
  /* allocate size */
  MIXER = calloc (1, sizeof (Mixer));

  /*MIXER->num_ports = 0;*/

  /* create master channel */
  MIXER->master = channel_create_master ();

  /* init channel strips array and add one of each */
  ADD_CHANNEL (channel_create (CT_MIDI));
  ADD_CHANNEL (channel_create (CT_AUDIO));
}
