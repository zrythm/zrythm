// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/port.h"
#include "utils/icloneable.h"
#include "utils/monotonic_time_provider.h"
#include "utils/ring_buffer.h"

#include <nlohmann/json.hpp>

namespace zrythm::dsp
{

/**
 * @brief Audio port specifics.
 */
class AudioPort final
    : public QObject,
      public Port,
      public RingBufferOwningPortMixin,
      public PortConnectionsCacheMixin<AudioPort>,
      private utils::QElapsedTimeProvider
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  /**
   * @brief Description of the channel layout of this port.
   */
  enum class BusLayout : uint8_t
  {
    Unknown,

    // Implies a single channel
    Mono,

    // Implies 2 channels: L + R
    Stereo,

    // Unimplemented
    Surround,
    Ambisonic,
  };

  /**
   * @brief Purpose of this port.
   */
  enum class Purpose : uint8_t
  {
    Main,
    Sidechain,
  };

  AudioPort (
    utils::Utf8String label,
    PortFlow          flow,
    BusLayout         layout,
    uint8_t           num_channels,
    Purpose           purpose = Purpose::Main);

  static constexpr size_t AUDIO_RING_SIZE = 65536;

  [[gnu::hot]] void
  process_block (EngineProcessTimeInfo time_nfo) noexcept override;

  void clear_buffer (std::size_t offset, std::size_t nframes) override;

  [[nodiscard]] auto  layout () const { return layout_; }
  [[nodiscard]] auto  purpose () const { return purpose_; }
  [[nodiscard]] auto &buffers () const { return buf_; }
  [[nodiscard]] auto &audio_ring_buffers () const { return audio_ring_; }
  auto                num_channels () const { return num_channels_; }

  void mark_as_requires_limiting () { requires_limiting_ = true; }
  auto requires_limiting () const { return requires_limiting_; }

  /**
   * @brief Adds the contents of @p src to this port.
   */
  void add_source_rt (
    const AudioPort      &src,
    EngineProcessTimeInfo time_nfo,
    float                 multiplier = 1.f);

  void copy_source_rt (
    const AudioPort      &src,
    EngineProcessTimeInfo time_nfo,
    float                 multiplier = 1.f);

  friend void init_from (
    AudioPort             &obj,
    const AudioPort       &other,
    utils::ObjectCloneType clone_type);

  void
  prepare_for_processing (sample_rate_t sample_rate, nframes_t max_block_length)
    override;
  void release_resources () override;

private:
  static constexpr auto kBusLayoutId = "busLayout"sv;
  static constexpr auto kPurposeId = "purpose"sv;
  static constexpr auto kRequiresLimitingId = "requiresLimiting"sv;
  friend void           to_json (nlohmann::json &j, const AudioPort &port)
  {
    to_json (j, static_cast<const Port &> (port));
    j[kBusLayoutId] = port.layout_;
    j[kPurposeId] = port.purpose_;
    j[kRequiresLimitingId] = port.requires_limiting_;
  }
  friend void from_json (const nlohmann::json &j, AudioPort &port)
  {
    from_json (j, static_cast<Port &> (port));
    j.at (kBusLayoutId).get_to (port.layout_);
    j.at (kPurposeId).get_to (port.purpose_);
    j.at (kRequiresLimitingId).get_to (port.requires_limiting_);
  }

private:
  BusLayout layout_{};
  Purpose   purpose_{};

  uint8_t num_channels_{};

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

  /**
   * Ring buffer(s) for saving the contents of the audio buffer to be used in
   * the UI instead of directly accessing the buffer.
   *
   * 1 ring buffer per channel.
   *
   * This should contain blocks of block_length samples and should maintain at
   * least 10 cycles' worth of buffers.
   */
  std::vector<RingBuffer<float>> audio_ring_;

  BOOST_DESCRIBE_CLASS (
    AudioPort,
    (Port),
    (),
    (),
    (layout_, purpose_, requires_limiting_))
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
