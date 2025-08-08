// SPDX-FileCopyrightText: © 2020-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/channel_send.h"
#include "utils/dsp.h"
#include "utils/math.h"

#include <fmt/format.h>

namespace zrythm::structure::tracks
{

ChannelSend::ChannelSend (
  dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
  dsp::PortType                                 signal_type,
  int                                           slot,
  bool                                          is_prefader,
  QObject *                                     parent)
    : QObject (parent), dsp::ProcessorBase (dependencies),
      signal_type_ (signal_type), is_prefader_ (is_prefader)
{
  add_parameter (dependencies.param_registry_.create_object<dsp::ProcessorParameter> (
    dependencies.port_registry_,
    dsp::ProcessorParameter::UniqueId (
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("channel_send_{}_amount", slot + 1))),
    dsp::ParameterRange (
      dsp::ParameterRange::Type::GainAmplitude, 0.f, 2.f, 0.f, 1.f),
    utils::Utf8String::from_qstring (QObject::tr ("Amount"))));

  if (is_audio ())
    {
      {
        auto stereo_in_ports = dsp::StereoPorts::create_stereo_ports (
          dependencies.port_registry_, true,
          utils::Utf8String::from_qstring (QObject::tr ("Audio input")),
          utils::Utf8String::from_utf8_encoded_string (
            fmt::format ("channel_send_{}_audio_in", slot + 1)));
        add_input_port (stereo_in_ports.first);
        add_input_port (stereo_in_ports.second);
      }

      {
        auto stereo_out_ports = dsp::StereoPorts::create_stereo_ports (
          dependencies.port_registry_, false,
          utils::Utf8String::from_qstring (QObject::tr ("Audio output")),
          utils::Utf8String::from_utf8_encoded_string (
            fmt::format ("channel_send_{}_audio_out", slot + 1)));
        add_output_port (stereo_out_ports.first);
        add_output_port (stereo_out_ports.second);
      }
    }
  else if (is_midi ())
    {
      add_input_port (dependencies.port_registry_.create_object<dsp::MidiPort> (
        utils::Utf8String::from_qstring (QObject::tr ("MIDI input")),
        dsp::PortFlow::Input));
      auto &midi_in_port = get_midi_in_port ();
      midi_in_port.set_symbol (
        utils::Utf8String::from_utf8_encoded_string (
          fmt::format ("channel_send_{}_midi_in", slot + 1)));

      add_output_port (dependencies.port_registry_.create_object<dsp::MidiPort> (
        utils::Utf8String::from_qstring (QObject::tr ("MIDI output")),
        dsp::PortFlow::Output));
      auto &midi_out_port = get_midi_out_port ();
      midi_out_port.set_symbol (
        utils::Utf8String::from_utf8_encoded_string (
          fmt::format ("channel_send_{}_midi_out", slot + 1)));
    }

  set_name (
    utils::Utf8String::from_utf8_encoded_string (
      fmt::format ("Channel Send {}", slot + 1)));
}

void
ChannelSend::custom_process_block (const EngineProcessTimeInfo time_nfo) noexcept
{
  const auto local_offset = time_nfo.local_offset_;
  const auto nframes = time_nfo.nframes_;
  if (is_audio ())
    {
      const auto &amount_param = amountParam ();
      const auto  amount_val =
        amount_param->range ().convertFrom0To1 (amount_param->currentValue ());
      const auto &stereo_in_ports = get_stereo_in_ports ();
      const auto &stereo_out_ports = get_stereo_out_ports ();
      if (utils::math::floats_near (amount_val, 1.f, 0.00001f))
        {
          utils::float_ranges::copy (
            &stereo_out_ports.first.buf_[local_offset],
            &stereo_in_ports.first.buf_[local_offset], nframes);
          utils::float_ranges::copy (
            &stereo_out_ports.second.buf_[local_offset],
            &stereo_in_ports.second.buf_[local_offset], nframes);
        }
      else
        {
          utils::float_ranges::product (
            &stereo_out_ports.first.buf_[local_offset],
            &stereo_in_ports.first.buf_[local_offset], amount_val, nframes);
          utils::float_ranges::product (
            &stereo_out_ports.second.buf_[local_offset],
            &stereo_in_ports.second.buf_[local_offset], amount_val, nframes);
        }
    }
  else if (is_midi ())
    {
      get_midi_out_port ().midi_events_.queued_events_.append (
        get_midi_in_port ().midi_events_.active_events_, local_offset, nframes);
    }
}

void
init_from (
  ChannelSend           &obj,
  const ChannelSend     &other,
  utils::ObjectCloneType clone_type)
{
  // TODO
  // init_from (
  //   static_cast<dsp::ProcessorBase &> (obj),
  //   static_cast<const dsp::ProcessorBase &> (other), clone_type);
  obj.signal_type_ = other.signal_type_;
  obj.is_prefader_ = other.is_prefader_;
}

void
from_json (const nlohmann::json &j, ChannelSend &p)
{
  from_json (j, static_cast<dsp::ProcessorBase &> (p));
  j.at (ChannelSend::kSignalTypeKey).get_to (p.signal_type_);
  j.at (ChannelSend::kIsPrefaderKey).get_to (p.is_prefader_);
}
}
