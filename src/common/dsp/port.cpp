// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "common/dsp/channel.h"
#include "common/dsp/control_port.h"
#include "common/dsp/cv_port.h"
#include "common/dsp/engine_jack.h"
#include "common/dsp/graph.h"
#include "common/dsp/hardware_processor.h"
#include "common/dsp/master_track.h"
#include "common/dsp/midi_event.h"
#include "common/dsp/midi_port.h"
#include "common/dsp/modulator_macro_processor.h"
#include "common/dsp/modulator_track.h"
#include "common/dsp/port.h"
#include "common/dsp/port_identifier.h"
#include "common/dsp/recordable_track.h"
#include "common/dsp/router.h"
#include "common/dsp/rtaudio_device.h"
#include "common/dsp/rtmidi_device.h"
#include "common/dsp/tempo_track.h"
#include "common/dsp/tracklist.h"
#include "common/plugins/carla_native_plugin.h"
#include "common/plugins/plugin.h"
#include "common/utils/dsp.h"
#include "common/utils/hash.h"
#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/event_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/gtk_widgets/channel.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <fmt/format.h>

Port::Port (
  std::string label,
  PortType    type,
  PortFlow    flow,
  float       minf,
  float       maxf,
  float       zerof)
    : minf_ (minf), maxf_ (maxf), zerof_ (zerof)
{
  id_.label_ = label;
  id_.type_ = type;
  id_.flow_ = flow;
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

template <typename T>
T *
Port::find_from_identifier (const PortIdentifier &id)
{
  static_assert (FinalPortSubclass<T>);

  const auto &flags = id.flags_;
  const auto &flags2 = id.flags2_;
  const auto  track_name_hash = id.track_name_hash_;
  switch (id.owner_type_)
    {
    case PortIdentifier::OwnerType::AudioEngine:
      if constexpr (std::is_same_v<T, MidiPort>)
        {
          if (id.is_output ())
            { /* TODO */
            }
          else if (id.is_input ())
            {
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags,
                  PortIdentifier::Flags::ManualPress))
                return AUDIO_ENGINE->midi_editor_manual_press_.get ();
            }
        }
      else if constexpr (std::is_same_v<T, AudioPort>)
        {
          if (id.is_output ())
            {
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::StereoL))
                return &AUDIO_ENGINE->monitor_out_->get_l ();
              else if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::StereoR))
                return &AUDIO_ENGINE->monitor_out_->get_r ();
            }
        }
      break;
    case PortIdentifier::OwnerType::Plugin:
      {
        auto tr = TRACKLIST->find_track_by_name_hash<ProcessableTrack> (
          track_name_hash);
        if (!tr)
          tr = SAMPLE_PROCESSOR->tracklist_->find_track_by_name_hash<
            ProcessableTrack> (track_name_hash);
        z_return_val_if_fail (tr, nullptr);
        zrythm::plugins::Plugin * pl = nullptr;
        if (tr->has_channel ())
          {
            auto channel_track = dynamic_cast<ChannelTrack *> (tr);
            pl = channel_track->channel_->get_plugin_at_slot (
              id.plugin_id_.slot_, id.plugin_id_.slot_type_);
          }
        else if (tr->is_modulator ())
          {
            auto modulator_track = dynamic_cast<ModulatorTrack *> (tr);
            pl = modulator_track->modulators_[id.plugin_id_.slot_].get ();
          }
        z_return_val_if_fail (pl, nullptr);

        switch (id.flow_)
          {
          case PortFlow::Input:
            return dynamic_cast<T *> (pl->in_ports_[id.port_index_].get ());
          case PortFlow::Output:
            return dynamic_cast<T *> (pl->out_ports_[id.port_index_].get ());
          default:
            z_return_val_if_reached (nullptr);
          }
      }
      break;
    case PortIdentifier::OwnerType::TrackProcessor:
      {
        auto tr = TRACKLIST->find_track_by_name_hash<ProcessableTrack> (
          track_name_hash);
        if (!tr)
          tr = SAMPLE_PROCESSOR->tracklist_->find_track_by_name_hash<
            ProcessableTrack> (track_name_hash);
        z_return_val_if_fail (tr, nullptr);
        auto &processor = tr->processor_;
        if constexpr (std::is_same_v<T, MidiPort>)
          {
            if (id.is_output ())
              return processor->midi_out_.get ();
            else if (id.is_input ())
              {
                if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags, flags,
                    PortIdentifier::Flags::PianoRoll))
                  return processor->piano_roll_.get ();
                else
                  return processor->midi_in_.get ();
              }
          }
        else if constexpr (std::is_same_v<T, AudioPort>)
          {
            if (id.is_output ())
              {
                if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags, flags, PortIdentifier::Flags::StereoL))
                  return &processor->stereo_out_->get_l ();
                else if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags, flags, PortIdentifier::Flags::StereoR))
                  return &processor->stereo_out_->get_r ();
              }
            else if (id.is_input ())
              {
                z_return_val_if_fail (processor->stereo_in_, nullptr);
                if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags, flags, PortIdentifier::Flags::StereoL))
                  return &processor->stereo_in_->get_l ();
                else if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags, flags, PortIdentifier::Flags::StereoR))
                  return &processor->stereo_in_->get_r ();
              }
          }
        else if constexpr (std::is_same_v<T, ControlPort>)
          {
            if (ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::TpMono))
              return processor->mono_.get ();
            else if (
              ENUM_BITSET_TEST (
                PortIdentifier::Flags, flags, PortIdentifier::Flags::TpInputGain))
              return processor->input_gain_.get ();
            else if (
              ENUM_BITSET_TEST (
                PortIdentifier::Flags2, flags2,
                PortIdentifier::Flags2::TpOutputGain))
              return processor->output_gain_.get ();
            else if (
              ENUM_BITSET_TEST (
                PortIdentifier::Flags2, flags2,
                PortIdentifier::Flags2::TpMonitorAudio))
              return processor->monitor_audio_.get ();
            else if (
              ENUM_BITSET_TEST (
                PortIdentifier::Flags, flags,
                PortIdentifier::Flags::MidiAutomatable))
              {
                if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags2, flags2,
                    PortIdentifier::Flags2::MidiPitchBend))
                  return processor->pitch_bend_[id.port_index_].get ();
                else if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags2, flags2,
                    PortIdentifier::Flags2::MidiPolyKeyPressure))
                  return processor->poly_key_pressure_[id.port_index_].get ();
                else if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags2, flags2,
                    PortIdentifier::Flags2::MidiChannelPressure))
                  return processor->channel_pressure_[id.port_index_].get ();
                else
                  return processor->midi_cc_[id.port_index_].get ();
              }
            break;
          }
      }
      break;
    case PortIdentifier::OwnerType::Track:
      {
        auto tr = TRACKLIST->find_track_by_name_hash<Track> (track_name_hash);
        if (!tr)
          tr = SAMPLE_PROCESSOR->tracklist_->find_track_by_name_hash<Track> (
            track_name_hash);
        z_return_val_if_fail (tr, nullptr);
        if constexpr (std::is_same_v<T, ControlPort>)
          {
            if (ENUM_BITSET_TEST (
                  PortIdentifier::Flags, flags, PortIdentifier::Flags::Bpm))
              return dynamic_cast<TempoTrack *> (tr)->bpm_port_.get ();
            else if (
              ENUM_BITSET_TEST (
                PortIdentifier::Flags2, flags2,
                PortIdentifier::Flags2::BeatsPerBar))
              return dynamic_cast<TempoTrack *> (tr)->beats_per_bar_port_.get ();
            else if (
              ENUM_BITSET_TEST (
                PortIdentifier::Flags2, flags2, PortIdentifier::Flags2::BeatUnit))
              return dynamic_cast<TempoTrack *> (tr)->beat_unit_port_.get ();
            else if (
              ENUM_BITSET_TEST (
                PortIdentifier::Flags2, flags2,
                PortIdentifier::Flags2::TrackRecording))
              return dynamic_cast<RecordableTrack *> (tr)->recording_.get ();
          }
      }
      break;
    case PortIdentifier::OwnerType::Fader:
      {
        auto fader = Fader::find_from_port_identifier (id);
        z_return_val_if_fail (fader, nullptr);
        if constexpr (std::is_same_v<T, MidiPort>)
          {
            return id.is_input () ? fader->midi_in_.get () : fader->midi_out_.get ();
          }
        else if constexpr (std::is_same_v<T, AudioPort>)
          {
            if (id.is_output ())
              {
                if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags, flags, PortIdentifier::Flags::StereoL))
                  return &fader->stereo_out_->get_l ();
                else if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags, flags, PortIdentifier::Flags::StereoR))
                  return &fader->stereo_out_->get_r ();
              }
            else if (id.is_input ())
              {
                if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags, flags, PortIdentifier::Flags::StereoL))
                  return &fader->stereo_in_->get_l ();
                else if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags, flags, PortIdentifier::Flags::StereoR))
                  return &fader->stereo_in_->get_r ();
              }
          }
        else if constexpr (std::is_same_v<T, ControlPort>)
          {
            if (id.is_input ())
              {
                if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags, flags,
                    PortIdentifier::Flags::Amplitude))
                  return fader->amp_.get ();
                else if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags, flags,
                    PortIdentifier::Flags::StereoBalance))
                  return fader->balance_.get ();
                else if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags, flags,
                    PortIdentifier::Flags::FaderMute))
                  return fader->mute_.get ();
                else if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags2, flags2,
                    PortIdentifier::Flags2::FaderSolo))
                  return fader->solo_.get ();
                else if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags2, flags2,
                    PortIdentifier::Flags2::FaderListen))
                  return fader->listen_.get ();
                else if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags2, flags2,
                    PortIdentifier::Flags2::FaderMonoCompat))
                  return fader->mono_compat_enabled_.get ();
                else if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags2, flags2,
                    PortIdentifier::Flags2::FaderSwapPhase))
                  return fader->swap_phase_.get ();
              }
          }
        z_return_val_if_reached (nullptr);
      }
      break;
    case PortIdentifier::OwnerType::ChannelSend:
      {
        auto tr =
          TRACKLIST->find_track_by_name_hash<ChannelTrack> (track_name_hash);
        if (!tr)
          tr = SAMPLE_PROCESSOR->tracklist_->find_track_by_name_hash<
            ChannelTrack> (track_name_hash);
        z_return_val_if_fail (tr, nullptr);
        auto &ch = tr->channel_;
        if constexpr (std::is_same_v<T, ControlPort>)
          {
            if (
              ENUM_BITSET_TEST (
                PortIdentifier::Flags2, flags2,
                PortIdentifier::Flags2::ChannelSendEnabled))
              return ch->sends_.at (id.port_index_)->enabled_.get ();
            else if (
              ENUM_BITSET_TEST (
                PortIdentifier::Flags2, flags2,
                PortIdentifier::Flags2::ChannelSendAmount))
              return ch->sends_.at (id.port_index_)->amount_.get ();
          }
        else
          {
            if (id.is_input ())
              {
                if constexpr (std::is_same_v<T, AudioPort>)
                  {
                    if (
                      ENUM_BITSET_TEST (
                        PortIdentifier::Flags, flags,
                        PortIdentifier::Flags::StereoL))
                      return &ch->sends_.at (id.port_index_)->stereo_in_->get_l ();
                    else if (
                      ENUM_BITSET_TEST (
                        PortIdentifier::Flags, flags,
                        PortIdentifier::Flags::StereoR))
                      return &ch->sends_.at (id.port_index_)->stereo_in_->get_r ();
                  }
                else if constexpr (std::is_same_v<T, MidiPort>)
                  return ch->sends_.at (id.port_index_)->midi_in_.get ();
              }
            else if (id.is_output ())
              {
                if constexpr (std::is_same_v<T, AudioPort>)
                  {
                    if (
                      ENUM_BITSET_TEST (
                        PortIdentifier::Flags, flags,
                        PortIdentifier::Flags::StereoL))
                      return &ch->sends_.at (id.port_index_)->stereo_out_->get_l ();
                    else if (
                      ENUM_BITSET_TEST (
                        PortIdentifier::Flags, flags,
                        PortIdentifier::Flags::StereoR))
                      return &ch->sends_.at (id.port_index_)->stereo_out_->get_r ();
                  }
                else if constexpr (std::is_same_v<T, MidiPort>)
                  return ch->sends_.at (id.port_index_)->midi_out_.get ();
              }
          }
        z_return_val_if_reached (nullptr);
      }
      break;
    case PortIdentifier::OwnerType::HardwareProcessor:
      {
        Port * port = nullptr;

        /* note: flows are reversed */
        if (id.is_output ())
          port = HW_IN_PROCESSOR->find_port (id.ext_port_id_);
        else if (id.is_input ())
          port = HW_OUT_PROCESSOR->find_port (id.ext_port_id_);

        /* just warn if hardware is not connected anymore */
        z_warn_if_fail (port);

        return dynamic_cast<T *> (port);
      }
      break;
    case PortIdentifier::OwnerType::Transport:
      if constexpr (std::is_same_v<T, MidiPort>)
        {
          if (id.is_input ())
            {
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::TransportRoll))
                return TRANSPORT->roll_.get ();
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::TransportStop))
                return TRANSPORT->stop_.get ();
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::TransportBackward))
                return TRANSPORT->backward_.get ();
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::TransportForward))
                return TRANSPORT->forward_.get ();
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::TransportLoopToggle))
                return TRANSPORT->loop_toggle_.get ();
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, flags2,
                  PortIdentifier::Flags2::TransportRecToggle))
                return TRANSPORT->rec_toggle_.get ();
            }
        }
      break;
    case PortIdentifier::OwnerType::ModulatorMacroProcessor:
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, flags, PortIdentifier::Flags::ModulatorMacro))
        {
          auto tr = TRACKLIST->find_track_by_name_hash<ModulatorTrack> (
            track_name_hash);
          z_return_val_if_fail (tr, nullptr);
          auto &processor = tr->modulator_macro_processors_[id.port_index_];
          if (id.is_input ())
            {
              if constexpr (std::is_same_v<T, CVPort>)
                {
                  return processor->cv_in_.get ();
                }
              else if constexpr (std::is_same_v<T, ControlPort>)
                {
                  return processor->macro_.get ();
                }
            }
          else if (id.is_output ())
            {
              if constexpr (std::is_same_v<T, CVPort>)
                {
                  return processor->cv_out_.get ();
                }
            }
        }
      break;
    case PortIdentifier::OwnerType::Channel:
      {
        auto tr =
          TRACKLIST->find_track_by_name_hash<ChannelTrack> (track_name_hash);
        if (!tr)
          tr = SAMPLE_PROCESSOR->tracklist_->find_track_by_name_hash<
            ChannelTrack> (track_name_hash);
        z_return_val_if_fail (tr, nullptr);
        auto &ch = tr->channel_;
        z_return_val_if_fail (ch, nullptr);
        if constexpr (std::is_same_v<T, MidiPort>)
          {
            if (id.is_output ())
              {
                return ch->midi_out_.get ();
              }
          }
        else if constexpr (std::is_same_v<T, AudioPort>)
          {
            if (id.is_output ())
              {
                if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags, flags, PortIdentifier::Flags::StereoL))
                  return &ch->stereo_out_->get_l ();
                else if (
                  ENUM_BITSET_TEST (
                    PortIdentifier::Flags, flags, PortIdentifier::Flags::StereoR))
                  return &ch->stereo_out_->get_r ();
              }
          }
      }
      break;
    default:
      z_return_val_if_reached (nullptr);
    }

  z_return_val_if_reached (nullptr);
}

