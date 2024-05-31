// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <cstdio>

#include "actions/tracklist_selections.h"
#include "dsp/channel.h"
#include "dsp/ditherer.h"
#include "dsp/engine.h"
#ifdef HAVE_JACK
#  include "dsp/engine_jack.h"
#endif
#include "dsp/exporter.h"
#include "dsp/marker_track.h"
#include "dsp/master_track.h"
#include "dsp/midi_event.h"
#include "dsp/position.h"
#include "dsp/router.h"
#include "dsp/tempo_track.h"
#include "dsp/transport.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/progress_info.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "midilib/src/midifile.h"
#include <sndfile.h>

#define AMPLITUDE (1.0 * 0x7F000000)

static const char * pretty_formats[] = {
  "AIFF",       "AU",  "CAF", "FLAC", "MP3",         "OGG (Vorbis)",
  "OGG (OPUS)", "RAW", "WAV", "W64",  "MIDI Type 0", "MIDI Type 1",
};

static const char * format_exts[] = {
  "aiff", "au",  "caf", "flac", "mp3", "ogg",
  "ogg",  "raw", "wav", "w64",  "mid", "mid",
};

/**
 * Returns the format as a human friendly label.
 */
const char *
export_format_to_pretty_str (ExportFormat format)
{
  return pretty_formats[ENUM_VALUE_TO_INT (format)];
}

/**
 * Returns the audio format as a file extension.
 */
const char *
export_format_to_ext (ExportFormat format)
{
  return format_exts[ENUM_VALUE_TO_INT (format)];
}

ExportFormat
export_format_from_pretty_str (const char * pretty_str)
{
  for (size_t i = 0; i < ENUM_COUNT (ExportFormat); i++)
    {
      if (string_is_equal (pretty_str, pretty_formats[i]))
        return ENUM_INT_TO_VALUE (ExportFormat, i);
    }

  g_return_val_if_reached (ExportFormat::EXPORT_FORMAT_FLAC);
}

NONNULL_ARGS (1)
static void get_export_time_range (
  const ExportSettings * info,
  Position *             start_pos,
  Position *             end_pos)
{
  switch (info->time_range)
    {
    case ExportTimeRange::TIME_RANGE_SONG:
      {
        ArrangerObject * start =
          (ArrangerObject *) marker_track_get_start_marker (P_MARKER_TRACK);
        ArrangerObject * end =
          (ArrangerObject *) marker_track_get_end_marker (P_MARKER_TRACK);
        *start_pos = start->pos;
        *end_pos = end->pos;
      }
      break;
    case ExportTimeRange::TIME_RANGE_LOOP:
      *start_pos = TRANSPORT->loop_start_pos;
      *end_pos = TRANSPORT->loop_end_pos;
      break;
    case ExportTimeRange::TIME_RANGE_CUSTOM:
      *start_pos = info->custom_start;
      *end_pos = info->custom_end;
      break;
    }
}

