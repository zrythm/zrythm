/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/audio_region.h"
#include "audio/engine.h"
#include "audio/group_target_track.h"
#include "audio/metronome.h"
#include "audio/midi_event.h"
#include "audio/midi_file.h"
#include "audio/port.h"
#include "audio/router.h"
#include "audio/sample_processor.h"
#include "audio/tempo_track.h"
#include "audio/tracklist.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/plugin_settings.h"
#include "settings/settings.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

static void
init_common (
  SampleProcessor * self)
{
  self->tracklist =
    tracklist_new (NULL, self);
  self->midi_events = midi_events_new ();

  if (!ZRYTHM_TESTING)
    {
      char * setting_yaml =
        g_settings_get_string (
          S_UI_FILE_BROWSER, "instrument");
      PluginSetting * setting = NULL;
      if (strlen (setting_yaml) > 0)
        {
          setting =
            (PluginSetting *)
            yaml_deserialize (
              setting_yaml,
              &plugin_setting_schema);
          (void) setting;
        }
      else
        {
          /* pick first instrument found */
          PluginDescriptor * instrument =
            plugin_manager_pick_instrument (
              PLUGIN_MANAGER);
          if (!instrument)
            return;

          setting =
            plugin_settings_find (
              S_PLUGIN_SETTINGS, instrument);
          if (!setting)
            {
              setting =
                plugin_setting_new_default (
                  instrument);
            }
        }

      g_return_if_fail (setting);
      self->instrument_setting =
        plugin_setting_clone (setting, F_VALIDATE);
    }
}

void
sample_processor_init_loaded (
  SampleProcessor * self,
  AudioEngine *     engine)
{
  self->audio_engine = engine;
  fader_init_loaded (
    self->fader, NULL, NULL, self);

  init_common (self);
}

/**
 * Initializes a SamplePlayback with a sample to
 * play back.
 */
SampleProcessor *
sample_processor_new (
  AudioEngine * engine)
{
  SampleProcessor * self =
    object_new (SampleProcessor);
  self->audio_engine = engine;
  self->schema_version =
    SAMPLE_PROCESSOR_SCHEMA_VERSION;

  self->fader =
    fader_new (
      FADER_TYPE_SAMPLE_PROCESSOR, false,
      NULL, NULL, self);

  init_common (self);

  return self;
}

/**
 * Clears the buffers.
 */
void
sample_processor_prepare_process (
  SampleProcessor * self,
  const nframes_t   nframes)
{
  fader_clear_buffers (self->fader);
}

/**
 * Removes a SamplePlayback from the array.
 */
void
sample_processor_remove_sample_playback (
  SampleProcessor * self,
  SamplePlayback *  in_sp)
{
  int i, j;
  SamplePlayback * sp, * next_sp;
  for (i = 0; i < self->num_current_samples; i++)
    {
      sp = &self->current_samples[i];
      if (in_sp != sp)
        continue;

      for (j = i; j < self->num_current_samples - 1;
           j++)
        {
          sp = &self->current_samples[j];
          next_sp = &self->current_samples[j + 1];

          sp->buf = next_sp->buf;
          sp->buf_size = next_sp->buf_size;
          sp->offset = next_sp->offset;
          sp->volume = next_sp->volume;
          sp->start_offset = next_sp->start_offset;
        }
      break;
    }
  self->num_current_samples--;
}


/**
 * Process the samples for the given number of
 * frames.
 *
 * @param cycle_offset The local offset in the
 *   processing cycle.
 */