Port *
Port::find_from_identifier (const PortIdentifier &id)
{
  switch (id.type_)
    {
    case PortType::Audio:
      return find_from_identifier<AudioPort> (id);
    case PortType::Event:
      return find_from_identifier<MidiPort> (id);
    case PortType::Control:
      return find_from_identifier<ControlPort> (id);
    case PortType::CV:
      return find_from_identifier<CVPort> (id);
    default:
      z_return_val_if_reached (nullptr);
    };
}

#if HAVE_JACK
void
Port::sum_data_from_jack (const nframes_t start_frame, const nframes_t nframes)
{
  if (
    id_.owner_type_ == PortIdentifier::OwnerType::AudioEngine
    || internal_type_ != Port::InternalType::JackPort || !is_input ())
    return;

  /* append events from JACK if any */
  if (
    is_midi () && AUDIO_ENGINE->midi_backend_ == MidiBackend::MIDI_BACKEND_JACK)
    {
      auto midi_port = static_cast<MidiPort *> (this);
      midi_port->receive_midi_events_from_jack (start_frame, nframes);
    }
  else if (
    is_audio ()
    && AUDIO_ENGINE->audio_backend_ == AudioBackend::AUDIO_BACKEND_JACK)
    {
      auto audio_port = static_cast<AudioPort *> (this);
      audio_port->receive_audio_data_from_jack (start_frame, nframes);
    }
}

