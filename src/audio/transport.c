/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/audio_region.h"
#include "audio/engine.h"
#include "audio/marker.h"
#include "audio/marker_track.h"
#include "audio/midi_event.h"
#include "audio/tempo_track.h"
#include "audio/transport.h"
#include "project.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/top_bar.h"
#include "settings/settings.h"
#include "utils/debug.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

/**
 * A buffer of n bars after the end of the last
 * object.
 */
#define BARS_END_BUFFER 4

static void
init_common (
  Transport * self)
{
  self->schema_version = TRANSPORT_SCHEMA_VERSION;

  /* set playstate */
  self->play_state = PLAYSTATE_PAUSED;

  self->loop =
    ZRYTHM_TESTING ? true :
    g_settings_get_boolean (
      S_TRANSPORT, "loop");
  self->metronome_enabled =
    ZRYTHM_TESTING ? true :
    g_settings_get_boolean (
      S_TRANSPORT, "metronome-enabled");
  self->punch_mode =
    ZRYTHM_TESTING ? true :
    g_settings_get_boolean (
      S_TRANSPORT, "punch-mode");
  self->start_playback_on_midi_input =
    ZRYTHM_TESTING ? false :
    g_settings_get_boolean (
      S_TRANSPORT, "start-on-midi-input");
  self->recording_mode =
    ZRYTHM_TESTING ? RECORDING_MODE_TAKES :
    g_settings_get_enum (
      S_TRANSPORT, "recording-mode");

  zix_sem_init (&self->paused, 0);
}

/**
 * Initialize loaded transport.
 *
 * @param engine Owner engine, if any.
 * @param tempo_track Tempo track, used to
 *   initialize the caches. Only needed on the
 *   active project transport.
 */
void
transport_init_loaded (
  Transport *   self,
  AudioEngine * engine,
  Track *       tempo_track)
{
  self->audio_engine = engine;

  z_return_if_fail_cmp (self->total_bars, >, 0);

  init_common (self);

  if (tempo_track)
    {
      int beats_per_bar =
        tempo_track_get_beats_per_bar (
          tempo_track);
      int beat_unit =
        tempo_track_get_beat_unit (
          tempo_track);
      transport_update_caches (
        self, beats_per_bar, beat_unit);
    }

#define INIT_LOADED_PORT(x) \
  port_init_loaded (self->x, self)

  INIT_LOADED_PORT (roll);
  INIT_LOADED_PORT (stop);
  INIT_LOADED_PORT (backward);
  INIT_LOADED_PORT (forward);
  INIT_LOADED_PORT (loop_toggle);
  INIT_LOADED_PORT (rec_toggle);

#undef INIT_LOADED_PORT
}

/**
 * Create a new transport.
 */
