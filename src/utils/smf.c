/*
 * utils/smf.c - SMF file writer/reader
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


#include "project.h"
#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/midi.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "utils/io.h"
#include "utils/smf.h"

#include <gtk/gtk.h>

#include <smf.h>

/**
 * Saves regions into MIDI files (.smf)
 */
void
smf_save_regions (char * filename)
{
  char * dir = io_get_dir (filename);
  io_mkdir (dir);
  char * separator = io_get_separator ();

  for (int i = 0; i < PROJECT->num_regions; i++)
    {
      Region * region = PROJECT->regions[i];

      if (!region->linked_region)
        {
          smf_t *smf;
          smf_track_t *track;
          smf_event_t *event;

          smf = smf_new();
          if (smf == NULL)
            {
              g_warning ("smf_new failed");
              return;
            }

          track = smf_track_new();
          if (track == NULL)
            {
              g_warning ("smf_track_new failed");
              return;
            }

          /* add tempo event */
          smf_add_track(smf, track);
          jack_midi_event_t ev;
          /* see
           * http://www.mixagesoftware.com/en/midikit/help/HTML/meta_events.html
           */
          ev.size = 6;
          ev.buffer = calloc (6, sizeof (jack_midi_data_t));
          ev.buffer[0] = 0xFF;
          ev.buffer[1]= 0x51;
          ev.buffer[2] = 0x03;
          /* convert bpm to tempo value */
          int tempo = 60000000 / TRANSPORT->bpm;
          ev.buffer[3] = (tempo >> 16) & 0xFF;
          ev.buffer[4] = (tempo >> 8) & 0xFF;
          ev.buffer[5] = tempo & 0xFF;
          event = smf_event_new_from_pointer (ev.buffer,
                                              ev.size);
          smf_track_add_event_seconds(
            track,
            event,
            0);
          free (ev.buffer);

          MidiEvents * events = calloc (1, sizeof (MidiEvents));
          midi_note_notes_to_events (region->midi_notes,
                                     region->num_midi_notes,
                                     &region->start_pos,
                                     events);

          for (int j = 0; j < events->num_events; j++)
            {
              jack_midi_event_t * ev = &events->jack_midi_events[j];
              event = smf_event_new_from_pointer (ev->buffer,
                                                  ev->size);
              if (event == NULL)
                {
                  g_warning ("smf event is NULL");
                  return;
                }

              smf_track_add_event_seconds(
                track,
                event,
                (double) ev->time / (double) AUDIO_ENGINE->sample_rate);
            }

          free (events);

          char * filename = g_strdup_printf (REGION_PRINTF_FILENAME,
                                             dir,
                                             separator,
                                             separator,
                                             region->id,
                                             region->track->channel->name,
                                             region->name);
          g_message ("filename %s", filename);


          /* save the midi file */
          int ret = smf_save(smf, filename);
          g_free (filename);
          if (ret)
            {
              g_warning ("smf_save failed");
              return;
            }

          smf_delete(smf);
        }
    }

  /* cleanup */
  g_free (dir);
  g_free (separator);
}

