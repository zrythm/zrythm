// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <unordered_map>
#include <vector>

#include "controllers/recording_coordinator.h"
#include "utils/logger.h"
#include "utils/qt.h"

#include <QTimer>

namespace zrythm::controllers
{

struct RecordingCoordinator::Impl
{
  std::unordered_map<
    structure::tracks::TrackUuid,
    std::unique_ptr<AudioRecordingSession>>
    sessions;

  std::unordered_map<AudioRecordingSession *, uint64_t> last_reported_dropped;

  std::vector<std::pair<
    structure::tracks::TrackUuid,
    std::unique_ptr<AudioRecordingSession>>>
    pending_deletion;

  utils::QObjectUniquePtr<QTimer> timer;
};

RecordingCoordinator::RecordingCoordinator (QObject * parent)
    : QObject (parent), impl_ (std::make_unique<Impl> ())
{
  rt_snapshot_.store (new Snapshot (), std::memory_order_release);

  impl_->timer = utils::make_qobject_unique<QTimer> (this);
  QObject::connect (
    impl_->timer.get (), &QTimer::timeout, this,
    &RecordingCoordinator::process_pending);
}

RecordingCoordinator::~RecordingCoordinator ()
{
  impl_->timer->stop ();
  delete rt_snapshot_.load (std::memory_order_acquire);
}

void
RecordingCoordinator::arm_track (
  structure::tracks::TrackUuid track_id,
  units::sample_u32_t          max_block_length)
{
  if (impl_->sessions.contains (track_id))
    return;

  impl_->sessions.emplace (
    track_id, std::make_unique<AudioRecordingSession> (max_block_length));
  publish_snapshot ();
  impl_->timer->start (kDrainInterval);
}

void
RecordingCoordinator::disarm_track (structure::tracks::TrackUuid track_id)
{
  auto it = impl_->sessions.find (track_id);
  if (it == impl_->sessions.end ())
    return;

  it->second->finalize ();
  impl_->last_reported_dropped.erase (it->second.get ());
  impl_->pending_deletion.emplace_back (track_id, std::move (it->second));
  impl_->sessions.erase (it);
  publish_snapshot ();
  impl_->timer->start (kDrainInterval);
}

bool
RecordingCoordinator::has_session (structure::tracks::TrackUuid track_id) const
{
  return impl_->sessions.contains (track_id);
}

AudioRecordingSession *
RecordingCoordinator::session_for_track (
  structure::tracks::TrackUuid track_id) const noexcept
{
  const auto * snap = rt_snapshot_.load (std::memory_order_acquire);
  if (snap == nullptr)
    return nullptr;

  auto it = snap->find (track_id);
  return it != snap->end () ? it->second : nullptr;
}

void
RecordingCoordinator::prepare_for_processing (units::sample_u32_t block_length)
{
  for (auto &[track_id, session] : impl_->sessions)
    {
      session->prepare_for_processing (block_length);
    }
}

void
RecordingCoordinator::process_pending ()
{
  DrainResult ready;

  for (auto &[id, session] : impl_->sessions)
    {
      auto packets = session->drain_pending ();
      if (!packets.empty ())
        {
          ready.emplace_back (id, std::move (packets));
        }

      const auto dropped = session->dropped_packets ();
      auto       drop_it = impl_->last_reported_dropped.find (session.get ());
      const auto prev =
        (drop_it != impl_->last_reported_dropped.end ()) ? drop_it->second : 0;
      if (dropped > prev)
        {
          impl_->last_reported_dropped[session.get ()] = dropped;
          z_warning (
            "Recording session for ID {} dropped {} packets ({} new) due to FIFO overflow",
            id, dropped, dropped - prev);
        }
    }

  const bool had_pending = drain_pending_deletion (ready);

  old_snapshots_.swap (pending_snapshot_deletion_);

  for (auto &[id, packets] : ready)
    {
      Q_EMIT audioDataReady (id, std::move (packets));
    }

  if (impl_->sessions.empty ())
    {
      impl_->timer->stop ();
      if (had_pending)
        {
          Q_EMIT recordingSessionEnded ();
        }
    }
  else
    impl_->timer->start (kDrainInterval);
}

bool
RecordingCoordinator::drain_pending_deletion (DrainResult &ready)
{
  for (auto &[track_id, session] : impl_->pending_deletion)
    {
      auto packets = session->drain_pending ();
      if (!packets.empty ())
        {
          ready.emplace_back (track_id, std::move (packets));
        }
    }
  const bool had_pending = !impl_->pending_deletion.empty ();
  impl_->pending_deletion.clear ();
  return had_pending;
}

void
RecordingCoordinator::publish_snapshot ()
{
  auto new_snap = std::make_unique<Snapshot> ();
  for (const auto &[id, session] : impl_->sessions)
    {
      new_snap->emplace (id, session.get ());
    }

  auto * old =
    rt_snapshot_.exchange (new_snap.release (), std::memory_order_acq_rel);
  if (old != nullptr)
    {
      pending_snapshot_deletion_.emplace_back (std::unique_ptr<Snapshot>{ old });
    }
}

void
RecordingCoordinator::finalizeAllSessions ()
{
  DrainResult ready;

  for (auto &[id, session] : impl_->sessions)
    {
      auto packets = session->drain_pending ();
      if (!packets.empty ())
        {
          ready.emplace_back (id, std::move (packets));
        }
      impl_->last_reported_dropped.erase (session.get ());
      // Safe: transport is already Paused so no RT write_samples() is in
      // flight (audio callbacks only write when Rolling).
      session->reset ();
    }

  // Also drain pending-deletion sessions to avoid data loss for
  // recently-disarmed tracks.
  const bool had_pending = drain_pending_deletion (ready);

  old_snapshots_.swap (pending_snapshot_deletion_);

  for (auto &[id, packets] : ready)
    {
      Q_EMIT audioDataReady (id, std::move (packets));
    }

  // recordingSessionEnded is intentionally emitted whenever any recording
  // activity occurred — even if all tracks were disarmed before this call
  // (had_pending is true) and sessions are now empty. An arm/disarm cycle
  // constitutes a completed session lifecycle that consumers must observe.
  if (!impl_->sessions.empty () || had_pending)
    {
      Q_EMIT recordingSessionEnded ();
    }
}

}