void
Port::send_data_to_jack (const nframes_t start_frame, const nframes_t nframes)
{
  if (internal_type_ != Port::InternalType::JackPort || !is_output ())
    return;

  if (
    is_midi () && AUDIO_ENGINE->midi_backend_ == MidiBackend::MIDI_BACKEND_JACK)
    {
      auto midi_port = static_cast<MidiPort *> (this);
      midi_port->send_midi_events_to_jack (start_frame, nframes);
    }
  if (
    is_audio ()
    && AUDIO_ENGINE->audio_backend_ == AudioBackend::AUDIO_BACKEND_JACK)
    {
      auto audio_port = static_cast<AudioPort *> (this);
      audio_port->send_audio_data_to_jack (start_frame, nframes);
    }
}

void
Port::expose_to_jack (bool expose)
{
  enum JackPortFlags flags;
  if (id_.owner_type_ == PortIdentifier::OwnerType::HardwareProcessor)
    {
      /* these are reversed */
      if (is_input ())
        flags = JackPortIsOutput;
      else if (is_output ())
        flags = JackPortIsInput;
      else
        {
          z_return_if_reached ();
        }
    }
  else
    {
      if (is_input ())
        flags = JackPortIsInput;
      else if (is_output ())
        flags = JackPortIsOutput;
      else
        {
          z_return_if_reached ();
        }
    }

  const char * type = engine_jack_get_jack_type (id_.type_);
  if (!type)
    z_return_if_reached ();

  auto label = get_full_designation ();
  if (expose)
    {
      z_info ("exposing port {} to JACK", label);
      if (!data_)
        {
          data_ = (void *) jack_port_register (
            AUDIO_ENGINE->client_, label.c_str (), type, flags, 0);
        }
      z_return_if_fail (data_);
      internal_type_ = InternalType::JackPort;
    }
  else
    {
      z_info ("unexposing port {} from JACK", label);
      if (AUDIO_ENGINE->client_)
        {
          z_warn_if_fail (data_);
          int ret =
            jack_port_unregister (AUDIO_ENGINE->client_, JACK_PORT_T (data_));
          if (ret)
            {
              auto jack_error =
                engine_jack_get_error_message ((jack_status_t) ret);
              z_warning ("JACK port unregister error: {}", jack_error);
            }
        }
      internal_type_ = InternalType::None;
      data_ = NULL;
    }

  exposed_to_backend_ = expose;
}
#endif /* HAVE_JACK */

