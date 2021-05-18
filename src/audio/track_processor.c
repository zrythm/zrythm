/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/audio_track.h"
#include "audio/channel.h"
#include "audio/clip.h"
#include "audio/control_port.h"
#include "audio/control_room.h"
#include "audio/engine.h"
#include "audio/fader.h"
#include "audio/midi_event.h"
#include "audio/midi_track.h"
#include "audio/recording_manager.h"
#include "audio/track.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/dsp.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "zrythm.h"

#include <glib/gi18n.h>

/**
 * Inits fader after a project is loaded.
 */
void
track_processor_init_loaded (
  TrackProcessor * self,
  bool             is_project)
{
  self->magic = TRACK_PROCESSOR_MAGIC;

  size_t max_size = 20;
  Port ** ports =
    object_new_n (max_size, Port *);
  int num_ports = 0;
  track_processor_append_ports (
    self, &ports, &num_ports, true, &max_size);
  for (int i = 0; i < num_ports; i++)
    {
      Port * port = ports[i];
      port->magic = PORT_MAGIC;
      port->is_project = is_project;
      port_set_owner_track_processor (port, self);
    }
  free (ports);

  track_processor_set_is_project (self, is_project);
}

/**
 * Inits the MIDI In port of the Channel while
 * exposing it to JACK.
 *
 * This assumes the caller already checked that
 * this channel should have the given MIDI port
 * enabled.
 *
 * @param in 1 for input, 0 for output.
 * @param loading 1 if loading a channel, 0 if
 *   new.
 */
static void
init_midi_port (
  TrackProcessor * self,
  int              in)
{
  if (in)
    {
      self->midi_in =
        port_new_with_type (
          TYPE_EVENT, FLOW_INPUT, "TP MIDI in");
      g_warn_if_fail (IS_PORT (self->midi_in));
      port_set_owner_track_processor (
        self->midi_in, self);
      self->midi_in->id.flags |= PORT_FLAG_SEND_RECEIVABLE;
    }
  else
    {
      self->midi_out =
        port_new_with_type (
          TYPE_EVENT, FLOW_OUTPUT, "TP MIDI out");
      g_warn_if_fail (IS_PORT (self->midi_out));
      port_set_owner_track_processor (
        self->midi_out, self);
    }
}

static void
init_midi_cc_ports (
  TrackProcessor * self,
  int              loading)
{
#define INIT_MIDI_PORT(x,idx) \
  x->id.flags |= PORT_FLAG_MIDI_AUTOMATABLE; \
  x->id.flags |= PORT_FLAG_AUTOMATABLE; \
  x->id.port_index = idx; \
  port_set_owner_track_processor (x, self)

  char name[400];
  for (int i = 0; i < 16; i++)
    {
      /* starting from 1 */
      int channel = i + 1;

      for (int j = 0; j < 128; j++)
        {
          sprintf (
            name, "Ch%d %s", channel,
            midi_get_cc_name (j));
          Port * cc =
            port_new_with_type (
              TYPE_CONTROL, FLOW_INPUT, name);
          INIT_MIDI_PORT (cc, i * 128 + j);
          self->midi_cc[i * 128 + j] = cc;
        }

      sprintf (
        name, "Ch%d Pitch bend", i + 1);
      Port * cc =
        port_new_with_type (
          TYPE_CONTROL, FLOW_INPUT, name);
      INIT_MIDI_PORT (cc, i);
      cc->maxf = 8191.f;
      cc->minf = -8192.f;
      cc->deff = 0.f;
      cc->zerof = 0.f;
      cc->id.flags2 |= PORT_FLAG2_MIDI_PITCH_BEND;
      self->pitch_bend[i] = cc;

      sprintf (
        name, "Ch%d Poly key pressure", i + 1);
      cc =
        port_new_with_type (
          TYPE_CONTROL, FLOW_INPUT, name);
      INIT_MIDI_PORT (cc, i);
      cc->id.flags2 |=
        PORT_FLAG2_MIDI_POLY_KEY_PRESSURE;
      self->poly_key_pressure[i] = cc;

      sprintf (
        name, "Ch%d Channel pressure", i + 1);
      cc =
        port_new_with_type (
          TYPE_CONTROL, FLOW_INPUT, name);
      INIT_MIDI_PORT (cc, i);
      cc->id.flags2 |=
        PORT_FLAG2_MIDI_CHANNEL_PRESSURE;
      self->channel_pressure[i] = cc;
    }

#undef INIT_MIDI_PORT
}

