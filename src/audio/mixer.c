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
 * process callback
 */
void
mixer_process (nframes_t     nframes)           ///< number of frames to fill in
{
  int loop = 1;

  /* wait for channels to finish processing */
  while (loop)
    {
      loop = 0;
      for (int i = 0; i < MIXER->num_channels; i++)
        {
          if (!MIXER->channels[i]->processed)
            {
              loop = 1;
              break;
            }
        }
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

  MIXER->num_channels = 0;

  /* create master channel */
  MIXER->master = channel_create_master ();

  /* init channel strips array and add one of each */
  ADD_CHANNEL (channel_create (CT_MIDI, "Ch 1"));
  /*ADD_CHANNEL (channel_create (CT_AUDIO));*/
}
