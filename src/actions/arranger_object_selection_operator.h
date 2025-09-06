// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <ranges>

#include "commands/move_arranger_objects_command.h"
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
  explicit ArrangerObjectSelectionOperator (QObject * parent = nullptr)
      : QObject (parent)
  {
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

  Q_INVOKABLE bool moveByTicks (double tick_delta)
  {
    // Validation checks
    if (undo_stack_ == nullptr)
      {
        qWarning () << "UndoStack not set for ArrangerObjectSelectionOperator";
        return false;
      }

    if (selection_model_ == nullptr)
      {
        qWarning ()
          << "SelectionModel not set for ArrangerObjectSelectionOperator";
        return false;
      }

    if (tick_delta == 0.0)
      {
        // No movement needed
        return true;
      }

    // Extract selected objects from selection model
    auto selected_objects = extractSelectedObjects ();
    if (selected_objects.empty ())
      {
        qWarning () << "No objects selected for movement";
        return false;
      }

    // Validate object bounds (don't move objects before timeline start)
    if (!validateMovement (selected_objects, tick_delta))
      {
        qWarning ()
          << "Movement validation failed - objects would be moved before timeline start";
        return false;
      }

    // Create and push command
    auto * command = new commands::MoveArrangerObjectsCommand (
      std::move (selected_objects), tick_delta);
    undo_stack_->push (command);

    return true;
  }

private:
  std::vector<structure::arrangement::ArrangerObjectUuidReference>
  extractSelectedObjects ()
  {
    std::vector<structure::arrangement::ArrangerObjectUuidReference> objects;

    if (selection_model_ == nullptr)
      {
        return objects;
      }

    const auto selected_indexes = selection_model_->selectedIndexes ();
    for (const auto &index : selected_indexes)
      {
        // Get the object from the model index
        auto variant = index.data (
          structure::arrangement::ArrangerObjectListModel::
            ArrangerObjectUuidReferenceRole);
        if (variant.canConvert<
              structure::arrangement::ArrangerObjectUuidReference *> ())
          {
            auto * obj_ref = variant.value<
              structure::arrangement::ArrangerObjectUuidReference *> ();
            objects.push_back (*obj_ref);
          }
      }

    return objects;
  }

  bool validateMovement (
    const std::vector<structure::arrangement::ArrangerObjectUuidReference>
          &objects,
    double tick_delta)
  {
    return std::ranges::all_of (objects, [tick_delta] (const auto &obj_ref) {
      if (auto * obj = obj_ref.get_object_base ())
        {
          const double new_position = obj->position ()->ticks () + tick_delta;
          return new_position >= 0.0;
        }
      return true; // Skip objects that can't be accessed
    });
  }

private:
  undo::UndoStack *     undo_stack_{};
  QItemSelectionModel * selection_model_{};
};

} // namespace zrythm::actions