/**
 * Inits the stereo ports of the Channel while
 * exposing them to the backend.
 *
 * This assumes the caller already checked that
 * this channel should have the given ports
 * enabled.
 *
 * @param in 1 for input, 0 for output.
 * @param loading 1 if loading a channel, 0 if
 *   new.
 */
static void
init_stereo_out_ports (
  TrackProcessor * self,
  bool             in)
{
  Port * l, * r;
  StereoPorts ** sp =
    in ? &self->stereo_in : &self->stereo_out;
  PortFlow flow = in ? FLOW_INPUT : FLOW_OUTPUT;

  l =
    port_new_with_type (
      TYPE_AUDIO, flow,
      in ?
        "TP Stereo in L" :
        "TP Stereo out L");

  r =
    port_new_with_type (
      TYPE_AUDIO, flow,
      in ?
        "TP Stereo in R" :
        "TP Stereo out R");

  port_set_owner_track_processor (l, self);
  port_set_owner_track_processor (r, self);

  *sp = stereo_ports_new_from_existing (l, r);

  if (in)
    {
      l->id.flags |= PORT_FLAG_SEND_RECEIVABLE;
      r->id.flags |= PORT_FLAG_SEND_RECEIVABLE;
    }
}

/**
 * Creates a new track processor for the given
 * track.
 */
TrackProcessor *
track_processor_new (
  Track *          tr)
{
  TrackProcessor * self =
    object_new (TrackProcessor);

  self->schema_version =
    TRACK_PROCESSOR_SCHEMA_VERSION;
  self->magic = TRACK_PROCESSOR_MAGIC;
  self->track_pos = tr->pos;
  self->track = tr;
  self->l_port_db = 0.f;
  self->r_port_db = 0.f;

  switch  (tr->in_signal_type)
    {
    case TYPE_EVENT:
      init_midi_port (self, 0);
      init_midi_port (self, 1);

      /* set up piano roll port */
      if (track_type_has_piano_roll (tr->type) ||
          tr->type == TRACK_TYPE_CHORD)
        {
          self->piano_roll =
            port_new_with_type (
              TYPE_EVENT, FLOW_INPUT,
              "TP Piano Roll");
          self->piano_roll->id.flags =
            PORT_FLAG_PIANO_ROLL;
          port_set_owner_track_processor (
            self->piano_roll, self);
          if (tr->type != TRACK_TYPE_CHORD)
            {
              init_midi_cc_ports (self, false);
            }
        }
      break;
    case TYPE_AUDIO:
      init_stereo_out_ports (self, false);
      init_stereo_out_ports (self, true);
      if (tr->type == TRACK_TYPE_AUDIO)
        {
          self->mono =
            port_new_with_type (
              TYPE_CONTROL, FLOW_INPUT,
              "TP Mono Toggle");
          port_set_owner_track_processor (
            self->mono, self);
          self->mono->id.flags |= PORT_FLAG_TOGGLE;
          self->mono->id.flags |= PORT_FLAG_TP_MONO;
          self->input_gain =
            port_new_with_type (
              TYPE_CONTROL, FLOW_INPUT,
              "TP Input Gain");
          self->input_gain->minf = 0.f;
          self->input_gain->maxf = 4.f;
          self->input_gain->zerof = 0.f;
          self->input_gain->deff = 1.f;
          self->input_gain->id.flags |=
            PORT_FLAG_TP_INPUT_GAIN;
          port_set_control_value (
            self->input_gain, 1.f, F_NOT_NORMALIZED,
            F_NO_PUBLISH_EVENTS);
          port_set_owner_track_processor (
            self->input_gain, self);
        }
      break;
    default:
      break;
    }

  if (tr->type == TRACK_TYPE_AUDIO)
    {
      self->output_gain =
        port_new_with_type (
          TYPE_CONTROL, FLOW_INPUT,
          "TP Output Gain");
      self->output_gain->minf = 0.f;
      self->output_gain->maxf = 4.f;
      self->output_gain->zerof = 0.f;
      self->output_gain->deff = 1.f;
      self->output_gain->id.flags2 |=
        PORT_FLAG2_TP_OUTPUT_GAIN;
      port_set_control_value (
        self->output_gain, 1.f, F_NOT_NORMALIZED,
        F_NO_PUBLISH_EVENTS);
      port_set_owner_track_processor (
        self->output_gain, self);
    }

  return self;
}

