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
    size_t                                   num_ports = 1);

  ~MidiPassthroughProcessor () override;

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
    size_t                                   num_channels);

  ~AudioPassthroughProcessor () override;

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
    ProcessorBase::ProcessorBaseDependencies dependencies);

  ~StereoPassthroughProcessor () override;
};

} // namespace zrythm::dsp
