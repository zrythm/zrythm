// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "dsp/graph.h"
#include "dsp/port_identifier.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/channel.h"
#include "gui/dsp/carla_native_plugin.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/cv_port.h"
#include "gui/dsp/engine_jack.h"
#include "gui/dsp/hardware_processor.h"
#include "gui/dsp/master_track.h"
#include "gui/dsp/midi_event.h"
#include "gui/dsp/midi_port.h"
#include "gui/dsp/modulator_macro_processor.h"
#include "gui/dsp/modulator_track.h"
#include "gui/dsp/plugin.h"
#include "gui/dsp/port.h"
#include "gui/dsp/recordable_track.h"
#include "gui/dsp/router.h"
#include "gui/dsp/rtaudio_device.h"
#include "gui/dsp/rtmidi_device.h"
#include "gui/dsp/tempo_track.h"
#include "gui/dsp/tracklist.h"
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
  minf_ = minf;
  maxf_ = maxf;
  zerof_ = zerof;
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
  return (backend_ && backend_->is_exposed ())
         || id_->owner_type_ == PortIdentifier::OwnerType::AudioEngine
         || exposed_to_backend_;
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
          if (
            ENUM_BITSET_TEST (
              dsp::PortIdentifier::Flags, port->id_->flags_,
              dsp::PortIdentifier::Flags::ManualPress))
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
    nullptr, *id_, sources);
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

template <typename T>
void
Port::set_owner (T * owner)
{
  auto get_track_name_hash = [] (const auto &track) -> Track::NameHashT {
    return track->name_.empty () ? 0 : track->get_name_hash ();
  };

  if constexpr (std::derived_from<T, gui::old_dsp::plugins::Plugin>)
    {
      id_->plugin_id_ = owner->id_;
      id_->track_name_hash_ = owner->id_.track_name_hash_;
      id_->owner_type_ = PortIdentifier::OwnerType::Plugin;

      if (is_control ())
        {
          auto * control_port = dynamic_cast<ControlPort *> (this);
          if (control_port->at_)
            {
              control_port->at_->set_port_id (*id_);
            }
        }
    }
  else if constexpr (std::derived_from<T, TrackProcessor>)
    {
      auto track = owner->get_track ();
      z_return_if_fail (track);
      id_->track_name_hash_ = get_track_name_hash (track);
      id_->owner_type_ = PortIdentifier::OwnerType::TrackProcessor;
      track_ = track;
    }
  else if constexpr (std::derived_from<T, gui::Channel>)
    {
      auto track = owner->get_track ();
      z_return_if_fail (track);
      id_->track_name_hash_ = get_track_name_hash (track);
      id_->owner_type_ = PortIdentifier::OwnerType::Channel;
      track_ = track;
    }
  else if constexpr (std::derived_from<T, ExtPort>)
    {
      ext_port_ = owner;
      id_->owner_type_ = PortIdentifier::OwnerType::HardwareProcessor;
    }
  else if constexpr (std::derived_from<T, ChannelSend>)
    {
      id_->track_name_hash_ = owner->track_name_hash_;
      id_->port_index_ = owner->slot_;
      id_->owner_type_ = PortIdentifier::OwnerType::ChannelSend;
      channel_send_ = owner;

      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id_->flags2_,
          PortIdentifier::Flags2::ChannelSendEnabled))
        {
          minf_ = 0.f;
          maxf_ = 1.f;
          zerof_ = 0.0f;
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id_->flags2_,
          PortIdentifier::Flags2::ChannelSendAmount))
        {
          minf_ = 0.f;
          maxf_ = 2.f;
          zerof_ = 0.f;
        }
    }
  else if constexpr (std::derived_from<T, Fader>)
    {
      id_->owner_type_ = PortIdentifier::OwnerType::Fader;
      fader_ = owner;
      z_return_if_fail (fader_);

      if (
        owner->type_ == Fader::Type::AudioChannel
        || owner->type_ == Fader::Type::MidiChannel)
        {
          auto track = owner->get_track ();
          z_return_if_fail (track);
          id_->track_name_hash_ = get_track_name_hash (track);
          if (owner->passthrough_)
            {
              id_->flags2_ |= PortIdentifier::Flags2::Prefader;
            }
          else
            {
              id_->flags2_ |= PortIdentifier::Flags2::Postfader;
            }
        }
      else if (owner->type_ == Fader::Type::SampleProcessor)
        {
          id_->flags2_ |= PortIdentifier::Flags2::SampleProcessorFader;
        }
      else
        {
          id_->flags2_ |= PortIdentifier::Flags2::MonitorFader;
        }

      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id_->flags_, PortIdentifier::Flags::Amplitude))
        {
          minf_ = 0.f;
          maxf_ = 2.f;
          zerof_ = 0.f;
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id_->flags_,
          PortIdentifier::Flags::StereoBalance))
        {
          minf_ = 0.f;
          maxf_ = 1.f;
          zerof_ = 0.5f;
        }
    }
  else if constexpr (std::derived_from<T, Transport>)
    {
      transport_ = owner;
      id_->owner_type_ = PortIdentifier::OwnerType::Transport;
    }
  else if constexpr (std::derived_from<T, Track>)
    {
      // z_return_if_fail (!track.name_.empty ());
      id_->track_name_hash_ =
        owner->name_.empty () ? 0 : owner->get_name_hash ();
      id_->owner_type_ = PortIdentifier::OwnerType::Track;
      track_ = owner;
    }
  else if constexpr (std::derived_from<T, ModulatorMacroProcessor>)
    {
      modulator_macro_processor_ = owner;
      id_->owner_type_ = PortIdentifier::OwnerType::ModulatorMacroProcessor;
      z_return_if_fail (owner->get_track ());
      id_->track_name_hash_ = owner->get_track ()->get_name_hash ();
      track_ = owner->get_track ();
    }
  else if constexpr (std::derived_from<T, AudioEngine>)
    {
      engine_ = owner;
      id_->owner_type_ = PortIdentifier::OwnerType::AudioEngine;
    }
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
  PORT_CONNECTIONS_MGR->get_sources_or_dests (&srcs, *id_, true);
  for (const auto &conn : srcs)
    {
      PORT_CONNECTIONS_MGR->ensure_disconnect (*conn->src_id_, *conn->dest_id_);
    }

  PortConnectionsManager::ConnectionsVector dests;
  PORT_CONNECTIONS_MGR->get_sources_or_dests (&dests, *id_, false);
  for (const auto &conn : dests)
    {
      PORT_CONNECTIONS_MGR->ensure_disconnect (*conn->src_id_, *conn->dest_id_);
    }

  if (backend_)
    {
      backend_->unexpose ();
    }
}

