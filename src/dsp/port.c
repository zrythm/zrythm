// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "dsp/channel.h"
#include "dsp/clip.h"
#include "dsp/control_port.h"
#include "dsp/engine_jack.h"
#include "dsp/graph.h"
#include "dsp/hardware_processor.h"
#include "dsp/master_track.h"
#include "dsp/midi_event.h"
#include "dsp/pan.h"
#include "dsp/port.h"
#include "dsp/router.h"
#include "dsp/rtaudio_device.h"
#include "dsp/rtmidi_device.h"
#include "dsp/tempo_track.h"
#include "dsp/windows_mme_device.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/channel.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/lv2/lv2_ui.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/dsp.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/hash.h"
#include "utils/math.h"
#include "utils/mem.h"
#include "utils/mpmc_queue.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

#include "zix/ring.h"

#define SLEEPTIME_USEC 60

#define AUDIO_RING_SIZE 65536

/**
 * Allocates buffers used during DSP.
 *
 * To be called only where necessary to save
 * RAM.
 */
NONNULL void
port_allocate_bufs (Port * self)
{
#if 0
  size_t sz = 600;
  char str[sz];
  port_identifier_print_to_str (
    &self->id, str, sz);
  g_message ("allocating bufs for %s", str);
#endif

  switch (self->id.type)
    {
    case TYPE_EVENT:
      object_free_w_func_and_null (midi_events_free, self->midi_events);
      self->midi_events = midi_events_new ();
      object_free_w_func_and_null (zix_ring_free, self->midi_ring);
      self->midi_ring = zix_ring_new (
        zix_default_allocator (), sizeof (MidiEvent) * (size_t) 11);
      break;
    case TYPE_AUDIO:
    case TYPE_CV:
      {
        object_free_w_func_and_null (zix_ring_free, self->audio_ring);
        self->audio_ring = zix_ring_new (
          zix_default_allocator (), sizeof (float) * AUDIO_RING_SIZE);
        object_zero_and_free (self->buf);
        size_t max = MAX (AUDIO_ENGINE->block_length, self->min_buf_size);
        max = MAX (max, 1);
        self->buf = object_new_n (max, float);
        self->last_buf_sz = max;
      }
    default:
      break;
    }
}

/**
 * Frees buffers.
 *
 * To be used when removing ports from the
 * project/graph.
 */
void
port_free_bufs (Port * self)
{
  object_free_w_func_and_null (midi_events_free, self->midi_events);
  object_free_w_func_and_null (zix_ring_free, self->midi_ring);
  object_free_w_func_and_null (zix_ring_free, self->audio_ring);
  object_zero_and_free (self->buf);
}

/**
 * This function finds the Ports corresponding to
 * the PortIdentifiers for srcs and dests.
 *
 * Should be called after the ports are loaded from
 * yml.
 */
void
port_init_loaded (Port * self, void * owner)
{
  self->magic = PORT_MAGIC;

  self->unsnapped_control = self->control;

  port_set_owner (self, self->id.owner_type, owner);

#if 0
  if (self->track
      && self->id.flags & PORT_FLAG_AUTOMATABLE)
    {
      if (!self->at)
        {
          self->at =
            automation_track_find_from_port (
              self, NULL, false);
        }
      g_return_if_fail (self->at);
    }
#endif
}

/**
 * Finds the Port corresponding to the identifier.
 *
 * @param id The PortIdentifier to use for
 *   searching.
 */
Port *
port_find_from_identifier (const PortIdentifier * const id)
{
  Track *    tr = NULL;
  Channel *  ch = NULL;
  Plugin *   pl = NULL;
  Fader *    fader = NULL;
  PortFlags  flags = id->flags;
  PortFlags2 flags2 = id->flags2;
  switch (id->owner_type)
    {
    case PORT_OWNER_TYPE_AUDIO_ENGINE:
      switch (id->type)
        {
        case TYPE_EVENT:
          if (id->flow == FLOW_OUTPUT)
            { /* TODO */
            }
          else if (id->flow == FLOW_INPUT)
            {
              if (flags & PORT_FLAG_MANUAL_PRESS)
                return AUDIO_ENGINE->midi_editor_manual_press;
            }
          break;
        case TYPE_AUDIO:
          if (id->flow == FLOW_OUTPUT)
            {
              if (flags & PORT_FLAG_STEREO_L)
                return AUDIO_ENGINE->monitor_out->l;
              else if (flags & PORT_FLAG_STEREO_R)
                return AUDIO_ENGINE->monitor_out->r;
            }
          else if (id->flow == FLOW_INPUT)
            {
              /* none */
            }
          break;
        default:
          break;
        }
      break;
    case PORT_OWNER_TYPE_PLUGIN:
      tr = tracklist_find_track_by_name_hash (TRACKLIST, id->track_name_hash);
      if (!tr)
        tr = tracklist_find_track_by_name_hash (
          SAMPLE_PROCESSOR->tracklist, id->track_name_hash);
      g_return_val_if_fail (IS_TRACK_AND_NONNULL (tr), NULL);
      switch (id->plugin_id.slot_type)
        {
        case PLUGIN_SLOT_MIDI_FX:
          g_return_val_if_fail (tr->channel, NULL);
          pl = tr->channel->midi_fx[id->plugin_id.slot];
          break;
        case PLUGIN_SLOT_INSTRUMENT:
          g_return_val_if_fail (tr->channel, NULL);
          pl = tr->channel->instrument;
          break;
        case PLUGIN_SLOT_INSERT:
          g_return_val_if_fail (tr->channel, NULL);
          pl = tr->channel->inserts[id->plugin_id.slot];
          break;
        case PLUGIN_SLOT_MODULATOR:
          g_return_val_if_fail (tr->modulators, NULL);
          pl = tr->modulators[id->plugin_id.slot];
          break;
        default:
          g_return_val_if_reached (NULL);
          break;
        }
      if (!IS_PLUGIN_AND_NONNULL (pl))
        {
          g_critical (
            "could not find plugin for port of "
            "track %s",
            tr->name);
          return NULL;
        }

      switch (id->flow)
        {
        case FLOW_INPUT:
          return pl->in_ports[id->port_index];
          break;
        case FLOW_OUTPUT:
          return pl->out_ports[id->port_index];
          break;
        default:
          g_return_val_if_reached (NULL);
          break;
        }
      break;
    case PORT_OWNER_TYPE_TRACK_PROCESSOR:
      tr = tracklist_find_track_by_name_hash (TRACKLIST, id->track_name_hash);
      if (!tr)
        tr = tracklist_find_track_by_name_hash (
          SAMPLE_PROCESSOR->tracklist, id->track_name_hash);
      g_return_val_if_fail (IS_TRACK_AND_NONNULL (tr), NULL);
      switch (id->type)
        {
        case TYPE_EVENT:
          if (id->flow == FLOW_OUTPUT)
            {
              return tr->processor->midi_out;
            }
          else if (id->flow == FLOW_INPUT)
            {
              if (flags & PORT_FLAG_PIANO_ROLL)
                return tr->processor->piano_roll;
              else
                return tr->processor->midi_in;
            }
          break;
        case TYPE_AUDIO:
          if (id->flow == FLOW_OUTPUT)
            {
              if (flags & PORT_FLAG_STEREO_L)
                return tr->processor->stereo_out->l;
              else if (flags & PORT_FLAG_STEREO_R)
                return tr->processor->stereo_out->r;
            }
          else if (id->flow == FLOW_INPUT)
            {
              g_return_val_if_fail (tr->processor->stereo_in, NULL);
              if (flags & PORT_FLAG_STEREO_L)
                return tr->processor->stereo_in->l;
              else if (flags & PORT_FLAG_STEREO_R)
                return tr->processor->stereo_in->r;
            }
          break;
        case TYPE_CONTROL:
          if (flags & PORT_FLAG_TP_MONO)
            {
              return tr->processor->mono;
            }
          else if (flags & PORT_FLAG_TP_INPUT_GAIN)
            {
              return tr->processor->input_gain;
            }
          else if (flags2 & PORT_FLAG2_TP_OUTPUT_GAIN)
            {
              return tr->processor->output_gain;
            }
          else if (flags2 & PORT_FLAG2_TP_MONITOR_AUDIO)
            {
              return tr->processor->monitor_audio;
            }
          else if (flags & PORT_FLAG_MIDI_AUTOMATABLE)
            {
              if (flags2 & PORT_FLAG2_MIDI_PITCH_BEND)
                {
                  return tr->processor->pitch_bend[id->port_index];
                }
              else if (flags2 & PORT_FLAG2_MIDI_POLY_KEY_PRESSURE)
                {
                  return tr->processor->poly_key_pressure[id->port_index];
                }
              else if (flags2 & PORT_FLAG2_MIDI_CHANNEL_PRESSURE)
                {
                  return tr->processor->channel_pressure[id->port_index];
                }
              else
                {
                  return tr->processor->midi_cc[id->port_index];
                }
            }
          break;
        default:
          break;
        }
      break;
    case PORT_OWNER_TYPE_TRACK:
      tr = tracklist_find_track_by_name_hash (TRACKLIST, id->track_name_hash);
      if (!tr)
        tr = tracklist_find_track_by_name_hash (
          SAMPLE_PROCESSOR->tracklist, id->track_name_hash);
      g_return_val_if_fail (tr, NULL);
      if (flags & PORT_FLAG_BPM)
        {
          return tr->bpm_port;
        }
      else if (flags2 & PORT_FLAG2_BEATS_PER_BAR)
        {
          return tr->beats_per_bar_port;
        }
      else if (flags2 & PORT_FLAG2_BEAT_UNIT)
        {
          return tr->beat_unit_port;
        }
      else if (flags2 & PORT_FLAG2_TRACK_RECORDING)
        {
          return tr->recording;
        }
      break;
    case PORT_OWNER_TYPE_FADER:
      fader = fader_find_from_port_identifier (id);
      g_return_val_if_fail (fader, NULL);
      switch (id->type)
        {
        case TYPE_EVENT:
          switch (id->flow)
            {
            case FLOW_INPUT:
              if (fader)
                return fader->midi_in;
              break;
            case FLOW_OUTPUT:
              if (fader)
                return fader->midi_out;
              break;
            default:
              break;
            }
          break;
        case TYPE_AUDIO:
          if (id->flow == FLOW_OUTPUT)
            {
              if (flags & PORT_FLAG_STEREO_L && fader)
                return fader->stereo_out->l;
              else if (flags & PORT_FLAG_STEREO_R && fader)
                return fader->stereo_out->r;
            }
          else if (id->flow == FLOW_INPUT)
            {
              if (flags & PORT_FLAG_STEREO_L && fader)
                return fader->stereo_in->l;
              else if (flags & PORT_FLAG_STEREO_R && fader)
                return fader->stereo_in->r;
            }
          break;
        case TYPE_CONTROL:
          if (id->flow == FLOW_INPUT)
            {
              if (flags & PORT_FLAG_AMPLITUDE && fader)
                return fader->amp;
              else if (flags & PORT_FLAG_STEREO_BALANCE && fader)
                return fader->balance;
              else if (flags & PORT_FLAG_FADER_MUTE && fader)
                return fader->mute;
              else if (flags2 & PORT_FLAG2_FADER_SOLO && fader)
                return fader->solo;
              else if (flags2 & PORT_FLAG2_FADER_LISTEN && fader)
                return fader->listen;
              else if (flags2 & PORT_FLAG2_FADER_MONO_COMPAT && fader)
                return fader->mono_compat_enabled;
              else if (flags2 & PORT_FLAG2_FADER_SWAP_PHASE && fader)
                return fader->swap_phase;
            }
          break;
        default:
          break;
        }
      g_return_val_if_reached (NULL);
      break;
    case PORT_OWNER_TYPE_CHANNEL_SEND:
      tr = tracklist_find_track_by_name_hash (TRACKLIST, id->track_name_hash);
      if (!tr)
        tr = tracklist_find_track_by_name_hash (
          SAMPLE_PROCESSOR->tracklist, id->track_name_hash);
      g_return_val_if_fail (tr, NULL);
      ch = tr->channel;
      g_return_val_if_fail (ch, NULL);
      if (id->flags2 & PORT_FLAG2_CHANNEL_SEND_ENABLED)
        {
          return ch->sends[id->port_index]->enabled;
        }
      else if (id->flags2 & PORT_FLAG2_CHANNEL_SEND_AMOUNT)
        {
          return ch->sends[id->port_index]->amount;
        }
      else if (id->flow == FLOW_INPUT)
        {
          if (id->type == TYPE_AUDIO)
            {
              if (id->flags & PORT_FLAG_STEREO_L)
                {
                  return ch->sends[id->port_index]->stereo_in->l;
                }
              else if (id->flags & PORT_FLAG_STEREO_R)
                {
                  return ch->sends[id->port_index]->stereo_in->r;
                }
            }
          else if (id->type == TYPE_EVENT)
            {
              return ch->sends[id->port_index]->midi_in;
            }
        }
      else if (id->flow == FLOW_OUTPUT)
        {
          if (id->type == TYPE_AUDIO)
            {
              if (id->flags & PORT_FLAG_STEREO_L)
                {
                  return ch->sends[id->port_index]->stereo_out->l;
                }
              else if (id->flags & PORT_FLAG_STEREO_R)
                {
                  return ch->sends[id->port_index]->stereo_out->r;
                }
            }
          else if (id->type == TYPE_EVENT)
            {
              return ch->sends[id->port_index]->midi_out;
            }
        }
      else
        {
          g_return_val_if_reached (NULL);
        }
      break;
    case PORT_OWNER_TYPE_HW:
      {
        Port * port = NULL;

        /* note: flows are reversed */
        if (id->flow == FLOW_OUTPUT)
          {
            port =
              hardware_processor_find_port (HW_IN_PROCESSOR, id->ext_port_id);
          }
        else if (id->flow == FLOW_INPUT)
          {
            port =
              hardware_processor_find_port (HW_OUT_PROCESSOR, id->ext_port_id);
          }

        /* only warn when hardware is not
         * connected anymore */
        g_warn_if_fail (port);
        /*g_return_val_if_fail (port, NULL);*/
        return port;
      }
      break;
    case PORT_OWNER_TYPE_TRANSPORT:
      switch (id->type)
        {
        case TYPE_EVENT:
          if (id->flow == FLOW_INPUT)
            {
              if (flags2 & PORT_FLAG2_TRANSPORT_ROLL)
                return TRANSPORT->roll;
              if (flags2 & PORT_FLAG2_TRANSPORT_STOP)
                return TRANSPORT->stop;
              if (flags2 & PORT_FLAG2_TRANSPORT_BACKWARD)
                return TRANSPORT->backward;
              if (flags2 & PORT_FLAG2_TRANSPORT_FORWARD)
                return TRANSPORT->forward;
              if (flags2 & PORT_FLAG2_TRANSPORT_LOOP_TOGGLE)
                return TRANSPORT->loop_toggle;
              if (flags2 & PORT_FLAG2_TRANSPORT_REC_TOGGLE)
                return TRANSPORT->rec_toggle;
            }
          break;
        default:
          break;
        }
      break;
    case PORT_OWNER_TYPE_MODULATOR_MACRO_PROCESSOR:
      if (flags & PORT_FLAG_MODULATOR_MACRO)
        {
          tr =
            tracklist_find_track_by_name_hash (TRACKLIST, id->track_name_hash);
          g_return_val_if_fail (IS_TRACK_AND_NONNULL (tr), NULL);
          ModulatorMacroProcessor * processor =
            tr->modulator_macros[id->port_index];
          if (id->flow == FLOW_INPUT)
            {
              if (id->type == TYPE_CV)
                {
                  return processor->cv_in;
                }
              else if (id->type == TYPE_CONTROL)
                {
                  return processor->macro;
                }
            }
          else if (id->flow == FLOW_OUTPUT)
            {
              return processor->cv_out;
            }
        }
      break;
    case PORT_OWNER_TYPE_CHANNEL:
      tr = tracklist_find_track_by_name_hash (TRACKLIST, id->track_name_hash);
      if (!tr)
        tr = tracklist_find_track_by_name_hash (
          SAMPLE_PROCESSOR->tracklist, id->track_name_hash);
      g_return_val_if_fail (tr, NULL);
      ch = tr->channel;
      g_return_val_if_fail (ch, NULL);
      switch (id->type)
        {
        case TYPE_EVENT:
          if (id->flow == FLOW_OUTPUT)
            {
              return ch->midi_out;
            }
          break;
        case TYPE_AUDIO:
          if (id->flow == FLOW_OUTPUT)
            {
              if (flags & PORT_FLAG_STEREO_L)
                return ch->stereo_out->l;
              else if (flags & PORT_FLAG_STEREO_R)
                return ch->stereo_out->r;
            }
          break;
        default:
          break;
        }
      break;
    default:
      g_return_val_if_reached (NULL);
    }

  g_return_val_if_reached (NULL);
}

