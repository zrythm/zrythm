// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "dsp/tick_types.h"
#include "utils/units.h"

#include <nlohmann/json_fwd.hpp>

namespace zrythm::dsp
{

/**
 * @brief A musical time signature, e.g. 4/4.
 */
struct TimeSignature
{
  int numerator{};   ///< Beats per bar
  int denominator{}; ///< Beat unit (2, 4, 8, 16)
};

/**
 * @brief Validates a time signature's numerator and denominator.
 *
 * @param time_signature Time signature to validate.
 * @throw std::invalid_argument If the numerator is smaller than 1 or the
 * denominator is not a power of two in [1, 128].
 */
inline void
throw_if_invalid_time_signature (const TimeSignature &time_signature)
{
  if (time_signature.numerator < 1)
    throw std::invalid_argument ("Time signature numerator must be >= 1");
  const auto denominator = time_signature.denominator;
  if (
    denominator < 1 || denominator > 128
    || (denominator & (denominator - 1)) != 0)
    throw std::invalid_argument (
      "Time signature denominator must be a power of two in [1, 128]");
}

/**
 * @class FixedPpqTempoMap
 * @brief Manages tempo and time signature events for a DAW timeline.
 *
 * The FixedPpqTempoMap class handles:
 * - Tempo events (constant and linear ramps)
 * - Time signature changes
 * - Conversion between musical time (ticks) and absolute time (seconds/samples)
 * - Conversion between ticks and musical position (bar:beat:sixteenth:tick)
 *
 * All tempo events are stored in musical time (ticks) and automatically
 * adjust to time signature changes. A precondition for this is that tempo
 * events are only added after all time signature events are added.
 *
 * @note The map always carries a base tempo (@ref base_bpm_) and base time
 * signature (@ref base_time_sig_) anchored at tick 0. These govern the region
 * from tick 0 up to the first inserted event (as a constant segment), or the
 * whole timeline if no events exist. An inserted event at tick 0 shadows the
 * base for the region it covers.
 *
 * @tparam PPQ Pulses (ticks) per quarter note
 */
template <units::tick_t::NTTP PPQ> class FixedPpqTempoMap
{
  friend class TempoMapWrapper;

public:
  /// Tempo curve type (constant or linear ramp)
  enum class CurveType : std::uint8_t
  {
    Constant, ///< Constant tempo
    Linear    ///< Linear tempo ramp
  };

  /// Tempo event definition
  struct TempoEvent
  {
    units::tick_t tick;    ///< Position in ticks
    units::bpm_t  bpm{};   ///< Tempo
    CurveType     curve{}; ///< Curve type from this event to the next

    friend void to_json (nlohmann::json &j, const TempoEvent &e);
    friend void from_json (const nlohmann::json &j, TempoEvent &e);
  };

  /// Time signature event definition
  struct TimeSignatureEvent
  {
    units::tick_t tick;           ///< Position in ticks
    TimeSignature time_signature; ///< Numerator and denominator

    constexpr auto quarters_per_bar () const
    {
      return (time_signature.numerator * 4) / time_signature.denominator;
    }

    constexpr auto ticks_per_bar () const
    {
      return quarters_per_bar () * FixedPpqTempoMap::get_ppq ();
    }

    constexpr auto ticks_per_beat () const
    {
      return ticks_per_bar () / time_signature.numerator;
    }

    constexpr auto
    is_different_time_signature (const TimeSignatureEvent &other) const
    {
      return time_signature.numerator != other.time_signature.numerator
             || time_signature.denominator != other.time_signature.denominator;
    }

    friend void to_json (nlohmann::json &j, const TimeSignatureEvent &e);
    friend void from_json (const nlohmann::json &j, TimeSignatureEvent &e);
  };

  /// Musical position representation
  struct MusicalPosition
  {
    int bar{ 1 };       ///< Bar number (1-indexed)
    int beat{ 1 };      ///< Beat in bar (1-indexed)
    int sixteenth{ 1 }; ///< Sixteenth in beat (1-indexed)
    int tick{};         ///< Ticks in sixteenth (0-indexed)

    friend bool
    operator== (const MusicalPosition &lhs, const MusicalPosition &rhs) =
      default;
  };

  /**
   * @brief Construct a new FixedPpqTempoMap object
   * @param sampleRate Sample rate in Hz
   */
  explicit FixedPpqTempoMap (units::precise_sample_rate_t sampleRate)
      : sample_rate_ (sampleRate)
  {
    rebuild_time_signature_cache ();
  }

  /// Set the sample rate
  void set_sample_rate (units::precise_sample_rate_t sampleRate)
  {
    sample_rate_ = sampleRate;
  }

  /**
   * @brief Add a tempo event
   * @param tick Position in ticks
   * @param bpm Tempo
   * @param curve Curve type
   *
   * @warning Tempo events must only be added after all time signature events
   * have been added.
   *
   * @throws std::invalid_argument for invalid BPM or tick values
   */
  void add_tempo_event (units::tick_t tick, units::bpm_t bpm, CurveType curve);

  /// Remove a tempo event at the specified tick
  void remove_tempo_event (units::tick_t tick);

  /**
   * @brief Add a time signature event
   * @param tick Position in ticks
   * @param numerator Beats per bar
   * @param denominator Beat unit (2,4,8,16).
   *
   * For the beat unit:
   * - 2 means 1 beat is a 1/2 note (8 sixteenths).
   * - 4 means 1 beat is a 1/4 note (4 sixteenths).
   * - 8 means 1 beat is a 1/8 note (2 sixteenths).
   * - 16 means 1 beat is a 1/16 note (1 sixteenth).
   *
   * @throws std::invalid_argument for invalid parameters
   */
  void
  add_time_signature_event (units::tick_t tick, TimeSignature time_signature);

  /// Remove a time signature event at the specified tick
  void remove_time_signature_event (units::tick_t tick);

  /// Convert timeline ticks to seconds
  auto tick_to_seconds (TimelineTick tick) const -> units::precise_second_t;

  /// Convert timeline ticks to samples
  units::precise_sample_t tick_to_samples (TimelineTick tick) const
  {
    return tick_to_seconds (tick) * sample_rate_;
  }

  units::sample_t tick_to_samples_rounded (TimelineTick tick) const
  {
    return au::round_as<int64_t> (units::samples, tick_to_samples (tick));
  }

  /// Convert seconds to timeline ticks
  TimelineTick seconds_to_tick (units::precise_second_t seconds) const;

  /// Convert samples to timeline ticks
  TimelineTick samples_to_tick (units::precise_sample_t samples) const
  {
    const auto seconds = samples / sample_rate_;
    return seconds_to_tick (seconds);
  }

  /**
   * @brief Get the time signature event active at the given tick.
   * @param tick Position in ticks
   * @return Time signature event (or default 4/4 if none found)
   */
  TimeSignatureEvent time_signature_at_tick (units::tick_t tick) const;

  /**
   * @brief Get the tempo (BPM) active at the given tick.
   * @param tick Position in ticks
   * @return Tempo (or default 120 BPM if none found)
   */
  units::bpm_t tempo_at_tick (units::tick_t tick) const;

  /**
   * @brief Read-only view of all inserted tempo events, sorted ascending by
   * tick.
   *
   * The base tempo (@ref base_bpm_) governs any region not covered by these
   * events, starting from tick 0.
   */
  std::span<const TempoEvent> tempo_events () const noexcept { return events_; }

  /**
   * @brief Read-only view of all inserted time signature events, sorted
   * ascending by tick.
   *
   * The base time signature (@ref base_time_sig_) governs any region not
   * covered by these events, starting from tick 0.
   */
  std::span<const TimeSignatureEvent> time_signature_events () const noexcept
  {
    return time_sig_events_;
  }

  /**
   * @brief Convert ticks to musical position (bar:beat:sixteenth:tick)
   * @param tick Position in ticks
   * @return Musical position
   */
  MusicalPosition tick_to_musical_position (units::tick_t tick) const;

  MusicalPosition samples_to_musical_position (units::sample_t samples) const;

  /**
   * @brief Convert musical position to ticks
   * @param pos Musical position
   * @return Position in ticks
   *
   * @throws std::invalid_argument for invalid position
   */
  TimelineTickI musical_position_to_tick (const MusicalPosition &pos) const;

  /// Get pulses per quarter note
  static consteval auto get_ppq () { return from_nttp (PPQ); }

  /// Get current sample rate
  auto get_sample_rate () const { return sample_rate_; }

  /// Base tempo at tick 0.
  units::bpm_t base_bpm () const { return base_bpm_; }

  /// Set the base tempo at tick 0 and recompute the lead-segment timing.
  void set_base_bpm (units::bpm_t bpm)
  {
    base_bpm_ = bpm;
    rebuild_cumulative_times ();
  }

  /// Base time signature at tick 0.
  const TimeSignatureEvent &base_time_signature () const
  {
    return base_time_sig_;
  }

  /// Set the base time signature at tick 0.
  void set_base_time_signature (TimeSignature time_signature)
  {
    throw_if_invalid_time_signature (time_signature);
    base_time_sig_.time_signature = time_signature;
    rebuild_time_signature_cache ();
  }

  /// Read-only view of the time signature events with the base signature
  /// prepended at tick 0 when no inserted event sits at tick 0. Backed by a
  /// cache (@ref effective_time_sig_events_) rebuilt on mutation, so the read
  /// path is allocation-free.
  ///
  /// @note Realtime safety relies on a caller-enforced contract: the cache
  /// must not be mutated while the audio thread is mid-read. All mutation
  /// entry points (add/remove/clear/set_base and project reload) must run
  /// while the engine is stopped or paused.
  std::span<const TimeSignatureEvent>
  effective_time_signature_events () const noexcept
  {
    return effective_time_sig_events_;
  }

private:
  /// Rebuild cumulative time cache
  void rebuild_cumulative_times ();

  /// Rebuild the cached effective time signature view
  /// (@ref effective_time_sig_events_).
  void rebuild_time_signature_cache ();

  /// Compute time duration for a segment between two tempo events
  units::precise_second_t compute_segment_time (
    const TempoEvent &start,
    const TempoEvent &end,
    units::tick_t     segmentTicks) const;

  /// Clear all tempo events
  void clear_tempo_events ()
  {
    events_.clear ();
    cumulative_seconds_.clear ();
  }

  /// Clear all time signature events
  void clear_time_signature_events ()
  {
    time_sig_events_.clear ();
    rebuild_time_signature_cache ();
  }

  static constexpr std::string_view kTempoChangesKey = "tempoChanges";
  static constexpr std::string_view kTimeSignaturesKey = "timeSignatures";
  static constexpr std::string_view kBaseBpmKey = "baseBpm";
  static constexpr std::string_view kBaseTimeSignatureKey = "baseTimeSignature";
  friend void to_json (nlohmann::json &j, const FixedPpqTempoMap &tempo_map);
  friend void from_json (const nlohmann::json &j, FixedPpqTempoMap &tempo_map);

private:
  units::precise_sample_rate_t sample_rate_; ///< Current sample rate
  static constexpr auto        ticks_per_sixteenth_ =
    from_nttp (PPQ) / 4; ///< Ticks per sixteenth note (PPQ/4)

  // Base tempo and time signature anchored at tick 0. These govern the region
  // from tick 0 up to the first inserted event (constant), unless an inserted
  // event exists at tick 0.
  units::bpm_t       base_bpm_{ units::bpm (120.0) };
  TimeSignatureEvent base_time_sig_{
    units::ticks (0), TimeSignature{ 4, 4 }
  };

  std::vector<TempoEvent>         events_;          ///< Tempo events
  std::vector<TimeSignatureEvent> time_sig_events_; ///< Time signature events
  std::vector<units::precise_second_t>
    cumulative_seconds_; ///< Cumulative seconds at tempo events
  // Cache of time-signature events with the base signature prepended at tick 0
  // when no inserted event sits there. Rebuilt on every mutation so the read
  // path (@ref effective_time_signature_events) stays allocation-free.
  std::vector<TimeSignatureEvent> effective_time_sig_events_;
};

/**
 * @see FixedPpqTempoMap.
 */
using TempoMap = FixedPpqTempoMap<units::PPQ>;

extern template class FixedPpqTempoMap<units::PPQ>;
}