void
sample_processor_process (
  SampleProcessor * self,
  const nframes_t   cycle_offset,
  const nframes_t   nframes)
{
  nframes_t j;
  nframes_t max_frames;
  SamplePlayback * sp;
  g_return_if_fail (
    self && self->fader &&
    self->fader->stereo_out &&
    self->fader->stereo_out->l &&
    self->fader->stereo_out->l->buf &&
    self->fader->stereo_out->r &&
    self->fader->stereo_out->r->buf);

  z_return_if_fail_cmp (
    self->num_current_samples, <, 256);

  float * l = self->fader->stereo_out->l->buf,
        * r = self->fader->stereo_out->r->buf;

  /* process the samples in the queue */
  for (int i = self->num_current_samples - 1;
       i >= 0; i--)
    {
      sp = &self->current_samples[i];
      z_return_if_fail_cmp (sp->channels, >, 0);

      /* if sample starts after this cycle (eg,
       * when counting in for metronome),
       * update offset and skip processing */
      if (sp->start_offset >= nframes)
        {
          sp->start_offset -= nframes;
          continue;
        }

      /* if sample is already playing */
      if (sp->offset > 0)
        {
          /* fill in the buffer for as many frames
           * as possible */
          max_frames =
            MIN (
              (nframes_t)
                (sp->buf_size - sp->offset),
              nframes);
          for (j = 0; j < max_frames; j++)
            {
              nframes_t buf_offset =
                j + cycle_offset;
              z_return_if_fail_cmp (
                buf_offset, <, nframes)
              z_return_if_fail_cmp (
                sp->offset, <, sp->buf_size);
              if (sp->channels == 1)
                {
                  l[buf_offset] +=
                    (*sp->buf)[sp->offset] *
                    sp->volume;
                  r[buf_offset] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                }
              else if (sp->channels == 2)
                {
                  l[buf_offset] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                  r[buf_offset] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                }
            }
        }
      /* else if we can start playback in this
       * cycle */
      else if (sp->start_offset >= cycle_offset)
        {
          z_return_if_fail_cmp (sp->offset, ==, 0);
          z_return_if_fail_cmp (
            (cycle_offset + nframes), >=,
            sp->start_offset);

          /* fill in the buffer for as many frames
           * as possible */
          max_frames =
            MIN (
              (nframes_t) sp->buf_size,
              (cycle_offset + nframes) -
                sp->start_offset);
          for (j = 0; j < max_frames; j++)
            {
              nframes_t buf_offset =
                j + sp->start_offset;
              z_return_if_fail_cmp (
                buf_offset, <,
                cycle_offset + nframes);
              z_return_if_fail_cmp (
                sp->offset, <, sp->buf_size);
              if (sp->channels == 1)
                {
                  l[buf_offset] +=
                    (*sp->buf)[sp->offset] *
                    sp->volume;
                  r[buf_offset] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                }
              else if (sp->channels == 2)
                {
                  l[buf_offset] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                  r[buf_offset] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                }
            }
        }

      /* if the sample is finished playing, remove
       * it */
      if (sp->offset >= sp->buf_size)
        {
          sample_processor_remove_sample_playback (
            self, sp);
        }
    }

  if (self->roll)
    {
      midi_events_clear (
        self->midi_events, F_NOT_QUEUED);
      for (int i = self->tracklist->num_tracks - 1;
           i >= 1; i--)
        {
          Track * track = self->tracklist->tracks[i];

          float * audio_data_l = NULL,
                * audio_data_r = NULL;

          track_processor_clear_buffers (
            track->processor);

          EngineProcessTimeInfo time_nfo = {
            .g_start_frame =
              (unsigned_frame_t)
              self->playhead.frames + cycle_offset,
            .local_offset = cycle_offset,
            .nframes = nframes, };

          if (track->type == TRACK_TYPE_AUDIO)
            {
              track_processor_process (
                track->processor, &time_nfo);

              audio_data_l =
                track->processor->stereo_out->l->buf;
              audio_data_r =
                track->processor->stereo_out->l->buf;
            }
          else if (track->type == TRACK_TYPE_MIDI)
            {
              track_processor_process (
                track->processor, &time_nfo);
              midi_events_append (
                track->processor->midi_out->
                  midi_events,
                self->midi_events,
                cycle_offset, nframes,
                F_NOT_QUEUED);
            }
          else if (
            track->type == TRACK_TYPE_INSTRUMENT)
            {
              if (!track->channel->instrument)
                return;

              plugin_prepare_process (
                track->channel->instrument);
              midi_events_append (
                self->midi_events,
                track->channel->instrument->
                  midi_in_port->midi_events,
                cycle_offset, nframes,
                F_NOT_QUEUED);
              plugin_process (
                track->channel->instrument,
                &time_nfo);
              audio_data_l =
                track->channel->instrument->l_out->
                  buf;
              audio_data_r =
                track->channel->instrument->r_out->
                  buf;
            }

          if (audio_data_l && audio_data_r)
            {
              dsp_mix2 (
                &l[cycle_offset],
                &audio_data_l[cycle_offset],
                1.f, self->fader->amp->control,
                nframes);
              dsp_mix2 (
                &r[cycle_offset],
                &audio_data_r[cycle_offset],
                1.f, self->fader->amp->control,
                nframes);
            }
        }
    }

  position_add_frames (&self->playhead, nframes);

  /* stop rolling if no more material */
  if (position_is_after (
        &self->playhead, &self->file_end_pos))
    self->roll = false;
}

