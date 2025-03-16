// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "dsp/port_identifier.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/cv_port.h"
#include "gui/dsp/midi_port.h"
#include "gui/dsp/port.h"
#include "gui/dsp/rtmidi_device.h"
#include "utils/dsp.h"
#include "utils/hash.h"
#include "utils/rt_thread_id.h"

#include <fmt/format.h>

Port::Port () : id_ (std::make_unique<PortIdentifier> ()) { }

Port::Port (
  std::string label,
  PortType    type,
  PortFlow    flow,
  float       minf,
  float       maxf,
  float       zerof)
    : Port ()
{
  range_.minf_ = minf;
  range_.maxf_ = maxf;
  range_.zerof_ = zerof;
  id_->label_ = std::move (label);
  id_->type_ = type;
  id_->flow_ = flow;
}

std::unique_ptr<Port>
Port::create_unique_from_type (PortType type)
{
  switch (type)
    {
    case PortType::Audio:
      return std::make_unique<AudioPort> ();
    case PortType::Event:
      return std::make_unique<MidiPort> ();
    case PortType::Control:
      return std::make_unique<ControlPort> ();
    case PortType::CV:
      return std::make_unique<CVPort> ();
    default:
      z_return_val_if_reached (nullptr);
    }
}

bool
Port::is_exposed_to_backend () const
{
  return (is_audio () || is_midi ())
         && ((backend_ && backend_->is_exposed ()) || id_->owner_type_ == PortIdentifier::OwnerType::AudioEngine || exposed_to_backend_);
}

bool
Port::needs_external_buffer_clear_on_early_return () const
{
  return id_->flow_ == dsp::PortFlow::Output && is_exposed_to_backend ();
}

void
Port::process_block (EngineProcessTimeInfo time_nfo)
{
  // FIXME: avoid this mess and calling another process function
  std::visit (
    [&] (auto &&port) {
      using PortT = base_type<decltype (port)>;
      if constexpr (std::is_same_v<PortT, MidiPort>)
        {
          if (ENUM_BITSET_TEST (
                port->id_->flags_, dsp::PortIdentifier::Flags::ManualPress))
            {
              port->midi_events_.dequeue (
                time_nfo.local_offset_, time_nfo.nframes_);
              return;
            }
        }
      if constexpr (std::is_same_v<PortT, AudioPort>)
        {
          if (
            port->id_->is_monitor_fader_stereo_in_or_out_port ()
            && AUDIO_ENGINE->exporting_)
            {
              /* if exporting and the port is not a project port, skip
               * processing */
              return;
            }
        }

      port->process (time_nfo, false);
    },
    convert_to_variant<PortPtrVariant> (this));
}

int
Port::get_num_unlocked (bool sources) const
{
  z_return_val_if_fail (is_in_active_project (), 0);
  return PORT_CONNECTIONS_MGR->get_unlocked_sources_or_dests (
    nullptr, get_uuid (), sources);
}

int
Port::get_num_unlocked_dests () const
{
  return get_num_unlocked (false);
}

int
Port::get_num_unlocked_srcs () const
{
  return get_num_unlocked (true);
}

void
Port::set_owner (IPortOwner &owner)
{
  owner_ = std::addressof (owner);
  owner_->set_port_metadata_from_owner (*id_, range_);
}

std::string
Port::get_label () const
{
  return id_->get_label ();
}

void
Port::disconnect_all ()
{
  srcs_.clear ();
  dests_.clear ();

  if (!gZrythm || !PROJECT || !PORT_CONNECTIONS_MGR)
    return;

  if (!is_in_active_project ())
    {
#if 0
      z_debug ("{} ({}) is not a project port, skipping", this->id_.label, fmt::ptr(this));
#endif
      return;
    }

  PortConnectionsManager::ConnectionsVector srcs;
  PORT_CONNECTIONS_MGR->get_sources_or_dests (&srcs, get_uuid (), true);
  for (const auto &conn : srcs)
    {
      PORT_CONNECTIONS_MGR->ensure_disconnect (conn->src_id_, conn->dest_id_);
    }

  PortConnectionsManager::ConnectionsVector dests;
  PORT_CONNECTIONS_MGR->get_sources_or_dests (&dests, get_uuid (), false);
  for (const auto &conn : dests)
    {
      PORT_CONNECTIONS_MGR->ensure_disconnect (conn->src_id_, conn->dest_id_);
    }

  if (backend_)
    {
      backend_->unexpose ();
    }
}

void
Port::change_track (IPortOwner::TrackUuid new_track_id)
{
  id_->set_track_id (new_track_id);
}

void
Port::copy_members_from (const Port &other, ObjectCloneType clone_type)
{
  id_ = other.id_->clone_unique ();
  exposed_to_backend_ = other.exposed_to_backend_;
  range_ = other.range_;
}

