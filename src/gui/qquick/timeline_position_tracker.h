// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "structure/arrangement/arranger_object.h"

#include <QObject>
#include <QPointer>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui::qquick
{

/**
 * @brief Reactive QML component for warp-aware timeline positions.
 *
 * Wrap an ArrangerObject and bind to @ref timelineTicks or
 * @ref timelineEndTicks in QML. The properties auto-update when the object's
 * position, length, parent clip, or parent clip's ContentTimeWarp changes.
 *
 * For imperative one-shot queries, use @ref ArrangerObjectHelper instead.
 *
 * @code{.qml}
 * TimelinePositionTracker {
 *   id: tracker
 *   arrangerObject: myNote
 * }
 * Text { text: tracker.timelineTicks }
 * @endcode
 */
class TimelinePositionTracker : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY (
    zrythm::structure::arrangement::ArrangerObject * arrangerObject READ
      arrangerObject WRITE setArrangerObject NOTIFY arrangerObjectChanged)
  Q_PROPERTY (
    double timelineTicks READ timelineTicks NOTIFY timelineTicksChanged)
  Q_PROPERTY (
    double timelineEndTicks READ timelineEndTicks NOTIFY timelineEndTicksChanged)

public:
  explicit TimelinePositionTracker (QObject * parent = nullptr);

  structure::arrangement::ArrangerObject * arrangerObject () const
  {
    return obj_;
  }
  void setArrangerObject (structure::arrangement::ArrangerObject * obj);

  double timelineTicks () const;
  double timelineEndTicks () const;

Q_SIGNALS:
  void arrangerObjectChanged ();
  void timelineTicksChanged ();
  void timelineEndTicksChanged ();

private:
  void disconnect_all ();
  void reconnect ();

  QPointer<structure::arrangement::ArrangerObject> obj_;
  std::vector<QMetaObject::Connection>             connections_;
};

} // namespace zrythm::gui::qquick