int
Port::get_num_unlocked (bool sources) const
{
  z_return_val_if_fail (is_in_active_project (), 0);
  return PORT_CONNECTIONS_MGR->get_unlocked_sources_or_dests (
    nullptr, id_, sources);
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

  if constexpr (std::derived_from<T, zrythm::plugins::Plugin>)
    {
      id_.plugin_id_ = owner->id_;
      id_.track_name_hash_ = owner->id_.track_name_hash_;
      id_.owner_type_ = PortIdentifier::OwnerType::Plugin;

      if (is_control ())
        {
          auto control_port = static_cast<ControlPort *> (this);
          if (control_port->at_)
            {
              control_port->at_->port_id_ = id_;
            }
        }
    }
  else if constexpr (std::derived_from<T, TrackProcessor>)
    {
      auto track = owner->get_track ();
      z_return_if_fail (track);
      id_.track_name_hash_ = get_track_name_hash (track);
      id_.owner_type_ = PortIdentifier::OwnerType::TrackProcessor;
    }
  else if constexpr (std::derived_from<T, Channel>)
    {
      auto track = owner->get_track ();
      z_return_if_fail (track);
      id_.track_name_hash_ = get_track_name_hash (track);
      id_.owner_type_ = PortIdentifier::OwnerType::Channel;
    }
  else if constexpr (std::derived_from<T, ExtPort>)
    {
      ext_port_ = owner;
      id_.owner_type_ = PortIdentifier::OwnerType::HardwareProcessor;
    }
  else if constexpr (std::derived_from<T, ChannelSend>)
    {
      id_.track_name_hash_ = owner->track_name_hash_;
      id_.port_index_ = owner->slot_;
      id_.owner_type_ = PortIdentifier::OwnerType::ChannelSend;
      channel_send_ = owner;

      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id_.flags2_,
          PortIdentifier::Flags2::ChannelSendEnabled))
        {
          minf_ = 0.f;
          maxf_ = 1.f;
          zerof_ = 0.0f;
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id_.flags2_,
          PortIdentifier::Flags2::ChannelSendAmount))
        {
          minf_ = 0.f;
          maxf_ = 2.f;
          zerof_ = 0.f;
        }
    }
  else if constexpr (std::derived_from<T, Fader>)
    {
      id_.owner_type_ = PortIdentifier::OwnerType::Fader;
      fader_ = owner;
      z_return_if_fail (fader_);

      if (
        owner->type_ == Fader::Type::AudioChannel
        || owner->type_ == Fader::Type::MidiChannel)
        {
          auto track = owner->get_track ();
          z_return_if_fail (track);
          id_.track_name_hash_ = get_track_name_hash (track);
          if (owner->passthrough_)
            {
              id_.flags2_ |= PortIdentifier::Flags2::Prefader;
            }
          else
            {
              id_.flags2_ |= PortIdentifier::Flags2::Postfader;
            }
        }
      else if (owner->type_ == Fader::Type::SampleProcessor)
        {
          id_.flags2_ |= PortIdentifier::Flags2::SampleProcessorFader;
        }
      else
        {
          id_.flags2_ |= PortIdentifier::Flags2::MonitorFader;
        }

      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::Amplitude))
        {
          minf_ = 0.f;
          maxf_ = 2.f;
          zerof_ = 0.f;
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id_.flags_,
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
      id_.owner_type_ = PortIdentifier::OwnerType::Transport;
    }
  else if constexpr (std::derived_from<T, Track>)
    {
      // z_return_if_fail (!track.name_.empty ());
      id_.track_name_hash_ = owner->name_.empty () ? 0 : owner->get_name_hash ();
      id_.owner_type_ = PortIdentifier::OwnerType::Track;
      track_ = owner;
    }
  else if constexpr (std::derived_from<T, ModulatorMacroProcessor>)
    {
      modulator_macro_processor_ = owner;
      id_.owner_type_ = PortIdentifier::OwnerType::ModulatorMacroProcessor;
      z_return_if_fail (owner->get_track ());
      id_.track_name_hash_ = owner->get_track ()->get_name_hash ();
      track_ = owner->get_track ();
    }
  else if constexpr (std::derived_from<T, AudioEngine>)
    {
      engine_ = owner;
      id_.owner_type_ = PortIdentifier::OwnerType::AudioEngine;
    }
}

