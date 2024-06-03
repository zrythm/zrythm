// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_region.h"
#include "dsp/engine.h"
#include "dsp/group_target_track.h"
#include "dsp/metronome.h"
#include "dsp/midi_event.h"
#include "dsp/port.h"
#include "dsp/router.h"
#include "dsp/sample_processor.h"
#include "dsp/tempo_track.h"
#include "dsp/tracklist.h"
#include "io/midi_file.h"
#include "io/serialization/plugin.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/chord_preset.h"
#include "settings/g_settings_manager.h"
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
init_common (SampleProcessor * self)
{
  self->tracklist = tracklist_new (NULL, self);
  self->midi_events = midi_events_new ();
  zix_sem_init (&self->rebuilding_sem, 1);

  if (!ZRYTHM_TESTING)
    {
      char * setting_json =
        g_settings_get_string (S_UI_FILE_BROWSER, "instrument");
      PluginSetting * setting = NULL;
      bool            json_read = false;
      if (strlen (setting_json) > 0)
        {
          yyjson_doc * doc = yyjson_read_opts (
            (char *) setting_json, strlen (setting_json), 0, NULL, NULL);
          if (doc)
            {
              yyjson_val * root = yyjson_doc_get_root (doc);
              g_return_if_fail (root);
              setting = object_new (PluginSetting);
              GError * err = NULL;
              plugin_setting_deserialize_from_json (doc, root, setting, &err);
              json_read = true;
            }
          else
            {
              g_warning (
                "failed to get instrument from JSON:\n%s", setting_json);
              g_free (setting_json);
            }
        }

      if (!json_read)
        {
          /* pick first instrument found */
          PluginDescriptor * instrument =
            plugin_manager_pick_instrument (PLUGIN_MANAGER);
          if (!instrument)
            return;

          setting = plugin_settings_find (S_PLUGIN_SETTINGS, instrument);
          if (!setting)
            {
              setting = plugin_setting_new_default (instrument);
            }
        }

      g_return_if_fail (setting);
      self->instrument_setting = plugin_setting_clone (setting, F_VALIDATE);
    }
}

void
sample_processor_init_loaded (SampleProcessor * self, AudioEngine * engine)
{
  self->audio_engine = engine;
  fader_init_loaded (self->fader, NULL, NULL, self);

  init_common (self);
}

/**
 * Initializes a SamplePlayback with a sample to
 * play back.
 */
SampleProcessor *
sample_processor_new (AudioEngine * engine)
{
  SampleProcessor * self = object_new (SampleProcessor);
  self->audio_engine = engine;

  self->fader =
    fader_new (FaderType::FADER_TYPE_SAMPLE_PROCESSOR, false, NULL, NULL, self);

  init_common (self);

  return self;
}

/**
 * Clears the buffers.
 */