Transport *
transport_new (
  AudioEngine * engine)
{
  g_message (
    "%s: Creating transport...", __func__);

  Transport * self = object_new (Transport);
  self->audio_engine = engine;
  self->schema_version =
    TRANSPORT_SCHEMA_VERSION;

  if (engine)
    {
      engine->transport = self;
    }

  // set initial total number of beats
  // this is applied to the ruler
  self->total_bars =
    TRANSPORT_DEFAULT_TOTAL_BARS;

  g_return_val_if_fail (
    engine->sample_rate > 0, NULL);

  /* hack to allow setting positions */
  /*double frames_per_tick_before =*/
    /*AUDIO_ENGINE->frames_per_tick;*/
  /*if (math_doubles_equal (*/
        /*frames_per_tick_before, 0))*/
    /*{*/
      /*AUDIO_ENGINE->frames_per_tick = 512;*/
    /*}*/

  /* set positions */
  position_init (&self->playhead_pos);
  position_init (&self->cue_pos);
  position_init (&self->loop_start_pos);
  position_init (&self->loop_end_pos);
  position_init (&self->punch_in_pos);
  position_init (&self->punch_out_pos);
  position_init (&self->range_1);
  position_init (&self->range_2);

  double ticks_per_bar =
    TICKS_PER_QUARTER_NOTE * 4.0;
  self->loop_end_pos.ticks =
    4 * ticks_per_bar;
  self->punch_in_pos.ticks =
    2 * ticks_per_bar;
  self->punch_out_pos.ticks =
    4 * ticks_per_bar;

  self->range_1.ticks = 1 * ticks_per_bar;
  self->range_2.ticks = 1 * ticks_per_bar;

  /*if (math_doubles_equal (*/
        /*frames_per_tick_before, 0))*/
    /*{*/
      /*AUDIO_ENGINE->frames_per_tick = 0;*/
    /*}*/

  /* create ports */
  self->roll =
    port_new_with_type (
      TYPE_EVENT, FLOW_INPUT, "Roll");
  self->roll->id.sym = g_strdup ("roll");
  port_set_owner (
    self->roll, PORT_OWNER_TYPE_TRANSPORT, self);
  self->roll->id.flags |= PORT_FLAG_TOGGLE;
  self->roll->id.flags2 |=
    PORT_FLAG2_TRANSPORT_ROLL;

  self->stop =
    port_new_with_type (
      TYPE_EVENT, FLOW_INPUT, "Stop");
  self->stop->id.sym = g_strdup ("stop");
  port_set_owner (
    self->stop, PORT_OWNER_TYPE_TRANSPORT, self);
  self->stop->id.flags |= PORT_FLAG_TOGGLE;
  self->stop->id.flags2 |=
    PORT_FLAG2_TRANSPORT_STOP;

  self->backward =
    port_new_with_type (
      TYPE_EVENT, FLOW_INPUT, "Backward");
  self->backward->id.sym = g_strdup ("backward");
  port_set_owner (
    self->backward, PORT_OWNER_TYPE_TRANSPORT,
    self);
  self->backward->id.flags |= PORT_FLAG_TOGGLE;
  self->backward->id.flags2 |=
    PORT_FLAG2_TRANSPORT_BACKWARD;

  self->forward =
    port_new_with_type (
      TYPE_EVENT, FLOW_INPUT, "Forward");
  self->forward->id.sym = g_strdup ("forward");
  port_set_owner (
    self->forward, PORT_OWNER_TYPE_TRANSPORT,
    self);
  self->forward->id.flags |= PORT_FLAG_TOGGLE;
  self->forward->id.flags2 |=
    PORT_FLAG2_TRANSPORT_FORWARD;

  self->loop_toggle =
    port_new_with_type (
      TYPE_EVENT, FLOW_INPUT, "Loop toggle");
  self->loop_toggle->id.sym =
    g_strdup ("loop_toggle");
  port_set_owner (
    self->loop_toggle, PORT_OWNER_TYPE_TRANSPORT,
    self);
  self->loop_toggle->id.flags |= PORT_FLAG_TOGGLE;
  self->loop_toggle->id.flags2 |=
    PORT_FLAG2_TRANSPORT_LOOP_TOGGLE;

  self->rec_toggle =
    port_new_with_type (
      TYPE_EVENT, FLOW_INPUT, "Rec toggle");
  self->rec_toggle->id.sym =
    g_strdup ("rec_toggle");
  port_set_owner (
    self->rec_toggle, PORT_OWNER_TYPE_TRANSPORT,
    self);
  self->rec_toggle->id.flags |= PORT_FLAG_TOGGLE;
  self->rec_toggle->id.flags2 |=
    PORT_FLAG2_TRANSPORT_REC_TOGGLE;

  init_common (self);

  return self;
}

/**
 * Clones the transport values.
 */
Transport *
transport_clone (
  const Transport * src)
{
  Transport * self =
    object_new (Transport);
  self->schema_version =
    TRANSPORT_SCHEMA_VERSION;

  self->total_bars = src->total_bars;
  self->has_range = src->has_range;
  self->position = src->position;

  position_set_to_pos (
    &self->loop_start_pos, &src->loop_start_pos);
  position_set_to_pos (
    &self->playhead_pos, &src->playhead_pos);
  position_set_to_pos (
    &self->loop_end_pos, &src->loop_end_pos);
  position_set_to_pos (
    &self->cue_pos, &src->cue_pos);
  position_set_to_pos (
    &self->punch_in_pos, &src->punch_in_pos);
  position_set_to_pos (
    &self->punch_out_pos, &src->punch_out_pos);
  position_set_to_pos (
    &self->range_1, &src->range_1);
  position_set_to_pos (
    &self->range_2, &src->range_2);

#define CLONE_PORT_IF_EXISTS(x) \
  if (src->x) \
    self->x = port_clone (src->x)

  CLONE_PORT_IF_EXISTS (roll);
  CLONE_PORT_IF_EXISTS (stop);
  CLONE_PORT_IF_EXISTS (backward);
  CLONE_PORT_IF_EXISTS (forward);
  CLONE_PORT_IF_EXISTS (loop_toggle);
  CLONE_PORT_IF_EXISTS (rec_toggle);

#undef CLONE_PORT_IF_EXISTS

  return self;
}

