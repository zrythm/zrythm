// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <inttypes.h>

#include "dsp/channel.h"
#include "dsp/control_port.h"
#include "dsp/engine_jack.h"
#include "dsp/graph.h"
#include "dsp/hardware_processor.h"
#include "dsp/master_track.h"
#include "dsp/midi_event.h"
#include "dsp/pan.h"
#include "dsp/port.h"
#include "dsp/port_identifier.h"
#include "dsp/router.h"
#include "dsp/rtaudio_device.h"
#include "dsp/rtmidi_device.h"
#include "dsp/tempo_track.h"
#include "dsp/windows_mme_device.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/channel.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/dsp.h"
#include "utils/flags.h"
#include "utils/hash.h"
#include "utils/math.h"
#include "utils/mpmc_queue.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include "gtk_wrapper.h"
#include "zix/ring.h"
#include <fmt/format.h>

#define SLEEPTIME_USEC 60

#define AUDIO_RING_SIZE 65536

void
Port::allocate_bufs ()
{
#if 0
  size_t sz = 600;
  char str[sz];
  port_identifier_print_to_str (
    &this->id_, str, sz);
  g_message ("allocating bufs for %s", str);
#endif

  switch (this->id_.type_)
    {
    case PortType::Event:
      object_free_w_func_and_null (midi_events_free, this->midi_events_);
      this->midi_events_ = midi_events_new ();
      object_free_w_func_and_null (zix_ring_free, this->midi_ring_);
      this->midi_ring_ = zix_ring_new (
        zix_default_allocator (), sizeof (MidiEvent) * (size_t) 11);
      break;
    case PortType::Audio:
    case PortType::CV:
      {
        object_free_w_func_and_null (zix_ring_free, this->audio_ring_);
        this->audio_ring_ = zix_ring_new (
          zix_default_allocator (), sizeof (float) * AUDIO_RING_SIZE);
        size_t max = AUDIO_ENGINE->block_length;
        max = MAX (max, 1);
        this->buf_.resize (max);
        this->last_buf_sz_ = max;
      }
    default:
      break;
    }
}

void
Port::free_bufs ()
{
  object_free_w_func_and_null (midi_events_free, this->midi_events_);
  object_free_w_func_and_null (zix_ring_free, this->midi_ring_);
  object_free_w_func_and_null (zix_ring_free, this->audio_ring_);
}

void
Port::clear_buffer (AudioEngine &_engine)
{
  if (this->id_.type_ == PortType::Audio || this->id_.type_ == PortType::CV)
    {
      dsp_fill (
        this->buf_.data (), DENORMAL_PREVENTION_VAL (&_engine),
        _engine.block_length);
    }
  else if (this->id_.type_ == PortType::Event)
    {
      if (this->midi_events_)
        this->midi_events_->num_events = 0;
    }
}

/**
 * This function finds the Ports corresponding to
 * the PortIdentifiers for srcs and dests.
 *
 * Should be called after the ports are loaded from
 * yml.
 */
void
Port::init_loaded (void * owner)
{
  this->magic_ = PORT_MAGIC;

  this->unsnapped_control_ = this->control_;

  this->set_owner (this->id_.owner_type_, owner);

#if 0
  if (this->track
      && ENUM_BITSET_TEST(PortIdentifier::Flags,this->id_.flags_,PortIdentifier::Flags::AUTOMATABLE))
    {
      if (!this->at)
        {
          this->at =
            automation_track_find_from_port (
              this, NULL, false);
        }
      g_return_if_fail (this->at);
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
Port::find_from_identifier (const PortIdentifier * const id)
{
  Track *                tr = NULL;
  Channel *              ch = NULL;
  Plugin *               pl = NULL;
  Fader *                fader = NULL;
  PortIdentifier::Flags  flags = id->flags_;
  PortIdentifier::Flags2 flags2 = id->flags2_;
  switch (id->owner_type_)
    {
    case PortIdentifier::OwnerType::PORT_OWNER_TYPE_AUDIO_ENGINE:
      switch (id->type_)
        {
        case PortType::Event:
          if (id->flow_ == PortFlow::Output)
            { /* TODO */
            }
          else if (id->flow_ == PortFlow::Input)
            {
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags,
                  PortIdentifier::Flags::MANUAL_PRESS))
                return AUDIO_ENGINE->midi_editor_manual_press;
            }
          break;
        case PortType::Audio:
          if (id->flow_ == PortFlow::Output)
            {
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::STEREO_L))
                return &AUDIO_ENGINE->monitor_out->get_l ();
              else if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::STEREO_R))
                return &AUDIO_ENGINE->monitor_out->get_r ();
            }
          else if (id->flow_ == PortFlow::Input)
            {
              /* none */
            }
          break;
        default:
          break;
        }
      break;
    case PortIdentifier::OwnerType::PLUGIN:
      tr = tracklist_find_track_by_name_hash (TRACKLIST, id->track_name_hash_);
      if (!tr)
        tr = tracklist_find_track_by_name_hash (
          SAMPLE_PROCESSOR->tracklist, id->track_name_hash_);
      g_return_val_if_fail (IS_TRACK_AND_NONNULL (tr), NULL);
      switch (id->plugin_id_.slot_type)
        {
        case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
          g_return_val_if_fail (tr->channel, NULL);
          pl = tr->channel->midi_fx[id->plugin_id_.slot];
          break;
        case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
          g_return_val_if_fail (tr->channel, NULL);
          pl = tr->channel->instrument;
          break;
        case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
          g_return_val_if_fail (tr->channel, NULL);
          pl = tr->channel->inserts[id->plugin_id_.slot];
          break;
        case ZPluginSlotType::Z_PLUGIN_SLOT_MODULATOR:
          g_return_val_if_fail (tr->modulators, NULL);
          pl = tr->modulators[id->plugin_id_.slot];
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

      switch (id->flow_)
        {
        case PortFlow::Input:
          return pl->in_ports[id->port_index_];
          break;
        case PortFlow::Output:
          return pl->out_ports[id->port_index_];
          break;
        default:
          g_return_val_if_reached (NULL);
          break;
        }
      break;
    case PortIdentifier::OwnerType::TRACK_PROCESSOR:
      tr = tracklist_find_track_by_name_hash (TRACKLIST, id->track_name_hash_);
      if (!tr)
        tr = tracklist_find_track_by_name_hash (
          SAMPLE_PROCESSOR->tracklist, id->track_name_hash_);
      g_return_val_if_fail (IS_TRACK_AND_NONNULL (tr), NULL);
      switch (id->type_)
        {
        case PortType::Event:
          if (id->flow_ == PortFlow::Output)
            {
              return tr->processor->midi_out;
            }
          else if (id->flow_ == PortFlow::Input)
            {
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::PianoRoll))
                return tr->processor->piano_roll;
              else
                return tr->processor->midi_in;
            }
          break;
        case PortType::Audio:
          if (id->flow_ == PortFlow::Output)
            {
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::STEREO_L))
                return &tr->processor->stereo_out->get_l ();
              else if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::STEREO_R))
                return &tr->processor->stereo_out->get_r ();
            }
          else if (id->flow_ == PortFlow::Input)
            {
              g_return_val_if_fail (tr->processor->stereo_in, NULL);
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::STEREO_L))
                return &tr->processor->stereo_in->get_l ();
              else if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::STEREO_R))
                return &tr->processor->stereo_in->get_r ();
            }
          break;
        case PortType::Control:
          if (ENUM_BITSET_TEST (
                PortIdentifier::Flags, flags, PortIdentifier::Flags::TP_MONO))
            {
              return tr->processor->mono;
            }
          else if (
            ENUM_BITSET_TEST (
              PortIdentifier::Flags, flags, PortIdentifier::Flags::TP_INPUT_GAIN))
            {
              return tr->processor->input_gain;
            }
          else if (
            ENUM_BITSET_TEST (
              PortIdentifier::Flags2, flags2,
              PortIdentifier::Flags2::TP_OUTPUT_GAIN))
            {
              return tr->processor->output_gain;
            }
          else if (
            ENUM_BITSET_TEST (
              PortIdentifier::Flags2, flags2,
              PortIdentifier::Flags2::TP_MONITOR_AUDIO))
            {
              return tr->processor->monitor_audio;
            }
          else if (
            ENUM_BITSET_TEST (
              PortIdentifier::Flags, flags,
              PortIdentifier::Flags::MIDI_AUTOMATABLE))
            {
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::MIDI_PITCH_BEND))
                {
                  return tr->processor->pitch_bend[id->port_index_];
                }
              else if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::MIDI_POLY_KEY_PRESSURE))
                {
                  return tr->processor->poly_key_pressure[id->port_index_];
                }
              else if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::MIDI_CHANNEL_PRESSURE))
                {
                  return tr->processor->channel_pressure[id->port_index_];
                }
              else
                {
                  return tr->processor->midi_cc[id->port_index_];
                }
            }
          break;
        default:
          break;
        }
      break;
    case PortIdentifier::OwnerType::TRACK:
      tr = tracklist_find_track_by_name_hash (TRACKLIST, id->track_name_hash_);
      if (!tr)
        tr = tracklist_find_track_by_name_hash (
          SAMPLE_PROCESSOR->tracklist, id->track_name_hash_);
      g_return_val_if_fail (tr, NULL);
      if (ENUM_BITSET_TEST (
            PortIdentifier::Flags, flags, PortIdentifier::Flags::BPM))
        {
          return tr->bpm_port;
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, flags2, PortIdentifier::Flags2::BEATS_PER_BAR))
        {
          return tr->beats_per_bar_port;
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, flags2, PortIdentifier::Flags2::BEAT_UNIT))
        {
          return tr->beat_unit_port;
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, flags2,
          PortIdentifier::Flags2::TRACK_RECORDING))
        {
          return tr->recording;
        }
      break;
    case PortIdentifier::OwnerType::FADER:
      fader = fader_find_from_port_identifier (id);
      g_return_val_if_fail (fader, NULL);
      switch (id->type_)
        {
        case PortType::Event:
          switch (id->flow_)
            {
            case PortFlow::Input:
              if (fader)
                return fader->midi_in;
              break;
            case PortFlow::Output:
              if (fader)
                return fader->midi_out;
              break;
            default:
              break;
            }
          break;
        case PortType::Audio:
          if (id->flow_ == PortFlow::Output)
            {
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::STEREO_L)
                && fader)
                return &fader->stereo_out->get_l ();
              else if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::STEREO_R)
                && fader)
                return &fader->stereo_out->get_r ();
            }
          else if (id->flow_ == PortFlow::Input)
            {
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::STEREO_L)
                && fader)
                return &fader->stereo_in->get_l ();
              else if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::STEREO_R)
                && fader)
                return &fader->stereo_in->get_r ();
            }
          break;
        case PortType::Control:
          if (id->flow_ == PortFlow::Input)
            {
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::AMPLITUDE)
                && fader)
                return fader->amp;
              else if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags,
                  PortIdentifier::Flags::STEREO_BALANCE)
                && fader)
                return fader->balance;
              else if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::FADER_MUTE)
                && fader)
                return fader->mute;
              else if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::FADER_SOLO)
                && fader)
                return fader->solo;
              else if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::FADER_LISTEN)
                && fader)
                return fader->listen;
              else if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::FADER_MONO_COMPAT)
                && fader)
                return fader->mono_compat_enabled;
              else if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::FADER_SWAP_PHASE)
                && fader)
                return fader->swap_phase;
            }
          break;
        default:
          break;
        }
      g_return_val_if_reached (NULL);
      break;
    case PortIdentifier::OwnerType::CHANNEL_SEND:
      tr = tracklist_find_track_by_name_hash (TRACKLIST, id->track_name_hash_);
      if (!tr)
        tr = tracklist_find_track_by_name_hash (
          SAMPLE_PROCESSOR->tracklist, id->track_name_hash_);
      g_return_val_if_fail (tr, NULL);
      ch = tr->channel;
      g_return_val_if_fail (ch, NULL);
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id->flags2_,
          PortIdentifier::Flags2::CHANNEL_SEND_ENABLED))
        {
          return ch->sends[id->port_index_]->enabled;
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id->flags2_,
          PortIdentifier::Flags2::CHANNEL_SEND_AMOUNT))
        {
          return ch->sends[id->port_index_]->amount;
        }
      else if (id->flow_ == PortFlow::Input)
        {
          if (id->type_ == PortType::Audio)
            {
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, id->flags_,
                  PortIdentifier::Flags::STEREO_L))
                {
                  return &ch->sends[id->port_index_]->stereo_in->get_l ();
                }
              else if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, id->flags_,
                  PortIdentifier::Flags::STEREO_R))
                {
                  return &ch->sends[id->port_index_]->stereo_in->get_r ();
                }
            }
          else if (id->type_ == PortType::Event)
            {
              return ch->sends[id->port_index_]->midi_in;
            }
        }
      else if (id->flow_ == PortFlow::Output)
        {
          if (id->type_ == PortType::Audio)
            {
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, id->flags_,
                  PortIdentifier::Flags::STEREO_L))
                {
                  return &ch->sends[id->port_index_]->stereo_out->get_l ();
                }
              else if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, id->flags_,
                  PortIdentifier::Flags::STEREO_R))
                {
                  return &ch->sends[id->port_index_]->stereo_out->get_r ();
                }
            }
          else if (id->type_ == PortType::Event)
            {
              return ch->sends[id->port_index_]->midi_out;
            }
        }
      else
        {
          g_return_val_if_reached (NULL);
        }
      break;
    case PortIdentifier::OwnerType::HW:
      {
        Port * port = NULL;

        /* note: flows are reversed */
        if (id->flow_ == PortFlow::Output)
          {
            port = hardware_processor_find_port (
              HW_IN_PROCESSOR, id->ext_port_id_.c_str ());
          }
        else if (id->flow_ == PortFlow::Input)
          {
            port = hardware_processor_find_port (
              HW_OUT_PROCESSOR, id->ext_port_id_.c_str ());
          }

        /* only warn when hardware is not
         * connected anymore */
        g_warn_if_fail (port);
        /*g_return_val_if_fail (port, NULL);*/
        return port;
      }
      break;
    case PortIdentifier::OwnerType::PORT_OWNER_TYPE_TRANSPORT:
      switch (id->type_)
        {
        case PortType::Event:
          if (id->flow_ == PortFlow::Input)
            {
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::TRANSPORT_ROLL))
                return TRANSPORT->roll;
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::TRANSPORT_STOP))
                return TRANSPORT->stop;
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::TRANSPORT_BACKWARD))
                return TRANSPORT->backward;
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::TRANSPORT_FORWARD))
                return TRANSPORT->forward;
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::TRANSPORT_LOOP_TOGGLE))
                return TRANSPORT->loop_toggle;
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::TRANSPORT_REC_TOGGLE))
                return TRANSPORT->rec_toggle;
            }
          break;
        default:
          break;
        }
      break;
    case PortIdentifier::OwnerType::MODULATOR_MACRO_PROCESSOR:
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, flags, PortIdentifier::Flags::MODULATOR_MACRO))
        {
          tr =
            tracklist_find_track_by_name_hash (TRACKLIST, id->track_name_hash_);
          g_return_val_if_fail (IS_TRACK_AND_NONNULL (tr), NULL);
          ModulatorMacroProcessor * processor =
            tr->modulator_macros[id->port_index_];
          if (id->flow_ == PortFlow::Input)
            {
              if (id->type_ == PortType::CV)
                {
                  return processor->cv_in;
                }
              else if (id->type_ == PortType::Control)
                {
                  return processor->macro;
                }
            }
          else if (id->flow_ == PortFlow::Output)
            {
              return processor->cv_out;
            }
        }
      break;
    case PortIdentifier::OwnerType::CHANNEL:
      tr = tracklist_find_track_by_name_hash (TRACKLIST, id->track_name_hash_);
      if (!tr)
        tr = tracklist_find_track_by_name_hash (
          SAMPLE_PROCESSOR->tracklist, id->track_name_hash_);
      g_return_val_if_fail (tr, NULL);
      ch = tr->channel;
      g_return_val_if_fail (ch, NULL);
      switch (id->type_)
        {
        case PortType::Event:
          if (id->flow_ == PortFlow::Output)
            {
              return ch->midi_out;
            }
          break;
        case PortType::Audio:
          if (id->flow_ == PortFlow::Output)
            {
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::STEREO_L))
                return &ch->stereo_out->get_l ();
              else if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::STEREO_R))
                return &ch->stereo_out->get_r ();
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

