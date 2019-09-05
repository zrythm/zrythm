/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Audio file exporter.
 *
 * Currently used by the export dialog to export
 * the mixdown.
 */

#include <stdio.h>

#include "config.h"

#include "audio/channel.h"
#include "audio/engine.h"
#ifdef HAVE_JACK
#include "audio/engine_jack.h"
#endif
#include "audio/exporter.h"
#include "audio/marker_track.h"
#include "audio/master_track.h"
#include "audio/mixer.h"
#include "audio/position.h"
#include "audio/routing.h"
#include "audio/transport.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/io.h"
#include "utils/ui.h"

#include <sndfile.h>

#define	AMPLITUDE	(1.0 * 0x7F000000)

/**
 * Returns the audio format as string.
 *
 * Must be g_free()'d by caller.
 */
char *
exporter_stringize_audio_format (
  AudioFormat format)
{
  switch (format)
    {
    case AUDIO_FORMAT_FLAC:
      return g_strdup ("FLAC");
      break;
    case AUDIO_FORMAT_OGG:
      return g_strdup ("ogg");
      break;
    case AUDIO_FORMAT_WAV:
      return g_strdup ("wav");
      break;
    case AUDIO_FORMAT_MP3:
      return g_strdup ("mp3");
      break;
    case NUM_AUDIO_FORMATS:
      break;
    }

  g_return_val_if_reached (NULL);
}

/**
 * Exports an audio file based on the given
 * settings.
 *
 * TODO move some things into functions.
 */
