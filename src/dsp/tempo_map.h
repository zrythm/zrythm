// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "utils/units.h"

#include <nlohmann/json_fwd.hpp>

namespace zrythm::dsp
{

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
 * @note If no events are added, the tempo map will use defaults which can be
 * modified.
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
    double        bpm{};   ///< Tempo in BPM
    CurveType     curve{}; ///< Curve type from this event to the next

    friend void to_json (nlohmann::json &j, const TempoEvent &e);
    friend void from_json (const nlohmann::json &j, TempoEvent &e);
  };

  /// Time signature event definition
  struct TimeSignatureEvent
  {
    units::tick_t tick;          ///< Position in ticks
    int           numerator{};   ///< Beats per bar
    int           denominator{}; ///< Beat unit (2,4,8,16)

    constexpr auto quarters_per_bar () const
    {
      return (numerator * 4) / denominator;
    }

    constexpr auto ticks_per_bar () const
    {
      return units::ticks (quarters_per_bar () * FixedPpqTempoMap::get_ppq ());
    }

    constexpr auto ticks_per_beat () const
    {
      return ticks_per_bar () / numerator;
    }

    constexpr auto
    is_different_time_signature (const TimeSignatureEvent &other) const
    {
      return numerator != other.numerator || denominator != other.denominator;
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
  }

  /// Set the sample rate
  void set_sample_rate (units::precise_sample_rate_t sampleRate)
  {
    sample_rate_ = sampleRate;
  }

  /**
   * @brief Add a tempo event
   * @param tick Position in ticks
   * @param bpm Tempo in BPM
   * @param curve Curve type
   *
   * @warning Tempo events must only be added after all time signature events
   * have been added.
   *
   * @throws std::invalid_argument for invalid BPM or tick values
   */
  void add_tempo_event (units::tick_t tick, double bpm, CurveType curve);

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
  add_time_signature_event (units::tick_t tick, int numerator, int denominator);

  /// Remove a time signature event at the specified tick
  void remove_time_signature_event (units::tick_t tick);

  /// Convert fractional ticks to seconds
  auto
  tick_to_seconds (units::precise_tick_t tick) const -> units::precise_second_t;

  /// Convert fractional ticks to samples
  units::precise_sample_t tick_to_samples (units::precise_tick_t tick) const
  {
    return tick_to_seconds (tick) * sample_rate_;
  }

  units::sample_t tick_to_samples_rounded (units::precise_tick_t tick) const
  {
    return au::round_as<int64_t> (units::samples, tick_to_samples (tick));
  }

  /// Convert seconds to fractional ticks
  units::precise_tick_t seconds_to_tick (units::precise_second_t seconds) const;

  /// Convert samples to fractional ticks
  units::precise_tick_t samples_to_tick (units::precise_sample_t samples) const
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
   * @brief Get the tempo event active at the given tick.
   * @param tick Position in ticks
   * @return Tempo (or default 120 BPM if none found)
   */
  double tempo_at_tick (units::tick_t tick) const;

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
  units::tick_t musical_position_to_tick (const MusicalPosition &pos) const;

  /// Get pulses per quarter note
  static consteval int get_ppq () { return from_nttp (PPQ).in (units::ticks); }

  /// Get current sample rate
  double get_sample_rate () const
  {
    return sample_rate_.in (units::sample_rate);
  }

  void set_default_bpm (double bpm) { default_tempo_.bpm = bpm; }
  void set_default_time_signature (int numerator, int denominator)
  {
    default_time_sig_.numerator = numerator;
    default_time_sig_.denominator = denominator;
  }

private:
  /// Get all tempo events
  const std::vector<TempoEvent> &get_tempo_events () const { return events_; }

  /// Get all time signature events
  const std::vector<TimeSignatureEvent> &get_time_signature_events () const
  {
    return time_sig_events_;
  }

  /// Rebuild cumulative time cache
  void rebuild_cumulative_times ();

  /// Compute time duration for a segment between two tempo events
  units::precise_second_t compute_segment_time (
    const TempoEvent &start,
    const TempoEvent &end,
    units::tick_t     segmentTicks) const;

  auto get_events_or_default () const;
  auto get_time_signature_events_or_default () const;

  /// Clear all tempo events
  void clear_tempo_events ()
  {
    events_.clear ();
    cumulative_seconds_.clear ();
  }

  /// Clear all time signature events
  void clear_time_signature_events () { time_sig_events_.clear (); }

  static constexpr std::string_view kTempoChangesKey = "tempoChanges";
  static constexpr std::string_view kTimeSignaturesKey = "timeSignatures";
  friend void to_json (nlohmann::json &j, const FixedPpqTempoMap &tempo_map);
  friend void from_json (const nlohmann::json &j, FixedPpqTempoMap &tempo_map);

private:
  units::precise_sample_rate_t sample_rate_; ///< Current sample rate
  static constexpr auto        ticks_per_sixteenth_ =
    from_nttp (PPQ) / 4; ///< Ticks per sixteenth note (PPQ/4)

  static constexpr auto DEFAULT_BPM_EVENT = TempoEvent{
    units::ticks (0), 120.0, CurveType::Constant
  }; ///< Default tempo in BPM
  static constexpr auto DEFAULT_TIME_SIG_EVENT =
    TimeSignatureEvent{ units::ticks (0), 4, 4 }; ///< Default time signature
  static constexpr auto DEFAULT_CUMULATIVE_SECONDS = units::seconds (0.0);

  // Default tempo and time signature to be used when no events are present
  TempoEvent         default_tempo_{ DEFAULT_BPM_EVENT };
  TimeSignatureEvent default_time_sig_{ DEFAULT_TIME_SIG_EVENT };

  std::vector<TempoEvent>         events_;          ///< Tempo events
  std::vector<TimeSignatureEvent> time_sig_events_; ///< Time signature events
  std::vector<units::precise_second_t>
    cumulative_seconds_; ///< Cumulative seconds at tempo events
};

/**
 * @see FixedPpqTempoMap.
 */
using TempoMap = FixedPpqTempoMap<units::PPQ>;

extern template class FixedPpqTempoMap<units::PPQ>;
}
