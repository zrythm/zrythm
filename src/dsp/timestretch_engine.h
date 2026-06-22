// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>

#include "dsp/time_warp_map.h"
#include "utils/audio.h"
#include "utils/units.h"

namespace zrythm::dsp
{

/**
 * @brief Options governing a stretch operation.
 */
struct StretchOptions
{
  /** How pitch is handled while time-stretching. */
  enum class PitchMode : std::uint8_t
  {
    /** Preserve pitch (phase-vocoder time-stretch). */
    Preserve,
    /** Re-pitch: varispeed resampling, so pitch follows tempo (tape-style). */
    Repitch,
  };

  PitchMode pitch_mode{ PitchMode::Preserve };
  /** Preserve formants (only meaningful when @ref pitch_mode is Preserve). */
  bool preserve_formants{ false };
};

/**
 * @brief Abstract time-stretch engine.
 *
 * Implementations consume a sample-domain TimeWarpMap and produce a stretched
 * buffer. The engine has no knowledge of tempo maps or warp markers — only the
 * composed source→output mapping — which keeps auto-timestretch and (future)
 * manual warp points sharing the same engine. Add new engines by implementing
 * this interface and adding a case to @ref create_timestretch_engine; callers
 * never name a concrete engine type.
 */
class ITimeStretchEngine
{
public:
  virtual ~ITimeStretchEngine () = default;

  /**
   * @brief A stable, non-translatable identifier for persistence (settings).
   */
  [[nodiscard]] virtual std::string_view id () const = 0;

  /**
   * @brief Whether this engine supports the given pitch mode.
   */
  [[nodiscard]] virtual bool
  supports (StretchOptions::PitchMode mode) const = 0;

  /**
   * @brief Stretch @p input according to @p warp (Non-Virtual Interface).
   *
   * Validates the preconditions below, then dispatches to @ref stretch_impl.
   *
   * @pre @p warp.is_valid()
   * @pre @p warp.source_length == input.getNumSamples()
   * @pre supports(@p options.pitch_mode)
   * @return A new buffer with exactly @c warp.output_length frames.
   *
   * @throws std::invalid_argument if any precondition is violated.
   */
  [[nodiscard]] utils::audio::AudioBuffer stretch (
    const utils::audio::AudioBuffer &input,
    const TimeWarpMap               &warp,
    const StretchOptions            &options) [[clang::blocking]]
  {
    if (!warp.is_valid ())
      throw std::invalid_argument ("TimeWarpMap is not valid");
    if (warp.source_length != units::samples (input.getNumSamples ()))
      throw std::invalid_argument (
        "TimeWarpMap source_length does not match input frame count");
    if (!supports (options.pitch_mode))
      throw std::invalid_argument (
        "engine does not support the requested pitch mode");
    return stretch_impl (input, warp, options);
  }

private:
  /**
   * @brief Engine-specific stretch implementation (overrides provide this).
   *
   * Called only after @ref stretch has validated preconditions. Implementations
   * are blocking (they allocate and run offline DSP) and must never run on the
   * audio thread.
   */
  [[nodiscard]] virtual utils::audio::AudioBuffer stretch_impl (
    const utils::audio::AudioBuffer &input,
    const TimeWarpMap               &warp,
    const StretchOptions            &options) [[clang::blocking]] = 0;
};

/** Compile-time-known engine identifiers (extend when adding engines). */
enum class TimeStretchEngineId : std::uint8_t
{
  RubberBand,
  Resampler, ///< Reserved for a future varispeed/repitch engine.
};

/** Metadata describing an engine, for the settings UI. */
struct TimeStretchEngineInfo
{
  TimeStretchEngineId id;
  /** Non-translatable identifier (matches @ref ITimeStretchEngine::id). */
  std::string_view key;
  /** Translation context string for the display name. */
  std::string_view display_name;
  bool             supports_pitch_preserve;
};

/**
 * @brief All engines available at compile time.
 *
 * @return A span over static storage; safe to retain.
 */
[[nodiscard]] std::span<const TimeStretchEngineInfo>
available_timestretch_engines () noexcept;

/**
 * @brief Create an engine by id, configured for the given sample rate.
 *
 * The returned engine reads its channel count from each input buffer passed to
 * stretch(), so it can be reused across buffers of differing channel counts.
 *
 * @throws std::invalid_argument if @p id is unknown or unimplemented.
 */
[[nodiscard]] std::unique_ptr<ITimeStretchEngine>
create_timestretch_engine (
  TimeStretchEngineId  id,
  units::sample_rate_t sample_rate);

/**
 * @brief Create the first available engine supporting @p options' pitch mode.
 *
 * @throws std::runtime_error if no engine supports the requested mode.
 */
[[nodiscard]] std::unique_ptr<ITimeStretchEngine>
create_default_timestretch_engine (
  const StretchOptions &options,
  units::sample_rate_t  sample_rate);

} // namespace zrythm::dsp