void
Port::update_identifier (
  const PortIdentifier &prev_id,
  Track *               track,
  bool                  update_automation_track)
{
  if (this->is_in_active_project ())
    {
      /* update in all sources */
      PortConnectionsManager::ConnectionsVector srcs;
      PORT_CONNECTIONS_MGR->get_sources_or_dests (&srcs, prev_id, true);
      for (const auto &conn : srcs)
        {
          if (conn->dest_id_ != id_)
            {
              auto new_conn = conn->clone_unique ();
              new_conn->dest_id_ = id_->clone_unique ();
              PORT_CONNECTIONS_MGR->replace_connection (conn, *new_conn);
            }
        }

      /* update in all dests */
      PortConnectionsManager::ConnectionsVector dests;
      PORT_CONNECTIONS_MGR->get_sources_or_dests (&dests, prev_id, false);
      for (const auto &conn : dests)
        {
          if (conn->src_id_ != id_)
            {
              auto new_conn = conn->clone_unique ();
              new_conn->src_id_ = id_->clone_unique ();
              PORT_CONNECTIONS_MGR->replace_connection (conn, *new_conn);
            }
        }

      if (
        update_automation_track && (id_->track_name_hash_ != 0)
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags, this->id_->flags_,
          PortIdentifier::Flags::Automatable))
        {
          auto * control_port = dynamic_cast<ControlPort *> (this);
          /* update automation track's port id */
          auto * automatable_track = dynamic_cast<AutomatableTrack *> (track);
          control_port->at_ = AutomationTrack::find_from_port (
            *control_port, automatable_track, true);
          z_return_if_fail (control_port->at_);
          control_port->at_->set_port_id (*id_);
        }
    }
}

void
Port::update_track_name_hash (Track &track, unsigned int new_hash)
{
  auto copy = id_->clone_unique ();

  this->id_->track_name_hash_ = new_hash;
  if (this->id_->owner_type_ == PortIdentifier::OwnerType::Plugin)
    {
      this->id_->plugin_id_.track_name_hash_ = new_hash;
    }
  update_identifier (*copy, &track, true);
}

void
Port::copy_members_from (const Port &other)
{
  id_ = other.id_->clone_unique ();
  exposed_to_backend_ = other.exposed_to_backend_;
  minf_ = other.minf_;
  maxf_ = other.maxf_;
  zerof_ = other.zerof_;
}

