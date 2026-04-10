// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>

#include "structure/tracks/automation_track.h"
#include "structure/tracks/automation_tracklist.h"
#include "structure/tracks/playback_cache_activity_aggregator.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_collection.h"

namespace zrythm::structure::tracks
{

using namespace std::chrono_literals;

PlaybackCacheActivityAggregator::PlaybackCacheActivityAggregator (
  QObject * parent)
    : QObject (parent)
{
  cache_complete_count_timer_.setSingleShot (true);
  cache_complete_count_timer_.setInterval (100ms);
  QObject::connect (
    &cache_complete_count_timer_, &QTimer::timeout, this,
    &PlaybackCacheActivityAggregator::cacheCompleteCountChanged);
}

PlaybackCacheActivityAggregator::~PlaybackCacheActivityAggregator () noexcept
{
  // Connections auto-clean via Qt context-object mechanism.
  // Just reset state in case a signal fires during destruction.
  tracker_states_.clear ();
  cache_pending_count_ = 0;
  cache_complete_count_ = 0;
}

TrackCollection *
PlaybackCacheActivityAggregator::collection () const
{
  return collection_;
}

void
PlaybackCacheActivityAggregator::setCollection (TrackCollection * collection)
{
  if (collection_ == collection)
    return;

  // Full teardown of previous collection
  QObject::disconnect (rows_inserted_conn_);
  QObject::disconnect (rows_about_to_be_removed_conn_);
  QObject::disconnect (model_about_to_be_reset_conn_);
  disconnectAll ();
  collection_ = collection;
  Q_EMIT collectionChanged ();

  if (collection_ == nullptr)
    return;

  // Connect to track additions/removals
  rows_inserted_conn_ = QObject::connect (
    collection_, &TrackCollection::rowsInserted, this,
    [this] (const QModelIndex &, int first, int last) {
      for (int i = first; i <= last; ++i)
        {
          const auto &track_ref = collection_->tracks ().at (i);
          auto *      track = track_ref.get_object_base ();
          connectToTrack (track);
        }
    });

  rows_about_to_be_removed_conn_ = QObject::connect (
    collection_, &TrackCollection::rowsAboutToBeRemoved, this,
    [this] (const QModelIndex &, int first, int last) {
      for (int i = first; i <= last; ++i)
        {
          const auto &track_ref = collection_->tracks ().at (i);
          auto *      track = track_ref.get_object_base ();
          disconnectFromTrack (track);
        }
    });

  // Handle clear() which uses beginResetModel/endResetModel
  model_about_to_be_reset_conn_ = QObject::connect (
    collection_, &QAbstractItemModel::modelAboutToBeReset, this,
    &PlaybackCacheActivityAggregator::disconnectAll);

  // Connect to existing tracks
  for (const auto &track_ref : collection_->tracks ())
    {
      auto * track = track_ref.get_object_base ();
      connectToTrack (track);
    }
}

int
PlaybackCacheActivityAggregator::cachePendingCount () const
{
  return cache_pending_count_;
}

int
PlaybackCacheActivityAggregator::cacheCompleteCount () const
{
  return cache_complete_count_;
}

void
PlaybackCacheActivityAggregator::connectToTrack (Track * track)
{
  // Connect to the track's tracker (may be null for non-lanes tracks)
  if (auto * tracker = track->playbackCacheActivityTracker ())
    {
      connectToTracker (tracker);
    }

  // Also connect to the automation tracklist if present
  if (auto * atl = track->automationTracklist ())
    {
      // Connect existing automation trackers
      for (const auto &at : atl->automation_tracks ())
        {
          connectToTracker (at->playbackCacheActivityTracker ());
        }

      // Watch for new/removed automation tracks.
      // Connections use `this` as context so they auto-clean when either
      // the aggregator or the ATL (sender) is destroyed.
      QObject::connect (
        atl, &AutomationTracklist::rowsInserted, this,
        [this, atl] (const QModelIndex &, int first, int last) {
          for (int i = first; i <= last; ++i)
            {
              auto * at = atl->automation_track_at (i);
              connectToTracker (at->playbackCacheActivityTracker ());
            }
        });

      QObject::connect (
        atl, &AutomationTracklist::rowsAboutToBeRemoved, this,
        [this, atl] (const QModelIndex &, int first, int last) {
          for (int i = first; i <= last; ++i)
            {
              auto * at = atl->automation_track_at (i);
              disconnectFromTracker (at->playbackCacheActivityTracker ());
            }
        });
    }
}

void
PlaybackCacheActivityAggregator::disconnectFromTrack (Track * track)
{
  // Subtract counts for automation tracks
  if (auto * atl = track->automationTracklist ())
    {
      for (const auto &at : atl->automation_tracks ())
        {
          disconnectFromTracker (at->playbackCacheActivityTracker ());
        }
    }

  // Subtract counts for the track's own tracker
  if (auto * tracker = track->playbackCacheActivityTracker ())
    {
      disconnectFromTracker (tracker);
    }
}

void
PlaybackCacheActivityAggregator::connectToTracker (
  PlaybackCacheActivityTracker * tracker)
{
  if (tracker_states_.contains (tracker))
    return;

  // Connections use `this` as context, so they auto-clean when either the
  // aggregator or the tracker (sender) is destroyed. The handlers early-return
  // if the tracker is not in tracker_states_, making stale connections
  // harmless and avoiding the need for manual disconnect bookkeeping.
  QObject::connect (
    tracker, &PlaybackCacheActivityTracker::pendingChanged, this,
    [this, tracker] () {
      updateCachePendingCount (tracker, tracker->isPending ());
    });
  QObject::connect (
    tracker, &PlaybackCacheActivityTracker::entriesChanged, this,
    [this, tracker] () {
      updateCacheCompleteCount (
        tracker, static_cast<int> (tracker->entryCount ()));
    });

  tracker_states_[tracker] = {};
}

void
PlaybackCacheActivityAggregator::disconnectFromTracker (
  PlaybackCacheActivityTracker * tracker)
{
  auto it = tracker_states_.find (tracker);
  if (it == tracker_states_.end ())
    return;

  // Subtract counts
  if (it->second.is_pending)
    --cache_pending_count_;
  cache_complete_count_ -= it->second.entry_count;

  assert (cache_pending_count_ >= 0);
  assert (cache_complete_count_ >= 0);

  // No manual disconnect needed — connections auto-clean when the tracker
  // (sender) is destroyed, and the handlers early-return if not in the map.
  tracker_states_.erase (it);

  Q_EMIT cachePendingCountChanged ();
  Q_EMIT cacheCompleteCountChanged ();
}

void
PlaybackCacheActivityAggregator::disconnectAll ()
{
  // No manual disconnect needed — connections auto-clean when trackers/ATLs
  // are destroyed. Handlers early-return if the tracker is not in the map.
  tracker_states_.clear ();

  cache_pending_count_ = 0;
  cache_complete_count_ = 0;

  Q_EMIT cachePendingCountChanged ();
  Q_EMIT cacheCompleteCountChanged ();
}

void
PlaybackCacheActivityAggregator::updateCachePendingCount (
  PlaybackCacheActivityTracker * tracker,
  bool                           new_pending)
{
  auto it = tracker_states_.find (tracker);
  if (it == tracker_states_.end ())
    return;

  if (new_pending == it->second.is_pending)
    return;

  it->second.is_pending = new_pending;
  cache_pending_count_ += new_pending ? 1 : -1;
  Q_EMIT cachePendingCountChanged ();
}

void
PlaybackCacheActivityAggregator::updateCacheCompleteCount (
  PlaybackCacheActivityTracker * tracker,
  int                            new_count)
{
  auto it = tracker_states_.find (tracker);
  if (it == tracker_states_.end ())
    return;

  const int old_count = it->second.entry_count;
  it->second.entry_count = new_count;
  cache_complete_count_ += new_count - old_count;
  cache_complete_count_timer_.start ();
}

} // namespace zrythm::structure::tracks