Port::Port (std::string label)
{
  id_.label_ = label;
}

Port::Port (PortType type, PortFlow flow, std::string label) : Port (label)
{
  this->id_.type_ = type;
  this->id_.flow_ = flow;

  switch (type)
    {
    case PortType::Event:
      this->maxf_ = 1.f;
#ifdef _WIN32
      if (AUDIO_ENGINE->midi_backend == MidiBackend::MIDI_BACKEND_WINDOWS_MME)
        {
          zix_sem_init (&this->mme_connections_sem, 1);
        }
#endif
      break;
    case PortType::Control:
      this->minf_ = 0.f;
      this->maxf_ = 1.f;
      this->zerof_ = 0.f;
      break;
    case PortType::Audio:
      this->minf_ = 0.f;
      this->maxf_ = 2.f;
      this->zerof_ = 0.f;
      break;
    case PortType::CV:
      this->minf_ = -1.f;
      this->maxf_ = 1.f;
      this->zerof_ = 0.f;
    default:
      break;
    }
}

Port::Port (
  PortType                  type,
  PortFlow                  flow,
  std::string               label,
  PortIdentifier::OwnerType owner_type,
  void *                    owner)
    : Port (type, flow, label)
{
  this->set_owner (owner_type, owner);
}

StereoPorts::StereoPorts (Port &&l, Port &&r)
    : l_ (std::move (l)), r_ (std::move (r))
{
  l_.id_.flags_ |= PortIdentifier::Flags::STEREO_L;
  r_.id_.flags_ |= PortIdentifier::Flags::STEREO_R;
}

void
StereoPorts::disconnect ()
{
  l_.disconnect_all ();
  r_.disconnect_all ();
}

#ifdef HAVE_JACK
void
Port::receive_midi_events_from_jack (
  const nframes_t start_frame,
  const nframes_t nframes)
{
  if (
    this->internal_type_ != Port::InternalType::JackPort
    || this->id_.type_ != PortType::Event)
    return;

  void *   port_buf = jack_port_get_buffer (JACK_PORT_T (this->data_), nframes);
  uint32_t num_events = jack_midi_get_event_count (port_buf);

  jack_midi_event_t jack_ev;
  for (unsigned i = 0; i < num_events; i++)
    {
      jack_midi_event_get (&jack_ev, port_buf, i);

      if (jack_ev.time >= start_frame && jack_ev.time < start_frame + nframes)
        {
          midi_byte_t channel = jack_ev.buffer[0] & 0xf;
          Track *     track = this->get_track (0);
          if (
            this->id_.owner_type_ == PortIdentifier::OwnerType::TRACK_PROCESSOR
            && !track)
            {
              g_return_if_reached ();
            }

          if (
            this->id_.owner_type_ == PortIdentifier::OwnerType::TRACK_PROCESSOR
            && track
            && (track->type == TrackType::TRACK_TYPE_MIDI || track->type == TrackType::TRACK_TYPE_INSTRUMENT)
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
                this->midi_events_, jack_ev.time, jack_ev.buffer,
                (int) jack_ev.size, F_NOT_QUEUED);
            }
        }
    }

#  if 0
  if (midi_events_has_any (this->midi_events_, F_NOT_QUEUED))
    {
      char designation[600];
      this->get_full_designation ( designation);
      g_debug ("JACK MIDI (%s): have %d events", designation, num_events);
      midi_events_print (this->midi_events_, F_NOT_QUEUED);
    }
#  endif
}

void
Port::receive_audio_data_from_jack (
  const nframes_t start_frames,
  const nframes_t nframes)
{
  if (
    this->internal_type_ != Port::InternalType::JackPort
    || this->id_.type_ != PortType::Audio)
    return;

  float * in;
  in = (float *) jack_port_get_buffer (
    JACK_PORT_T (this->data_), AUDIO_ENGINE->nframes);

  dsp_add2 (&this->buf_[start_frames], &in[start_frames], nframes);
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
  if (
    port->internal_type_ != Port::InternalType::JackPort
    || port->id_.type_ != PortType::Event)
    return;

  jack_port_t * jport = JACK_PORT_T (port->data_);

  if (jack_port_connected (jport) <= 0)
    {
      return;
    }

  void * buf = jack_port_get_buffer (jport, AUDIO_ENGINE->nframes);
  midi_events_copy_to_jack (port->midi_events_, start_frames, nframes, buf);
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
  if (
    port->internal_type_ != Port::InternalType::JackPort
    || port->id_.type_ != PortType::Audio)
    return;

  jack_port_t * jport = JACK_PORT_T (port->data_);

  if (jack_port_connected (jport) <= 0)
    {
      return;
    }

  float * out = (float *) jack_port_get_buffer (jport, AUDIO_ENGINE->nframes);

  dsp_copy (&out[start_frames], &port->buf_[start_frames], nframes);
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
    self->id_.owner_type_
      == PortIdentifier::OwnerType::PORT_OWNER_TYPE_AUDIO_ENGINE
    || self->internal_type_ != Port::InternalType::JackPort
    || self->id_.flow_ != PortFlow::Input)
    return;

  /* append events from JACK if any */
  if (AUDIO_ENGINE->midi_backend == MidiBackend::MIDI_BACKEND_JACK)
    {
      self->receive_midi_events_from_jack (start_frame, nframes);
    }

  /* audio */
  if (AUDIO_ENGINE->audio_backend == AudioBackend::AUDIO_BACKEND_JACK)
    {
      self->receive_audio_data_from_jack (start_frame, nframes);
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
  if (
    self->internal_type_ != Port::InternalType::JackPort
    || self->id_.flow_ != PortFlow::Output)
    return;

  /* send midi events */
  if (AUDIO_ENGINE->midi_backend == MidiBackend::MIDI_BACKEND_JACK)
    {
      send_midi_events_to_jack (self, start_frame, nframes);
    }

  /* send audio data */
  if (AUDIO_ENGINE->audio_backend == AudioBackend::AUDIO_BACKEND_JACK)
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
  if (self->id_.owner_type_ == PortIdentifier::OwnerType::HW)
    {
      /* these are reversed */
      if (self->id_.flow_ == PortFlow::Input)
        flags = JackPortIsOutput;
      else if (self->id_.flow_ == PortFlow::Output)
        flags = JackPortIsInput;
      else
        {
          g_return_if_reached ();
        }
    }
  else
    {
      if (self->id_.flow_ == PortFlow::Input)
        flags = JackPortIsInput;
      else if (self->id_.flow_ == PortFlow::Output)
        flags = JackPortIsOutput;
      else
        {
          g_return_if_reached ();
        }
    }

  const char * type = engine_jack_get_jack_type (self->id_.type_);
  if (!type)
    g_return_if_reached ();

  char label[600];
  self->get_full_designation (label);
  if (expose)
    {
      g_message ("exposing port %s to JACK", label);
      if (!self->data_)
        {
          self->data_ = (void *) jack_port_register (
            AUDIO_ENGINE->client, label, type, flags, 0);
        }
      g_return_if_fail (self->data_);
      self->internal_type_ = Port::InternalType::JackPort;
    }
  else
    {
      g_message ("unexposing port %s from JACK", label);
      if (AUDIO_ENGINE->client)
        {
          g_warn_if_fail (self->data_);
          int ret = jack_port_unregister (
            AUDIO_ENGINE->client, JACK_PORT_T (self->data_));
          if (ret)
            {
              char jack_error[600];
              engine_jack_get_error_message ((jack_status_t) ret, jack_error);
              g_warning ("JACK port unregister error: %s", jack_error);
            }
        }
      self->internal_type_ = Port::InternalType::None;
      self->data_ = NULL;
    }

  self->exposed_to_backend_ = expose;
}
#endif /* HAVE_JACK */