/**
 * Prepares audio regions for stretching (sets the
 * \ref ZRegion.before_length).
 *
 * @param selections If NULL, all audio regions
 *   are used. If non-NULL, only the regions in the
 *   selections are used.
 */
void
transport_prepare_audio_regions_for_stretch (
  Transport *          self,
  TimelineSelections * sel)
{
  if (sel)
    {
      for (int i = 0; i < sel->num_regions; i++)
        {
          ZRegion * region = sel->regions[i];
          region->before_length =
            arranger_object_get_length_in_ticks (
              (ArrangerObject *) region);
        }
    }
  else
    {
      for (int i = 0; i < TRACKLIST->num_tracks; i++)
        {
          Track * track = TRACKLIST->tracks[i];

          if (track->type != TRACK_TYPE_AUDIO)
            continue;

          for (int j = 0; j < track->num_lanes; j++)
            {
              TrackLane * lane = track->lanes[j];

              for (int k = 0; k < lane->num_regions;
                   k++)
                {
                  ZRegion * region =
                    lane->regions[k];
                  region->before_length =
                    arranger_object_get_length_in_ticks (
                      (ArrangerObject *) region);
                } // foreach region
            } // foreach lane
        } // foreach track
    }
}

/**
 * Stretches audio regions.
 *
 * @param selections If NULL, all audio regions
 *   are used. If non-NULL, only the regions in the
 *   selections are used.
 * @param with_fixed_ratio Stretch all regions with
 *   a fixed ratio. If this is off, the current
 *   region length and \ref ZRegion.before_length
 *   will be used to calculate the ratio.
 */
void
transport_stretch_audio_regions (
  Transport *          self,
  TimelineSelections * sel,
  bool                 with_fixed_ratio,
  double               time_ratio)
{
  if (sel)
    {
      for (int i = 0; i < sel->num_regions; i++)
        {
          ZRegion * region =
            TL_SELECTIONS->regions[i];

          /* don't stretch audio regions with
           * musical mode off */
          if (region->id.type == REGION_TYPE_AUDIO
              && !region_get_musical_mode (region))
            continue;

          ArrangerObject * r_obj =
            (ArrangerObject *) region;
          double ratio =
            with_fixed_ratio ? time_ratio :
            arranger_object_get_length_in_ticks (
              r_obj) /
            region->before_length;
          region_stretch (region, ratio);
        }
    }
  else
    {
      for (int i = 0; i < TRACKLIST->num_tracks; i++)
        {
          Track * track = TRACKLIST->tracks[i];

          if (track->type != TRACK_TYPE_AUDIO)
            continue;

          for (int j = 0; j < track->num_lanes; j++)
            {
              TrackLane * lane = track->lanes[j];

              for (int k = 0; k < lane->num_regions;
                   k++)
                {
                  ZRegion * region =
                    lane->regions[k];

                  /* don't stretch regions with
                   * musical mode off */
                  if (!region_get_musical_mode (
                         region))
                    continue;

                  ArrangerObject * r_obj =
                    (ArrangerObject *) region;
                  double ratio =
                    with_fixed_ratio ? time_ratio :
                    arranger_object_get_length_in_ticks (
                      r_obj) /
                    region->before_length;
                  region_stretch (region, ratio);
                }
            }
        }
    }
}

void
transport_set_punch_mode_enabled (
  Transport * self,
  bool        enabled)
{
  self->punch_mode = enabled;

  if (!ZRYTHM_TESTING)
    {
      g_settings_set_boolean (
        S_TRANSPORT, "punch-mode", enabled);
    }
}

void
transport_set_start_playback_on_midi_input (
  Transport * self,
  bool        enabled)
{
  self->start_playback_on_midi_input = enabled;
  g_settings_set_boolean (
    S_TRANSPORT, "start-on-midi-input", enabled);
}

void
transport_set_recording_mode (
  Transport * self,
  TransportRecordingMode mode)
{
  self->recording_mode = mode;

  if (!ZRYTHM_TESTING)
    {
      g_settings_set_enum (
        S_TRANSPORT, "recording-mode", mode);
    }
}