void
sample_processor_prepare_process (SampleProcessor * self, const nframes_t nframes)
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
  int             i, j;
  SamplePlayback *sp, *next_sp;
  for (i = 0; i < self->num_current_samples; i++)
    {
      sp = &self->current_samples[i];
      if (in_sp != sp)
        continue;

      for (j = i; j < self->num_current_samples - 1; j++)
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
 * Process the samples for the given number of frames.
 *
 * @param offset The local offset in the processing cycle.
 * @param nframes The number of frames to process in this call.
 */
void
sample_processor_process (
  SampleProcessor * self,
  const nframes_t   cycle_offset,
  const nframes_t   nframes)
{
  nframes_t j;
  nframes_t max_frames;
  g_return_if_fail (self && self->fader && self->fader->stereo_out);

  z_return_if_fail_cmp (self->num_current_samples, <, 256);

  bool lock_acquired =
    zix_sem_try_wait (&self->rebuilding_sem) == ZIX_STATUS_SUCCESS;
  if (!lock_acquired)
    {
      g_message ("lock not acquired");
      return;
    }

  float * l = self->fader->stereo_out->get_l ().buf_.data ();
  float * r = self->fader->stereo_out->get_r ().buf_.data ();

  /* process the samples in the queue */
  for (int i = self->num_current_samples - 1; i >= 0; i--)
    {
      SamplePlayback * sp = &self->current_samples[i];
      z_return_if_fail_cmp (sp->channels, >, 0);

      /* if sample starts after this cycle (eg, when counting in for metronome),
       * update offset and skip processing */
      if (sp->start_offset >= nframes)
        {
          sp->start_offset -= nframes;
          continue;
        }

      /* if sample is already playing */
      if (sp->offset > 0)
        {
          /* fill in the buffer for as many frames as possible */
          max_frames = MIN ((nframes_t) (sp->buf_size - sp->offset), nframes);
          for (j = 0; j < max_frames; j++)
            {
              nframes_t buf_offset = j + cycle_offset;
              z_return_if_fail_cmp (buf_offset, <, AUDIO_ENGINE->block_length);
              z_return_if_fail_cmp (sp->offset, <, sp->buf_size);
              if (sp->channels == 1)
                {
                  l[buf_offset] += (*sp->buf)[sp->offset] * sp->volume;
                  r[buf_offset] += (*sp->buf)[sp->offset++] * sp->volume;
                }
              else if (sp->channels == 2)
                {
                  l[buf_offset] += (*sp->buf)[sp->offset++] * sp->volume;
                  r[buf_offset] += (*sp->buf)[sp->offset++] * sp->volume;
                }
            }
        }
      /* else if we can start playback in this
       * cycle */
      else if (sp->start_offset >= cycle_offset)
        {
          z_return_if_fail_cmp (sp->offset, ==, 0);
          z_return_if_fail_cmp ((cycle_offset + nframes), >=, sp->start_offset);

          /* fill in the buffer for as many frames
           * as possible */
          max_frames = MIN (
            (nframes_t) sp->buf_size,
            (cycle_offset + nframes) - sp->start_offset);
          for (j = 0; j < max_frames; j++)
            {
              nframes_t buf_offset = j + sp->start_offset;
              z_return_if_fail_cmp (buf_offset, <, cycle_offset + nframes);
              z_return_if_fail_cmp (sp->offset, <, sp->buf_size);
              if (sp->channels == 1)
                {
                  l[buf_offset] += (*sp->buf)[sp->offset] * sp->volume;
                  r[buf_offset] += (*sp->buf)[sp->offset++] * sp->volume;
                }
              else if (sp->channels == 2)
                {
                  l[buf_offset] += (*sp->buf)[sp->offset++] * sp->volume;
                  r[buf_offset] += (*sp->buf)[sp->offset++] * sp->volume;
                }
            }
        }

      /* if the sample is finished playing, remove
       * it */
      if (sp->offset >= sp->buf_size)
        {
          sample_processor_remove_sample_playback (self, sp);
        }
    }

  if (self->roll)
    {
      midi_events_clear (self->midi_events, F_NOT_QUEUED);
      for (int i = self->tracklist->tracks.size () - 1; i >= 1; i--)
        {
          Track * track = self->tracklist->tracks[i];

          float *audio_data_l = nullptr, *audio_data_r = nullptr;

          track_processor_clear_buffers (track->processor);

          EngineProcessTimeInfo time_nfo = {
            .g_start_frame = (unsigned_frame_t) self->playhead.frames,
            .g_start_frame_w_offset =
              (unsigned_frame_t) self->playhead.frames + cycle_offset,
            .local_offset = cycle_offset,
            .nframes = nframes,
          };

          if (track->type == TrackType::TRACK_TYPE_AUDIO)
            {
              track_processor_process (track->processor, &time_nfo);

              audio_data_l = track->processor->stereo_out->get_l ().buf_.data ();
              audio_data_r = track->processor->stereo_out->get_l ().buf_.data ();
            }
          else if (track->type == TrackType::TRACK_TYPE_MIDI)
            {
              track_processor_process (track->processor, &time_nfo);
              midi_events_append (
                self->midi_events, track->processor->midi_out->midi_events_,
                cycle_offset, nframes, F_NOT_QUEUED);
            }
          else if (track->type == TrackType::TRACK_TYPE_INSTRUMENT)
            {
              Plugin * const ins = track->channel->instrument;
              if (!ins)
                return;

              plugin_prepare_process (ins);
              midi_events_append (
                ins->midi_in_port->midi_events_, self->midi_events,
                cycle_offset, nframes, F_NOT_QUEUED);
              plugin_process (ins, &time_nfo);
              audio_data_l = ins->l_out->buf_.data ();
              audio_data_r = ins->r_out->buf_.data ();
            }

          if (audio_data_l && audio_data_r)
            {
              dsp_mix2 (
                &l[cycle_offset], &audio_data_l[cycle_offset], 1.f,
                self->fader->amp->control_, nframes);
              dsp_mix2 (
                &r[cycle_offset], &audio_data_r[cycle_offset], 1.f,
                self->fader->amp->control_, nframes);
            }
        }
    }

  position_add_frames (&self->playhead, nframes);

  /* stop rolling if no more material */
  if (position_is_after (&self->playhead, &self->file_end_pos))
    self->roll = false;

  zix_sem_post (&self->rebuilding_sem);
}

/**
 * Queues a metronomem tick at the given offset.
 *
 * Used for countin.
 */
void
sample_processor_queue_metronome_countin (SampleProcessor * self)
{
  PrerollCountBars bars = ENUM_INT_TO_VALUE (
    PrerollCountBars, g_settings_get_enum (S_TRANSPORT, "metronome-countin"));
  int num_bars = transport_preroll_count_bars_enum_to_int (bars);
  int beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  int num_beats = beats_per_bar * num_bars;

  double frames_per_bar =
    AUDIO_ENGINE->frames_per_tick * (double) TRANSPORT->ticks_per_bar;
  for (int i = 0; i < num_bars; i++)
    {
      long             offset = (long) ((double) i * frames_per_bar);
      SamplePlayback * sp = &self->current_samples[self->num_current_samples];
      sample_playback_init (
        sp, &METRONOME->emphasis, METRONOME->emphasis_size,
        METRONOME->emphasis_channels, 0.1f * METRONOME->volume, offset);
      self->num_current_samples++;
    }

  double frames_per_beat =
    AUDIO_ENGINE->frames_per_tick * (double) TRANSPORT->ticks_per_beat;
  for (int i = 0; i < num_beats; i++)
    {
      if (i % beats_per_bar == 0)
        continue;

      long             offset = (long) ((double) i * frames_per_beat);
      SamplePlayback * sp = &self->current_samples[self->num_current_samples];
      sample_playback_init (
        sp, &METRONOME->normal, METRONOME->normal_size,
        METRONOME->normal_channels, 0.1f * METRONOME->volume, offset);
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
  g_return_if_fail (METRONOME->emphasis && METRONOME->normal);

#if 0
  Position pos;
  position_set_to_pos (&pos, PLAYHEAD);
  position_add_frames (&pos, offset);
  char metronome_pos_str[60];
  position_to_string (&pos, metronome_pos_str);
  g_message (
    "%s metronome queued at %s (loffset %d)",
    (type == MetronomeType::METRONOME_TYPE_EMPHASIS) ?
      "emphasis" : "normal",
    metronome_pos_str, offset);
#endif

  SamplePlayback * sp = &self->current_samples[self->num_current_samples];

  g_return_if_fail (offset < AUDIO_ENGINE->block_length);

  /*g_message ("queuing %u", offset);*/
  if (type == MetronomeType::METRONOME_TYPE_EMPHASIS)
    {
      sample_playback_init (
        sp, &METRONOME->emphasis, METRONOME->emphasis_size,
        METRONOME->emphasis_channels, 0.1f * METRONOME->volume, offset);
    }
  else if (type == MetronomeType::METRONOME_TYPE_NORMAL)
    {
      sample_playback_init (
        sp, &METRONOME->normal, METRONOME->normal_size,
        METRONOME->normal_channels, 0.1f * METRONOME->volume, offset);
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

static void
queue_file_or_chord_preset (
  SampleProcessor *      self,
  const FileDescriptor * file,
  const ChordPreset *    chord_pset)
{
  zix_sem_wait (&self->rebuilding_sem);

  /* clear tracks */
  for (int i = self->tracklist->tracks.size () - 1; i >= 0; i--)
    {
      Track * track = self->tracklist->tracks[i];

      /* remove state dir if instrument */
      if (track->type == TrackType::TRACK_TYPE_INSTRUMENT)
        {
          char * state_dir = plugin_get_abs_state_dir (
            track->channel->instrument, F_NOT_BACKUP, true);
          if (state_dir)
            {
              io_rmdir (state_dir, Z_F_FORCE);
              g_free (state_dir);
            }
        }

      tracklist_remove_track (
        self->tracklist, track, true, F_FREE, F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);
    }

  Position start_pos;
  position_set_to_bar (&start_pos, 1);
  position_set_to_bar (&self->file_end_pos, 1);

  /* create master track */
  g_debug ("creating master track...");
  Track * track = track_new (
    TrackType::TRACK_TYPE_MASTER, self->tracklist->tracks.size (),
    "Sample Processor Master", F_WITHOUT_LANE);
  self->tracklist->master_track = track;
  tracklist_insert_track (
    self->tracklist, track, track->pos, F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);

  if (file && file->is_audio ())
    {
      g_debug ("creating audio track...");
      track = track_new (
        TrackType::TRACK_TYPE_AUDIO, self->tracklist->tracks.size (),
        "Sample processor audio", F_WITH_LANE);
      tracklist_insert_track (
        self->tracklist, track, track->pos, F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);

      /* create an audio region & add to
       * track */
      GError * err = NULL;
      Region * ar = audio_region_new (
        -1, file->abs_path.c_str (), false, NULL, 0, NULL, 0,
        ENUM_INT_TO_VALUE (BitDepth, 0), &start_pos, 0, 0, 0, &err);
      if (!ar)
        {
          HANDLE_ERROR_LITERAL (err, "Failed to create audio region");
          return;
        }
      bool success = track_add_region (
        track, ar, NULL, 0, F_GEN_NAME, F_NO_PUBLISH_EVENTS, &err);
      if (!success)
        {
          HANDLE_ERROR (err, "Failed to add region to track %s", track->name);
          return;
        }

      ArrangerObject * obj = (ArrangerObject *) ar;
      position_set_to_pos (&self->file_end_pos, &obj->end_pos);
    }
  else if (
    ((file && file->is_midi ()) || chord_pset) && self->instrument_setting)
    {
      /* create an instrument track */
      g_debug ("creating instrument track...");
      Track * instrument_track = track_new (
        TrackType::TRACK_TYPE_INSTRUMENT, self->tracklist->tracks.size (),
        "Sample processor instrument", F_WITH_LANE);
      tracklist_insert_track (
        self->tracklist, instrument_track, instrument_track->pos,
        F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);
      GError * err = NULL;
      Plugin * pl = plugin_new_from_setting (
        self->instrument_setting, track_get_name_hash (*instrument_track),
        ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT, -1, &err);
      if (!pl)
        {
          HANDLE_ERROR (
            err, _ ("Failed to create plugin %s"),
            self->instrument_setting->descr->name);
          return;
        }
      int ret = plugin_instantiate (pl, &err);
      if (ret != 0)
        {
          HANDLE_ERROR (
            err, _ ("Failed to instantiate plugin %s"),
            self->instrument_setting->descr->name);
          return;
        }
      g_return_if_fail (plugin_activate (pl, F_ACTIVATE) == 0);
      g_return_if_fail (pl->midi_in_port);
      g_return_if_fail (pl->l_out);
      g_return_if_fail (pl->r_out);

      instrument_track->channel->add_plugin (
        ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT, pl->id.slot, pl,
        F_NO_CONFIRM, F_NOT_MOVING_PLUGIN, F_GEN_AUTOMATABLES,
        F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS);

      int num_tracks = 1;
      if (file)
        {
          num_tracks = midi_file_get_num_tracks (file->abs_path.c_str (), true);
        }
      g_debug ("creating %d MIDI tracks...", num_tracks);
      for (int i = 0; i < num_tracks; i++)
        {
          char name[600];
          sprintf (name, "Sample processor MIDI %d", i);
          track = track_new (
            TrackType::TRACK_TYPE_MIDI, self->tracklist->tracks.size (), name,
            F_WITH_LANE);
          tracklist_insert_track (
            self->tracklist, track, track->pos, F_NO_PUBLISH_EVENTS,
            F_NO_RECALC_GRAPH);

          /* route track to instrument */
          group_target_track_add_child (
            instrument_track, track_get_name_hash (*track), F_CONNECT,
            F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS);

          if (file)
            {
              /* create a MIDI region from the MIDI file & add to track */
              Region * mr = midi_region_new_from_midi_file (
                &start_pos, file->abs_path.c_str (),
                track_get_name_hash (*track), 0, 0, i);
              if (mr)
                {
                  err = NULL;
                  bool success = track_add_region (
                    track, mr, NULL, 0,
                    /* name could already be generated
                     * based
                     * on the track name (if any) in
                     * the MIDI file */
                    mr->name ? F_NO_GEN_NAME : F_GEN_NAME, F_NO_PUBLISH_EVENTS,
                    &err);
                  if (success)
                    {
                      ArrangerObject * obj = (ArrangerObject *) mr;
                      if (position_is_after (&obj->end_pos, &self->file_end_pos))
                        {
                          position_set_to_pos (
                            &self->file_end_pos, &obj->end_pos);
                        }
                    }
                  else
                    {
                      HANDLE_ERROR (
                        err, "Failed to add region to track %s", track->name);
                    }
                }
              else
                {
                  g_message (
                    "Failed to create MIDI region from file %s",
                    file->abs_path.c_str ());
                }
            }
          else if (chord_pset)
            {
              /* create a MIDI region from the chord
               * preset and add to track */
              Position end_pos;
              position_from_seconds (&end_pos, 13.0);
              Region * mr = midi_region_new (
                &start_pos, &end_pos, track_get_name_hash (*track), 0, 0);

              /* add notes */
              for (int j = 0; j < 12; j++)
                {
                  ChordDescriptor * descr =
                    chord_descriptor_clone (chord_pset->descr[j]);
                  chord_descriptor_update_notes (descr);
                  if (descr->type == ChordType::CHORD_TYPE_NONE)
                    {
                      chord_descriptor_free (descr);
                      continue;
                    }

                  Position cur_pos;
                  position_from_seconds (&cur_pos, j * 1.0);
                  Position cur_end_pos;
                  position_from_seconds (&cur_end_pos, j * 1.0 + 0.5);
                  for (int k = 0; k < CHORD_DESCRIPTOR_MAX_NOTES; k++)
                    {
                      if (descr->notes[k])
                        {
                          MidiNote * mn = midi_note_new (
                            &mr->id, &cur_pos, &cur_end_pos, k + 36,
                            VELOCITY_DEFAULT);
                          midi_region_add_midi_note (
                            mr, mn, F_NO_PUBLISH_EVENTS);
                        }
                    } /* endforeach notes in chord */
                  chord_descriptor_free (descr);

                } /* endforeach chord descriptor */

              ArrangerObject * obj = (ArrangerObject *) mr;
              if (position_is_after (&obj->end_pos, &self->file_end_pos))
                {
                  position_set_to_pos (&self->file_end_pos, &obj->end_pos);
                }

              err = NULL;
              bool success = track_add_region (
                track, mr, NULL, 0, mr->name ? F_NO_GEN_NAME : F_GEN_NAME,
                F_NO_PUBLISH_EVENTS, &err);
              if (!success)
                {
                  HANDLE_ERROR_LITERAL (err, "Failed to add region to track");
                }

            } /* endif chord preset */

        } /* endforeach track */
    }

  self->roll = true;
  position_set_to_bar (&self->playhead, 1);

  /* add some room to end pos */
  char file_end_pos_str[600];
  position_to_string_full (&self->file_end_pos, file_end_pos_str, 3);
  g_message ("playing until %s", file_end_pos_str);
  position_add_bars (&self->file_end_pos, 1);

  /*
   * Create a graph so that port buffers are created (FIXME this is hacky,
   * create port buffers elsewhere).
   *
   * WARNING: this must not run while the engine is running because the engine
   * might access the port buffer created here, so we use the port operation
   * lock also used by the engine. The engine will skip its cycle until the port
   * operation lock is released.
   */
  zix_sem_wait (&AUDIO_ENGINE->port_operation_lock);
  self->graph = graph_new_full (NULL, self);
  graph_setup (self->graph, 1, 1);
  graph_free (self->graph);
  zix_sem_post (&AUDIO_ENGINE->port_operation_lock);

  zix_sem_post (&self->rebuilding_sem);
}

/**
 * Adds a file (audio or MIDI) to the queue.
 */
void
sample_processor_queue_file (SampleProcessor * self, const FileDescriptor * file)
{
  queue_file_or_chord_preset (self, file, NULL);
}

/**
 * Adds a chord preset to the queue.
 */
void
sample_processor_queue_chord_preset (
  SampleProcessor *   self,
  const ChordPreset * chord_pset)
{
  queue_file_or_chord_preset (self, NULL, chord_pset);
}

/**
 * Stops playback of files (auditioning).
 */
void
sample_processor_stop_file_playback (SampleProcessor * self)
{
  self->roll = false;
  position_set_to_bar (&self->playhead, 1);
}

void
sample_processor_disconnect (SampleProcessor * self)
{
  fader_disconnect_all (self->fader);
}

SampleProcessor *
sample_processor_clone (const SampleProcessor * src)
{
  SampleProcessor * self = object_new (SampleProcessor);

  self->fader = fader_clone (src->fader);

  return self;
}

void
sample_processor_free (SampleProcessor * self)
{
  if (self == SAMPLE_PROCESSOR)
    sample_processor_disconnect (self);

  object_free_w_func_and_null (tracklist_free, self->tracklist);
  object_free_w_func_and_null (fader_free, self->fader);
  object_free_w_func_and_null (midi_events_free, self->midi_events);

  zix_sem_destroy (&self->rebuilding_sem);

  object_zero_and_free (self);
}