void
Port::sum_data_from_dummy (const nframes_t start_frame, const nframes_t nframes)
{
  if (
    id_.owner_type_ == PortIdentifier::OwnerType::PORT_OWNER_TYPE_AUDIO_ENGINE
    || id_.flow_ != PortFlow::Input || id_.type_ != PortType::Audio
    || AUDIO_ENGINE->audio_backend != AudioBackend::AUDIO_BACKEND_DUMMY
    || AUDIO_ENGINE->midi_backend != MidiBackend::MIDI_BACKEND_DUMMY)
    return;

  if (AUDIO_ENGINE->dummy_input)
    {
      Port * port = NULL;
      if (ENUM_BITSET_TEST (
            PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::STEREO_L))
        {
          port = &AUDIO_ENGINE->dummy_input->get_l ();
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::STEREO_R))
        {
          port = &AUDIO_ENGINE->dummy_input->get_r ();
        }

      if (port)
        {
          dsp_add2 (&buf_[start_frame], &port->buf_[start_frame], nframes);
        }
    }
}

void
StereoPorts::connect_to (StereoPorts &dest, bool locked)
{
  l_.connect_to (dest.l_, locked);
  r_.connect_to (dest.r_, locked);
}

static int
get_num_unlocked (const Port * self, bool sources)
{
  g_return_val_if_fail (
    IS_PORT_AND_NONNULL (self) && self->is_in_active_project (), 0);

  int num_unlocked_conns =
    port_connections_manager_get_unlocked_sources_or_dests (
      PORT_CONNECTIONS_MGR, NULL, &self->id_, sources);

  return num_unlocked_conns;
}

int
Port::get_num_unlocked_dests () const
{
  return get_num_unlocked (this, false);
}

int
Port::get_num_unlocked_srcs () const
{
  return get_num_unlocked (this, true);
}

/**
 * Gathers all ports in the project and appends them
 * in the given array.
 */
void
Port::get_all (GPtrArray * ports)
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
  plugin_identifier_copy (&port->id_.plugin_id_, &pl->id);
  port->id_.track_name_hash_ = pl->id.track_name_hash;
  port->id_.owner_type_ = PortIdentifier::OwnerType::PLUGIN;

  if (port->at_)
    {
      port->at_->port_id = port->id_;
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
  port->id_.track_name_hash_ = track_get_name_hash (*track);
  port->id_.owner_type_ = PortIdentifier::OwnerType::TRACK_PROCESSOR;
}

/**
 * Sets the owner fader & its ID.
 */
static void
set_owner_fader (Port * self, Fader * fader)
{
  PortIdentifier * id = &self->id_;
  id->owner_type_ = PortIdentifier::OwnerType::FADER;
  self->fader_ = fader;

  if (
    fader->type == FaderType::FADER_TYPE_AUDIO_CHANNEL
    || fader->type == FaderType::FADER_TYPE_MIDI_CHANNEL)
    {
      Track * track = fader->track;
      g_return_if_fail (track && track->name);
      self->id_.track_name_hash_ = track_get_name_hash (*track);
      if (fader->passthrough)
        {
          id->flags2_ |= PortIdentifier::Flags2::PREFADER;
        }
      else
        {
          id->flags2_ |= PortIdentifier::Flags2::POSTFADER;
        }
    }
  else if (fader->type == FaderType::FADER_TYPE_SAMPLE_PROCESSOR)
    {
      id->flags2_ |= PortIdentifier::Flags2::SAMPLE_PROCESSOR_FADER;
    }
  else
    {
      id->flags2_ |= PortIdentifier::Flags2::MonitorFader;
    }

  if (ENUM_BITSET_TEST (
        PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::AMPLITUDE))
    {
      self->minf_ = 0.f;
      self->maxf_ = 2.f;
      self->zerof_ = 0.f;
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::STEREO_BALANCE))
    {
      self->minf_ = 0.f;
      self->maxf_ = 1.f;
      self->zerof_ = 0.5f;
    }
}

/**
 * Sets the owner track & its ID.
 */
NONNULL static void
set_owner_track (Port * port, Track * track)
{
  g_return_if_fail (track->name);
  port->id_.track_name_hash_ = track_get_name_hash (*track);
  port->id_.owner_type_ = PortIdentifier::OwnerType::TRACK;
}

/**
 * Sets the channel send as the port's owner.
 */
NONNULL static void
set_owner_channel_send (Port * self, ChannelSend * send)
{
  self->id_.track_name_hash_ = send->track_name_hash;
  self->id_.port_index_ = send->slot;
  self->id_.owner_type_ = PortIdentifier::OwnerType::CHANNEL_SEND;
  self->channel_send_ = send;

  if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags2, self->id_.flags2_,
      PortIdentifier::Flags2::CHANNEL_SEND_ENABLED))
    {
      self->minf_ = 0.f;
      self->maxf_ = 1.f;
      self->zerof_ = 0.0f;
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags2, self->id_.flags2_,
      PortIdentifier::Flags2::CHANNEL_SEND_AMOUNT))
    {
      self->minf_ = 0.f;
      self->maxf_ = 2.f;
      self->zerof_ = 0.f;
    }
}

NONNULL static void
set_owner_channel (Port * port, Channel * ch)
{
  Track &track = ch->track_;
  g_return_if_fail (track.name);
  port->id_.track_name_hash_ = track_get_name_hash (track);
  port->id_.owner_type_ = PortIdentifier::OwnerType::CHANNEL;
}

NONNULL static void
set_owner_transport (Port * self, Transport * transport)
{
  self->transport_ = transport;
  self->id_.owner_type_ = PortIdentifier::OwnerType::PORT_OWNER_TYPE_TRANSPORT;
}

NONNULL static void
set_owner_modulator_macro_processor (Port * self, ModulatorMacroProcessor * mmp)
{
  self->modulator_macro_processor_ = mmp;
  self->id_.owner_type_ = PortIdentifier::OwnerType::MODULATOR_MACRO_PROCESSOR;
  g_return_if_fail (IS_TRACK_AND_NONNULL (mmp->track));
  self->id_.track_name_hash_ = track_get_name_hash (*mmp->track);
  self->track_ = mmp->track;
}

NONNULL static void
set_owner_audio_engine (Port * self, AudioEngine * engine)
{
  self->engine_ = engine;
  self->id_.owner_type_ =
    PortIdentifier::OwnerType::PORT_OWNER_TYPE_AUDIO_ENGINE;
}

NONNULL static void
set_owner_ext_port (Port * self, ExtPort * ext_port)
{
  self->ext_port_ = ext_port;
  self->id_.owner_type_ = PortIdentifier::OwnerType::HW;
}

NONNULL void
Port::set_owner (PortIdentifier::OwnerType owner_type, void * owner)
{
  switch (owner_type)
    {
    case PortIdentifier::OwnerType::CHANNEL_SEND:
      set_owner_channel_send (this, (ChannelSend *) owner);
      break;
    case PortIdentifier::OwnerType::FADER:
      set_owner_fader (this, (Fader *) owner);
      break;
    case PortIdentifier::OwnerType::TRACK:
      set_owner_track (this, (Track *) owner);
      break;
    case PortIdentifier::OwnerType::TRACK_PROCESSOR:
      set_owner_track_processor (this, (TrackProcessor *) owner);
      break;
    case PortIdentifier::OwnerType::CHANNEL:
      set_owner_channel (this, (Channel *) owner);
      break;
    case PortIdentifier::OwnerType::PLUGIN:
      set_owner_plugin (this, (Plugin *) owner);
      break;
    case PortIdentifier::OwnerType::PORT_OWNER_TYPE_TRANSPORT:
      set_owner_transport (this, (Transport *) owner);
      break;
    case PortIdentifier::OwnerType::MODULATOR_MACRO_PROCESSOR:
      set_owner_modulator_macro_processor (
        this, (ModulatorMacroProcessor *) owner);
      break;
    case PortIdentifier::OwnerType::PORT_OWNER_TYPE_AUDIO_ENGINE:
      set_owner_audio_engine (this, (AudioEngine *) owner);
      break;
    case PortIdentifier::OwnerType::HW:
      set_owner_ext_port (this, (ExtPort *) owner);
      break;
    default:
      g_return_if_reached ();
    }
}

bool
Port::can_be_connected_to (const Port * dest) const
{
  Graph * graph = graph_new (ROUTER);
  bool    valid = graph_validate_with_connection (graph, this, dest);
  graph_free (graph);

  return valid;
}

void
Port::disconnect_ports (Port ** ports, int num_ports, bool deleting)
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
        PORT_CONNECTIONS_MGR, &port->id_);

      port->srcs_.clear ();
      port->dests_.clear ();
      port->deleting_ = deleting;
    }
}