/**
 * Creates port.
 *
 * Sets id and updates appropriate counters.
 */
RETURNS_NONNULL
static Port *
_port_new (const char * label)
{
  Port * self = object_new (Port);

  /*self->schema_version = PORT_SCHEMA_VERSION;*/
  port_identifier_init (&self->id);
  self->magic = PORT_MAGIC;

  self->num_dests = 0;
  self->id.flow = FLOW_UNKNOWN;
  self->id.label = g_strdup (label);

  return self;
}

/**
 * Creates port.
 */
Port *
port_new_with_type (PortType type, PortFlow flow, const char * label)
{
  Port * self = _port_new (label);

  self->id.type = type;
  self->id.flow = flow;

  switch (type)
    {
    case TYPE_EVENT:
      self->maxf = 1.f;
#ifdef _WOE32
      if (AUDIO_ENGINE->midi_backend == MIDI_BACKEND_WINDOWS_MME)
        {
          zix_sem_init (&self->mme_connections_sem, 1);
        }
#endif
      break;
    case TYPE_CONTROL:
      self->minf = 0.f;
      self->maxf = 1.f;
      self->zerof = 0.f;
      break;
    case TYPE_AUDIO:
      self->minf = 0.f;
      self->maxf = 2.f;
      self->zerof = 0.f;
      break;
    case TYPE_CV:
      self->minf = -1.f;
      self->maxf = 1.f;
      self->zerof = 0.f;
    default:
      break;
    }

  g_return_val_if_fail (IS_PORT (self), NULL);

  g_return_val_if_fail (self->magic == PORT_MAGIC, NULL);

  return self;
}

Port *
port_new_with_type_and_owner (
  PortType      type,
  PortFlow      flow,
  const char *  label,
  PortOwnerType owner_type,
  void *        owner)
{
  Port * self = port_new_with_type (type, flow, label);
  port_set_owner (self, owner_type, owner);

  return self;
}

/**
 * Creates stereo ports.
 */
StereoPorts *
stereo_ports_new_from_existing (Port * l, Port * r)
{
  StereoPorts * sp = object_new (StereoPorts);
  /*sp->schema_version = STEREO_PORTS_SCHEMA_VERSION;*/
  sp->l = l;
  l->id.flags |= PORT_FLAG_STEREO_L;
  r->id.flags |= PORT_FLAG_STEREO_R;
  sp->r = r;

  return sp;
}

void
stereo_ports_disconnect (StereoPorts * self)
{
  port_disconnect_all (self->l);
  port_disconnect_all (self->r);
}

StereoPorts *
stereo_ports_clone (const StereoPorts * src)
{
  StereoPorts * sp = object_new (StereoPorts);
  /*sp->schema_version = STEREO_PORTS_SCHEMA_VERSION;*/

  sp->l = port_clone (src->l);
  sp->r = port_clone (src->r);

  return sp;
}

void
stereo_ports_free (StereoPorts * self)
{
  object_free_w_func_and_null (port_free, self->l);
  object_free_w_func_and_null (port_free, self->r);

  object_zero_and_free (self);
}

#ifdef HAVE_JACK
void
port_receive_midi_events_from_jack (
  Port *          self,
  const nframes_t start_frame,
  const nframes_t nframes)
{
  if (self->internal_type != INTERNAL_JACK_PORT || self->id.type != TYPE_EVENT)
    return;

  void *   port_buf = jack_port_get_buffer (JACK_PORT_T (self->data), nframes);
  uint32_t num_events = jack_midi_get_event_count (port_buf);

  jack_midi_event_t jack_ev;
  for (unsigned i = 0; i < num_events; i++)
    {
      jack_midi_event_get (&jack_ev, port_buf, i);

      if (jack_ev.time >= start_frame && jack_ev.time < start_frame + nframes)
        {
          midi_byte_t channel = jack_ev.buffer[0] & 0xf;
          Track *     track = port_get_track (self, 0);
          if (self->id.owner_type == PORT_OWNER_TYPE_TRACK_PROCESSOR && !track)
            {
              g_return_if_reached ();
            }

          if (
            self->id.owner_type == PORT_OWNER_TYPE_TRACK_PROCESSOR && track
            && (track->type == TRACK_TYPE_MIDI || track->type == TRACK_TYPE_INSTRUMENT)
            && !track->channel->all_midi_channels
            && !track->channel->midi_channels[channel])
            {
              /* different channel */
              /*g_debug ("received event on different channel");*/
            }
          else if (jack_ev.size == 3)
            {
              /*g_debug ("received event at %u", jack_ev.time);*/
              midi_events_add_event_from_buf (
                self->midi_events, jack_ev.time, jack_ev.buffer,
                (int) jack_ev.size, F_NOT_QUEUED);
            }
        }
    }

#  if 0
  if (midi_events_has_any (self->midi_events, F_NOT_QUEUED))
    {
      char designation[600];
      port_get_full_designation (self, designation);
      g_debug ("JACK MIDI (%s): have %d events", designation, num_events);
      midi_events_print (self->midi_events, F_NOT_QUEUED);
    }
#  endif
}

void
port_receive_audio_data_from_jack (
  Port *          self,
  const nframes_t start_frames,
  const nframes_t nframes)
{
  if (self->internal_type != INTERNAL_JACK_PORT || self->id.type != TYPE_AUDIO)
    return;

  float * in;
  in = (float *) jack_port_get_buffer (
    JACK_PORT_T (self->data), AUDIO_ENGINE->nframes);

  dsp_add2 (&self->buf[start_frames], &in[start_frames], nframes);
}

/**
 * Pastes the MIDI data in the port to the JACK port.
 *
 * @note MIDI timings are assumed to be at the correct
 *   positions in the current cycle (ie, already added
 *   the start_frames in this cycle).
 */
static void
send_midi_events_to_jack (
  Port *          port,
  const nframes_t start_frames,
  const nframes_t nframes)
{
  if (port->internal_type != INTERNAL_JACK_PORT || port->id.type != TYPE_EVENT)
    return;

  jack_port_t * jport = JACK_PORT_T (port->data);

  if (jack_port_connected (jport) <= 0)
    {
      return;
    }

  void * buf = jack_port_get_buffer (jport, AUDIO_ENGINE->nframes);
  midi_events_copy_to_jack (port->midi_events, start_frames, nframes, buf);
}

/**
 * Pastes the audio data in the port starting at
 * @ref start_frames to the JACK port starting at
 * @ref start_frames.
 */
static void
send_audio_data_to_jack (
  Port *          port,
  const nframes_t start_frames,
  const nframes_t nframes)
{
  if (port->internal_type != INTERNAL_JACK_PORT || port->id.type != TYPE_AUDIO)
    return;

  jack_port_t * jport = JACK_PORT_T (port->data);

  if (jack_port_connected (jport) <= 0)
    {
      return;
    }

  float * out = (float *) jack_port_get_buffer (jport, AUDIO_ENGINE->nframes);

  dsp_copy (&out[start_frames], &port->buf[start_frames], nframes);
}

/**
 * Sums the inputs coming in from JACK, before the
 * port is processed.
 */
static void
sum_data_from_jack (
  Port *          self,
  const nframes_t start_frame,
  const nframes_t nframes)
{
  if (
    self->id.owner_type == PORT_OWNER_TYPE_AUDIO_ENGINE
    || self->internal_type != INTERNAL_JACK_PORT || self->id.flow != FLOW_INPUT)
    return;

  /* append events from JACK if any */
  if (AUDIO_ENGINE->midi_backend == MIDI_BACKEND_JACK)
    {
      port_receive_midi_events_from_jack (self, start_frame, nframes);
    }

  /* audio */
  if (AUDIO_ENGINE->audio_backend == AUDIO_BACKEND_JACK)
    {
      port_receive_audio_data_from_jack (self, start_frame, nframes);
    }
}

/**
 * Sends the port data to JACK, after the port
 * is processed.
 */
static void
send_data_to_jack (
  Port *          self,
  const nframes_t start_frame,
  const nframes_t nframes)
{
  if (self->internal_type != INTERNAL_JACK_PORT || self->id.flow != FLOW_OUTPUT)
    return;

  /* send midi events */
  if (AUDIO_ENGINE->midi_backend == MIDI_BACKEND_JACK)
    {
      send_midi_events_to_jack (self, start_frame, nframes);
    }

  /* send audio data */
  if (AUDIO_ENGINE->audio_backend == AUDIO_BACKEND_JACK)
    {
      send_audio_data_to_jack (self, start_frame, nframes);
    }
}

/**
 * Sets whether to expose the port to JACk and
 * exposes it or removes it from JACK.
 */
