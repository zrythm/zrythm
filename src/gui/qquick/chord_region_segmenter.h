// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>
#include <vector>

#include <QObject>
#include <QPointer>
#include <QRangeModel>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::structure::arrangement
{
class ChordRegion;
class ChordObject;
}

namespace zrythm::gui::qquick
{

/**
 * @brief POD gadget carrying one (chord × visible loop iteration) tuple.
 *
 * Each instance represents a single chord as it should be drawn within the
 * region's timeline bounds. When a region is looped, a single source chord
 * may produce multiple ChordSegment instances (one per visible iteration).
 *
 * Exposed to QML as roles of the segmenter's model. Property names match
 * role names automatically (QRangeModel maps Q_PROPERTY names to roles
 * when MultiRoleItem row category is set).
 */
class ChordSegment
{
  Q_GADGET
  Q_PROPERTY (double absStartTicks MEMBER abs_start_ticks)
  Q_PROPERTY (double absEndTicks MEMBER abs_end_ticks)
  Q_PROPERTY (
    zrythm::structure::arrangement::ChordObject * chordObject MEMBER chord_object)
  Q_PROPERTY (int chordIndex MEMBER chord_index)

public:
  /** Absolute start position within the region, in ticks. */
  double abs_start_ticks = 0;

  /**
   * Absolute end position within the region, in ticks.
   *
   * Either the next chord's start (within the same loop iteration) or the
   * iteration's boundary, whichever comes first.
   */
  double abs_end_ticks = 0;

  /** Pointer to the source chord object. Not null for valid segments. */
  zrythm::structure::arrangement::ChordObject * chord_object = nullptr;

  /**
   * Index of the chord in the region's chordObjects list (insertion order,
   * not sorted order). Useful for stable identity across recalculations.
   */
  int chord_index = 0;
};

} // namespace zrythm::gui::qquick

/** Tell QRangeModel to expose each Q_PROPERTY of ChordSegment as a separate
 * role (not as separate columns). Must be at global scope because
 * QRangeModel is global. */
template <> struct QRangeModel::RowOptions<zrythm::gui::qquick::ChordSegment>
{
  static constexpr auto rowCategory = QRangeModel::RowCategory::MultiRoleItem;
};

namespace zrythm::gui::qquick
{

/**
 * @brief Reactive QML type that observes a ChordRegion and exposes a
 * QAbstractItemModel of ChordSegment rows.
 *
 * Each row represents one chord as it should be drawn within the region's
 * timeline bounds, taking loop iterations into account.
 *
 * On any relevant input change (chord added/removed/moved, loop config
 * changed, region resized), recalculate() rebuilds the segment list, a new
 * QRangeModel is constructed (moving the data in), and segmentsChanged() is
 * emitted.
 *
 * Loop-expansion algorithm is ported from
 * @ref midi_region_canvas_renderer.cpp and shares the same semantics as
 * MIDI region rendering.
 */
class ChordRegionSegmenter : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::structure::arrangement::ChordRegion * region READ region WRITE
      setRegion NOTIFY regionChanged REQUIRED)
  Q_PROPERTY (QAbstractItemModel * segments READ segments NOTIFY segmentsChanged)
  QML_ELEMENT

public:
  explicit ChordRegionSegmenter (QObject * parent = nullptr);

  zrythm::structure::arrangement::ChordRegion * region () const
  {
    return region_;
  }
  void setRegion (zrythm::structure::arrangement::ChordRegion * region);

  QAbstractItemModel * segments () const { return segments_model_.get (); }

  Q_SIGNAL void regionChanged ();
  Q_SIGNAL void segmentsChanged ();

private:
  /** Disconnects all stored connections from the current region and clears
   * the connection list. */
  void disconnectAll ();

  /** Connects to all relevant signals on the current region. Called after
   * region_ is assigned. */
  void connectToRegion ();

  /**
   * Rebuilds the segment list from the region's current state, creates a
   * new QRangeModel wrapping it, and emits segmentsChanged().
   */
  void recalculate ();

  QPointer<zrythm::structure::arrangement::ChordRegion> region_;

  /** Owned vector of segments; moved into segments_model_ on each
   * recalculate. */
  std::vector<ChordSegment> segments_data_;

  /** Owned QRangeModel wrapping segments_data_. Recreated on each
   * recalculate. */
  std::unique_ptr<QRangeModel> segments_model_;

  /** Active QMetaObject::Connections so we can disconnect cleanly when the
   * region changes. */
  std::vector<QMetaObject::Connection> connections_;
};

} // namespace zrythm::gui::qquick
