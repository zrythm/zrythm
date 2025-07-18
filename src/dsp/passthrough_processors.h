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
    return *input_ports_.at (index).get_object_as<dsp::MidiPort> ();
  }
  auto get_midi_out_port (size_t index) -> dsp::MidiPort &
  {
    return *output_ports_.at (index).get_object_as<dsp::MidiPort> ();
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
    size_t                                   num_ports)
      : ProcessorBase (dependencies)
  {
    set_name (u8"Audio Passthrough");
    for (const auto i : std::views::iota (0u, num_ports))
      {
        const utils::Utf8String index_str =
          num_ports == 1
            ? u8""
            : utils::Utf8String::from_utf8_encoded_string (std::to_string (i + 1));
        add_input_port (dependencies.port_registry_.create_object<AudioPort> (
          get_node_name () + u8" In " + index_str, PortFlow::Input));
        add_output_port (dependencies.port_registry_.create_object<AudioPort> (
          get_node_name () + u8" Out " + index_str, PortFlow::Output));
      }
  }

  ~AudioPassthroughProcessor () override = default;

  auto get_audio_in_port (size_t index) -> dsp::AudioPort &
  {
    return *input_ports_.at (index).get_object_as<dsp::AudioPort> ();
  }
  auto get_audio_out_port (size_t index) -> dsp::AudioPort &
  {
    return *output_ports_.at (index).get_object_as<dsp::AudioPort> ();
  }
  auto
  get_first_stereo_in_pair () -> std::pair<dsp::AudioPort &, dsp::AudioPort &>
  {
    return { get_audio_in_port (0), get_audio_in_port (1) };
  }
  auto
  get_first_stereo_out_pair () -> std::pair<dsp::AudioPort &, dsp::AudioPort &>
  {
    return { get_audio_out_port (0), get_audio_out_port (1) };
  }
};

class StereoPassthroughProcessor : public AudioPassthroughProcessor
{
public:
  StereoPassthroughProcessor (
    ProcessorBase::ProcessorBaseDependencies dependencies)
      : AudioPassthroughProcessor (dependencies, 2)
  {
    set_name (u8"Stereo Passthrough");
    input_ports_.clear ();
    output_ports_.clear ();
    auto stereo_in_ports = dsp::StereoPorts::create_stereo_ports (
      dependencies.port_registry_, true, get_node_name () + u8" In",
      u8"stereo_passthrough_in");
    auto stereo_out_ports = dsp::StereoPorts::create_stereo_ports (
      dependencies.port_registry_, false, get_node_name () + u8" Out",
      u8"stereo_passthrough_out");
    add_input_port (stereo_in_ports.first);
    add_input_port (stereo_in_ports.second);
    add_output_port (stereo_out_ports.first);
    add_output_port (stereo_out_ports.second);
  }

  ~StereoPassthroughProcessor () override = default;
};

} // namespace zrythm::dsp