static void
expose_to_jack (Port * self, bool expose)
{
  enum JackPortFlags flags;
  if (self->id.owner_type == PORT_OWNER_TYPE_HW)
    {
      /* these are reversed */
      if (self->id.flow == FLOW_INPUT)
        flags = JackPortIsOutput;
      else if (self->id.flow == FLOW_OUTPUT)
        flags = JackPortIsInput;
      else
        {
          g_return_if_reached ();
        }
    }
  else
    {
      if (self->id.flow == FLOW_INPUT)
        flags = JackPortIsInput;
      else if (self->id.flow == FLOW_OUTPUT)
        flags = JackPortIsOutput;
      else
        {
          g_return_if_reached ();
        }
    }

  const char * type = engine_jack_get_jack_type (self->id.type);
  if (!type)
    g_return_if_reached ();

  char label[600];
  port_get_full_designation (self, label);
  if (expose)
    {
      g_message ("exposing port %s to JACK", label);
      if (!self->data)
        {
          self->data = (void *) jack_port_register (
            AUDIO_ENGINE->client, label, type, flags, 0);
        }
      g_return_if_fail (self->data);
      self->internal_type = INTERNAL_JACK_PORT;
    }
  else
    {
      g_message ("unexposing port %s from JACK", label);
      if (AUDIO_ENGINE->client)
        {
          g_warn_if_fail (self->data);
          int ret = jack_port_unregister (
            AUDIO_ENGINE->client, JACK_PORT_T (self->data));
          if (ret)
            {
              char jack_error[600];
              engine_jack_get_error_message ((jack_status_t) ret, jack_error);
              g_warning ("JACK port unregister error: %s", jack_error);
            }
        }
      self->internal_type = INTERNAL_NONE;
      self->data = NULL;
    }

  self->exposed_to_backend = expose;
}
#endif /* HAVE_JACK */

/**
 * Sums the inputs coming in from dummy, before the
 * port is processed.
 */
static void
sum_data_from_dummy (
  Port *          self,
  const nframes_t start_frame,
  const nframes_t nframes)
{
  if (
    self->id.owner_type == PORT_OWNER_TYPE_AUDIO_ENGINE
    || self->id.flow != FLOW_INPUT || self->id.type != TYPE_AUDIO
    || AUDIO_ENGINE->audio_backend != AUDIO_BACKEND_DUMMY
    || AUDIO_ENGINE->midi_backend != MIDI_BACKEND_DUMMY)
    return;

  if (AUDIO_ENGINE->dummy_input)
    {
      Port * port = NULL;
      if (self->id.flags & PORT_FLAG_STEREO_L)
        {
          port = AUDIO_ENGINE->dummy_input->l;
        }
      else if (self->id.flags & PORT_FLAG_STEREO_R)
        {
          port = AUDIO_ENGINE->dummy_input->r;
        }

      if (port)
        {
          dsp_add2 (&self->buf[start_frame], &port->buf[start_frame], nframes);
        }
    }
}

/**
 * Connects the internal ports using
 * port_connect().
 *
 * @param locked Lock the connection.
 */
void
stereo_ports_connect (StereoPorts * src, StereoPorts * dest, int locked)
{
  port_connect (src->l, dest->l, locked);
  port_connect (src->r, dest->r, locked);
}

static int
get_num_unlocked (const Port * self, bool sources)
{
  g_return_val_if_fail (
    IS_PORT_AND_NONNULL (self) && port_is_in_active_project (self), 0);

  int num_unlocked_conns =
    port_connections_manager_get_unlocked_sources_or_dests (
      PORT_CONNECTIONS_MGR, NULL, &self->id, sources);

  return num_unlocked_conns;
}

/**
 * Returns the number of unlocked (user-editable)
 * destinations.
 */
int
port_get_num_unlocked_dests (const Port * self)
{
  return get_num_unlocked (self, false);
}

/**
 * Returns the number of unlocked (user-editable)
 * sources.
 */
int
port_get_num_unlocked_srcs (const Port * self)
{
  return get_num_unlocked (self, true);
}

/**
 * Gathers all ports in the project and appends them
 * in the given array.
 */
void
port_get_all (GPtrArray * ports)
{
#define _ADD(port) \
  g_return_if_fail (port); \
  g_ptr_array_add (ports, port)

  engine_append_ports (AUDIO_ENGINE, ports);

  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * tr = TRACKLIST->tracks[i];
      track_append_ports (tr, ports, F_INCLUDE_PLUGINS);
    }

#undef _ADD
}

/**
 * Sets the owner plugin & its slot.
 */
NONNULL static void
set_owner_plugin (Port * port, Plugin * pl)
{
  plugin_identifier_copy (&port->id.plugin_id, &pl->id);
  port->id.track_name_hash = pl->id.track_name_hash;
  port->id.owner_type = PORT_OWNER_TYPE_PLUGIN;

  if (port->at)
    {
      port_identifier_copy (&port->at->port_id, &port->id);
    }
}

/**
 * Sets the owner track & its ID.
 */
NONNULL static void
set_owner_track_processor (Port * port, TrackProcessor * track_processor)
{
  Track * track = track_processor->track;
  g_return_if_fail (track && track->name);
  port->id.track_name_hash = track_get_name_hash (track);
  port->id.owner_type = PORT_OWNER_TYPE_TRACK_PROCESSOR;
}

/**
 * Sets the owner fader & its ID.
 */
static void
set_owner_fader (Port * self, Fader * fader)
{
  PortIdentifier * id = &self->id;
  id->owner_type = PORT_OWNER_TYPE_FADER;
  self->fader = fader;

  if (
    fader->type == FADER_TYPE_AUDIO_CHANNEL
    || fader->type == FADER_TYPE_MIDI_CHANNEL)
    {
      Track * track = fader->track;
      g_return_if_fail (track && track->name);
      self->id.track_name_hash = track_get_name_hash (track);
      if (fader->passthrough)
        {
          id->flags2 |= PORT_FLAG2_PREFADER;
        }
      else
        {
          id->flags2 |= PORT_FLAG2_POSTFADER;
        }
    }
  else if (fader->type == FADER_TYPE_SAMPLE_PROCESSOR)
    {
      id->flags2 |= PORT_FLAG2_SAMPLE_PROCESSOR_FADER;
    }
  else
    {
      id->flags2 |= PORT_FLAG2_MONITOR_FADER;
    }

  if (id->flags & PORT_FLAG_AMPLITUDE)
    {
      self->minf = 0.f;
      self->maxf = 2.f;
      self->zerof = 0.f;
    }
  else if (id->flags & PORT_FLAG_STEREO_BALANCE)
    {
      self->minf = 0.f;
      self->maxf = 1.f;
      self->zerof = 0.5f;
    }
}

/**
 * Sets the owner track & its ID.
 */
NONNULL static void
set_owner_track (Port * port, Track * track)
{
  g_return_if_fail (track->name);
  port->id.track_name_hash = track_get_name_hash (track);
  port->id.owner_type = PORT_OWNER_TYPE_TRACK;
}

/**
 * Sets the channel send as the port's owner.
 */
NONNULL static void
set_owner_channel_send (Port * self, ChannelSend * send)
{
  self->id.track_name_hash = send->track_name_hash;
  self->id.port_index = send->slot;
  self->id.owner_type = PORT_OWNER_TYPE_CHANNEL_SEND;
  self->channel_send = send;

  if (self->id.flags2 & PORT_FLAG2_CHANNEL_SEND_ENABLED)
    {
      self->minf = 0.f;
      self->maxf = 1.f;
      self->zerof = 0.0f;
    }
  else if (self->id.flags2 & PORT_FLAG2_CHANNEL_SEND_AMOUNT)
    {
      self->minf = 0.f;
      self->maxf = 2.f;
      self->zerof = 0.f;
    }
}

NONNULL static void
set_owner_channel (Port * port, Channel * ch)
{
  Track * track = ch->track;
  g_return_if_fail (track && track->name);
  port->id.track_name_hash = track_get_name_hash (track);
  port->id.owner_type = PORT_OWNER_TYPE_CHANNEL;
}

NONNULL static void
set_owner_transport (Port * self, Transport * transport)
{
  self->transport = transport;
  self->id.owner_type = PORT_OWNER_TYPE_TRANSPORT;
}

NONNULL static void
set_owner_modulator_macro_processor (Port * self, ModulatorMacroProcessor * mmp)
{
  self->modulator_macro_processor = mmp;
  self->id.owner_type = PORT_OWNER_TYPE_MODULATOR_MACRO_PROCESSOR;
  g_return_if_fail (IS_TRACK_AND_NONNULL (mmp->track));
  self->id.track_name_hash = track_get_name_hash (mmp->track);
  self->track = mmp->track;
}

NONNULL static void
set_owner_audio_engine (Port * self, AudioEngine * engine)
{
  self->engine = engine;
  self->id.owner_type = PORT_OWNER_TYPE_AUDIO_ENGINE;
}

NONNULL static void
set_owner_ext_port (Port * self, ExtPort * ext_port)
{
  self->ext_port = ext_port;
  self->id.owner_type = PORT_OWNER_TYPE_HW;
}

NONNULL void
port_set_owner (Port * self, PortOwnerType owner_type, void * owner)
{
  switch (owner_type)
    {
    case PORT_OWNER_TYPE_CHANNEL_SEND:
      set_owner_channel_send (self, (ChannelSend *) owner);
      break;
    case PORT_OWNER_TYPE_FADER:
      set_owner_fader (self, (Fader *) owner);
      break;
    case PORT_OWNER_TYPE_TRACK:
      set_owner_track (self, (Track *) owner);
      break;
    case PORT_OWNER_TYPE_TRACK_PROCESSOR:
      set_owner_track_processor (self, (TrackProcessor *) owner);
      break;
    case PORT_OWNER_TYPE_CHANNEL:
      set_owner_channel (self, (Channel *) owner);
      break;
    case PORT_OWNER_TYPE_PLUGIN:
      set_owner_plugin (self, (Plugin *) owner);
      break;
    case PORT_OWNER_TYPE_TRANSPORT:
      set_owner_transport (self, (Transport *) owner);
      break;
    case PORT_OWNER_TYPE_MODULATOR_MACRO_PROCESSOR:
      set_owner_modulator_macro_processor (
        self, (ModulatorMacroProcessor *) owner);
      break;
    case PORT_OWNER_TYPE_AUDIO_ENGINE:
      set_owner_audio_engine (self, (AudioEngine *) owner);
      break;
    case PORT_OWNER_TYPE_HW:
      set_owner_ext_port (self, (ExtPort *) owner);
      break;
    default:
      g_return_if_reached ();
    }
}

/**
 * Returns whether the Port's can be connected (if
 * the connection will be valid and won't break the
 * acyclicity of the graph).
 */
bool
ports_can_be_connected (const Port * src, const Port * dest)
{
  Graph * graph = graph_new (ROUTER);
  bool    valid = graph_validate_with_connection (graph, src, dest);
  graph_free (graph);

  return valid;
}

/**
 * Disconnects all the given ports.
 */
void
ports_disconnect (Port ** ports, int num_ports, int deleting)
{
  if (!PORT_CONNECTIONS_MGR)
    return;

  /* can only be called from the gtk thread */
  g_return_if_fail (ZRYTHM_APP_IS_GTK_THREAD);

  /* go through each port */
  for (int i = 0; i < num_ports; i++)
    {
      Port * port = ports[i];

      port_connections_manager_ensure_disconnect_all (
        PORT_CONNECTIONS_MGR, &port->id);

      port->num_srcs = 0;
      port->num_dests = 0;
      port->deleting = deleting;
    }
}

/**
 * Disconnects all srcs and dests from port.
 */
int
port_disconnect_all (Port * self)
{
  g_return_val_if_fail (IS_PORT (self), -1);

  self->num_srcs = 0;
  self->num_dests = 0;

  if (!PORT_CONNECTIONS_MGR)
    return 0;

  if (!port_is_in_active_project (self))
    {
#if 0
      g_debug (
        "%s (%p) is not a project port, "
        "skipping",
        self->id.label, self);
#endif
      return 0;
    }

  GPtrArray * srcs = g_ptr_array_new ();
  int         num_srcs = port_connections_manager_get_sources_or_dests (
    PORT_CONNECTIONS_MGR, srcs, &self->id, true);
  for (int i = 0; i < num_srcs; i++)
    {
      PortConnection * conn = (PortConnection *) g_ptr_array_index (srcs, i);
      port_connections_manager_ensure_disconnect (
        PORT_CONNECTIONS_MGR, conn->src_id, conn->dest_id);
    }
  g_ptr_array_unref (srcs);

  GPtrArray * dests = g_ptr_array_new ();
  int         num_dests = port_connections_manager_get_sources_or_dests (
    PORT_CONNECTIONS_MGR, dests, &self->id, false);
  for (int i = 0; i < num_dests; i++)
    {
      PortConnection * conn = (PortConnection *) g_ptr_array_index (dests, i);
      port_connections_manager_ensure_disconnect (
        PORT_CONNECTIONS_MGR, conn->src_id, conn->dest_id);
    }
  g_ptr_array_unref (dests);

#ifdef HAVE_JACK
  if (self->internal_type == INTERNAL_JACK_PORT)
    {
      expose_to_jack (self, false);
    }
#endif

#ifdef HAVE_RTMIDI
  for (int i = self->num_rtmidi_ins - 1; i >= 0; i--)
    {
      rtmidi_device_close (self->rtmidi_ins[i], 1);
      self->num_rtmidi_ins--;
    }
#endif

  return 0;
}

/**
 * To be called when the port's identifier changes to update
 * corresponding identifiers.
 *
 * @param prev_id Previous identifier to be used
 *   for searching.
 * @param track The track that owns this port.
 * @param update_automation_track Whether to update
 *   the identifier in the corresponding automation
 *   track as well. This should be false when
 *   moving a plugin.
 */
