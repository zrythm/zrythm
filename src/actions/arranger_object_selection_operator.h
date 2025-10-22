// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"
#include "undo/undo_stack.h"

#include <QItemSelectionModel>
#include <QtQmlIntegration>

namespace zrythm::actions
{
class ArrangerObjectSelectionOperator : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::undo::UndoStack * undoStack READ undoStack WRITE setUndoStack NOTIFY
      undoStackChanged)
  Q_PROPERTY (
    QItemSelectionModel * selectionModel READ selectionModel WRITE
      setSelectionModel NOTIFY selectionModelChanged)
  QML_ELEMENT

public:
  using SelectedObjectsVector =
    std::vector<structure::arrangement::ArrangerObjectUuidReference>;

  explicit ArrangerObjectSelectionOperator (QObject * parent = nullptr);

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
  Q_SIGNAL void undoStackChanged ();

  QItemSelectionModel * selectionModel () const { return selection_model_; }
  void                  setSelectionModel (QItemSelectionModel * selectionModel)
  {
    if (selectionModel == nullptr)
      {
        throw std::invalid_argument ("SelectionModel cannot be null");
      }
    if (selection_model_ != selectionModel)
      {
        selection_model_ = selectionModel;
        Q_EMIT selectionModelChanged ();
      }
  }
  Q_SIGNAL void selectionModelChanged ();

  Q_INVOKABLE bool moveByTicks (double tick_delta);

  Q_INVOKABLE bool moveNotesByPitch (int pitch_delta);

  Q_INVOKABLE bool moveAutomationPointsByDelta (double delta);

private:
  auto extractSelectedObjects () const -> SelectedObjectsVector;

  static bool validateHorizontalMovement (
    const SelectedObjectsVector &objects,
    double                       tick_delta);
  static bool
  validateVerticalMovement (const SelectedObjectsVector &objects, double delta);

  bool process_vertical_move (double delta);

private:
  undo::UndoStack *     undo_stack_{};
  QItemSelectionModel * selection_model_{};
};

} // namespace zrythm::actions
