// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track_all.h"
#include "structure/tracks/track_routing.h"
#include "undo/undo_stack.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::actions
{
class TrackOperator : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::structure::tracks::Track * track READ track WRITE setTrack NOTIFY
      trackChanged)
  Q_PROPERTY (
    zrythm::undo::UndoStack * undoStack READ undoStack WRITE setUndoStack NOTIFY
      undoStackChanged)
  Q_PROPERTY (
    zrythm::structure::tracks::TrackRouting * trackRouting READ trackRouting
      WRITE setTrackRouting NOTIFY trackRoutingChanged)
  QML_ELEMENT

public:
  explicit TrackOperator (QObject * parent = nullptr) : QObject (parent) { }

  structure::tracks::Track * track () const { return track_; }
  void                       setTrack (structure::tracks::Track * track)
  {
    if (track_ != track)
      {
        track_ = track;
        Q_EMIT trackChanged ();
      }
  }
  Q_SIGNAL void trackChanged ();

  undo::UndoStack * undoStack () const { return undo_stack_; }
  void              setUndoStack (undo::UndoStack * undoStack)
  {
    if (undo_stack_ != undoStack)
      {
        undo_stack_ = undoStack;
        Q_EMIT undoStackChanged ();
      }
  }
  Q_SIGNAL void undoStackChanged ();

  structure::tracks::TrackRouting * trackRouting () const
  {
    return track_routing_;
  }
  void setTrackRouting (structure::tracks::TrackRouting * routing)
  {
    if (track_routing_ != routing)
      {
        track_routing_ = routing;
        Q_EMIT trackRoutingChanged ();
      }
  }
  Q_SIGNAL void trackRoutingChanged ();

  Q_INVOKABLE void rename (const QString &newName);
  Q_INVOKABLE void setColor (const QColor &color);
  Q_INVOKABLE void setComment (const QString &comment);
  Q_INVOKABLE void setOutputTrack (structure::tracks::Track * destination);

private:
  structure::tracks::Track *        track_{};
  undo::UndoStack *                 undo_stack_{};
  structure::tracks::TrackRouting * track_routing_{};
};
}