void
port_update_identifier (
  Port *                 self,
  const PortIdentifier * prev_id,
  Track *                track,
  bool                   update_automation_track)
{
  /*g_message (*/
  /*"updating identifier for %p %s (track pos %d)", */
  /*self, self->id.label, self->id.track_pos);*/

  if (port_is_in_active_project (self))
    {
      /* update in all sources */
      GPtrArray * srcs = g_ptr_array_new ();
      int         num_srcs = port_connections_manager_get_sources_or_dests (
        PORT_CONNECTIONS_MGR, srcs, prev_id, true);
      for (int i = 0; i < num_srcs; i++)
        {
          PortConnection * conn = (PortConnection *) g_ptr_array_index (srcs, i);
          if (!port_identifier_is_equal (conn->dest_id, &self->id))
            {
              port_identifier_copy (conn->dest_id, &self->id);
              port_connections_manager_regenerate_hashtables (
                PORT_CONNECTIONS_MGR);
            }
        }
      g_ptr_array_unref (srcs);

      /* update in all dests */
      GPtrArray * dests = g_ptr_array_new ();
      int         num_dests = port_connections_manager_get_sources_or_dests (
        PORT_CONNECTIONS_MGR, dests, prev_id, false);
      for (int i = 0; i < num_dests; i++)
        {
          PortConnection * conn =
            (PortConnection *) g_ptr_array_index (dests, i);
          if (!port_identifier_is_equal (conn->src_id, &self->id))
            {
              port_identifier_copy (conn->src_id, &self->id);
              port_connections_manager_regenerate_hashtables (
                PORT_CONNECTIONS_MGR);
            }
        }
      g_ptr_array_unref (dests);

      if (
        update_automation_track && self->id.track_name_hash
        && self->id.flags & PORT_FLAG_AUTOMATABLE)
        {
          /* update automation track's port id */
          self->at = automation_track_find_from_port (self, track, true);
          AutomationTrack * at = self->at;
          g_return_if_fail (at);
          port_identifier_copy (&at->port_id, &self->id);
        }
    }

#if 0
  if (self->lv2_port)
    {
      Lv2Port * port = self->lv2_port;
      port_identifier_copy (
        &port->port_id, &self->id);
    }
#endif
}

/**
 * Updates the track name hash on a track port and
 * all its source/destination identifiers.
 *
 * @param track Owner track.
 * @param hash The new hash.
 */
void
port_update_track_name_hash (Port * self, Track * track, unsigned int new_hash)
{
  PortIdentifier * copy = port_identifier_clone (&self->id);

  self->id.track_name_hash = new_hash;
  if (self->id.owner_type == PORT_OWNER_TYPE_PLUGIN)
    {
      self->id.plugin_id.track_name_hash = new_hash;
    }
  port_update_identifier (self, copy, track, F_UPDATE_AUTOMATION_TRACK);
  object_free_w_func_and_null (port_identifier_free, copy);
}

#ifdef HAVE_ALSA
#  if 0

/**
 * Returns the Port matching the given ALSA
 * sequencer port ID.
 */
Port *
port_find_by_alsa_seq_id (
  const int id)
{
  Port * ports[10000];
  int num_ports;
  Port * port;
  port_get_all (ports, &num_ports);
  for (int i = 0; i < num_ports; i++)
    {
      port = ports[i];

      if (port->internal_type ==
            INTERNAL_ALSA_SEQ_PORT &&
          snd_seq_port_info_get_port (
            (snd_seq_port_info_t *) port->data) ==
            id)
        return port;
    }
  g_return_val_if_reached (NULL);
}

static void
expose_to_alsa (
  Port * self,
  int    expose)
{
  if (self->id.type == TYPE_EVENT)
    {
      unsigned int flags = 0;
      if (self->id.flow == FLOW_INPUT)
        flags =
          SND_SEQ_PORT_CAP_WRITE |
          SND_SEQ_PORT_CAP_SUBS_WRITE;
      else if (self->id.flow == FLOW_OUTPUT)
        flags =
          SND_SEQ_PORT_CAP_READ |
          SND_SEQ_PORT_CAP_SUBS_READ;
      else
        g_return_if_reached ();

      if (expose)
        {
          char lbl[600];
          port_get_full_designation (self, lbl);

          g_return_if_fail (
            AUDIO_ENGINE->seq_handle);

          int id =
            snd_seq_create_simple_port (
              AUDIO_ENGINE->seq_handle,
              lbl, flags,
              SND_SEQ_PORT_TYPE_APPLICATION);
          g_return_if_fail (id >= 0);
          snd_seq_port_info_t * info;
          snd_seq_port_info_malloc (&info);
          snd_seq_get_port_info (
            AUDIO_ENGINE->seq_handle,
            id, info);
          self->data = (void *) info;
          self->internal_type =
            INTERNAL_ALSA_SEQ_PORT;
        }
      else
        {
          snd_seq_delete_port (
            AUDIO_ENGINE->seq_handle,
            snd_seq_port_info_get_port (
              (snd_seq_port_info_t *)
                self->data));
          self->internal_type = INTERNAL_NONE;
          self->data = NULL;
        }
    }
  self->exposed_to_backend = expose;
}
#  endif
#endif // HAVE_ALSA

#ifdef HAVE_RTMIDI
static void
expose_to_rtmidi (Port * self, int expose)
{
  char lbl[600];
  port_get_full_designation (self, lbl);
  if (expose)
    {
#  if 0

      if (self->id.flow == FLOW_INPUT)
        {
          self->rtmidi_in =
            rtmidi_in_create (
#    ifdef _WOE32
              RTMIDI_API_WINDOWS_MM,
#    elif defined(__APPLE__)
              RTMIDI_API_MACOSX_CORE,
#    else
              RTMIDI_API_LINUX_ALSA,
#    endif
              "Zrythm",
              AUDIO_ENGINE->midi_buf_size);

          /* don't ignore any messages */
          rtmidi_in_ignore_types (
            self->rtmidi_in, 0, 0, 0);

          rtmidi_open_port (
            self->rtmidi_in, 1, lbl);
        }
#  endif
      g_message ("exposing %s", lbl);
    }
  else
    {
#  if 0
      if (self->id.flow == FLOW_INPUT &&
          self->rtmidi_in)
        {
          rtmidi_close_port (self->rtmidi_in);
          self->rtmidi_in = NULL;
        }
#  endif
      g_message ("unexposing %s", lbl);
    }
  self->exposed_to_backend = expose;
}

/**
 * Sums the inputs coming in from RtMidi
 * before the port is processed.
 */
void
port_sum_data_from_rtmidi (
  Port *          self,
  const nframes_t start_frame,
  const nframes_t nframes)
{
  g_return_if_fail (
    /*self->id.flow == FLOW_INPUT &&*/
    midi_backend_is_rtmidi (AUDIO_ENGINE->midi_backend));

  for (int i = 0; i < self->num_rtmidi_ins; i++)
    {
      RtMidiDevice * dev = self->rtmidi_ins[i];
      MidiEvent *    ev;
      for (int j = 0; j < dev->events->num_events; j++)
        {
          ev = &dev->events->events[j];

          if (ev->time >= start_frame && ev->time < start_frame + nframes)
            {
              midi_byte_t channel = ev->raw_buffer[0] & 0xf;
              Track *     track = port_get_track (self, 0);
              if (
                self->id.owner_type == PORT_OWNER_TYPE_TRACK_PROCESSOR && !track)
                {
                  g_return_if_reached ();
                }

              if (
                self->id.owner_type == PORT_OWNER_TYPE_TRACK_PROCESSOR && track
                && (track->type == TRACK_TYPE_MIDI || track->type == TRACK_TYPE_INSTRUMENT)
                && !track->channel->all_midi_channels
                && !track->channel->midi_channels[channel])
                {
                  /* different channel */
                }
              else
                {
                  midi_events_add_event_from_buf (
                    self->midi_events, ev->time, ev->raw_buffer, 3,
                    F_NOT_QUEUED);
                }
            }
        }
    }

#  if 0
  if (DEBUGGING &&
      self->midi_events->num_events > 0)
    {
      MidiEvent * ev =
        &self->midi_events->events[0];
      char designation[600];
      port_get_full_designation (
        self, designation);
      g_message (
        "RtMidi (%s): have %d events\n"
        "first event is: [%u] %hhx %hhx %hhx",
        designation, self->midi_events->num_events,
        ev->time, ev->raw_buffer[0],
        ev->raw_buffer[1], ev->raw_buffer[2]);
    }
#  endif
}

/**
 * Dequeue the midi events from the ring
 * buffers into \ref RtMidiDevice.events.
 */
void
port_prepare_rtmidi_events (Port * self)
{
  g_return_if_fail (
    /*self->id.flow == FLOW_INPUT &&*/
    midi_backend_is_rtmidi (AUDIO_ENGINE->midi_backend));

  gint64 cur_time = g_get_monotonic_time ();
  for (int i = 0; i < self->num_rtmidi_ins; i++)
    {
      RtMidiDevice * dev = self->rtmidi_ins[i];

      /* clear the events */
      midi_events_clear (dev->events, 0);

      uint32_t read_space = 0;
      zix_sem_wait (&dev->midi_ring_sem);
      do
        {
          read_space = zix_ring_read_space (dev->midi_ring);
          if (read_space <= sizeof (MidiEventHeader))
            {
              /* no more events */
              break;
            }

          /* peek the next event header to check
           * the time */
          MidiEventHeader h = { 0, 0 };
          zix_ring_peek (dev->midi_ring, &h, sizeof (h));
          g_return_if_fail (h.size > 0);

          /* read event header */
          zix_ring_read (dev->midi_ring, &h, sizeof (h));

          /* read event body */
          midi_byte_t raw[h.size];
          zix_ring_read (dev->midi_ring, raw, sizeof (raw));

          /* calculate event timestamp */
          gint64      length = cur_time - self->last_midi_dequeue;
          midi_time_t ev_time =
            (midi_time_t) (((double) h.time / (double) length)
                           * (double) AUDIO_ENGINE->block_length);

          if (ev_time >= AUDIO_ENGINE->block_length)
            {
              g_warning (
                "event with invalid time %u "
                "received. the maximum allowed time "
                "is %" PRIu32
                ". setting it to "
                "%u...",
                ev_time, AUDIO_ENGINE->block_length - 1,
                AUDIO_ENGINE->block_length - 1);
              ev_time = (midi_byte_t) (AUDIO_ENGINE->block_length - 1);
            }

          midi_events_add_event_from_buf (
            dev->events, ev_time, raw, (int) h.size, F_NOT_QUEUED);
        }
      while (read_space > sizeof (MidiEventHeader));
      zix_sem_post (&dev->midi_ring_sem);
    }
  self->last_midi_dequeue = cur_time;
}
#endif // HAVE_RTMIDI

#ifdef HAVE_RTAUDIO
static void
expose_to_rtaudio (Port * self, int expose)
{
  Track * track = port_get_track (self, 0);
  if (!track)
    return;

  Channel * ch = track->channel;
  if (!ch)
    return;

  char lbl[600];
  port_get_full_designation (self, lbl);
  if (expose)
    {
      if (self->id.flow == FLOW_INPUT)
        {
          if (self->id.flags & PORT_FLAG_STEREO_L)
            {
              if (ch->all_stereo_l_ins)
                {
                }
              else
                {
                  for (int i = 0; i < ch->num_ext_stereo_l_ins; i++)
                    {
                      int       idx = self->num_rtaudio_ins;
                      ExtPort * ext_port = ch->ext_stereo_l_ins[i];
                      ext_port_print (ext_port);
                      self->rtaudio_ins[idx] = rtaudio_device_new (
                        ext_port->rtaudio_is_input, NULL, ext_port->rtaudio_id,
                        ext_port->rtaudio_channel_idx, self);
                      rtaudio_device_open (self->rtaudio_ins[idx], true);
                      self->num_rtaudio_ins++;
                    }
                }
            }
          else if (self->id.flags & PORT_FLAG_STEREO_R)
            {
              if (ch->all_stereo_r_ins)
                {
                }
              else
                {
                  for (int i = 0; i < ch->num_ext_stereo_r_ins; i++)
                    {
                      int       idx = self->num_rtaudio_ins;
                      ExtPort * ext_port = ch->ext_stereo_r_ins[i];
                      ext_port_print (ext_port);
                      self->rtaudio_ins[idx] = rtaudio_device_new (
                        ext_port->rtaudio_is_input, NULL, ext_port->rtaudio_id,
                        ext_port->rtaudio_channel_idx, self);
                      rtaudio_device_open (self->rtaudio_ins[idx], true);
                      self->num_rtaudio_ins++;
                    }
                }
            }
        }
      g_message ("exposing %s", lbl);
    }
  else
    {
      if (self->id.flow == FLOW_INPUT)
        {
          for (int i = 0; i < self->num_rtaudio_ins; i++)
            {
              rtaudio_device_close (self->rtaudio_ins[i], true);
            }
          self->num_rtaudio_ins = 0;
        }
      g_message ("unexposing %s", lbl);
    }
  self->exposed_to_backend = expose;
}

/**
 * Dequeue the audio data from the ring buffer
 * into @ref RtAudioDevice.buf.
 */
