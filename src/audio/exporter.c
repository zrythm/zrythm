/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-config.h"

#include <stdio.h>

#include "actions/tracklist_selections.h"
#include "audio/channel.h"
#include "audio/engine.h"
#ifdef HAVE_JACK
#include "audio/engine_jack.h"
#endif
#include "audio/exporter.h"
#include "audio/marker_track.h"
#include "audio/master_track.h"
#include "audio/router.h"
#include "audio/position.h"
#include "audio/tempo_track.h"
#include "audio/transport.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include "midilib/src/midifile.h"

#include <glib/gi18n.h>

#include <sndfile.h>

#define  AMPLITUDE  (1.0 * 0x7F000000)

/**
 * Returns the audio format as string.
 *
 * @param extension Whether to return the extension
 *   for this format, or a human friendly label.
 */
const char *
exporter_stringize_audio_format (
  AudioFormat format,
  bool        extension)
{
  switch (format)
    {
    case AUDIO_FORMAT_FLAC:
      return "FLAC";
    case AUDIO_FORMAT_OGG_VORBIS:
      if (extension)
        {
          return "ogg";
        }
      else
        {
          return "ogg (Vorbis)";
        }
    case AUDIO_FORMAT_OGG_OPUS:
      if (extension)
        {
          return "ogg";
        }
      else
        {
          return "ogg (OPUS)";
        }
    case AUDIO_FORMAT_WAV:
      return "wav";
    case AUDIO_FORMAT_MP3:
      return "mp3";
    case AUDIO_FORMAT_MIDI:
      return "mid";
    case AUDIO_FORMAT_RAW:
      return "raw";
    case NUM_AUDIO_FORMATS:
      break;
    }

  g_return_val_if_reached (NULL);
}

