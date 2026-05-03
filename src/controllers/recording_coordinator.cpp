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

  std::vector<std::unique_ptr<AudioRecordingSession>> pending_deletion;

  utils::QObjectUniquePtr<QTimer> timer;
};

RecordingCoordinator::RecordingCoordinator (QObject * parent)
    : QObject (parent), impl_ (std::make_unique<Impl> ())
{
  impl_->timer = utils::make_qobject_unique<QTimer> (this);
  QObject::connect (
    impl_->timer.get (), &QTimer::timeout, this,
    &RecordingCoordinator::process_pending);
}

RecordingCoordinator::~RecordingCoordinator () = default;

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
  impl_->pending_deletion.push_back (std::move (it->second));
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
  structure::tracks::TrackUuid track_id) noexcept
{
  decltype (rt_snapshot_)::ScopedAccess<farbot::ThreadType::realtime> snap{
    rt_snapshot_
  };
  auto it = snap->find (track_id);
  return it != snap->end () ? it->second : nullptr;
}

void
RecordingCoordinator::process_pending ()
{
  for (auto &[id, session] : impl_->sessions)
    {
      // TODO: process drained packets (create/append regions)
      std::ignore = session->drain_pending ();

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

  for (auto &session : impl_->pending_deletion)
    {
      // TODO: process drained packets (create/append regions)
      std::ignore = session->drain_pending ();
    }
  impl_->pending_deletion.clear ();

  if (impl_->sessions.empty ())
    impl_->timer->stop ();
  else
    impl_->timer->start (kDrainInterval);
}

void
RecordingCoordinator::publish_snapshot ()
{
  Snapshot snapshot;
  for (const auto &[id, session] : impl_->sessions)
    {
      snapshot.emplace (id, session.get ());
    }

  decltype (rt_snapshot_)::ScopedAccess<farbot::ThreadType::nonRealtime> snap{
    rt_snapshot_
  };
  *snap = std::move (snapshot);
}
}
