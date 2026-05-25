// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <unordered_map>
#include <variant>
#include <vector>

#include "controllers/recording_coordinator.h"
#include "utils/logger.h"
#include "utils/qt.h"

#include <QTimer>

namespace zrythm::controllers
{

namespace
{

template <typename SessionT> struct SessionPool
{
  using TrackUuid = structure::tracks::TrackUuid;
  using PacketT = typename SessionT::PacketType;
  using DrainResult = std::vector<std::pair<TrackUuid, std::vector<PacketT>>>;

  template <typename Fn> void for_each_session (Fn &&fn) const
  {
    for (const auto &[id, session] : sessions)
      {
        fn (id, *session);
      }
  }

  void arm (TrackUuid track_id, units::sample_u32_t max_block_length)
  {
    if (sessions.contains (track_id))
      return;
    sessions.emplace (track_id, std::make_unique<SessionT> (max_block_length));
  }

  bool disarm (TrackUuid track_id)
  {
    auto it = sessions.find (track_id);
    if (it == sessions.end ())
      return false;

    it->second->finalize ();
    last_reported_dropped.erase (it->second.get ());
    pending_deletion.emplace_back (track_id, std::move (it->second));
    sessions.erase (it);
    return true;
  }

  bool contains (TrackUuid track_id) const
  {
    return sessions.contains (track_id);
  }

  void prepare_for_processing (units::sample_u32_t block_length)
  {
    for (auto &[id, session] : sessions)
      {
        session->prepare_for_processing (block_length);
      }
  }

  DrainResult drain_active ()
  {
    DrainResult ready;
    for (auto &[id, session] : sessions)
      {
        auto packets = session->drain_pending ();
        if (!packets.empty ())
          {
            ready.emplace_back (id, std::move (packets));
          }

        const auto dropped = session->dropped_packets ();
        auto       drop_it = last_reported_dropped.find (session.get ());
        const auto prev =
          (drop_it != last_reported_dropped.end ()) ? drop_it->second : 0;
        if (dropped > prev)
          {
            last_reported_dropped[session.get ()] = dropped;
            z_warning (
              "Recording session for ID {} dropped {} packets ({} new) due to FIFO overflow",
              id, dropped, dropped - prev);
          }
      }
    return ready;
  }

  DrainResult drain_pending_deletion ()
  {
    DrainResult ready;
    for (auto &[track_id, session] : pending_deletion)
      {
        auto packets = session->drain_pending ();
        if (!packets.empty ())
          {
            ready.emplace_back (track_id, std::move (packets));
          }
      }
    pending_deletion.clear ();
    return ready;
  }

  void finalize_all (DrainResult &ready)
  {
    for (auto &[id, session] : sessions)
      {
        auto packets = session->drain_pending ();
        if (!packets.empty ())
          {
            ready.emplace_back (id, std::move (packets));
          }
        last_reported_dropped.erase (session.get ());
        session->reset ();
      }
  }

  bool has_pending_deletion () const { return !pending_deletion.empty (); }

  bool empty () const { return sessions.empty (); }

private:
  std::unordered_map<TrackUuid, std::unique_ptr<SessionT>> sessions;
  std::unordered_map<SessionT *, uint64_t> last_reported_dropped;
  std::vector<std::pair<TrackUuid, std::unique_ptr<SessionT>>> pending_deletion;
};

}

// ============================================================================

struct RecordingCoordinator::Impl
{
  SessionPool<AudioRecordingSession> audio;
  SessionPool<MidiRecordingSession>  midi;

