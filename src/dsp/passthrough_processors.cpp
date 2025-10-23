// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/passthrough_processors.h"

namespace zrythm::dsp
{

MidiPassthroughProcessor::MidiPassthroughProcessor (
  ProcessorBase::ProcessorBaseDependencies dependencies,
  size_t                                   num_ports)
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

MidiPassthroughProcessor::~MidiPassthroughProcessor () = default;

AudioPassthroughProcessor::AudioPassthroughProcessor (
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

AudioPassthroughProcessor::~AudioPassthroughProcessor () = default;

StereoPassthroughProcessor::StereoPassthroughProcessor (
  ProcessorBase::ProcessorBaseDependencies dependencies)
    : AudioPassthroughProcessor (dependencies, AudioPort::BusLayout::Stereo, 2)
{
}

StereoPassthroughProcessor::~StereoPassthroughProcessor () = default;

} // namespace zrythm::dsp
