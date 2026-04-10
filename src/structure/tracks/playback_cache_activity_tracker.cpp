// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/playback_cache_activity_tracker.h"

#include <QDateTime>

namespace zrythm::structure::tracks
{

PlaybackCacheActivityTracker::PlaybackCacheActivityTracker (
  utils::PlaybackCacheScheduler * scheduler,
  QObject *                       parent)
    : QObject (parent)
{
  sweep_timer_.setSingleShot (false);
  sweep_timer_.setInterval (kEntryLifetimeMs);
  QObject::connect (
    &sweep_timer_, &QTimer::timeout, this,
    &PlaybackCacheActivityTracker::sweepExpiredEntries);

  Q_ASSERT (
    this->thread () == scheduler->thread ()
    && "PlaybackCacheActivityTracker requires same thread as scheduler");

  QObject::connect (
    scheduler, &utils::PlaybackCacheScheduler::isPendingChanged, this,
    [this, scheduler] () {
      pending_ = scheduler->isPending ();
      Q_EMIT pendingChanged ();
    });

  // Sync initial state in case the scheduler is already pending
  pending_ = scheduler->isPending ();
}

QVariantList
PlaybackCacheActivityTracker::entries () const
{
  QVariantList result;
  result.reserve (static_cast<int> (entries_.size ()));
  for (const auto &e : entries_)
    result.append (QVariant::fromValue (e));
  return result;
}

void
PlaybackCacheActivityTracker::onRegenerationComplete (
  utils::ExpandableTickRange affectedRange)
{
  PlaybackCacheActivityEntry entry;
  entry.id = next_id_++;
  entry.createdAtMs =
    std::chrono::milliseconds (QDateTime::currentMSecsSinceEpoch ());
  if (affectedRange.is_full_content ())
    {
      entry.isFullContent = true;
    }
  else
    {
      auto range = affectedRange.range ().value ();
      entry.startTick = range.first;
      entry.endTick = range.second;
    }

  entries_.push_back (entry);

  if (entries_.size () > kMaxEntries)
    entries_.erase (entries_.begin (), entries_.end () - kMaxEntries);

  Q_EMIT entriesChanged ();

  // Start the sweep timer on first entry
  if (!sweep_timer_.isActive ())
    sweep_timer_.start ();
}

void
PlaybackCacheActivityTracker::sweepExpiredEntries ()
{
  const auto now =
    std::chrono::milliseconds (QDateTime::currentMSecsSinceEpoch ());
  const auto old_size = entries_.size ();

  std::erase_if (entries_, [now] (const PlaybackCacheActivityEntry &e) {
    return (now - e.createdAtMs) >= kEntryLifetimeMs;
  });

  if (entries_.size () != old_size)
    Q_EMIT entriesChanged ();

  // Stop the timer when no entries remain
  if (entries_.empty ())
    sweep_timer_.stop ();
}

} // namespace zrythm::structure::tracks
