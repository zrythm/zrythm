/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/audio_track.h"
#include "audio/channel.h"
#include "audio/clip.h"
#include "audio/control_room.h"
#include "audio/engine.h"
#include "audio/fader.h"
#include "audio/midi.h"
#include "audio/midi_track.h"
#include "audio/track.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "zrythm.h"

#include <glib/gi18n.h>

/**
 * Inits fader after a project is loaded.
 */
void
track_processor_init_loaded (
  TrackProcessor * self)
{
  Track * tr = self->track;

  switch (tr->in_signal_type)
    {
    case TYPE_AUDIO:
      port_set_owner_track_processor (
        self->stereo_in->l, tr);
      port_set_owner_track_processor (
        self->stereo_in->r, tr);
      port_set_owner_track_processor (
        self->stereo_out->l, tr);
      port_set_owner_track_processor (
        self->stereo_out->r, tr);
      break;
    case TYPE_EVENT:
      if (track_has_piano_roll (tr))
        {
          self->piano_roll->identifier.flags =
            PORT_FLAG_PIANO_ROLL;
          port_set_owner_track_processor (
            self->piano_roll, tr);
          self->piano_roll->midi_events =
            midi_events_new (
              self->piano_roll);
        }
      self->midi_in->midi_events =
        midi_events_new (
          self->midi_in);
      self->midi_out->midi_events =
        midi_events_new (
          self->midi_out);
      port_set_owner_track_processor (
        self->midi_in, tr);
      port_set_owner_track_processor (
        self->midi_out, tr);
      break;
    default:
      break;
    }
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
  int              in,
  int              loading)
{
  const char * str =
    in ? "MIDI in" : "(internal) MIDI out";
  Port ** port =
    in ? &self->midi_in : &self->midi_out;
  PortFlow flow = in ? FLOW_INPUT : FLOW_OUTPUT;

  *port =
    port_new_with_type (
      TYPE_EVENT,
      flow,
      str);

  port_set_owner_track_processor (
    *port, self->track);
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
  int              in,
  int              loading)
{
  char str[80];
  strcpy (
    str,
    in ? "Stereo in" : "(internal) Stereo out");
  Port * l, * r;
  StereoPorts ** sp =
    in ? &self->stereo_in : &self->stereo_out;
  PortFlow flow = in ? FLOW_INPUT : FLOW_OUTPUT;

  if (loading)
    {
      l = NULL;
      r = NULL;
    }
  else
    {
      strcat (str, " L");
      l = port_new_with_type (
        TYPE_AUDIO,
        flow,
        str);

      str[10] = '\0';
      strcat (str, " R");
      r = port_new_with_type (
        TYPE_AUDIO,
        flow,
        str);
    }

  port_set_owner_track_processor (
    l, self->track);
  port_set_owner_track_processor (
    r, self->track);

  *sp =
    stereo_ports_new_from_existing (
      l, r);
}

/**
 * Inits the TrackProcessor to default values.
 *
 * @param self The TrackProcessor to init.
 * @param track The owner Track.
 */
void
track_processor_init (
  TrackProcessor * self,
  Track *          tr)
{
  self->track = tr;

  self->l_port_db = 0.f;
  self->r_port_db = 0.f;

  switch  (tr->in_signal_type)
    {
    case TYPE_EVENT:
      init_midi_port (self, 0, 0);
      init_midi_port (self, 1, 0);

      /* set up piano roll port */
      if (track_has_piano_roll (tr))
        {
          char *str = _("Piano Roll");
          self->piano_roll =
            port_new_with_type (
              TYPE_EVENT,
              FLOW_INPUT,
              str);
          self->piano_roll->identifier.flags =
            PORT_FLAG_PIANO_ROLL;
          port_set_owner_track_processor (
            self->piano_roll,
            tr);
        }
      break;
    case TYPE_AUDIO:
      init_stereo_out_ports (
        self, 0, 0);
      init_stereo_out_ports (
        self, 1, 0);
      break;
    default:
      break;
    }
}

/**
 * Clears all buffers.
 */
