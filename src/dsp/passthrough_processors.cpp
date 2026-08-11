// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/passthrough_processors.h"
#include "utils/registry_utils.h"

namespace zrythm::dsp
{

MidiPassthroughProcessor::MidiPassthroughProcessor (
  utils::IObjectRegistry &registry,
  size_t                  num_ports,
  QObject *               parent)
    : ProcessorBase (registry, u8"MidiPassthroughProcessor", parent)
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
      add_input_port (
        utils::create_object<MidiPort> (
          registry, get_node_name () + u8" In" + index_str, PortFlow::Input));
      add_output_port (
        utils::create_object<MidiPort> (
          registry, get_node_name () + u8" Out" + index_str, PortFlow::Output));
    }
}

MidiPassthroughProcessor::~MidiPassthroughProcessor () = default;

AudioPassthroughProcessor::AudioPassthroughProcessor (
  utils::IObjectRegistry &registry,
  SpeakerArrangement      arrangement,
  QObject *               parent)
    : ProcessorBase (registry, u8"AudioPassthroughProcessor", parent)
{
  set_name (u8"Audio Passthrough");
  add_input_port (
    utils::create_object<AudioPort> (
      registry, get_node_name () + u8" In", PortFlow::Input, arrangement));
  add_output_port (
    utils::create_object<AudioPort> (
      registry, get_node_name () + u8" Out", PortFlow::Output, arrangement));
}

AudioPassthroughProcessor::~AudioPassthroughProcessor () = default;

StereoPassthroughProcessor::StereoPassthroughProcessor (
  utils::IObjectRegistry &registry,
  QObject *               parent)
    : AudioPassthroughProcessor (registry, SpeakerArrangement::stereo (), parent)
{
}

StereoPassthroughProcessor::~StereoPassthroughProcessor () = default;

} // namespace zrythm::dsp