std::string
Port::get_label () const
{
  return id_.get_label ();
}

bool
Port::can_be_connected_to (const Port &dest) const
{
  return Graph (ROUTER.get ()).validate_with_connection (this, &dest);
}

void
Port::disconnect_ports (std::vector<Port *> &ports, bool deleting)
{
  if (!PORT_CONNECTIONS_MGR)
    return;

  /* can only be called from the gtk thread */
  z_return_if_fail (ZRYTHM_APP_IS_GTK_THREAD);

  /* go through each port */
  for (auto port : ports)
    {
      PORT_CONNECTIONS_MGR->ensure_disconnect_all (port->id_);
      port->srcs_.clear ();
      port->dests_.clear ();
      port->deleting_ = deleting;
    }
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

  std::vector<PortConnection> srcs;
  PORT_CONNECTIONS_MGR->get_sources_or_dests (&srcs, id_, true);
  for (const auto &conn : srcs)
    {
      PORT_CONNECTIONS_MGR->ensure_disconnect (conn.src_id_, conn.dest_id_);
    }

  std::vector<PortConnection> dests;
  PORT_CONNECTIONS_MGR->get_sources_or_dests (&dests, id_, false);
  for (const auto &conn : dests)
    {
      PORT_CONNECTIONS_MGR->ensure_disconnect (conn.src_id_, conn.dest_id_);
    }

#if HAVE_JACK
  if (this->internal_type_ == Port::InternalType::JackPort)
    {
      expose_to_jack (false);
    }
#endif

#if HAVE_RTMIDI
  if (is_midi ())
    {
      auto midi_port = static_cast<MidiPort *> (this);
      for (
        auto it = midi_port->rtmidi_ins_.begin ();
        it != midi_port->rtmidi_ins_.end (); ++it)
        {
          auto dev = *it;
          dev->close ();
          it = midi_port->rtmidi_ins_.erase (it);
        }
    }
#endif
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
      std::vector<PortConnection> srcs;
      PORT_CONNECTIONS_MGR->get_sources_or_dests (&srcs, prev_id, true);
      for (const auto &conn : srcs)
        {
          if (conn.dest_id_ != id_)
            {
              auto new_conn = conn;
              new_conn.dest_id_ = id_;
              PORT_CONNECTIONS_MGR->replace_connection (conn, new_conn);
            }
        }

      /* update in all dests */
      std::vector<PortConnection> dests;
      PORT_CONNECTIONS_MGR->get_sources_or_dests (&dests, prev_id, false);
      for (const auto &conn : dests)
        {
          if (conn.src_id_ != id_)
            {
              auto new_conn = conn;
              new_conn.src_id_ = id_;
              PORT_CONNECTIONS_MGR->replace_connection (conn, new_conn);
            }
        }

      if (
        update_automation_track && (id_.track_name_hash_ != 0)
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags, this->id_.flags_,
          PortIdentifier::Flags::Automatable))
        {
          auto * control_port = dynamic_cast<ControlPort *> (this);
          /* update automation track's port id */
          auto * automatable_track = dynamic_cast<AutomatableTrack *> (track);
          control_port->at_ = AutomationTrack::find_from_port (
            *control_port, automatable_track, true);
          z_return_if_fail (control_port->at_);
          control_port->at_->port_id_ = id_;
        }
    }
}