static int
export_audio (ExportSettings * info)
{
  SF_INFO sfinfo = {};

  ProgressInfo * pinfo = info->progress_info;

#define EXPORT_CHANNELS 2

  int type_major = 0;

  switch (info->format)
    {
    case ExportFormat::EXPORT_FORMAT_AIFF:
      type_major = SF_FORMAT_AIFF;
      break;
    case ExportFormat::EXPORT_FORMAT_AU:
      type_major = SF_FORMAT_AU;
      break;
    case ExportFormat::EXPORT_FORMAT_CAF:
      type_major = SF_FORMAT_CAF;
      break;
    case ExportFormat::EXPORT_FORMAT_FLAC:
      type_major = SF_FORMAT_FLAC;
      break;
    case ExportFormat::EXPORT_FORMAT_RAW:
      type_major = SF_FORMAT_RAW;
      break;
    case ExportFormat::EXPORT_FORMAT_WAV:
      type_major = SF_FORMAT_WAV;
      break;
    case ExportFormat::EXPORT_FORMAT_W64:
      type_major = SF_FORMAT_W64;
      break;
    case ExportFormat::EXPORT_FORMAT_OGG_VORBIS:
#ifdef HAVE_OPUS
    case ExportFormat::EXPORT_FORMAT_OGG_OPUS:
#endif
      type_major = SF_FORMAT_OGG;
      break;
    default:
      {
        const char * format = export_format_to_pretty_str (info->format);

        char * err_str =
          g_strdup_printf (_ ("Format %s not supported yet"), format);
        progress_info_mark_completed (
          pinfo, PROGRESS_COMPLETED_HAS_ERROR, err_str);
        g_warning ("%s", err_str);
        g_free (err_str);

        return -1;
      }
      break;
    }

  int type_minor = 0;
  if (info->format == ExportFormat::EXPORT_FORMAT_OGG_VORBIS)
    {
      type_minor = SF_FORMAT_VORBIS;
    }
#ifdef HAVE_OPUS
  else if (info->format == ExportFormat::EXPORT_FORMAT_OGG_OPUS)
    {
      type_minor = SF_FORMAT_OPUS;
    }
#endif
  else if (info->depth == BitDepth::BIT_DEPTH_16)
    {
      type_minor = SF_FORMAT_PCM_16;
      g_message ("PCM 16");
    }
  else if (info->depth == BitDepth::BIT_DEPTH_24)
    {
      type_minor = SF_FORMAT_PCM_24;
      g_message ("PCM 24");
    }
  else if (info->depth == BitDepth::BIT_DEPTH_32)
    {
      type_minor = SF_FORMAT_PCM_32;
      g_message ("PCM 32");
    }

  sfinfo.format = type_major | type_minor;

  Position start_pos, end_pos;
  position_init (&start_pos);
  position_init (&end_pos);
  get_export_time_range (info, &start_pos, &end_pos);
  sfinfo.frames =
    position_to_frames (&end_pos) - position_to_frames (&start_pos);

  g_return_val_if_fail (sfinfo.frames > 0, -1);

  /* set samplerate */
  if (info->format == ExportFormat::EXPORT_FORMAT_OGG_OPUS)
    {
      /* Opus only supports sample rates of 8000,
       * 12000, 16000, 24000 and 48000 */
      /* TODO add option */
      sfinfo.samplerate = 48000;
    }
  else
    {
      sfinfo.samplerate = (int) AUDIO_ENGINE->sample_rate;
    }

  sfinfo.channels = EXPORT_CHANNELS;

  if (!sf_format_check (&sfinfo))
    {
      char * err_str = g_strdup (_ ("SF INFO invalid"));
      progress_info_mark_completed (
        pinfo, PROGRESS_COMPLETED_HAS_ERROR, err_str);
      g_warning ("%s", err_str);
      g_free (err_str);

      return -1;
    }

  char *   dir = io_get_dir (info->file_uri);
  GError * err = NULL;
  bool     success = io_mkdir (dir, &err);
  if (!success)
    {
      g_warning ("Failed to create directory %s: %s", dir, err->message);
      return -1;
    }
  g_free (dir);
  SNDFILE * sndfile = sf_open (info->file_uri, SFM_WRITE, &sfinfo);

  if (!sndfile)
    {
      int          error = sf_error (NULL);
      const char * error_str = sf_error_number (error);

      char * err_str = g_strdup_printf (
        _ ("Couldn't open SNDFILE %s:\n%d: %s"), info->file_uri, error,
        error_str);
      progress_info_mark_completed (
        pinfo, PROGRESS_COMPLETED_HAS_ERROR, err_str);
      g_free (err_str);

      return -1;
    }
  if (sfinfo.format != (type_major | type_minor))
    {
      char * err_str = g_strdup_printf (
        _ ("Invalid SNDFILE format %s: 0x%08X != 0x%08X"), info->file_uri,
        sfinfo.format, type_major | type_minor);
      progress_info_mark_completed (
        pinfo, PROGRESS_COMPLETED_HAS_ERROR, err_str);
      g_free (err_str);

      return -1;
    }

  sf_set_string (sndfile, SF_STR_TITLE, PROJECT->title);
  sf_set_string (sndfile, SF_STR_SOFTWARE, PROGRAM_NAME);
  sf_set_string (sndfile, SF_STR_ARTIST, info->artist);
  sf_set_string (sndfile, SF_STR_TITLE, info->title);
  sf_set_string (sndfile, SF_STR_GENRE, info->genre);

  Position prev_playhead_pos;
  position_set_to_pos (&prev_playhead_pos, &TRANSPORT->playhead_pos);
  transport_set_playhead_pos (TRANSPORT, &start_pos);
  /* note - for custom ranges, the old code used :
    transport_move_playhead (
        TRANSPORT, &info->custom_start, F_PANIC,
        F_NO_SET_CUE_POINT, F_NO_PUBLISH_EVENTS);
    not sure why
  */
  AUDIO_ENGINE->bounce_mode =
    info->mode == ExportMode::EXPORT_MODE_FULL
      ? BounceMode::BOUNCE_OFF
      : BounceMode::BOUNCE_ON;
  AUDIO_ENGINE->bounce_step = info->bounce_step;
  AUDIO_ENGINE->bounce_with_parents = info->bounce_with_parents;

  /* set jack freewheeling mode and temporarily
   * disable transport link */
#ifdef HAVE_JACK
  AudioEngineJackTransportType transport_type = AUDIO_ENGINE->transport_type;
  if (AUDIO_ENGINE->audio_backend == AudioBackend::AUDIO_BACKEND_JACK)
    {
      engine_jack_set_transport_type (
        AUDIO_ENGINE,
        AudioEngineJackTransportType::AUDIO_ENGINE_NO_JACK_TRANSPORT);

      /* FIXME this is not how freewheeling should
       * work. see https://todo.sr.ht/~alextee/zrythm-feature/371 */
#  if 0
      g_message ("setting freewheel on");
      jack_set_freewheel (
        AUDIO_ENGINE->client, 1);
#  endif
    }
#endif

  /* init ditherer */
  Ditherer ditherer;
  memset (&ditherer, 0, sizeof (Ditherer));
  if (info->dither)
    {
      g_message ("dither %d bits", audio_bit_depth_enum_to_int (info->depth));
      ditherer_reset (&ditherer, audio_bit_depth_enum_to_int (info->depth));
    }

  g_return_val_if_fail (end_pos.frames >= 1 || start_pos.frames >= 0, -1);
  /*const unsigned long total_frames =*/
  /*(unsigned long)*/
  /*((end_pos.frames - 1) -*/
  /*start_pos.frames);*/
  const double total_ticks = (end_pos.ticks - start_pos.ticks);
  /* frames written so far */
  sf_count_t covered_frames = 0;
  double     covered_ticks = 0;
  /*sf_count_t last_playhead_frames = start_pos.frames;*/
  const size_t out_ptr_sz = AUDIO_ENGINE->block_length * EXPORT_CHANNELS;
  float        out_ptr[out_ptr_sz];
  bool         clipped = false;
  float        clip_amp = 0.f;
  do
    {
      /* calculate number of frames to process this time */
      const double    nticks = end_pos.ticks - TRANSPORT->playhead_pos.ticks;
      const nframes_t nframes = (nframes_t) MIN (
        (long) ceil (AUDIO_ENGINE->frames_per_tick * nticks),
        (long) AUDIO_ENGINE->block_length);
      g_return_val_if_fail (nframes > 0, -1);

      /* run process code */
      engine_process_prepare (AUDIO_ENGINE, nframes);
      EngineProcessTimeInfo time_nfo = {
        .g_start_frame = (unsigned_frame_t) PLAYHEAD->frames,
        .g_start_frame_w_offset = (unsigned_frame_t) PLAYHEAD->frames,
        .local_offset = 0,
        .nframes = nframes,
      };
      router_start_cycle (ROUTER, time_nfo);
      engine_post_process (AUDIO_ENGINE, nframes, nframes);

      /* by this time, the Master channel should have its Stereo Out ports
       * filled - pass its buffers to the output */
      float tmp_l[nframes];
      float tmp_r[nframes];
      /*
       * bypass gcc analyzer bug
       * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109789
       */
      dsp_fill (tmp_l, 0.f, nframes);
      dsp_fill (tmp_r, 0.f, nframes);
      for (nframes_t i = 0; i < nframes; i++)
        {
          tmp_l[i] = P_MASTER_TRACK->channel->stereo_out->get_l ().buf_[i];
          tmp_r[i] = P_MASTER_TRACK->channel->stereo_out->get_r ().buf_[i];
          out_ptr[i * 2] = tmp_l[i];
          out_ptr[i * 2 + 1] = tmp_r[i];
        }

      /* clipping detection */
      float max_amp = dsp_abs_max (tmp_l, nframes);
      if (max_amp > 1.f && max_amp > clip_amp)
        {
          clip_amp = max_amp;
          clipped = true;
        }
      max_amp = dsp_abs_max (tmp_r, nframes);
      if (max_amp > 1.f && max_amp > clip_amp)
        {
          clip_amp = max_amp;
          clipped = true;
        }

      /* apply dither */
      if (info->dither)
        {
          ditherer_process (&ditherer, out_ptr, nframes, 2);
        }

      /* no seek needed */
      (void) covered_frames; /* avoid unused warning */
#if 0
      /* seek to the write position in the file */
      if (covered_frames != 0)
        {
          sf_count_t seek_cnt =
            sf_seek (sndfile, covered_frames, SEEK_SET | SFM_WRITE);
          /*g_debug ("seek count: %ld", seek_cnt);*/

          /* note: FLAC returns -1
           * see https://github.com/libsndfile/libsndfile/issues/34#issuecomment-19867245
           * although it says it's fixed, this error still appears in 1.2.2 */
          if (seek_cnt < 0)
            {
              char * err_str = g_strdup_printf (
                _ ("Export failed: Error seeking file at %ld"),
                covered_frames);
              progress_info_mark_completed (
                pinfo, PROGRESS_COMPLETED_HAS_ERROR, err_str);
              g_free (err_str);
              return -1;
            }
        }
#endif

      /* write the frames for the current cycle */
      sf_count_t written_frames = sf_writef_float (sndfile, out_ptr, nframes);
      if (written_frames != nframes)
        {
          written_frames = sf_writef_float (sndfile, out_ptr, nframes);
          char * err_str = g_strdup_printf (
            _ ("Export failed: %ld frames written (expected %d)"),
            written_frames, nframes);
          progress_info_mark_completed (
            pinfo, PROGRESS_COMPLETED_HAS_ERROR, err_str);
          g_free (err_str);
          return -1;
        }
      /*g_debug ("wrote %d frames (total %ld)", nframes, covered_frames +
       * nframes);*/

      covered_frames += nframes;
      covered_ticks += AUDIO_ENGINE->ticks_per_frame * nframes;
#if 0
      long expected_nframes =
        TRANSPORT->playhead_pos.frames -
          last_playhead_frames;
      if (G_UNLIKELY (nframes != expected_nframes))
        {
          g_critical (
            "covered (%ld) != "
            "TRANSPORT->playhead_pos.frames (%ld) "
            "- start_pos.frames (%ld) (=%ld)",
            covered, TRANSPORT->playhead_pos.frames,
            start_pos.frames, expected_nframes);
          return -1;
        }
      last_playhead_frames += nframes;
#endif

      progress_info_update_progress (
        pinfo, (TRANSPORT->playhead_pos.ticks - start_pos.ticks) / total_ticks,
        NULL);
    }
  while (
    TRANSPORT->playhead_pos.ticks < end_pos.ticks
    && !progress_info_pending_cancellation (pinfo));

  if (!progress_info_pending_cancellation (pinfo))
    {
      g_warn_if_fail (
        math_floats_equal_epsilon (covered_ticks, total_ticks, 1.0));
    }

  /* TODO silence output */

  progress_info_update_progress (pinfo, 1.0, NULL);

  /* set jack freewheeling mode and transport type */
#ifdef HAVE_JACK
  if (AUDIO_ENGINE->audio_backend == AudioBackend::AUDIO_BACKEND_JACK)
    {
      /* FIXME this is not how freewheeling should
       * work. see https://todo.sr.ht/~alextee/zrythm-feature/371 */
#  if 0
      g_message ("setting freewheel off");
      jack_set_freewheel (
        AUDIO_ENGINE->client, 0);
#  endif
      engine_jack_set_transport_type (AUDIO_ENGINE, transport_type);
    }
#endif

  AUDIO_ENGINE->bounce_mode = BounceMode::BOUNCE_OFF;
  AUDIO_ENGINE->bounce_with_parents = false;
  transport_move_playhead (
    TRANSPORT, &prev_playhead_pos, F_PANIC, F_NO_SET_CUE_POINT,
    F_NO_PUBLISH_EVENTS);

  sf_close (sndfile);

  /* if cancelled, delete */
  if (progress_info_pending_cancellation (pinfo))
    {
      io_remove (info->file_uri);
    }

  /* if cancelled, delete */
  if (progress_info_pending_cancellation (pinfo))
    {
      g_message ("cancelled export to %s", info->file_uri);

      progress_info_mark_completed (pinfo, PROGRESS_COMPLETED_CANCELLED, NULL);
      return 0;
    }
  else
    {
      g_message ("successfully exported to %s", info->file_uri);

      if (clipped)
        {
          float  max_db = math_amp_to_dbfs (clip_amp);
          char * warn_str = g_strdup_printf (
            _ ("The exported audio contains segments louder than 0 dB (max detected %.1f dB)."),
            max_db);
          progress_info_mark_completed (
            pinfo, PROGRESS_COMPLETED_HAS_WARNING, warn_str);
          g_free (warn_str);
          return 0;
        }
    }

  progress_info_mark_completed (pinfo, PROGRESS_COMPLETED_SUCCESS, NULL);

  return 0;
}