int
Port::disconnect_all ()
{
  g_return_val_if_fail (IS_PORT (this), -1);

  this->srcs_.clear ();
  this->dests_.clear ();

  if (!PORT_CONNECTIONS_MGR)
    return 0;

  if (!this->is_in_active_project ())
    {
#if 0
      g_debug ("%s (%p) is not a project port, skipping", this->id_.label, this);
#endif
      return 0;
    }

  GPtrArray * srcs = g_ptr_array_new ();
  int         num_srcs = port_connections_manager_get_sources_or_dests (
    PORT_CONNECTIONS_MGR, srcs, &this->id_, true);
  for (int i = 0; i < num_srcs; i++)
    {
      PortConnection * conn = (PortConnection *) g_ptr_array_index (srcs, i);
      port_connections_manager_ensure_disconnect (
        PORT_CONNECTIONS_MGR, conn->src_id, conn->dest_id);
    }
  g_ptr_array_unref (srcs);

  GPtrArray * dests = g_ptr_array_new ();
  int         num_dests = port_connections_manager_get_sources_or_dests (
    PORT_CONNECTIONS_MGR, dests, &this->id_, false);
  for (int i = 0; i < num_dests; i++)
    {
      PortConnection * conn = (PortConnection *) g_ptr_array_index (dests, i);
      port_connections_manager_ensure_disconnect (
        PORT_CONNECTIONS_MGR, conn->src_id, conn->dest_id);
    }
  g_ptr_array_unref (dests);

#ifdef HAVE_JACK
  if (this->internal_type_ == Port::InternalType::JackPort)
    {
      expose_to_jack (this, false);
    }
#endif

#ifdef HAVE_RTMIDI
  for (auto it = rtmidi_ins_.begin (); it != rtmidi_ins_.end (); ++it)
    {
      auto dev = *it;
      rtmidi_device_close (dev, true);
      it = rtmidi_ins_.erase (it);
    }
#endif

  return 0;
}

void
Port::update_identifier (
  const PortIdentifier * prev_id,
  Track *                track,
  bool                   update_automation_track)
{
  /*g_message (*/
  /*"updating identifier for %p %s (track pos %d)", this, this->id_.label,
   * this->id_.track_pos);*/

  if (this->is_in_active_project ())
    {
      /* update in all sources */
      GPtrArray * srcs = g_ptr_array_new ();
      int         num_srcs = port_connections_manager_get_sources_or_dests (
        PORT_CONNECTIONS_MGR, srcs, prev_id, true);
      for (int i = 0; i < num_srcs; i++)
        {
          PortConnection * conn = (PortConnection *) g_ptr_array_index (srcs, i);
          if (!conn->dest_id->is_equal (this->id_))
            {
              *conn->dest_id = this->id_;
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
          if (!conn->src_id->is_equal (this->id_))
            {
              *conn->src_id = this->id_;
              port_connections_manager_regenerate_hashtables (
                PORT_CONNECTIONS_MGR);
            }
        }
      g_ptr_array_unref (dests);

      if (
        update_automation_track && this->id_.track_name_hash_
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags, this->id_.flags_,
          PortIdentifier::Flags::AUTOMATABLE))
        {
          /* update automation track's port id */
          this->at_ = automation_track_find_from_port (this, track, true);
          AutomationTrack * at = this->at_;
          g_return_if_fail (at);
          at->port_id = this->id_;
        }
    }
}

void
Port::update_track_name_hash (Track * track, unsigned int new_hash)
{
  PortIdentifier copy = PortIdentifier (this->id_);

  this->id_.track_name_hash_ = new_hash;
  if (this->id_.owner_type_ == PortIdentifier::OwnerType::PLUGIN)
    {
      this->id_.plugin_id_.track_name_hash = new_hash;
    }
  update_identifier (&copy, track, F_UPDATE_AUTOMATION_TRACK);
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
  Port::get_all (ports, &num_ports);
  for (int i = 0; i < num_ports; i++)
    {
      port = ports[i];

      if (port->internal_type_ ==
            Port::InternalType::INTERNAL_ALSA_SEQ_PORT &&
          snd_seq_port_info_get_port (
            (snd_seq_port_info_t *) port->data_) ==
            id)
        return port;
    }
  g_return_val_if_reached (NULL);
}

static void
expose_to_alsa (
  Port * this,
  int    expose)
{
  if (this->id_.type_ == PortType::Event)
    {
      unsigned int flags = 0;
      if (this->id_.flow_ == PortFlow::Input)
        flags =
          SND_SEQ_PORT_CAP_WRITE |
          SND_SEQ_PORT_CAP_SUBS_WRITE;
      else if (this->id_.flow_ == PortFlow::Output)
        flags =
          SND_SEQ_PORT_CAP_READ |
          SND_SEQ_PORT_CAP_SUBS_READ;
      else
        g_return_if_reached ();

      if (expose)
        {
          char lbl[600];
          this->get_full_designation ( lbl);

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
          this->data_ = (void *) info;
          this->internal_type_ =
            Port::InternalType::INTERNAL_ALSA_SEQ_PORT;
        }
      else
        {
          snd_seq_delete_port (
            AUDIO_ENGINE->seq_handle,
            snd_seq_port_info_get_port (
              (snd_seq_port_info_t *)
                this->data_));
          this->internal_type_ = Port::InternalType::INTERNAL_NONE;
          this->data_ = NULL;
        }
    }
  this->exposed_to_backend = expose;
}
#  endif
#endif // HAVE_ALSA