static int
export_audio (
  ExportSettings * info)
{
  SF_INFO sfinfo;
  memset (&sfinfo, 0, sizeof (sfinfo));

#define EXPORT_CHANNELS 2

  switch (info->format)
    {
    case AUDIO_FORMAT_FLAC:
      sfinfo.format = SF_FORMAT_FLAC;
      break;
    case AUDIO_FORMAT_RAW:
      sfinfo.format = SF_FORMAT_RAW;
      break;
    case AUDIO_FORMAT_WAV:
      sfinfo.format = SF_FORMAT_WAV;
      break;
    case AUDIO_FORMAT_OGG_VORBIS:
#ifdef HAVE_OPUS
    case AUDIO_FORMAT_OGG_OPUS:
#endif
      sfinfo.format = SF_FORMAT_OGG;
      break;
    default:
      {
        const char * format =
          exporter_stringize_audio_format (
            info->format, false);

        info->has_error = true;
        sprintf (
          info->error_str,
          _("Format %s not supported yet"),
          format);
        g_warning ("%s", info->error_str);

        return -1;
      }
      break;
    }

  if (info->format == AUDIO_FORMAT_OGG_VORBIS)
    {
      sfinfo.format =
        sfinfo.format | SF_FORMAT_VORBIS;
    }
#ifdef HAVE_OPUS
  else if (info->format == AUDIO_FORMAT_OGG_OPUS)
    {
      sfinfo.format =
        sfinfo.format | SF_FORMAT_OPUS;
    }
#endif
  else if (info->depth == BIT_DEPTH_16)
    {
      sfinfo.format =
        sfinfo.format | SF_FORMAT_PCM_16;
      g_message ("PCM 16");
    }
  else if (info->depth == BIT_DEPTH_24)
    {
      sfinfo.format =
        sfinfo.format | SF_FORMAT_PCM_24;
      g_message ("PCM 24");
    }
  else if (info->depth == BIT_DEPTH_32)
    {
      sfinfo.format =
        sfinfo.format | SF_FORMAT_PCM_32;
      g_message ("PCM 32");
    }

  switch (info->time_range)
    {
    case TIME_RANGE_SONG:
      {
        ArrangerObject * start =
          (ArrangerObject *)
          marker_track_get_start_marker (
            P_MARKER_TRACK);
        ArrangerObject * end =
          (ArrangerObject *)
          marker_track_get_end_marker (
            P_MARKER_TRACK);
        sfinfo.frames =
          position_to_frames (
            &end->pos) -
          position_to_frames (
            &start->pos);
      }
      break;
    case TIME_RANGE_LOOP:
      sfinfo.frames =
        position_to_frames (
          &TRANSPORT->loop_end_pos) -
          position_to_frames (
            &TRANSPORT->loop_start_pos);
      break;
    case TIME_RANGE_CUSTOM:
      sfinfo.frames =
        position_to_frames (
          &info->custom_start) -
          position_to_frames (
            &info->custom_end);
      break;
    }

  /* set samplerate */
  if (info->format == AUDIO_FORMAT_OGG_OPUS)
    {
      /* Opus only supports sample rates of 8000,
       * 12000, 16000, 24000 and 48000 */
      /* TODO add option */
      sfinfo.samplerate = 48000;
    }
  else
    {
      sfinfo.samplerate =
        (int) AUDIO_ENGINE->sample_rate;
    }

  sfinfo.channels = EXPORT_CHANNELS;

  if (!sf_format_check (&sfinfo))
    {
      info->has_error = true;
      strcpy (
        info->error_str, _("SF INFO invalid"));
      g_warning ("%s", info->error_str);

      return - 1;
    }

  char * dir = io_get_dir (info->file_uri);
  io_mkdir (dir);
  g_free (dir);
  SNDFILE * sndfile =
    sf_open (info->file_uri, SFM_WRITE, &sfinfo);

  if (!sndfile)
    {
      int error = sf_error (NULL);
      const char * error_str =
        sf_error_number (error);

      info->has_error = true;
      sprintf (
        info->error_str,
        _("Couldn't open SNDFILE %s:\n%d: %s"),
        info->file_uri, error, error_str);
      g_warning ("%s", info->error_str);

      return - 1;
    }

  sf_set_string (
    sndfile, SF_STR_TITLE, PROJECT->title);
  sf_set_string (
    sndfile, SF_STR_SOFTWARE, PROGRAM_NAME);
  sf_set_string (
    sndfile, SF_STR_ARTIST, info->artist);
  sf_set_string (
    sndfile, SF_STR_GENRE, info->genre);

  Position prev_playhead_pos;
  /* position to start at */
  POSITION_INIT_ON_STACK (start_pos);
  /* position to stop at */
  POSITION_INIT_ON_STACK (stop_pos);
  position_set_to_pos (
    &prev_playhead_pos,
    &TRANSPORT->playhead_pos);
  switch (info->time_range)
    {
    case TIME_RANGE_SONG:
      {
        ArrangerObject * start =
          (ArrangerObject *)
          marker_track_get_start_marker (
            P_MARKER_TRACK);
        ArrangerObject * end =
          (ArrangerObject *)
          marker_track_get_end_marker (
            P_MARKER_TRACK);
        transport_move_playhead (
          TRANSPORT, &start->pos, F_PANIC,
          F_NO_SET_CUE_POINT);
        position_set_to_pos (
          &start_pos,
          &start->pos);
        position_set_to_pos (
          &stop_pos,
          &end->pos);
      }
      break;
    case TIME_RANGE_LOOP:
      transport_move_playhead (
        TRANSPORT, &TRANSPORT->loop_start_pos,
        F_PANIC, F_NO_SET_CUE_POINT);
      position_set_to_pos (
        &start_pos,
        &TRANSPORT->loop_start_pos);
      position_set_to_pos (
        &stop_pos,
        &TRANSPORT->loop_end_pos);
      break;
    case TIME_RANGE_CUSTOM:
      transport_move_playhead (
        TRANSPORT, &info->custom_start,
        F_PANIC, F_NO_SET_CUE_POINT);
      position_set_to_pos (
        &start_pos,
        &info->custom_start);
      position_set_to_pos (
        &stop_pos,
        &info->custom_end);
      break;
    }
  AUDIO_ENGINE->bounce_mode =
    info->mode == EXPORT_MODE_FULL ?
      BOUNCE_OFF : BOUNCE_ON;

  /* set jack freewheeling mode and temporarily
   * disable transport link */
#ifdef HAVE_JACK
  AudioEngineJackTransportType transport_type =
    AUDIO_ENGINE->transport_type;
  if (AUDIO_ENGINE->audio_backend ==
        AUDIO_BACKEND_JACK)
    {
      jack_set_freewheel (
        AUDIO_ENGINE->client, 1);

      engine_jack_set_transport_type (
        AUDIO_ENGINE,
        AUDIO_ENGINE_NO_JACK_TRANSPORT);
    }
#endif

  nframes_t nframes;
  g_return_val_if_fail (
    stop_pos.frames >= 1 ||
    start_pos.frames >= 0, -1);
  const unsigned long total_frames =
    (unsigned long)
    ((stop_pos.frames - 1) -
     start_pos.frames);
  sf_count_t covered = 0;
  float out_ptr[
    AUDIO_ENGINE->block_length * EXPORT_CHANNELS];
  do
    {
      /* calculate number of frames to process
       * this time */
      nframes =
        (nframes_t)
        MIN (
          (stop_pos.frames - 1) -
            TRANSPORT->playhead_pos.frames,
          (long) AUDIO_ENGINE->block_length);
      g_return_val_if_fail (nframes > 0, -1);

      /* run process code */
      engine_process_prepare (
        AUDIO_ENGINE, nframes);
      router_start_cycle (
        ROUTER, nframes, 0, PLAYHEAD);
      engine_post_process (
        AUDIO_ENGINE, nframes);

      /* by this time, the Master channel should
       * have its Stereo Out ports filled.
       * pass its buffers to the output */
      for (nframes_t i = 0; i < nframes; i++)
        {
          out_ptr[i * 2] =
            P_MASTER_TRACK->channel->
              stereo_out->l->buf[i];
          out_ptr[i * 2 + 1] =
            P_MASTER_TRACK->channel->
              stereo_out->r->buf[i];
        }

      /* seek to the write position in the file */
      if (covered != 0)
        {
          sf_count_t seek_cnt =
            sf_seek (
              sndfile, covered,
              SEEK_SET | SFM_WRITE);

          /* wav is weird for some reason */
          if (info->format == AUDIO_FORMAT_WAV ||
              info->format == AUDIO_FORMAT_RAW)
            {
              if (seek_cnt < 0)
                {
                  char err[256];
                  sf_error_str (
                    0, err, sizeof (err) - 1);
                  g_message (
                    "Error seeking file: %s", err);
                }
              g_warn_if_fail (seek_cnt == covered);
            }
        }

      /* write the frames for the current
       * cycle */
      sf_count_t written_frames =
        sf_writef_float (
          sndfile, out_ptr, nframes);
      g_warn_if_fail (written_frames == nframes);

      covered += nframes;
      if (G_UNLIKELY (
            covered !=
              TRANSPORT->playhead_pos.frames -
                start_pos.frames))
        {
          g_critical (
            "covered (%ld) != "
            "TRANSPORT->playhead_pos.frames (%ld) "
            "- start_pos.frames (%ld)",
            covered, TRANSPORT->playhead_pos.frames,
            start_pos.frames);
          return -1;
        }

      info->progress =
        (double)
        (TRANSPORT->playhead_pos.frames -
          start_pos.frames) /
        (double) total_frames;
    } while (
      TRANSPORT->playhead_pos.frames <
      stop_pos.frames - 1 && !info->cancelled);

  if (!info->cancelled)
    {
      g_warn_if_fail (
        covered == (sf_count_t) total_frames);
    }

  info->progress = 1.0;

  /* set jack freewheeling mode and transport type */
#ifdef HAVE_JACK
  if (AUDIO_ENGINE->audio_backend ==
        AUDIO_BACKEND_JACK)
    {
      jack_set_freewheel (
        AUDIO_ENGINE->client, 0);
      engine_jack_set_transport_type (
        AUDIO_ENGINE, transport_type);
    }
#endif

  AUDIO_ENGINE->bounce_mode = BOUNCE_OFF;
  transport_move_playhead (
    TRANSPORT, &prev_playhead_pos, F_PANIC,
    F_NO_SET_CUE_POINT);

  sf_close (sndfile);

  /* if cancelled, delete */
  if (info->cancelled)
    {
      io_remove (info->file_uri);
    }

  /* if cancelled, delete */
  if (info->cancelled)
    {
      g_message (
        "cancelled export to %s",
        info->file_uri);
    }
  else
    {
      g_message (
        "successfully exported to %s",
        info->file_uri);
    }

  return 0;
}