void
Port::update_track_name_hash (Track &track, unsigned int new_hash)
{
  PortIdentifier copy = this->id_;

  this->id_.track_name_hash_ = new_hash;
  if (this->id_.owner_type_ == PortIdentifier::OwnerType::Plugin)
    {
      this->id_.plugin_id_.track_name_hash_ = new_hash;
    }
  update_identifier (copy, &track, true);
}

void
Port::copy_members_from (const Port &other)
{
  id_ = other.id_;
  exposed_to_backend_ = other.exposed_to_backend_;
  minf_ = other.minf_;
  maxf_ = other.maxf_;
  zerof_ = other.zerof_;
}

void
Port::disconnect_hw_inputs ()
{
  std::vector<PortConnection> srcs;
  PORT_CONNECTIONS_MGR->get_sources_or_dests (&srcs, id_, true);
  for (const auto &conn : srcs)
    {
      if (
        conn.src_id_.owner_type_ == PortIdentifier::OwnerType::HardwareProcessor)
        PORT_CONNECTIONS_MGR->ensure_disconnect (conn.src_id_, id_);
    }
}

void
Port::set_expose_to_backend (bool expose)
{
  z_return_if_fail (AUDIO_ENGINE);

  if (!AUDIO_ENGINE->setup_)
    {
      z_warning (
        "audio engine not set up, skipping expose to backend for {}", id_.sym_);
      return;
    }

  if (this->id_.type_ == PortType::Audio)
    {
      auto audio_port = static_cast<AudioPort *> (this);
      (void) audio_port;
      switch (AUDIO_ENGINE->audio_backend_)
        {
        case AudioBackend::AUDIO_BACKEND_DUMMY:
          z_debug ("called with dummy audio backend");
          return;
#if HAVE_JACK
        case AudioBackend::AUDIO_BACKEND_JACK:
          expose_to_jack (expose);
          break;
#endif
#if HAVE_RTAUDIO
        case AudioBackend::AUDIO_BACKEND_ALSA_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_JACK_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_PULSEAUDIO_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_COREAUDIO_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_WASAPI_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_ASIO_RTAUDIO:
          audio_port->expose_to_rtaudio (expose);
          break;
#endif
        default:
          break;
        }
    }
  else if (this->id_.type_ == PortType::Event)
    {
      auto midi_port = static_cast<MidiPort *> (this);
      (void) midi_port;
      switch (AUDIO_ENGINE->midi_backend_)
        {
        case MidiBackend::MIDI_BACKEND_DUMMY:
          z_debug ("called with MIDI dummy backend");
          return;
#if HAVE_JACK
        case MidiBackend::MIDI_BACKEND_JACK:
          expose_to_jack (expose);
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
          midi_port->expose_to_rtmidi (expose);
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
}

void
Port::rename_backend ()
{
  if (!this->is_exposed_to_backend ())
    return;

  switch (this->internal_type_)
    {
#if HAVE_JACK
    case Port::InternalType::JackPort:
      {
        std::string str = this->get_full_designation ();
        jack_port_rename (
          AUDIO_ENGINE->client_, static_cast<jack_port_t *> (this->data_),
          str.c_str ());
      }
      break;
#endif
    default:
      break;
    }
}

std::string
Port::get_full_designation () const
{
  const auto &id = this->id_;

  switch (id.owner_type_)
    {
    case PortIdentifier::OwnerType::AudioEngine:
      return id.label_;
    case PortIdentifier::OwnerType::Plugin:
      {
        auto pl = this->get_plugin (true);
        z_return_val_if_fail (pl, {});
        auto track = pl->get_track ();
        z_return_val_if_fail (track, {});
        return fmt::format (
          "{}/{}/{}", track->get_name (), pl->get_name (), id.label_);
      }
    case PortIdentifier::OwnerType::Channel:
    case PortIdentifier::OwnerType::Track:
    case PortIdentifier::OwnerType::TrackProcessor:
    case PortIdentifier::OwnerType::ChannelSend:
      {
        auto tr = this->get_track (true);
        z_return_val_if_fail (tr, {});
        return fmt::format ("{}/{}", tr->get_name (), id.label_);
      }
    case PortIdentifier::OwnerType::Fader:
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id.flags2_, PortIdentifier::Flags2::Prefader)
        || ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id.flags2_, PortIdentifier::Flags2::Postfader))
        {
          auto tr = this->get_track (true);
          z_return_val_if_fail (tr, {});
          return fmt::format ("{}/{}", tr->get_name (), id.label_);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id.flags2_,
          PortIdentifier::Flags2::MonitorFader))
        {
          return fmt::format ("Engine/{}", id.label_);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id.flags2_,
          PortIdentifier::Flags2::SampleProcessorFader))
        {
          return id.label_;
        }
      break;
    case PortIdentifier::OwnerType::HardwareProcessor:
      return fmt::format ("HW/{}", id.label_);
    case PortIdentifier::OwnerType::Transport:
      return fmt::format ("Transport/{}", id.label_);
    case PortIdentifier::OwnerType::ModulatorMacroProcessor:
      return fmt::format ("Modulator Macro Processor/{}", id.label_);
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
  if (!this->is_exposed_to_backend ())
    {
      return;
    }