#ifdef HAVE_RTMIDI
static void
expose_to_rtmidi (Port * self, int expose)
{
  char lbl[600];
  self->get_full_designation (lbl);
  if (expose)
    {
#  if 0

      if (self->id_.flow_ == PortFlow::Input)
        {
          self->rtmidi_in =
            rtmidi_in_create (
#    ifdef _WIN32
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
      if (self->id_.flow_ == PortFlow::Input &&
          self->rtmidi_in)
        {
          rtmidi_close_port (self->rtmidi_in);
          self->rtmidi_in = NULL;
        }
#  endif
      g_message ("unexposing %s", lbl);
    }
  self->exposed_to_backend_ = expose;
}

void
Port::sum_data_from_rtmidi (const nframes_t start_frame, const nframes_t nframes)
{
  g_return_if_fail (
    /*this->id_.flow_ == PortFlow::Input &&*/
    midi_backend_is_rtmidi (AUDIO_ENGINE->midi_backend));

  for (auto &dev : rtmidi_ins_)
    {
      MidiEvent * ev;
      for (int j = 0; j < dev->events->num_events; j++)
        {
          ev = &dev->events->events[j];

          if (ev->time >= start_frame && ev->time < start_frame + nframes)
            {
              midi_byte_t channel = ev->raw_buffer[0] & 0xf;
              Track *     track = this->get_track (0);
              if (
                this->id_.owner_type_ == PortIdentifier::OwnerType::TRACK_PROCESSOR
                && !track)
                {
                  g_return_if_reached ();
                }

              if (
                this->id_.owner_type_ == PortIdentifier::OwnerType::TRACK_PROCESSOR
                && track
                && (track->type == TrackType::TRACK_TYPE_MIDI || track->type == TrackType::TRACK_TYPE_INSTRUMENT)
                && !track->channel->all_midi_channels
                && !track->channel->midi_channels[channel])
                {
                  /* different channel */
                }
              else
                {
                  midi_events_add_event_from_buf (
                    this->midi_events_, ev->time, ev->raw_buffer, 3,
                    F_NOT_QUEUED);
                }
            }
        }
    }

#  if 0
  if (DEBUGGING &&
      this->midi_events_->num_events > 0)
    {
      MidiEvent * ev =
        &this->midi_events_->events[0];
      char designation[600];
      port_get_full_designation (
        this, designation);
      g_message (
        "RtMidi (%s): have %d events\n"
        "first event is: [%u] %hhx %hhx %hhx",
        designation, this->midi_events_->num_events,
        ev->time, ev->raw_buffer[0],
        ev->raw_buffer[1], ev->raw_buffer[2]);
    }
#  endif
}

void
Port::prepare_rtmidi_events ()
{
  g_return_if_fail (
    /*this->id_.flow_ == PortFlow::Input &&*/
    midi_backend_is_rtmidi (AUDIO_ENGINE->midi_backend));

  gint64 cur_time = g_get_monotonic_time ();
  for (auto &dev : rtmidi_ins_)
    {
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
          gint64      length = cur_time - this->last_midi_dequeue_;
          midi_time_t ev_time =
            (midi_time_t) (((double) h.time / (double) length)
                           * (double) AUDIO_ENGINE->block_length);

          if (ev_time >= AUDIO_ENGINE->block_length)
            {
              g_warning (
                "event with invalid time %u received. the maximum allowed time "
                "is %" PRIu32 ". setting it to %u...",
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
  this->last_midi_dequeue_ = cur_time;
}
#endif // HAVE_RTMIDI

#ifdef HAVE_RTAUDIO
static void
expose_to_rtaudio (Port * self, int expose)
{
  Track * track = self->get_track (0);
  if (!track)
    return;

  Channel * ch = track->channel;
  if (!ch)
    return;

  char lbl[600];
  self->get_full_designation (lbl);
  if (expose)
    {
      if (self->id_.flow_ == PortFlow::Input)
        {
          if (
            ENUM_BITSET_TEST (
              PortIdentifier::Flags, self->id_.flags_,
              PortIdentifier::Flags::STEREO_L))
            {
              if (ch->all_stereo_l_ins)
                {
                }
              else
                {
                  for (int i = 0; i < ch->num_ext_stereo_l_ins; i++)
                    {
                      ExtPort * ext_port = ch->ext_stereo_l_ins[i];
                      ext_port_print (ext_port);
                      auto dev = rtaudio_device_new (
                        ext_port->rtaudio_is_input, NULL, ext_port->rtaudio_id,
                        ext_port->rtaudio_channel_idx, self);
                      rtaudio_device_open (dev, true);
                      self->rtaudio_ins_.push_back (dev);
                    }
                }
            }
          else if (
            ENUM_BITSET_TEST (
              PortIdentifier::Flags, self->id_.flags_,
              PortIdentifier::Flags::STEREO_R))
            {
              if (ch->all_stereo_r_ins)
                {
                }
              else
                {
                  for (int i = 0; i < ch->num_ext_stereo_r_ins; i++)
                    {
                      ExtPort * ext_port = ch->ext_stereo_r_ins[i];
                      ext_port_print (ext_port);
                      auto dev = rtaudio_device_new (
                        ext_port->rtaudio_is_input, NULL, ext_port->rtaudio_id,
                        ext_port->rtaudio_channel_idx, self);
                      rtaudio_device_open (dev, true);
                      self->rtaudio_ins_.push_back (dev);
                    }
                }
            }
        }
      g_message ("exposing %s", lbl);
    }
  else
    {
      if (self->id_.flow_ == PortFlow::Input)
        {
          for (
            auto it = self->rtaudio_ins_.begin ();
            it != self->rtaudio_ins_.end (); ++it)
            {
              auto dev = *it;
              rtaudio_device_close (dev, true);
              it = self->rtaudio_ins_.erase (it);
            }
        }
      g_message ("unexposing %s", lbl);
    }
  self->exposed_to_backend_ = expose;
}

void
Port::prepare_rtaudio_data ()
{
  g_return_if_fail (
    /*this->id_.flow_ == PortFlow::Input &&*/
    audio_backend_is_rtaudio (AUDIO_ENGINE->audio_backend));

  for (auto &dev : rtaudio_ins_)
    {
      /* clear the data */
      dsp_fill (
        dev->buf, DENORMAL_PREVENTION_VAL (AUDIO_ENGINE), AUDIO_ENGINE->nframes);

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

void
Port::sum_data_from_rtaudio (const nframes_t start_frame, const nframes_t nframes)
{
  g_return_if_fail (
    /*this->id_.flow_ == PortFlow::Input &&*/
    audio_backend_is_rtaudio (AUDIO_ENGINE->audio_backend));

  for (auto &dev : rtaudio_ins_)
    {
      dsp_add2 (&this->buf_[start_frame], &dev->buf[start_frame], nframes);
    }
}
#endif // HAVE_RTAUDIO

#ifdef _WIN32
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
    self->id_.flow_ == PortFlow::Input
    && AUDIO_ENGINE->midi_backend == MidiBackend::MIDI_BACKEND_WINDOWS_MME);

  if (
    self->id_.owner_type
    == PortIdentifier::OwnerType::PORT_OWNER_TYPE_AUDIO_ENGINE)
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
              Track *     track = self->get_track (0);
              if (
                self->id_.owner_type_ == PortIdentifier::OwnerType::TRACK_PROCESSOR
                && (track->type == TrackType::TRACK_TYPE_MIDI || track->type == TrackType::TRACK_TYPE_INSTRUMENT)
                && !track->channel->all_midi_channels
                && !track->channel->midi_channels[channel])
                {
                  /* different channel */
                }
              else
                {
                  midi_events_add_event_from_buf (
                    self->midi_events_, ev.time, ev.raw_buffer, 3, 0);
                }
            }
        }
      self->last_midi_dequeue = cur_time;

#  if 0
      if (self->midi_events_->num_events > 0)
        {
          MidiEvent * ev =
            &self->midi_events_->events[0];
          char designation[600];
          port_get_full_designation (
            self, designation);
          g_message (
            "MME MIDI (%s): have %d events\n"
            "first event is: [%u] %hhx %hhx %hhx",
            designation,
            self->midi_events_->num_events,
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
    self->id_.flow_ == PortFlow::Output
    && AUDIO_ENGINE->midi_backend == MidiBackend::MIDI_BACKEND_WINDOWS_MME);

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
  if (self->id_.owner_type_ == PortIdentifier::OwnerType::PLUGIN)
    {
      Plugin * pl = self->get_plugin (1);
      if (pl)
        {
#ifdef HAVE_CARLA
          if (pl->setting->open_with_carla && self->carla_param_id_ >= 0)
            {
              g_return_if_fail (pl->carla);
              carla_native_plugin_set_param_value (
                pl->carla, (uint32_t) self->carla_param_id_, self->control_);
            }
#endif
          if (!g_atomic_int_get (&pl->state_changed_event_sent))
            {
              EVENTS_PUSH (EventType::ET_PLUGIN_STATE_CHANGED, pl);
              g_atomic_int_set (&pl->state_changed_event_sent, 1);
            }
        }
    }
  else if (self->id_.owner_type_ == PortIdentifier::OwnerType::FADER)
    {
      Track * track = self->get_track (1);
      g_return_if_fail (track && track->channel);

      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, self->id_.flags_,
          PortIdentifier::Flags::FADER_MUTE)
        || ENUM_BITSET_TEST (
          PortIdentifier::Flags2, self->id_.flags2_,
          PortIdentifier::Flags2::FADER_SOLO)
        || ENUM_BITSET_TEST (
          PortIdentifier::Flags2, self->id_.flags2_,
          PortIdentifier::Flags2::FADER_LISTEN)
        || ENUM_BITSET_TEST (
          PortIdentifier::Flags2, self->id_.flags2_,
          PortIdentifier::Flags2::FADER_MONO_COMPAT))
        {
          EVENTS_PUSH (EventType::ET_TRACK_FADER_BUTTON_CHANGED, track);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, self->id_.flags_,
          PortIdentifier::Flags::AMPLITUDE))
        {
          if (ZRYTHM_HAVE_UI)
            g_return_if_fail (track->channel->widget);
          fader_update_volume_and_fader_val (track->channel->fader);
          EVENTS_PUSH (EventType::ET_CHANNEL_FADER_VAL_CHANGED, track->channel);
        }
    }
  else if (self->id_.owner_type_ == PortIdentifier::OwnerType::CHANNEL_SEND)
    {
      Track * track = self->get_track (1);
      g_return_if_fail (IS_TRACK_AND_NONNULL (track));
      ChannelSend * send = track->channel->sends[self->id_.port_index_];
      EVENTS_PUSH (EventType::ET_CHANNEL_SEND_CHANGED, send);
    }
  else if (self->id_.owner_type_ == PortIdentifier::OwnerType::TRACK)
    {
      Track * track = self->get_track (1);
      EVENTS_PUSH (EventType::ET_TRACK_STATE_CHANGED, track);
    }
}

void
Port::set_control_value (
  const float val,
  const bool  is_normalized,
  const bool  forward_event)
{
  PortIdentifier * id = &this->id_;

  /* set the base value */
  if (is_normalized)
    {
      float minf = this->minf_;
      float maxf = this->maxf_;
      this->base_value_ = minf + val * (maxf - minf);
    }
  else
    {
      this->base_value_ = val;
    }

  this->unsnapped_control_ = this->base_value_;
  this->base_value_ =
    control_port_get_snapped_val_from_val (this, this->unsnapped_control_);

  if (!math_floats_equal (this->control_, this->base_value_))
    {
      this->control_ = this->base_value_;

      /* remember time */
      this->last_change_ = g_get_monotonic_time ();
      this->value_changed_from_reading_ = false;

      /* if bpm, update engine */
      if (ENUM_BITSET_TEST (
            PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::BPM))
        {
          /* this must only be called during processing kickoff or while the
           * engine is stopped */
          g_return_if_fail (
            !engine_get_run (AUDIO_ENGINE)
            || router_is_processing_kickoff_thread (ROUTER));

          int beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
          engine_update_frames_per_tick (
            AUDIO_ENGINE, beats_per_bar, this->control_,
            AUDIO_ENGINE->sample_rate, false, true, true);
          EVENTS_PUSH (EventType::ET_BPM_CHANGED, NULL);
        }

      /* if time sig value, update transport caches */
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id->flags2_,
          PortIdentifier::Flags2::BEATS_PER_BAR)
        || ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id->flags2_, PortIdentifier::Flags2::BEAT_UNIT))
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
          bool update_from_ticks = ENUM_BITSET_TEST (
            PortIdentifier::Flags2, id->flags2_,
            PortIdentifier::Flags2::BEATS_PER_BAR);
          engine_update_frames_per_tick (
            AUDIO_ENGINE, beats_per_bar, bpm, AUDIO_ENGINE->sample_rate, false,
            update_from_ticks, false);
          EVENTS_PUSH (EventType::ET_TIME_SIGNATURE_CHANGED, NULL);
        }

      /* if plugin enabled port, also set plugin's own enabled port value and
       * vice versa */
      if (
        this->is_in_active_project ()
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags, this->id_.flags_,
          PortIdentifier::Flags::PLUGIN_ENABLED))
        {
          Plugin * pl = this->get_plugin (1);
          g_return_if_fail (pl);

          if (
            ENUM_BITSET_TEST (
              PortIdentifier::Flags, this->id_.flags_,
              PortIdentifier::Flags::GENERIC_PLUGIN_PORT))
            {
              if (
                pl->own_enabled_port
                && !math_floats_equal (
                  pl->own_enabled_port->control_, this->control_))
                {
                  g_debug (
                    "generic enabled changed - changing plugin's own enabled");

                  pl->own_enabled_port->set_control_value (
                    this->control_, false, F_PUBLISH_EVENTS);
                }
            }
          else if (!math_floats_equal (pl->enabled->control_, this->control_))
            {
              g_debug (
                "plugin's own enabled changed - changing generic enabled");
              pl->enabled->set_control_value (
                this->control_, false, F_PUBLISH_EVENTS);
            }
        } /* endif plugin-enabled port */

      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, this->id_.flags_,
          PortIdentifier::Flags::MIDI_AUTOMATABLE))
        {
          Track * track = this->get_track (true);
          g_return_if_fail (track);
          mpmc_queue_push_back (
            track->processor->updated_midi_automatable_ports, this);
        }

    } /* endif port value changed */

  if (forward_event)
    {
      port_forward_control_change_event (this);
    }
}

float
Port::get_control_value (const bool normalize) const
{
  g_return_val_if_fail (this->id_.type_ == PortType::Control, 0.f);

  /* verify that plugin exists if plugin control */
  if (
    ZRYTHM_TESTING && this->is_in_active_project ()
    && ENUM_BITSET_TEST (
      PortIdentifier::Flags, this->id_.flags_,
      PortIdentifier::Flags::PLUGIN_CONTROL))
    {
      Plugin * pl = this->get_plugin (1);
      g_return_val_if_fail (IS_PLUGIN_AND_NONNULL (pl), 0.f);
    }

  if (normalize)
    {
      return control_port_real_val_to_normalized (this, this->control_);
    }
  else
    {
      return this->control_;
    }
}

Port::ScalePoint::ScalePoint (float val, std::string label)
{
  this->val_ = val;
  this->label_ = label;
}

void
Port::copy_metadata_from_project (Port * prj_port)
{
  this->control_ = prj_port->control_;
}

void
Port::copy_values (const Port &other)
{
  /* set value */
  this->control_ = other.control_;
}

void
Port::restore_from_non_project (Port &non_project)
{
  /* set value */
  control_ = non_project.control_;
}

StereoPorts::StereoPorts (
  bool                      input,
  std::string               name,
  std::string               symbol,
  PortIdentifier::OwnerType owner_type,
  void *                    owner)
    : StereoPorts (
      Port (
        PortType::Audio,
        input ? PortFlow::Input : PortFlow::Output,
        fmt::format ("{} L", name)),
      Port (
        PortType::Audio,
        input ? PortFlow::Input : PortFlow::Output,
        fmt::format ("{} R", name)))
{
  l_.id_.flags_ |= PortIdentifier::Flags::STEREO_L;
  l_.id_.sym_ = fmt::format ("{}_l", symbol);
  r_.id_.flags_ |= PortIdentifier::Flags::STEREO_R;
  r_.id_.sym_ = fmt::format ("{}_r", symbol);
  l_.set_owner (owner_type, owner);
  r_.set_owner (owner_type, owner);
}

