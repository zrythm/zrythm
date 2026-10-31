// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/midi_event.h"
#include "dsp/midi_port.h"
#include "dsp/port.h"
#include "plugins/internal_plugin_base.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"
#include "utils/views.h"

#include <fmt/format.h>

namespace zrythm::structure::tracks
{

/**
 * @brief MIDI FX test double that passes incoming notes through and appends
 * a note absent from its input.
 */
class NoteGeneratorPlugin : public plugins::InternalPluginBase
{
  Q_OBJECT

public:
  NoteGeneratorPlugin (
    utils::IObjectRegistry &registry,
    midi_byte_t             generated_pitch)
      : plugins::InternalPluginBase (registry), generated_pitch_ (generated_pitch)
  {
    auto descriptor = std::make_unique<plugins::PluginDescriptor> ();
    descriptor->name_ = u8"Note Generator";
    descriptor->protocol_ = plugins::Protocol::ProtocolType::Internal;
    plugins::PluginConfiguration config;
    config.descr_ = std::move (descriptor);
    set_configuration (config);

    add_input_port (
      utils::create_object<dsp::MidiPort> (
        registry, u8"MIDI In", dsp::PortFlow::Input));
    add_output_port (
      utils::create_object<dsp::MidiPort> (
        registry, u8"MIDI Out", dsp::PortFlow::Output));
  }

private:
  void process_impl (
    dsp::graph::ProcessBlockInfo time_nfo,
    const dsp::ITransport &,
    const dsp::TempoMap &) noexcept override
  {
    auto &in_port =
      *get_all_input_ports ().front ().get_object_as<dsp::MidiPort> ();
    auto &out_port =
      *get_all_output_ports ().front ().get_object_as<dsp::MidiPort> ();
    dsp::midi_event::append_in_range (
      out_port.buffer_, in_port.buffer_,
      std::pair{
        time_nfo.buffer_offset_, time_nfo.buffer_offset_ + time_nfo.nframes_ });
    const auto generated_ev = dsp::midi_event::make_note_on (
      0, generated_pitch_, 100, units::samples (0));
    out_port.buffer_.push_back (generated_ev.time_, generated_ev.data ());
  }

  midi_byte_t generated_pitch_;
};

/**
 * @brief Instrument test double that consumes MIDI and produces silence on
 * two audio outputs.
 */
class InstrumentSinkPlugin : public plugins::InternalPluginBase
{
  Q_OBJECT

public:
  InstrumentSinkPlugin (utils::IObjectRegistry &registry)
      : plugins::InternalPluginBase (registry)
  {
    auto descriptor = std::make_unique<plugins::PluginDescriptor> ();
    descriptor->name_ = u8"Instrument Sink";
    descriptor->protocol_ = plugins::Protocol::ProtocolType::Internal;
    plugins::PluginConfiguration config;
    config.descr_ = std::move (descriptor);
    set_configuration (config);

    add_input_port (
      utils::create_object<dsp::MidiPort> (
        registry, u8"MIDI In", dsp::PortFlow::Input));
    for (const auto channel_index : std::views::iota (0, 2))
      {
        add_output_port (
          utils::create_object<dsp::AudioPort> (
            registry,
            utils::Utf8String::from_utf8_encoded_string (
              fmt::format ("Audio Out {}", channel_index + 1)),
            dsp::PortFlow::Output, dsp::SpeakerArrangement::mono ()));
      }
  }
};

} // namespace zrythm::structure::tracks