void
track_processor_append_ports (
  TrackProcessor * self,
  Port ***         ports,
  int *            size,
  bool             is_dynamic,
  size_t *         max_size)
{
#define _ADD(port) \
  if (is_dynamic) \
    { \
      array_double_size_if_full ( \
        *ports, (*size), (*max_size), Port *); \
    } \
  else if ((size_t) *size == *max_size) \
    { \
      g_return_if_reached (); \
    } \
  g_warn_if_fail (port); \
  array_append ( \
    *ports, (*size), port)

  if (self->stereo_in)
    {
      _ADD (self->stereo_in->l);
      _ADD (self->stereo_in->r);
    }
  if (self->mono)
    {
      _ADD (self->mono);
    }
  if (self->input_gain)
    {
      _ADD (self->input_gain);
    }
  if (self->output_gain)
    {
      _ADD (self->output_gain);
    }
  if (self->stereo_out)
    {
      _ADD (self->stereo_out->l);
      _ADD (self->stereo_out->r);
    }
  if (self->midi_in)
    {
      _ADD (self->midi_in);
    }
  if (self->midi_out)
    {
      _ADD (self->midi_out);
    }
  if (self->piano_roll)
    {
      _ADD (self->piano_roll);
    }
  for (int i = 0; i < 16; i++)
    {
      for (int j = 0; j < 128; j++)
        {
          if (self->midi_cc[i * 128 + j])
            {
              _ADD (self->midi_cc[i * 128 + j]);
            }
        }
      if (self->pitch_bend[i])
        {
          _ADD (self->pitch_bend[i]);
        }
      if (self->poly_key_pressure[i])
        {
          _ADD (self->poly_key_pressure[i]);
        }
      if (self->channel_pressure[i])
        {
          _ADD (self->channel_pressure[i]);
        }
    }
}

void
track_processor_set_is_project (
  TrackProcessor * self,
  bool             is_project)
{
  self->is_project = is_project;

  size_t max_size = 20;
  Port ** ports =
    object_new_n (max_size, Port *);
  int num_ports = 0;
  track_processor_append_ports (
    self, &ports, &num_ports, true, &max_size);
  for (int i = 0; i < num_ports; i++)
    {
      Port * port = ports[i];
      port_set_is_project (port, is_project);
    }
  free (ports);
}

/**
 * Clears all buffers.
 */
void
track_processor_clear_buffers (
  TrackProcessor * self)
{
  /*Track * track =*/
    /*track_processor_get_track (self);*/

  if (self->stereo_in)
    {
      port_clear_buffer (self->stereo_in->l);
      port_clear_buffer (self->stereo_in->r);
    }
  if (self->stereo_out)
    {
      port_clear_buffer (self->stereo_out->l);
      port_clear_buffer (self->stereo_out->r);
    }
  if (self->midi_in)
    {
      port_clear_buffer (self->midi_in);
    }
  if (self->midi_out)
    {
      port_clear_buffer (self->midi_out);
    }
  if (self->piano_roll)
    {
      port_clear_buffer (self->piano_roll);
    }
}

/**
 * Disconnects all ports connected to the
 * TrackProcessor.
 */
void
track_processor_disconnect_all (
  TrackProcessor * self)
{
  Track * track =
    track_processor_get_track (self);
  g_return_if_fail (track);

  switch (track->in_signal_type)
    {
    case TYPE_AUDIO:
      port_disconnect_all (self->mono);
      port_disconnect_all (self->input_gain);
      port_disconnect_all (self->output_gain);
      port_disconnect_all (self->stereo_in->l);
      port_disconnect_all (self->stereo_in->r);
      port_disconnect_all (self->stereo_out->l);
      port_disconnect_all (self->stereo_out->r);
      break;
    case TYPE_EVENT:
      port_disconnect_all (self->midi_in);
      port_disconnect_all (self->midi_out);
      if (track_type_has_piano_roll (track->type))
        port_disconnect_all (self->piano_roll);
      break;
    default:
      break;
    }
}

Track *
track_processor_get_track (
  TrackProcessor * self)
{
  g_return_val_if_fail (
    IS_TRACK_PROCESSOR (self) &&
      IS_TRACK (self->track), NULL);

  return self->track;

#if 0
  g_return_val_if_fail (
    self &&
    self->track_pos < TRACKLIST->num_tracks, NULL);
  Track * track =
    TRACKLIST->tracks[self->track_pos];
  g_return_val_if_fail (track, NULL);

  return track;
#endif
}

