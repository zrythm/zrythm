// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

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
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  qint64                    id{};
  double                    startTick{};
  double                    endTick{};
  bool                      isFullContent{};
  std::chrono::milliseconds createdAtMs;
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
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  static constexpr auto kMaxEntries = 100zu;
  static constexpr auto kEntryLifetimeMs = std::chrono::milliseconds (300);

  /**
   * @param scheduler The playback cache scheduler to track.
   */
  explicit PlaybackCacheActivityTracker (
    utils::PlaybackCacheScheduler * scheduler,
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

Q_SIGNALS:
  void pendingChanged ();
  void entriesChanged ();

private:
  void sweepExpiredEntries ();

  bool                                    pending_ = false;
  std::vector<PlaybackCacheActivityEntry> entries_;
  qint64                                  next_id_ = 0;

  QTimer sweep_timer_;
};

} // namespace zrythm::structure::tracks

Q_DECLARE_METATYPE (zrythm::structure::tracks::PlaybackCacheActivityEntry)
