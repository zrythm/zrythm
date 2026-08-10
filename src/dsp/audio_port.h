// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/audio_bus_channel_routing.h"
#include "dsp/port.h"
#include "dsp/speaker_arrangement.h"
#include "utils/icloneable.h"
#include "utils/monotonic_time_provider.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <nlohmann/json_fwd.hpp>

namespace zrythm::dsp
{

class AudioPort final
    : public Port,
      public PortConnectionsCacheMixin<AudioPort>,
      private utils::QElapsedTimeProvider
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  /**
   * @brief Purpose of this port.
   */
  enum class Purpose : uint8_t
  {
    Main,
    Sidechain,
  };

  AudioPort (
    utils::Utf8String  label,
    PortFlow           flow,
    SpeakerArrangement arrangement,
    Purpose            purpose = Purpose::Main);

  [[gnu::hot]] void process_block (
    dsp::graph::ProcessBlockInfo time_nfo,
    const dsp::ITransport       &transport,
    const dsp::TempoMap         &tempo_map) noexcept override;

  void clear_buffer (std::size_t offset, std::size_t nframes) override;

  [[nodiscard]] auto  arrangement () const { return arrangement_; }
  [[nodiscard]] auto  purpose () const { return purpose_; }
  [[nodiscard]] auto &buffers () const { return buf_; }
  auto num_channels () const { return arrangement_.channel_count (); }

  void mark_as_requires_limiting () { requires_limiting_ = true; }
  auto requires_limiting () const { return requires_limiting_; }

  /**
   * @brief Adds the contents of @p src to this port, leaving destination
   * channels that @p routing does not feed as they are.
   */
  void add_source_rt (
    const AudioPort              &src,
    const AudioBusChannelRouting &routing,
    dsp::graph::ProcessBlockInfo  time_nfo,
    float                         multiplier = 1.f);

  /** Adds the contents of @p src using routing derived from the arrangements. */
  void add_source_rt (
    const AudioPort             &src,
    dsp::graph::ProcessBlockInfo time_nfo,
    float                        multiplier = 1.f)
  {
    add_source_rt (src, AudioBusChannelRouting{}, time_nfo, multiplier);
  }

  /**
   * @brief Replaces the contents of this port with @p src, clearing
   * destination channels that @p routing does not feed.
   */
  void copy_source_rt (
    const AudioPort              &src,
    const AudioBusChannelRouting &routing,
    dsp::graph::ProcessBlockInfo  time_nfo,
    float                         multiplier = 1.f);

  /** Replaces the contents using routing derived from the arrangements. */
  void copy_source_rt (
    const AudioPort             &src,
    dsp::graph::ProcessBlockInfo time_nfo,
    float                        multiplier = 1.f)
  {
    copy_source_rt (src, AudioBusChannelRouting{}, time_nfo, multiplier);
  }

  friend void init_from (
    AudioPort             &obj,
    const AudioPort       &other,
    utils::ObjectCloneType clone_type);

  void prepare_for_processing_impl (
    const graph::GraphNode * node,
    units::sample_rate_t     sample_rate,
    units::sample_u32_t      max_block_length) override;
  void release_resources () override;

private:
  static constexpr auto kSpeakerArrangementId = "speakerArrangement"sv;
  static constexpr auto kPurposeId = "purpose"sv;
  static constexpr auto kRequiresLimitingId = "requiresLimiting"sv;
  friend void           to_json (nlohmann::json &j, const AudioPort &port);
  friend void           from_json (const nlohmann::json &j, AudioPort &port);

private:
  SpeakerArrangement arrangement_;
  Purpose            purpose_{};

  /**
   * @brief Whether to clip the port's data to the range [-2, 2] (3dB).
   *
   * Limiting wastes around 50% of port processing CPU cycles so this is only
   * enabled if requested.
   */
  bool requires_limiting_{};

  /**
   * Audio data buffer(s).
   */
  std::unique_ptr<juce::AudioSampleBuffer> buf_;

  BOOST_DESCRIBE_CLASS (
    AudioPort,
    (Port),
    (),
    (),
    (arrangement_, purpose_, requires_limiting_))
};

/**
 * Convenience factory for L/R audio port pairs.
 */
class StereoPorts final
{
public:
  static std::pair<utils::Utf8String, utils::Utf8String> get_name_and_symbols (
    bool              left,
    utils::Utf8String name,
    utils::Utf8String symbol)
  {
    return std::make_pair (
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("{} {}", name, left ? "L" : "R")),
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("{}_{}", symbol, left ? "l" : "r")));
  }
};

} // namespace zrythm::dsp