void
port_prepare_rtaudio_data (Port * self)
{
  g_return_if_fail (
    /*self->id.flow == FLOW_INPUT &&*/
    audio_backend_is_rtaudio (AUDIO_ENGINE->audio_backend));

  for (int i = 0; i < self->num_rtaudio_ins; i++)
    {
      RtAudioDevice * dev = self->rtaudio_ins[i];

      /* clear the data */
      dsp_fill (dev->buf, DENORMAL_PREVENTION_VAL, AUDIO_ENGINE->nframes);

      zix_sem_wait (&dev->audio_ring_sem);

      uint32_t read_space = zix_ring_read_space (dev->audio_ring);

      /* only process if data exists */
      if (read_space >= AUDIO_ENGINE->nframes * sizeof (float))
        {
          /* read the buffer */
          zix_ring_read (
            dev->audio_ring, &dev->buf[0],
            AUDIO_ENGINE->nframes * sizeof (float));
        }

      zix_sem_post (&dev->audio_ring_sem);
    }
}

/**
 * Sums the inputs coming in from RtAudio
 * before the port is processed.
 */
void
port_sum_data_from_rtaudio (
  Port *          self,
  const nframes_t start_frame,
  const nframes_t nframes)
{
  g_return_if_fail (
    /*self->id.flow == FLOW_INPUT &&*/
    audio_backend_is_rtaudio (AUDIO_ENGINE->audio_backend));

  for (int i = 0; i < self->num_rtaudio_ins; i++)
    {
      RtAudioDevice * dev = self->rtaudio_ins[i];

      dsp_add2 (&self->buf[start_frame], &dev->buf[start_frame], nframes);
    }
}
#endif // HAVE_RTAUDIO

#ifdef _WOE32
/**
 * Sums the inputs coming in from Windows MME,
 * before the port is processed.
 */
static void
sum_data_from_windows_mme (
  Port *          self,
  const nframes_t start_frame,
  const nframes_t nframes)
{
  g_return_if_fail (
    self->id.flow == FLOW_INPUT
    && AUDIO_ENGINE->midi_backend == MIDI_BACKEND_WINDOWS_MME);

  if (self->id.owner_type == PORT_OWNER_TYPE_AUDIO_ENGINE)
    return;

  /* append events from Windows MME if any */
  for (int i = 0; i < self->num_mme_connections; i++)
    {
      WindowsMmeDevice * dev = self->mme_connections[i];
      if (!dev)
        {
          g_warn_if_reached ();
          continue;
        }

      MidiEvent ev;
      gint64    cur_time = g_get_monotonic_time ();
      while (windows_mme_device_dequeue_midi_event_struct (
        dev, self->last_midi_dequeue, cur_time, &ev))
        {
          int is_valid =
            ev.time >= start_frame && ev.time < start_frame + nframes;
          if (!is_valid)
            {
              g_warning ("Invalid event time %u", ev.time);
              continue;
            }

          if (ev.time >= start_frame && ev.time < start_frame + nframes)
            {
              midi_byte_t channel = ev.raw_buffer[0] & 0xf;
              Track *     track = port_get_track (self, 0);
              if (
                self->id.owner_type == PORT_OWNER_TYPE_TRACK_PROCESSOR
                && (track->type == TRACK_TYPE_MIDI || track->type == TRACK_TYPE_INSTRUMENT)
                && !track->channel->all_midi_channels
                && !track->channel->midi_channels[channel])
                {
                  /* different channel */
                }
              else
                {
                  midi_events_add_event_from_buf (
                    self->midi_events, ev.time, ev.raw_buffer, 3, 0);
                }
            }
        }
      self->last_midi_dequeue = cur_time;

#  if 0
      if (self->midi_events->num_events > 0)
        {
          MidiEvent * ev =
            &self->midi_events->events[0];
          char designation[600];
          port_get_full_designation (
            self, designation);
          g_message (
            "MME MIDI (%s): have %d events\n"
            "first event is: [%u] %hhx %hhx %hhx",
            designation,
            self->midi_events->num_events,
            ev->time, ev->raw_buffer[0],
            ev->raw_buffer[1], ev->raw_buffer[2]);
        }
#  endif
    }
}

/**
 * Sends the port data to Windows MME, after the
 * port is processed.
 */
static void
send_data_to_windows_mme (
  Port *          self,
  const nframes_t start_frame,
  const nframes_t nframes)
{
  g_return_if_fail (
    self->id.flow == FLOW_OUTPUT
    && AUDIO_ENGINE->midi_backend == MIDI_BACKEND_WINDOWS_MME);

  /* TODO send midi events */
}
#endif

/**
 * To be called when a control's value changes
 * so that a message can be sent to the UI.
 */
static void
port_forward_control_change_event (Port * self)
{
  /* if lv2 port/parameter */
  if (self->value_type > 0)
    {
      Plugin * pl = port_get_plugin (self, 1);
      g_return_if_fail (IS_PLUGIN_AND_NONNULL (pl) && pl->lv2);
      lv2_ui_send_control_val_event_from_plugin_to_ui (pl->lv2, self);
    }
  else if (self->id.owner_type == PORT_OWNER_TYPE_PLUGIN)
    {
      Plugin * pl = port_get_plugin (self, 1);
      if (pl)
        {
#ifdef HAVE_CARLA
          if (pl->setting->open_with_carla && self->carla_param_id >= 0)
            {
              g_return_if_fail (pl->carla);
              carla_native_plugin_set_param_value (
                pl->carla, (uint32_t) self->carla_param_id, self->control);
            }
#endif
          if (!g_atomic_int_get (&pl->state_changed_event_sent))
            {
              EVENTS_PUSH (ET_PLUGIN_STATE_CHANGED, pl);
              g_atomic_int_set (&pl->state_changed_event_sent, 1);
            }
        }
    }
  else if (self->id.owner_type == PORT_OWNER_TYPE_FADER)
    {
      Track * track = port_get_track (self, 1);
      g_return_if_fail (track && track->channel);

      if (
        self->id.flags & PORT_FLAG_FADER_MUTE
        || self->id.flags2 & PORT_FLAG2_FADER_SOLO
        || self->id.flags2 & PORT_FLAG2_FADER_LISTEN
        || self->id.flags2 & PORT_FLAG2_FADER_MONO_COMPAT)
        {
          EVENTS_PUSH (ET_TRACK_FADER_BUTTON_CHANGED, track);
        }
      else if (self->id.flags & PORT_FLAG_AMPLITUDE)
        {
          if (ZRYTHM_HAVE_UI)
            g_return_if_fail (track->channel->widget);
          fader_update_volume_and_fader_val (track->channel->fader);
          EVENTS_PUSH (ET_CHANNEL_FADER_VAL_CHANGED, track->channel);
        }
    }
  else if (self->id.owner_type == PORT_OWNER_TYPE_CHANNEL_SEND)
    {
      Track * track = port_get_track (self, 1);
      g_return_if_fail (IS_TRACK_AND_NONNULL (track));
      ChannelSend * send = track->channel->sends[self->id.port_index];
      EVENTS_PUSH (ET_CHANNEL_SEND_CHANGED, send);
    }
  else if (self->id.owner_type == PORT_OWNER_TYPE_TRACK)
    {
      Track * track = port_get_track (self, 1);
      EVENTS_PUSH (ET_TRACK_STATE_CHANGED, track);
    }
}

void
port_set_control_value (
  Port *      self,
  const float val,
  const bool  is_normalized,
  const bool  forward_event)
{
  PortIdentifier * id = &self->id;

  /* set the base value */
  if (is_normalized)
    {
      float minf = self->minf;
      float maxf = self->maxf;
      self->base_value = minf + val * (maxf - minf);
    }
  else
    {
      self->base_value = val;
    }

  self->unsnapped_control = self->base_value;
  self->base_value =
    control_port_get_snapped_val_from_val (self, self->unsnapped_control);

  if (!math_floats_equal (self->control, self->base_value))
    {
      self->control = self->base_value;

      /* remember time */
      self->last_change = g_get_monotonic_time ();
      self->value_changed_from_reading = false;

      /* if bpm, update engine */
      if (id->flags & PORT_FLAG_BPM)
        {
          /* this must only be called during processing kickoff or while the
           * engine is stopped */
          g_return_if_fail (
            !engine_get_run (AUDIO_ENGINE)
            || router_is_processing_kickoff_thread (ROUTER));

          int beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
          engine_update_frames_per_tick (
            AUDIO_ENGINE, beats_per_bar, self->control,
            AUDIO_ENGINE->sample_rate, false, true, true);
          EVENTS_PUSH (ET_BPM_CHANGED, NULL);
        }

      /* if time sig value, update transport caches */
      if (
        id->flags2 & PORT_FLAG2_BEATS_PER_BAR
        || id->flags2 & PORT_FLAG2_BEAT_UNIT)
        {
          /* this must only be called during processing kickoff or while the
           * engine is stopped */
          g_return_if_fail (
            !engine_get_run (AUDIO_ENGINE)
            || router_is_processing_kickoff_thread (ROUTER));

          int   beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
          int   beat_unit = tempo_track_get_beat_unit (P_TEMPO_TRACK);
          bpm_t bpm = tempo_track_get_current_bpm (P_TEMPO_TRACK);
          transport_update_caches (TRANSPORT, beats_per_bar, beat_unit);
          bool update_from_ticks = id->flags2 & PORT_FLAG2_BEATS_PER_BAR;
          engine_update_frames_per_tick (
            AUDIO_ENGINE, beats_per_bar, bpm, AUDIO_ENGINE->sample_rate, false,
            update_from_ticks, false);
          EVENTS_PUSH (ET_TIME_SIGNATURE_CHANGED, NULL);
        }

      /* if plugin enabled port, also set plugin's own enabled port value and
       * vice versa */
      if (
        port_is_in_active_project (self)
        && self->id.flags & PORT_FLAG_PLUGIN_ENABLED)
        {
          Plugin * pl = port_get_plugin (self, 1);
          g_return_if_fail (pl);

          if (self->id.flags & PORT_FLAG_GENERIC_PLUGIN_PORT)
            {
              if (
                pl->own_enabled_port
                && !math_floats_equal (
                  pl->own_enabled_port->control, self->control))
                {
                  g_debug (
                    "generic enabled changed - changing plugin's own enabled");
                  port_set_control_value (
                    pl->own_enabled_port, self->control, false,
                    F_PUBLISH_EVENTS);
                }
            }
          else if (!math_floats_equal (pl->enabled->control, self->control))
            {
              g_debug (
                "plugin's own enabled changed - changing generic enabled");
              port_set_control_value (
                pl->enabled, self->control, false, F_PUBLISH_EVENTS);
            }
        } /* endif plugin-enabled port */

      if (self->id.flags & PORT_FLAG_MIDI_AUTOMATABLE)
        {
          Track * track = port_get_track (self, true);
          g_return_if_fail (track);
          mpmc_queue_push_back (
            track->processor->updated_midi_automatable_ports, self);
        }

    } /* endif port value changed */

  if (forward_event)
    {
      port_forward_control_change_event (self);
    }
}

/**
 * Gets the given control value from the
 * corresponding underlying structure in the Port.
 *
 * @param normalized Whether to get the value
 *   normalized or not.
 */
float
port_get_control_value (Port * self, const bool normalize)
{
  g_return_val_if_fail (self->id.type == TYPE_CONTROL, 0.f);

  /* verify that plugin exists if plugin control */
  if (
    ZRYTHM_TESTING && port_is_in_active_project (self)
    && self->id.flags & PORT_FLAG_PLUGIN_CONTROL)
    {
      Plugin * pl = port_get_plugin (self, 1);
      g_return_val_if_fail (IS_PLUGIN_AND_NONNULL (pl), 0.f);
    }

  if (normalize)
    {
      return control_port_real_val_to_normalized (self, self->control);
    }
  else
    {
      return self->control;
    }
}

int
port_scale_point_cmp (const void * _a, const void * _b)
{
  const PortScalePoint * a = *(PortScalePoint const **) _a;
  const PortScalePoint * b = *(PortScalePoint const **) _b;
  return a->val - b->val > 0.f;
}

PortScalePoint *
port_scale_point_new (const float val, const char * label)
{
  PortScalePoint * self = object_new (PortScalePoint);

  self->val = val;
  self->label = g_strdup (label);

  return self;
}

void
port_scale_point_free (PortScalePoint * self)
{
  g_free_and_null (self->label);

  object_zero_and_free (self);
}

/**
 * Copies the metadata from a project port to
 * the given port.
 *
 * Used when doing delete actions so that ports
 * can be restored on undo.
 */
void
port_copy_metadata_from_project (Port * clone_port, Port * prj_port)
{
  clone_port->control = prj_port->control;
}

/**
 * Copies the port values from @ref other to @ref
 * self.
 *
 * @param self
 * @param other
 */
void
port_copy_values (Port * self, const Port * other)
{
  /* set value */
  self->control = other->control;
}

/**
 * Reverts the data on the corresponding project
 * port for the given non-project port.
 *
 * This restores src/dest connections and the
 * control value.
 *
 * @param self Project port.
 * @param non_project Non-project port.
 */
