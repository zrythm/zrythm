// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/qt.h"

#include <QUndoCommand>
#include <QUndoStack>
#include <QtQmlIntegration>

#include "nlohmann/json_fwd.hpp"

namespace zrythm::undo
{

class UndoStack : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QStringList undoActions READ undoActions NOTIFY undoActionsChanged)
  Q_PROPERTY (QStringList redoActions READ redoActions NOTIFY redoActionsChanged)
  Q_PROPERTY (int index READ index WRITE setIndex NOTIFY indexChanged)
  Q_PROPERTY (bool canUndo READ canUndo NOTIFY canUndoChanged)
  Q_PROPERTY (bool canRedo READ canRedo NOTIFY canRedoChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")
public:
  using UndoCommand = QUndoCommand;

  explicit UndoStack (
    GraphStopRequester   graph_stop_requester,
    GraphResumeRequester graph_resume_requester,
    QObject *            parent = nullptr);

  // all users of the undo stack should use these wrappers instead of
  // QUndoStack's push/undo/redo. all other functionality should handed off to
  // the underlying QUndoStack
  void             push (UndoCommand * cmd);
  Q_INVOKABLE void undo ();
  Q_INVOKABLE void redo ();

  bool canUndo () const { return stack_->canUndo (); }
  bool canRedo () const { return stack_->canRedo (); }

  Q_INVOKABLE void beginMacro (const QString &text)
  {
    stack_->beginMacro (text);
  }
  Q_INVOKABLE void endMacro () { stack_->endMacro (); }

  QStringList undoActions ();
  QStringList redoActions ();

  int  index () const { return stack_->index (); }
  void setIndex (int idx);

  auto count () const { return stack_->count (); }
  auto text (int idx) const { return stack_->text (idx); }

Q_SIGNALS:
  void undoActionsChanged ();
  void redoActionsChanged ();
  void indexChanged ();
  void canUndoChanged ();
  void canRedoChanged ();

private:
  // persistence (TODO)
  static constexpr auto kStackContentKey = "stackContent"sv;
  friend void           to_json (nlohmann::json &j, const UndoStack &u);
  friend void           from_json (const nlohmann::json &j, UndoStack &u);

  /**
   * @brief Returns whether any of the command's nested children requires
   * pausing the graph.
   */
  bool command_or_children_require_graph_pause (const UndoCommand &cmd) const;

private:
  utils::QObjectUniquePtr<QUndoStack> stack_;

  // Graph operations
  GraphStopRequester   graph_stop_requester_;
  GraphResumeRequester graph_resume_requester_;
};
};