/**
 * Splits the cycle and handles recording for each
 * slot.
 */
static void
handle_recording (
  TrackProcessor * self,
  const long       g_start_frames,
  const nframes_t  local_offset,
  const nframes_t  nframes)
{
#if 0
  g_message ("handle recording %ld %" PRIu32,
    g_start_frames, nframes);
#endif

  long split_points[6];
  long each_nframes[6];
  int num_split_points = 1;

  long start_frames = g_start_frames;
  long end_frames = start_frames + nframes;

  /* split the cycle at loop and punch points and
   * record */
  bool loop_hit = false;
  bool punch_in_hit = false;
  /*bool punch_out_hit = false;*/
  /*int loop_end_idx = -1;*/
  split_points[0] = start_frames;
  each_nframes[0] = nframes;
  if (TRANSPORT->loop)
    {
      if (TRANSPORT->loop_end_pos.frames ==
            end_frames)
        {
          loop_hit = true;
          num_split_points = 3;

          /* adjust start slot until loop end */
          each_nframes[0] =
            TRANSPORT->loop_end_pos.frames -
            start_frames;
          /*loop_end_idx = 0;*/

          /* add loop end pause */
          split_points[1] =
            TRANSPORT->loop_end_pos.frames;
          each_nframes[1] = 0;

          /* add part after looping */
          split_points[2] =
            TRANSPORT->loop_start_pos.frames;
          each_nframes[2] =
            nframes - each_nframes[0];
        }
    }
  if (TRANSPORT->punch_mode)
    {
      if (loop_hit)
        {
          /* before loop */
          if (position_between_frames_excl2 (
                &TRANSPORT->punch_in_pos,
                start_frames,
                TRANSPORT->loop_end_pos.frames))
            {
              punch_in_hit = true;
              num_split_points = 4;

              /* move loop start to next slot */
              each_nframes[3] = each_nframes[2];
              split_points[3] = split_points[2];
              each_nframes[2] = each_nframes[1];
              split_points[2] = split_points[1];

              /* add punch in pos */
              split_points[1] =
                TRANSPORT->punch_in_pos.frames;
              each_nframes[1] =
                TRANSPORT->loop_end_pos.frames -
                TRANSPORT->punch_in_pos.frames;
              /*loop_end_idx = 1;*/

              /* adjust num frames for initial
               * pos */
              each_nframes[0] -= each_nframes[1];
            }
          if (position_between_frames_excl2 (
                &TRANSPORT->punch_out_pos,
                start_frames,
                TRANSPORT->loop_end_pos.frames))
            {
              /*punch_out_hit = true;*/
              if (punch_in_hit)
                {
                  num_split_points = 6;

                  /* move loop start to next slot */
                  each_nframes[5] = each_nframes[3];
                  split_points[5] = split_points[3];
                  each_nframes[4] = each_nframes[2];
                  split_points[4] = split_points[2];

                  /* add punch out pos */
                  split_points[2] =
                    TRANSPORT->punch_out_pos.frames;
                  each_nframes[2] =
                    TRANSPORT->loop_end_pos.frames -
                    TRANSPORT->punch_out_pos.frames;

                  /* add pause */
                  split_points[3] =
                    split_points[2] +
                    each_nframes[2];
                  each_nframes[3] = 0;
                  /*loop_end_idx = 2;*/

                  /* adjust num frames for punch in
                   * pos */
                  each_nframes[1] -= each_nframes[2];
                }
              else
                {
                  num_split_points = 5;

                  /* move loop start to next slot */
                  each_nframes[4] = each_nframes[2];
                  split_points[4] = split_points[2];
                  each_nframes[3] = each_nframes[1];
                  split_points[3] = split_points[1];

                  /* add punch out pos */
                  split_points[1] =
                    TRANSPORT->punch_out_pos.frames;
                  each_nframes[1] =
                    TRANSPORT->loop_end_pos.frames -
                    TRANSPORT->punch_out_pos.frames;
                  /*loop_end_idx = 1;*/

                  /* add pause */
                  split_points[2] =
                    split_points[1] +
                    each_nframes[1];
                  each_nframes[2] = 0;

                  /* adjust num frames for init
                   * pos */
                  each_nframes[0] -= each_nframes[1];
                }
            }
        }
      else /* loop not hit */
        {
          if (position_between_frames_excl2 (
                &TRANSPORT->punch_in_pos,
                start_frames, end_frames))
            {
              punch_in_hit = true;
              num_split_points = 2;

              /* add punch in pos */
              split_points[1] =
                TRANSPORT->punch_in_pos.frames;
              each_nframes[1] =
                end_frames -
                TRANSPORT->punch_in_pos.frames;

              /* adjust num frames for initial
               * pos */
              each_nframes[0] -= each_nframes[1];
            }
          if (position_between_frames_excl2 (
                &TRANSPORT->punch_out_pos,
                start_frames, end_frames))
            {
              /*punch_out_hit = true;*/
              if (punch_in_hit)
                {
                  num_split_points = 4;

                  /* add punch out pos */
                  split_points[2] =
                    TRANSPORT->punch_out_pos.frames;
                  each_nframes[2] =
                    end_frames -
                    TRANSPORT->punch_out_pos.frames;

                  /* add pause */
                  split_points[3] =
                    split_points[2] +
                    each_nframes[2];
                  each_nframes[3] = 0;

                  /* adjust num frames for punch in
                   * pos */
                  each_nframes[1] -= each_nframes[2];
                }
              else
                {
                  num_split_points = 3;

                  /* add punch out pos */
                  split_points[1] =
                    TRANSPORT->punch_out_pos.frames;
                  each_nframes[1] =
                    end_frames -
                    TRANSPORT->punch_out_pos.frames;

                  /* add pause */
                  split_points[2] =
                    split_points[1] +
                    each_nframes[1];
                  each_nframes[2] = 0;

                  /* adjust num frames for init
                   * pos */
                  each_nframes[0] -=
                    each_nframes[1];
                }
            }
        }
    }

  long split_point = -1;
  /*bool is_loop_end_idx = false;*/
  for (int i = 0; i < num_split_points; i++)
    {
      g_warn_if_fail (
        split_points[i] >= 0 &&
        each_nframes[i] >= 0);

      /* skip if same as previous point */
      if (split_points[i] == split_point)
        continue;

      split_point = split_points[i];

      /*is_loop_end_idx =*/
        /*loop_hit && i == loop_end_idx;*/

#if 0
      g_message ("sending %ld for %ld frames",
        split_point, each_nframes[i]);
#endif

      recording_manager_handle_recording (
        RECORDING_MANAGER, self, split_point,
        0, each_nframes[i]);
    }
}