/**
 * Updates beat unit and anything depending on it.
 */
void
transport_update_caches (
  Transport * self,
  int         beats_per_bar,
  int         beat_unit)
{
  /**
   * Regarding calculation:
   * 3840 = TICKS_PER_QUARTER_NOTE * 4 to get the ticks
   * per full note.
   * Divide by beat unit (e.g. if beat unit is 2,
   * it means it is a 1/2th note, so multiply 1/2
   * with the ticks per note
   */
  self->ticks_per_beat =
    3840 / beat_unit;
  self->ticks_per_bar =
    self->ticks_per_beat * beats_per_bar;
  self->sixteenths_per_beat = 16 / beat_unit;
  self->sixteenths_per_bar =
    (self->sixteenths_per_beat * beats_per_bar);
  g_warn_if_fail (self->ticks_per_bar > 0.0);
  g_warn_if_fail (self->ticks_per_beat > 0.0);
}

void
transport_request_pause (
  Transport * self)
{
  self->play_state = PLAYSTATE_PAUSE_REQUESTED;

  TRANSPORT->playhead_before_pause =
    self->playhead_pos;
  if (!ZRYTHM_TESTING &&
      g_settings_get_boolean (
        S_TRANSPORT, "return-to-cue"))
    {
      transport_move_playhead (
        self, &self->cue_pos, F_PANIC,
        F_NO_SET_CUE_POINT, F_PUBLISH_EVENTS);
    }
}

void
transport_request_roll (
  Transport * self)
{
  if (!ZRYTHM_TESTING)
    {
      /* handle countin */
      PrerollCountBars bars =
        g_settings_get_enum (
          S_TRANSPORT, "metronome-countin");
      int num_bars =
        transport_preroll_count_bars_enum_to_int (
          bars);
      double frames_per_bar =
        AUDIO_ENGINE->frames_per_tick *
        (double) TRANSPORT->ticks_per_bar;
      self->countin_frames_remaining =
        (long) ((double) num_bars * frames_per_bar);
      if (self->metronome_enabled)
        {
          sample_processor_queue_metronome_countin (
            SAMPLE_PROCESSOR);
        }

      if (self->recording)
        {
          /* handle preroll */
          bars =
            g_settings_get_enum (
              S_TRANSPORT, "recording-preroll");
          num_bars =
            transport_preroll_count_bars_enum_to_int (
              bars);
          Position pos;
          position_set_to_pos (
            &pos, &self->playhead_pos);
          position_add_bars (&pos, - num_bars);
          position_print (&pos);
          if (pos.frames < 0)
            position_init (&pos);
          self->preroll_frames_remaining =
            self->playhead_pos.frames - pos.frames;
          transport_set_playhead_pos (self, &pos);
#if 0
          g_debug (
            "preroll %ld frames",
            self->preroll_frames_remaining);
#endif
        }
    }

  self->play_state = PLAYSTATE_ROLL_REQUESTED;
}


/**
 * Moves the playhead by the time corresponding to
 * given samples, taking into account the loop
 * end point.
 */
void
transport_add_to_playhead (
  Transport *          self,
  const signed_frame_t nframes)
{
  transport_position_add_frames (
    self, &TRANSPORT->playhead_pos, nframes);
  EVENTS_PUSH (ET_PLAYHEAD_POS_CHANGED, NULL);
}

/**
 * Setter for playhead Position.
 */
void
transport_set_playhead_pos (
  Transport * self,
  Position *  pos)
{
  position_set_to_pos (
    &self->playhead_pos, pos);
  EVENTS_PUSH (
    ET_PLAYHEAD_POS_CHANGED_MANUALLY, NULL);
}

void
transport_set_playhead_to_bar (
  Transport * self,
  int         bar)
{
  Position pos;
  position_set_to_bar (&pos, bar);
  transport_set_playhead_pos (self, &pos);
}

/**
 * Getter for playhead Position.
 */
void
transport_get_playhead_pos (
  Transport * self,
  Position *  pos)
{
  g_return_if_fail (self && pos);

  position_set_to_pos (
    pos, &self->playhead_pos);
}

/**
 * Moves playhead to given pos.
 *
 * This is only for moves other than while playing
 * and for looping while playing.
 *
 * @param target Position to set to.
 * @param panic Send MIDI panic or not.
 * @param set_cue_point Also set the cue point at
 *   this position.
 */
