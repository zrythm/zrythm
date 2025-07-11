// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/graph_node.h"
#include "dsp/port_all.h"

#include <QObject>

namespace zrythm::dsp
{

/**
 * @brief A base class for processors that process signals of the same type as
 * inputs and outputs.
 *
 * This class does passthrough processing by default and supports all port types.
 */
class SameSignalProcessorBase : public dsp::graph::IProcessable
{
public:
  SameSignalProcessorBase (dsp::PortRegistry &port_registry)
      : port_registry_ (port_registry)
  {
  }

  ~SameSignalProcessorBase () override = default;

  /**
   * @brief Set a custom name to be used in the DSP graph.
   */
  void set_name (const utils::Utf8String &name);

  /**
   * @brief Adds a pair of input/output ports.
   */
  void add_port_pair (
    std::pair<dsp::PortUuidReference, dsp::PortUuidReference> port_pair);

  utils::Utf8String get_node_name () const override { return name_; }
  void              process_block (EngineProcessTimeInfo time_nfo) override;
  void
  prepare_for_processing (sample_rate_t sample_rate, nframes_t max_block_length)
    override;
  void release_resources () override;

private:
  static constexpr auto kNameKey = "name"sv;
  static constexpr auto kInputsKey = "inputs"sv;
  static constexpr auto kOutputsKey = "outputs"sv;
  friend void to_json (nlohmann::json &j, const SameSignalProcessorBase &p)
  {
    j[kNameKey] = p.name_;
    j[kInputsKey] = p.input_ports_;
    j[kOutputsKey] = p.output_ports_;
  }
  friend void from_json (const nlohmann::json &j, SameSignalProcessorBase &p)
  {
    j.at (kNameKey).get_to (p.name_);
    p.input_ports_.clear ();
    for (const auto &input_port : j.at (kInputsKey))
      {
        auto port_ref = dsp::PortUuidReference{ p.port_registry_ };
        from_json (input_port, port_ref);
        p.input_ports_.emplace_back (std::move (port_ref));
      }
    p.output_ports_.clear ();
    for (const auto &output_port : j.at (kOutputsKey))
      {
        auto port_ref = dsp::PortUuidReference{ p.port_registry_ };
        from_json (output_port, port_ref);
        p.output_ports_.emplace_back (std::move (port_ref));
      }
  }

protected:
  dsp::PortRegistry                  &port_registry_;
  utils::Utf8String                   name_{ u8"Same Signal Passthrough" };
  std::vector<dsp::PortUuidReference> input_ports_;
  std::vector<dsp::PortUuidReference> output_ports_;
};

/**
 * @brief Processor that processes MIDI signals (passthrough by default).
 */
class MidiPassthroughProcessor : public SameSignalProcessorBase
{
public:
  MidiPassthroughProcessor (
    dsp::PortRegistry &port_registry,
    size_t             num_ports = 1)
      : SameSignalProcessorBase (port_registry)
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
        auto input_port = port_registry_.create_object<MidiPort> (
          get_node_name () + u8" In" + index_str, PortFlow::Input);
        auto output_port = port_registry_.create_object<MidiPort> (
          get_node_name () + u8" Out" + index_str, PortFlow::Output);
        add_port_pair ({ input_port, output_port });
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

private:
  friend void to_json (nlohmann::json &j, const MidiPassthroughProcessor &p)
  {
    to_json (j, static_cast<const SameSignalProcessorBase &> (p));
  }
  friend void from_json (const nlohmann::json &j, MidiPassthroughProcessor &p)
  {
    from_json (j, static_cast<SameSignalProcessorBase &> (p));
  }
};

/**
 * @brief Processor that passes through stereo audio signals.
 */
class AudioPassthroughProcessor : public SameSignalProcessorBase
{
public:
  AudioPassthroughProcessor (dsp::PortRegistry &port_registry, size_t num_ports)
      : SameSignalProcessorBase (port_registry)
  {
    set_name (u8"Audio Passthrough");
    for (const auto i : std::views::iota (0u, num_ports))
      {
        const utils::Utf8String index_str =
          num_ports == 1
            ? u8""
            : utils::Utf8String::from_utf8_encoded_string (std::to_string (i + 1));
        auto input_port = port_registry_.create_object<AudioPort> (
          get_node_name () + u8" In " + index_str, PortFlow::Input);
        auto output_port = port_registry_.create_object<AudioPort> (
          get_node_name () + u8" Out " + index_str, PortFlow::Output);
        add_port_pair ({ input_port, output_port });
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

private:
  friend void to_json (nlohmann::json &j, const AudioPassthroughProcessor &p)
  {
    to_json (j, static_cast<const SameSignalProcessorBase &> (p));
  }
  friend void from_json (const nlohmann::json &j, AudioPassthroughProcessor &p)
  {
    from_json (j, static_cast<SameSignalProcessorBase &> (p));
  }
};

class StereoPassthroughProcessor : public AudioPassthroughProcessor
{
public:
  StereoPassthroughProcessor (dsp::PortRegistry &port_registry)
      : AudioPassthroughProcessor (port_registry, 2)
  {
    set_name (u8"Stereo Passthrough");
    input_ports_.clear ();
    output_ports_.clear ();
    auto stereo_in_ports = dsp::StereoPorts::create_stereo_ports (
      port_registry_, true, get_node_name () + u8" In",
      u8"stereo_passthrough_in");
    auto stereo_out_ports = dsp::StereoPorts::create_stereo_ports (
      port_registry_, false, get_node_name () + u8" Out",
      u8"stereo_passthrough_out");
    add_port_pair ({ stereo_in_ports.first, stereo_out_ports.first });
    add_port_pair ({ stereo_in_ports.second, stereo_out_ports.second });
  }

  ~StereoPassthroughProcessor () override = default;
};
} // namespace zrythm::dsp