static inline void
add_events_from_midi_controls (
  TrackProcessor * self,
  const nframes_t  local_offset)
{
  return;
  for (int i = 0; i < 16; i++)
    {
      /* starting from 1 */
      int channel = i + 1;
      Port * cc = NULL;
      int offset = i * 128;
      for (int j = 0; j < 128; j++)
        {
          cc = self->midi_cc[offset + j];
          if (math_floats_equal (
                cc->last_sent_control,
                cc->control))
            continue;

          midi_events_add_control_change (
            self->midi_out->midi_events,
            channel, (midi_byte_t) j,
            math_round_float_to_type (
              cc->control * 127.f, midi_byte_t),
            local_offset, false);
          cc->last_sent_control = cc->control;
        }

      cc = self->pitch_bend[i];
      if (!math_floats_equal (
            cc->last_sent_control, cc->control))
        {
          midi_events_add_pitchbend (
            self->midi_out->midi_events,
            channel,
            math_round_float_to_int (cc->control),
            local_offset, false);
          cc->last_sent_control = cc->control;
        }

      /* TODO */
#if 0
      cc = self->poly_key_pressure[i];
      midi_events_add_pitchbend (
        self->midi_out->midi_events,
        channel,
        math_round_float_to_int (cc->control),
        local_offset, false);
      cc->last_sent_control = cc->control;

      cc = self->channel_pressure[i];
      midi_events_add_pitchbend (
        self->midi_out->midi_events,
        channel,
        math_round_float_to_int (cc->control),
        local_offset, false);
      cc->last_sent_control = cc->control;
#endif
    }
}

/**
 * Process the TrackProcessor.
 *
 * @param g_start_frames The global start frames.
 * @param local_offset The local start frames.
 * @param nframes The number of frames to process.
 */
