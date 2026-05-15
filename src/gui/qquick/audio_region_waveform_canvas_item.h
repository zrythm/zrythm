// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>

#include "gui/qquick/waveform_canvas_item.h"

#include <QPointer>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::structure::arrangement
{
class AudioRegion;
}

namespace zrythm::gui::qquick
{

/**
 * @brief AudioRegion-specific waveform canvas.
 *
 * Handles AudioRegion buffer serialization and re-serialization
 * on region property changes (loop, bounds, fades).
 *
 * Uses a cached snapshot of region properties to avoid redundant
 * re-serializations when multiple signals fire for the same change.
 */
class AudioRegionWaveformCanvasItem : public WaveformCanvasItem
{
  Q_OBJECT
  QML_NAMED_ELEMENT (AudioRegionWaveformCanvas)

  Q_PROPERTY (QObject * region READ region WRITE setRegion NOTIFY regionChanged)

public:
  explicit AudioRegionWaveformCanvasItem (QQuickItem * parent = nullptr);

  QObject * region () const { return region_; }
  void      setRegion (QObject * region);

Q_SIGNALS:
  void regionChanged ();

private:
  /**
   * @brief Snapshot of region properties that affect the waveform.
   *
   * Compared against the current state to determine what changed and
   * whether an incremental append or full re-serialize is needed.
   */
  struct RegionSnapshot
  {
    int64_t clip_start_samples{};
    int64_t loop_start_samples{};
    int64_t loop_end_samples{};
    int64_t length_samples{};
    int64_t fade_in_samples{};
    int64_t fade_out_samples{};
    float   gain{ 1.0f };

    friend bool
    operator== (const RegionSnapshot &, const RegionSnapshot &) = default;
  };

  RegionSnapshot take_snapshot () const;

  void handle_property_change ();

  QObject *                                     region_ = nullptr;
  QPointer<structure::arrangement::AudioRegion> audio_region_;
  std::vector<QMetaObject::Connection>          region_connections_;
  RegionSnapshot                                last_snapshot_;
};

} // namespace zrythm::gui::qquick