void
track_processor_clear_buffers (
  TrackProcessor * self)
{
  switch (self->track->in_signal_type)
    {
    case TYPE_AUDIO:
      port_clear_buffer (self->stereo_in->l);
      port_clear_buffer (self->stereo_in->r);
      port_clear_buffer (self->stereo_out->l);
      port_clear_buffer (self->stereo_out->r);
      break;
    case TYPE_EVENT:
      port_clear_buffer (self->midi_in);
      port_clear_buffer (self->midi_out);
      if (track_has_piano_roll (self->track))
        port_clear_buffer (self->piano_roll);
      break;
    default:
      break;
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
  switch (self->track->in_signal_type)
    {
    case TYPE_AUDIO:
      port_disconnect_all (self->stereo_in->l);
      port_disconnect_all (self->stereo_in->r);
      port_disconnect_all (self->stereo_out->l);
      port_disconnect_all (self->stereo_out->r);
      break;
    case TYPE_EVENT:
      port_disconnect_all (self->midi_in);
      port_disconnect_all (self->midi_out);
      if (track_has_piano_roll (self->track))
        port_disconnect_all (self->piano_roll);
      break;
    default:
      break;
    }
}

/**
 * Handles the recording logic inside the process
 * cycle.
 *
 * The MidiEvents are already dequeued at this
 * point.
 *
 * @param g_frames_start Global start frames.
 * @param nframes Number of frames to process.
 */
static inline void
handle_recording (
  TrackProcessor * self,
  const long       g_start_frames,
  const nframes_t  local_offset,
  const nframes_t  nframes)
{
  Track * tr = self->track;

  if (!TRANSPORT->recording ||
      !tr->recording ||
      !TRANSPORT_IS_ROLLING)
    {
      tr->recording_region = NULL;
      return;
    }

  /* get end position */
  long start_frames = g_start_frames;
  long end_frames =
    g_start_frames + (long) nframes;

  /* adjust for transport loop end */
  int loop_met = 0;
  nframes_t frames_till_loop = 0;
  if ((frames_till_loop =
         transport_is_loop_point_met (
           TRANSPORT, g_start_frames, nframes)))
    {
      loop_met = 1;
      start_frames =
        TRANSPORT->loop_start_pos.frames;
      end_frames =
        (end_frames -
           TRANSPORT->loop_end_pos.frames) +
        start_frames;
    }

  Position start_pos, end_pos;
  position_from_frames (
    &start_pos, start_frames);
  position_from_frames (
    &end_pos, end_frames);

  int is_audio = tr->type == TRACK_TYPE_AUDIO;

  if (track_has_piano_roll (tr) ||
      is_audio)
    {
      /* get the recording region */
      Region * region =
        tr->recording_region;

      /* the region before the loop point, if
       * loop point is met */
      Region * region_before_loop_end = NULL;

      /* the clip, if audio */
      AudioClip * clip = NULL;
      AudioClip * clip_before_loop_end = NULL;

      if (region)
        {
          if (is_audio)
            {
              clip =
                audio_region_get_clip (region);
            }
          if (loop_met)
            {
              region_before_loop_end = region;
              clip_before_loop_end = clip;

              /* set current region end pos  to
               * transport loop end */
              region_set_end_pos (
                region, &TRANSPORT->loop_end_pos,
                AO_UPDATE_ALL);
              region->end_pos.frames =
                TRANSPORT->loop_end_pos.frames;
              if (is_audio)
                {
                  clip->num_frames =
                    region->end_pos.frames -
                    region->start_pos.frames;
                  clip->frames =
                    (sample_t *) realloc (
                    clip->frames,
                    (size_t)
                    (clip->num_frames *
                       clip->channels) *
                    sizeof (sample_t));
                }
              region_set_loop_end_pos (
                region, &TRANSPORT->loop_end_pos,
                AO_UPDATE_ALL);
              region->loop_end_pos.frames =
                TRANSPORT->loop_end_pos.frames;

              /* start new region in new lane at
               * TRANSPORT loop start */
              Region * new_region = NULL;
              if (track_has_piano_roll (tr))
                {
                  new_region =
                    midi_region_new (
                      &TRANSPORT->loop_start_pos,
                      &end_pos, 1);
                }
              else if (tr->type == TRACK_TYPE_AUDIO)
                {
                  new_region =
                    audio_region_new (
                      -1, NULL, NULL, nframes, 2,
                      &TRANSPORT->loop_start_pos,
                      1);
                }
              track_add_region (
                tr, new_region, NULL,
                region->lane_pos + 1, F_GEN_NAME,
                F_PUBLISH_EVENTS);
              region = new_region;

              if (is_audio)
                {
                  clip =
                    audio_region_get_clip (region);
                }
            }
          else /* loop not met */
            {
              /* set region end pos */
              region_set_end_pos (
                region, &end_pos, AO_UPDATE_ALL);
              region->end_pos.frames =
                end_pos.frames;
              if (is_audio)
                {
                  clip->num_frames =
                    region->end_pos.frames -
                    region->start_pos.frames;
                  clip->frames =
                    (sample_t *) realloc (
                    clip->frames,
                    (size_t)
                    (clip->num_frames *
                       clip->channels) *
                    sizeof (sample_t));
                }
              region_set_loop_end_pos (
                region, &end_pos, AO_UPDATE_ALL);
              region->loop_end_pos.frames =
                end_pos.frames;
            }
        }
      else
        {
          /* create region */
          if (track_has_piano_roll (tr))
            {
              region =
                midi_region_new (
                  &start_pos, &end_pos, 1);
            }
          else if (tr->type == TRACK_TYPE_AUDIO)
            {
              region =
                audio_region_new (
                  -1, NULL, NULL, nframes, 2,
                  &start_pos, 1);
            }
          g_return_if_fail (region);
          track_add_region (
            tr, region, NULL,
            tr->num_lanes - 1,
            F_GEN_NAME, F_PUBLISH_EVENTS);

          if (is_audio)
            {
              clip =
                audio_region_get_clip (region);
            }
        }

      tr->recording_region =
        region;

      if (track_has_piano_roll (tr))
        {
          MidiEvents * midi_events =
            self->midi_in->midi_events;
          MidiNote * mn;

          /* add midi note off if loop met */
          if (loop_met)
            {
              while (
                (mn =
                  midi_region_pop_unended_note (
                    region_before_loop_end, -1)))
                {
                  midi_note_set_end_pos (
                    mn, &TRANSPORT->loop_end_pos,
                    AO_UPDATE_ALL);
                }
            }

          /* convert MIDI data to midi notes */
          if (midi_events->num_events > 0)
            {
              for (int i = 0;
                   i < midi_events->num_events; i++)
                {
                  MidiEvent * ev =
                    & midi_events->events[i];

                  switch (ev->type)
                    {
                      case MIDI_EVENT_TYPE_NOTE_ON:
                        g_return_if_fail (region);
                        mn =
                          midi_note_new (
                            region, &start_pos,
                            &end_pos,
                            ev->note_pitch,
                            ev->velocity, 1);
                        midi_region_add_midi_note (
                          region, mn);

                        /* add to unended notes */
                        array_append (
                          region->unended_notes,
                          region->num_unended_notes,
                          mn);
                        break;
                      case MIDI_EVENT_TYPE_NOTE_OFF:
                        g_return_if_fail (region);
                        mn =
                          midi_region_pop_unended_note (
                            region, ev->note_pitch);
                        if (mn)
                          midi_note_set_end_pos (
                            mn, &end_pos, AO_UPDATE_ALL);
                        break;
                      default:
                        /* TODO */
                        break;
                    }
                } /* for loop num events */
            } /* if have midi events */
        } /* if track has piano roll */
      else if (is_audio)
        {
          if (loop_met)
            {
              /* handle the samples until loop end */
              if (region_before_loop_end)
                {
                  long clip_offset_before_loop =
                    g_start_frames -
                    region_before_loop_end->
                      start_pos.frames;
                  for (
                    nframes_t i =
                      local_offset;
                    i <
                      local_offset +
                        frames_till_loop;
                    i++)
                    {
                      g_warn_if_fail (
                        clip_offset_before_loop >= 0 &&
                        clip_offset_before_loop <
                          clip_before_loop_end->
                            num_frames);
                      g_warn_if_fail (
                        i >= local_offset &&
                        i < local_offset + nframes);
                      clip_before_loop_end->frames[
                        clip_before_loop_end->channels *
                          clip_offset_before_loop] =
                            self->stereo_in->l->buf[i];
                      clip_before_loop_end->frames[
                        clip_before_loop_end->channels *
                          (clip_offset_before_loop++)] =
                            self->stereo_in->r->buf[i];
                    }
                }

              /* handle samples after loop start */
              long clip_offset = 0;
              for (
                nframes_t i =
                  nframes -
                    (local_offset +
                      frames_till_loop);
                i < nframes;
                i++)
                {
                  g_warn_if_fail (
                    clip_offset >= 0 &&
                    clip_offset <
                      clip->num_frames);
                  g_warn_if_fail (
                    i >= local_offset &&
                    i < local_offset + nframes);
                  clip->frames[
                    clip->channels *
                      clip_offset] =
                        self->stereo_in->l->buf[i];
                  clip->frames[
                    clip->channels *
                      (clip_offset++)] =
                        self->stereo_in->r->buf[i];
                }
            }

          /* handle the samples normally */
          nframes_t cur_local_offset =
            local_offset;
          g_return_if_fail (region);
          for (long i =
                 start_frames -
                   region->start_pos.frames;
               i <
                 end_frames -
                   region->start_pos.frames; i++)
            {
              g_warn_if_fail (
                i >= 0 &&
                i < clip->num_frames);
              g_warn_if_fail (
                cur_local_offset >= local_offset &&
                cur_local_offset <
                  local_offset + nframes);
              clip->frames[
                i * clip->channels] =
                  self->stereo_in->l->buf[
                    cur_local_offset];
              clip->frames[
                i * clip->channels] =
                  self->stereo_in->l->buf[
                    cur_local_offset++];
            }
        } /* if audio track */
    } /* if track is MIDI or AUDIO */
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
  Track * tr = self->track;

  /* if record is not armed, clear the input buffers
   * to ignore the data */
  if (!tr->recording &&
      track_type_can_record (tr->type))
    {
      switch (tr->in_signal_type)
        {
        case TYPE_EVENT:
          port_clear_buffer (self->midi_in);
          break;
        case TYPE_AUDIO:
          port_clear_buffer (self->stereo_in->l);
          port_clear_buffer (self->stereo_in->r);
        default:
          break;
        }
    }

  /* set the audio clip contents to stereo out */
  if (tr->type == TRACK_TYPE_AUDIO)
    {
      audio_track_fill_stereo_ports_from_clip (
        tr, self->stereo_out,
        g_start_frames, local_offset,
        nframes);
    }

  /* set the piano roll contents to midi out */
  if (track_has_piano_roll (tr))
    {
      Port * port = self->piano_roll;

      /* panic MIDI if necessary */
      if (g_atomic_int_get (
            &AUDIO_ENGINE->panic))
        {
          midi_events_panic (
            port->midi_events, 1);
        }
      /* get events from track if playing */
      else if (TRANSPORT->play_state ==
               PLAYSTATE_ROLLING)
        {
          /* fill midi events to pass to
           * ins plugin */
          midi_track_fill_midi_events (
            tr, g_start_frames,
            local_offset, nframes,
            port->midi_events);
        }
      midi_events_dequeue (
        port->midi_events);
      if (port->midi_events->num_events > 0)
        g_message (
          "%s piano roll has %d events",
          tr->name,
          port->midi_events->num_events);

      /* set the midi events to MIDI out */
      midi_events_append (
        port->midi_events,
        self->midi_out->midi_events, local_offset,
        nframes, 0);
    }

  /* handle recording. this will only create events in
   * regions. it will not copy the input content
   * to the output ports */
  handle_recording (
    self, g_start_frames, local_offset, nframes);

  /* add inputs to outputs */
  switch (tr->in_signal_type)
    {
    case TYPE_AUDIO:
      for (nframes_t l = local_offset;
           l < nframes; l++)
        {
          self->stereo_out->l->buf[l] +=
            self->stereo_in->l->buf[l];
          self->stereo_out->r->buf[l] +=
            self->stereo_in->r->buf[l];
        }
      break;
    case TYPE_EVENT:
      midi_events_append (
        self->midi_in->midi_events,
        self->midi_out->midi_events, local_offset,
        nframes, 0);
      break;
    default:
      break;
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
  Track * tr = self->track;
  PassthroughProcessor * prefader =
    &tr->channel->prefader;
  switch (self->track->in_signal_type)
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
  Track * tr = self->track;
  PassthroughProcessor * prefader =
    &tr->channel->prefader;

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
  Track * tr = self->track;

  int i;
  Port * in_port;
  PortType type = tr->in_signal_type;

  for (i = 0; i < pl->num_in_ports; i++)
    {
      in_port = pl->in_ports[i];

      if (type == TYPE_AUDIO)
        {
          if (in_port->identifier.type !=
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
          if (in_port->identifier.type !=
                TYPE_EVENT)
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
  Track * tr = self->track;
  int last_index, num_ports_to_connect, i;
  Port * in_port;

  if (tr->in_signal_type == TYPE_EVENT)
    {
      /* Connect MIDI port to the plugin */
      for (i = 0; i < pl->num_in_ports; i++)
        {
          in_port = pl->in_ports[i];
          if (in_port->identifier.type ==
                TYPE_EVENT &&
              in_port->identifier.flow ==
                FLOW_INPUT)
            {
              port_connect (
                self->midi_out, in_port, 1);
            }
        }
    }
  else if (tr->in_signal_type == TYPE_AUDIO)
    {
      num_ports_to_connect = 0;
      if (pl->descr->num_audio_ins == 1)
        {
          num_ports_to_connect = 1;
        }
      else if (pl->descr->num_audio_ins > 1)
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
              if (in_port->identifier.type ==
                    TYPE_AUDIO)
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

/**
 * Frees the members of the TrackProcessor.
 */
void
track_processor_free_members (
  TrackProcessor * self)
{
  switch (self->track->in_signal_type)
    {
    case TYPE_AUDIO:
      port_free (self->stereo_in->l);
      port_free (self->stereo_in->r);
      break;
    case TYPE_EVENT:
      port_free (self->midi_in);
      if (track_has_piano_roll (self->track))
        port_free (self->piano_roll);
      break;
    default:
      break;
    }
}