void
track_processor_process (
  TrackProcessor * self,
  const long       g_start_frames,
  const nframes_t  local_offset,
  const nframes_t  nframes)
{
  Track * tr = track_processor_get_track (self);
  g_return_if_fail (tr);

  /* if frozen or disabled, skip */
  if (tr->frozen || !track_is_enabled (tr))
    {
      return;
    }

  /* set the audio clip contents to stereo out */
  if (tr->type == TRACK_TYPE_AUDIO)
    {
      track_fill_events (
        tr, g_start_frames, local_offset, nframes,
        NULL, self->stereo_out);
    }

  /* set the piano roll contents to midi out */
  if (track_type_has_piano_roll (tr->type) ||
      tr->type == TRACK_TYPE_CHORD)
    {
      Port * pr = self->piano_roll;

      /* panic MIDI if necessary */
      if (g_atomic_int_get (
            &AUDIO_ENGINE->panic))
        {
          midi_events_panic (
            pr->midi_events, 1);
        }
      /* get events from track if playing */
      else if (TRANSPORT->play_state ==
               PLAYSTATE_ROLLING)
        {
          /* fill midi events from piano roll
           * data */
#if 0
          g_message (
            "filling midi events for %s from %ld",
            tr->name, g_start_frames);
#endif
          track_fill_events (
            tr, g_start_frames, local_offset,
            nframes, pr->midi_events, NULL);
        }
      midi_events_dequeue (
        pr->midi_events);
#if 0
      if (pr->midi_events->num_events > 0)
        {
          g_message (
            "%s piano roll has %d events",
            tr->name,
            pr->midi_events->num_events);
        }
#endif

      /* append midi events from modwheel and
       * pitchbend to MIDI out */
      if (tr->type != TRACK_TYPE_CHORD)
        {
          add_events_from_midi_controls (
            self, local_offset);
        }
      if (self->midi_out->midi_events->num_events > 0)
        {
          g_message (
            "%s midi processor out has %d events",
            tr->name,
            self->midi_out->midi_events->num_events);
        }

      /* append the midi events from piano roll to
       * MIDI out */
      midi_events_append (
        pr->midi_events,
        self->midi_out->midi_events, local_offset,
        nframes, false);

#if 0
      if (pr->midi_events->num_events > 0)
        {
          g_message ("PR");
          midi_events_print (
            pr->midi_events, F_NOT_QUEUED);
        }

      if (self->midi_out->midi_events->num_events >
            0)
        {
          g_message ("midi out");
          midi_events_print (
            self->midi_out->midi_events,
            F_NOT_QUEUED);
        }
#endif
    } /* if has piano roll or is chord track */

  /* add inputs to outputs */
  switch (tr->in_signal_type)
    {
    case TYPE_AUDIO:
      dsp_mix2 (
        &self->stereo_out->l->buf[local_offset],
        &self->stereo_in->l->buf[local_offset],
        1.f,
        self->input_gain ?
          self->input_gain->control : 1.f,
        nframes);

      if (self->mono &&
          control_port_is_toggled (self->mono))
        {
          dsp_mix2 (
            &self->stereo_out->r->buf[local_offset],
            &self->stereo_in->l->buf[local_offset],
            1.f,
            self->input_gain ?
              self->input_gain->control : 1.f,
            nframes);
        }
      else
        {
          dsp_mix2 (
            &self->stereo_out->r->buf[local_offset],
            &self->stereo_in->r->buf[local_offset],
            1.f,
            self->input_gain ?
              self->input_gain->control : 1.f,
            nframes);
        }
      break;
    case TYPE_EVENT:
      /* change the MIDI channel on the midi input
       * to the channel set on the track */
      if (!tr->passthrough_midi_input)
        {
          midi_events_set_channel (
            self->midi_in->midi_events, 0,
            tr->midi_ch);
        }
      midi_events_append (
        self->midi_in->midi_events,
        self->midi_out->midi_events, local_offset,
        nframes, F_NOT_QUEUED);
      break;
    default:
      break;
    }

  if (track_type_can_record (tr->type) ||
      tr->automation_tracklist.num_ats > 0)
    {
      /* handle recording. this will only create
       * events in regions. it will not copy the
       * input content to the output ports */
      handle_recording (
        self, g_start_frames, local_offset,
        nframes);
    }

  /* apply output gain */
  if (tr->type == TRACK_TYPE_AUDIO)
    {
      for (nframes_t l = local_offset;
           l < nframes; l++)
        {
          self->stereo_out->l->buf[l] *=
            self->output_gain->control;
          self->stereo_out->r->buf[l] *=
            self->output_gain->control;
        }
    }
}

