// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <limits>

#include "utils/format.h"
#include "utils/types.h"
#include "utils/units.h"

#include <fmt/format.h>
#include <nlohmann/json_fwd.hpp>

namespace zrythm::dsp
{

namespace internal
{
static_assert (
  std::numeric_limits<double>::is_iec559,
  "Requires IEEE-754 doubles");
static_assert (
  std::atomic<uint64_t>::is_always_lock_free,
  "64-bit atomics not lock-free");

/**
 * @brief Stores a double and a bool in an atomic uint64_t.
 */
class AtomicDoubleWithBool
{
  std::atomic<uint64_t> packed_;

  // Mask to clear LSB (where we store the bool)
  static constexpr uint64_t kValueMask = ~1ULL;

public:
  AtomicDoubleWithBool () noexcept = default;
  AtomicDoubleWithBool (double d, bool b) noexcept { store (d, b); }

  void store (double d, bool b) noexcept
  {
    assert (std::isfinite (d) && "Only finite doubles supported");
    const auto bits = std::bit_cast<uint64_t> (d);
    packed_.store ((bits & kValueMask) | (b ? 1 : 0), std::memory_order_release);
  }

  std::pair<double, bool> load () const noexcept
  {
    const uint64_t bits = packed_.load (std::memory_order_acquire);
    return {
      std::bit_cast<double> (bits & kValueMask), // Original value ≈ d
      static_cast<bool> (bits & 1)               // Stored bool
    };
  }
};
} // namespace internal

/**
 * @class AtomicPosition
 * @brief Thread-safe position storage with automatic musical/absolute time
 * conversion.
 *
 * Maintains a position value that can be stored in either:
 * - Musical time (ticks)
 * - Absolute time (seconds)
 *
 * Automatically converts between formats when the storage mode changes or when
 * accessing in a different format than currently stored. Relies on a TempoMap
 * for conversions between musical and absolute time domains.
 */
class AtomicPosition
{
public:
  struct TimeConversionFunctions
  {
    std::function<units::precise_second_t (units::precise_tick_t)>
      tick_to_seconds;
    std::function<units::precise_tick_t (units::precise_second_t)>
      seconds_to_tick;
    std::function<units::precise_sample_t (units::precise_tick_t)>
      tick_to_samples;
    std::function<units::precise_tick_t (units::precise_sample_t)>
      samples_to_tick;
  };

  /**
   * @brief Construct a new AtomicPosition object.
   * @param tempo_map Reference to tempo map for time conversions.
   */
  AtomicPosition (const TimeConversionFunctions &conversion_funcs) noexcept
      : conversion_funcs_ (conversion_funcs),
        value_ (0, format_to_bool (TimeFormat::Musical))
  {
  }

  /// @brief Get current storage format (musical ticks or absolute seconds).
  auto get_current_mode () const
  {
    return bool_to_format (value_.load ().second);
  }

  /**
   * @brief Change storage format with automatic value conversion.
   * @param format New storage format (TimeFormat::Musical or
   * TimeFormat::Absolute)
   *
   * Converts the stored value to match the new format:
   * - Musical -> Absolute: Converts ticks to seconds
   * - Absolute -> Musical: Converts seconds to ticks
   *
   * @warning This is NOT thread-safe. Ensure that no other thread is writing to
   * the position while this function is called.
   */
  void set_mode (TimeFormat format)
  {
    const auto cur_mode = get_current_mode ();
    if (cur_mode == TimeFormat::Absolute && format == TimeFormat::Musical)
      {
        value_.store (
          get_ticks ().in (units::ticks), format_to_bool (TimeFormat::Musical));
      }
    else if (cur_mode == TimeFormat::Musical && format == TimeFormat::Absolute)
      {
        value_.store (
          get_seconds ().in (units::seconds),
          format_to_bool (TimeFormat::Absolute));
      }
  }

  /**
   * @brief Set position in musical ticks.
   *
   * Converts to seconds if currently in Absolute mode.
   */
  void set_ticks (units::precise_tick_t ticks)
  {
    const auto cur_mode = get_current_mode ();
    if (cur_mode == TimeFormat::Musical)
      {
        value_.store (
          ticks.in (units::ticks), format_to_bool (TimeFormat::Musical));
      }
    else if (cur_mode == TimeFormat::Absolute)
      {
        set_seconds (conversion_funcs_.tick_to_seconds (ticks));
      }
  }

  /**
   * @brief Set position in absolute seconds.
   *
   * Converts to ticks if currently in Musical mode.
   */
  void set_seconds (units::precise_second_t seconds)
  {
    const auto cur_mode = get_current_mode ();
    if (cur_mode == TimeFormat::Absolute)
      {
        value_.store (
          seconds.in (units::seconds), format_to_bool (TimeFormat::Absolute));
      }
    else if (cur_mode == TimeFormat::Musical)
      {
        set_ticks (conversion_funcs_.seconds_to_tick (seconds));
      }
  }

  /// @brief Get position in musical ticks (converts if necessary).
  units::precise_tick_t get_ticks () const
  {
    const auto &[d, b] = value_.load ();
    if (bool_to_format (b) == TimeFormat::Musical)
      {
        return units::ticks (d);
      }
    return conversion_funcs_.seconds_to_tick (units::seconds (d));
  }

  /// @brief Get position in absolute seconds (converts if necessary).
  units::precise_second_t get_seconds () const
  {
    const auto &[d, b] = value_.load ();
    if (bool_to_format (b) == TimeFormat::Absolute)
      {
        return units::seconds (d);
      }
    return conversion_funcs_.tick_to_seconds (units::ticks (d));
  }

  /// @brief Helper method to get the position as samples
  units::sample_t get_samples () const
  {
    const auto &[d, b] = value_.load ();
    auto tick =
      bool_to_format (b) == TimeFormat::Musical
        ? units::ticks (d)
        : conversion_funcs_.seconds_to_tick (units::seconds (d));
    return au::round_as<std::int64_t> (
      units::samples, conversion_funcs_.tick_to_samples (tick));
  }

  void set_samples (units::precise_sample_t samples)
  {
    set_ticks (conversion_funcs_.samples_to_tick (samples));
  }

  const auto &time_conversion_functions () const { return conversion_funcs_; }

private:
  static constexpr bool format_to_bool (TimeFormat format) noexcept
  {
    return format == TimeFormat::Absolute;
  }
  static constexpr TimeFormat bool_to_format (bool b) noexcept
  {
    return b ? TimeFormat::Absolute : TimeFormat::Musical;
  }

  static constexpr auto kMode = "mode"sv;
  static constexpr auto kValue = "value"sv;
  friend void           to_json (nlohmann::json &j, const AtomicPosition &pos);
  friend void from_json (const nlohmann::json &j, AtomicPosition &pos);

private:
  const TimeConversionFunctions &conversion_funcs_;
  internal::AtomicDoubleWithBool value_;
};

} // namespace zrythm::dsp

// Formatter for AtomicPosition
template <>
struct fmt::formatter<zrythm::dsp::AtomicPosition>
    : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const zrythm::dsp::AtomicPosition &pos, FormatContext &ctx) const
  {
    return fmt::formatter<std::string_view>{}.format (
      fmt::format (
        "Ticks: {:.2f} | Seconds: {:.3f} | Samples: {} | [Mode: {}]",
        pos.get_ticks (), pos.get_seconds (), pos.get_samples (),
        pos.get_current_mode ()),
      ctx);
  }
};