static int
export_midi (ExportSettings * info)
{
  MIDI_FILE * mf;

  Position start_pos, end_pos;
  get_export_time_range (info, &start_pos, &end_pos);

  if ((mf = midiFileCreate (info->file_uri, TRUE)))
    {
      /* Write tempo information out to track 1 */
      midiSongAddTempo (
        mf, 1, (int) tempo_track_get_current_bpm (P_TEMPO_TRACK));

      midiFileSetPPQN (mf, TICKS_PER_QUARTER_NOTE);

      int midi_version =
        info->format == ExportFormat::EXPORT_FORMAT_MIDI0 ? 0 : 1;
      g_debug ("setting MIDI version to %d", midi_version);
      midiFileSetVersion (mf, midi_version);

      /* common time: 4 crochet beats, per bar */
      int beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
      midiSongAddSimpleTimeSig (
        mf, 1, beats_per_bar,
        math_round_double_to_signed_32 (TRANSPORT->ticks_per_beat));

      /* add generic export name if version 0 */
      if (midi_version == 0)
        {
          midiTrackAddText (mf, 1, textTrackName, info->title);
        }

      for (int i = 0; i < TRACKLIST->num_tracks; i++)
        {
          Track * track = TRACKLIST->tracks[i];

          if (track_type_has_piano_roll (track->type))
            {
              MidiEvents * events = NULL;
              if (midi_version == 0)
                {
                  events = midi_events_new ();
                }

              /* write track to midi file */
              track_write_to_midi_file (
                track, mf, midi_version == 0 ? events : NULL, &start_pos,
                &end_pos, midi_version == 0 ? false : info->lanes_as_tracks,
                midi_version == 0 ? false : true);

              if (events)
                {
                  midi_events_write_to_midi_file (events, mf, 1);
                  object_free_w_func_and_null (midi_events_free, events);
                }
            }
          progress_info_update_progress (
            info->progress_info, (double) i / (double) TRACKLIST->num_tracks,
            NULL);
        }

      midiFileClose (mf);
    }

  progress_info_mark_completed (
    info->progress_info, PROGRESS_COMPLETED_SUCCESS, NULL);

  return 0;
}