void
transport_move_playhead (
  Transport * self,
  Position *  target,
  bool        panic,
  bool        set_cue_point,
  bool        fire_events)
{
  int i, j, k, l;
  /* send MIDI note off on currently playing timeline
   * objects */
  Track * track;
  ZRegion * region;
  MidiNote * midi_note;
  MidiEvents * midi_events;
  TrackLane * lane;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      for (k = 0; k < track->num_lanes; k++)
        {
          lane = track->lanes[k];

          for (l = 0;
               l < lane->num_regions; l++)
            {
              region = lane->regions[l];

              if (!region_is_hit (
                    region, PLAYHEAD->frames, 1))
                continue;

              for (j = 0;
                   j < region->num_midi_notes;
                   j++)
                {
                  midi_note = region->midi_notes[j];

                  if (midi_note_hit (
                        midi_note, PLAYHEAD->frames))
                    {
                      midi_events =
                        track->processor->
                          piano_roll->
                            midi_events;

                      zix_sem_wait (
                        &midi_events->access_sem);
                      midi_events_add_note_off (
                        midi_events, 1,
                        midi_note->val,
                        0, 1);
                      zix_sem_post (
                        &midi_events->access_sem);
                    }
                }
            }
        }
    }

  /* move to new pos */
  position_set_to_pos (&self->playhead_pos, target);

  if (set_cue_point)
    {
      /* move cue point */
      position_set_to_pos (&self->cue_pos, target);
    }

  if (fire_events)
    {
      /* FIXME use another flag to decide when
       * to do this */
      self->last_manual_playhead_change =
        g_get_monotonic_time ();

      EVENTS_PUSH (
        ET_PLAYHEAD_POS_CHANGED_MANUALLY, NULL);
    }
}

/**
 * Sets whether metronome is enabled or not.
 */
void
transport_set_metronome_enabled (
  Transport * self,
  const int   enabled)
{
  self->metronome_enabled = enabled;
  g_settings_set_boolean (
    S_TRANSPORT, "metronome-enabled", enabled);
}

/**
 * Returns the PPQN (Parts/Ticks Per Quarter Note).
 */
double
transport_get_ppqn (
  Transport * self)
{
  int beat_unit =
    tempo_track_get_beat_unit (P_TEMPO_TRACK);
  double res =
    self->ticks_per_beat *
    ((double) beat_unit / 4.0);
  return res;
}

/**
 * Updates the frames in all transport positions
 *
 * @param update_from_ticks Whether to update the
 *   positions based on ticks (true) or frames
 *   (false).
 */
void
transport_update_positions (
  Transport * self,
  bool        update_from_ticks)
{
  position_update (
    &self->playhead_pos, update_from_ticks);
  position_update (
    &self->cue_pos, update_from_ticks);
  position_update (
    &self->loop_start_pos, update_from_ticks);
  position_update (
    &self->loop_end_pos, update_from_ticks);
  position_update (
    &self->punch_in_pos, update_from_ticks);
  position_update (
    &self->punch_out_pos, update_from_ticks);
}

#define GATHER_MARKERS \
  /* gather all markers */ \
  Position markers[60]; \
  int num_markers = 0, i; \
  for (i = 0; \
       i < P_MARKER_TRACK->num_markers; i++) \
    { \
      ArrangerObject * m_obj = \
        (ArrangerObject *) \
        P_MARKER_TRACK->markers[i]; \
       position_set_to_pos ( \
          &markers[num_markers++], \
          &m_obj->pos); \
    } \
  position_set_to_pos ( \
    &markers[num_markers++], \
    &self->cue_pos); \
  position_set_to_pos ( \
    &markers[num_markers++], \
    &self->loop_start_pos); \
  position_set_to_pos ( \
    &markers[num_markers++], \
    &self->loop_end_pos); \
  position_set_to_pos ( \
    &markers[num_markers++], \
    &POSITION_START); \
  position_sort_array ( \
    markers, (size_t) num_markers)

/**
 * Moves the playhead to the prev Marker.
 */
