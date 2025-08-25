// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/qt.h"

#include <QUndoStack>
#include <QtQmlIntegration>

#include "nlohmann/json.hpp"

namespace zrythm::undo
{

class UndoStack : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QUndoStack * undoStack READ undoStack CONSTANT)
  Q_PROPERTY (QStringList undoActions READ undoActions NOTIFY undoActionsChanged)
  Q_PROPERTY (QStringList redoActions READ redoActions NOTIFY redoActionsChanged)
  Q_PROPERTY (int index READ index WRITE setIndex NOTIFY indexChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")
public:
  explicit UndoStack (QObject * parent = nullptr);

  QUndoStack * undoStack () const { return stack_.get (); }

  // all users of the undo stack should use these wrappers instead of
  // QUndoStack's push/undo/redo. all other functionality should handed off to
  // the underlying QUndoStack
  void             push (QUndoCommand * cmd);
  Q_INVOKABLE void undo ();
  Q_INVOKABLE void redo ();

  QStringList undoActions ();
  QStringList redoActions ();

  int  index () const { return stack_->index (); }
  void setIndex (int idx);

Q_SIGNALS:
  void undoActionsChanged ();
  void redoActionsChanged ();
  void indexChanged ();

private:
  // persistence (TODO)
  static constexpr auto kStackContentKey = "stackContent"sv;
  friend void           to_json (nlohmann::json &j, const UndoStack &u);
  friend void           from_json (const nlohmann::json &j, UndoStack &u);

private:
  utils::QObjectUniquePtr<QUndoStack> stack_;
};
};
