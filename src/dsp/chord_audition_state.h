// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/chord_descriptor.h"
#include "dsp/graph_node.h"
#include "dsp/midi_event_buffer.h"

#include <boost/container/static_vector.hpp>
#include <farbot/RealtimeObject.hpp>

namespace zrythm::dsp
{

/**
 * @brief Realtime-safe chord audition state machine with polyphonic support.
 *
 * Supports multiple simultaneous chords (e.g., triggering from different pads
 * or an external device). Pitches shared between chords are reference-counted
 * so that stopping one chord doesn't cut off a pitch still used by another.
 *
 * Retrigger behavior:
 * When the active pitch set changes, process() sends note-offs for all
 * previously sounding pitches and note-ons for all currently desired pitches,
 * even pitches that are present in both sets. This "full retrigger" design is
 * intentional — it ensures a clean envelope reset on every chord change.
 *
 * Shared pitch velocity:
 * When multiple active chords share a pitch, the velocity from whichever
 * chord was started first wins. This self-corrects when any chord stops,
 * because the full retrigger resends all remaining pitches with their correct
 * velocities.
 *
 * Thread safety:
 * - `start()` and `stop()` are called from the main thread (non-realtime).
 * - `process()` is called from the audio thread (realtime).
 * - Active pitches are shared via farbot::RealtimeObject (lock-free).
 */
class ChordAuditionState
{
public:
  static constexpr midi_byte_t kDefaultVelocity = 100;

  struct PitchEntry
  {
    std::uint8_t pitch;
    std::uint8_t velocity;
    bool         operator== (const PitchEntry &) const = default;
  };

  /**
   * @brief Merged pitch set shared between main and audio threads.
   *
   * Sized for the full MIDI pitch range (128 unique pitches).
   */
  using ActivePitches = boost::container::static_vector<PitchEntry, 128>;

  /**
   * @brief Start auditioning a chord.
   *
   * Main thread only. Adds the chord's pitches to the active set.
   * Can be called multiple times for different (or same) chords.
   *
   * @param velocity MIDI velocity to use (defaults to kDefaultVelocity).
   */
  void start (
    const ChordDescriptor &descriptor,
    midi_byte_t            velocity = kDefaultVelocity);

  /**
   * @brief Stop auditioning a chord.
   *
   * Main thread only. Removes one matching chord entry from the active set.
   * Shared pitches still used by other active chords will remain sounding.
   * If no matching chord is found, this is a no-op.
   */
  void stop (const ChordDescriptor &descriptor);

  /**
   * @brief Stop auditioning by exact pitches.
   *
   * Main thread only. As above, but matches on pitches directly, so a caller
   * does not need to keep a (non-copyable) ChordDescriptor around.
   */
  void stop (const ChordDescriptor::ChordPitches &pitches);

  /**
   * @brief Stop all auditioning.
   *
   * Main thread only. Clears all active pitches.
   */
  void stopAll ();

  /**
   * @brief Process audition state and generate MIDI events.
   *
   * Audio thread only. Diffs current active pitches against what is sounding.
   * Sends note-offs for removed pitches and note-ons for added pitches.
   */
  void process (
    MidiEventBuffer               &out_events,
    const graph::ProcessBlockInfo &time_nfo) noexcept [[clang::nonblocking]];

private:
  /**
   * @brief A chord currently being auditioned, with its velocity.
   */
  struct ActiveChordEntry
  {
    ChordDescriptor::ChordPitches pitches;
    midi_byte_t                   velocity;
  };

  /**
   * @brief Rebuild the merged active pitch set from active chords.
   *
   * Main thread only. Called after each start()/stop() mutation.
   */
  void rebuild_active_pitches ();

  static constexpr size_t kMaxActiveChords = 32;

  /**
   * @brief Active chord entries (main thread only).
   *
   * Each start() appends an entry; stop() removes the last matching one.
   */
  boost::container::static_vector<ActiveChordEntry, kMaxActiveChords>
    active_chords_;

  /**
   * @brief Merged active pitch set, written from main thread, read from audio.
   */
  farbot::RealtimeObject<
    ActivePitches,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    active_pitches_;

  /**
   * @brief Pitches currently sounding (audio thread only).
   */
  ActivePitches sounding_pitches_;
};

} // namespace zrythm::dsp