void
port_restore_from_non_project (Port * self, Port * non_project)
{
  Port * prj_port = self;

  /* set value */
  prj_port->control = non_project->control;

  g_return_if_fail (non_project->num_srcs <= (int) non_project->srcs_size);
  g_return_if_fail (non_project->num_dests <= (int) non_project->dests_size);
}

/**
 * Creates stereo ports for generic use.
 *
 * @param in 1 for in, 0 for out.
 * @param owner Pointer to the owner. The type is
 *   determined by owner_type.
 */
StereoPorts *
stereo_ports_new_generic (
  int           in,
  const char *  name,
  const char *  symbol,
  PortOwnerType owner_type,
  void *        owner)
{
  char * pll = g_strdup_printf ("%s L", name);
  char * plr = g_strdup_printf ("%s R", name);

  Port * l = port_new_with_type (TYPE_AUDIO, in ? FLOW_INPUT : FLOW_OUTPUT, pll);
  g_return_val_if_fail (l, NULL);
  Port * r = port_new_with_type (TYPE_AUDIO, in ? FLOW_INPUT : FLOW_OUTPUT, plr);
  g_return_val_if_fail (r, NULL);
  StereoPorts * ports = stereo_ports_new_from_existing (l, r);
  ports->l->id.flags |= PORT_FLAG_STEREO_L;
  ports->l->id.sym = g_strdup_printf ("%s_l", symbol);
  ports->r->id.flags |= PORT_FLAG_STEREO_R;
  ports->r->id.sym = g_strdup_printf ("%s_r", symbol);

  port_set_owner (ports->l, owner_type, owner);
  port_set_owner (ports->r, owner_type, owner);

  g_free (pll);
  g_free (plr);

  return ports;
}

void
port_process (Port * port, const EngineProcessTimeInfo time_nfo, const bool noroll)
{
  const PortIdentifier id = port->id;
  PortOwnerType        owner_type = id.owner_type;

  Track * track = NULL;
  if (
    owner_type == PORT_OWNER_TYPE_TRACK_PROCESSOR
    || owner_type == PORT_OWNER_TYPE_TRACK
    || owner_type == PORT_OWNER_TYPE_CHANNEL ||
    /* if track/channel fader */
    (owner_type == PORT_OWNER_TYPE_FADER
     && (id.flags2 & PORT_FLAG2_PREFADER || id.flags2 & PORT_FLAG2_POSTFADER))
    || (owner_type == PORT_OWNER_TYPE_PLUGIN && id.plugin_id.slot_type == PLUGIN_SLOT_INSTRUMENT))
    {
      if (ZRYTHM_TESTING)
        track = port_get_track (port, true);
      else
        track = port->track;
      g_return_if_fail (IS_TRACK_AND_NONNULL (track));
    }

  bool is_stereo_port =
    id.flags & PORT_FLAG_STEREO_L || id.flags & PORT_FLAG_STEREO_R;

  switch (id.type)
    {
    case TYPE_EVENT:
      if (noroll)
        break;

      if (G_UNLIKELY (owner_type == PORT_OWNER_TYPE_TRACK_PROCESSOR && !track))
        {
          g_return_if_reached ();
        }

      /* if piano roll keys, add the notes to the
       * piano roll "current notes" (to show
       * pressed keys in the UI) */
      if (
        G_UNLIKELY (
          owner_type == PORT_OWNER_TYPE_TRACK_PROCESSOR && track
          && port == track->processor->midi_out
          && port->midi_events->num_events > 0 && CLIP_EDITOR->has_region
          && CLIP_EDITOR->region_id.track_name_hash
               == track_get_name_hash (track)))
        {
          MidiEvents * events = port->midi_events;
          bool         events_processed = false;
          for (int i = 0; i < events->num_events; i++)
            {
              midi_byte_t * buf = events->events[i].raw_buffer;
              if (midi_is_note_on (buf))
                {
                  piano_roll_add_current_note (
                    PIANO_ROLL, midi_get_note_number (buf));
                  events_processed = true;
                }
              else if (midi_is_note_off (buf))
                {
                  piano_roll_remove_current_note (
                    PIANO_ROLL, midi_get_note_number (buf));
                  events_processed = true;
                }
              else if (midi_is_all_notes_off (buf))
                {
                  PIANO_ROLL->num_current_notes = 0;
                  events_processed = true;
                }
            }
          if (events_processed)
            {
              EVENTS_PUSH (ET_PIANO_ROLL_KEY_ON_OFF, NULL);
            }
        }

      /* only consider incoming external data if
       * armed for recording (if the port is owner
       * by a track), otherwise always consider
       * incoming external data */
      if ((owner_type !=
             PORT_OWNER_TYPE_TRACK_PROCESSOR ||
           (owner_type ==
              PORT_OWNER_TYPE_TRACK_PROCESSOR &&
            track &&
            track_type_can_record (track->type) &&
            track_get_recording (track))) &&
           id.flow == FLOW_INPUT)
        {
          switch (AUDIO_ENGINE->midi_backend)
            {
#ifdef HAVE_JACK
            case MIDI_BACKEND_JACK:
              sum_data_from_jack (port, time_nfo.local_offset, time_nfo.nframes);
              break;
#endif
#ifdef _WOE32
            case MIDI_BACKEND_WINDOWS_MME:
              sum_data_from_windows_mme (
                port, time_nfo.local_offset, time_nfo.nframes);
              break;
#endif
#ifdef HAVE_RTMIDI
            case MIDI_BACKEND_ALSA_RTMIDI:
            case MIDI_BACKEND_JACK_RTMIDI:
            case MIDI_BACKEND_WINDOWS_MME_RTMIDI:
            case MIDI_BACKEND_COREMIDI_RTMIDI:
              port_sum_data_from_rtmidi (
                port, time_nfo.local_offset, time_nfo.nframes);
              break;
#endif
            default:
              break;
            }
        }

      /* set midi capture if hardware */
      if (owner_type == PORT_OWNER_TYPE_HW)
        {
          MidiEvents * events = port->midi_events;
          if (events->num_events > 0)
            {
              AUDIO_ENGINE->trigger_midi_activity = 1;

              /* queue playback if recording and
               * we should record on MIDI input */
              if (
                TRANSPORT_IS_RECORDING && TRANSPORT_IS_PAUSED
                && TRANSPORT->start_playback_on_midi_input)
                {
                  EVENTS_PUSH (ET_TRANSPORT_ROLL_REQUIRED, NULL);
                }

              /* capture cc if capturing */
              if (AUDIO_ENGINE->capture_cc)
                {
                  memcpy (
                    AUDIO_ENGINE->last_cc,
                    events->events[events->num_events - 1].raw_buffer,
                    sizeof (midi_byte_t) * 3);
                }

              /* send cc to mapped ports */
              for (int i = 0; i < events->num_events; i++)
                {
                  MidiEvent * ev = &events->events[i];
                  midi_mappings_apply (MIDI_MAPPINGS, ev->raw_buffer);
                }
            }
        }

      /* handle MIDI clock */
      if (
        port->id.flags2 & PORT_FLAG2_MIDI_CLOCK && port->id.flow == FLOW_OUTPUT)
        {
          /* continue or start */
          bool start =
            TRANSPORT_IS_ROLLING && !AUDIO_ENGINE->pos_nfo_before.is_rolling;
          if (start)
            {
              uint8_t start_msg = MIDI_CLOCK_CONTINUE;
              if (PLAYHEAD->frames == 0)
                {
                  start_msg = MIDI_CLOCK_START;
                }
              midi_events_add_raw (port->midi_events, &start_msg, 1, 0, false);
            }
          else if (
            !TRANSPORT_IS_ROLLING && AUDIO_ENGINE->pos_nfo_before.is_rolling)
            {
              uint8_t stop_msg = MIDI_CLOCK_STOP;
              midi_events_add_raw (port->midi_events, &stop_msg, 1, 0, false);
            }

          /* song position */
          int32_t sixteenth_within_song =
            position_get_total_sixteenths (PLAYHEAD, false);
          if (
            AUDIO_ENGINE->pos_nfo_at_end.sixteenth_within_song
              != AUDIO_ENGINE->pos_nfo_current.sixteenth_within_song
            || start)
            {
              /* TODO interpolate */
              midi_events_add_song_pos (
                port->midi_events, sixteenth_within_song, 0, false);
            }

          /* clock beat */
          if (
            AUDIO_ENGINE->pos_nfo_at_end.ninetysixth_notes
            > AUDIO_ENGINE->pos_nfo_current.ninetysixth_notes)
            {
              for (
                int32_t i = AUDIO_ENGINE->pos_nfo_current.ninetysixth_notes + 1;
                i <= AUDIO_ENGINE->pos_nfo_at_end.ninetysixth_notes; i++)
                {
                  double ninetysixth_ticks = i * TICKS_PER_NINETYSIXTH_NOTE_DBL;
                  double ratio = (ninetysixth_ticks - AUDIO_ENGINE->pos_nfo_current.playhead_ticks) / (AUDIO_ENGINE->pos_nfo_at_end.playhead_ticks - AUDIO_ENGINE->pos_nfo_current.playhead_ticks);
                  midi_time_t midi_time = (midi_time_t) floor (
                    ratio * (double) AUDIO_ENGINE->block_length);
                  if (
                    midi_time >= time_nfo.local_offset
                    && midi_time < time_nfo.local_offset + time_nfo.nframes)
                    {
                      uint8_t beat_msg = MIDI_CLOCK_BEAT;
                      midi_events_add_raw (
                        port->midi_events, &beat_msg, 1, midi_time, false);
#if 0
                      g_debug (
                        "(i = %d) time %u / %u", i, midi_time,
                        time_nfo.local_offset + time_nfo.nframes);
#endif
                    }
                }
            }

          midi_events_sort (port->midi_events, false);
        }

      /* append data from each source */
      for (int k = 0; k < port->num_srcs; k++)
        {
          Port *                 src_port = port->srcs[k];
          const PortConnection * conn = port->src_connections[k];
          if (!conn->enabled)
            continue;

          g_return_if_fail (src_port->id.type == TYPE_EVENT);

          /* if hardware device connected to
           * track processor input, only allow
           * signal to pass if armed and
           * MIDI channel is valid */
          if (
            src_port->id.owner_type == PORT_OWNER_TYPE_HW
            && owner_type == PORT_OWNER_TYPE_TRACK_PROCESSOR)
            {
              g_return_if_fail (track);

              /* skip if not armed */
              if (!track_get_recording (track))
                continue;

              /* if not set to "all channels",
               * filter-append */
              if (
                (track->type == TRACK_TYPE_MIDI
                 || track->type == TRACK_TYPE_INSTRUMENT)
                && !track->channel->all_midi_channels)
                {
                  midi_events_append_w_filter (
                    port->midi_events, src_port->midi_events,
                    track->channel->midi_channels, time_nfo.local_offset,
                    time_nfo.nframes, F_NOT_QUEUED);
                  continue;
                }

              /* otherwise append normally */
            }

          midi_events_append (
            port->midi_events, src_port->midi_events, time_nfo.local_offset,
            time_nfo.nframes, F_NOT_QUEUED);
        } /* foreach source */

      if (id.flow == FLOW_OUTPUT)
        {
          switch (AUDIO_ENGINE->midi_backend)
            {
#ifdef HAVE_JACK
            case MIDI_BACKEND_JACK:
              send_data_to_jack (port, time_nfo.local_offset, time_nfo.nframes);
              break;
#endif
#ifdef _WOE32
            case MIDI_BACKEND_WINDOWS_MME:
              send_data_to_windows_mme (
                port, time_nfo.local_offset, time_nfo.nframes);
              break;
#endif
            default:
              break;
            }
        }

      /* send UI notification */
      if (port->midi_events->num_events > 0)
        {
#if 0
          g_message (
            "port %s has %d events",
            id.label,
            port->midi_events->num_events);
#endif

          if (owner_type == PORT_OWNER_TYPE_TRACK_PROCESSOR)
            {
              g_return_if_fail (IS_TRACK_AND_NONNULL (track));

              track->trigger_midi_activity = 1;
            }
        }

      if (time_nfo.local_offset + time_nfo.nframes == AUDIO_ENGINE->block_length)
        {
          MidiEvents * events = port->midi_events;
          if (port->write_ring_buffers)
            {
              for (int i = events->num_events - 1; i >= 0; i--)
                {
                  if (
                    zix_ring_write_space (port->midi_ring) < sizeof (MidiEvent))
                    {
                      zix_ring_skip (port->midi_ring, sizeof (MidiEvent));
                    }

                  MidiEvent * ev = &events->events[i];
                  ev->systime = g_get_monotonic_time ();
                  zix_ring_write (port->midi_ring, ev, sizeof (MidiEvent));
                }
            }
          else
            {
              if (events->num_events > 0)
                {
                  port->last_midi_event_time = g_get_monotonic_time ();
                  g_atomic_int_set (&port->has_midi_events, 1);
                }
            }
        }
      break;
    case TYPE_AUDIO:
    case TYPE_CV:
      if (noroll)
        {
          dsp_fill (
            &port->buf[time_nfo.local_offset], DENORMAL_PREVENTION_VAL,
            time_nfo.nframes);
          break;
        }

      g_return_if_fail (
        owner_type != PORT_OWNER_TYPE_TRACK_PROCESSOR
        || IS_TRACK_AND_NONNULL (track));

      /* only consider incoming external data if
       * armed for recording (if the port is owner
       * by a track), otherwise always consider
       * incoming external data */
      if ((owner_type !=
             PORT_OWNER_TYPE_TRACK_PROCESSOR
           || (owner_type ==
                 PORT_OWNER_TYPE_TRACK_PROCESSOR
               && track_type_can_record (track->type)
               && track_get_recording (track)))
          && id.flow == FLOW_INPUT)
        {
          switch (AUDIO_ENGINE->audio_backend)
            {
#ifdef HAVE_JACK
            case AUDIO_BACKEND_JACK:
              sum_data_from_jack (port, time_nfo.local_offset, time_nfo.nframes);
              break;
#endif
            case AUDIO_BACKEND_DUMMY:
              sum_data_from_dummy (
                port, time_nfo.local_offset, time_nfo.nframes);
              break;
            default:
              break;
            }
        }

      for (int k = 0; k < port->num_srcs; k++)
        {
          Port *                 src_port = port->srcs[k];
          const PortConnection * conn = port->src_connections[k];
          if (!conn->enabled)
            continue;

          float minf = 0.f, maxf = 0.f, depth_range, multiplier;
          if (G_LIKELY (id.type == TYPE_AUDIO))
            {
              minf = -2.f;
              maxf = 2.f;
              depth_range = 1.f;
              multiplier = conn->multiplier;
            }
          else if (id.type == TYPE_CV)
            {
              maxf = port->maxf;
              minf = port->minf;

              /* (maxf - minf) / 2 */
              depth_range = (maxf - minf) * 0.5f;

              multiplier = depth_range * conn->multiplier;
            }
          else
            g_return_if_reached ();

          /* sum the signals */
          if (G_LIKELY (math_floats_equal_epsilon (multiplier, 1.f, 0.00001f)))
            {
              dsp_add2 (
                &port->buf[time_nfo.local_offset],
                &src_port->buf[time_nfo.local_offset], time_nfo.nframes);
            }
          else
            {
              dsp_mix2 (
                &port->buf[time_nfo.local_offset],
                &src_port->buf[time_nfo.local_offset], 1.f, multiplier,
                time_nfo.nframes);
            }

          if (
            G_UNLIKELY (id.type == TYPE_CV)
            || owner_type == PORT_OWNER_TYPE_FADER)
            {
              float abs_peak = dsp_abs_max (
                &port->buf[time_nfo.local_offset], time_nfo.nframes);
              if (abs_peak > maxf)
                {
                  /* this limiting wastes around 50% of port processing so only
                   * do it on CV connections and faders if they exceed maxf */
                  dsp_limit1 (
                    &port->buf[time_nfo.local_offset], minf, maxf,
                    time_nfo.nframes);
                }
            }
        } /* foreach source */

      if (id.flow == FLOW_OUTPUT)
        {
          switch (AUDIO_ENGINE->audio_backend)
            {
#ifdef HAVE_JACK
            case AUDIO_BACKEND_JACK:
              send_data_to_jack (port, time_nfo.local_offset, time_nfo.nframes);
              break;
#endif
            default:
              break;
            }
        }

      if (time_nfo.local_offset + time_nfo.nframes == AUDIO_ENGINE->block_length)
        {
          size_t size = sizeof (float) * (size_t) AUDIO_ENGINE->block_length;
          size_t write_space_avail = zix_ring_write_space (port->audio_ring);

          /* move the read head 8 blocks to make space if no space avail to
           * write */
          if (write_space_avail / size < 1)
            {
              zix_ring_skip (port->audio_ring, size * 8);
            }

          zix_ring_write (port->audio_ring, &port->buf[0], size);
        }

      /* if track output (to be shown on mixer) */
      if (
        owner_type == PORT_OWNER_TYPE_CHANNEL && is_stereo_port
        && id.flow == FLOW_OUTPUT)
        {
          g_return_if_fail (IS_TRACK_AND_NONNULL (track));
          Channel * ch = track->channel;
          g_return_if_fail (ch);

          /* calculate meter values */
          if (port == ch->stereo_out->l || port == ch->stereo_out->r)
            {
              /* reset peak if needed */
              gint64 time_now = g_get_monotonic_time ();
              if (time_now - port->peak_timestamp > TIME_TO_RESET_PEAK)
                port->peak = -1.f;

              bool changed = dsp_abs_max_with_existing_peak (
                &port->buf[time_nfo.local_offset], &port->peak,
                time_nfo.nframes);
              if (changed)
                {
                  port->peak_timestamp = g_get_monotonic_time ();
                }
            }
        }

      /* if bouncing tracks directly to master (e.g., when bouncing the track on
       * its own without parents), clear master input */
      if (G_UNLIKELY (
              AUDIO_ENGINE->bounce_mode > BOUNCE_OFF
              && !AUDIO_ENGINE->bounce_with_parents
              &&
              (port ==
                 P_MASTER_TRACK->processor->
                   stereo_in->l
               ||
               port ==
                 P_MASTER_TRACK->processor->
                   stereo_in->r)))
        {
          dsp_fill (
            &port->buf[time_nfo.local_offset],
            AUDIO_ENGINE->denormal_prevention_val, time_nfo.nframes);
        }

      /* if bouncing track directly to master (e.g., when bouncing the track on
       * its own without parents), add the buffer to master output */
      if (G_UNLIKELY (
              AUDIO_ENGINE->bounce_mode >
                BOUNCE_OFF &&
              (owner_type ==
                 PORT_OWNER_TYPE_CHANNEL ||
               owner_type ==
                 PORT_OWNER_TYPE_TRACK_PROCESSOR ||
               (owner_type ==
                 PORT_OWNER_TYPE_FADER
                &&
                id.flags2 &
                  PORT_FLAG2_PREFADER)
               ||
               (owner_type ==
                 PORT_OWNER_TYPE_PLUGIN &&
                id.plugin_id.slot_type ==
                  PLUGIN_SLOT_INSTRUMENT)) &&
              is_stereo_port &&
              id.flow == FLOW_OUTPUT &&
              track && track->bounce_to_master))
        {

#define _ADD(l_or_r) \
  dsp_add2 ( \
    &P_MASTER_TRACK->channel->stereo_out->l_or_r->buf[time_nfo.local_offset], \
    &port->buf[time_nfo.local_offset], time_nfo.nframes)

          Channel *        ch;
          Fader *          prefader;
          TrackProcessor * tp;
          switch (AUDIO_ENGINE->bounce_step)
            {
            case BOUNCE_STEP_BEFORE_INSERTS:
              tp = track->processor;
              g_return_if_fail (tp);
              if (track->type == TRACK_TYPE_INSTRUMENT)
                {
                  if (port == track->channel->instrument->l_out)
                    {
                      _ADD (l);
                    }
                  if (port == track->channel->instrument->r_out)
                    {
                      _ADD (r);
                    }
                }
              else if (tp->stereo_out && track->bounce)
                {
                  if (port == tp->stereo_out->l)
                    {
                      _ADD (l);
                    }
                  else if (port == tp->stereo_out->r)
                    {
                      _ADD (r);
                    }
                }
              break;
            case BOUNCE_STEP_PRE_FADER:
              ch = track->channel;
              g_return_if_fail (ch);
              prefader = ch->prefader;
              if (port == prefader->stereo_out->l)
                {
                  _ADD (l);
                }
              else if (port == prefader->stereo_out->r)
                {
                  _ADD (r);
                }
              break;
            case BOUNCE_STEP_POST_FADER:
              ch = track->channel;
              g_return_if_fail (ch);
              if (track->type != TRACK_TYPE_MASTER)
                {
                  if (port == ch->stereo_out->l)
                    {
                      _ADD (l);
                    }
                  else if (port == ch->stereo_out->r)
                    {
                      _ADD (r);
                    }
                }
              break;
            }
#undef _ADD
        }
      break;
    case TYPE_CONTROL:
      {
        if (
          id.flow != FLOW_INPUT
          || (owner_type == PORT_OWNER_TYPE_FADER && (id.flags2 & PORT_FLAG2_MONITOR_FADER || id.flags2 & PORT_FLAG2_PREFADER))
          || id.flags & PORT_FLAG_TP_MONO || id.flags & PORT_FLAG_TP_INPUT_GAIN
          || !(id.flags & PORT_FLAG_AUTOMATABLE))
          {
            break;
          }

        /* calculate value from automation track */
        g_return_if_fail (id.flags & PORT_FLAG_AUTOMATABLE);
        AutomationTrack * at = port->at;
        if (G_UNLIKELY (!at))
          {
            g_critical (
              "No automation track found for port "
              "%s",
              id.label);
          }
        if (ZRYTHM_TESTING && at)
          {
            AutomationTrack * found_at =
              automation_track_find_from_port (port, NULL, true);
            g_return_if_fail (at == found_at);
          }

        if (
          at && id.flags & PORT_FLAG_AUTOMATABLE
          && automation_track_should_read_automation (
            at, AUDIO_ENGINE->timestamp_start))
          {
            Position pos;
            position_from_frames (
              &pos, (signed_frame_t) time_nfo.g_start_frame_w_offset);

            /* if playhead pos changed manually recently or transport is
             * rolling, we will force the last known automation point value
             * regardless of whether there is a region at current pos */
            bool can_read_previous_automation =
              TRANSPORT_IS_ROLLING
              || (TRANSPORT->last_manual_playhead_change - AUDIO_ENGINE->last_timestamp_start > 0);

            /* if there was an automation event at the playhead position, set
             * val and flag */
            AutomationPoint * ap = automation_track_get_ap_before_pos (
              at, &pos, !can_read_previous_automation, Z_F_USE_SNAPSHOTS);
            if (ap)
              {
                float val = automation_track_get_val_at_pos (
                  at, &pos, true, !can_read_previous_automation,
                  Z_F_USE_SNAPSHOTS);
                control_port_set_val_from_normalized (port, val, true);
                port->value_changed_from_reading = true;
              }
          }

        float maxf, minf, depth_range, val_to_use;
        /* whether this is the first CV processed on this control port */
        bool first_cv = true;
        for (int k = 0; k < port->num_srcs; k++)
          {
            const PortConnection * conn = port->src_connections[k];
            if (G_UNLIKELY (!conn->enabled))
              continue;

            Port * src_port = port->srcs[k];
            if (src_port->id.type == TYPE_CV)
              {
                maxf = port->maxf;
                minf = port->minf;

                /*float deff =*/
                /*port->lv2_port->lv2_control->deff;*/
                /*port->lv2_port->control =*/
                /*deff + (maxf - deff) **/
                /*src_port->buf[0];*/
                depth_range = (maxf - minf) / 2.f;

                /* figure out whether to use base
                 * value or the current value */
                if (first_cv)
                  {
                    val_to_use = port->base_value;
                    first_cv = false;
                  }
                else
                  {
                    val_to_use = port->control;
                  }

                float result = CLAMP (
                  val_to_use + depth_range * src_port->buf[0] * conn->multiplier,
                  minf, maxf);
                port->control = result;
                port_forward_control_change_event (port);
              }
          }
      }
      break;
    default:
      break;
    }
}

