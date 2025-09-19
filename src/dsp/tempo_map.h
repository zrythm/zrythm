// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "utils/types.h"

#include <nlohmann/json.hpp>

using namespace std::literals;

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
template <int PPQ> class FixedPpqTempoMap
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
    int64_t   tick{};  ///< Position in ticks
    double    bpm{};   ///< Tempo in BPM
    CurveType curve{}; ///< Curve type from this event to the next

    NLOHMANN_DEFINE_TYPE_INTRUSIVE (TempoEvent, tick, bpm, curve)
  };

  /// Time signature event definition
  struct TimeSignatureEvent
  {
    int64_t tick{};        ///< Position in ticks
    int     numerator{};   ///< Beats per bar
    int     denominator{}; ///< Beat unit (2,4,8,16)

    NLOHMANN_DEFINE_TYPE_INTRUSIVE (
      TimeSignatureEvent,
      tick,
      numerator,
      denominator)
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
  explicit FixedPpqTempoMap (double sampleRate) : sample_rate_ (sampleRate) { }

  /// Set the sample rate
  void set_sample_rate (double sampleRate) { sample_rate_ = sampleRate; }

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
  void add_tempo_event (int64_t tick, double bpm, CurveType curve)
  {
    if (bpm <= 0)
      throw std::invalid_argument ("BPM must be positive");
    if (tick < 0)
      throw std::invalid_argument ("Tick must be non-negative");

    // Automatically add a default tempo event at tick 0
    if (events_.empty () && tick != 0)
      {
        events_.push_back (default_tempo_);
      }

    // Find and remove existing event at same tick
    auto it = std::ranges::find (events_, tick, [] (const TempoEvent &e) {
      return e.tick;
    });
    if (it != events_.end ())
      {
        events_.erase (it);
      }

    events_.push_back ({ tick, bpm, curve });
    rebuild_cumulative_times ();
  }

  /// Remove a tempo event at the specified tick
  void remove_tempo_event (int64_t tick)
  {
    if (events_.size () > 1 && tick == 0)
      throw std::invalid_argument (
        "Cannot remove first tempo event - remove other tempo event first");

    auto it = std::ranges::find (events_, tick, [] (const TempoEvent &e) {
      return e.tick;
    });
    if (it != events_.end ())
      {
        events_.erase (it);
        rebuild_cumulative_times ();
      }
  }

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
  void add_time_signature_event (int64_t tick, int numerator, int denominator)
  {
    if (tick < 0)
      throw std::invalid_argument ("Tick must be non-negative");
    if (numerator <= 0 || denominator <= 0)
      throw std::invalid_argument ("Invalid time signature");
    if (!events_.empty ())
      throw std::logic_error (
        "Time signature events must be added before tempo events");

    // Automatically add a default time signature event at tick 0
    if (time_sig_events_.empty () && tick != 0)
      {
        time_sig_events_.push_back (default_time_sig_);
      }

    // Remove existing event at same tick
    auto it = std::ranges::find (
      time_sig_events_, tick,
      [] (const TimeSignatureEvent &e) { return e.tick; });
    if (it != time_sig_events_.end ())
      {
        time_sig_events_.erase (it);
      }

    time_sig_events_.push_back ({ tick, numerator, denominator });
    std::ranges::sort (time_sig_events_, {}, &TimeSignatureEvent::tick);
  }

  /// Remove a time signature event at the specified tick
  void remove_time_signature_event (int64_t tick)
  {
    if (time_sig_events_.size () > 1 && tick == 0)
      throw std::invalid_argument (
        "Cannot remove time signature event at tick 0 - remove other events first");

    auto it = std::ranges::find (
      time_sig_events_, tick,
      [] (const TimeSignatureEvent &e) { return e.tick; });
    if (it != time_sig_events_.end ())
      {
        time_sig_events_.erase (it);
      }
  }

  /// Convert fractional ticks to seconds
  double tick_to_seconds (double tick) const
  {
    const auto &[events, cumulative_seconds] = get_events_or_default ();

    // Find the last event <= target tick
    auto it = std::ranges::upper_bound (events, tick, {}, &TempoEvent::tick);

    if (it == events.begin ())
      {
        return 0.0;
      }

    size_t        index = std::distance (events.begin (), it) - 1;
    const auto   &startEvent = events[index];
    const int64_t segmentStart = startEvent.tick;
    const double  ticksFromStart = tick - static_cast<double> (segmentStart);
    const double  baseSeconds = cumulative_seconds[index];

    // Last event segment
    if (index == events.size () - 1)
      {
        return baseSeconds
               + (ticksFromStart / static_cast<double> (get_ppq ()))
                   * (60.0 / startEvent.bpm);
      }

    const auto   &endEvent = events[index + 1];
    const int64_t segmentTicks = endEvent.tick - segmentStart;
    const double  dSegmentTicks = static_cast<double> (segmentTicks);

    // Constant tempo segment
    if (startEvent.curve == CurveType::Constant)
      {
        return baseSeconds
               + (ticksFromStart / static_cast<double> (get_ppq ()))
                   * (60.0 / startEvent.bpm);
      }
    // Linear tempo ramp
    else if (startEvent.curve == CurveType::Linear)
      {
        const double bpm0 = startEvent.bpm;
        const double bpm1 = endEvent.bpm;

        if (std::abs (bpm1 - bpm0) < 1e-5)
          {
            return baseSeconds
                   + (ticksFromStart / static_cast<double> (get_ppq ()))
                       * (60.0 / bpm0);
          }

        const double fraction = ticksFromStart / dSegmentTicks;
        const double currentBpm = bpm0 + fraction * (bpm1 - bpm0);

        return baseSeconds
               + (60.0 * dSegmentTicks) / (get_ppq () * (bpm1 - bpm0))
                   * std::log (currentBpm / bpm0);
      }

    return baseSeconds;
  }

  /// Convert fractional ticks to samples
  double tick_to_samples (double tick) const
  {
    return tick_to_seconds (tick) * sample_rate_;
  }

  int64_t tick_to_samples_rounded (double tick) const
  {
    return static_cast<int64_t> (std::round (tick_to_samples (tick)));
  }

  /// Convert seconds to fractional ticks
  double seconds_to_tick (double seconds) const
  {
    if (seconds <= 0.0)
      return 0.0;

    const auto &[events, cumulative_seconds] = get_events_or_default ();

    // Find the segment containing the time
    auto         it = std::ranges::upper_bound (cumulative_seconds, seconds);
    const size_t index =
      (it == cumulative_seconds.begin ())
        ? 0
        : std::distance (cumulative_seconds.begin (), it) - 1;

    const double      baseSeconds = cumulative_seconds[index];
    const double      timeInSegment = seconds - baseSeconds;
    const TempoEvent &startEvent = events[index];

    // Last segment
    if (index == events.size () - 1)
      {
        const double beats = timeInSegment * (startEvent.bpm / 60.0);
        return static_cast<double> (startEvent.tick) + beats * get_ppq ();
      }

    const TempoEvent &endEvent = events[index + 1];
    const int64_t     segmentTicks = endEvent.tick - startEvent.tick;
    const double      dSegmentTicks = static_cast<double> (segmentTicks);

    // Constant tempo segment
    if (startEvent.curve == CurveType::Constant)
      {
        const double beats = timeInSegment * (startEvent.bpm / 60.0);
        return static_cast<double> (startEvent.tick) + beats * get_ppq ();
      }
    // Linear tempo ramp
    else if (startEvent.curve == CurveType::Linear)
      {
        const double bpm0 = startEvent.bpm;
        const double bpm1 = endEvent.bpm;

        if (std::abs (bpm1 - bpm0) < 1e-5)
          {
            const double beats = timeInSegment * (bpm0 / 60.0);
            return static_cast<double> (startEvent.tick) + beats * get_ppq ();
          }

        const double A = (get_ppq () * (bpm1 - bpm0)) / (60.0 * dSegmentTicks);
        const double expVal = std::exp (A * timeInSegment);
        const double f = (expVal - 1.0) * (bpm0 / (bpm1 - bpm0));

        return static_cast<double> (startEvent.tick) + f * dSegmentTicks;
      }

    return static_cast<double> (startEvent.tick);
  }

  /// Convert samples to fractional ticks
  double samples_to_tick (double samples) const
  {
    const double seconds = samples / sample_rate_;
    return seconds_to_tick (seconds);
  }

  /**
   * @brief Get the time signature event active at the given tick.
   * @param tick Position in ticks
   * @return Time signature event (or default 4/4 if none found)
   */
  TimeSignatureEvent time_signature_at_tick (int64_t tick) const
  {
    if (time_sig_events_.empty ())
      return default_time_sig_;

    // Find the last time signature change <= tick
    auto it = std::ranges::upper_bound (
      time_sig_events_, tick, {}, &TimeSignatureEvent::tick);
    if (it == time_sig_events_.begin ())
      {
        // No event before tick - return default
        return default_time_sig_;
      }
    --it;

    return *it;
  }

  /**
   * @brief Get the tempo event active at the given tick.
   * @param tick Position in ticks
   * @return Tempo (or default 120 BPM if none found)
   */
  double tempo_at_tick (int64_t tick) const
  {
    if (events_.empty ())
      return default_tempo_.bpm;

    // Find the last tempo change <= tick
    auto it = std::ranges::upper_bound (events_, tick, {}, &TempoEvent::tick);
    if (it == events_.begin ())
      {
        // No event before tick - return default
        return default_tempo_.bpm;
      }
    --it;

    // If this is the last event or constant, return as-is
    if (it == events_.end () - 1 || (it)->curve == CurveType::Constant)
      {
        return it->bpm;
      }

    // Handle linear ramp segment
    const auto   &startEvent = *it;
    const auto   &endEvent = *(it + 1);
    const int64_t segmentTicks = endEvent.tick - startEvent.tick;
    const double  fraction =
      static_cast<double> (tick - startEvent.tick) / segmentTicks;
    const double currentBpm =
      startEvent.bpm + fraction * (endEvent.bpm - startEvent.bpm);

    return currentBpm;
  }

  /**
   * @brief Convert ticks to musical position (bar:beat:sixteenth:tick)
   * @param tick Position in ticks
   * @return Musical position
   */
  MusicalPosition tick_to_musical_position (int64_t tick) const
  {
    const auto &time_sig_events = get_time_signature_events_or_default ();

    if (time_sig_events.empty ())
      return { 1, 1, 1, 0 };

    // Find the last time signature change <= tick
    auto it = std::ranges::upper_bound (
      time_sig_events, tick, {}, &TimeSignatureEvent::tick);
    if (it == time_sig_events.begin ())
      {
        it = time_sig_events.end (); // No valid event
      }
    else
      {
        --it;
      }

    if (it == time_sig_events.end ())
      {
        return { 1, 1, 1, 0 };
      }

    const auto &sigEvent = *it;
    const int   numerator = sigEvent.numerator;
    const int   denominator = sigEvent.denominator;

    // Calculate ticks per bar and beat
    const double  quarters_per_bar = numerator * (4.0 / denominator);
    const int64_t ticks_per_bar =
      static_cast<int64_t> (quarters_per_bar * get_ppq ());
    const int64_t ticks_per_beat = ticks_per_bar / numerator;

    // Calculate absolute bar number
    int64_t cumulative_bars = 1;
    // int64_t cumulative_ticks = 0;

    // Calculate total bars from previous time signatures
    for (auto prev = time_sig_events.begin (); prev != it; ++prev)
      {
        const int    prev_numerator = prev->numerator;
        const int    prev_denominator = prev->denominator;
        const double prev_quarters_per_bar =
          prev_numerator * (4.0 / prev_denominator);
        const int64_t prev_ticks_per_bar =
          static_cast<int64_t> (prev_quarters_per_bar * get_ppq ());

        // Ticks from this signature to next
        auto          next = std::next (prev);
        const int64_t end_tick =
          (next != time_sig_events.end ()) ? next->tick : sigEvent.tick;
        const int64_t segment_ticks = end_tick - prev->tick;

        cumulative_bars += segment_ticks / prev_ticks_per_bar;
        // cumulative_ticks += segment_ticks;
      }

    // Calculate bars since current signature
    const int64_t ticks_since_sig = tick - sigEvent.tick;
    const int64_t bars_since_sig = ticks_since_sig / ticks_per_bar;
    const int     bar = cumulative_bars + bars_since_sig;

    // Calculate position within current bar
    const int64_t ticks_in_bar = ticks_since_sig % ticks_per_bar;
    const int     beat = 1 + static_cast<int> (ticks_in_bar / ticks_per_beat);

    // Calculate position within current beat
    const int64_t ticks_in_beat = ticks_in_bar % ticks_per_beat;
    const int     sixteenth =
      1 + static_cast<int> (ticks_in_beat / ticks_per_sixteenth_);
    const int tick_in_sixteenth =
      static_cast<int> (ticks_in_beat % ticks_per_sixteenth_);

    return { bar, beat, sixteenth, tick_in_sixteenth };
  }

  MusicalPosition samples_to_musical_position (int64_t samples) const
  {
    // Note: we are using `floor()` because we never want the MusicalPosition to
    // be after the given samples
    const auto tick = static_cast<std::int64_t> (
      std::floor (samples_to_tick (static_cast<double> (samples))));
    return tick_to_musical_position (tick);
  }

  /**
   * @brief Convert musical position to ticks
   * @param pos Musical position
   * @return Position in ticks
   *
   * @throws std::invalid_argument for invalid position
   */
  int64_t musical_position_to_tick (const MusicalPosition &pos) const
  {
    const auto &time_sig_events = get_time_signature_events_or_default ();

    // Validate position
    if (pos.bar < 1 || pos.beat < 1 || pos.sixteenth < 1 || pos.tick < 0)
      {
        throw std::invalid_argument ("Invalid musical position");
      }

    int64_t cumulative_ticks = 0;
    int     current_bar = 1;

    // Iterate through time signature changes
    for (size_t i = 0; i < time_sig_events.size (); ++i)
      {
        const auto   &event = time_sig_events[i];
        const int     numerator = event.numerator;
        const int     denominator = event.denominator;
        const double  quarters_per_bar = numerator * (4.0 / denominator);
        const int64_t ticks_per_bar =
          static_cast<int64_t> (quarters_per_bar * get_ppq ());

        // Determine bars covered by this time signature
        int bars_in_this_sig = 0;
        if (i < time_sig_events.size () - 1)
          {
            const int64_t next_tick = time_sig_events[i + 1].tick;
            bars_in_this_sig =
              static_cast<int> ((next_tick - event.tick) / ticks_per_bar);
          }
        else
          {
            bars_in_this_sig = pos.bar - current_bar + 1;
          }

        // Check if position falls in this time signature segment
        if (pos.bar < current_bar + bars_in_this_sig)
          {
            const int     bar_in_seg = pos.bar - current_bar;
            const int64_t bar_ticks = event.tick + bar_in_seg * ticks_per_bar;
            const int64_t ticks_per_beat = ticks_per_bar / numerator;

            // Add beat and sub-beat components
            return bar_ticks
                   + (static_cast<int64_t> (pos.beat - 1) * ticks_per_beat)
                   + (static_cast<int64_t> (pos.sixteenth - 1) * ticks_per_sixteenth_)
                   + pos.tick;
          }

        // Move to next time signature segment
        cumulative_ticks +=
          static_cast<int64_t> (bars_in_this_sig) * ticks_per_bar;
        current_bar += bars_in_this_sig;
      }

    return cumulative_ticks;
  }

  /// Get pulses per quarter note
  static consteval int get_ppq () { return PPQ; }

  /// Get current sample rate
  double get_sample_rate () const { return sample_rate_; }

  void set_default_bpm (double bpm) { default_tempo_.bpm = bpm; }
  void set_default_time_signature (int numerator, int denominator)
  {
    default_tempo_.numerator = numerator;
    default_tempo_.denominator = denominator;
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
  void rebuild_cumulative_times ()
  {
    if (events_.empty ())
      return;

    // Sort events by tick
    std::ranges::sort (events_, {}, &TempoEvent::tick);

    cumulative_seconds_.resize (events_.size ());
    cumulative_seconds_[0] = 0.0;

    // Compute cumulative time at each event point
    for (size_t i = 0; i < events_.size () - 1; ++i)
      {
        const int64_t segmentTicks = events_[i + 1].tick - events_[i].tick;
        cumulative_seconds_[i + 1] =
          cumulative_seconds_[i]
          + compute_segment_time (events_[i], events_[i + 1], segmentTicks);
      }
  }

  /// Compute time duration for a segment between two tempo events
  double compute_segment_time (
    const TempoEvent &start,
    const TempoEvent &end,
    int64_t           segmentTicks) const
  {
    if (start.curve == CurveType::Constant)
      {
        return (static_cast<double> (segmentTicks)
                / static_cast<double> (get_ppq ()))
               * (60.0 / start.bpm);
      }
    if (start.curve == CurveType::Linear)
      {
        const double bpm0 = start.bpm;
        const double bpm1 = end.bpm;

        if (std::abs (bpm1 - bpm0) < 1e-5)
          {
            return (static_cast<double> (segmentTicks)
                    / static_cast<double> (get_ppq ()))
                   * (60.0 / bpm0);
          }

        return (60.0 * static_cast<double> (segmentTicks))
               / (get_ppq () * (bpm1 - bpm0)) * std::log (bpm1 / bpm0);
      }
    return 0.0;
  }

  auto get_events_or_default () const
  {
    if (events_.empty ())
      {
        return std::make_pair (
          std::span{ &default_tempo_, 1 },
          std::span{ &DEFAULT_CUMULATIVE_SECONDS, 1 });
      }

    return std::make_pair (
      std::span{ events_.data (), events_.size () },
      std::span{ cumulative_seconds_.data (), cumulative_seconds_.size () });
  }
  auto get_time_signature_events_or_default () const
  {
    if (time_sig_events_.empty ())
      {
        return std::span{ &default_time_sig_, 1 };
      }

    return std::span{ time_sig_events_.data (), time_sig_events_.size () };
  }

  static constexpr auto kEventsKey = "events"sv;
  static constexpr auto kTimeSigEventsKey = "timeSigEvents"sv;
  friend void to_json (nlohmann::json &j, const FixedPpqTempoMap &tempo_map)
  {
    j[kTimeSigEventsKey] = tempo_map.time_sig_events_;
    j[kEventsKey] = tempo_map.events_;
  }
  friend void from_json (const nlohmann::json &j, FixedPpqTempoMap &tempo_map)
  {
    j.at (kTimeSigEventsKey).get_to (tempo_map.time_sig_events_);
    j.at (kEventsKey).get_to (tempo_map.events_);
    tempo_map.rebuild_cumulative_times ();
  }

private:
  double               sample_rate_; ///< Current sample rate
  static constexpr int ticks_per_sixteenth_ =
    PPQ / 4; ///< Ticks per sixteenth note (PPQ/4)

  static constexpr auto DEFAULT_BPM_EVENT =
    TempoEvent{ 0, 120.0, CurveType::Constant }; ///< Default tempo in BPM
  static constexpr auto DEFAULT_TIME_SIG_EVENT =
    TimeSignatureEvent{ 0, 4, 4 }; ///< Default time signature
  static constexpr auto DEFAULT_CUMULATIVE_SECONDS = 0.0;

  // Default tempo and time signature to be used when no events are present
  TempoEvent         default_tempo_{ DEFAULT_BPM_EVENT };
  TimeSignatureEvent default_time_sig_{ DEFAULT_TIME_SIG_EVENT };

  std::vector<TempoEvent>         events_;          ///< Tempo events
  std::vector<TimeSignatureEvent> time_sig_events_; ///< Time signature events
  std::vector<double>
    cumulative_seconds_; ///< Cumulative seconds at tempo events
};

/**
 * @see FixedPpqTempoMap.
 */
using TempoMap = FixedPpqTempoMap<960>;
}