static int
export_midi (
  ExportSettings * info)
{
  MIDI_FILE *mf;

  int i;
  if ((mf = midiFileCreate (info->file_uri, TRUE)))
    {
      /* Write tempo information out to track 1 */
      midiSongAddTempo (
        mf, 1,
        (int)
        tempo_track_get_current_bpm (P_TEMPO_TRACK));

      midiFileSetPPQN (mf, TICKS_PER_QUARTER_NOTE);

      midiFileSetVersion (mf, 0);

      /* common time: 4 crochet beats, per bar */
      midiSongAddSimpleTimeSig (
        mf, 1, TRANSPORT->time_sig.beats_per_bar,
        math_round_double_to_int (
          TRANSPORT->ticks_per_beat));

      Track * track;
      for (i = 0; i < TRACKLIST->num_tracks; i++)
        {
          track = TRACKLIST->tracks[i];

          if (track_has_piano_roll (track))
            {
              /* write track to midi file */
              track_write_to_midi_file (
                track, mf);
            }
          info->progress =
            (double) i /
            (double) TRACKLIST->num_tracks;
        }

      midiFileClose(mf);
    }
  info->progress = 1.0;

  return 0;
}

/**
 * Returns an instance of default ExportSettings.
 *
 * It must be free'd with export_settings_free().
 */