/**
 * Copy port values from \ref src to \ref dest.
 */
void
track_processor_copy_values (
  TrackProcessor * dest,
  TrackProcessor * src)
{
  if (src->input_gain)
    {
      dest->input_gain->control =
        src->input_gain->control;
    }
  if (src->mono)
    {
      dest->mono->control = src->mono->control;
    }
}

/**
 * Disconnect stereo in ports from the fader.
 *
 * Used when there is no plugin in the channel.
 */
void
track_processor_disconnect_from_prefader (
  TrackProcessor * self)
{
  Track * tr = track_processor_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (tr));

  Fader * prefader = tr->channel->prefader;
  switch (tr->in_signal_type)
    {
    case TYPE_AUDIO:
      if (tr->out_signal_type == TYPE_AUDIO)
        {
          port_disconnect (
            self->stereo_out->l,
            prefader->stereo_in->l);
          port_disconnect (
            self->stereo_out->r,
            prefader->stereo_in->r);
        }
      break;
    case TYPE_EVENT:
      if (tr->out_signal_type == TYPE_EVENT)
        {
          port_disconnect (
            self->midi_out, prefader->midi_in);
        }
      break;
    default:
      break;
    }
}

/**
 * Connects the TrackProcessor's stereo out ports to
 * the Channel's prefader in ports.
 *
 * Used when deleting the only plugin left.
 */
void
track_processor_connect_to_prefader (
  TrackProcessor * self)
{
  Track * tr = track_processor_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (tr));

  Fader * prefader = tr->channel->prefader;

  /* connect only if signals match */
  if (tr->in_signal_type == TYPE_AUDIO &&
      tr->out_signal_type == TYPE_AUDIO)
    {
      port_connect (
        self->stereo_out->l,
        prefader->stereo_in->l, 1);
      port_connect (
        self->stereo_out->r,
        prefader->stereo_in->r, 1);
    }
  if (tr->in_signal_type == TYPE_EVENT &&
      tr->out_signal_type == TYPE_EVENT)
    {
      port_connect (
        self->midi_out,
        prefader->midi_in, 1);
    }
}

/**
 * Disconnect the TrackProcessor's out ports
 * from the Plugin's input ports.
 */
void
track_processor_disconnect_from_plugin (
  TrackProcessor * self,
  Plugin         * pl)
{
  Track * tr = track_processor_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (tr));

  int i;
  Port * in_port;
  PortType type = tr->in_signal_type;

  for (i = 0; i < pl->num_in_ports; i++)
    {
      in_port = pl->in_ports[i];

      if (type == TYPE_AUDIO)
        {
          if (in_port->id.type !=
                TYPE_AUDIO)
            continue;

          if (ports_connected (
                self->stereo_out->l,
                in_port))
            port_disconnect (
              self->stereo_out->l,
              in_port);
          if (ports_connected (
                self->stereo_out->r,
                in_port))
            port_disconnect (
              self->stereo_out->r,
              in_port);
        }
      else if (type == TYPE_EVENT)
        {
          if (in_port->id.type != TYPE_EVENT)
            continue;

          if (ports_connected (
                self->midi_out, in_port))
            port_disconnect (
              self->midi_out, in_port);
        }
    }
}

/**
 * Connect the TrackProcessor's out ports to the
 * Plugin's input ports.
 */
void
track_processor_connect_to_plugin (
  TrackProcessor * self,
  Plugin         * pl)
{
  Track * tr = track_processor_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (tr));

  int last_index, num_ports_to_connect, i;
  Port * in_port;

  if (tr->in_signal_type == TYPE_EVENT)
    {
      /* Connect MIDI port to the plugin */
      for (i = 0; i < pl->num_in_ports; i++)
        {
          in_port = pl->in_ports[i];
          if (in_port->id.type == TYPE_EVENT &&
              in_port->id.flow == FLOW_INPUT)
            {
              port_connect (
                self->midi_out, in_port, 1);
            }
        }
    }
  else if (tr->in_signal_type == TYPE_AUDIO)
    {
      num_ports_to_connect = 0;
      if (pl->setting->descr->num_audio_ins == 1)
        {
          num_ports_to_connect = 1;
        }
      else if (pl->setting->descr->num_audio_ins > 1)
        {
          num_ports_to_connect = 2;
        }

      last_index = 0;
      for (i = 0; i < num_ports_to_connect; i++)
        {
          for (;
               last_index < pl->num_in_ports;
               last_index++)
            {
              in_port =
                pl->in_ports[
                  last_index];
              if (in_port->id.type == TYPE_AUDIO)
                {
                  if (i == 0)
                    {
                      port_connect (
                        self->stereo_out->l,
                        in_port, 1);
                      last_index++;
                      break;
                    }
                  else if (i == 1)
                    {
                      port_connect (
                        self->stereo_out->r,
                        in_port, 1);
                      last_index++;
                      break;
                    }
                }
            }
        }
    }
}