void
Port::set_expose_to_backend (AudioEngine &engine, bool expose)
{
  if (!engine.setup_)
    {
      z_warning (
        "audio engine not set up, skipping expose to backend for {}", id_->sym_);
      return;
    }

  if (this->id_->type_ == PortType::Audio)
    {
      auto * audio_port = dynamic_cast<AudioPort *> (this);
      (void) audio_port;
      switch (engine.audio_backend_)
        {
        case AudioBackend::AUDIO_BACKEND_DUMMY:
          z_debug ("called with dummy audio backend");
          return;
#if HAVE_JACK
        case AudioBackend::AUDIO_BACKEND_JACK:
          if (
            !backend_
            || (dynamic_cast<JackPortBackend *> (backend_.get ()) == nullptr))
            {
              backend_ = std::make_unique<JackPortBackend> (*engine.client_);
            }
          break;
#endif
#if HAVE_RTAUDIO
        case AudioBackend::AUDIO_BACKEND_ALSA_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_JACK_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_PULSEAUDIO_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_COREAUDIO_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_WASAPI_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_ASIO_RTAUDIO:
          if (
            !backend_
            || (dynamic_cast<RtAudioPortBackend *> (backend_.get ()) == nullptr))
            {
              bool is_stereo_l =
                ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::StereoL);
              bool is_stereo_r =
                ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::StereoR);
              if (!is_stereo_l && !is_stereo_r)
                {
                  return;
                }

              // FIXME: get_track() unimplemented
              // this is bad design anyway
              auto track = dynamic_cast<ChannelTrack *> (get_track (false));
              if (!track)
                return;

              auto &ch = track->channel_;

              backend_ = std::make_unique<RtAudioPortBackend> (
                [ch, is_stereo_l] (
                  std::vector<RtAudioPortBackend::RtAudioPortInfo> &nfo) {
                  for (
                    const auto &ext_port :
                    is_stereo_l ? ch->ext_stereo_l_ins_ : ch->ext_stereo_r_ins_)
                    {
                      nfo.emplace_back (
                        ext_port->rtaudio_is_input_, ext_port->rtaudio_id_,
                        ext_port->rtaudio_channel_idx_);
                    }
                },
                [ch, is_stereo_l] () {
                  return is_stereo_l ? ch->all_stereo_l_ins_ : ch->all_stereo_r_ins_;
                });
            }
          break;
#endif
        default:
          break;
        }
    }
  else if (id_->type_ == PortType::Event)
    {
      auto * midi_port = dynamic_cast<MidiPort *> (this);
      (void) midi_port;
      switch (engine.midi_backend_)
        {
        case MidiBackend::MIDI_BACKEND_DUMMY:
          z_debug ("called with MIDI dummy backend");
          return;
#if HAVE_JACK
        case MidiBackend::MIDI_BACKEND_JACK:
          if (
            !backend_
            || (dynamic_cast<JackPortBackend *> (backend_.get ()) == nullptr))
            {
              backend_ = std::make_unique<JackPortBackend> (*engine.client_);
            }
          break;
#endif
#if HAVE_RTMIDI
        case MidiBackend::MIDI_BACKEND_ALSA_RTMIDI:
        case MidiBackend::MIDI_BACKEND_JACK_RTMIDI:
        case MidiBackend::MIDI_BACKEND_WINDOWS_MME_RTMIDI:
        case MidiBackend::MIDI_BACKEND_COREMIDI_RTMIDI:
#  if HAVE_RTMIDI_6
        case MidiBackend::MIDI_BACKEND_WINDOWS_UWP_RTMIDI:
#  endif
          if (
            !backend_
            || (dynamic_cast<RtMidiPortBackend *> (backend_.get ()) == nullptr))
            {
              backend_ = std::make_unique<RtMidiPortBackend> ();
            }
          break;
#endif
        default:
          break;
        }
    }
  else /* else if not audio or MIDI */
    {
      z_return_if_reached ();
    }

  if (backend_)
    {
      if (expose)
        {
          backend_->expose (*id_, [this] () {
            return get_full_designation ();
          });
        }
      else
        {
          backend_->unexpose ();
        }
      exposed_to_backend_ = expose;
    }
}

void
Port::rename_backend (AudioEngine &engine)
{
  if (!is_exposed_to_backend ())
    return;

  // just re-expose - this causes a rename if already exposed
  set_expose_to_backend (engine, true);
}

void
Port::print_full_designation () const
{
  z_info ("{}", get_full_designation ());
}

void
Port::clear_external_buffer ()
{
  if (!is_exposed_to_backend () || !backend_)
    {
      return;
    }

  backend_->clear_backend_buffer (id_->type_, AUDIO_ENGINE->block_length_);
}

uint32_t
Port::get_hash () const
{
  return utils::hash::get_object_hash (*this);
}