void
Port::disconnect_hw_inputs ()
{
  PortConnectionsManager::ConnectionsVector srcs;
  PORT_CONNECTIONS_MGR->get_sources_or_dests (&srcs, *id_, true);
  for (const auto &conn : srcs)
    {
      if (
        conn->src_id_->owner_type_
        == PortIdentifier::OwnerType::HardwareProcessor)
        PORT_CONNECTIONS_MGR->ensure_disconnect (*conn->src_id_, *id_);
    }
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
              auto track = dynamic_cast<ChannelTrack *> (get_track (false));
              if (!track)
                return;

              auto &ch = track->channel_;
              bool  is_stereo_l = ENUM_BITSET_TEST (
                PortIdentifier::Flags, id_->flags_,
                PortIdentifier::Flags::StereoL);
              bool is_stereo_r = ENUM_BITSET_TEST (
                PortIdentifier::Flags, id_->flags_,
                PortIdentifier::Flags::StereoR);
              if (!is_stereo_l && !is_stereo_r)
                {
                  return;
                }

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
Port::rename_backend ()
{
  if (!is_exposed_to_backend ())
    return;

  // just re-expose - this causes a rename if already exposed
  set_expose_to_backend (*AUDIO_ENGINE, true);
}

std::string
Port::get_full_designation () const
{
  const auto &id = this->id_;

  switch (id->owner_type_)
    {
    case PortIdentifier::OwnerType::AudioEngine:
      return id->label_;
    case PortIdentifier::OwnerType::Plugin:
      {
        auto * pl = this->get_plugin (true);
        z_return_val_if_fail (pl, {});
        auto * track = pl->get_track ();
        z_return_val_if_fail (track, {});
        return fmt::format (
          "{}/{}/{}", track->get_name (), pl->get_name (), id->label_);
      }
    case PortIdentifier::OwnerType::Channel:
    case PortIdentifier::OwnerType::Track:
    case PortIdentifier::OwnerType::TrackProcessor:
    case PortIdentifier::OwnerType::ChannelSend:
      {
        auto tr = this->get_track (true);
        z_return_val_if_fail (tr, {});
        return fmt::format ("{}/{}", tr->get_name (), id->label_);
      }
    case PortIdentifier::OwnerType::Fader:
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id->flags2_, PortIdentifier::Flags2::Prefader)
        || ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id->flags2_, PortIdentifier::Flags2::Postfader))
        {
          auto * tr = this->get_track (true);
          z_return_val_if_fail (tr, {});
          return fmt::format ("{}/{}", tr->get_name (), id->label_);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id->flags2_,
          PortIdentifier::Flags2::MonitorFader))
        {
          return fmt::format ("Engine/{}", id->label_);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id->flags2_,
          PortIdentifier::Flags2::SampleProcessorFader))
        {
          return id->label_;
        }
      break;
    case PortIdentifier::OwnerType::HardwareProcessor:
      return fmt::format ("HW/{}", id->label_);
    case PortIdentifier::OwnerType::Transport:
      return fmt::format ("Transport/{}", id->label_);
    case PortIdentifier::OwnerType::ModulatorMacroProcessor:
      return fmt::format ("Modulator Macro Processor/{}", id->label_);
    default:
      break;
    }

  z_return_val_if_reached ({});
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

Track *
Port::get_track (bool warn_if_fail) const
{
  z_return_val_if_fail (IS_PORT (this), nullptr);

  /* return the pointer if dsp thread */
  if (
    (track_ != nullptr) && PROJECT && PROJECT->loaded_ && ROUTER
    && ROUTER->is_processing_thread ()) [[likely]]
    {
      z_return_val_if_fail (track_, nullptr);
      return track_;
    }

  Track * track = nullptr;
  if (id_->track_name_hash_ != 0)
    {
      z_return_val_if_fail (gZrythm && TRACKLIST, nullptr);

      auto track_var =
        TRACKLIST->find_track_by_name_hash (this->id_->track_name_hash_);
      if (!track_var)
        track_var = SAMPLE_PROCESSOR->tracklist_->find_track_by_name_hash (
          id_->track_name_hash_);
      z_return_val_if_fail (track_var, nullptr);
      std::visit ([&] (auto &&track_ptr) { track = track_ptr; }, *track_var);
    }

  if ((track == nullptr) && warn_if_fail)
    {
      z_warning (
        "track with name hash %d not found for port %s", id_->track_name_hash_,
        id_->label_);
    }

  return track;
}

