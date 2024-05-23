// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "dsp/audio_region.h"
#include "dsp/audio_track.h"
#include "dsp/channel.h"
#include "dsp/clip.h"
#include "dsp/control_port.h"
#include "dsp/control_room.h"
#include "dsp/engine.h"
#include "dsp/fader.h"
#include "dsp/midi_event.h"
#include "dsp/midi_mapping.h"
#include "dsp/midi_track.h"
#include "dsp/recording_manager.h"
#include "dsp/track.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/dsp.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/mem.h"
#include "utils/mpmc_queue.h"
#include "utils/objects.h"
#include "zrythm.h"

#include <glib/gi18n.h>

static void
init_common (TrackProcessor * self)
{
  if (self->midi_cc[0])
    {
      self->cc_mappings = midi_mappings_new ();

      for (size_t i = 0; i < 16; i++)
        {
          for (size_t j = 0; j < 128; j++)
            {
              Port * cc_port = self->midi_cc[i * 128 + j];
              g_return_if_fail (cc_port);

              /* set caches */
              cc_port->midi_channel = i + 1;
              cc_port->midi_cc_no = j;

              /* set model bytes for CC:
               * [0] = ctrl change + channel
               * [1] = controller
               * [2] (unused) = control */
              midi_byte_t buf[3];
              buf[0] = (midi_byte_t) (MIDI_CH1_CTRL_CHANGE | (midi_byte_t) i);
              buf[1] = (midi_byte_t) j;
              buf[2] = 0;

              /* bind */
              midi_mappings_bind_track (
                self->cc_mappings, buf, cc_port, F_NO_PUBLISH_EVENTS);
            } /* endforeach 0..127 */

          Port * pb = self->pitch_bend[i];
          g_return_if_fail (pb);
          Port * kp = self->poly_key_pressure[i];
          g_return_if_fail (kp);
          Port * cp = self->channel_pressure[i];
          g_return_if_fail (cp);

          /* set caches */
          pb->midi_channel = i + 1;
          kp->midi_channel = i + 1;
          cp->midi_channel = i + 1;

        } /* endforeach 0..15 */
    }

  self->updated_midi_automatable_ports = mpmc_queue_new ();
  mpmc_queue_reserve (self->updated_midi_automatable_ports, 128 * 16);
}

/**
 * Inits fader after a project is loaded.
 */
