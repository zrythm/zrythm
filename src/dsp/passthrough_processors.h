// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/processor_base.h"

namespace zrythm::dsp
{
/**
 * @brief Processor that processes MIDI signals (passthrough by default).
 */
class MidiPassthroughProcessor : public ProcessorBase
{
public:
  MidiPassthroughProcessor (
    ProcessorBase::ProcessorBaseDependencies dependencies,
    size_t                                   num_ports = 1)
      : ProcessorBase (dependencies)
  {
    set_name (u8"MIDI Passthrough");
    for (const auto i : std::views::iota (0u, num_ports))
      {
        const utils::Utf8String index_str =
          num_ports == 1
            ? u8""
            : (utils::Utf8String (u8" ")
               + utils::Utf8String::from_utf8_encoded_string (
                 std::to_string (i + 1)));
        add_input_port (dependencies.port_registry_.create_object<MidiPort> (
          get_node_name () + u8" In" + index_str, PortFlow::Input));
        add_output_port (dependencies.port_registry_.create_object<MidiPort> (
          get_node_name () + u8" Out" + index_str, PortFlow::Output));
      }
  }

  ~MidiPassthroughProcessor () override = default;

  auto get_midi_in_port (size_t index) -> dsp::MidiPort &
  {
    return *get_input_ports ().at (index).get_object_as<dsp::MidiPort> ();
  }
  auto get_midi_out_port (size_t index) -> dsp::MidiPort &
  {
    return *get_output_ports ().at (index).get_object_as<dsp::MidiPort> ();
  }
};

/**
 * @brief Processor that passes through stereo audio signals.
 */
class AudioPassthroughProcessor : public ProcessorBase
{
public:
  AudioPassthroughProcessor (
    ProcessorBase::ProcessorBaseDependencies dependencies,
    AudioPort::BusLayout                     bus_layout,
    size_t                                   num_channels)
      : ProcessorBase (dependencies)
  {
    set_name (u8"Audio Passthrough");
    add_input_port (dependencies.port_registry_.create_object<AudioPort> (
      get_node_name () + u8" In", PortFlow::Input, bus_layout, num_channels));
    add_output_port (dependencies.port_registry_.create_object<AudioPort> (
      get_node_name () + u8" Out", PortFlow::Output, bus_layout, num_channels));
  }

  ~AudioPassthroughProcessor () override = default;

  auto get_audio_in_port () -> dsp::AudioPort &
  {
    return *get_input_ports ().at (0).get_object_as<dsp::AudioPort> ();
  }
  auto get_audio_out_port () -> dsp::AudioPort &
  {
    return *get_output_ports ().at (0).get_object_as<dsp::AudioPort> ();
  }
};

class StereoPassthroughProcessor : public AudioPassthroughProcessor
{
public:
  StereoPassthroughProcessor (
    ProcessorBase::ProcessorBaseDependencies dependencies)
      : AudioPassthroughProcessor (dependencies, AudioPort::BusLayout::Stereo, 2)
  {
  }

  ~StereoPassthroughProcessor () override = default;
};

} // namespace zrythm::dsp
