// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>
#include <vector>

#include "dsp/timeline_data_cache.h"
#include "utils/playback_cache_scheduler.h"

#include <QTimer>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::structure::tracks
{

/**
 * @brief A single completed cache regeneration entry for debug visualization.
 */
struct PlaybackCacheActivityEntry
{
  Q_GADGET
  Q_PROPERTY (qint64 id MEMBER id)
  Q_PROPERTY (double startTick MEMBER startTick)
  Q_PROPERTY (double endTick MEMBER endTick)
  Q_PROPERTY (bool isFullContent MEMBER isFullContent)
  QML_VALUE_TYPE (playbackCacheActivityEntry)
  QML_UNCREATABLE ("")

public:
  qint64                    id{};
  double                    startTick{};
  double                    endTick{};
  bool                      isFullContent{};
  std::chrono::milliseconds createdAtMs;
};

/**
 * @brief A tick range representing currently valid cached content.
 */
struct CachedTickRange
{
  Q_GADGET
  Q_PROPERTY (double startTick MEMBER startTick)
  Q_PROPERTY (double endTick MEMBER endTick)
  QML_VALUE_TYPE (cachedTickRange)
  QML_UNCREATABLE ("")

public:
  double startTick{};
  double endTick{};
};

/**
 * @brief Helper that manages cache activity state for a cache-holding object.
 *
 * Encapsulates the shared logic used by both Track and AutomationTrack
 * for tracking pending cache requests and completed regeneration entries.
 *
 * The pending state is driven solely by the scheduler's isPendingChanged
 * signal. The entries list is capped at kMaxEntries to prevent unbounded
 * growth. Entries are automatically removed after kEntryLifetimeMs by a
 * periodic sweep timer.
 */
class PlaybackCacheActivityTracker : public QObject
{
  Q_OBJECT
  Q_PROPERTY (bool pending READ isPending NOTIFY pendingChanged)
  Q_PROPERTY (QVariantList entries READ entries NOTIFY entriesChanged)
  Q_PROPERTY (
    QVariantList cachedRanges READ cachedRanges NOTIFY cachedRangesChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  static constexpr auto kMaxEntries = 100zu;
  static constexpr auto kEntryLifetimeMs = std::chrono::milliseconds (300);

  /**
   * @brief Function type for converting sample position to tick position.
   */
  using SampleToTickConverter = std::function<double (units::sample_t)>;

  /**
   * @param scheduler The playback cache scheduler to track.
   * @param cache The timeline data cache to observe for range changes.
   * @param sample_to_tick Function that converts sample positions to tick
   * positions.
   * @param parent Parent QObject.
   */
  explicit PlaybackCacheActivityTracker (
    utils::PlaybackCacheScheduler * scheduler,
    const dsp::TimelineDataCache   &cache,
    SampleToTickConverter           sample_to_tick,
    QObject *                       parent = nullptr);

  /**
   * @brief Called when cache regeneration completes.
   *
   * Adds an entry to the entries list and trims to kMaxEntries.
   * Does NOT modify pending state -- that is driven solely by isPendingChanged.
   */
  void onRegenerationComplete (utils::ExpandableTickRange affectedRange);

  [[nodiscard]] bool         isPending () const { return pending_; }
  [[nodiscard]] QVariantList entries () const;
  [[nodiscard]] size_t       entryCount () const { return entries_.size (); }
  [[nodiscard]] QVariantList cachedRanges () const;

Q_SIGNALS:
  void pendingChanged ();
  void entriesChanged ();
  void cachedRangesChanged ();

private:
  void sweepExpiredEntries ();

  bool                                    pending_ = false;
  std::vector<PlaybackCacheActivityEntry> entries_;
  std::vector<CachedTickRange>            cached_ranges_;
  qint64                                  next_id_ = 0;

  QTimer sweep_timer_;
};

} // namespace zrythm::structure::tracks

Q_DECLARE_METATYPE (zrythm::structure::tracks::PlaybackCacheActivityEntry)
Q_DECLARE_METATYPE (zrythm::structure::tracks::CachedTickRange)
