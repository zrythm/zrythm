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
      fmt::format (
        "{} Send {}", is_prefader_ ? "Pre-Fader" : "Post-Fader", slot + 1)));
}

void
ChannelSend::custom_process_block (const EngineProcessTimeInfo time_nfo) noexcept
{
  const auto local_offset = time_nfo.local_offset_;
  const auto nframes = time_nfo.nframes_;
  if (is_audio ())
    {
      const auto &amount_param = processing_caches_->amount_param_;
      const auto  amount_val =
        amount_param->range ().convertFrom0To1 (amount_param->currentValue ());
      for (
        const auto &[out, in] : std::views::zip (
          processing_caches_->audio_outs_rt_, processing_caches_->audio_ins_rt_))
        {
          if (utils::math::floats_near (amount_val, 1.f, 0.00001f))
            {
              utils::float_ranges::copy (
                &out->buf_[local_offset], &in->buf_[local_offset], nframes);
            }
          else
            {
              utils::float_ranges::product (
                &out->buf_[local_offset], &in->buf_[local_offset], amount_val,
                nframes);
            }
        }
    }
  else if (is_midi ())
    {
      processing_caches_->midi_out_rt_->midi_events_.queued_events_.append (
        processing_caches_->midi_in_rt_->midi_events_.active_events_,
        local_offset, nframes);
    }
}

void
ChannelSend::custom_prepare_for_processing (
  sample_rate_t sample_rate,
  nframes_t     max_block_length)
{
  processing_caches_ = std::make_unique<ChannelSendProcessingCaches> ();

  processing_caches_->amount_param_ = amountParam ();
  if (is_midi ())
    {
      processing_caches_->midi_in_rt_ = &get_midi_in_port ();
      processing_caches_->midi_out_rt_ = &get_midi_out_port ();
    }
  else if (is_audio ())
    {
      const auto stereo_in = get_stereo_in_ports ();
      processing_caches_->audio_ins_rt_.push_back (&stereo_in.first);
      processing_caches_->audio_ins_rt_.push_back (&stereo_in.second);
      const auto stereo_out = get_stereo_out_ports ();
      processing_caches_->audio_outs_rt_.push_back (&stereo_out.first);
      processing_caches_->audio_outs_rt_.push_back (&stereo_out.second);
    }
}

void
ChannelSend::custom_release_resources ()
{
  processing_caches_.reset ();
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