void
transport_goto_prev_marker (
  Transport * self)
{
  GATHER_MARKERS;

  for (i = num_markers - 1; i >= 0; i--)
    {
      if (position_is_before (
            &markers[i], &self->playhead_pos) &&
          TRANSPORT_IS_ROLLING &&
          i > 0 &&
          (position_to_ms (&self->playhead_pos) -
             position_to_ms (&markers[i])) <
           180)
        {
          transport_move_playhead (
            self, &markers[i - 1], F_PANIC,
            F_SET_CUE_POINT,
            F_PUBLISH_EVENTS);
          break;
        }
      else if (
        position_is_before (
          &markers[i], &self->playhead_pos))
        {
          transport_move_playhead (
            self, &markers[i],
            F_PANIC, F_SET_CUE_POINT,
            F_PUBLISH_EVENTS);
          break;
        }
    }
}

/**
 * Moves the playhead to the next Marker.
 */
void
transport_goto_next_marker (
  Transport * self)
{
  GATHER_MARKERS;

  for (i = 0; i < num_markers; i++)
    {
      if (position_is_after (
            &markers[i], &self->playhead_pos))
        {
          transport_move_playhead (
            self, &markers[i], F_PANIC,
            F_SET_CUE_POINT, F_PUBLISH_EVENTS);
          break;
        }
    }
}

/**
 * Enables or disables loop.
 */
void
transport_set_loop (
  Transport * self,
  bool        enabled)
{
  self->loop = enabled;

  if (!ZRYTHM_TESTING)
    {
      g_settings_set_boolean (
        S_TRANSPORT, "loop", enabled);
    }

  EVENTS_PUSH (ET_LOOP_TOGGLED, NULL);
}

#if 0
/**
 * Adds frames to the given global frames, while
 * adjusting the new frames to loop back if the
 * loop point was crossed.
 *
 * @return The new frames adjusted.
 */
long
transport_frames_add_frames (
  const Transport * self,
  const long        gframes,
  const nframes_t   frames)
{
  long new_frames = gframes + (long) frames;

  /* if start frames were before the loop-end point
   * and the new frames are after (loop crossed) */
  if (TRANSPORT_IS_LOOPING &&
      gframes < self->loop_end_pos.frames &&
      new_frames >= self->loop_end_pos.frames)
    {
      /* adjust the new frames */
      new_frames +=
        self->loop_start_pos.frames -
        self->loop_end_pos.frames;
    }

  return new_frames;
}
#endif

/**
 * Adds frames to the given position similar to
 * position_add_frames(), except that it adjusts
 * the new Position to loop back if the loop end
 * point was crossed.
 */
void
transport_position_add_frames (
  const Transport * self,
  Position *        pos,
  const long        frames)
{
  Position pos_before_adding = *pos;
  position_add_frames (pos, frames);

  /* if start frames were before the loop-end point
   * and the new frames are after (loop crossed) */
  if (TRANSPORT_IS_LOOPING &&
      pos_before_adding.frames <
        self->loop_end_pos.frames &&
      pos->frames >=
        self->loop_end_pos.frames)
    {
      /* adjust the new frames */
      position_add_ticks (
        pos,
        self->loop_start_pos.ticks -
          self->loop_end_pos.ticks);

      g_warn_if_fail (
        pos->frames < self->loop_end_pos.frames);
    }

  /*long new_global_frames =*/
    /*transport_frames_add_frames (*/
      /*self, pos->frames, frames);*/
  /*position_from_frames (*/
    /*pos, new_global_frames);*/

  /* set the frames manually again because
   * position_from_frames rounds them */
  /*pos->frames = new_global_frames;*/
}

/**
 * Sets if the project has range and updates UI.
 */
void
transport_set_has_range (
  Transport * self,
  bool        has_range)
{
  self->has_range = has_range;

  EVENTS_PUSH (ET_RANGE_SELECTION_CHANGED, NULL);
}

/**
 * Stores the position of the range in \ref pos.
 */
void
transport_get_range_pos (
  Transport * self,
  bool        first,
  Position *  pos)
{
  bool range1_first =
    position_is_before_or_equal (
      &self->range_1, &self->range_2);

  if (first)
    {
      if (range1_first)
        {
          position_set_to_pos (pos, &self->range_1);
        }
      else
        {
          position_set_to_pos (pos, &self->range_2);
        }
    }
  else
    {
      if (range1_first)
        {
          position_set_to_pos (pos, &self->range_2);
        }
      else
        {
          position_set_to_pos (pos, &self->range_1);
        }
    }
}