#if HAVE_JACK
  if (this->internal_type_ != Port::InternalType::JackPort)
    {
      return;
    }

  auto jport = static_cast<jack_port_t *> (this->data_);
  z_return_if_fail (jport);
  void * buf = jack_port_get_buffer (jport, AUDIO_ENGINE->block_length_);
  z_return_if_fail (buf);
  if (
    this->id_.type_ == PortType::Audio
    && AUDIO_ENGINE->audio_backend_ == AudioBackend::AUDIO_BACKEND_JACK)
    {
      auto fbuf = static_cast<float *> (buf);
      dsp_fill (
        &fbuf[0], DENORMAL_PREVENTION_VAL (AUDIO_ENGINE),
        AUDIO_ENGINE->block_length_);
    }
  else if (
    this->id_.type_ == PortType::Event
    && AUDIO_ENGINE->midi_backend_ == MidiBackend::MIDI_BACKEND_JACK)
    {
      jack_midi_clear_buffer (buf);
    }
#endif
}

Track *
Port::get_track (bool warn_if_fail) const
{
  z_return_val_if_fail (IS_PORT (this), nullptr);

  /* return the pointer if dsp thread */
  if (
    this->track_ && PROJECT && PROJECT->loaded_ && ROUTER
    && ROUTER->is_processing_thread ()) [[likely]]
    {
      z_return_val_if_fail (track_, nullptr);
      return track_;
    }

  Track * track = nullptr;
  if (this->id_.track_name_hash_ != 0)
    {
      z_return_val_if_fail (gZrythm && TRACKLIST, nullptr);

      track = TRACKLIST->find_track_by_name_hash (this->id_.track_name_hash_);
      if (!track)
        track = SAMPLE_PROCESSOR->tracklist_->find_track_by_name_hash (
          id_.track_name_hash_);
    }

  if (!track && warn_if_fail)
    {
      z_warning (
        "track with name hash %d not found for port %s", id_.track_name_hash_,
        id_.label_);
    }

  return track;
}

