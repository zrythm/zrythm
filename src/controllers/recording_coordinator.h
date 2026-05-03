// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <chrono>
#include <memory>
#include <unordered_map>

#include "controllers/audio_recording_session.h"
#include "structure/tracks/track_fwd.h"

#include <QObject>

#include <farbot/RealtimeObject.hpp>

namespace zrythm::controllers
{

/**
 * @brief Orchestrates the recording lifecycle across all armed tracks.
 *
 * Owns per-track AudioRecordingSession objects and runs a periodic timer that
 * drains recorded audio from each session's ring buffer for region creation.
 *
 * @section thread_safety Thread Safety
 *
 * Session access from the RT thread is mediated by a farbot::RealtimeObject
 * snapshot (rt_snapshot_). The RT thread reads session pointers from the
 * snapshot via a ScopedAccess; the non-RT thread publishes updated snapshots
 * after arm/disarm operations.  This guarantees lock-free, allocation-free RT
 * reads.
 *
 * Destroyed sessions are deferred: disarm_track() moves sessions to a pending
 * deletion list and restarts the drain timer, guaranteeing that
 * process_pending() will not clear the list for at least kDrainInterval
 * (currently 100 ms). Since audio callbacks complete in ~1-10 ms, the RT
 * thread has long since released its ScopedAccess before any session is
 * destroyed.
 *
 * @note process_pending() may be called manually only when it is guaranteed
 * that no RT processing is occurring simultaneously (e.g., when the audio
 * engine is stopped). Otherwise, pending sessions could be destroyed while
 * the RT thread still holds a reference.
 */
class RecordingCoordinator : public QObject
{
  Q_OBJECT

public:
  static constexpr auto kDrainInterval = std::chrono::milliseconds (100);

  explicit RecordingCoordinator (QObject * parent = nullptr);
  ~RecordingCoordinator () override;

  Q_DISABLE_COPY_MOVE (RecordingCoordinator)

  /**
   * @brief Arms a track for recording. Creates an AudioRecordingSession.
   *
   * Must be called from the non-RT thread.
   */
  void arm_track (
    structure::tracks::TrackUuid track_id,
    units::sample_u32_t          max_block_length);

  /**
   * @brief Disarms a track. Moves session to pending deletion.
   *
   * Must be called from the non-RT thread.
   */
  void disarm_track (structure::tracks::TrackUuid track_id);

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
   * @brief Gets the session for a track (for RT callback access).
   *
   * Returns nullptr if no session exists.
   */
  [[nodiscard]] AudioRecordingSession *
  session_for_track (structure::tracks::TrackUuid track_id) noexcept
    [[clang::nonblocking]];

private:
  using Snapshot =
    std::unordered_map<structure::tracks::TrackUuid, AudioRecordingSession *>;

  void publish_snapshot ();

  farbot::
    RealtimeObject<Snapshot, farbot::RealtimeObjectOptions::nonRealtimeMutatable>
      rt_snapshot_;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