/**
 * Returns an instance of default ExportSettings.
 *
 * It must be free'd with export_settings_free().
 */
ExportSettings *
export_settings_new (void)
{
  ExportSettings * self = object_new_unresizable (ExportSettings);

  self->progress_info = progress_info_new ();

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
  ExportFormat     format,
  const char *     filepath,
  const char *     bounce_name)
{
  self->format = format;
  self->artist = g_strdup ("");
  self->title = g_strdup ("");
  self->genre = g_strdup ("");
  self->depth = BitDepth::BIT_DEPTH_16;
  self->time_range = ExportTimeRange::TIME_RANGE_CUSTOM;
  switch (self->mode)
    {
    case ExportMode::EXPORT_MODE_REGIONS:
      arranger_selections_get_start_pos (
        (ArrangerSelections *) TL_SELECTIONS, &self->custom_start, F_GLOBAL);
      arranger_selections_get_end_pos (
        (ArrangerSelections *) TL_SELECTIONS, &self->custom_end, F_GLOBAL);
      break;
    case ExportMode::EXPORT_MODE_TRACKS:
      self->disable_after_bounce =
        ZRYTHM_TESTING
          ? false
          : g_settings_get_boolean (S_UI, "disable-after-bounce");
      /* fallthrough */
    case ExportMode::EXPORT_MODE_FULL:
      {
        ArrangerObject * start =
          (ArrangerObject *) marker_track_get_start_marker (P_MARKER_TRACK);
        ArrangerObject * end =
          (ArrangerObject *) marker_track_get_end_marker (P_MARKER_TRACK);
        position_set_to_pos (&self->custom_start, &start->pos);
        position_set_to_pos (&self->custom_end, &end->pos);
      }
      break;
    }
  position_add_ms (
    &self->custom_end,
    ZRYTHM_TESTING ? 100 : g_settings_get_int (S_UI, "bounce-tail"));

  self->bounce_step =
    ZRYTHM_TESTING
      ? BounceStep::BOUNCE_STEP_POST_FADER
      : ENUM_INT_TO_VALUE (BounceStep, g_settings_get_enum (S_UI, "bounce-step"));
  self->bounce_with_parents =
    ZRYTHM_TESTING ? true : g_settings_get_boolean (S_UI, "bounce-with-parents");

  if (filepath)
    {
      self->file_uri = g_strdup (filepath);
      return;
    }
  else
    {
      char *       tmp_dir = g_dir_make_tmp ("zrythm_bounce_XXXXXX", NULL);
      const char * ext = export_format_to_ext (self->format);
      char         filename[800];
      sprintf (filename, "%s.%s", bounce_name, ext);
      self->file_uri = g_build_filename (tmp_dir, filename, NULL);
      g_free (tmp_dir);
    }
}