void
Port::process (const EngineProcessTimeInfo time_nfo, const bool noroll)
{
  const PortIdentifier      id = id_;
  PortIdentifier::OwnerType owner_type = id.owner_type_;

  Track * track = NULL;
  if (
    owner_type == PortIdentifier::OwnerType::TRACK_PROCESSOR
    || owner_type == PortIdentifier::OwnerType::TRACK
    || owner_type == PortIdentifier::OwnerType::CHANNEL ||
    /* if track/channel fader */
    (owner_type == PortIdentifier::OwnerType::FADER
     && (ENUM_BITSET_TEST (PortIdentifier::Flags2, id.flags2_, PortIdentifier::Flags2::PREFADER) || ENUM_BITSET_TEST (PortIdentifier::Flags2, id.flags2_, PortIdentifier::Flags2::POSTFADER)))
    || (owner_type == PortIdentifier::OwnerType::PLUGIN && id.plugin_id_.slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT))
    {
      if (ZRYTHM_TESTING)
        track = get_track (true);
      else
        track = track_;
      g_return_if_fail (IS_TRACK_AND_NONNULL (track));
    }

  bool is_stereo_port =
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id.flags_, PortIdentifier::Flags::STEREO_L)
    || ENUM_BITSET_TEST (
      PortIdentifier::Flags, id.flags_, PortIdentifier::Flags::STEREO_R);

  switch (id.type_)
    {
    case PortType::Event:
      if (noroll)
        break;

      if (G_UNLIKELY (
            owner_type == PortIdentifier::OwnerType::TRACK_PROCESSOR && !track))
        {
          g_return_if_reached ();
        }

      /* if piano roll keys, add the notes to the
       * piano roll "current notes" (to show
       * pressed keys in the UI) */
      if (
        G_UNLIKELY (
          owner_type == PortIdentifier::OwnerType::TRACK_PROCESSOR && track
          && this == track->processor->midi_out && midi_events_->num_events > 0
          && CLIP_EDITOR->has_region
          && CLIP_EDITOR->region_id.track_name_hash
               == track_get_name_hash (*track)))
        {
          MidiEvents * events = midi_events_;
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
              EVENTS_PUSH (EventType::ET_PIANO_ROLL_KEY_ON_OFF, NULL);
            }
        }

      /* only consider incoming external data if armed for recording (if the
       * port is owner by a track), otherwise always consider incoming external
       * data */
      if ((owner_type != PortIdentifier::OwnerType::TRACK_PROCESSOR || (owner_type == PortIdentifier::OwnerType::TRACK_PROCESSOR && track && track_type_can_record (track->type) && track_get_recording (track))) && id.flow_ == PortFlow::Input)
        {
          switch (AUDIO_ENGINE->midi_backend)
            {
#ifdef HAVE_JACK
            case MidiBackend::MIDI_BACKEND_JACK:
              sum_data_from_jack (this, time_nfo.local_offset, time_nfo.nframes);
              break;
#endif
#ifdef _WIN32
            case MidiBackend::MIDI_BACKEND_WINDOWS_MME:
              sum_data_from_windows_mme (
                this, time_nfo.local_offset, time_nfo.nframes);
              break;
#endif
#ifdef HAVE_RTMIDI
            case MidiBackend::MIDI_BACKEND_ALSA_RTMIDI:
            case MidiBackend::MIDI_BACKEND_JACK_RTMIDI:
            case MidiBackend::MIDI_BACKEND_WINDOWS_MME_RTMIDI:
            case MidiBackend::MIDI_BACKEND_COREMIDI_RTMIDI:
#  ifdef HAVE_RTMIDI_6
            case MidiBackend::MIDI_BACKEND_WINDOWS_UWP_RTMIDI:
#  endif
              sum_data_from_rtmidi (time_nfo.local_offset, time_nfo.nframes);
              break;
#endif
            default:
              break;
            }
        }

      /* set midi capture if hardware */
      if (owner_type == PortIdentifier::OwnerType::HW)
        {
          MidiEvents * events = midi_events_;
          if (events->num_events > 0)
            {
              AUDIO_ENGINE->trigger_midi_activity = 1;

              /* queue playback if recording and
               * we should record on MIDI input */
              if (
                TRANSPORT_IS_RECORDING && TRANSPORT_IS_PAUSED
                && TRANSPORT->start_playback_on_midi_input)
                {
                  EVENTS_PUSH (EventType::ET_TRANSPORT_ROLL_REQUIRED, NULL);
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
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id_.flags2_, PortIdentifier::Flags2::MIDI_CLOCK)
        && id_.flow_ == PortFlow::Output)
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
              midi_events_add_raw (midi_events_, &start_msg, 1, 0, false);
            }
          else if (
            !TRANSPORT_IS_ROLLING && AUDIO_ENGINE->pos_nfo_before.is_rolling)
            {
              uint8_t stop_msg = MIDI_CLOCK_STOP;
              midi_events_add_raw (midi_events_, &stop_msg, 1, 0, false);
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
                midi_events_, sixteenth_within_song, 0, false);
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
                  double      ratio = (ninetysixth_ticks - AUDIO_ENGINE->pos_nfo_current.playhead_ticks) / (AUDIO_ENGINE->pos_nfo_at_end.playhead_ticks - AUDIO_ENGINE->pos_nfo_current.playhead_ticks);
                  midi_time_t midi_time = (midi_time_t) floor (
                    ratio * (double) AUDIO_ENGINE->block_length);
                  if (
                    midi_time >= time_nfo.local_offset
                    && midi_time < time_nfo.local_offset + time_nfo.nframes)
                    {
                      uint8_t beat_msg = MIDI_CLOCK_BEAT;
                      midi_events_add_raw (
                        midi_events_, &beat_msg, 1, midi_time, false);
#if 0
                      g_debug (
                        "(i = %d) time %u / %u", i, midi_time,
                        time_nfo.local_offset + time_nfo.nframes);
#endif
                    }
                }
            }

          midi_events_sort (midi_events_, false);
        }

      /* append data from each source */
      for (size_t k = 0; k < srcs_.size (); k++)
        {
          Port *                 src_port = srcs_[k];
          const PortConnection * conn = src_connections_[k];
          if (!conn->enabled)
            continue;

          g_return_if_fail (src_port->id_.type_ == PortType::Event);

          /* if hardware device connected to
           * track processor input, only allow
           * signal to pass if armed and
           * MIDI channel is valid */
          if (
            src_port->id_.owner_type_ == PortIdentifier::OwnerType::HW
            && owner_type == PortIdentifier::OwnerType::TRACK_PROCESSOR)
            {
              g_return_if_fail (track);

              /* skip if not armed */
              if (!track_get_recording (track))
                continue;

              /* if not set to "all channels",
               * filter-append */
              if (
                (track->type == TrackType::TRACK_TYPE_MIDI
                 || track->type == TrackType::TRACK_TYPE_INSTRUMENT)
                && !track->channel->all_midi_channels)
                {
                  midi_events_append_w_filter (
                    midi_events_, src_port->midi_events_,
                    track->channel->midi_channels, time_nfo.local_offset,
                    time_nfo.nframes, F_NOT_QUEUED);
                  continue;
                }

              /* otherwise append normally */
            }

          midi_events_append (
            midi_events_, src_port->midi_events_, time_nfo.local_offset,
            time_nfo.nframes, F_NOT_QUEUED);
        } /* foreach source */

      if (id.flow_ == PortFlow::Output)
        {
          switch (AUDIO_ENGINE->midi_backend)
            {
#ifdef HAVE_JACK
            case MidiBackend::MIDI_BACKEND_JACK:
              send_data_to_jack (this, time_nfo.local_offset, time_nfo.nframes);
              break;
#endif
#ifdef _WIN32
            case MidiBackend::MIDI_BACKEND_WINDOWS_MME:
              send_data_to_windows_mme (
                this, time_nfo.local_offset, time_nfo.nframes);
              break;
#endif
            default:
              break;
            }
        }

      /* send UI notification */
      if (midi_events_->num_events > 0)
        {
#if 0
          g_message (
            "port %s has %d events",
            id.label,
            midi_events_->num_events);
#endif

          if (owner_type == PortIdentifier::OwnerType::TRACK_PROCESSOR)
            {
              g_return_if_fail (IS_TRACK_AND_NONNULL (track));

              track->trigger_midi_activity = 1;
            }
        }

      if (time_nfo.local_offset + time_nfo.nframes == AUDIO_ENGINE->block_length)
        {
          MidiEvents * events = midi_events_;
          if (write_ring_buffers_)
            {
              for (int i = events->num_events - 1; i >= 0; i--)
                {
                  if (zix_ring_write_space (midi_ring_) < sizeof (MidiEvent))
                    {
                      zix_ring_skip (midi_ring_, sizeof (MidiEvent));
                    }

                  MidiEvent * ev = &events->events[i];
                  ev->systime = g_get_monotonic_time ();
                  zix_ring_write (midi_ring_, ev, sizeof (MidiEvent));
                }
            }
          else
            {
              if (events->num_events > 0)
                {
                  last_midi_event_time_ = g_get_monotonic_time ();
                  g_atomic_int_set (&has_midi_events_, 1);
                }
            }
        }
      break;
    case PortType::Audio:
    case PortType::CV:
      if (noroll)
        {
          dsp_fill (
            &buf_[time_nfo.local_offset],
            DENORMAL_PREVENTION_VAL (AUDIO_ENGINE), time_nfo.nframes);
          break;
        }

      g_return_if_fail (
        owner_type != PortIdentifier::OwnerType::TRACK_PROCESSOR
        || IS_TRACK_AND_NONNULL (track));

      /* only consider incoming external data if
       * armed for recording (if the port is owner
       * by a track), otherwise always consider
       * incoming external data */
      if ((owner_type != PortIdentifier::OwnerType::TRACK_PROCESSOR || (owner_type == PortIdentifier::OwnerType::TRACK_PROCESSOR && track_type_can_record (track->type) && track_get_recording (track))) && id.flow_ == PortFlow::Input)
        {
          switch (AUDIO_ENGINE->audio_backend)
            {
#ifdef HAVE_JACK
            case AudioBackend::AUDIO_BACKEND_JACK:
              sum_data_from_jack (this, time_nfo.local_offset, time_nfo.nframes);
              break;
#endif
            case AudioBackend::AUDIO_BACKEND_DUMMY:
              sum_data_from_dummy (time_nfo.local_offset, time_nfo.nframes);
              break;
            default:
              break;
            }
        }

      for (size_t k = 0; k < srcs_.size (); k++)
        {
          Port *                 src_port = srcs_[k];
          const PortConnection * conn = src_connections_[k];
          if (!conn->enabled)
            continue;

          float minf = 0.f, maxf = 0.f, depth_range, multiplier;
          if (G_LIKELY (id.type_ == PortType::Audio))
            {
              minf = -2.f;
              maxf = 2.f;
              depth_range = 1.f;
              multiplier = conn->multiplier;
            }
          else if (id.type_ == PortType::CV)
            {
              maxf = maxf_;
              minf = minf_;

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
                &buf_[time_nfo.local_offset],
                &src_port->buf_[time_nfo.local_offset], time_nfo.nframes);
            }
          else
            {
              dsp_mix2 (
                &buf_[time_nfo.local_offset],
                &src_port->buf_[time_nfo.local_offset], 1.f, multiplier,
                time_nfo.nframes);
            }

          if (
            G_UNLIKELY (id.type_ == PortType::CV)
            || owner_type == PortIdentifier::OwnerType::FADER)
            {
              float abs_peak =
                dsp_abs_max (&buf_[time_nfo.local_offset], time_nfo.nframes);
              if (abs_peak > maxf)
                {
                  /* this limiting wastes around 50% of port processing so only
                   * do it on CV connections and faders if they exceed maxf */
                  dsp_limit1 (
                    &buf_[time_nfo.local_offset], minf, maxf, time_nfo.nframes);
                }
            }
        } /* foreach source */

      if (id.flow_ == PortFlow::Output)
        {
          switch (AUDIO_ENGINE->audio_backend)
            {
#ifdef HAVE_JACK
            case AudioBackend::AUDIO_BACKEND_JACK:
              send_data_to_jack (this, time_nfo.local_offset, time_nfo.nframes);
              break;
#endif
            default:
              break;
            }
        }

      if (time_nfo.local_offset + time_nfo.nframes == AUDIO_ENGINE->block_length)
        {
          size_t size = sizeof (float) * (size_t) AUDIO_ENGINE->block_length;
          size_t write_space_avail = zix_ring_write_space (audio_ring_);

          /* move the read head 8 blocks to make space if no space avail to
           * write */
          if (write_space_avail / size < 1)
            {
              zix_ring_skip (audio_ring_, size * 8);
            }

          zix_ring_write (audio_ring_, &buf_[0], size);
        }

      /* if track output (to be shown on mixer) */
      if (
        owner_type == PortIdentifier::OwnerType::CHANNEL && is_stereo_port
        && id.flow_ == PortFlow::Output)
        {
          g_return_if_fail (IS_TRACK_AND_NONNULL (track));
          Channel * ch = track->channel;
          g_return_if_fail (ch);

          /* calculate meter values */
          if (
            this == &ch->stereo_out->get_l ()
            || this == &ch->stereo_out->get_r ())
            {
              /* reset peak if needed */
              gint64 time_now = g_get_monotonic_time ();
              if (time_now - peak_timestamp_ > TIME_TO_RESET_PEAK)
                peak_ = -1.f;

              bool changed = dsp_abs_max_with_existing_peak (
                &buf_[time_nfo.local_offset], &peak_, time_nfo.nframes);
              if (changed)
                {
                  peak_timestamp_ = g_get_monotonic_time ();
                }
            }
        }

      /* if bouncing tracks directly to master (e.g., when bouncing the track on
       * its own without parents), clear master input */
      if (G_UNLIKELY (AUDIO_ENGINE->bounce_mode > BounceMode::BOUNCE_OFF && !AUDIO_ENGINE->bounce_with_parents && (this == &P_MASTER_TRACK->processor->stereo_in->get_l () || this == &P_MASTER_TRACK->processor->stereo_in->get_r ())))
        {
          dsp_fill (
            &buf_[time_nfo.local_offset], AUDIO_ENGINE->denormal_prevention_val,
            time_nfo.nframes);
        }

      /* if bouncing track directly to master (e.g., when bouncing the track on
       * its own without parents), add the buffer to master output */
      if (G_UNLIKELY (AUDIO_ENGINE->bounce_mode > BounceMode::BOUNCE_OFF && (owner_type == PortIdentifier::OwnerType::CHANNEL || owner_type == PortIdentifier::OwnerType::TRACK_PROCESSOR || (owner_type == PortIdentifier::OwnerType::FADER && ENUM_BITSET_TEST (PortIdentifier::Flags2, id.flags2_, PortIdentifier::Flags2::PREFADER)) || (owner_type == PortIdentifier::OwnerType::PLUGIN && id.plugin_id_.slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT)) && is_stereo_port && id.flow_ == PortFlow::Output && track && track->bounce_to_master))
        {

#define _ADD(l_or_r) \
  dsp_add2 ( \
    &P_MASTER_TRACK->channel->stereo_out->l_or_r.buf_[time_nfo.local_offset], \
    &buf_[time_nfo.local_offset], time_nfo.nframes)

          Channel *        ch;
          Fader *          prefader;
          TrackProcessor * tp;
          switch (AUDIO_ENGINE->bounce_step)
            {
            case BounceStep::BOUNCE_STEP_BEFORE_INSERTS:
              tp = track->processor;
              g_return_if_fail (tp);
              if (track->type == TrackType::TRACK_TYPE_INSTRUMENT)
                {
                  if (this == track->channel->instrument->l_out)
                    {
                      _ADD (get_l ());
                    }
                  if (this == track->channel->instrument->r_out)
                    {
                      _ADD (get_r ());
                    }
                }
              else if (tp->stereo_out && track->bounce)
                {
                  if (this == &tp->stereo_out->get_l ())
                    {
                      _ADD (get_l ());
                    }
                  else if (this == &tp->stereo_out->get_r ())
                    {
                      _ADD (get_r ());
                    }
                }
              break;
            case BounceStep::BOUNCE_STEP_PRE_FADER:
              ch = track->channel;
              g_return_if_fail (ch);
              prefader = ch->prefader;
              if (this == &prefader->stereo_out->get_l ())
                {
                  _ADD (get_l ());
                }
              else if (this == &prefader->stereo_out->get_r ())
                {
                  _ADD (get_r ());
                }
              break;
            case BounceStep::BOUNCE_STEP_POST_FADER:
              ch = track->channel;
              g_return_if_fail (ch);
              if (track->type != TrackType::TRACK_TYPE_MASTER)
                {
                  if (this == &ch->stereo_out->get_l ())
                    {
                      _ADD (get_l ());
                    }
                  else if (this == &ch->stereo_out->get_r ())
                    {
                      _ADD (get_r ());
                    }
                }
              break;
            }
#undef _ADD
        }
      break;
    case PortType::Control:
      {
        if (
          id.flow_ != PortFlow::Input
          || (owner_type == PortIdentifier::OwnerType::FADER && (ENUM_BITSET_TEST (PortIdentifier::Flags2, id.flags2_, PortIdentifier::Flags2::MonitorFader) || ENUM_BITSET_TEST (PortIdentifier::Flags2, id.flags2_, PortIdentifier::Flags2::PREFADER)))
          || ENUM_BITSET_TEST (
            PortIdentifier::Flags, id.flags_, PortIdentifier::Flags::TP_MONO)
          || ENUM_BITSET_TEST (
            PortIdentifier::Flags, id.flags_,
            PortIdentifier::Flags::TP_INPUT_GAIN)
          || !(ENUM_BITSET_TEST (
            PortIdentifier::Flags, id.flags_,
            PortIdentifier::Flags::AUTOMATABLE)))
          {
            break;
          }

        /* calculate value from automation track */
        g_return_if_fail (ENUM_BITSET_TEST (
          PortIdentifier::Flags, id.flags_, PortIdentifier::Flags::AUTOMATABLE));
        AutomationTrack * at = at_;
        if (G_UNLIKELY (!at))
          {
            g_critical (
              "No automation track found for port "
              "%s",
              id.get_label ().c_str ());
          }
        if (ZRYTHM_TESTING && at)
          {
            AutomationTrack * found_at =
              automation_track_find_from_port (this, NULL, true);
            g_return_if_fail (at == found_at);
          }

        if (
          at
          && ENUM_BITSET_TEST (
            PortIdentifier::Flags, id.flags_, PortIdentifier::Flags::AUTOMATABLE)
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
                control_port_set_val_from_normalized (this, val, true);
                value_changed_from_reading_ = true;
              }
          }

        float maxf, minf, depth_range, val_to_use;
        /* whether this is the first CV processed on this control port */
        bool first_cv = true;
        for (int k = 0; k < srcs_.size (); k++)
          {
            const PortConnection * conn = src_connections_[k];
            if (G_UNLIKELY (!conn->enabled))
              continue;

            Port * src_port = srcs_[k];
            if (src_port->id_.type_ == PortType::CV)
              {
                maxf = maxf_;
                minf = minf_;

                depth_range = (maxf - minf) / 2.f;

                /* figure out whether to use base value or the current value */
                if (first_cv)
                  {
                    val_to_use = base_value_;
                    first_cv = false;
                  }
                else
                  {
                    val_to_use = control_;
                  }

                float result = CLAMP (
                  val_to_use + depth_range * src_port->buf_[0] * conn->multiplier,
                  minf, maxf);
                control_ = result;
                port_forward_control_change_event (this);
              }
          }
      }
      break;
    default:
      break;
    }
}

