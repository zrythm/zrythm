// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/itransport.h"
#include "dsp/tempo_map.h"
#include "dsp/tick_types.h"

namespace zrythm::plugins
{

/**
 * @brief Host transport state at the start of a process block, in the shape
 * plugin formats expect.
 *
 * Derived per block from the ITransport view and the tempo map (no state of
 * its own) and mapped by plugin hosts onto format-specific structures (VST3
 * `ProcessContext`, CLAP `clap_event_transport_t`).
 */
struct PluginTransportContext
{
  /** Whether the transport is rolling. */
  bool playing_{};

  /** Whether recording is enabled. */
  bool recording_{};

  /** Whether the transport is within a recording preroll. */
  bool within_preroll_{};

  /** Whether looping is active. */
  bool loop_enabled_{};

  /** Playhead position. */
  units::quarter_note_t   position_{};
  units::precise_second_t position_seconds_{};

  /** Start of the current bar. */
  units::quarter_note_t bar_start_{};

  /** Current bar number (0-based; the bar at position 0 is 0). */
  int bar_number_{};

  /** Current tempo. */
  units::bpm_t tempo_{};

  /** Current time signature (e.g. 3/4). */
  int time_sig_numerator_{};
  int time_sig_denominator_{};

  /** Loop range (always filled; loop_enabled_ says whether it is active). */
  units::quarter_note_t   loop_start_{};
  units::quarter_note_t   loop_end_{};
  units::precise_second_t loop_start_seconds_{};
  units::precise_second_t loop_end_seconds_{};
};

/**
 * @brief Derives the plugin-facing transport context for the start of a
 * process block.
 *
 * @param transport Transport view for the current process cycle.
 * @param tempo_map Project tempo map.
 * @param transport_position Playhead position at the start of the block.
 */
[[gnu::hot]] inline PluginTransportContext
build_plugin_transport_context (
  const dsp::ITransport &transport,
  const dsp::TempoMap   &tempo_map,
  units::sample_t        transport_position) noexcept [[clang::nonblocking]]
{
  const auto sample_rate = tempo_map.get_sample_rate ();
  const auto to_precise_samples = [] (units::sample_t samples) {
    return units::precise_sample_t{ samples };
  };
  const auto to_quarters =
    [&tempo_map, &to_precise_samples] (units::sample_t samples) {
      return tempo_map.samples_to_tick (to_precise_samples (samples))
        .asQuantity ()
        .as (units::quarter_notes);
    };

  const auto position_ticks_precise =
    tempo_map.samples_to_tick (to_precise_samples (transport_position))
      .asQuantity ();
  const auto position_quarters =
    position_ticks_precise.as (units::quarter_notes);
  // Floor (not round) for the integer-tick lookups below, matching the
  // convention of TempoMap::samples_to_musical_position: the looked-up
  // bar/tempo/time signature must never be ahead of the given position
  const auto position_tick =
    au::floor_as<int64_t> (units::ticks, position_ticks_precise);

  PluginTransportContext context;
  context.playing_ =
    transport.get_play_state () == dsp::ITransport::PlayState::Rolling;
  context.recording_ = transport.recording_enabled ();
  context.within_preroll_ = transport.has_recording_preroll_frames_remaining ();
  context.loop_enabled_ = transport.loop_enabled ();

  context.position_ = position_quarters;
  context.position_seconds_ =
    to_precise_samples (transport_position) / sample_rate;

  const auto musical_position =
    tempo_map.tick_to_musical_position (position_tick);
  // The bar comes from tick_to_musical_position, so it is always a valid
  // (>= 1) bar in the map and this conversion never throws
  const units::precise_tick_t bar_start_ticks =
    tempo_map
      .musical_position_to_tick (
        dsp::TempoMap::MusicalPosition{
          .bar = musical_position.bar, .beat = 1, .sixteenth = 1, .tick = 0 })
      .asQuantity ();
  context.bar_start_ = bar_start_ticks.as (units::quarter_notes);
  context.bar_number_ = musical_position.bar - 1;

  context.tempo_ = tempo_map.tempo_at_tick (position_tick);
  const auto time_sig = tempo_map.time_signature_at_tick (position_tick);
  context.time_sig_numerator_ = time_sig.numerator;
  context.time_sig_denominator_ = time_sig.denominator;

  const auto [loop_start, loop_end] = transport.get_loop_range_positions ();
  context.loop_start_ = to_quarters (loop_start);
  context.loop_end_ = to_quarters (loop_end);
  context.loop_start_seconds_ = to_precise_samples (loop_start) / sample_rate;
  context.loop_end_seconds_ = to_precise_samples (loop_end) / sample_rate;

  return context;
}

} // namespace zrythm::plugins