/**
 * This must be called on the main thread after the
 * intended tracks have been marked for bounce and
 * before exporting.
 *
 * @param engine_state Engine state when export was started so
 *   that it can be re-set after exporting.
 */
GPtrArray *
exporter_prepare_tracks_for_export (
  const ExportSettings * const settings,
  EngineState *                engine_state)
{
  AUDIO_ENGINE->preparing_to_export = true;

  engine_wait_for_pause (AUDIO_ENGINE, engine_state, Z_F_NO_FORCE, true);
  g_message ("engine paused");

  TRANSPORT->play_state = PlayState::PLAYSTATE_ROLLING;

  AUDIO_ENGINE->exporting = true;
  AUDIO_ENGINE->preparing_to_export = false;
  TRANSPORT->loop = false;

  g_message ("deactivating and reactivating plugins");

  /* deactivate and activate all plugins to make
   * them reset their states */
  /* TODO this doesn't reset the plugin state as
   * expected, so sending note off is needed */
  tracklist_activate_all_plugins (TRACKLIST, false);
  tracklist_activate_all_plugins (TRACKLIST, true);

  GPtrArray * conns = NULL;
  if (settings->mode != ExportMode::EXPORT_MODE_FULL)
    {
      /* disconnect all track faders from their channel outputs so that sends
       * and custom connections will work */
      conns = g_ptr_array_new_full (100, (GDestroyNotify) port_connection_free);
      for (int j = 0; j < TRACKLIST->num_tracks; j++)
        {
          Track * cur_tr = TRACKLIST->tracks[j];
          if (
            cur_tr->bounce || !track_type_has_channel (cur_tr->type)
            || cur_tr->out_signal_type != PortType::Audio)
            continue;

          PortIdentifier * l_src_id =
            &cur_tr->channel->fader->stereo_out->get_l ().id_;
          PortIdentifier * l_dest_id =
            &cur_tr->channel->stereo_out->get_l ().id_;
          PortConnection * l_conn = port_connections_manager_find_connection (
            PORT_CONNECTIONS_MGR, l_src_id, l_dest_id);
          g_return_val_if_fail (l_conn, NULL);
          g_ptr_array_add (conns, port_connection_clone (l_conn));
          port_connections_manager_ensure_disconnect (
            PORT_CONNECTIONS_MGR, l_src_id, l_dest_id);

          PortIdentifier * r_src_id =
            &cur_tr->channel->fader->stereo_out->get_r ().id_;
          PortIdentifier * r_dest_id =
            &cur_tr->channel->stereo_out->get_r ().id_;
          PortConnection * r_conn = port_connections_manager_find_connection (
            PORT_CONNECTIONS_MGR, r_src_id, r_dest_id);
          g_return_val_if_fail (r_conn, NULL);
          g_ptr_array_add (conns, port_connection_clone (r_conn));
          port_connections_manager_ensure_disconnect (
            PORT_CONNECTIONS_MGR, r_src_id, r_dest_id);
        }

      /* recalculate the graph to apply the
       * changes */
      router_recalc_graph (ROUTER, F_NOT_SOFT);

      /* remark all tracks for bounce */
      tracklist_mark_all_tracks_for_bounce (TRACKLIST, true);
    }

  g_message ("preparing playback snapshots...");
  tracklist_set_caches (TRACKLIST, CACHE_TYPE_PLAYBACK_SNAPSHOTS);

  return conns;
}

