// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui::qquick
{

/**
 * @brief QML singleton providing imperative arranger object queries.
 *
 * Thin wrapper over the pure C++ free functions in
 * @ref arrangement/arranger_object_all.h. Use this from imperative JS
 * handlers (drag, resize). For reactive bindings, use
 * @ref TimelinePositionTracker.
 */
class ArrangerObjectHelper : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  /**
   * @brief Warp-aware absolute timeline position of an arranger object.
   *
   * For clips and other timeline objects: returns the object's position.
   * For child objects inside a clip (MidiNotes, etc.): converts through the
   * parent clip's ContentTimeWarp.
   */
  Q_INVOKABLE static double
  timelineTicks (structure::arrangement::ArrangerObject * obj);

  /**
   * @brief Warp-aware absolute timeline end position.
   *
   * For objects with a length, this is the warp-adjusted end. For objects
   * without a length (e.g. Marker), returns the same as @ref timelineTicks.
   */
  Q_INVOKABLE static double
  timelineEndTicks (structure::arrangement::ArrangerObject * obj);

  /**
   * @brief Whether the object is a Clip (MidiClip, AudioClip, ChordClip, or
   * AutomationClip).
   */
  Q_INVOKABLE static bool isClip (structure::arrangement::ArrangerObject * obj);

  /**
   * @brief Sets an object's length so that its timeline end matches the given
   * timeline tick position.
   *
   * Reverse-warp converts the timeline end into content space before setting
   * the length. No-op for objects without a length.
   */
  Q_INVOKABLE static void setEndFromTimelineTicks (
    structure::arrangement::ArrangerObject * obj,
    double                                   timeline_end_ticks);
};

} // namespace zrythm::gui::qquick