zrythm::plugins::Plugin *
Port::get_plugin (bool warn_if_fail) const
{
  z_return_val_if_fail (IS_PORT (this), nullptr);

  /* if DSP thread, return the pointer */
  if (
    this->plugin_ && PROJECT && PROJECT->loaded_ && ROUTER
    && ROUTER->is_processing_thread ()) [[likely]]
    {
      z_return_val_if_fail (plugin_, nullptr);
      return plugin_;
    }

  if (this->id_.owner_type_ != PortIdentifier::OwnerType::Plugin)
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

  zrythm::plugins::Plugin * pl = nullptr;
  const auto               &pl_id = id_.plugin_id_;
  switch (pl_id.slot_type_)
    {
    case zrythm::plugins::PluginSlotType::MidiFx:
      pl =
        dynamic_cast<ChannelTrack *> (track)
          ->channel_->midi_fx_[pl_id.slot_]
          .get ();
      break;
    case zrythm::plugins::PluginSlotType::Instrument:
      pl = dynamic_cast<ChannelTrack *> (track)->channel_->instrument_.get ();
      break;
    case zrythm::plugins::PluginSlotType::Insert:
      pl =
        dynamic_cast<ChannelTrack *> (track)
          ->channel_->inserts_[pl_id.slot_]
          .get ();
      break;
    case zrythm::plugins::PluginSlotType::Modulator:
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
        ENUM_NAME (pl_id.slot_type_), pl_id.slot_, id_.label_);
      return nullptr;
    }

  return pl;
}

uint32_t
Port::get_hash () const
{
  return hash_get_for_struct (this, sizeof (Port));
}

uint32_t
Port::get_hash (const void * ptr)
{
  const auto * self = static_cast<const Port *> (ptr);
  return self->get_hash ();
}

bool
Port::is_connected_to (const Port &dest) const
{
  return PORT_CONNECTIONS_MGR->find_connection (id_, dest.id_).has_value ();
}

bool
Port::is_in_active_project () const
{
  switch (id_.owner_type_)
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
Port::connect_to (Port &dest, bool locked)
{
  PORT_CONNECTIONS_MGR->ensure_connect (id_, dest.id_, 1.f, locked, true);
}

void
Port::disconnect_from (Port &dest)
{
  PORT_CONNECTIONS_MGR->ensure_disconnect (id_, dest.id_);
}

template MidiPort *
Port::find_from_identifier<MidiPort> (const PortIdentifier &);
template AudioPort *
Port::find_from_identifier (const PortIdentifier &);
template CVPort *
Port::find_from_identifier (const PortIdentifier &);
template ControlPort *
Port::find_from_identifier (const PortIdentifier &);
template void
Port::set_owner<zrythm::plugins::Plugin> (zrythm::plugins::Plugin *);
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
Port::set_owner<Channel> (Channel *);
template void
Port::set_owner<TrackProcessor> (TrackProcessor *);
template void
Port::set_owner<HardwareProcessor> (HardwareProcessor *);
template void
Port::set_owner<TempoTrack> (TempoTrack *);
template void
Port::set_owner<RecordableTrack> (RecordableTrack *);