void
track_processor_set_track_pos (
  TrackProcessor * self,
  int              pos)
{
  self->track_pos = pos;

  if (self->mono)
    {
      port_update_track_pos (
        self->mono, NULL, pos);
    }
  if (self->input_gain)
    {
      port_update_track_pos (
        self->input_gain, NULL, pos);
    }
  if (self->stereo_in)
    {
      port_update_track_pos (
        self->stereo_in->l, NULL, pos);
      port_update_track_pos (
        self->stereo_in->r, NULL, pos);
    }
  if (self->stereo_out)
    {
      port_update_track_pos (
        self->stereo_out->l, NULL, pos);
      port_update_track_pos (
        self->stereo_out->r, NULL, pos);
    }
  if (self->midi_in)
    {
      port_update_track_pos (
        self->midi_in, NULL, pos);
    }
  if (self->midi_out)
    {
      port_update_track_pos (
        self->midi_out, NULL, pos);
    }
  if (self->piano_roll)
    {
      port_update_track_pos (
        self->piano_roll, NULL, pos);
    }
}

/**
 * Frees the members of the TrackProcessor.
 */
void
track_processor_free (
  TrackProcessor * self)
{
  if (IS_PORT_AND_NONNULL (self->mono))
    {
      port_disconnect_all (self->mono);
      object_free_w_func_and_null (
        port_free, self->mono);
    }
  if (IS_PORT_AND_NONNULL (self->input_gain))
    {
      port_disconnect_all (self->input_gain);
      object_free_w_func_and_null (
        port_free, self->input_gain);
    }
  if (IS_PORT_AND_NONNULL (self->output_gain))
    {
      port_disconnect_all (self->output_gain);
      object_free_w_func_and_null (
        port_free, self->output_gain);
    }
  if (self->stereo_in)
    {
      stereo_ports_disconnect (self->stereo_in);
      object_free_w_func_and_null (
        stereo_ports_free, self->stereo_in);
    }
  if (IS_PORT_AND_NONNULL (self->midi_in))
    {
      port_disconnect_all (self->midi_in);
      object_free_w_func_and_null (
        port_free, self->midi_in);
    }
  if (IS_PORT_AND_NONNULL (self->piano_roll))
    {
      port_disconnect_all (self->piano_roll);
      object_free_w_func_and_null (
        port_free, self->piano_roll);
    }
  for (int i = 0; i < 16; i++)
    {
      Port * cc = NULL;
      for (int j = 0; j < 128; j++)
        {
          cc = self->midi_cc[i * 128 + j];
          if (IS_PORT_AND_NONNULL (cc))
            {
              port_disconnect_all (cc);
              object_free_w_func_and_null (
                port_free, cc);
            }
        }

      cc = self->pitch_bend[i];
      if (IS_PORT_AND_NONNULL (cc))
        {
          port_disconnect_all (cc);
          object_free_w_func_and_null (
            port_free, cc);
        }

      cc = self->poly_key_pressure[i];
      if (IS_PORT_AND_NONNULL (cc))
        {
          port_disconnect_all (cc);
          object_free_w_func_and_null (
            port_free, cc);
        }

      cc = self->channel_pressure[i];
      if (IS_PORT_AND_NONNULL (cc))
        {
          port_disconnect_all (cc);
          object_free_w_func_and_null (
            port_free, cc);
        }
    }

  if (self->stereo_out)
    {
      stereo_ports_disconnect (self->stereo_out);
      object_free_w_func_and_null (
        stereo_ports_free, self->stereo_out);
    }
  if (IS_PORT_AND_NONNULL (self->midi_out))
    {
      port_disconnect_all (self->midi_out);
      object_free_w_func_and_null (
        port_free, self->midi_out);
    }

  object_zero_and_free (self);
}
