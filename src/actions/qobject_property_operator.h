// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <stdexcept>

#include "undo/undo_stack.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::actions
{
class QObjectPropertyOperator : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    QObject * currentObject READ currentObject WRITE setCurrentObject NOTIFY
      currentObjectChanged)
  Q_PROPERTY (
    zrythm::undo::UndoStack * undoStack READ undoStack WRITE setUndoStack NOTIFY
      undoStackChanged)
  QML_ELEMENT

public:
  explicit QObjectPropertyOperator (QObject * parent = nullptr)
      : QObject (parent)
  {
  }

  Q_SIGNAL void currentObjectChanged ();
  Q_SIGNAL void undoStackChanged ();

  QObject * currentObject () const { return current_object_; }
  void      setCurrentObject (QObject * object)
  {
    if (object == nullptr)
      {
        throw std::invalid_argument ("Object cannot be null");
      }
    if (current_object_ != object)
      {
        current_object_ = object;
        Q_EMIT currentObjectChanged ();
      }
  }

  undo::UndoStack * undoStack () const { return undo_stack_; }
  void              setUndoStack (undo::UndoStack * undoStack)
  {
    if (undoStack == nullptr)
      {
        throw std::invalid_argument ("UndoStack cannot be null");
      }
    if (undo_stack_ != undoStack)
      {
        undo_stack_ = undoStack;
        Q_EMIT undoStackChanged ();
      }
  }

  Q_INVOKABLE void setValue (QString propertyName, QVariant value);

  /**
   * @brief Set a value that affects the tempo map.
   *
   * The audio engine will be stopped while changes to this property are made.
   */
  Q_INVOKABLE void
  setValueAffectingTempoMap (QString propertyName, QVariant value);

private:
  QObject *         current_object_{};
  undo::UndoStack * undo_stack_{};
};
}