/**
 * Disconnects all hardware inputs from the port.
 */
void
port_disconnect_hw_inputs (Port * self)
{
  GPtrArray * srcs = g_ptr_array_new ();
  int         num_srcs = port_connections_manager_get_sources_or_dests (
    PORT_CONNECTIONS_MGR, srcs, &self->id, true);
  for (int i = 0; i < num_srcs; i++)
    {
      PortConnection * conn = (PortConnection *) g_ptr_array_index (srcs, i);
      if (conn->src_id->owner_type == PORT_OWNER_TYPE_HW)
        port_connections_manager_ensure_disconnect (
          PORT_CONNECTIONS_MGR, conn->src_id, &self->id);
    }
  g_ptr_array_unref (srcs);
}

/**
 * Sets whether to expose the port to the backend
 * and exposes it or removes it.
 *
 * It checks what the backend is using the engine's
 * audio backend or midi backend settings.
 */
void
port_set_expose_to_backend (Port * self, int expose)
{
  g_return_if_fail (AUDIO_ENGINE);

  if (!AUDIO_ENGINE->setup)
    {
      g_message (
        "audio engine not set up, skipping expose "
        "to backend for %s",
        self->id.sym);
      return;
    }

  if (self->id.type == TYPE_AUDIO)
    {
      switch (AUDIO_ENGINE->audio_backend)
        {
        case AUDIO_BACKEND_DUMMY:
          g_message ("called %s with dummy audio backend", __func__);
          return;
#ifdef HAVE_JACK
        case AUDIO_BACKEND_JACK:
          expose_to_jack (self, expose);
          break;
#endif
#ifdef HAVE_RTAUDIO
        case AUDIO_BACKEND_ALSA_RTAUDIO:
        case AUDIO_BACKEND_JACK_RTAUDIO:
        case AUDIO_BACKEND_PULSEAUDIO_RTAUDIO:
        case AUDIO_BACKEND_COREAUDIO_RTAUDIO:
        case AUDIO_BACKEND_WASAPI_RTAUDIO:
        case AUDIO_BACKEND_ASIO_RTAUDIO:
          expose_to_rtaudio (self, expose);
          break;
#endif
        default:
          break;
        }
    }
  else if (self->id.type == TYPE_EVENT)
    {
      switch (AUDIO_ENGINE->midi_backend)
        {
        case MIDI_BACKEND_DUMMY:
          g_message ("called %s with MIDI dummy backend", __func__);
          return;
#ifdef HAVE_JACK
        case MIDI_BACKEND_JACK:
          expose_to_jack (self, expose);
          break;
#endif
#ifdef HAVE_ALSA
        case MIDI_BACKEND_ALSA:
#  if 0
          expose_to_alsa (self, expose);
#  endif
          break;
#endif
#ifdef HAVE_RTMIDI
        case MIDI_BACKEND_ALSA_RTMIDI:
        case MIDI_BACKEND_JACK_RTMIDI:
        case MIDI_BACKEND_WINDOWS_MME_RTMIDI:
        case MIDI_BACKEND_COREMIDI_RTMIDI:
          expose_to_rtmidi (self, expose);
          break;
#endif
        default:
          break;
        }
    }
  else /* else if not audio or MIDI */
    {
      g_return_if_reached ();
    }
}