ExportSettings *
export_settings_default ()
{
  ExportSettings * self =
    calloc (1, sizeof (ExportSettings));

  /* TODO */

  return self;
}

/**
 * Sets the defaults for bouncing.
 *
 * @note \ref ExportSettings.mode must already be
 *   set at this point.
 *
 * @param filepath Path to bounce to. If NULL, this
 *   will generate a temporary filepath.
 * @param bounce_name Name used for the file if
 *   \ref filepath is NULL.
 */
void
export_settings_set_bounce_defaults (
  ExportSettings * self,
  const char *     filepath,
  const char *     bounce_name)
{
  self->format = AUDIO_FORMAT_WAV;
  self->artist = g_strdup ("");
  self->genre = g_strdup ("");
  self->depth = BIT_DEPTH_16;
  self->time_range = TIME_RANGE_CUSTOM;
  self->cancelled = false;
  self->has_error = false;
  switch (self->mode)
    {
    case EXPORT_MODE_REGIONS:
      arranger_selections_get_start_pos (
        (ArrangerSelections *) TL_SELECTIONS,
        &self->custom_start, F_GLOBAL);
      arranger_selections_get_end_pos (
        (ArrangerSelections *) TL_SELECTIONS,
        &self->custom_end, F_GLOBAL);
      break;
    case EXPORT_MODE_TRACKS:
    case EXPORT_MODE_FULL:
      {
        ArrangerObject * start =
          (ArrangerObject *)
          marker_track_get_start_marker (
            P_MARKER_TRACK);
        ArrangerObject * end =
          (ArrangerObject *)
          marker_track_get_end_marker (
            P_MARKER_TRACK);
        position_set_to_pos (
          &self->custom_start, &start->pos);
        position_set_to_pos (
          &self->custom_end, &end->pos);
      }
      break;
    }
  position_add_ms (
    &self->custom_end,
    ZRYTHM_TESTING ?
      100 :
      g_settings_get_int (
        S_UI, "bounce-tail"));

  if (filepath)
    {
      self->file_uri = g_strdup (filepath);
      return;
    }
  else
    {
      char * tmp_dir =
        g_dir_make_tmp (
          "zrythm_bounce_XXXXXX", NULL);
      char filename[800];
      sprintf (
        filename, "%s.wav", bounce_name);
      self->file_uri =
        g_build_filename (
          tmp_dir, filename, NULL);
      g_free (tmp_dir);
    }
}