  utils::QObjectUniquePtr<QTimer> timer;
};

RecordingCoordinator::RecordingCoordinator (QObject * parent)
    : QObject (parent), impl_ (std::make_unique<Impl> ())
{
  rt_snapshot_.store (new RtSnapshot (), std::memory_order_release);

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
  units::sample_u32_t          max_block_length,
  SessionType                  type)
{
  if (has_session (track_id))
    return;

  if (type == SessionType::Audio)
    {
      impl_->audio.arm (track_id, max_block_length);
    }
  else
    {
      impl_->midi.arm (track_id, max_block_length);
    }

  publish_snapshot ();
  impl_->timer->start (kDrainInterval);
}

void
RecordingCoordinator::disarm_track (structure::tracks::TrackUuid track_id)
{
  const bool disarmed =
    impl_->audio.disarm (track_id) || impl_->midi.disarm (track_id);
  if (!disarmed)
    return;

  publish_snapshot ();
  impl_->timer->start (kDrainInterval);
}

bool
RecordingCoordinator::has_session (structure::tracks::TrackUuid track_id) const
{
  return impl_->audio.contains (track_id) || impl_->midi.contains (track_id);
}

RecordingCoordinator::SessionHandle
RecordingCoordinator::session_for_track (
  structure::tracks::TrackUuid track_id) const noexcept
{
  const auto * snap = rt_snapshot_.load (std::memory_order_acquire);
  if (snap == nullptr)
    return std::monostate{};

  if (auto it = snap->audio.find (track_id); it != snap->audio.end ())
    return it->second;
  if (auto it = snap->midi.find (track_id); it != snap->midi.end ())
    return it->second;
  return std::monostate{};
}

void
RecordingCoordinator::prepare_for_processing (units::sample_u32_t block_length)
{
  impl_->audio.prepare_for_processing (block_length);
  impl_->midi.prepare_for_processing (block_length);
}

void
RecordingCoordinator::process_pending ()
{
  auto audio_ready = impl_->audio.drain_active ();
  auto midi_ready = impl_->midi.drain_active ();

  const bool had_pending =
    impl_->audio.has_pending_deletion () || impl_->midi.has_pending_deletion ();

  auto audio_pending = impl_->audio.drain_pending_deletion ();
  auto midi_pending = impl_->midi.drain_pending_deletion ();

  audio_ready.insert (
    audio_ready.end (), std::make_move_iterator (audio_pending.begin ()),
    std::make_move_iterator (audio_pending.end ()));
  midi_ready.insert (
    midi_ready.end (), std::make_move_iterator (midi_pending.begin ()),
    std::make_move_iterator (midi_pending.end ()));

  old_snapshots_ = std::move (pending_snapshot_deletion_);
  pending_snapshot_deletion_.clear ();

  for (const auto &[id, packets] : audio_ready)
    {
      Q_EMIT audioDataReady (id, std::move (packets));
    }
  for (const auto &[id, packets] : midi_ready)
    {
      Q_EMIT midiDataReady (id, std::move (packets));
    }

  if (impl_->audio.empty () && impl_->midi.empty ())
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
RecordingCoordinator::drain_pending_deletion (
  AudioDrainResult &audio_ready,
  MidiDrainResult  &midi_ready)
{
  const bool had_pending =
    impl_->audio.has_pending_deletion () || impl_->midi.has_pending_deletion ();

  auto audio_pending = impl_->audio.drain_pending_deletion ();
  auto midi_pending = impl_->midi.drain_pending_deletion ();

  audio_ready.insert (
    audio_ready.end (), std::make_move_iterator (audio_pending.begin ()),
    std::make_move_iterator (audio_pending.end ()));
  midi_ready.insert (
    midi_ready.end (), std::make_move_iterator (midi_pending.begin ()),
    std::make_move_iterator (midi_pending.end ()));

  return had_pending;
}

void
RecordingCoordinator::publish_snapshot ()
{
  auto new_snap = std::make_unique<RtSnapshot> ();
  impl_->audio.for_each_session (
    [&] (const auto &id, AudioRecordingSession &session) {
      new_snap->audio.emplace (id, &session);
    });
  impl_->midi.for_each_session (
    [&] (const auto &id, MidiRecordingSession &session) {
      new_snap->midi.emplace (id, &session);
    });

  auto * old =
    rt_snapshot_.exchange (new_snap.release (), std::memory_order_acq_rel);
  if (old != nullptr)
    {
      pending_snapshot_deletion_.emplace_back (
        std::unique_ptr<RtSnapshot>{ old });
    }
}

void
RecordingCoordinator::finalizeAllSessions ()
{
  AudioDrainResult audio_ready;
  MidiDrainResult  midi_ready;

  impl_->audio.finalize_all (audio_ready);
  impl_->midi.finalize_all (midi_ready);

  // Also drain pending-deletion sessions to avoid data loss for
  // recently-disarmed tracks.
  const bool had_pending = drain_pending_deletion (audio_ready, midi_ready);

  old_snapshots_ = std::move (pending_snapshot_deletion_);
  pending_snapshot_deletion_.clear ();

  for (const auto &[id, packets] : audio_ready)
    {
      Q_EMIT audioDataReady (id, std::move (packets));
    }
  for (const auto &[id, packets] : midi_ready)
    {
      Q_EMIT midiDataReady (id, std::move (packets));
    }

  // recordingSessionEnded is intentionally emitted whenever any recording
  // activity occurred — even if all tracks were disarmed before this call
  // (had_pending is true) and sessions are now empty. An arm/disarm cycle
  // constitutes a completed session lifecycle that consumers must observe.
  if (!impl_->audio.empty () || !impl_->midi.empty () || had_pending)
    {
      Q_EMIT recordingSessionEnded ();
    }
}

}
