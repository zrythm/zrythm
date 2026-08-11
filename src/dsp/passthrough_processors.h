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
    utils::IObjectRegistry &registry,
    size_t                  num_ports = 1,
    QObject *               parent = nullptr);

  ~MidiPassthroughProcessor () override;

  auto get_midi_in_port (size_t index) -> dsp::MidiPort &
  {
    return *get_all_input_ports ().at (index).get_object_as<dsp::MidiPort> ();
  }
  auto get_midi_out_port (size_t index) -> dsp::MidiPort &
  {
    return *get_all_output_ports ().at (index).get_object_as<dsp::MidiPort> ();
  }
};

/**
 * @brief Processor that passes through stereo audio signals.
 */
class AudioPassthroughProcessor : public ProcessorBase
{
public:
  AudioPassthroughProcessor (
    utils::IObjectRegistry &registry,
    SpeakerArrangement      arrangement,
    QObject *               parent = nullptr);

  ~AudioPassthroughProcessor () override;

  auto get_audio_in_port () -> dsp::AudioPort &
  {
    return *get_all_input_ports ().at (0).get_object_as<dsp::AudioPort> ();
  }
  auto get_audio_out_port () -> dsp::AudioPort &
  {
    return *get_all_output_ports ().at (0).get_object_as<dsp::AudioPort> ();
  }
};

class StereoPassthroughProcessor : public AudioPassthroughProcessor
{
public:
  StereoPassthroughProcessor (
    utils::IObjectRegistry &registry,
    QObject *               parent = nullptr);

  ~StereoPassthroughProcessor () override;
};

} // namespace zrythm::dsp