void
exporter_export (ExportSettings * info)
{
  SF_INFO sfinfo;
  memset (&sfinfo, 0, sizeof (sfinfo));

  if (info->format == AUDIO_FORMAT_FLAC)
    {
      sfinfo.format = SF_FORMAT_FLAC;
    }
  else if (info->format == AUDIO_FORMAT_WAV)
    {
      sfinfo.format = SF_FORMAT_WAV;
    }
  else if (info->format == AUDIO_FORMAT_OGG)
    {
      sfinfo.format = SF_FORMAT_OGG;
    }
  else
    {
      char * format =
        exporter_stringize_audio_format (
          info->format);
      char * str =
        g_strdup_printf (
          "Format %s not supported yet",
          format);
      ui_show_error_message (
        MAIN_WINDOW,
        str);
      g_free (format);
      g_free (str);

      return;
    }

  if (info->format == AUDIO_FORMAT_OGG)
    {
      sfinfo.format =
        sfinfo.format | SF_FORMAT_VORBIS;
    }
  else if (info->depth == BIT_DEPTH_16)
    {
      sfinfo.format =
        sfinfo.format | SF_FORMAT_PCM_16;
    }
  else if (info->depth == BIT_DEPTH_24)
    {
      sfinfo.format =
        sfinfo.format | SF_FORMAT_PCM_24;
    }
  else if (info->depth == BIT_DEPTH_32)
    {
      sfinfo.format =
        sfinfo.format | SF_FORMAT_PCM_32;
    }

  if (info->time_range ==
        TIME_RANGE_SONG)
    {
      Marker * start =
        marker_track_get_start_marker (
          P_MARKER_TRACK);
      Marker * end =
        marker_track_get_end_marker (
          P_MARKER_TRACK);
      sfinfo.frames =
        position_to_frames (
          &end->pos) -
        position_to_frames (
          &start->pos);
    }
  else if (info->time_range ==
             TIME_RANGE_LOOP)
    {
      sfinfo.frames =
        position_to_frames (
          &TRANSPORT->loop_end_pos) -
          position_to_frames (
            &TRANSPORT->loop_start_pos);
    }

  sfinfo.samplerate = AUDIO_ENGINE->sample_rate;
  sfinfo.channels = 2;

  if (!sf_format_check (&sfinfo))
    {
      ui_show_error_message (
        MAIN_WINDOW,
        "SF INFO invalid");
      return;
    }

  char * dir = io_get_dir (info->file_uri);
  io_mkdir (dir);
  g_free (dir);
  SNDFILE * sndfile =
    sf_open (info->file_uri, SFM_WRITE, &sfinfo);

  if (sndfile)
    {
      sf_set_string (
        sndfile,
        SF_STR_TITLE,
        PROJECT->title);
      sf_set_string (
        sndfile,
        SF_STR_SOFTWARE,
        "Zrythm");
      sf_set_string (
        sndfile,
        SF_STR_ARTIST,
        info->artist);
      sf_set_string (
        sndfile,
        SF_STR_GENRE,
        info->genre);

      Position prev_playhead_pos;
      /* position to start at */
      Position start_pos;
      /* position to stop at */
      Position stop_pos; // position to stop at
      position_set_to_pos (
        &prev_playhead_pos,
        &TRANSPORT->playhead_pos);
      if (info->time_range ==
            TIME_RANGE_SONG)
        {
          Marker * start =
            marker_track_get_start_marker (
              P_MARKER_TRACK);
          Marker * end =
            marker_track_get_end_marker (
              P_MARKER_TRACK);
          position_set_to_pos (
            &TRANSPORT->playhead_pos,
            &start->pos);
          position_set_to_pos (
            &start_pos,
            &start->pos);
          position_set_to_pos (
            &stop_pos,
            &end->pos);
        }
      else if (info->time_range ==
                 TIME_RANGE_LOOP)
        {
          position_set_to_pos (
            &TRANSPORT->playhead_pos,
            &TRANSPORT->loop_start_pos);
          position_set_to_pos (
            &start_pos,
            &TRANSPORT->loop_start_pos);
          position_set_to_pos (
            &stop_pos,
            &TRANSPORT->loop_end_pos);
        }
      Play_State prev_play_state =
        TRANSPORT->play_state;
      TRANSPORT->play_state =
        PLAYSTATE_ROLLING;

      zix_sem_wait (
        &AUDIO_ENGINE->port_operation_lock);

      int count = 0;
      int nframes = AUDIO_ENGINE->nframes;
      int out_ptr[nframes * 2];
      do
        {
          /* run process code */
          engine_process_prepare (
            AUDIO_ENGINE,
            AUDIO_ENGINE->nframes);
          router_start_cycle (
            &MIXER->router, AUDIO_ENGINE->nframes,
            0, PLAYHEAD);
          engine_post_process (AUDIO_ENGINE);

          /* by this time, the Master channel should
           * have its Stereo Out ports filled.
           * pass its buffers to the output */
          count= 0;
          for (int i = 0; i < nframes; i++)
            {
              out_ptr[count++] = AMPLITUDE *
                P_MASTER_TRACK->channel->
                  stereo_out->l->buf[i];
              out_ptr[count++] = AMPLITUDE *
                P_MASTER_TRACK->channel->
                  stereo_out->r->buf[i];
            }

          sf_write_int (sndfile, out_ptr, count);

          info->progress =
            (float)
            (TRANSPORT->playhead_pos.frames -
              start_pos.frames) /
            (float) (
            stop_pos.frames - start_pos.frames);

        } while (
          TRANSPORT->playhead_pos.frames <
          stop_pos.frames);

      info->progress = 1.f;

      zix_sem_post (
        &AUDIO_ENGINE->port_operation_lock);

      TRANSPORT->play_state = prev_play_state;
      position_set_to_pos (
        &TRANSPORT->playhead_pos,
        &prev_playhead_pos);


      sf_close (sndfile);
    }
  else
    {
      int error = sf_error (NULL);
      char * error_str = NULL;
      switch (error)
        {
        case 1:
          error_str =
            g_strdup ("Unrecognized format");
          break;
        case 2:
          error_str =
            g_strdup ("System error");
          break;
        case 3:
          error_str =
            g_strdup ("Malformed file");
          break;
        case 4:
          error_str =
            g_strdup ("Unsupported encoding");
          break;
        default:
          g_warn_if_reached ();
          return;
        }

      g_warning ("Couldn't open SNDFILE %s:\n%s",
                 info->file_uri,
                 error_str);
      g_free (error_str);

      return;
    }
}