/**
 * Generic export thread to be used for simple
 * exporting.
 *
 * See bounce_dialog for an example.
 */
void *
exporter_generic_export_thread (
  ExportSettings * info)
{
  /* export */
  exporter_export (info);

  return NULL;
}

void
export_settings_free_members (
  ExportSettings * self)
{
  g_free_and_null (self->artist);
  g_free_and_null (self->genre);
  g_free_and_null (self->file_uri);
}

void
export_settings_free (
  ExportSettings * self)
{
  export_settings_free_members (self);

  free (self);
}

/**
 * To be called to create and perform an undoable
 * action for creating an audio track with the
 * bounced material.
 *
 * @param pos Position to place the audio region
 *   at.
 */
void
exporter_create_audio_track_after_bounce (
  ExportSettings * settings,
  const Position * pos)
{
  /* assert exporting is finished */
  g_return_if_fail (!AUDIO_ENGINE->exporting);

  SupportedFile * descr =
    supported_file_new_from_path (
      settings->file_uri);

  /* find next track */
  Track * track = NULL;
  switch (settings->mode)
    {
    case EXPORT_MODE_REGIONS:
      track =
        timeline_selections_get_last_track (
          TL_SELECTIONS);
      break;
    case EXPORT_MODE_TRACKS:
      track =
        tracklist_selections_get_lowest_track (
          TRACKLIST_SELECTIONS);
      break;
    default:
      g_return_if_reached ();
    }

  UndoableAction * ua =
    tracklist_selections_action_new_create (
      TRACK_TYPE_AUDIO, NULL,
      descr, track->pos + 1, pos, 1);
  Position tmp;
  position_set_to_pos (&tmp, PLAYHEAD);
  transport_set_playhead_pos (
    TRANSPORT, &settings->custom_start);
  undo_manager_perform (UNDO_MANAGER, ua);
  transport_set_playhead_pos (
    TRANSPORT, &tmp);
}

/**
 * Exports an audio file based on the given
 * settings.
 *
 * @return Non-zero if fail.
 */
int
exporter_export (ExportSettings * info)
{
  g_return_val_if_fail (info && info->file_uri, -1);

  g_message ("exporting to %s", info->file_uri);

  /* pause engine */
  EngineState state;
  engine_wait_for_pause (
    AUDIO_ENGINE, &state, F_NO_FORCE);

  TRANSPORT->play_state =
    PLAYSTATE_ROLLING;

  AUDIO_ENGINE->exporting = true;
  TRANSPORT->loop = false;

  /* deactivate and activate all plugins to make
   * them reset their states */
  /* TODO this doesn't reset the plugin state as
   * expected, so sending note off is needed */
  tracklist_activate_all_plugins (
    TRACKLIST, false);
  tracklist_activate_all_plugins (
    TRACKLIST, true);

  int ret = 0;
  if (info->format == AUDIO_FORMAT_MIDI)
    {
      ret = export_midi (info);
    }
  else
    {
      ret = export_audio (info);
    }

  /* restart engine */
  AUDIO_ENGINE->exporting = false;
  engine_resume (AUDIO_ENGINE, &state);

  if (ret)
    {
      g_warning ("export failed");
    }
  else
    {
      g_message ("done");
    }

  return ret;
}