/**
 * Renames the port on the backend side.
 */
void
port_rename_backend (Port * self)
{
  if ((!port_is_exposed_to_backend (self)))
    return;

  switch (self->internal_type)
    {
#ifdef HAVE_JACK
    case INTERNAL_JACK_PORT:
      {
        char str[600];
        port_get_full_designation (self, str);
        jack_port_rename (AUDIO_ENGINE->client, (jack_port_t *) self->data, str);
      }
      break;
#endif
    case INTERNAL_ALSA_SEQ_PORT:
      break;
    default:
      break;
    }
}

/**
 * If MIDI port, returns if there are any events,
 * if audio port, returns if there is sound in the
 * buffer.
 */
bool
port_has_sound (Port * self)
{
  switch (self->id.type)
    {
    case TYPE_AUDIO:
      g_return_val_if_fail (self->buf, false);
      for (nframes_t i = 0; i < AUDIO_ENGINE->block_length; i++)
        {
          if (fabsf (self->buf[i]) > 0.0000001f)
            {
              return true;
            }
        }
      break;
    case TYPE_EVENT:
      /* TODO */
      break;
    default:
      break;
    }

  return false;
}

/**
 * Copies a full designation of \p self in the
 * format "Track/Port" or "Track/Plugin/Port" in
 * \p buf.
 */
void
port_get_full_designation (Port * const self, char * buf)
{
  const PortIdentifier * id = &self->id;

  switch (id->owner_type)
    {
    case PORT_OWNER_TYPE_AUDIO_ENGINE:
      strcpy (buf, id->label);
      return;
    case PORT_OWNER_TYPE_PLUGIN:
      {
        Plugin * pl = port_get_plugin (self, 1);
        g_return_if_fail (pl);
        Track * track = plugin_get_track (pl);
        g_return_if_fail (track);
        sprintf (
          buf, "%s/%s/%s", track->name, pl->setting->descr->name, id->label);
      }
      return;
    case PORT_OWNER_TYPE_CHANNEL:
    case PORT_OWNER_TYPE_TRACK:
    case PORT_OWNER_TYPE_TRACK_PROCESSOR:
    case PORT_OWNER_TYPE_CHANNEL_SEND:
      {
        Track * tr = port_get_track (self, 1);
        g_return_if_fail (IS_TRACK_AND_NONNULL (tr));
        sprintf (buf, "%s/%s", tr->name, id->label);
      }
      return;
    case PORT_OWNER_TYPE_FADER:
      if (id->flags2 & PORT_FLAG2_PREFADER || id->flags2 & PORT_FLAG2_POSTFADER)
        {
          Track * tr = port_get_track (self, 1);
          g_return_if_fail (IS_TRACK_AND_NONNULL (tr));
          sprintf (buf, "%s/%s", tr->name, id->label);
          return;
        }
      else if (id->flags2 & PORT_FLAG2_MONITOR_FADER)

        {
          sprintf (buf, "Engine/%s", id->label);
          return;
        }
      else if (id->flags2 & PORT_FLAG2_SAMPLE_PROCESSOR_FADER)
        {
          strcpy (buf, id->label);
          return;
        }
      break;
    case PORT_OWNER_TYPE_HW:
      sprintf (buf, "HW/%s", id->label);
      return;
    case PORT_OWNER_TYPE_TRANSPORT:
      sprintf (buf, "Transport/%s", id->label);
      return;
    case PORT_OWNER_TYPE_MODULATOR_MACRO_PROCESSOR:
      sprintf (buf, "Modulator Macro Processor/%s", id->label);
      return;
    default:
      break;
    }

  g_return_if_reached ();
}

void
port_print_full_designation (Port * const self)
{
  char buf[1200];
  port_get_full_designation (self, buf);
  g_message ("%s", buf);
}

/**
 * Clears the backend's port buffer.
 */
void
port_clear_external_buffer (Port * port)
{
  if (!port_is_exposed_to_backend (port))
    {
      return;
    }

#ifdef HAVE_JACK
  if (port->internal_type != INTERNAL_JACK_PORT)
    {
      return;
    }

  jack_port_t * jport = JACK_PORT_T (port->data);
  g_return_if_fail (jport);
  void * buf = jack_port_get_buffer (jport, AUDIO_ENGINE->block_length);
  g_return_if_fail (buf);
  if (
    port->id.type == TYPE_AUDIO
    && AUDIO_ENGINE->audio_backend == AUDIO_BACKEND_JACK)
    {
      float * fbuf = (float *) buf;
      dsp_fill (&fbuf[0], DENORMAL_PREVENTION_VAL, AUDIO_ENGINE->block_length);
    }
  else if (
    port->id.type == TYPE_EVENT
    && AUDIO_ENGINE->midi_backend == MIDI_BACKEND_JACK)
    {
      jack_midi_clear_buffer (buf);
    }
#endif
}

Track *
port_get_track (const Port * const self, int warn_if_fail)
{
  g_return_val_if_fail (IS_PORT (self), NULL);

  /* return the pointer if dsp thread */
  if (
    G_LIKELY (
      self->track && PROJECT && PROJECT->loaded && ROUTER
      && router_is_processing_thread (ROUTER)))
    {
      g_return_val_if_fail (IS_TRACK (self->track), NULL);
      return self->track;
    }

  Track * track = NULL;
  if (self->id.track_name_hash != 0)
    {
      g_return_val_if_fail (ZRYTHM && TRACKLIST, NULL);

      track =
        tracklist_find_track_by_name_hash (TRACKLIST, self->id.track_name_hash);
      if (!track)
        track = tracklist_find_track_by_name_hash (
          SAMPLE_PROCESSOR->tracklist, self->id.track_name_hash);
    }

  if (!track && warn_if_fail)
    {
      g_warning (
        "track with name hash %d not found for "
        "port %s",
        self->id.track_name_hash, self->id.label);
    }

  return track;
}

Plugin *
port_get_plugin (Port * const self, const bool warn_if_fail)
{
  g_return_val_if_fail (IS_PORT (self), NULL);

  /* if DSP thread, return the pointer */
  if (
    G_LIKELY (
      self->plugin && PROJECT && PROJECT->loaded && ROUTER
      && router_is_processing_thread (ROUTER)))
    {
      g_return_val_if_fail (IS_PLUGIN (self->plugin), NULL);
      return self->plugin;
    }

  if (self->id.owner_type != PORT_OWNER_TYPE_PLUGIN)
    {
      if (warn_if_fail)
        g_warning ("port not owned by plugin");
      return NULL;
    }

  Track * track = port_get_track (self, 0);
  if (!track && self->tmp_plugin)
    {
      return self->tmp_plugin;
    }
  if (!track || (track->type != TRACK_TYPE_MODULATOR && !track->channel))
    {
      if (warn_if_fail)
        {
          g_warning ("No track found for port");
        }
      return NULL;
    }

  Plugin *                       pl = NULL;
  const PluginIdentifier * const pl_id = &self->id.plugin_id;
  switch (pl_id->slot_type)
    {
    case PLUGIN_SLOT_MIDI_FX:
      pl = track->channel->midi_fx[pl_id->slot];
      break;
    case PLUGIN_SLOT_INSTRUMENT:
      pl = track->channel->instrument;
      break;
    case PLUGIN_SLOT_INSERT:
      pl = track->channel->inserts[pl_id->slot];
      break;
    case PLUGIN_SLOT_MODULATOR:
      pl = track->modulators[pl_id->slot];
      break;
    default:
      g_return_val_if_reached (NULL);
      break;
    }
  if (!pl && self->tmp_plugin)
    return self->tmp_plugin;

  if (!pl && warn_if_fail)
    {
      g_critical (
        "plugin at slot type %s (slot %d) not "
        "found for port %s",
        plugin_slot_type_to_string (pl_id->slot_type), pl_id->slot,
        self->id.label);
      return NULL;
    }

  /* unset \ref Port.tmp_plugin if a Plugin was found */
  /* FIXME self should be const - this should not be done here */
  self->tmp_plugin = NULL;

  return pl;
}

/**
 * Applies the pan to the given L/R ports.
 */
void
port_apply_pan_stereo (
  Port *       l,
  Port *       r,
  float        pan,
  PanLaw       pan_law,
  PanAlgorithm pan_algo)
{
  /* FIXME not used */
  g_warn_if_reached ();
  /*int nframes = AUDIO_ENGINE->block_length;*/
  /*int i;*/
  /*switch (pan_algo)*/
  /*{*/
  /*case PAN_ALGORITHM_SINE_LAW:*/
  /*for (i = 0; i < nframes; i++)*/
  /*{*/
  /*if (r->buf[i] != 0.f)*/
  /*r->buf[i] *= sinf (pan * (M_PIF / 2.f));*/
  /*if (l->buf[i] != 0.f)*/
  /*l->buf[i] *= sinf ((1.f - pan) * (M_PIF / 2.f));*/
  /*}*/
  /*break;*/
  /*case PAN_ALGORITHM_SQUARE_ROOT:*/
  /*break;*/
  /*case PAN_ALGORITHM_LINEAR:*/
  /*for (i = 0; i < nframes; i++)*/
  /*{*/
  /*if (r->buf[i] != 0.f)*/
  /*r->buf[i] *= pan;*/
  /*if (l->buf[i] != 0.f)*/
  /*l->buf[i] *= (1.f - pan);*/
  /*}*/
  /*break;*/
  /*}*/
}

/**
 * Applies the pan to the given port.
 *
 * @param start_frame The start frame offset from
 *   0 in this cycle.
 * @param nframes The number of frames to process.
 */
void
port_apply_pan (
  Port *          port,
  float           pan,
  PanLaw          pan_law,
  PanAlgorithm    pan_algo,
  nframes_t       start_frame,
  const nframes_t nframes)
{
  float calc_r, calc_l;
  pan_get_calc_lr (pan_law, pan_algo, pan, &calc_l, &calc_r);

  /* if stereo R */
  if (port->id.flags & PORT_FLAG_STEREO_R)
    {
      dsp_mul_k2 (&port->buf[start_frame], calc_r, nframes);
    }
  /* else if stereo L */
  else
    {
      dsp_mul_k2 (&port->buf[start_frame], calc_l, nframes);
    }
}

/**
 * Generates a hash for a given port.
 */
uint32_t
port_get_hash (const void * ptr)
{
  Port * self = (Port *) ptr;
  return hash_get_for_struct (self, sizeof (Port));
}

/**
 * To be used during serialization.
 */
Port *
port_clone (const Port * src)
{
  g_return_val_if_fail (IS_PORT (src), NULL);

  Port * self = object_new (Port);
  /*self->schema_version = PORT_SCHEMA_VERSION;*/
  self->magic = PORT_MAGIC;

  port_identifier_copy (&self->id, &src->id);

#define COPY_MEMBER(x) self->x = src->x

  COPY_MEMBER (exposed_to_backend);
  COPY_MEMBER (control);
  COPY_MEMBER (minf);
  COPY_MEMBER (maxf);
  COPY_MEMBER (zerof);
  COPY_MEMBER (deff);
  COPY_MEMBER (carla_param_id);

#undef COPY_MEMBER

  return self;
}

/**
 * Deletes port, doing required cleanup and
 * updating counters.
 */
void
port_free (Port * self)
{
  port_free_bufs (self);

#ifdef HAVE_RTMIDI
  for (int i = 0; i < self->num_rtmidi_ins; i++)
    {
      rtmidi_device_close (self->rtmidi_ins[i], 1);
    }
#endif

  object_zero_and_free (self->srcs);
  object_zero_and_free (self->dests);
  object_zero_and_free (self->src_connections);
  object_zero_and_free (self->dest_connections);

  for (int i = 0; i < self->num_scale_points; i++)
    {
      object_free_w_func_and_null (port_scale_point_free, self->scale_points[i]);
    }
  object_zero_and_free (self->scale_points);

  object_free_w_func_and_null (lv2_evbuf_free, self->evbuf);

  port_identifier_free_members (&self->id);

  object_zero_and_free (self);
}