zrythm::gui::old_dsp::plugins::Plugin *
Port::get_plugin (bool warn_if_fail) const
{
  z_return_val_if_fail (IS_PORT (this), nullptr);

  /* if DSP thread, return the pointer */
  if (
    plugin_ && PROJECT && PROJECT->loaded_ && ROUTER
    && ROUTER->is_processing_thread ()) [[likely]]
    {
      z_return_val_if_fail (plugin_, nullptr);
      return plugin_;
    }

  if (this->id_->owner_type_ != PortIdentifier::OwnerType::Plugin)
    {
      if (warn_if_fail)
        z_warning ("port not owned by plugin");
      return nullptr;
    }

  auto track = this->get_track (false);
  if (!track || (!track->is_modulator () && !track->has_channel ()))
    {
      if (warn_if_fail)
        {
          z_warning ("No track found for port");
        }
      return nullptr;
    }

  zrythm::gui::old_dsp::plugins::Plugin * pl = nullptr;
  const auto                         &pl_id = id_->plugin_id_;
  switch (pl_id.slot_type_)
    {
    case zrythm::dsp::PluginSlotType::MidiFx:
      pl =
        dynamic_cast<ChannelTrack *> (track)
          ->channel_->midi_fx_[pl_id.slot_]
          .get ();
      break;
    case zrythm::dsp::PluginSlotType::Instrument:
      pl = dynamic_cast<ChannelTrack *> (track)->channel_->instrument_.get ();
      break;
    case zrythm::dsp::PluginSlotType::Insert:
      pl =
        dynamic_cast<ChannelTrack *> (track)
          ->channel_->inserts_[pl_id.slot_]
          .get ();
      break;
    case zrythm::dsp::PluginSlotType::Modulator:
      pl =
        dynamic_cast<ModulatorTrack *> (track)->modulators_[pl_id.slot_].get ();
      break;
    default:
      z_return_val_if_reached (nullptr);
      break;
    }

  if (!pl && warn_if_fail)
    {
      z_error (
        "plugin at slot type %s (slot %d) not found for port %s",
        ENUM_NAME (pl_id.slot_type_), pl_id.slot_, id_->label_);
      return nullptr;
    }

  return pl;
}

uint32_t
Port::get_hash () const
{
  return utils::hash::get_object_hash (*this);
}

bool
Port::is_in_active_project () const
{
  switch (id_->owner_type_)
    {
    case PortIdentifier::OwnerType::AudioEngine:
      return engine_ && engine_->is_in_active_project ();
    case PortIdentifier::OwnerType::Plugin:
      return plugin_ && plugin_->is_in_active_project ();
    case PortIdentifier::OwnerType::Track:
    case PortIdentifier::OwnerType::Channel:
    case PortIdentifier::OwnerType::TrackProcessor:
      return track_ && track_->is_in_active_project ();
    case PortIdentifier::OwnerType::Fader:
      return fader_ && fader_->is_in_active_project ();
    case PortIdentifier::OwnerType::ChannelSend:
      return channel_send_ && channel_send_->is_in_active_project ();
    case PortIdentifier::OwnerType::ModulatorMacroProcessor:
      return modulator_macro_processor_
             && modulator_macro_processor_->is_in_active_project ();
    case PortIdentifier::OwnerType::HardwareProcessor:
      return ext_port_ && ext_port_->is_in_active_project ();
    case PortIdentifier::OwnerType::Transport:
      return transport_ && transport_->is_in_active_project ();
    default:
      z_return_val_if_reached (false);
    }
}

void
Port::connect_to (PortConnectionsManager &mgr, Port &dest, bool locked)
{
  mgr.ensure_connect (*id_, *dest.id_, 1.f, locked, true);
}

void
Port::disconnect_from (PortConnectionsManager &mgr, Port &dest)
{
  mgr.ensure_disconnect (*id_, *dest.id_);
}

template void
Port::set_owner<zrythm::gui::old_dsp::plugins::Plugin> (
  zrythm::gui::old_dsp::plugins::Plugin *);
template void
Port::set_owner<Transport> (Transport *);
template void
Port::set_owner<ChannelSend> (ChannelSend *);
template void
Port::set_owner<AudioEngine> (AudioEngine *);
template void
Port::set_owner<Fader> (Fader *);
template void
Port::set_owner<Track> (Track *);
template void
Port::set_owner<ModulatorMacroProcessor> (ModulatorMacroProcessor *);
template void
Port::set_owner<ExtPort> (ExtPort *);
template void
Port::set_owner (zrythm::gui::Channel *);
template void
Port::set_owner<TrackProcessor> (TrackProcessor *);
template void
Port::set_owner<HardwareProcessor> (HardwareProcessor *);
template void
Port::set_owner<TempoTrack> (TempoTrack *);
template void
Port::set_owner<RecordableTrack> (RecordableTrack *);