/**
 * Queues a metronomem tick at the given offset.
 *
 * Used for countin.
 */
void
sample_processor_queue_metronome_countin (
  SampleProcessor * self)
{
  PrerollCountBars bars =
    g_settings_get_enum (
      S_TRANSPORT, "metronome-countin");
  int num_bars =
    transport_preroll_count_bars_enum_to_int (bars);
  int beats_per_bar =
    tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  int num_beats = beats_per_bar * num_bars;

  double frames_per_bar =
    AUDIO_ENGINE->frames_per_tick *
    (double) TRANSPORT->ticks_per_bar;
  for (int i = 0; i < num_bars; i++)
    {
      long offset =
        (long) ((double) i * frames_per_bar);
      SamplePlayback * sp =
        &self->current_samples[
          self->num_current_samples];
      sample_playback_init (
        sp, &METRONOME->emphasis,
        METRONOME->emphasis_size,
        METRONOME->emphasis_channels,
        0.1f * METRONOME->volume, offset);
      self->num_current_samples++;
    }

  double frames_per_beat =
    AUDIO_ENGINE->frames_per_tick *
    (double) TRANSPORT->ticks_per_beat;
  for (int i = 0; i < num_beats; i++)
    {
      if (i % beats_per_bar == 0)
        continue;

      long offset =
        (long) ((double) i * frames_per_beat);
      SamplePlayback * sp =
        &self->current_samples[
          self->num_current_samples];
      sample_playback_init (
        sp, &METRONOME->normal,
        METRONOME->normal_size,
        METRONOME->normal_channels,
        0.1f * METRONOME->volume, offset);
      self->num_current_samples++;
    }
}

/**
 * Queues a metronomem tick at the given local
 * offset.
 *
 * Realtime function.
 */
void
sample_processor_queue_metronome (
  SampleProcessor * self,
  MetronomeType     type,
  nframes_t         offset)
{
  g_return_if_fail (
    METRONOME->emphasis && METRONOME->normal);

#if 0
  Position pos;
  position_set_to_pos (&pos, PLAYHEAD);
  position_add_frames (&pos, offset);
  char metronome_pos_str[60];
  position_to_string (&pos, metronome_pos_str);
  g_message (
    "%s metronome queued at %s (loffset %d)",
    (type == METRONOME_TYPE_EMPHASIS) ?
      "emphasis" : "normal",
    metronome_pos_str, offset);
#endif

  SamplePlayback * sp =
    &self->current_samples[
      self->num_current_samples];

  g_return_if_fail (
    offset < AUDIO_ENGINE->block_length);

  /*g_message ("queuing %u", offset);*/
  if (type == METRONOME_TYPE_EMPHASIS)
    {
      sample_playback_init (
        sp, &METRONOME->emphasis,
        METRONOME->emphasis_size,
        METRONOME->emphasis_channels,
        0.1f * METRONOME->volume, offset);
    }
  else if (type == METRONOME_TYPE_NORMAL)
    {
      sample_playback_init (
        sp, &METRONOME->normal,
        METRONOME->normal_size,
        METRONOME->normal_channels,
        0.1f * METRONOME->volume, offset);
    }

  self->num_current_samples++;
}