void
track_processor_init_loaded (TrackProcessor * self, Track * track)
{
  self->magic = TRACK_PROCESSOR_MAGIC;
  self->track = track;

  GPtrArray * ports = g_ptr_array_new ();
  track_processor_append_ports (self, ports);
  for (size_t i = 0; i < ports->len; i++)
    {
      Port * port = (Port *) g_ptr_array_index (ports, i);
      port_init_loaded (port, self);
    }
  object_free_w_func_and_null (g_ptr_array_unref, ports);

  init_common (self);
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
init_midi_port (TrackProcessor * self, int in)
{
  if (in)
    {
      self->midi_in = port_new_with_type_and_owner (
        ZPortType::Z_PORT_TYPE_EVENT, ZPortFlow::Z_PORT_FLOW_INPUT,
        "TP MIDI in", ZPortOwnerType::Z_PORT_OWNER_TYPE_TRACK_PROCESSOR, self);
      self->midi_in->id.sym = g_strdup ("track_processor_midi_in");
      g_warn_if_fail (IS_PORT (self->midi_in));
      self->midi_in->id.flags |= ZPortFlags::Z_PORT_FLAG_SEND_RECEIVABLE;
    }
  else
    {
      self->midi_out = port_new_with_type_and_owner (
        ZPortType::Z_PORT_TYPE_EVENT, ZPortFlow::Z_PORT_FLOW_OUTPUT,
        "TP MIDI out", ZPortOwnerType::Z_PORT_OWNER_TYPE_TRACK_PROCESSOR, self);
      self->midi_out->id.sym = g_strdup ("track_processor_midi_out");
      g_warn_if_fail (IS_PORT (self->midi_out));
    }
}

static void
init_midi_cc_ports (TrackProcessor * self, int loading)
{
#define INIT_MIDI_PORT(x, idx) \
  x->id.flags |= ZPortFlags::Z_PORT_FLAG_MIDI_AUTOMATABLE; \
  x->id.flags |= ZPortFlags::Z_PORT_FLAG_AUTOMATABLE; \
  x->id.port_index = idx;

  char name[400];
  for (int i = 0; i < 16; i++)
    {
      /* starting from 1 */
      int channel = i + 1;

      for (int j = 0; j < 128; j++)
        {
          sprintf (name, "Ch%d %s", channel, midi_get_controller_name (j));
          Port * cc = port_new_with_type_and_owner (
            ZPortType::Z_PORT_TYPE_CONTROL, ZPortFlow::Z_PORT_FLOW_INPUT, name,
            ZPortOwnerType::Z_PORT_OWNER_TYPE_TRACK_PROCESSOR, self);
          INIT_MIDI_PORT (cc, i * 128 + j);
          cc->id.sym =
            g_strdup_printf ("midi_controller_ch%d_%d", channel, j + 1);
          self->midi_cc[i * 128 + j] = cc;
        }

      sprintf (name, "Ch%d Pitch bend", i + 1);
      Port * cc = port_new_with_type_and_owner (
        ZPortType::Z_PORT_TYPE_CONTROL, ZPortFlow::Z_PORT_FLOW_INPUT, name,
        ZPortOwnerType::Z_PORT_OWNER_TYPE_TRACK_PROCESSOR, self);
      cc->id.sym = g_strdup_printf ("ch%d_pitch_bend", i + 1);
      INIT_MIDI_PORT (cc, i);
      cc->maxf = 8191.f;
      cc->minf = -8192.f;
      cc->deff = 0.f;
      cc->zerof = 0.f;
      cc->id.flags2 |= ZPortFlags2::Z_PORT_FLAG2_MIDI_PITCH_BEND;
      self->pitch_bend[i] = cc;

      sprintf (name, "Ch%d Poly key pressure", i + 1);
      cc = port_new_with_type_and_owner (
        ZPortType::Z_PORT_TYPE_CONTROL, ZPortFlow::Z_PORT_FLOW_INPUT, name,
        ZPortOwnerType::Z_PORT_OWNER_TYPE_TRACK_PROCESSOR, self);
      cc->id.sym = g_strdup_printf ("ch%d_poly_key_pressure", i + 1);
      INIT_MIDI_PORT (cc, i);
      cc->id.flags2 |= ZPortFlags2::Z_PORT_FLAG2_MIDI_POLY_KEY_PRESSURE;
      self->poly_key_pressure[i] = cc;

      sprintf (name, "Ch%d Channel pressure", i + 1);
      cc = port_new_with_type_and_owner (
        ZPortType::Z_PORT_TYPE_CONTROL, ZPortFlow::Z_PORT_FLOW_INPUT, name,
        ZPortOwnerType::Z_PORT_OWNER_TYPE_TRACK_PROCESSOR, self);
      cc->id.sym = g_strdup_printf ("ch%d_channel_pressure", i + 1);
      INIT_MIDI_PORT (cc, i);
      cc->id.flags2 |= ZPortFlags2::Z_PORT_FLAG2_MIDI_CHANNEL_PRESSURE;
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
init_stereo_out_ports (TrackProcessor * self, bool in)
{
  Port *         l, *r;
  StereoPorts ** sp = in ? &self->stereo_in : &self->stereo_out;
  ZPortFlow      flow =
    in ? ZPortFlow::Z_PORT_FLOW_INPUT : ZPortFlow::Z_PORT_FLOW_OUTPUT;

  l = port_new_with_type_and_owner (
    ZPortType::Z_PORT_TYPE_AUDIO, flow,
    in ? "TP Stereo in L" : "TP Stereo out L",
    ZPortOwnerType::Z_PORT_OWNER_TYPE_TRACK_PROCESSOR, self);
  l->id.sym = g_strdup ("track_processor_stereo_out_l");

  r = port_new_with_type_and_owner (
    ZPortType::Z_PORT_TYPE_AUDIO, flow,
    in ? "TP Stereo in R" : "TP Stereo out R",
    ZPortOwnerType::Z_PORT_OWNER_TYPE_TRACK_PROCESSOR, self);
  r->id.sym = g_strdup ("track_processor_stereo_out_r");

  *sp = stereo_ports_new_from_existing (l, r);

  if (in)
    {
      l->id.flags |= ZPortFlags::Z_PORT_FLAG_SEND_RECEIVABLE;
      r->id.flags |= ZPortFlags::Z_PORT_FLAG_SEND_RECEIVABLE;
    }
}

/**
 * Creates a new track processor for the given
 * track.
 */
TrackProcessor *
track_processor_new (Track * tr)
{
  TrackProcessor * self = object_new (TrackProcessor);
  self->magic = TRACK_PROCESSOR_MAGIC;
  self->track = tr;

  self->l_port_db = 0.f;
  self->r_port_db = 0.f;

  switch (tr->in_signal_type)
    {
    case ZPortType::Z_PORT_TYPE_EVENT:
      init_midi_port (self, 0);
      init_midi_port (self, 1);

      /* set up piano roll port */
      if (
        track_type_has_piano_roll (tr->type)
        || tr->type == TrackType::TRACK_TYPE_CHORD)
        {
          self->piano_roll = port_new_with_type_and_owner (
            ZPortType::Z_PORT_TYPE_EVENT, ZPortFlow::Z_PORT_FLOW_INPUT,
            "TP Piano Roll", ZPortOwnerType::Z_PORT_OWNER_TYPE_TRACK_PROCESSOR,
            self);
          self->piano_roll->id.sym = g_strdup ("track_processor_piano_roll");
          self->piano_roll->id.flags = ZPortFlags::Z_PORT_FLAG_PIANO_ROLL;
          if (tr->type != TrackType::TRACK_TYPE_CHORD)
            {
              init_midi_cc_ports (self, false);
            }
        }
      break;
    case ZPortType::Z_PORT_TYPE_AUDIO:
      init_stereo_out_ports (self, false);
      init_stereo_out_ports (self, true);
      if (tr->type == TrackType::TRACK_TYPE_AUDIO)
        {
          self->mono = port_new_with_type_and_owner (
            ZPortType::Z_PORT_TYPE_CONTROL, ZPortFlow::Z_PORT_FLOW_INPUT,
            "TP Mono Toggle", ZPortOwnerType::Z_PORT_OWNER_TYPE_TRACK_PROCESSOR,
            self);
          self->mono->id.sym = g_strdup ("track_processor_mono_toggle");
          self->mono->id.flags |= ZPortFlags::Z_PORT_FLAG_TOGGLE;
          self->mono->id.flags |= ZPortFlags::Z_PORT_FLAG_TP_MONO;
          self->input_gain = port_new_with_type_and_owner (
            ZPortType::Z_PORT_TYPE_CONTROL, ZPortFlow::Z_PORT_FLOW_INPUT,
            "TP Input Gain", ZPortOwnerType::Z_PORT_OWNER_TYPE_TRACK_PROCESSOR,
            self);
          self->input_gain->id.sym = g_strdup ("track_processor_input_gain");
          self->input_gain->minf = 0.f;
          self->input_gain->maxf = 4.f;
          self->input_gain->zerof = 0.f;
          self->input_gain->deff = 1.f;
          self->input_gain->id.flags |= ZPortFlags::Z_PORT_FLAG_TP_INPUT_GAIN;
          port_set_control_value (
            self->input_gain, 1.f, F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);
        }
      break;
    default:
      break;
    }

  if (tr->type == TrackType::TRACK_TYPE_AUDIO)
    {
      self->output_gain = port_new_with_type_and_owner (
        ZPortType::Z_PORT_TYPE_CONTROL, ZPortFlow::Z_PORT_FLOW_INPUT,
        "TP Output Gain", ZPortOwnerType::Z_PORT_OWNER_TYPE_TRACK_PROCESSOR,
        self);
      self->output_gain->id.sym = g_strdup ("track_processor_output_gain");
      self->output_gain->minf = 0.f;
      self->output_gain->maxf = 4.f;
      self->output_gain->zerof = 0.f;
      self->output_gain->deff = 1.f;
      self->output_gain->id.flags2 |= ZPortFlags2::Z_PORT_FLAG2_TP_OUTPUT_GAIN;
      port_set_control_value (
        self->output_gain, 1.f, F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);

      self->monitor_audio = port_new_with_type_and_owner (
        ZPortType::Z_PORT_TYPE_CONTROL, ZPortFlow::Z_PORT_FLOW_INPUT,
        "Monitor audio", ZPortOwnerType::Z_PORT_OWNER_TYPE_TRACK_PROCESSOR,
        self);
      self->monitor_audio->id.sym = g_strdup ("track_processor_monitor_audio");
      self->monitor_audio->id.flags |= ZPortFlags::Z_PORT_FLAG_TOGGLE;
      self->monitor_audio->id.flags2 |=
        ZPortFlags2::Z_PORT_FLAG2_TP_MONITOR_AUDIO;
      port_set_control_value (
        self->monitor_audio, 0.f, F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);
    }

  init_common (self);

  return self;
}

void
track_processor_append_ports (TrackProcessor * self, GPtrArray * ports)
{
#define _ADD(port) \
  g_warn_if_fail (port); \
  g_ptr_array_add (ports, port)

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
  if (self->monitor_audio)
    {
      _ADD (self->monitor_audio);
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

/**
 * Clears all buffers.
 */
void
track_processor_clear_buffers (TrackProcessor * self)
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
track_processor_disconnect_all (TrackProcessor * self)
{
  Track * track = track_processor_get_track (self);
  g_return_if_fail (track);

  switch (track->in_signal_type)
    {
    case ZPortType::Z_PORT_TYPE_AUDIO:
      port_disconnect_all (self->mono);
      port_disconnect_all (self->input_gain);
      port_disconnect_all (self->output_gain);
      port_disconnect_all (self->monitor_audio);
      port_disconnect_all (self->stereo_in->l);
      port_disconnect_all (self->stereo_in->r);
      port_disconnect_all (self->stereo_out->l);
      port_disconnect_all (self->stereo_out->r);
      break;
    case ZPortType::Z_PORT_TYPE_EVENT:
      port_disconnect_all (self->midi_in);
      port_disconnect_all (self->midi_out);
      if (track_type_has_piano_roll (track->type))
        port_disconnect_all (self->piano_roll);
      break;
    default:
      break;
    }
}

/**
 * Splits the cycle and handles recording for each
 * slot.
 */
static void
handle_recording (
  const TrackProcessor *              self,
  const EngineProcessTimeInfo * const time_nfo)
{
#if 0
  g_message ("handle recording %ld %" PRIu32,
    g_start_frames, nframes);
#endif

  unsigned_frame_t split_points[6];
  unsigned_frame_t each_nframes[6];
  int              num_split_points = 1;

  unsigned_frame_t start_frames = time_nfo->g_start_frame_w_offset;
  unsigned_frame_t end_frames = time_nfo->g_start_frame + time_nfo->nframes;

  /* split the cycle at loop and punch points and
   * record */
  bool loop_hit = false;
  bool punch_in_hit = false;
  /*bool punch_out_hit = false;*/
  /*int loop_end_idx = -1;*/
  split_points[0] = start_frames;
  each_nframes[0] = time_nfo->nframes;
  if (TRANSPORT->loop)
    {
      if ((unsigned_frame_t) TRANSPORT->loop_end_pos.frames == end_frames)
        {
          loop_hit = true;
          num_split_points = 3;

          /* adjust start slot until loop end */
          each_nframes[0] =
            (unsigned_frame_t) TRANSPORT->loop_end_pos.frames - start_frames;
          /*loop_end_idx = 0;*/

          /* add loop end pause */
          split_points[1] = (unsigned_frame_t) TRANSPORT->loop_end_pos.frames;
          each_nframes[1] = 0;

          /* add part after looping */
          split_points[2] = (unsigned_frame_t) TRANSPORT->loop_start_pos.frames;
          each_nframes[2] =
            (unsigned_frame_t) time_nfo->nframes - each_nframes[0];
        }
    }
  if (TRANSPORT->punch_mode)
    {
      if (loop_hit)
        {
          /* before loop */
          if (
            position_between_frames_excl2 (
              &TRANSPORT->punch_in_pos, (signed_frame_t) start_frames,
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
                (unsigned_frame_t) TRANSPORT->punch_in_pos.frames;
              each_nframes[1] =
                (unsigned_frame_t) TRANSPORT->loop_end_pos.frames
                - (unsigned_frame_t) TRANSPORT->punch_in_pos.frames;
              /*loop_end_idx = 1;*/

              /* adjust num frames for initial
               * pos */
              each_nframes[0] -= each_nframes[1];
            }
          if (
            position_between_frames_excl2 (
              &TRANSPORT->punch_out_pos, (signed_frame_t) start_frames,
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
                    (unsigned_frame_t) TRANSPORT->punch_out_pos.frames;
                  each_nframes[2] =
                    (unsigned_frame_t) TRANSPORT->loop_end_pos.frames
                    - (unsigned_frame_t) TRANSPORT->punch_out_pos.frames;

                  /* add pause */
                  split_points[3] = split_points[2] + each_nframes[2];
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
                    (unsigned_frame_t) TRANSPORT->punch_out_pos.frames;
                  each_nframes[1] =
                    (unsigned_frame_t) TRANSPORT->loop_end_pos.frames
                    - (unsigned_frame_t) TRANSPORT->punch_out_pos.frames;
                  /*loop_end_idx = 1;*/

                  /* add pause */
                  split_points[2] = split_points[1] + each_nframes[1];
                  each_nframes[2] = 0;

                  /* adjust num frames for init
                   * pos */
                  each_nframes[0] -= each_nframes[1];
                }
            }
        }
      else /* loop not hit */
        {
          if (
            position_between_frames_excl2 (
              &TRANSPORT->punch_in_pos, (signed_frame_t) start_frames,
              (signed_frame_t) end_frames))
            {
              punch_in_hit = true;
              num_split_points = 2;

              /* add punch in pos */
              split_points[1] =
                (unsigned_frame_t) TRANSPORT->punch_in_pos.frames;
              each_nframes[1] =
                end_frames - (unsigned_frame_t) TRANSPORT->punch_in_pos.frames;

              /* adjust num frames for initial
               * pos */
              each_nframes[0] -= each_nframes[1];
            }
          if (
            position_between_frames_excl2 (
              &TRANSPORT->punch_out_pos, (signed_frame_t) start_frames,
              (signed_frame_t) end_frames))
            {
              /*punch_out_hit = true;*/
              if (punch_in_hit)
                {
                  num_split_points = 4;

                  /* add punch out pos */
                  split_points[2] =
                    (unsigned_frame_t) TRANSPORT->punch_out_pos.frames;
                  each_nframes[2] =
                    end_frames
                    - (unsigned_frame_t) TRANSPORT->punch_out_pos.frames;

                  /* add pause */
                  split_points[3] = split_points[2] + each_nframes[2];
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
                    (unsigned_frame_t) TRANSPORT->punch_out_pos.frames;
                  each_nframes[1] =
                    end_frames
                    - (unsigned_frame_t) TRANSPORT->punch_out_pos.frames;

                  /* add pause */
                  split_points[2] = split_points[1] + each_nframes[1];
                  each_nframes[2] = 0;

                  /* adjust num frames for init
                   * pos */
                  each_nframes[0] -= each_nframes[1];
                }
            }
        }
    }

  unsigned_frame_t split_point = 0;
  /*bool is_loop_end_idx = false;*/
  for (int i = 0; i < num_split_points; i++)
    {
      /* skip if same as previous point */
      if (i != 0 && split_points[i] == split_point)
        continue;

      split_point = split_points[i];

      /*is_loop_end_idx =*/
      /*loop_hit && i == loop_end_idx;*/

#if 0
      g_debug (
        "sending %ld for %ld frames", split_point, each_nframes[i]);
#endif

      EngineProcessTimeInfo cur_time_nfo = {
        .g_start_frame = split_point,
        .g_start_frame_w_offset = split_point,
        .local_offset = 0,
        .nframes = each_nframes[i],
      };
      recording_manager_handle_recording (
        RECORDING_MANAGER, self, &cur_time_nfo);
    }
}

/**
 * Adds events to midi out based on any changes in
 * MIDI CC control ports.
 */
static inline void
add_events_from_midi_cc_control_ports (
  const TrackProcessor * self,
  const nframes_t        local_offset)
{
  Port * cc;
  while (
    mpmc_queue_dequeue (self->updated_midi_automatable_ports, (void **) &cc))
    {
      /*port_identifier_print (&cc->id);*/
      if (
        ENUM_BITSET_TEST (
          ZPortFlags2, cc->id.flags2, ZPortFlags2::Z_PORT_FLAG2_MIDI_PITCH_BEND))
        {
          midi_events_add_pitchbend (
            self->midi_out->midi_events, cc->midi_channel,
            math_round_float_to_signed_32 (cc->control) + 0x2000, local_offset,
            F_NOT_QUEUED);
        }
      else if (
        ENUM_BITSET_TEST (
          ZPortFlags2, cc->id.flags2,
          ZPortFlags2::Z_PORT_FLAG2_MIDI_POLY_KEY_PRESSURE))
        {
#if ZRYTHM_TARGET_VER_MAJ > 1
          /* TODO - unsupported in v1 */
#endif
        }
      else if (
        ENUM_BITSET_TEST (
          ZPortFlags2, cc->id.flags2,
          ZPortFlags2::Z_PORT_FLAG2_MIDI_CHANNEL_PRESSURE))
        {
          midi_events_add_channel_pressure (
            self->midi_out->midi_events, cc->midi_channel,
            (midi_byte_t) math_round_float_to_signed_32 (cc->control * 127.f),
            local_offset, F_NOT_QUEUED);
        }
      else
        {
          midi_events_add_control_change (
            self->midi_out->midi_events, cc->midi_channel, cc->midi_cc_no,
            (midi_byte_t) math_round_float_to_signed_32 (cc->control * 127.f),
            local_offset, false);
        }
    }
}

void
track_processor_process (
  TrackProcessor *                    self,
  const EngineProcessTimeInfo * const time_nfo)
{
  Track * tr = self->track;

  /* if frozen or disabled, skip */
  if (tr->frozen || !track_is_enabled (tr))
    {
      return;
    }

  /* set the audio clip contents to stereo out */
  if (tr->type == TrackType::TRACK_TYPE_AUDIO)
    {
      track_fill_events (tr, time_nfo, NULL, self->stereo_out);
    }

  /* set the piano roll contents to midi out */
  if (
    track_type_has_piano_roll (tr->type)
    || tr->type == TrackType::TRACK_TYPE_CHORD)
    {
      Port * pr = self->piano_roll;

      /* panic MIDI if necessary */
      if (g_atomic_int_get (&AUDIO_ENGINE->panic))
        {
          midi_events_panic (pr->midi_events, F_QUEUED);
        }
      /* get events from track if playing */
      else if (
        TRANSPORT->play_state == PlayState::PLAYSTATE_ROLLING
        || track_is_auditioner (tr))
        {
          /* fill midi events from piano roll data */
#if 0
          g_message (
            "filling midi events for %s from %ld", tr->name, time_nfo->g_start_frame_w_offset);
#endif
          track_fill_events (tr, time_nfo, pr->midi_events, NULL);
        }
      midi_events_dequeue (pr->midi_events);
#if 0
      if (pr->midi_events->num_events > 0)
        {
          g_message (
            "%s piano roll has %d events",
            tr->name,
            pr->midi_events->num_events);
        }
#endif

      /* append midi events from modwheel and pitchbend control ports to MIDI
       * out */
      if (tr->type != TrackType::TRACK_TYPE_CHORD)
        {
          add_events_from_midi_cc_control_ports (self, time_nfo->local_offset);
        }
      if (self->midi_out->midi_events->num_events > 0)
        {
#if 0
          g_debug (
            "%s midi processor out has %d events",
            tr->name, self->midi_out->midi_events->num_events);
#endif
        }

      /* append the midi events from piano roll to MIDI out */
      midi_events_append (
        self->midi_out->midi_events, pr->midi_events, time_nfo->local_offset,
        time_nfo->nframes, false);

#if 0
      if (pr->midi_events->num_events > 0)
        {
          g_message ("PR");
          midi_events_print (pr->midi_events, F_NOT_QUEUED);
        }

      if (self->midi_out->midi_events->num_events > 0)
        {
          g_message ("midi out");
          midi_events_print (self->midi_out->midi_events, F_NOT_QUEUED);
        }
#endif
    } /* if has piano roll or is chord track */

  /* if currently active track on the piano roll,
   * fetch events */
  if (
    tr->in_signal_type == ZPortType::Z_PORT_TYPE_EVENT
    && CLIP_EDITOR->has_region)
    {
      Track * clip_editor_track = clip_editor_get_track (CLIP_EDITOR);
      if (clip_editor_track == tr)
        {
          /* if not set to "all channels",
           * filter-append */
          if (!tr->channel->all_midi_channels)
            {
              midi_events_append_w_filter (
                self->midi_in->midi_events,
                AUDIO_ENGINE->midi_editor_manual_press->midi_events,
                tr->channel->midi_channels, time_nfo->local_offset,
                time_nfo->nframes, F_NOT_QUEUED);
            }
          /* otherwise append normally */
          else
            {
              midi_events_append (
                self->midi_in->midi_events,
                AUDIO_ENGINE->midi_editor_manual_press->midi_events,
                time_nfo->local_offset, time_nfo->nframes, F_NOT_QUEUED);
            }
        }
    }

  /* add inputs to outputs */
  switch (tr->in_signal_type)
    {
    case ZPortType::Z_PORT_TYPE_AUDIO:
      if (
        tr->type != TrackType::TRACK_TYPE_AUDIO
        || (tr->type == TrackType::TRACK_TYPE_AUDIO && control_port_is_toggled (self->monitor_audio)))
        {
          dsp_mix2 (
            &self->stereo_out->l->buf[time_nfo->local_offset],
            &self->stereo_in->l->buf[time_nfo->local_offset], 1.f,
            self->input_gain ? self->input_gain->control : 1.f,
            time_nfo->nframes);

          if (self->mono && control_port_is_toggled (self->mono))
            {
              dsp_mix2 (
                &self->stereo_out->r->buf[time_nfo->local_offset],
                &self->stereo_in->l->buf[time_nfo->local_offset], 1.f,
                self->input_gain ? self->input_gain->control : 1.f,
                time_nfo->nframes);
            }
          else
            {
              dsp_mix2 (
                &self->stereo_out->r->buf[time_nfo->local_offset],
                &self->stereo_in->r->buf[time_nfo->local_offset], 1.f,
                self->input_gain ? self->input_gain->control : 1.f,
                time_nfo->nframes);
            }
        }
      break;
    case ZPortType::Z_PORT_TYPE_EVENT:
      /* change the MIDI channel on the midi input
       * to the channel set on the track */
      if (!tr->passthrough_midi_input)
        {
          midi_events_set_channel (
            self->midi_in->midi_events, F_NOT_QUEUED, tr->midi_ch);
        }

      /* process midi bindings */
      if (self->cc_mappings && TRANSPORT->recording)
        {
          midi_mappings_apply_from_cc_events (
            self->cc_mappings, self->midi_in->midi_events, F_NOT_QUEUED);
        }

      /* if chord track, transform MIDI input to
       * appropriate MIDI notes */
      if (tr->type == TrackType::TRACK_TYPE_CHORD)
        {
          midi_events_transform_chord_and_append (
            self->midi_out->midi_events, self->midi_in->midi_events,
            time_nfo->local_offset, time_nfo->nframes, F_NOT_QUEUED);
        }
      /* else if not chord track, simply pass the
       * input MIDI data to the output port */
      else
        {
          midi_events_append (
            self->midi_out->midi_events, self->midi_in->midi_events,
            time_nfo->local_offset, time_nfo->nframes, F_NOT_QUEUED);
        }

      /* if pending a panic message, append it */
      if (self->pending_midi_panic)
        {
          midi_events_panic_without_lock (
            self->midi_out->midi_events, F_NOT_QUEUED);
          self->pending_midi_panic = false;
        }

      break;
    default:
      break;
    }

  if (
    !track_is_auditioner (tr) && TRANSPORT->preroll_frames_remaining == 0
    && (track_type_can_record (tr->type) || tr->automation_tracklist.num_ats > 0))
    {
      /* handle recording. this will only create events in regions. it will not
       * copy the input content to the output ports. this will also create
       * automation for MIDI CC, if any (see midi_mappings_apply_cc_events
       * above) */
      handle_recording (self, time_nfo);
    }

  /* apply output gain */
  if (tr->type == TrackType::TRACK_TYPE_AUDIO)
    {
      dsp_mul_k2 (
        &self->stereo_out->l->buf[time_nfo->local_offset],
        self->output_gain->control, time_nfo->nframes);
      dsp_mul_k2 (
        &self->stereo_out->r->buf[time_nfo->local_offset],
        self->output_gain->control, time_nfo->nframes);
    }
}

/**
 * Copy port values from \ref src to \ref dest.
 */
void
track_processor_copy_values (TrackProcessor * dest, TrackProcessor * src)
{
  if (src->input_gain)
    {
      dest->input_gain->control = src->input_gain->control;
    }
  if (src->monitor_audio)
    {
      dest->monitor_audio->control = src->monitor_audio->control;
    }
  if (src->output_gain)
    {
      dest->output_gain->control = src->output_gain->control;
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
track_processor_disconnect_from_prefader (TrackProcessor * self)
{
  Track * tr = track_processor_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (tr));

  Fader * prefader = tr->channel->prefader;
  switch (tr->in_signal_type)
    {
    case ZPortType::Z_PORT_TYPE_AUDIO:
      if (tr->out_signal_type == ZPortType::Z_PORT_TYPE_AUDIO)
        {
          port_disconnect (self->stereo_out->l, prefader->stereo_in->l);
          port_disconnect (self->stereo_out->r, prefader->stereo_in->r);
        }
      break;
    case ZPortType::Z_PORT_TYPE_EVENT:
      if (tr->out_signal_type == ZPortType::Z_PORT_TYPE_EVENT)
        {
          port_disconnect (self->midi_out, prefader->midi_in);
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
track_processor_connect_to_prefader (TrackProcessor * self)
{
  Track * tr = track_processor_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (tr));

  Fader * prefader = tr->channel->prefader;
  g_return_if_fail (IS_FADER (prefader));

  /* connect only if signals match */
  if (
    tr->in_signal_type == ZPortType::Z_PORT_TYPE_AUDIO
    && tr->out_signal_type == ZPortType::Z_PORT_TYPE_AUDIO)
    {
      port_connect (self->stereo_out->l, prefader->stereo_in->l, 1);
      port_connect (self->stereo_out->r, prefader->stereo_in->r, 1);
    }
  if (
    tr->in_signal_type == ZPortType::Z_PORT_TYPE_EVENT
    && tr->out_signal_type == ZPortType::Z_PORT_TYPE_EVENT)
    {
      port_connect (self->midi_out, prefader->midi_in, 1);
    }
}

/**
 * Disconnect the TrackProcessor's out ports
 * from the Plugin's input ports.
 */
void
track_processor_disconnect_from_plugin (TrackProcessor * self, Plugin * pl)
{
  Track * tr = track_processor_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (tr));

  Port *    in_port;
  ZPortType type = tr->in_signal_type;

  for (int i = 0; i < pl->num_in_ports; i++)
    {
      in_port = pl->in_ports[i];

      if (type == ZPortType::Z_PORT_TYPE_AUDIO)
        {
          if (in_port->id.type != ZPortType::Z_PORT_TYPE_AUDIO)
            continue;

          if (ports_connected (self->stereo_out->l, in_port))
            port_disconnect (self->stereo_out->l, in_port);
          if (ports_connected (self->stereo_out->r, in_port))
            port_disconnect (self->stereo_out->r, in_port);
        }
      else if (type == ZPortType::Z_PORT_TYPE_EVENT)
        {
          if (in_port->id.type != ZPortType::Z_PORT_TYPE_EVENT)
            continue;

          if (ports_connected (self->midi_out, in_port))
            port_disconnect (self->midi_out, in_port);
        }
    }
}

/**
 * Connect the TrackProcessor's out ports to the
 * Plugin's input ports.
 */
void
track_processor_connect_to_plugin (TrackProcessor * self, Plugin * pl)
{
  Track * tr = track_processor_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (tr));

  int    last_index, num_ports_to_connect, i;
  Port * in_port;

  if (tr->in_signal_type == ZPortType::Z_PORT_TYPE_EVENT)
    {
      /* Connect MIDI port to the plugin */
      for (i = 0; i < pl->num_in_ports; i++)
        {
          in_port = pl->in_ports[i];
          if (
            in_port->id.type == ZPortType::Z_PORT_TYPE_EVENT
            && in_port->id.flow == ZPortFlow::Z_PORT_FLOW_INPUT)
            {
              port_connect (self->midi_out, in_port, 1);
            }
        }
    }
  else if (tr->in_signal_type == ZPortType::Z_PORT_TYPE_AUDIO)
    {
      /* get actual port counts */
      int num_pl_audio_ins = 0;
      for (i = 0; i < pl->num_in_ports; i++)
        {
          Port * port = pl->in_ports[i];
          if (port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
            num_pl_audio_ins++;
        }

      num_ports_to_connect = 0;
      if (num_pl_audio_ins == 1)
        {
          num_ports_to_connect = 1;
        }
      else if (num_pl_audio_ins > 1)
        {
          num_ports_to_connect = 2;
        }

      last_index = 0;
      for (i = 0; i < num_ports_to_connect; i++)
        {
          for (; last_index < pl->num_in_ports; last_index++)
            {
              in_port = pl->in_ports[last_index];
              if (in_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
                {
                  if (i == 0)
                    {
                      port_connect (self->stereo_out->l, in_port, 1);
                      last_index++;
                      break;
                    }
                  else if (i == 1)
                    {
                      port_connect (self->stereo_out->r, in_port, 1);
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
track_processor_free (TrackProcessor * self)
{
  object_free_w_func_and_null (midi_mappings_free, self->cc_mappings);

  if (IS_PORT_AND_NONNULL (self->mono))
    {
      port_disconnect_all (self->mono);
      object_free_w_func_and_null (port_free, self->mono);
    }
  if (IS_PORT_AND_NONNULL (self->input_gain))
    {
      port_disconnect_all (self->input_gain);
      object_free_w_func_and_null (port_free, self->input_gain);
    }
  if (IS_PORT_AND_NONNULL (self->output_gain))
    {
      port_disconnect_all (self->output_gain);
      object_free_w_func_and_null (port_free, self->output_gain);
    }
  if (IS_PORT_AND_NONNULL (self->monitor_audio))
    {
      port_disconnect_all (self->monitor_audio);
      object_free_w_func_and_null (port_free, self->monitor_audio);
    }
  if (self->stereo_in)
    {
      stereo_ports_disconnect (self->stereo_in);
      object_free_w_func_and_null (stereo_ports_free, self->stereo_in);
    }
  if (IS_PORT_AND_NONNULL (self->midi_in))
    {
      port_disconnect_all (self->midi_in);
      object_free_w_func_and_null (port_free, self->midi_in);
    }
  if (IS_PORT_AND_NONNULL (self->piano_roll))
    {
      port_disconnect_all (self->piano_roll);
      object_free_w_func_and_null (port_free, self->piano_roll);
    }

#define FREE_CC(cc) \
  if (cc) \
    { \
      port_disconnect_all (cc); \
      object_free_w_func_and_null (port_free, cc); \
    }

  for (int i = 0; i < 16; i++)
    {
      Port * cc = NULL;
      for (int j = 0; j < 128; j++)
        {
          cc = self->midi_cc[i * 128 + j];
          FREE_CC (cc);
        }

      cc = self->pitch_bend[i];
      FREE_CC (cc);

      cc = self->poly_key_pressure[i];
      FREE_CC (cc);

      cc = self->channel_pressure[i];
      FREE_CC (cc);
    }

#undef FREE_CC

  if (self->stereo_out)
    {
      stereo_ports_disconnect (self->stereo_out);
      object_free_w_func_and_null (stereo_ports_free, self->stereo_out);
    }
  if (IS_PORT_AND_NONNULL (self->midi_out))
    {
      port_disconnect_all (self->midi_out);
      object_free_w_func_and_null (port_free, self->midi_out);
    }

  object_free_w_func_and_null (
    mpmc_queue_free, self->updated_midi_automatable_ports);

  object_zero_and_free (self);
}