/**
 * This must be called on the main thread after the
 * export is completed.
 *
 * @param connections The array returned from
 *   exporter_prepare_tracks_for_export(). This
 *   function takes ownership of it and is
 *   responsible for freeing it.
 * @param engine_state Engine state when export was started so
 *   that it can be re-set after exporting.
 */
void
exporter_post_export (
  const ExportSettings * const settings,
  GPtrArray *                  connections,
  EngineState *                engine_state)
{
  /* not needed when exporting full */
  if (settings->mode != ExportMode::EXPORT_MODE_FULL)
    {
      g_return_if_fail (connections);

      /* re-connect disconnected connections */
      for (size_t j = 0; j < connections->len; j++)
        {
          PortConnection * conn =
            (PortConnection *) g_ptr_array_index (connections, j);
          port_connections_manager_ensure_connect_from_connection (
            PORT_CONNECTIONS_MGR, conn);
        }
      g_ptr_array_unref (connections);

      /* recalculate the graph to apply the
       * changes */
      router_recalc_graph (ROUTER, F_NOT_SOFT);
    }

  /* reset "bounce to master" on each track */
  for (int j = 0; j < TRACKLIST->num_tracks; j++)
    {
      Track * track = TRACKLIST->tracks[j];
      track->bounce_to_master = false;
    }

  /* restart engine */
  AUDIO_ENGINE->exporting = false;
  engine_resume (AUDIO_ENGINE, engine_state);
  g_message ("engine resumed");
}