/**
 * Adds a sample to play to the queue from a file
 * path.
 */
void
sample_processor_queue_sample_from_file (
  SampleProcessor * self,
  const char *      path)
{
  /* TODO */
}

/**
 * Adds a file (audio or MIDI) to the queue.
 */
void
sample_processor_queue_file (
  SampleProcessor *     self,
  const SupportedFile * file)
{
  EngineState state;
  engine_wait_for_pause (
    AUDIO_ENGINE, &state, false);

  /* clear tracks */
  for (int i = self->tracklist->num_tracks - 1;
       i >= 0; i--)
    {
      Track * track = self->tracklist->tracks[i];

      /* remove state dir if instrument */
      if (track->type == TRACK_TYPE_INSTRUMENT)
        {
          char * state_dir =
            plugin_get_abs_state_dir (
              track->channel->instrument,
              F_NOT_BACKUP);
          if (state_dir)
            {
              io_rmdir (state_dir, F_FORCE);
              g_free (state_dir);
            }
        }

      tracklist_remove_track (
        self->tracklist, track, true,
        F_FREE, F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);
    }

  Position start_pos;
  position_set_to_bar (&start_pos, 1);
  position_set_to_bar (&self->file_end_pos, 1);

  /* create master track */
  g_debug ("creating master track...");
  Track * track =
    track_new (
      TRACK_TYPE_MASTER, self->tracklist->num_tracks,
      "Sample Processor Master", F_WITHOUT_LANE);
  self->tracklist->master_track = track;
  tracklist_insert_track (
    self->tracklist, track, track->pos,
    F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);

  if (supported_file_type_is_audio (file->type))
    {
      g_debug ("creating audio track...");
      track =
        track_new (
          TRACK_TYPE_AUDIO,
          self->tracklist->num_tracks,
          "Sample processor audio",
          F_WITH_LANE);
      tracklist_insert_track (
        self->tracklist, track, track->pos,
        F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);

      /* create an audio region & add to
       * track */
      ZRegion * ar =
        audio_region_new (
          -1, file->abs_path, false,
          NULL, 0, NULL, 0, 0,
          &start_pos, 0, 0, 0);
      track_add_region (
        track, ar, NULL, 0, F_GEN_NAME,
        F_NO_PUBLISH_EVENTS);

      ArrangerObject * obj =  (ArrangerObject *) ar;
      position_set_to_pos (
        &self->file_end_pos, &obj->end_pos);
    }
  else if (supported_file_type_is_midi (
             file->type) &&
           self->instrument_setting)
    {
      /* create an instrument track */
      g_debug ("creating instrument track...");
      Track * instrument_track =
        track_new (
          TRACK_TYPE_INSTRUMENT,
          self->tracklist->num_tracks,
          "Sample processor instrument",
          F_WITH_LANE);
      tracklist_insert_track (
        self->tracklist, instrument_track,
        instrument_track->pos,
        F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);
      GError * err = NULL;
      Plugin * pl =
        plugin_new_from_setting (
          self->instrument_setting,
          track_get_name_hash (instrument_track),
          PLUGIN_SLOT_INSTRUMENT, -1, &err);
      if (!pl)
        {
          HANDLE_ERROR (
            err,
            _("Failed to create plugin %s"),
            self->instrument_setting->descr->name);
          return;
        }
      int ret =
        plugin_instantiate (pl, NULL, &err);
      if (ret != 0)
        {
          HANDLE_ERROR (
            err,
            _("Failed to instantiate plugin %s"),
            self->instrument_setting->descr->name);
          return;
        }
      g_return_if_fail (
        plugin_activate (pl, F_ACTIVATE) == 0);
      g_return_if_fail (pl->midi_in_port);
      g_return_if_fail (pl->l_out);
      g_return_if_fail (pl->r_out);
      channel_add_plugin (
        instrument_track->channel,
        PLUGIN_SLOT_INSTRUMENT,
        pl->id.slot, pl, F_NO_CONFIRM,
        F_NOT_MOVING_PLUGIN, F_GEN_AUTOMATABLES,
        F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS);

      int num_tracks =
        midi_file_get_num_tracks (
          file->abs_path, true);
      g_debug (
        "creating %d MIDI tracks...", num_tracks);
      for (int i = 0; i < num_tracks; i++)
        {
          char name[600];
          sprintf (
            name, "Sample processor MIDI %d", i);
          track =
            track_new (
              TRACK_TYPE_MIDI,
              self->tracklist->num_tracks, name,
              F_WITH_LANE);
          tracklist_insert_track (
            self->tracklist, track, track->pos,
            F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);

          /* route track to instrument */
          group_target_track_add_child (
            instrument_track,
            track_get_name_hash (track),
            F_CONNECT, F_NO_RECALC_GRAPH,
            F_NO_PUBLISH_EVENTS);

          /* create a MIDI region from the MIDI
           * file & add to track */
          ZRegion * mr =
            midi_region_new_from_midi_file (
              &start_pos, file->abs_path,
              track_get_name_hash (track), 0, 0, i);
          if (mr)
            {
              track_add_region (
                track, mr, NULL, 0,
                /* name could already be generated
                 * based
                 * on the track name (if any) in
                 * the MIDI file */
                mr->name ?
                  F_NO_GEN_NAME : F_GEN_NAME,
                F_NO_PUBLISH_EVENTS);

              ArrangerObject * obj =
                (ArrangerObject *) mr;
              if (position_is_after (
                    &obj->end_pos,
                    &self->file_end_pos))
                {
                  position_set_to_pos (
                    &self->file_end_pos,
                    &obj->end_pos);
                }
            }
          else
            {
              g_message (
                "Failed to create MIDI region from "
                "file %s",
                file->abs_path);
            }
        }
    }

  self->roll = true;
  position_set_to_bar (&self->playhead, 1);

  /* add some room to end pos */
  position_add_bars (&self->file_end_pos, 1);

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  engine_resume (AUDIO_ENGINE, &state);
}

