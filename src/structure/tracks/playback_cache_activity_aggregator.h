// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <unordered_map>

#include "structure/tracks/track_collection.h"

#include <QMetaObject>
#include <QTimer>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::structure::tracks
{

/**
 * @brief Aggregates playback cache activity counts across all tracks and
 * automation tracks in a TrackCollection.
 *
 * Connects to the collection's rowsInserted/rowsAboutToBeRemoved signals and
 * monitors each track's (and automation track's) PlaybackCacheActivityTracker
 * to provide total pending and complete counts as QML-bindable properties.
 *
 * Signal connections use `this` as the Qt context object, so they auto-clean
 * when either the aggregator or the sender (tracker/ATL) is destroyed.
 * The handlers early-return if the tracker is not in tracker_states_, making
 * stale connections harmless. This avoids the need for manual disconnect
 * bookkeeping.
 *
 * This is a debug-only utility.
 */
class PlaybackCacheActivityAggregator : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::structure::tracks::TrackCollection * collection READ collection
      WRITE setCollection NOTIFY collectionChanged)
  Q_PROPERTY (
    int cachePendingCount READ cachePendingCount NOTIFY cachePendingCountChanged)
  Q_PROPERTY (
    int cacheCompleteCount READ cacheCompleteCount NOTIFY
      cacheCompleteCountChanged)
  QML_ELEMENT

public:
  explicit PlaybackCacheActivityAggregator (QObject * parent = nullptr);
  ~PlaybackCacheActivityAggregator () noexcept override;

  TrackCollection * collection () const;
  void              setCollection (TrackCollection * collection);
  Q_SIGNAL void     collectionChanged ();

  int           cachePendingCount () const;
  Q_SIGNAL void cachePendingCountChanged ();

  // cacheCompleteCountChanged is debounced (100ms) to avoid flooding QML
  // during rapid cache regeneration. cachePendingCountChanged fires
  // immediately for real-time responsiveness.
  int           cacheCompleteCount () const;
  Q_SIGNAL void cacheCompleteCountChanged ();

private:
  void connectToTracker (PlaybackCacheActivityTracker * tracker);
  void disconnectFromTracker (PlaybackCacheActivityTracker * tracker);
  void disconnectAll ();

  void connectToTrack (Track * track);
  void disconnectFromTrack (Track * track);

  void updateCachePendingCount (
    PlaybackCacheActivityTracker * tracker,
    bool                           new_pending);
  void updateCacheCompleteCount (
    PlaybackCacheActivityTracker * tracker,
    int                            new_count);

  TrackCollection * collection_ = nullptr;

  struct TrackerState
  {
    bool is_pending = false;
    int  entry_count = 0;
  };
  std::unordered_map<PlaybackCacheActivityTracker *, TrackerState>
    tracker_states_;

  int cache_pending_count_ = 0;
  int cache_complete_count_ = 0;

  QTimer cache_complete_count_timer_{ this };

  QMetaObject::Connection rows_inserted_conn_;
  QMetaObject::Connection rows_about_to_be_removed_conn_;
  QMetaObject::Connection model_about_to_be_reset_conn_;
};

} // namespace zrythm::structure::tracks
