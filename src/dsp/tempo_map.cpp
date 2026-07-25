// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cassert>
#include <cmath>
#include <ranges>
#include <span>
#include <stdexcept>

#include "dsp/tempo_map.h"
#include "utils/logger.h"
#include "utils/serialization.h"

#include <au/math.hh>
#include <nlohmann/json.hpp>

namespace zrythm::dsp
{

void
to_json (nlohmann::json &j, const FixedPpqTempoMap<units::PPQ>::TempoEvent &e)
{
  j = nlohmann::json{
    { "tickPosition", e.tick  },
    { "bpm",          e.bpm   },
    { "curve",        e.curve }
  };
}

void
from_json (const nlohmann::json &j, FixedPpqTempoMap<units::PPQ>::TempoEvent &e)
{
  j.at ("tickPosition").get_to (e.tick);
  j.at ("bpm").get_to (e.bpm);
  j.at ("curve").get_to (e.curve);
}

void
to_json (
  nlohmann::json                                         &j,
  const FixedPpqTempoMap<units::PPQ>::TimeSignatureEvent &e)
{
  j = nlohmann::json{
    { "tickPosition", e.tick        },
    { "numerator",    e.numerator   },
    { "denominator",  e.denominator }
  };
}

void
from_json (
  const nlohmann::json                             &j,
  FixedPpqTempoMap<units::PPQ>::TimeSignatureEvent &e)
{
  j.at ("tickPosition").get_to (e.tick);
  j.at ("numerator").get_to (e.numerator);
  j.at ("denominator").get_to (e.denominator);
}

void
to_json (nlohmann::json &j, const FixedPpqTempoMap<units::PPQ> &tempo_map)
{
  j[tempo_map.kBaseBpmKey] = tempo_map.base_bpm_;
  j[tempo_map.kBaseTimeSignatureKey] = {
    { "numerator",   tempo_map.base_time_sig_.numerator   },
    { "denominator", tempo_map.base_time_sig_.denominator },
  };
  j[tempo_map.kTimeSignaturesKey] = tempo_map.time_sig_events_;
  j[tempo_map.kTempoChangesKey] = tempo_map.events_;
}

void
from_json (const nlohmann::json &j, FixedPpqTempoMap<units::PPQ> &tempo_map)
{
  if (j.contains (tempo_map.kBaseBpmKey))
    j.at (tempo_map.kBaseBpmKey).get_to (tempo_map.base_bpm_);
  if (j.contains (tempo_map.kBaseTimeSignatureKey))
    {
      const auto &bts = j.at (tempo_map.kBaseTimeSignatureKey);
      if (bts.contains ("numerator"))
        bts.at ("numerator").get_to (tempo_map.base_time_sig_.numerator);
      else
        z_warning (
          "Missing 'numerator' in serialized baseTimeSignature; keeping default");
      if (bts.contains ("denominator"))
        bts.at ("denominator").get_to (tempo_map.base_time_sig_.denominator);
      else
        z_warning (
          "Missing 'denominator' in serialized baseTimeSignature; keeping default");
    }

  j.at (tempo_map.kTimeSignaturesKey).get_to (tempo_map.time_sig_events_);
  j.at (tempo_map.kTempoChangesKey).get_to (tempo_map.events_);
  tempo_map.rebuild_time_signature_cache ();
  tempo_map.rebuild_cumulative_times ();
}

template <units::tick_t::NTTP PPQ>
void
FixedPpqTempoMap<
  PPQ>::add_tempo_event (units::tick_t tick, units::bpm_t bpm, CurveType curve)
{
  if (bpm <= units::bpm (0.0))
    throw std::invalid_argument ("BPM must be positive");
  if (tick < units::ticks (0))
    throw std::invalid_argument ("Tick must be non-negative");

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

template <units::tick_t::NTTP PPQ>
void
FixedPpqTempoMap<PPQ>::remove_tempo_event (units::tick_t tick)
{
  if (events_.size () > 1 && tick == units::ticks (0))
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

template <units::tick_t::NTTP PPQ>
void
FixedPpqTempoMap<PPQ>::
  add_time_signature_event (units::tick_t tick, int numerator, int denominator)
{
  if (tick < units::ticks (0))
    throw std::invalid_argument ("Tick must be non-negative");
  throw_if_invalid_time_signature (numerator, denominator);
  if (!events_.empty ())
    throw std::logic_error (
      "Time signature events must be added before tempo events");

  // Remove existing event at same tick
  auto it = std::ranges::find (
    time_sig_events_, tick, [] (const TimeSignatureEvent &e) { return e.tick; });
  if (it != time_sig_events_.end ())
    {
      time_sig_events_.erase (it);
    }

  time_sig_events_.push_back ({ tick, numerator, denominator });
  std::ranges::sort (time_sig_events_, {}, &TimeSignatureEvent::tick);
  rebuild_time_signature_cache ();
}

template <units::tick_t::NTTP PPQ>
void
FixedPpqTempoMap<PPQ>::remove_time_signature_event (units::tick_t tick)
{
  if (time_sig_events_.size () > 1 && tick == units::ticks (0))
    throw std::invalid_argument (
      "Cannot remove time signature event at tick 0 - remove other events first");

  auto it = std::ranges::find (
    time_sig_events_, tick, [] (const TimeSignatureEvent &e) { return e.tick; });
  if (it != time_sig_events_.end ())
    {
      time_sig_events_.erase (it);
      rebuild_time_signature_cache ();
    }
}

template <units::tick_t::NTTP PPQ>
auto
FixedPpqTempoMap<PPQ>::tick_to_seconds (TimelineTick tick) const
  -> units::precise_second_t
{
  const auto tick_q = tick.asQuantity ();

  // Negative timeline positions are invalid
  assert (tick_q >= units::ticks (0.0));

  // No inserted events: base tempo (constant) over the whole timeline.
  if (events_.empty ())
    return tick_q / base_bpm_;

  // Find the last event <= target tick
  auto it = std::ranges::upper_bound (events_, tick_q, {}, &TempoEvent::tick);

  if (it == events_.begin ())
    {
      // Before the first inserted event: base tempo (constant) from tick 0.
      return tick_q / base_bpm_;
    }

  size_t              index = std::distance (events_.begin (), it) - 1;
  const auto         &startEvent = events_[index];
  const units::tick_t segmentStart = startEvent.tick;
  const auto          ticksFromStart =
    tick_q - static_cast<units::precise_tick_t> (segmentStart);
  const auto baseSeconds = cumulative_seconds_[index];

  // Last event segment
  if (index == events_.size () - 1)
    {
      return baseSeconds + ticksFromStart / startEvent.bpm;
    }

  const auto         &endEvent = events_[index + 1];
  const units::tick_t segmentTicks = endEvent.tick - segmentStart;
  const auto dSegmentTicks = static_cast<units::precise_tick_t> (segmentTicks);

  // Constant tempo segment
  if (startEvent.curve == CurveType::Constant)
    {
      return baseSeconds + ticksFromStart / startEvent.bpm;
    }
  // Linear tempo ramp
  else if (startEvent.curve == CurveType::Linear)
    {
      const auto bpm0 = startEvent.bpm;
      const auto bpm1 = endEvent.bpm;

      if (abs (bpm1 - bpm0) < units::bpm (1e-5))
        {
          return baseSeconds + ticksFromStart / bpm0;
        }

      const auto fraction = ticksFromStart / dSegmentTicks;
      const auto currentBpm = bpm0 + fraction * (bpm1 - bpm0);

      return baseSeconds
             + dSegmentTicks / (bpm1 - bpm0) * std::log (currentBpm / bpm0);
    }

  return baseSeconds;
}

template <units::tick_t::NTTP PPQ>
TimelineTick
FixedPpqTempoMap<PPQ>::seconds_to_tick (units::precise_second_t seconds) const
{
  return TimelineTick{ [&] () -> units::precise_tick_t {
    if (seconds <= units::seconds (0.0))
      return units::ticks (0.0);

    // No inserted events: base tempo (constant) over the whole timeline.
    if (events_.empty ())
      return seconds * base_bpm_;

    // Lead base segment: before the first inserted event's cumulative time.
    if (seconds < cumulative_seconds_[0])
      return seconds * base_bpm_;

    // Find the segment containing the time
    auto         it = std::ranges::upper_bound (cumulative_seconds_, seconds);
    const size_t index =
      (it == cumulative_seconds_.begin ())
        ? 0
        : std::distance (cumulative_seconds_.begin (), it) - 1;

    const auto        baseSeconds = cumulative_seconds_[index];
    const auto        timeInSegment = seconds - baseSeconds;
    const TempoEvent &startEvent = events_[index];

    // Last segment
    if (index == events_.size () - 1)
      {
        return static_cast<units::precise_tick_t> (startEvent.tick)
               + timeInSegment * startEvent.bpm;
      }

    const TempoEvent   &endEvent = events_[index + 1];
    const units::tick_t segmentTicks = endEvent.tick - startEvent.tick;
    const auto dSegmentTicks = static_cast<units::precise_tick_t> (segmentTicks);

    // Constant tempo segment
    if (startEvent.curve == CurveType::Constant)
      {
        return static_cast<units::precise_tick_t> (startEvent.tick)
               + timeInSegment * startEvent.bpm;
      }
    // Linear tempo ramp
    else if (startEvent.curve == CurveType::Linear)
      {
        const auto bpm0 = startEvent.bpm;
        const auto bpm1 = endEvent.bpm;

        if (abs (bpm1 - bpm0) < units::bpm (1e-5))
          {
            return static_cast<units::precise_tick_t> (startEvent.tick)
                   + timeInSegment * bpm0;
          }

        // Coerce the mixed-unit product to plain ticks before dividing, so the
        // ratio is unitless (magnitude 1) and usable directly with std::exp.
        const auto exponent = au::as_raw_number (
          (timeInSegment * (bpm1 - bpm0)).as (units::ticks) / dSegmentTicks);
        const auto expVal = std::exp (exponent);
        const auto f = (expVal - 1.0) * (bpm0 / (bpm1 - bpm0));

        return static_cast<units::precise_tick_t> (startEvent.tick)
               + f * dSegmentTicks;
      }

    return static_cast<units::precise_tick_t> (startEvent.tick);
  }() };
}

template <units::tick_t::NTTP PPQ>
auto
FixedPpqTempoMap<PPQ>::time_signature_at_tick (units::tick_t tick) const
  -> TimeSignatureEvent
{
  if (time_sig_events_.empty ())
    return base_time_sig_;

  // Find the last time signature change <= tick
  auto it = std::ranges::upper_bound (
    time_sig_events_, tick, {}, &TimeSignatureEvent::tick);
  if (it == time_sig_events_.begin ())
    {
      // Before the first inserted event: base time signature.
      return base_time_sig_;
    }
  --it;

  return *it;
}

template <units::tick_t::NTTP PPQ>
auto
FixedPpqTempoMap<PPQ>::tempo_at_tick (units::tick_t tick) const -> units::bpm_t
{
  if (events_.empty ())
    return base_bpm_;

  // Find the last tempo change <= tick
  auto it = std::ranges::upper_bound (events_, tick, {}, &TempoEvent::tick);
  if (it == events_.begin ())
    {
      // Before the first inserted event: base tempo.
      return base_bpm_;
    }
  --it;

  // If this is the last event or constant, return as-is
  if (it == events_.end () - 1 || (it)->curve == CurveType::Constant)
    {
      return it->bpm;
    }

  // Handle linear ramp segment
  const auto         &startEvent = *it;
  const auto         &endEvent = *(it + 1);
  const units::tick_t segmentTicks = endEvent.tick - startEvent.tick;
  const auto          fraction =
    static_cast<units::precise_tick_t> (tick - startEvent.tick)
    / static_cast<units::precise_tick_t> (segmentTicks);
  const auto currentBpm =
    startEvent.bpm + fraction * (endEvent.bpm - startEvent.bpm);

  return currentBpm;
}

template <units::tick_t::NTTP PPQ>
auto
FixedPpqTempoMap<PPQ>::tick_to_musical_position (units::tick_t tick) const
  -> MusicalPosition
{
  const auto time_sig_events = effective_time_signature_events ();

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
  const double quarters_per_bar = numerator * (4.0 / denominator);
  const auto   ticks_per_bar = quarters_per_bar * get_ppq ();
  const auto   ticks_per_beat = ticks_per_bar / numerator;

  // Calculate absolute bar number
  int64_t cumulative_bars = 1;

  // Calculate total bars from previous time signatures
  for (auto prev = time_sig_events.begin (); prev != it; ++prev)
    {
      const int    prev_numerator = prev->numerator;
      const int    prev_denominator = prev->denominator;
      const double prev_quarters_per_bar =
        prev_numerator * (4.0 / prev_denominator);
      const auto prev_ticks_per_bar = prev_quarters_per_bar * get_ppq ();

      // Ticks from this signature to next
      auto       next = std::ranges::next (prev);
      const auto end_tick =
        (next != time_sig_events.end ()) ? next->tick : sigEvent.tick;
      const auto segment_ticks = end_tick - prev->tick;

      cumulative_bars +=
        static_cast<int64_t> (segment_ticks / prev_ticks_per_bar);
    }

  // Calculate bars since current signature
  const auto ticks_since_sig = tick - sigEvent.tick;
  const auto bar =
    cumulative_bars + static_cast<int64_t> (ticks_since_sig / ticks_per_bar);

  // Calculate position within current bar (integer tick arithmetic)
  const auto ticks_per_bar_int =
    au::floor_as<int64_t> (units::ticks, ticks_per_bar);
  const auto ticks_per_beat_int =
    au::floor_as<int64_t> (units::ticks, ticks_per_beat);
  const auto ticks_since_sig_int =
    au::floor_as<int64_t> (units::ticks, ticks_since_sig);

  const auto ticks_in_bar = ticks_since_sig_int % ticks_per_bar_int;
  const auto beat =
    1
    + au::as_raw_number (ticks_in_bar / au::unblock_int_div (ticks_per_beat_int));

  const auto ticks_in_beat = ticks_in_bar % ticks_per_beat_int;
  const auto sixteenth =
    1
    + au::as_raw_number (
      ticks_in_beat / au::unblock_int_div (ticks_per_sixteenth_));
  const auto tick_in_sixteenth = ticks_in_beat % ticks_per_sixteenth_;

  return {
    static_cast<int> (bar), static_cast<int> (beat),
    static_cast<int> (sixteenth),
    static_cast<int> (tick_in_sixteenth.in (units::ticks))
  };
}

template <units::tick_t::NTTP PPQ>
auto
FixedPpqTempoMap<PPQ>::samples_to_musical_position (
  units::sample_t samples) const -> MusicalPosition
{
  // Note: we are using `floor()` because we never want the MusicalPosition to
  // be after the given samples
  const auto tick = au::floor_as<int64_t> (
    units::ticks,
    samples_to_tick (static_cast<units::precise_sample_t> (samples))
      .asQuantity ());
  return tick_to_musical_position (tick);
}

template <units::tick_t::NTTP PPQ>
TimelineTickI
FixedPpqTempoMap<PPQ>::musical_position_to_tick (const MusicalPosition &pos) const
{
  const auto time_sig_events = effective_time_signature_events ();

  // Validate position
  if (pos.bar < 1 || pos.beat < 1 || pos.sixteenth < 1 || pos.tick < 0)
    {
      throw std::invalid_argument ("Invalid musical position");
    }

  auto cumulative_ticks = units::ticks (0);
  int  current_bar = 1;

  // Iterate through time signature changes
  for (size_t i = 0; i < time_sig_events.size (); ++i)
    {
      const auto   &event = time_sig_events[i];
      const int     numerator = event.numerator;
      const int     denominator = event.denominator;
      const double  quarters_per_bar = numerator * (4.0 / denominator);
      const int64_t ticks_per_bar = static_cast<int64_t> (
        (quarters_per_bar * get_ppq ()).in (units::ticks));

      // Determine bars covered by this time signature
      auto bars_in_this_sig = units::ticks (0);
      if (i < time_sig_events.size () - 1)
        {
          const units::tick_t next_tick = time_sig_events[i + 1].tick;
          bars_in_this_sig = (next_tick - event.tick) / ticks_per_bar;
        }
      else
        {
          bars_in_this_sig = units::ticks (pos.bar - current_bar + 1);
        }

      // Check if position falls in this time signature segment
      if (pos.bar < current_bar + bars_in_this_sig.in (units::ticks))
        {
          const int  bar_in_seg = pos.bar - current_bar;
          const auto bar_ticks =
            event.tick + units::ticks (bar_in_seg * ticks_per_bar);
          const int64_t ticks_per_beat = ticks_per_bar / numerator;

          // Add beat and sub-beat components
          return TimelineTickI{
            bar_ticks
            + units::ticks (static_cast<int64_t> (pos.beat - 1) * ticks_per_beat)
            + units::ticks (
              static_cast<int64_t> (pos.sixteenth - 1)
              * ticks_per_sixteenth_.in (units::ticks))
            + units::ticks (pos.tick)
          };
        }

      // Move to next time signature segment
      cumulative_ticks += bars_in_this_sig * ticks_per_bar;
      current_bar += bars_in_this_sig.in (units::ticks);
    }

  return TimelineTickI{ cumulative_ticks };
}

template <units::tick_t::NTTP PPQ>
void
FixedPpqTempoMap<PPQ>::rebuild_cumulative_times ()
{
  if (events_.empty ())
    return;

  // Sort events by tick
  std::ranges::sort (events_, {}, &TempoEvent::tick);

  cumulative_seconds_.resize (events_.size ());

  // Lead segment: constant base tempo from tick 0 to the first inserted event.
  // Only applies when the first event is not at tick 0 (otherwise that event
  // governs from tick 0 and there is no lead segment).
  if (events_[0].tick > units::ticks (0))
    cumulative_seconds_[0] =
      static_cast<units::precise_tick_t> (events_[0].tick) / base_bpm_;
  else
    cumulative_seconds_[0] = units::seconds (0.0);

  // Compute cumulative time at each event point
  for (size_t i = 0; i < events_.size () - 1; ++i)
    {
      const units::tick_t segmentTicks = events_[i + 1].tick - events_[i].tick;
      cumulative_seconds_[i + 1] =
        cumulative_seconds_[i]
        + compute_segment_time (events_[i], events_[i + 1], segmentTicks);
    }
}

template <units::tick_t::NTTP PPQ>
void
FixedPpqTempoMap<PPQ>::throw_if_invalid_time_signature (
  int numerator,
  int denominator)
{
  if (numerator < 1)
    throw std::invalid_argument ("Time signature numerator must be >= 1");
  // denominator must be a power of two in [1, 128].
  if (
    denominator < 1 || denominator > 128
    || (denominator & (denominator - 1)) != 0)
    throw std::invalid_argument (
      "Time signature denominator must be a power of two in [1, 128]");
}

template <units::tick_t::NTTP PPQ>
void
FixedPpqTempoMap<PPQ>::rebuild_time_signature_cache ()
{
  effective_time_sig_events_.clear ();
  if (
    time_sig_events_.empty ()
    || time_sig_events_.front ().tick > units::ticks (0))
    effective_time_sig_events_.push_back (base_time_sig_);
  for (const auto &e : time_sig_events_)
    effective_time_sig_events_.push_back (e);
}

template <units::tick_t::NTTP PPQ>
units::precise_second_t
FixedPpqTempoMap<PPQ>::compute_segment_time (
  const TempoEvent &start,
  const TempoEvent &end,
  units::tick_t     segmentTicks) const
{
  if (start.curve == CurveType::Constant)
    {
      return static_cast<units::precise_tick_t> (segmentTicks) / start.bpm;
    }
  if (start.curve == CurveType::Linear)
    {
      const auto bpm0 = start.bpm;
      const auto bpm1 = end.bpm;

      if (abs (bpm1 - bpm0) < units::bpm (1e-5))
        {
          return static_cast<units::precise_tick_t> (segmentTicks) / bpm0;
        }

      return static_cast<units::precise_tick_t> (segmentTicks) / (bpm1 - bpm0)
             * std::log (bpm1 / bpm0);
    }
  return units::seconds (0.0);
}

template class FixedPpqTempoMap<units::PPQ>;

} // namespace zrythm::dsp
