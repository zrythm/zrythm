// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

#include "controllers/recording_session.h"
#include "structure/tracks/track_fwd.h"

#include <QObject>

namespace zrythm::controllers
{

/**
 * @brief Orchestrates the recording lifecycle across all armed tracks.
 *
 * Owns per-track recording sessions (audio or MIDI) and runs a periodic timer
 * that drains recorded data from each session's ring buffer for clip
 * creation.
 *
 * @section thread_safety Thread Safety
 *
 * Session access from RT threads is mediated by an atomic snapshot pointer
 * (rt_snapshot_). The non-RT thread publishes new snapshots via atomic
 * exchange after arm/disarm operations; RT threads read via atomic load.
 * This guarantees lock-free, allocation-free RT reads and supports multiple
 * concurrent RT readers (the DSP graph processes nodes in parallel).
 *
 * Old snapshots are deferred: publish_snapshot() moves the previous snapshot
 * to a pending-deletion list. process_pending() clears that list, which runs
 * at least kDrainInterval (100 ms) after the last publish. Since audio
 * callbacks complete in ~1-10 ms, all RT threads have long since loaded a
 * newer snapshot before any old snapshot is destroyed.
 *
 * @note process_pending() may be called manually only when it is guaranteed
 * that no RT processing is occurring simultaneously (e.g., when the audio
 * engine is stopped). Otherwise, pending snapshots could be destroyed while
 * an RT thread still holds a pointer.
 */
class RecordingCoordinator : public QObject
{
  Q_OBJECT

public:
  static constexpr auto kDrainInterval = std::chrono::milliseconds (100);

  explicit RecordingCoordinator (QObject * parent = nullptr);
  ~RecordingCoordinator () override;

  Q_DISABLE_COPY_MOVE (RecordingCoordinator)

  enum class SessionType : std::uint8_t
  {
    Audio,
    Midi,
  };

  using SessionHandle =
    std::variant<std::monostate, AudioRecordingSession *, MidiRecordingSession *>;

  /**
   * @brief Arms a track for recording.
   *
   * Creates a session of the given type. A track can only have one session
   * at a time. Must be called from the non-RT thread.
   */
  void arm_track (
    structure::tracks::TrackUuid track_id,
    units::sample_u32_t          max_block_length,
    SessionType                  type) [[clang::blocking]];

  /**
   * @brief Disarms a track. Moves session to pending deletion.
   *
   * Must be called from the non-RT thread.
   */
  void disarm_track (structure::tracks::TrackUuid track_id) [[clang::blocking]];

  /**
   * @brief Returns whether a session exists for the given track.
   *
   * Must be called from the same non-RT thread as arm_track()/disarm_track().
   */
  [[nodiscard]] bool has_session (structure::tracks::TrackUuid track_id) const;

  /**
   * @brief Drains all active sessions and processes pending deletions.
   *
   * Called by the internal timer. May be called manually only when no
   * RT processing is occurring simultaneously (e.g., engine stopped).
   */
  void process_pending ();

  /**
   * @brief Finalizes the current recording take and resets sessions for reuse.
   *
   * Drains all pending audio data from active sessions, emits audioDataReady
   * for each track with data, then emits recordingSessionEnded to finalize the
   * undo macro. Sessions are reset to Armed state rather than destroyed, so
   * they accept new writes when recording resumes (e.g., after transport
   * restart).
   *
   * Intended to be called when transport stops. Sessions are only destroyed
   * via disarm_track().
   *
   * Must be called from the non-RT thread.
   */
  Q_SLOT void finalizeAllSessions ();

  /**
   * @brief Gets the session for a track (for RT callback access).
   *
   * Safe to call from multiple RT threads concurrently.
   * Returns std::monostate if no session exists.
   */
  [[nodiscard]] SessionHandle
  session_for_track (structure::tracks::TrackUuid track_id) const noexcept
    [[clang::nonblocking]];

  /**
   * @brief Prepares all active sessions for processing at the given block
   * length.
   *
   * Buffers are only grown, never shrunk. Must be called before audio
   * processing starts (not concurrently with write_samples() or
   * drain_pending()).
   */
  void prepare_for_processing (units::sample_u32_t block_length);

Q_SIGNALS:

  /**
   * @brief Emitted when recorded audio data has been drained from a session.
   *
   * Connected consumers (e.g. RecordingMaterializer) use this to create or
   * expand clips from the recorded audio packets.
   */
  void audioDataReady (
    structure::tracks::TrackUuid      track_id,
    std::vector<RecordingAudioPacket> packets);

  /**
   * @brief Emitted when recorded MIDI data has been drained from a session.
   *
   * Connected consumers (e.g. RecordingMaterializer) use this to create
   * MidiClip objects from the recorded MIDI packets.
   */
  void midiDataReady (
    structure::tracks::TrackUuid     track_id,
    std::vector<RecordingMidiPacket> packets);

  /**
   * @brief Emitted when a recording take has been finalized.
   *
   * Fires from finalizeAllSessions() after all pending audio data is drained
   * and sessions are reset to Armed state. Consumers should finalize any
   * open recording context (e.g. closing an undo macro) when this signal
   * is received — no more audio data will arrive for this take.
   * Sessions remain alive and can accept new writes when recording resumes.
   */
  void recordingSessionEnded ();

private:
  struct RtSnapshot
  {
    std::unordered_map<structure::tracks::TrackUuid, AudioRecordingSession *>
      audio;
    std::unordered_map<structure::tracks::TrackUuid, MidiRecordingSession *> midi;
  };

  using AudioDrainResult = std::vector<
    std::pair<structure::tracks::TrackUuid, std::vector<RecordingAudioPacket>>>;

  using MidiDrainResult = std::vector<
    std::pair<structure::tracks::TrackUuid, std::vector<RecordingMidiPacket>>>;

  void publish_snapshot ();

  bool drain_pending_deletion (
    AudioDrainResult &audio_ready,
    MidiDrainResult  &midi_ready);

  // TODO: Replace this raw atomic pointer with a proper multi-reader
  // realtime-safe container.  The current approach works but is ugly:
  // publish_snapshot() heap-allocates a new RtSnapshot, atomically swaps the
  // pointer, and defers the old RtSnapshot's deletion to
  // pending_snapshot_deletion_.  Those deferred snapshots are freed in
  // process_pending(), which runs at least kDrainInterval (100 ms) after
  // the last publish — long after any RT thread has finished reading the
  // old snapshot.  The initial snapshot (created in the constructor) and
  // any remaining pending snapshots are freed in the destructor.
  std::atomic<RtSnapshot *> rt_snapshot_{ nullptr };

  // Two-phase deferred deletion for retired snapshots:
  //   1. publish_snapshot() pushes the old snapshot into
  //      pending_snapshot_deletion_.
  //   2. process_pending() swaps pending_snapshot_deletion_ into
  //      old_snapshots_ (destroying what was previously there) and emits
  //      signals.  Each retired snapshot therefore survives at least two
  //      drain intervals (~200 ms), guaranteeing every RT reader has moved
  //      on to a newer snapshot before the old one is freed.
  std::vector<std::unique_ptr<RtSnapshot>> pending_snapshot_deletion_;
  std::vector<std::unique_ptr<RtSnapshot>> old_snapshots_;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