void
Port::disconnect_hw_inputs ()
{
  GPtrArray * srcs = g_ptr_array_new ();
  int         num_srcs = port_connections_manager_get_sources_or_dests (
    PORT_CONNECTIONS_MGR, srcs, &this->id_, true);
  for (int i = 0; i < num_srcs; i++)
    {
      PortConnection * conn = (PortConnection *) g_ptr_array_index (srcs, i);
      if (conn->src_id->owner_type_ == PortIdentifier::OwnerType::HW)
        port_connections_manager_ensure_disconnect (
          PORT_CONNECTIONS_MGR, conn->src_id, &this->id_);
    }
  g_ptr_array_unref (srcs);
}

void
Port::set_expose_to_backend (bool expose)
{
  g_return_if_fail (AUDIO_ENGINE);

  if (!AUDIO_ENGINE->setup)
    {
      g_message (
        "audio engine not set up, skipping expose to backend for %s",
        this->id_.sym_.c_str ());
      return;
    }

  if (this->id_.type_ == PortType::Audio)
    {
      switch (AUDIO_ENGINE->audio_backend)
        {
        case AudioBackend::AUDIO_BACKEND_DUMMY:
          g_message ("called %s with dummy audio backend", __func__);
          return;
#ifdef HAVE_JACK
        case AudioBackend::AUDIO_BACKEND_JACK:
          expose_to_jack (this, expose);
          break;
#endif
#ifdef HAVE_RTAUDIO
        case AudioBackend::AUDIO_BACKEND_ALSA_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_JACK_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_PULSEAUDIO_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_COREAUDIO_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_WASAPI_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_ASIO_RTAUDIO:
          expose_to_rtaudio (this, expose);
          break;
#endif
        default:
          break;
        }
    }
  else if (this->id_.type_ == PortType::Event)
    {
      switch (AUDIO_ENGINE->midi_backend)
        {
        case MidiBackend::MIDI_BACKEND_DUMMY:
          g_message ("called %s with MIDI dummy backend", __func__);
          return;
#ifdef HAVE_JACK
        case MidiBackend::MIDI_BACKEND_JACK:
          expose_to_jack (this, expose);
          break;
#endif
#ifdef HAVE_ALSA
        case MidiBackend::MIDI_BACKEND_ALSA:
#  if 0
          expose_to_alsa (this, expose);
#  endif
          break;
#endif
#ifdef HAVE_RTMIDI
        case MidiBackend::MIDI_BACKEND_ALSA_RTMIDI:
        case MidiBackend::MIDI_BACKEND_JACK_RTMIDI:
        case MidiBackend::MIDI_BACKEND_WINDOWS_MME_RTMIDI:
        case MidiBackend::MIDI_BACKEND_COREMIDI_RTMIDI:
#  ifdef HAVE_RTMIDI_6
        case MidiBackend::MIDI_BACKEND_WINDOWS_UWP_RTMIDI:
#  endif
          expose_to_rtmidi (this, expose);
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

void
Port::rename_backend ()
{
  if ((!this->is_exposed_to_backend ()))
    return;

  switch (this->internal_type_)
    {
#ifdef HAVE_JACK
    case Port::InternalType::JackPort:
      {
        char str[600];
        this->get_full_designation (str);
        jack_port_rename (
          AUDIO_ENGINE->client, (jack_port_t *) this->data_, str);
      }
      break;
#endif
    case Port::InternalType::AlsaSequencerPort:
      break;
    default:
      break;
    }
}

bool
Port::has_sound () const
{
  switch (this->id_.type_)
    {
    case PortType::Audio:
      g_return_val_if_fail (
        this->buf_.size () >= AUDIO_ENGINE->block_length, false);
      for (nframes_t i = 0; i < AUDIO_ENGINE->block_length; i++)
        {
          if (fabsf (this->buf_[i]) > 0.0000001f)
            {
              return true;
            }
        }
      break;
    case PortType::Event:
      /* TODO */
      break;
    default:
      break;
    }

  return false;
}

void
Port::get_full_designation (char * buf) const
{
  const PortIdentifier * id = &this->id_;

  switch (id->owner_type_)
    {
    case PortIdentifier::OwnerType::PORT_OWNER_TYPE_AUDIO_ENGINE:
      strcpy (buf, id->label_.c_str ());
      return;
    case PortIdentifier::OwnerType::PLUGIN:
      {
        Plugin * pl = this->get_plugin (1);
        g_return_if_fail (pl);
        Track * track = plugin_get_track (pl);
        g_return_if_fail (track);
        sprintf (
          buf, "%s/%s/%s", track->name, pl->setting->descr->name,
          id->get_label_as_c_str ());
      }
      return;
    case PortIdentifier::OwnerType::CHANNEL:
    case PortIdentifier::OwnerType::TRACK:
    case PortIdentifier::OwnerType::TRACK_PROCESSOR:
    case PortIdentifier::OwnerType::CHANNEL_SEND:
      {
        Track * tr = this->get_track (1);
        g_return_if_fail (IS_TRACK_AND_NONNULL (tr));
        sprintf (buf, "%s/%s", tr->name, id->get_label_as_c_str ());
      }
      return;
    case PortIdentifier::OwnerType::FADER:
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id->flags2_, PortIdentifier::Flags2::PREFADER)
        || ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id->flags2_, PortIdentifier::Flags2::POSTFADER))
        {
          Track * tr = this->get_track (1);
          g_return_if_fail (IS_TRACK_AND_NONNULL (tr));
          sprintf (buf, "%s/%s", tr->name, id->get_label_as_c_str ());
          return;
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id->flags2_,
          PortIdentifier::Flags2::MonitorFader))
        {
          sprintf (buf, "Engine/%s", id->get_label_as_c_str ());
          return;
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id->flags2_,
          PortIdentifier::Flags2::SAMPLE_PROCESSOR_FADER))
        {
          strcpy (buf, id->get_label_as_c_str ());
          return;
        }
      break;
    case PortIdentifier::OwnerType::HW:
      sprintf (buf, "HW/%s", id->get_label_as_c_str ());
      return;
    case PortIdentifier::OwnerType::PORT_OWNER_TYPE_TRANSPORT:
      sprintf (buf, "Transport/%s", id->get_label_as_c_str ());
      return;
    case PortIdentifier::OwnerType::MODULATOR_MACRO_PROCESSOR:
      sprintf (buf, "Modulator Macro Processor/%s", id->get_label_as_c_str ());
      return;
    default:
      break;
    }

  g_return_if_reached ();
}