void *
exporter_generic_export_thread (void * data)
{
  ExportSettings * info = (ExportSettings *) data;

  /* export */
  exporter_export (info);

  return NULL;
}

/**
 * Generic export task thread function.
 *
 * To be used as a GTaskThreadFunc.
 *
 * TODO.
 */
void
exporter_generic_export_task_thread (
  GTask *        task,
  gpointer       source_obj,
  gpointer       task_data,
  GCancellable * cancellable)
{
}

void
export_settings_print (const ExportSettings * self)
{
  const char * time_range_type_str = export_time_range_to_str (self->time_range);
  char time_range[600];
  if (self->time_range == ExportTimeRange::TIME_RANGE_CUSTOM)
    {
      char start_str[200];
      position_to_string (&self->custom_start, start_str);
      char end_str[200];
      position_to_string (&self->custom_end, end_str);
      sprintf (time_range, "Custom: %s ~ %s", start_str, end_str);
    }
  else
    {
      strcpy (time_range, time_range_type_str);
    }

  g_message (
    "~~~ Export Settings ~~~\n"
    "format: %s\n"
    "artist: %s\n"
    "title: %s\n"
    "genre: %s\n"
    "bit depth: %d\n"
    "time range: %s\n"
    "export mode: %s\n"
    "disable after bounce: %d\n"
    "bounce with parents: %d\n"
    "bounce step: %s\n"
    "dither: %d\n"
    "file: %s\n"
    "num files: %d\n",
    export_format_to_pretty_str (self->format), self->artist, self->title,
    self->genre, audio_bit_depth_enum_to_int (self->depth), time_range,
    export_mode_to_str (self->mode), self->disable_after_bounce,
    self->bounce_with_parents, bounce_step_to_str (self->bounce_step),
    self->dither, self->file_uri, self->num_files);
}