/**
 * Set the range1 or range2 position.
 *
 * @param range1 True to set range1, false to set
 *   range2.
 */
void
transport_set_range (
  Transport *      self,
  bool             range1,
  const Position * start_pos,
  const Position * pos,
  bool             snap)
{
  Position * pos_to_set =
    range1 ? &self->range_1 : &self->range_2;

  Position init_pos;
  position_init (&init_pos);
  if (position_is_before (pos, &init_pos))
    {
      position_set_to_pos (
        pos_to_set, &init_pos);
    }
  else
    {
      position_set_to_pos (
        pos_to_set, pos);
    }

  if (snap)
    {
      position_snap (
        start_pos, pos_to_set, NULL, NULL,
        SNAP_GRID_TIMELINE);
    }
}

bool
transport_position_is_inside_punch_range (
  Transport * self,
  Position *  pos)
{
  return
    pos->frames >= self->punch_in_pos.frames &&
    pos->frames < self->punch_out_pos.frames;
}

/**
 * Recalculates the total bars based on the last
 * object's position.
 *
 * @param sel If given, only these objects will
 *   be checked, otherwise every object in the
 *   project will be checked.
 */
void
transport_recalculate_total_bars (
  Transport *          self,
  ArrangerSelections * sel)
{
  if (!ZRYTHM_HAVE_UI)
    return;

  int total_bars = self->total_bars;
  if (sel)
    {
      int num_objs;
      ArrangerObject ** objs =
        arranger_selections_get_all_objects (
          sel, &num_objs);
      for (int i = 0; i < num_objs; i++)
        {
          ArrangerObject * obj = objs[i];
          Position pos;
          if (arranger_object_type_has_length (
                obj->type))
            {
              arranger_object_get_end_pos (
                obj, &pos);
            }
          else
            {
              arranger_object_get_pos (obj, &pos);
            }
          int pos_bars =
            position_get_total_bars (&pos, true);
          if (pos_bars > total_bars - 3)
            {
              total_bars =
                pos_bars + BARS_END_BUFFER;
            }
        }
    }
  /* else no selections, calculate total bars for
   * every object */
  else
    {
      total_bars = TRANSPORT_DEFAULT_TOTAL_BARS;

      tracklist_get_total_bars (
        TRACKLIST, &total_bars);

      total_bars += BARS_END_BUFFER;
    }

  transport_update_total_bars (
    self, total_bars, F_PUBLISH_EVENTS);
}

/**
 * Updates the total bars.
 */
void
transport_update_total_bars (
  Transport * self,
  int         total_bars,
  bool        fire_events)
{
  g_return_if_fail (
    self &&
    total_bars >= TRANSPORT_DEFAULT_TOTAL_BARS);

  if (self->total_bars == total_bars)
    return;

  self->total_bars = total_bars;

  if (fire_events)
    {
      EVENTS_PUSH (
        ET_TRANSPORT_TOTAL_BARS_CHANGED, NULL);
    }
}

/**
 * Move to the previous snap point on the timeline.
 */
void
transport_move_backward (
  Transport * self)
{
  Position pos;
  bool ret =
    snap_grid_get_nearby_snap_point (
      &pos, SNAP_GRID_TIMELINE,
      &self->playhead_pos, true);
  g_return_if_fail (ret);
  transport_move_playhead (
    self, &pos, F_PANIC, F_SET_CUE_POINT,
    F_PUBLISH_EVENTS);
}

/**
 * Move to the next snap point on the timeline.
 */
void
transport_move_forward (
  Transport * self)
{
  Position pos;
  bool ret =
    snap_grid_get_nearby_snap_point (
      &pos, SNAP_GRID_TIMELINE,
      &self->playhead_pos, false);
  g_return_if_fail (ret);
  transport_move_playhead (
    self, &pos, F_PANIC, F_SET_CUE_POINT,
    F_PUBLISH_EVENTS);
}

/**
 * Sets recording on/off.
 */
void
transport_set_recording (
  Transport * self,
  bool        record,
  bool        fire_events)
{
  self->recording = record;

  if (fire_events)
    {
      EVENTS_PUSH (
        ET_TRANSPORT_RECORDING_ON_OFF_CHANGED, NULL);
    }
}

void
transport_free (
  Transport * self)
{
  zix_sem_destroy (&self->paused);

  object_zero_and_free (self);
}
