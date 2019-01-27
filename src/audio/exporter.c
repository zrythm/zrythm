/*
 * audio/exporter.c - Audio file exporter
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

#include <stdio.h>

#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/exporter.h"
#include "audio/mixer.h"
#include "audio/position.h"
#include "audio/transport.h"
#include "project.h"
#include "utils/io.h"

#include <sndfile.h>

#define	AMPLITUDE	(1.0 * 0x7F000000)

void
exporter_export (ExportInfo * info)
{
  if (info->format == AUDIO_FORMAT_FLAC)
    {
      SF_INFO sfinfo;
      memset (&sfinfo, 0, sizeof (sfinfo));
      sfinfo.frames	= position_to_frames (&TRANSPORT->end_marker_pos) -
        position_to_frames (&TRANSPORT->start_marker_pos);
      sfinfo.samplerate = AUDIO_ENGINE->sample_rate;
      sfinfo.channels = 2;
      if (info->depth == BIT_DEPTH_16)
        {
          sfinfo.format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16;
        }
      else if (info->depth == BIT_DEPTH_24)
        {
          sfinfo.format = SF_FORMAT_FLAC | SF_FORMAT_PCM_24;
        }
      else if (info->depth == BIT_DEPTH_32)
        {
          sfinfo.format = SF_FORMAT_FLAC | SF_FORMAT_PCM_32;
        }
      if (!sf_format_check (&sfinfo))
        {
          g_warning ("SF_INFO invalid");
          return;
        }
      char * dir = io_get_dir (info->file_uri);
      io_mkdir (dir);
      g_free (dir);
      SNDFILE * sndfile = sf_open (info->file_uri, SFM_WRITE, &sfinfo);

      if (sndfile)
        {
          sf_set_string (sndfile, SF_STR_TITLE, PROJECT->title);
          sf_set_string (sndfile, SF_STR_SOFTWARE, "Zrythm");
          sf_set_string (sndfile, SF_STR_ARTIST, "One Night Alive");
          sf_set_string (sndfile, SF_STR_GENRE, "Electronic");

          Position prev_playhead_pos;
          position_set_to_pos (&prev_playhead_pos, &TRANSPORT->playhead_pos);
          position_set_to_pos (&TRANSPORT->playhead_pos,
                               &TRANSPORT->start_marker_pos);
          Play_State prev_play_state = TRANSPORT->play_state;
          TRANSPORT->play_state = PLAYSTATE_ROLLING;

          zix_sem_wait (&AUDIO_ENGINE->port_operation_lock);

          do
            {

              /* set all to unprocessed for this cycle */
              for (int i = 0; i < MIXER->num_channels; i++)
                {
                  Channel * channel = MIXER->channels[i];
                  channel->processed = 0;
                  for (int j = 0; j < STRIP_SIZE; j++)
                    {
                      if (channel->plugins[j])
                        {
                          channel->plugins[j]->processed = 0;
                        }
                    }
                }

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
              channel_process (MIXER->master);


              /* by this time, the Master channel should have its Stereo Out ports filled.
               * pass their buffers to jack's buffers */
              int count= 0;
              int out_ptr[AUDIO_ENGINE->nframes * 2];
              for (int i = 0; i < AUDIO_ENGINE->nframes; i++)
                {
                  out_ptr[count++] = AMPLITUDE *
                    MIXER->master->stereo_out->l->buf[i];
                  out_ptr[count++] = AMPLITUDE *
                    MIXER->master->stereo_out->r->buf[i];
                  /*if (out_ptr [count - 1] > 0)*/
                    /*g_message ("val l%d r%d", out_ptr [count - 2],*/
                               /*out_ptr [count - 1]);*/
                }

              sf_write_int (sndfile, out_ptr, count);

              /* move playhead as many samples as processed */
              transport_add_to_playhead (AUDIO_ENGINE->nframes);
            } while (position_compare (&TRANSPORT->playhead_pos,
                                       &TRANSPORT->end_marker_pos) <= 0);

          zix_sem_post (&AUDIO_ENGINE->port_operation_lock);

          TRANSPORT->play_state = prev_play_state;
          position_set_to_pos (&TRANSPORT->playhead_pos,
                               &prev_playhead_pos);


          sf_close (sndfile);
        }
      else
        {
          g_warning ("Couldn't open SNDFILE %s", info->file_uri);
          return;
        }

    }

}