static void
export_settings_free_members (ExportSettings * self)
{
  g_free_and_null (self->artist);
  g_free_and_null (self->title);
  g_free_and_null (self->genre);
  g_free_and_null (self->file_uri);
  object_free_w_func_and_null (progress_info_free, self->progress_info);
}

void
export_settings_free (ExportSettings * self)
{
  export_settings_free_members (self);

  object_zero_and_free_unresizable (ExportSettings, self);
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

  SupportedFile * descr = supported_file_new_from_path (settings->file_uri);

  /* find next track */
  Track * last_track = NULL;
  Track * track_to_disable = NULL;
  switch (settings->mode)
    {
    case ExportMode::EXPORT_MODE_REGIONS:
      last_track = timeline_selections_get_last_track (TL_SELECTIONS);
      break;
    case ExportMode::EXPORT_MODE_TRACKS:
      last_track = tracklist_selections_get_lowest_track (TRACKLIST_SELECTIONS);
      if (settings->disable_after_bounce)
        {
          track_to_disable = last_track;
        }
      break;
    default:
      g_return_if_reached ();
    }

  g_return_if_fail (last_track);
  Position tmp;
  position_set_to_pos (&tmp, PLAYHEAD);
  transport_set_playhead_pos (TRANSPORT, &settings->custom_start);
  GError * err = NULL;
  bool     success = track_create_with_action (
    TrackType::TRACK_TYPE_AUDIO, NULL, descr, pos, last_track->pos + 1, 1,
    track_to_disable ? track_to_disable->pos : -1, NULL, &err);
  transport_set_playhead_pos (TRANSPORT, &tmp);
  if (!success)
    {
      HANDLE_ERROR_LITERAL (err, _ ("Failed to create audio track"));
    }
}

int
exporter_export (ExportSettings * info)
{
  g_return_val_if_fail (info && info->file_uri, -1);

  g_message ("exporting to %s", info->file_uri);

  export_settings_print (info);

  /* validate */
  if (info->time_range == ExportTimeRange::TIME_RANGE_CUSTOM)
    {
      Position init_pos;
      position_set_to_bar (&init_pos, 1);
      if (
        !position_is_before (&info->custom_start, &info->custom_end)
        || !position_is_after_or_equal (&info->custom_start, &init_pos))
        {
          progress_info_mark_completed (
            info->progress_info, PROGRESS_COMPLETED_HAS_ERROR,
            _ ("Invalid time range"));
          g_warning ("invalid time range");
          return -1;
        }
    }

  int ret = 0;
  if (
    info->format == ExportFormat::EXPORT_FORMAT_MIDI0
    || info->format == ExportFormat::EXPORT_FORMAT_MIDI1)
    {
      ret = export_midi (info);
    }
  else
    {
      ret = export_audio (info);
    }

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