void
Port::print_full_designation () const
{
  char buf[1200];
  this->get_full_designation (buf);
  g_message ("%s", buf);
}

void
Port::clear_external_buffer ()
{
  if (!this->is_exposed_to_backend ())
    {
      return;
    }

#ifdef HAVE_JACK
  if (this->internal_type_ != Port::InternalType::JackPort)
    {
      return;
    }

  jack_port_t * jport = JACK_PORT_T (this->data_);
  g_return_if_fail (jport);
  void * buf = jack_port_get_buffer (jport, AUDIO_ENGINE->block_length);
  g_return_if_fail (buf);
  if (
    this->id_.type_ == PortType::Audio
    && AUDIO_ENGINE->audio_backend == AudioBackend::AUDIO_BACKEND_JACK)
    {
      float * fbuf = (float *) buf;
      dsp_fill (
        &fbuf[0], DENORMAL_PREVENTION_VAL (AUDIO_ENGINE),
        AUDIO_ENGINE->block_length);
    }
  else if (
    this->id_.type_ == PortType::Event
    && AUDIO_ENGINE->midi_backend == MidiBackend::MIDI_BACKEND_JACK)
    {
      jack_midi_clear_buffer (buf);
    }
#endif
}

Track *
Port::get_track (bool warn_if_fail) const
{
  g_return_val_if_fail (IS_PORT (this), NULL);

  /* return the pointer if dsp thread */
  if (
    G_LIKELY (
      this->track_ && PROJECT && PROJECT->loaded && ROUTER
      && router_is_processing_thread (ROUTER)))
    {
      g_return_val_if_fail (IS_TRACK (this->track_), NULL);
      return this->track_;
    }

  Track * track = NULL;
  if (this->id_.track_name_hash_ != 0)
    {
      g_return_val_if_fail (gZrythm && TRACKLIST, NULL);

      track = tracklist_find_track_by_name_hash (
        TRACKLIST, this->id_.track_name_hash_);
      if (!track)
        track = tracklist_find_track_by_name_hash (
          SAMPLE_PROCESSOR->tracklist, this->id_.track_name_hash_);
    }

  if (!track && warn_if_fail)
    {
      g_warning (
        "track with name hash %d not found for "
        "port %s",
        this->id_.track_name_hash_, this->id_.get_label_as_c_str ());
    }

  return track;
}

Plugin *
Port::get_plugin (bool warn_if_fail) const
{
  g_return_val_if_fail (IS_PORT (this), NULL);

  /* if DSP thread, return the pointer */
  if (
    G_LIKELY (
      this->plugin_ && PROJECT && PROJECT->loaded && ROUTER
      && router_is_processing_thread (ROUTER)))
    {
      g_return_val_if_fail (IS_PLUGIN (this->plugin_), NULL);
      return this->plugin_;
    }

  if (this->id_.owner_type_ != PortIdentifier::OwnerType::PLUGIN)
    {
      if (warn_if_fail)
        g_warning ("port not owned by plugin");
      return NULL;
    }

  Track * track = this->get_track (0);
  if (!track && this->tmp_plugin_)
    {
      return this->tmp_plugin_;
    }
  if (
    !track
    || (track->type != TrackType::TRACK_TYPE_MODULATOR && !track->channel))
    {
      if (warn_if_fail)
        {
          g_warning ("No track found for port");
        }
      return NULL;
    }

  Plugin *                       pl = NULL;
  const PluginIdentifier * const pl_id = &this->id_.plugin_id_;
  switch (pl_id->slot_type)
    {
    case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
      pl = track->channel->midi_fx[pl_id->slot];
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
      pl = track->channel->instrument;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
      pl = track->channel->inserts[pl_id->slot];
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_MODULATOR:
      pl = track->modulators[pl_id->slot];
      break;
    default:
      g_return_val_if_reached (NULL);
      break;
    }
  if (!pl && this->tmp_plugin_)
    return this->tmp_plugin_;

  if (!pl && warn_if_fail)
    {
      g_critical (
        "plugin at slot type %s (slot %d) not found for port %s",
        ENUM_NAME (pl_id->slot_type), pl_id->slot,
        this->id_.get_label_as_c_str ());
      return NULL;
    }

  /* unset \ref Port.tmp_plugin if a Plugin was found */
  /* FIXME this should be const - this should not be done here */
  this->tmp_plugin_ = NULL;

  return pl;
}

void
Port::apply_pan (
  float           pan,
  PanLaw          pan_law,
  PanAlgorithm    pan_algo,
  nframes_t       start_frame,
  const nframes_t nframes)
{
  float calc_r, calc_l;
  pan_get_calc_lr (pan_law, pan_algo, pan, &calc_l, &calc_r);

  /* if stereo R */
  if (ENUM_BITSET_TEST (
        PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::STEREO_R))
    {
      dsp_mul_k2 (&buf_[start_frame], calc_r, nframes);
    }
  /* else if stereo L */
  else
    {
      dsp_mul_k2 (&buf_[start_frame], calc_l, nframes);
    }
}

uint32_t
Port::get_hash () const
{
  return hash_get_for_struct (this, sizeof (Port));
}

uint32_t
Port::get_hash (const void * ptr)
{
  const Port * self = static_cast<const Port *> (ptr);
  return self->get_hash ();
}

void
Port::apply_fader (float amp, nframes_t start_frame, const nframes_t nframes)
{
  dsp_mul_k2 (&buf_[start_frame], amp, nframes);
}

bool
Port::is_connected_to (const Port * dest) const
{
  return (
    port_connections_manager_find_connection (
      PORT_CONNECTIONS_MGR, &this->id_, &dest->id_)
    != nullptr);
}

bool
Port::is_in_active_project () const
{
  switch (id_.owner_type_)
    {
    case PortIdentifier::OwnerType::PORT_OWNER_TYPE_AUDIO_ENGINE:
      return engine_ != nullptr && engine_is_in_active_project (engine_);
    case PortIdentifier::OwnerType::PLUGIN:
      return plugin_ != nullptr && plugin_is_in_active_project (plugin_);
    case PortIdentifier::OwnerType::TRACK:
    case PortIdentifier::OwnerType::CHANNEL:
    case PortIdentifier::OwnerType::TRACK_PROCESSOR:
      return track_ != nullptr && track_is_in_active_project (track_);
    case PortIdentifier::OwnerType::FADER:
      return fader_ != nullptr && fader_is_in_active_project (fader_);
    case PortIdentifier::OwnerType::CHANNEL_SEND:
      return channel_send_ != nullptr
             && channel_send_is_in_active_project (channel_send_);
    case PortIdentifier::OwnerType::MODULATOR_MACRO_PROCESSOR:
      return modulator_macro_processor_ != nullptr
             && modulator_macro_processor_is_in_active_project (
               modulator_macro_processor_);
    case PortIdentifier::OwnerType::HW:
      return ext_port_ != nullptr && ext_port_is_in_active_project (ext_port_);
    default:
      g_return_val_if_reached (false);
    }
}

void
Port::connect_to (Port &dest, bool locked)
{
  port_connections_manager_ensure_connect (
    PORT_CONNECTIONS_MGR, &id_, &dest.id_, 1.f, locked, true);
}

void
Port::disconnect_from (Port &dest)
{
  port_connections_manager_ensure_disconnect (
    PORT_CONNECTIONS_MGR, &id_, &dest.id_);
}

Port::~Port ()
{
  this->free_bufs ();

#ifdef HAVE_RTMIDI
  for (auto &dev : this->rtmidi_ins_)
    {
      rtmidi_device_close (dev, 1);
    }
#endif
}
