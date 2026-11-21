// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/audio_port.h"
#include "structure/tracks/track_all.h"
#include "undo/undo_stack.h"

#include <QtQmlIntegration/qqmlintegration.h>

#include "operations/operation_dispatcher.h"

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
    zrythm::operations::OperationDispatcher * operationDispatcher READ
      operationDispatcher WRITE setOperationDispatcher NOTIFY
        operationDispatcherChanged)
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

  operations::OperationDispatcher * operationDispatcher () const
  {
    return operation_dispatcher_;
  }
  void setOperationDispatcher (operations::OperationDispatcher * dispatcher)
  {
    if (operation_dispatcher_ != dispatcher)
      {
        operation_dispatcher_ = dispatcher;
        Q_EMIT operationDispatcherChanged ();
      }
  }
  Q_SIGNAL void operationDispatcherChanged ();

  Q_INVOKABLE void rename (const QString &newName);
  Q_INVOKABLE void setColor (const QColor &color);
  Q_INVOKABLE void unsetColor ();
  Q_INVOKABLE void setComment (const QString &comment);
  Q_INVOKABLE void setOutputTrack (structure::tracks::Track * destination);

  /// Sets the track's timebase (Musical or Absolute).
  Q_INVOKABLE void setTimebase (dsp::Timebase timebase);

  /// Appends a send to the track's channel (tapping before the fader
  /// when @p isPrefader is true).
  Q_INVOKABLE void addSend (bool isPrefader);

  /// Removes a send from the track's channel.
  Q_INVOKABLE void removeSend (structure::tracks::ChannelSend * send);

  /// Moves a send's tap point before or after the channel fader.
  Q_INVOKABLE void
  setSendPrefader (structure::tracks::ChannelSend * send, bool isPrefader);

  /// Points a send at a track destination, or clears it when
  /// @p destination is null.
  Q_INVOKABLE void setSendTargetTrack (
    structure::tracks::ChannelSend * send,
    structure::tracks::Track *       destination);

  /// Points a send at a plugin sidechain input port, or clears it when
  /// @p destination is null.
  Q_INVOKABLE void setSendTargetPort (
    structure::tracks::ChannelSend * send,
    dsp::AudioPort *                 destination);

private:
  structure::tracks::Track *        track_{};
  undo::UndoStack *                 undo_stack_{};
  operations::OperationDispatcher * operation_dispatcher_{};
};
}