/**
 * Adds a chord preset to the queue.
 */
void
sample_processor_queue_chord_preset (
  SampleProcessor *   self,
  const ChordPreset * chord_pset)
{
  /* TODO */
}

/**
 * Stops playback of files (auditioning).
 */
void
sample_processor_stop_file_playback (
  SampleProcessor *     self)
{
  EngineState state;
  engine_wait_for_pause (
    AUDIO_ENGINE, &state, false);

  self->roll = false;
  position_set_to_bar (&self->playhead, 1);

  engine_resume (AUDIO_ENGINE, &state);
}

void
sample_processor_disconnect (
  SampleProcessor * self)
{
  fader_disconnect_all (self->fader);
}

SampleProcessor *
sample_processor_clone (
  const SampleProcessor * src)
{
  SampleProcessor * self =
    object_new (SampleProcessor);
  self->schema_version =
    SAMPLE_PROCESSOR_SCHEMA_VERSION;

  self->fader = fader_clone (src->fader);

  return self;
}

void
sample_processor_free (
  SampleProcessor * self)
{
  if (self == SAMPLE_PROCESSOR)
    sample_processor_disconnect (self);

  object_free_w_func_and_null (
    tracklist_free, self->tracklist);
  object_free_w_func_and_null (
    fader_free, self->fader);

  object_zero_and_free (self);
}
